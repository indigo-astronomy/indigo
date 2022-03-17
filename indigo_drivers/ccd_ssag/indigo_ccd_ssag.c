// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// Code is based on OpenSSAG library
// Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
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

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Orion StarShoot AutoGuider driver
 \file indigo_ccd_ssag.c
 */

#define DRIVER_VERSION 0x0008
#define DRIVER_NAME "indigo_ccd_ssag"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <indigo/indigo_usb_utils.h>
#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_ssag.h"
#include "indigo_ccd_ssag_firmware.h"

#define PRIVATE_DATA        ((ssag_private_data *)device->private_data)

// -------------------------------------------------------------------------------- SX USB interface implementation

#define CPUCS_ADDRESS       0xe600
#define USB_TIMEOUT         5000
#define BUFFER_ENDPOINT     0x82
#define IMAGE_WIDTH         1280
#define IMAGE_HEIGHT        1024
#define HORIZONTAL_BLANKING 244
#define VERTICAL_BLANKING   25
#define BUFFER_WIDTH        (IMAGE_WIDTH + HORIZONTAL_BLANKING)
#define BUFFER_HEIGHT       (IMAGE_HEIGHT + VERTICAL_BLANKING + 1)
#define BUFFER_SIZE         (BUFFER_WIDTH * BUFFER_HEIGHT)
#define ROW_START           12
#define COLUMN_START        20
#define SHUTTER_WIDTH       (IMAGE_HEIGHT + VERTICAL_BLANKING)
#define PIXEL_OFFSET        (8 * (BUFFER_WIDTH + 31))

typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	unsigned char gain;
	int device_count;
	unsigned char *buffer;
	indigo_timer *exposure_timer;
} ssag_private_data;

typedef enum {
	guide_east  = 0x10,
	guide_south = 0x20,
	guide_north = 0x40,
	guide_west  = 0x80,
} guide_direction;

enum USB_REQUEST {
	USB_RQ_LOAD_FIRMWARE      = 0xa0,
	USB_RQ_WRITE_SMALL_EEPROM = 0xa2
};

typedef enum {
	USB_RQ_GUIDE = 16,
	USB_RQ_EXPOSE = 18,
	USB_RQ_SET_INIT_PACKET = 19,
	USB_RQ_PRE_EXPOSE = 20,
	USB_RQ_SET_BUFFER_MODE = 85,
	USB_RQ_CANCEL_GUIDE = 24,
	USB_RQ_CANCEL_GUIDE_NORTH_SOUTH = 34,
	USB_RQ_CANCEL_GUIDE_EAST_WEST = 33
} usb_request;

static unsigned char bootloader[] = { SSAG_BOOTLOADER };
static unsigned char firmware[] = { SSAG_FIRMWARE };

static int ssag_reset_mode(libusb_device_handle *handle, unsigned char data) {
	int rc;
	rc = libusb_control_transfer(handle, 0x40, USB_RQ_LOAD_FIRMWARE, 0x7f92, 0, &data, 1, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		rc = libusb_control_transfer(handle, 0x40, USB_RQ_LOAD_FIRMWARE, CPUCS_ADDRESS, 0, &data, 1, USB_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	return rc;
}

static int ssag_upload(libusb_device_handle *handle, unsigned char *data) {
	int rc = 0;
	for (;;) {
		unsigned char byte_count = *data;
		if (byte_count == 0)
			break;
		unsigned short address = *(unsigned int *)(data+1);
		rc = libusb_control_transfer(handle, 0x40, USB_RQ_LOAD_FIRMWARE, address, 0, (unsigned char *)(data+3), byte_count, USB_TIMEOUT);
		if (rc != byte_count) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc;
		}
		data += byte_count + 3;
	}
	return rc;
}

static void ssag_firmware(libusb_device *dev) {
	libusb_device_handle *handle;
	int rc = libusb_open(dev, &handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_upload(handle, bootloader);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x00);
		if (rc >=0)
			  indigo_usleep(ONE_SECOND_DELAY);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_upload(handle, firmware);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x01);
		rc = rc < 0 ? rc : ssag_reset_mode(handle, 0x00);
		libusb_close(handle);
		libusb_unref_device(dev);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
	}
}

static int ssag_init_sequence(indigo_device *device) {
	unsigned char init_packet[18] = {
		0x00, PRIVATE_DATA->gain, /* G1 Gain */
		0x00, PRIVATE_DATA->gain, /* B  Gain */
		0x00, PRIVATE_DATA->gain, /* R  Gain */
		0x00, PRIVATE_DATA->gain, /* G2 Gain */
		ROW_START >> 8, ROW_START & 0xff,
		COLUMN_START >> 8, COLUMN_START & 0xff,
		(IMAGE_HEIGHT - 1) >> 8, (unsigned char)((IMAGE_HEIGHT - 1) & 0xff),
		(IMAGE_WIDTH - 1) >> 8, (unsigned char)((IMAGE_WIDTH - 1) & 0xff),
		SHUTTER_WIDTH >> 8, SHUTTER_WIDTH & 0xff
	};
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, USB_RQ_SET_INIT_PACKET, BUFFER_SIZE & 0xffff, BUFFER_SIZE >> 16, init_packet, sizeof(init_packet), USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, USB_RQ_PRE_EXPOSE, PIXEL_OFFSET, 0, NULL, 0, USB_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	return rc;
}

static void ssag_set_gain(indigo_device *device, int gain) {
	if (gain < 1)
		gain = 1;
	if (gain > 15)
		gain = 15;
	if (gain == 7)
		PRIVATE_DATA->gain = 0x3b;
	else if (gain <= 4)
		PRIVATE_DATA->gain = gain * 8;
	else if (gain <= 8)
		PRIVATE_DATA->gain = (gain * 4) + 0x40;
	PRIVATE_DATA->gain = (gain - 8) + 0x60;
}

static bool ssag_open(indigo_device *device) {
	int rc = 0;
	libusb_device *dev = PRIVATE_DATA->dev;
	rc = libusb_open(dev, &PRIVATE_DATA->handle);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
#ifdef LIBUSB_H // not implemented in fake libusb
	if (rc >= 0) {
		if (libusb_kernel_driver_active(handle, 0) == 1) {
			rc = libusb_detach_kernel_driver(handle, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_detach_kernel_driver -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		}
	}
	if (rc >= 0) {
		rc = libusb_set_configuration(handle, 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_configuration -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0) {
		rc = libusb_claim_interface(handle, 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_claim_interface(%d) -> %s", handle, rc < 0 ? libusb_error_name(rc) : "OK");
	}
#endif
	if (rc >= 0) {
		unsigned char data[4];
		rc = libusb_control_transfer(handle, 0xc0, USB_RQ_SET_BUFFER_MODE, 0x00, 0x63, data, sizeof(data), USB_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	ssag_set_gain(device, 1);
	rc = rc < 0 ? rc : ssag_init_sequence(device);
	return rc >= 0;
}

static bool ssag_start_exposure(indigo_device *device, double exposure) {
	unsigned char data[16];
	unsigned duration = 1000 * exposure;
	int rc = ssag_init_sequence(device);
	rc = rc < 0 ? rc : libusb_control_transfer(PRIVATE_DATA->handle, 0xc0, USB_RQ_EXPOSE, duration & 0xFFFF, duration >> 16, data, 2, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	return rc >= 0;
}

static bool ssag_abort_exposure(indigo_device *device) {
	unsigned char data = 0;
	int transferred;
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, LIBUSB_ENDPOINT_IN, &data, 1, &transferred, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	return rc >= 0;
}

static bool ssag_read_pixels(indigo_device *device) {
	int transferred;
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, BUFFER_ENDPOINT, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, BUFFER_SIZE, &transferred, USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0 && transferred == BUFFER_SIZE) {
		unsigned char *in = PRIVATE_DATA->buffer + BUFFER_WIDTH + FITS_HEADER_SIZE;
		unsigned char *out = PRIVATE_DATA->buffer + IMAGE_WIDTH + FITS_HEADER_SIZE;
		for (int i = 1; i < IMAGE_HEIGHT; i++) {
			memcpy(out, in, IMAGE_WIDTH);
			in += BUFFER_WIDTH;
			out += IMAGE_WIDTH;
		}
	}
	return rc >= 0;
}

static bool ssag_guide(indigo_device *device, guide_direction direction, int duration) {
	unsigned char data[8];
	memcpy(data + 0, &duration, 4);
	memcpy(data + 4, &duration, 4);
	int rc = libusb_control_transfer(PRIVATE_DATA->handle, 0x40, USB_RQ_GUIDE, 0, (int)direction, data, sizeof(data), USB_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_control_transfer(%d, %d) -> %s", direction, duration, rc < 0 ? libusb_error_name(rc) : "OK");
	return rc >= 0;
	return false;
}

static void ssag_close(indigo_device *device) {
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
	free(PRIVATE_DATA->buffer);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	PRIVATE_DATA->exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (ssag_read_pixels(device)) {
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), 8, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
}

//static void streaming_timer_callback(indigo_device *device) {
//	if (!CONNECTION_CONNECTED_ITEM->sw.value)
//		return;
//	while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
//		if (ssag_read_pixels(device)) {
//			indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), 8, true, true, NULL, true);
//		} else {
//			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
//			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
//			break;
//		}
//		indigo_finalize_video_stream(device);
//		if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
//			CCD_STREAMING_COUNT_ITEM->number.value -= 1;
//		if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
//			CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
//			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
//			break;
//		} else {
//			ssag_start_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.target);
//			if (CCD_STREAMING_EXPOSURE_ITEM->number.target < 0.1) {
//				indigo_usleep(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000);
//				indigo_set_timer(device, 0, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
//			} else
//				indigo_set_timer(device, CCD_STREAMING_EXPOSURE_ITEM->number.target, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
//			CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
//			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
//		}
//	}
//}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_FRAME
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 8;
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = 1280;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = 1024;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
//		CCD_STREAMING_PROPERTY->hidden = false;
//		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		CCD_GAIN_PROPERTY->hidden = false;
		CCD_GAIN_ITEM->number.min = CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = 1;
		CCD_GAIN_ITEM->number.max = 15;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = ssag_open(device);
		}
		if (result) {
			PRIVATE_DATA->buffer = (unsigned char *)indigo_alloc_blob_buffer(FITS_HEADER_SIZE + BUFFER_SIZE);
			assert(PRIVATE_DATA->buffer != NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer))
				ssag_abort_exposure(device);
		}
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		if (--PRIVATE_DATA->device_count == 0) {
			ssag_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		ssag_start_exposure(device, CCD_EXPOSURE_ITEM->number.target);
		if (CCD_EXPOSURE_ITEM->number.target < 0.1) {
			indigo_usleep(CCD_EXPOSURE_ITEM->number.target * ONE_SECOND_DELAY);
			indigo_set_timer(device, 0, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		} else
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
//	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
//		// -------------------------------------------------------------------------------- CCD_STREAMING
//		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
//			return INDIGO_OK;
//		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
//		indigo_use_shortest_exposure_if_bias(device);
//		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
//		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
//		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
//			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
//			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
//		}
//		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
//			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
//			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
//		}
//		ssag_start_exposure(device, CCD_STREAMING_EXPOSURE_ITEM->number.target);
//		if (CCD_STREAMING_EXPOSURE_ITEM->number.target < 0.1) {
//			indigo_usleep(CCD_STREAMING_EXPOSURE_ITEM->number.target * ONE_SECOND_DELAY);
//			indigo_set_timer(device, 0, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
//		} else
//			indigo_set_timer(device, CCD_STREAMING_EXPOSURE_ITEM->number.target, streaming_timer_callback, &PRIVATE_DATA->exposure_timer);
//		return INDIGO_OK;
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer))
				ssag_abort_exposure(device);
		}
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		ssag_set_gain(device, CCD_GAIN_ITEM->number.target);
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = ssag_open(device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			ssag_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			ssag_guide(device, guide_north, duration);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				ssag_guide(device, guide_south, duration);
			}
		}
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			ssag_guide(device, guide_east, duration);
		} else {
			duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				ssag_guide(device, guide_west, duration);
			}
		}
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES 10

/* Orion Telescopes SSAG VID/PID */
#define SSAG_VENDOR_ID 0x1856
#define SSAG_PRODUCT_ID 0x0012

/* uninitialized SSAG VID/PID */
#define SSAG_LOADER_VENDOR_ID 0x1856
#define SSAG_LOADER_PRODUCT_ID 0x0011

/* uninitialized QHY5 VID/PID */
#define QHY5_LOADER_VENDOR_ID 0x1618
#define QHY5_LOADER_PRODUCT_ID 0x0901

/* uninitialized OTI VID/PID */
#define OTI_LOADER_VENDOR_ID 0x16C0
#define OTI_LOADER_PRODUCT_ID 0x296D

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		"",
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
	struct libusb_device_descriptor descriptor;
	pthread_mutex_lock(&device_mutex);
	INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
	if ((descriptor.idVendor == SSAG_LOADER_VENDOR_ID && descriptor.idProduct == SSAG_LOADER_PRODUCT_ID) || (descriptor.idVendor == QHY5_LOADER_VENDOR_ID && descriptor.idProduct == QHY5_LOADER_PRODUCT_ID) || (descriptor.idVendor == OTI_LOADER_VENDOR_ID && descriptor.idProduct == OTI_LOADER_PRODUCT_ID)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor ->  %s (0x%04x, 0x%04x)", rc < 0 ? libusb_error_name(rc) : "OK", descriptor.idVendor, descriptor.idProduct);
		libusb_ref_device(dev);
		indigo_async((void *)(void *)ssag_firmware, dev);
	} else if (descriptor.idVendor == SSAG_VENDOR_ID && descriptor.idProduct == SSAG_PRODUCT_ID) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor ->  %s (0x%04x, 0x%04x)", rc < 0 ? libusb_error_name(rc) : "OK", descriptor.idVendor, descriptor.idProduct);
		ssag_private_data *private_data = indigo_safe_malloc(sizeof(ssag_private_data));
		libusb_ref_device(dev);
		private_data->dev = dev;
		indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
		char usb_path[INDIGO_NAME_SIZE];
		indigo_get_usb_path(dev, usb_path);
		snprintf(device->name, INDIGO_NAME_SIZE, "SSAG #%s", usb_path);
		device->private_data = private_data;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				indigo_attach_device(devices[j] = device);
				break;
			}
		}
		device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		snprintf(device->name, INDIGO_NAME_SIZE, "SSAG (guider) #%s", usb_path);
		device->private_data = private_data;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				indigo_attach_device(devices[j] = device);
				break;
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&device_mutex);
	ssag_private_data *private_data = NULL;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->dev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		if (private_data->buffer)
			free(private_data->buffer);
		free(private_data);
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
	case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
		INDIGO_ASYNC(process_plug_event, dev);
		break;
	case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
		process_unplug_event(dev);
		break;
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_ssag(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "SSAG/QHY5 Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
