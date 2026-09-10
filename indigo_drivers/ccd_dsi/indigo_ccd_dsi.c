// Copyright (c) 2017-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_ccd_dsi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include "libdsi.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_dsi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000E
#define DRIVER_NAME          "indigo_ccd_dsi"
#define DRIVER_LABEL         "Meade DSI Camera"
#define CCD_DEVICE_NAME      "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((dsi_private_data *)device->private_data)

//+ define

#define MAX_CCD_TEMP         45     /* Max CCD temperature */
#define MIN_CCD_TEMP         -55     /* Min CCD temperature */

#define DEFAULT_BPP          16     /* Default bits per pixel */

#define TEMP_CHECK_TIME      3     /* Time between teperature checks (seconds) */
#define DSI_VENDOR_ID        0x156c
#undef MAX_DEVICES
#define MAX_DEVICES          32

//- define

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	//+ data
	char dev_sid[DSI_ID_LEN];
	enum DSI_BIN_MODE exp_bin_mode;
	dsi_camera_t *dsi;
	long int buffer_size;
	char *buffer;
	bool can_check_temperature;
	//- data
} dsi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

static void ccd_exposure_handler(indigo_device *device);

typedef enum {
	DSI_IMAGE_FAILED,
	DSI_IMAGE_PENDING,
	DSI_IMAGE_READY
} dsi_image_result;

static char connected_sids[DSI_MAX_DEVICES][DSI_ID_LEN];

static bool dsi_sid_is_connected(const char *sid) {
	for (int i = 0; i < DSI_MAX_DEVICES; i++) {
		if (!strncmp(connected_sids[i], sid, DSI_ID_LEN)) {
			return true;
		}
	}
	return false;
}

static void dsi_set_connected_sid(const char *sid, bool connected) {
	for (int i = 0; i < DSI_MAX_DEVICES; i++) {
		if (connected) {
			if (connected_sids[i][0] == '\0') {
				strncpy(connected_sids[i], sid, DSI_ID_LEN);
				connected_sids[i][DSI_ID_LEN - 1] = '\0';
				return;
			}
		} else if (!strncmp(connected_sids[i], sid, DSI_ID_LEN)) {
			connected_sids[i][0] = '\0';
			return;
		}
	}
}

static bool dsi_open(indigo_device *device) {
	// TODO: Split DSI camera initialization into short handler tasks.
	indigo_set_handler_max_run_time(2);
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	PRIVATE_DATA->dsi = dsi_open_camera(PRIVATE_DATA->dev_sid);
	if (PRIVATE_DATA->dsi == NULL) {
		indigo_global_unlock(device);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsi_open_camera(%s) = %p", PRIVATE_DATA->dev_sid, PRIVATE_DATA->dsi);
		return false;
	}
	if (PRIVATE_DATA->buffer == NULL) {
		PRIVATE_DATA->buffer_size = dsi_get_frame_width(PRIVATE_DATA->dsi) * dsi_get_frame_height(PRIVATE_DATA->dsi) * dsi_get_bytespp(PRIVATE_DATA->dsi) + FITS_HEADER_SIZE;
		PRIVATE_DATA->buffer = (char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
		if (PRIVATE_DATA->buffer == NULL) {
			dsi_close_camera(PRIVATE_DATA->dsi);
			PRIVATE_DATA->dsi = NULL;
			indigo_global_unlock(device);
			return false;
		}
	}
	return true;
}

static bool dsi_camera_start_exposure(indigo_device *device, double exposure, bool dark, int binning) {
	long res;
	enum DSI_BIN_MODE bin_mode = (binning > 1) ? BIN2X2 : BIN1X1;
	if (dsi_get_max_binning(PRIVATE_DATA->dsi) > 1) {
		res = dsi_set_binning(PRIVATE_DATA->dsi, bin_mode);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsi_set_binning(%s, %d) = %d", PRIVATE_DATA->dev_sid, bin_mode, res);
			return false;
		}
	}
	res = dsi_start_exposure(PRIVATE_DATA->dsi, exposure);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsi_start_exposure(%s) = %d", PRIVATE_DATA->dev_sid, res);
		return false;
	}
	PRIVATE_DATA->exp_bin_mode = bin_mode;
	return true;
}

static dsi_image_result dsi_camera_read_pixels(indigo_device *device, double *retry_delay) {
	long res;
	double exposure_time_left = dsi_get_exposure_time_left(PRIVATE_DATA->dsi);
	if (exposure_time_left > 0) {
		*retry_delay = fmax(exposure_time_left, 0.01);
		return DSI_IMAGE_PENDING;
	}
	dsi_set_image_little_endian(PRIVATE_DATA->dsi, 0);
	res = dsi_read_image(PRIVATE_DATA->dsi, (unsigned char *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), true);
	if (res == 0) {
		return DSI_IMAGE_READY;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed! dsi_read_image(%s) = %ld", PRIVATE_DATA->dev_sid, res);
	dsi_abort_exposure(PRIVATE_DATA->dsi);
	dsi_reset_camera(PRIVATE_DATA->dsi);
	return DSI_IMAGE_FAILED;
}

static bool dsi_camera_abort_exposure(indigo_device *device) {
	dsi_abort_exposure(PRIVATE_DATA->dsi);
	dsi_reset_camera(PRIVATE_DATA->dsi);
	PRIVATE_DATA->can_check_temperature = true;
	return true;
}

static void dsi_close(indigo_device *device) {
	// TODO: Split DSI camera shutdown into short handler tasks.
	indigo_set_handler_max_run_time(2);
	dsi_close_camera(PRIVATE_DATA->dsi);
	indigo_global_unlock(device);
	if (PRIVATE_DATA->buffer != NULL) {
		free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
	}
}

static bool dsi_get_plugged_device_name(const char *sid, char *name, size_t size) {
#ifdef __APPLE__
	snprintf(name, size, "Meade DSI");
#else
	// doesn't work on macOS, dsi_open_camera resets the device what leads to duplicate plug/unplug
	dsi_camera_t *dsi = dsi_open_camera(sid);
	if (dsi == NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Camera %s can not be open.", sid);
		return false;
	}
	snprintf(name, size, "%s", dsi_get_model_name(dsi));
	dsi_close_camera(dsi);
#endif
	return true;
}

//- code

//+ ccd.code

static void ccd_exposure_finalizer(indigo_device *device) {
	double retry_delay;
	// TODO: Split the blocking DSI image readout into incremental transfers.
	indigo_set_handler_max_run_time(2);
	PRIVATE_DATA->can_check_temperature = true;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		switch (dsi_camera_read_pixels(device, &retry_delay)) {
		case DSI_IMAGE_READY: {
			int binning = (PRIVATE_DATA->exp_bin_mode == BIN1X1) ? 1 : 2;
			const char *color_string = dsi_get_bayer_pattern(PRIVATE_DATA->dsi);
			if (color_string[0] != '\0') {
				/* NOTE: There is no need to take care about the offsets,
				 the SDK takes care the image to be in the correct bayer pattern */
				indigo_fits_keyword keywords[] = {
					{ INDIGO_FITS_STRING, "BAYERPAT", .string = color_string, "Bayer color pattern" },
					{ 0 }
				};
				indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / binning), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / binning), DEFAULT_BPP, true, true, keywords, false);
			} else {
				indigo_process_image(device, PRIVATE_DATA->buffer,  (int)(CCD_FRAME_WIDTH_ITEM->number.value / binning), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / binning), DEFAULT_BPP, true, true, NULL, false );
			}
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			break;
		}
		case DSI_IMAGE_PENDING:
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, retry_delay, ccd_exposure_finalizer);
			break;
		case DSI_IMAGE_FAILED:
			indigo_ccd_failure_cleanup(device);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
			break;
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}

//- ccd.code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	if (PRIVATE_DATA->can_check_temperature) {
		// check temperature;
		CCD_TEMPERATURE_ITEM->number.value = dsi_get_temperature(PRIVATE_DATA->dsi);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_execute_handler_in(device, TEMP_CHECK_TIME, ccd_timer_callback);
	}
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = dsi_open(device);
		if (connection_result) {
			//+ ccd.on_connect
			CCD_INFO_WIDTH_ITEM->number.value = dsi_get_frame_width(PRIVATE_DATA->dsi);
			CCD_INFO_HEIGHT_ITEM->number.value = dsi_get_frame_height(PRIVATE_DATA->dsi);
			CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
			CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
			sprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "%s", dsi_get_serial_number(PRIVATE_DATA->dsi));
			sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", dsi_get_model_name(PRIVATE_DATA->dsi));
			indigo_update_property(device, INFO_PROPERTY, NULL);
			CCD_INFO_PIXEL_WIDTH_ITEM->number.value = dsi_get_pixel_width(PRIVATE_DATA->dsi);
			CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = dsi_get_pixel_height(PRIVATE_DATA->dsi);
			CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;
			CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 1;
			CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 1;
			CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = DEFAULT_BPP;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = DEFAULT_BPP;
			char name[32];
			if (dsi_get_max_binning(PRIVATE_DATA->dsi) > 1) {
				CCD_BIN_PROPERTY->hidden = false;
				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_MODE_PROPERTY->count = 2;
				sprintf(name, "RAW 16 %dx%d", dsi_get_frame_width(PRIVATE_DATA->dsi), dsi_get_frame_height(PRIVATE_DATA->dsi));
				indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
				sprintf(name, "RAW 16 %dx%d", dsi_get_frame_width(PRIVATE_DATA->dsi)/2, dsi_get_frame_height(PRIVATE_DATA->dsi)/2);
				indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
			} else {
				CCD_BIN_PROPERTY->hidden = true;  // keep it hidden as device does not support binning!
				CCD_BIN_PROPERTY->perm = INDIGO_RO_PERM;
				CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_MODE_PROPERTY->count = 1;
				sprintf(name, "RAW 16 %dx%d", dsi_get_frame_width(PRIVATE_DATA->dsi), dsi_get_frame_height(PRIVATE_DATA->dsi));
				indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
			}
			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
			CCD_BIN_HORIZONTAL_ITEM->number.max = dsi_get_max_binning(PRIVATE_DATA->dsi);
			CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
			CCD_BIN_VERTICAL_ITEM->number.max = dsi_get_max_binning(PRIVATE_DATA->dsi);
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
			CCD_TEMPERATURE_PROPERTY->hidden = false;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
			CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
			CCD_TEMPERATURE_ITEM->number.step = 0;
			CCD_GAIN_PROPERTY->hidden = false;
			CCD_GAIN_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_GAIN_ITEM->number.min = 0;
			CCD_GAIN_ITEM->number.max = 100;
			CCD_GAIN_ITEM->number.value = dsi_get_amp_gain(PRIVATE_DATA->dsi);
			CCD_OFFSET_PROPERTY->hidden = false;
			CCD_OFFSET_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_OFFSET_ITEM->number.min = 0;
			CCD_OFFSET_ITEM->number.max = 100;
			CCD_OFFSET_ITEM->number.value = dsi_get_amp_offset(PRIVATE_DATA->dsi);
			double temp = dsi_get_temperature(PRIVATE_DATA->dsi);
			if (temp > 1000) {  /* no sensor */
				CCD_TEMPERATURE_PROPERTY->hidden = true;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dsi_get_temperature(%s) = NO_SENSOR", PRIVATE_DATA->dev_sid);
			} else {
				PRIVATE_DATA->can_check_temperature = true;
			}
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, ccd_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		PRIVATE_DATA->can_check_temperature = false;
		//- ccd.on_disconnect
		dsi_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	long ok;
	ok = dsi_camera_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value, CCD_BIN_VERTICAL_ITEM->number.value);
	if (ok) {
		indigo_ccd_exposure_setup(device);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.target, ccd_exposure_finalizer);
	} else {
		indigo_ccd_failure_cleanup(device);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, ccd_exposure_handler);
		indigo_cancel_pending_handler(device, ccd_exposure_finalizer);
		dsi_camera_abort_exposure(device);
	}
	PRIVATE_DATA->can_check_temperature = true;
	indigo_ccd_abort_exposure_cleanup(device);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	dsi_set_amp_gain(PRIVATE_DATA->dsi, (int)(CCD_GAIN_ITEM->number.value));
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_offset_handler(indigo_device *device) {
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_OFFSET.on_change
	dsi_set_amp_offset(PRIVATE_DATA->dsi, (int)(CCD_OFFSET_ITEM->number.value));
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//- ccd.CCD_OFFSET.on_change
	indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	int prev_bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int prev_bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	/* DSI requires BIN_X and BIN_Y to be equal, so keep them entangled */
	if ((int)CCD_BIN_HORIZONTAL_ITEM->number.value != prev_bin_x) {
		CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.value;
	} else if ((int)CCD_BIN_VERTICAL_ITEM->number.value != prev_bin_y) {
		CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.value;
	}
	indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY);
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		/* Use all info property fields */
		INFO_PROPERTY->count = 8;
		//- ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
		CCD_OFFSET_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
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
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, ccd_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_OFFSET_PROPERTY, ccd_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
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

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

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
	dsi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = indigo_safe_malloc(sizeof(dsi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == DSI_VENDOR_ID)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		#ifdef __APPLE__
			if (dsi_load_firmware()) {
				plug_result = false;
			}
		#endif
			if (plug_result) {
				plug_result = false;
				private_data->dev_sid[0] = 0;
				dsi_device_list dev_list;
				int count = dsi_scan_usb(dev_list);
				for (int i = 0; i < count; i++) {
					if (dsi_sid_is_connected(dev_list[i])) {
						continue;
					}
					char dev_name[DSI_NAME_LEN + 1];
					if (!dsi_get_plugged_device_name(dev_list[i], dev_name, sizeof(dev_name))) {
						continue;
					}
					strncpy(private_data->dev_sid, dev_list[i], DSI_ID_LEN);
					private_data->dev_sid[DSI_ID_LEN - 1] = '\0';
					dsi_set_connected_sid(private_data->dev_sid, true);
					snprintf(name, INDIGO_NAME_SIZE, "%s", dev_name);
					indigo_make_name_unique(name, "%s", private_data->dev_sid);
					plug_result = true;
					break;
				}
			}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *ccd = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
		ccd->private_data = private_data;
		snprintf(ccd->name, INDIGO_NAME_SIZE, "%s", name);
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
		}
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	dsi_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				//+ sdk.unplug
				dsi_set_connected_sid(private_data->dev_sid, false);
				//- sdk.unplug
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

indigo_result indigo_ccd_dsi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
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
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, DSI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
