// Copyright (c) 2016-2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file generated from indigo_ccd_ssag.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <stdatomic.h>

#include "indigo_ccd_ssag_firmware.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_ssag.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000D
#define DRIVER_NAME          "indigo_ccd_ssag"
#define DRIVER_LABEL         "SSAG/QHY5 Camera"
#define CCD_DEVICE_NAME      "SSAG%s"
#define GUIDER_DEVICE_NAME   "SSAG (guider)%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((ssag_private_data *)device->private_data)

//+ define

#define CPUCS_ADDRESS        0xe600
#define USB_TIMEOUT          5000
#define BUFFER_ENDPOINT      0x82
#define IMAGE_WIDTH          1280
#define IMAGE_HEIGHT         1024
#define HORIZONTAL_BLANKING  244
#define VERTICAL_BLANKING    25
#define BUFFER_WIDTH         (IMAGE_WIDTH + HORIZONTAL_BLANKING)
#define BUFFER_HEIGHT        (IMAGE_HEIGHT + VERTICAL_BLANKING + 1)
#define BUFFER_SIZE          (BUFFER_WIDTH * BUFFER_HEIGHT)
#define ROW_START            12
#define COLUMN_START         20
#define SHUTTER_WIDTH        (IMAGE_HEIGHT + VERTICAL_BLANKING)
#define PIXEL_OFFSET         (8 * (BUFFER_WIDTH + 31))
#define SSAG_VENDOR_ID       0x1856
#define SSAG_PRODUCT_ID      0x0012
#define SSAG_LOADER_VENDOR_ID 0x1856
#define SSAG_LOADER_PRODUCT_ID 0x0011
#define QHY5_LOADER_VENDOR_ID 0x1618
#define QHY5_LOADER_PRODUCT_ID 0x0901
#define OTI_LOADER_VENDOR_ID 0x16c0
#define OTI_LOADER_PRODUCT_ID 0x296d

//- define

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	//+ data
	libusb_device_handle *handle;
	unsigned char gain;
	unsigned char *buffer;
	bool interface_claimed;
	atomic_int exposure_state;
	bool ra_guiding;
	bool dec_guiding;
	atomic_bool ra_replacement;
	atomic_bool dec_replacement;
	//- data
} ssag_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

typedef enum {
	GUIDE_EAST = 0x10,
	GUIDE_SOUTH = 0x20,
	GUIDE_NORTH = 0x40,
	GUIDE_WEST = 0x80
} guide_direction;

typedef enum {
	USB_RQ_LOAD_FIRMWARE = 0xa0,
	USB_RQ_WRITE_SMALL_EEPROM = 0xa2
} firmware_request;

typedef enum {
	USB_RQ_GUIDE = 16,
	USB_RQ_EXPOSE = 18,
	USB_RQ_SET_INIT_PACKET = 19,
	USB_RQ_PRE_EXPOSE = 20,
	USB_RQ_CANCEL_GUIDE_EAST_WEST = 33,
	USB_RQ_CANCEL_GUIDE_NORTH_SOUTH = 34,
	USB_RQ_SET_BUFFER_MODE = 85
} usb_request;

typedef enum {
	EXPOSURE_IDLE,
	EXPOSURE_PENDING,
	EXPOSURE_STARTED,
	EXPOSURE_CANCELLED
} exposure_state;

static unsigned char bootloader[] = { SSAG_BOOTLOADER };
static unsigned char firmware[] = { SSAG_FIRMWARE };
static int custom_vid;
static int custom_pid;

static int ssag_reset_mode(libusb_device_handle *handle, unsigned char data) {
	int rc = libusb_control_transfer(handle, 0x40, USB_RQ_LOAD_FIRMWARE, 0x7f92, 0, &data, 1, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc == 1) {
		rc = libusb_control_transfer(handle, 0x40, USB_RQ_LOAD_FIRMWARE, CPUCS_ADDRESS, 0, &data, 1, USB_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	return rc == 1 ? LIBUSB_SUCCESS : rc < 0 ? rc : LIBUSB_ERROR_IO;
}

static int ssag_upload(libusb_device_handle *handle, const unsigned char *data) {
	for (;;) {
		unsigned char byte_count = data[0];
		if (byte_count == 0) {
			return LIBUSB_SUCCESS;
		}
		uint16_t address = (uint16_t)(data[1] | (data[2] << 8));
		int rc = libusb_control_transfer(handle, 0x40, USB_RQ_LOAD_FIRMWARE, address, 0, (unsigned char *)(data + 3), byte_count, USB_TIMEOUT);
		if (rc != byte_count) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "short transfer");
			return rc < 0 ? rc : LIBUSB_ERROR_IO;
		}
		data += byte_count + 3;
	}
}

static void *ssag_firmware(void *data) {
	libusb_device *dev = (libusb_device *)data;
	libusb_device_handle *handle = NULL;
	int rc = libusb_open(dev, &handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		rc = ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_upload(handle, bootloader);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x00);
		if (rc >= 0) {
			indigo_sleep(1);
		}
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_upload(handle, firmware);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x00);
		libusb_close(handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
	}
	libusb_unref_device(dev);
	return NULL;
}

static bool ssag_loader(struct libusb_device_descriptor *descriptor) {
	return (custom_vid != 0 && custom_pid != 0 && descriptor->idVendor == custom_vid && descriptor->idProduct == custom_pid) || (descriptor->idVendor == SSAG_LOADER_VENDOR_ID && descriptor->idProduct == SSAG_LOADER_PRODUCT_ID) || (descriptor->idVendor == QHY5_LOADER_VENDOR_ID && descriptor->idProduct == QHY5_LOADER_PRODUCT_ID) || (descriptor->idVendor == OTI_LOADER_VENDOR_ID && descriptor->idProduct == OTI_LOADER_PRODUCT_ID);
}

static bool ssag_match(libusb_device *dev, const char **name) {
	struct libusb_device_descriptor descriptor;
	int rc = libusb_get_device_descriptor(dev, &descriptor);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		return false;
	}
	if (ssag_loader(&descriptor)) {
		libusb_device *firmware_dev = libusb_ref_device(dev);
		if (!indigo_async(ssag_firmware, firmware_dev)) {
			libusb_unref_device(firmware_dev);
		}
		return false;
	}
	if (descriptor.idVendor != SSAG_VENDOR_ID || descriptor.idProduct != SSAG_PRODUCT_ID) {
		return false;
	}
	static char device_name[INDIGO_NAME_SIZE];
	char usb_path[INDIGO_NAME_SIZE];
	indigo_get_usb_path(dev, usb_path);
	snprintf(device_name, sizeof(device_name), "SSAG");
	indigo_make_name_unique(device_name, "%s", usb_path);
	*name = device_name + 4;
	return true;
}

static bool ssag_init_sequence(indigo_device *device) {
	unsigned char init_packet[18] = {
		0x00, PRIVATE_DATA->gain,
		0x00, PRIVATE_DATA->gain,
		0x00, PRIVATE_DATA->gain,
		0x00, PRIVATE_DATA->gain,
		ROW_START >> 8, ROW_START & 0xff,
		COLUMN_START >> 8, COLUMN_START & 0xff,
		(IMAGE_HEIGHT - 1) >> 8, (IMAGE_HEIGHT - 1) & 0xff,
		(IMAGE_WIDTH - 1) >> 8, (IMAGE_WIDTH - 1) & 0xff,
		SHUTTER_WIDTH >> 8, SHUTTER_WIDTH & 0xff
	};
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, USB_RQ_SET_INIT_PACKET, BUFFER_SIZE & 0xffff, BUFFER_SIZE >> 16, init_packet, sizeof(init_packet), USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc != sizeof(init_packet)) {
		return false;
	}
	rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, USB_RQ_PRE_EXPOSE, PIXEL_OFFSET, 0, NULL, 0, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	return rc == 0;
}

static void ssag_set_gain(indigo_device *device, int gain) {
	if (gain == 7) {
		PRIVATE_DATA->gain = 0x3b;
	} else if (gain <= 4) {
		PRIVATE_DATA->gain = gain * 8;
	} else if (gain <= 8) {
		PRIVATE_DATA->gain = gain * 4 + 0x40;
	} else {
		PRIVATE_DATA->gain = gain - 8 + 0x60;
	}
}

static void ssag_close(indigo_device *device);

static bool ssag_open(indigo_device *device) {
	indigo_set_handler_max_run_time(1);
	int rc = libusb_open(PRIVATE_DATA->usbdev, &PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0 && libusb_kernel_driver_active(PRIVATE_DATA->handle, 0) == 1) {
		rc = libusb_detach_kernel_driver(PRIVATE_DATA->handle, 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_detach_kernel_driver -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0) {
		rc = libusb_set_configuration(PRIVATE_DATA->handle, 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_configuration -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0) {
		rc = libusb_claim_interface(PRIVATE_DATA->handle, 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_claim_interface -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		PRIVATE_DATA->interface_claimed = rc >= 0;
	}
	if (rc >= 0) {
		unsigned char data[4];
		rc = libusb_control_transfer(PRIVATE_DATA->handle, 0xc0, USB_RQ_SET_BUFFER_MODE, 0, 0x63, data, sizeof(data), USB_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc != sizeof(data)) {
			rc = rc < 0 ? rc : LIBUSB_ERROR_IO;
		}
	}
	ssag_set_gain(device, 1);
	if (rc >= 0 && !ssag_init_sequence(device)) {
		rc = LIBUSB_ERROR_IO;
	}
	if (rc >= 0) {
		PRIVATE_DATA->buffer = (unsigned char *)indigo_alloc_blob_buffer(FITS_HEADER_SIZE + BUFFER_SIZE);
		if (PRIVATE_DATA->buffer == NULL) {
			rc = LIBUSB_ERROR_NO_MEM;
		}
	}
	if (rc < 0) {
		ssag_close(device);
		return false;
	}
	return true;
}

static void ssag_close(indigo_device *device) {
	if (PRIVATE_DATA->buffer != NULL) {
		free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
	}
	if (PRIVATE_DATA->handle != NULL) {
		if (PRIVATE_DATA->interface_claimed) {
			libusb_release_interface(PRIVATE_DATA->handle, 0);
			PRIVATE_DATA->interface_claimed = false;
		}
		libusb_close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
	}
}

static bool ssag_start_exposure(indigo_device *device, double exposure) {
	indigo_set_handler_max_run_time(1);
	unsigned char data[2];
	uint32_t duration = (uint32_t)llround(1000 * exposure);
	if (!ssag_init_sequence(device)) {
		return false;
	}
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, 0xc0, USB_RQ_EXPOSE, duration & 0xffff, duration >> 16, data, sizeof(data), USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	return rc == sizeof(data);
}

static bool ssag_read_pixels(indigo_device *device) {
	indigo_set_handler_max_run_time(2);
	int transferred = 0;
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, BUFFER_ENDPOINT, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, BUFFER_SIZE, &transferred, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0 || transferred != BUFFER_SIZE) {
		return false;
	}
	unsigned char *in = PRIVATE_DATA->buffer + FITS_HEADER_SIZE + BUFFER_WIDTH;
	unsigned char *out = PRIVATE_DATA->buffer + FITS_HEADER_SIZE + IMAGE_WIDTH;
	for (int i = 1; i < IMAGE_HEIGHT; i++) {
		memmove(out, in, IMAGE_WIDTH);
		in += BUFFER_WIDTH;
		out += IMAGE_WIDTH;
	}
	return true;
}

static bool ssag_guide(indigo_device *device, guide_direction direction, int duration) {
	unsigned char data[8];
	memcpy(data, &duration, sizeof(duration));
	memcpy(data + sizeof(duration), &duration, sizeof(duration));
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, USB_RQ_GUIDE, 0, direction, data, sizeof(data), USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer(%d, %d) -> %s", direction, duration, rc < 0 ? libusb_error_name(rc) : "OK");
	return rc == sizeof(data);
}

static bool ssag_cancel_guide(indigo_device *device, bool ra) {
	int request = ra ? USB_RQ_CANCEL_GUIDE_EAST_WEST : USB_RQ_CANCEL_GUIDE_NORTH_SOUTH;
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, request, 0, 0, NULL, 0, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer(%d) -> %s", request, rc < 0 ? libusb_error_name(rc) : "OK");
	return rc == 0;
}

//- code

//+ ccd.code

static void ccd_exposure_finalizer(indigo_device *device) {
	int state = atomic_load(&PRIVATE_DATA->exposure_state);
	if (!CONNECTION_CONNECTED_ITEM->sw.value || (state != EXPOSURE_STARTED && state != EXPOSURE_CANCELLED)) {
		return;
	}
	bool cancelled = state == EXPOSURE_CANCELLED;
	bool read_result = ssag_read_pixels(device);
	cancelled |= atomic_exchange(&PRIVATE_DATA->exposure_state, EXPOSURE_IDLE) == EXPOSURE_CANCELLED;
	if (cancelled) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "discarded cancelled exposure (%s)", read_result ? "OK" : "failed");
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, read_result ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, read_result ? "Camera is ready after the cancelled sensor cycle" : "Failed to discard the cancelled exposure");
		return;
	}
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	if (read_result) {
		indigo_process_image(device, PRIVATE_DATA->buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 8, true, true, NULL, false);
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		indigo_ccd_failure_cleanup(device);
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, "Exposure failed");
	}
}

//- ccd.code

//+ guider.code

static void guider_ra_finalizer(indigo_device *device) {
	PRIVATE_DATA->ra_guiding = false;
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_OK_STATE, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	PRIVATE_DATA->dec_guiding = false;
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_OK_STATE, NULL);
}

//- guider.code

#pragma mark - High level code (ccd)

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = ssag_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				ssag_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		if (atomic_exchange(&PRIVATE_DATA->exposure_state, EXPOSURE_IDLE) != EXPOSURE_IDLE) {
			indigo_ccd_abort_exposure_cleanup(device);
		}
		//- ccd.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			ssag_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	int expected_state = EXPOSURE_PENDING;
	if (!atomic_compare_exchange_strong(&PRIVATE_DATA->exposure_state, &expected_state, EXPOSURE_STARTED)) {
		return;
	}
	indigo_use_shortest_exposure_if_bias(device);
	if (ssag_start_exposure(device, CCD_EXPOSURE_ITEM->number.target)) {
		indigo_ccd_exposure_setup(device);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.target, ccd_exposure_finalizer);
	} else {
		atomic_store(&PRIVATE_DATA->exposure_state, EXPOSURE_IDLE);
		indigo_ccd_failure_cleanup(device);
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, "Exposure failed");
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	for (;;) {
		int state = atomic_load(&PRIVATE_DATA->exposure_state);
		if (state == EXPOSURE_PENDING) {
			indigo_cancel_pending_handler(device, ccd_exposure_handler);
			int expected_state = EXPOSURE_PENDING;
			if (atomic_compare_exchange_strong(&PRIVATE_DATA->exposure_state, &expected_state, EXPOSURE_IDLE)) {
				break;
			}
		} else if (state == EXPOSURE_STARTED) {
			int expected_state = EXPOSURE_STARTED;
			if (atomic_compare_exchange_strong(&PRIVATE_DATA->exposure_state, &expected_state, EXPOSURE_CANCELLED)) {
				break;
			}
		} else {
			break;
		}
	}
	indigo_ccd_abort_exposure_cleanup(device);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	ssag_set_gain(device, (int)CCD_GAIN_ITEM->number.target);
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 8;
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = IMAGE_WIDTH;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = IMAGE_HEIGHT;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
		CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 1;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 8;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.min = CCD_BIN_HORIZONTAL_ITEM->number.max = 1;
		CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.min = CCD_BIN_VERTICAL_ITEM->number.max = 1;
		CCD_GAIN_ITEM->number.min = CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = 1;
		CCD_GAIN_ITEM->number.max = 15;
		//- ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			if (CONNECTION_CONNECTED_ITEM->sw.value && PRIVATE_DATA->count == 0) {
				indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, ccd_connection_handler, &driver_queue_mutex);
			} else {
				indigo_execute_handler(device, ccd_connection_handler);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		//+ ccd.CCD_EXPOSURE.on_change_request
		int expected_state = EXPOSURE_IDLE;
		if (!atomic_compare_exchange_strong(&PRIVATE_DATA->exposure_state, &expected_state, EXPOSURE_PENDING)) {
			INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, "Camera exposure cycle is already active");
			return INDIGO_OK;
		}
		//- ccd.CCD_EXPOSURE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = ssag_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				ssag_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		if (PRIVATE_DATA->ra_guiding) {
			ssag_cancel_guide(device, true);
			PRIVATE_DATA->ra_guiding = false;
		}
		if (PRIVATE_DATA->dec_guiding) {
			ssag_cancel_guide(device, false);
			PRIVATE_DATA->dec_guiding = false;
		}
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			ssag_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	bool ok = !atomic_exchange(&PRIVATE_DATA->ra_replacement, false) || ssag_cancel_guide(device, true);
	PRIVATE_DATA->ra_guiding = false;
	int duration = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
	guide_direction direction = GUIDE_EAST;
	if (duration <= 0) {
		duration = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
		direction = GUIDE_WEST;
	}
	if (ok && duration > 0) {
		ok = ssag_guide(device, direction, duration);
	}
	if (ok && duration > 0) {
		PRIVATE_DATA->ra_guiding = true;
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_BUSY_STATE, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_ra_finalizer);
	} else {
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	bool ok = !atomic_exchange(&PRIVATE_DATA->dec_replacement, false) || ssag_cancel_guide(device, false);
	PRIVATE_DATA->dec_guiding = false;
	int duration = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
	guide_direction direction = GUIDE_NORTH;
	if (duration <= 0) {
		duration = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
		direction = GUIDE_SOUTH;
	}
	if (ok && duration > 0) {
		ok = ssag_guide(device, direction, duration);
	}
	if (ok && duration > 0) {
		PRIVATE_DATA->dec_guiding = true;
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_BUSY_STATE, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_dec_finalizer);
	} else {
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			if (CONNECTION_CONNECTED_ITEM->sw.value && PRIVATE_DATA->count == 0) {
				indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, &driver_queue_mutex);
			} else {
				indigo_execute_handler(device, guider_connection_handler);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		bool replacement = GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_BUSY_STATE;
		atomic_store(&PRIVATE_DATA->ra_replacement, replacement);
		if (!replacement) {
			indigo_cancel_pending_handler(device, guider_guide_ra_handler);
		}
		indigo_cancel_pending_handler(device, guider_ra_finalizer);
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		bool replacement = GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_BUSY_STATE;
		atomic_store(&PRIVATE_DATA->dec_replacement, replacement);
		if (!replacement) {
			indigo_cancel_pending_handler(device, guider_guide_dec_handler);
		}
		indigo_cancel_pending_handler(device, guider_dec_finalizer);
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	ssag_private_data *private_data = NULL;
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] && ((ssag_private_data *)devices[i]->private_data)->usbdev == dev) {
			libusb_unref_device(dev);
			return;
		}
	}
	const char *name;
	if (ssag_match(dev, &name)) {
		private_data = (ssag_private_data *)indigo_safe_malloc(sizeof(ssag_private_data));
		private_data->usbdev = dev;
			indigo_device *ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			ccd->private_data = private_data;
			snprintf(ccd->name, INDIGO_NAME_SIZE, "SSAG%s", name);
			bool ccd_attached = false;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					devices[j] = ccd;
					if (indigo_attach_device(ccd) == INDIGO_OK) {
						dev_ref_transferred = true;
						ccd_attached = true;
					} else {
						devices[j] = NULL;
					}
					break;
				}
			}
			if (!ccd_attached) {
				indigo_safe_free(ccd);
				indigo_safe_free(private_data);
				libusb_unref_device(dev);
				return;
			}
			indigo_device *guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = ccd;
			snprintf(guider->name, INDIGO_NAME_SIZE, "SSAG (guider)%s", name);
			bool guider_attached = false;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					devices[j] = guider;
					if (indigo_attach_device(guider) == INDIGO_OK) {
						dev_ref_transferred = true;
						guider_attached = true;
					} else {
						devices[j] = NULL;
					}
					break;
				}
			}
			if (!guider_attached) {
				indigo_safe_free(guider);
			}
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	ssag_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		indigo_safe_free(private_data);
	}
	libusb_unref_device(dev);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_plug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_unplug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_ccd_ssag(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			char *value = getenv("SSAG_VID");
			custom_vid = value == NULL ? 0 : (int)strtol(value, NULL, 16);
			value = getenv("SSAG_PID");
			custom_pid = value == NULL ? 0 : (int)strtol(value, NULL, 16);
			if (custom_vid != 0 && custom_pid != 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "using custom VID = 0x%04x, PID = 0x%04x", custom_vid, custom_pid);
			}
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
