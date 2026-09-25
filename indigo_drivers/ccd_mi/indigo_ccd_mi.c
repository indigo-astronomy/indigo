// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_ccd_mi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <gxccd.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_mi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000024
#define DRIVER_NAME          "indigo_ccd_mi"
#define DRIVER_LABEL         "Moravian Instruments Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s (guider)"
#define WHEEL_DEVICE_NAME    "%s (wheel)"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((mi_private_data *)device->private_data)

//+ define

#define MI_VID               0x1347
#define TEMP_PERIOD          5
#define TEMP_COOLER_OFF      50
#define EXPOSURE_POLL_PERIOD 0.01
#define EXPOSURE_READOUT_TIMEOUT 30
#define MI_MAX_ENUMERATED_IDS 64
#define CCD_READ_MODE_ITEM   (CCD_READ_MODE_PROPERTY->items + 0)

//- define

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	//+ data
	int eid;
	camera_t *camera;
	char model[INDIGO_NAME_SIZE];
	char guider_name[INDIGO_NAME_SIZE];
	char wheel_name[INDIGO_NAME_SIZE];
	bool has_guider;
	bool has_wheel;
	bool has_cooler;
	bool has_power_utilization;
	bool has_gain;
	bool asymmetric_binning;
	bool cooler_on;
	bool acquisition_active;
	unsigned char *buffer;
	size_t buffer_size;
	int read_mode;
	int image_width;
	int image_height;
	double exposure_end;
	double readout_deadline;
	float target_temperature;
	float current_temperature;
	int16_t guider_ra_duration;
	int16_t guider_dec_duration;
	double guider_ra_end;
	double guider_dec_end;
	int wheel_slot;
	//- data
} mi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static int enumerated_ids[MI_MAX_ENUMERATED_IDS];
static int enumerated_id_count;

static void enumerate_callback(int eid) {
	if (enumerated_id_count < MI_MAX_ENUMERATED_IDS) {
		enumerated_ids[enumerated_id_count++] = eid;
	}
}

static int enumerate_cameras(void) {
	enumerated_id_count = 0;
	gxccd_enumerate_usb(enumerate_callback);
	return enumerated_id_count;
}

static bool camera_is_enumerated(int eid) {
	int count = enumerate_cameras();
	for (int i = 0; i < count; i++) {
		if (enumerated_ids[i] == eid) {
			return true;
		}
	}
	return false;
}

static void trim_trailing_space(char *text) {
	size_t length = strlen(text);
	while (length > 0 && isspace((unsigned char)text[length - 1])) {
		text[--length] = 0;
	}
}

static void report_error(indigo_device *device, indigo_property *property, const char *operation) {
	char message[128] = "Moravian Instruments SDK error";
	if (PRIVATE_DATA->camera != NULL) {
		gxccd_get_last_error(PRIVATE_DATA->camera, message, sizeof(message));
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed: %s", operation, message);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

static bool get_integer(indigo_device *device, int index, int *value) {
	if (gxccd_get_integer_parameter(PRIVATE_DATA->camera, index, value) == 0) {
		return true;
	}
	char message[128] = { 0 };
	gxccd_get_last_error(PRIVATE_DATA->camera, message, sizeof(message));
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "gxccd_get_integer_parameter(%d) failed: %s", index, message);
	return false;
}

static bool mi_open(indigo_device *device) {
	PRIVATE_DATA->camera = gxccd_initialize_usb(PRIVATE_DATA->eid);
	if (PRIVATE_DATA->camera == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "gxccd_initialize_usb(%d) failed", PRIVATE_DATA->eid);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gxccd_initialize_usb(%d) succeeded", PRIVATE_DATA->eid);
	return true;
}

static void mi_close(indigo_device *device) {
	indigo_lock_master_device(device);
	if (PRIVATE_DATA->camera != NULL) {
		gxccd_release(PRIVATE_DATA->camera);
		PRIVATE_DATA->camera = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gxccd_release() succeeded");
	}
	indigo_unlock_master_device(device);
}

//- code

//+ ccd.code

static bool initialize_ccd(indigo_device *device) {
	int width = 0, height = 0, pixel_width = 0, pixel_height = 0, max_bin_x = 0, max_bin_y = 0;
	if (!get_integer(device, GIP_CHIP_W, &width) || !get_integer(device, GIP_CHIP_D, &height) || !get_integer(device, GIP_PIXEL_W, &pixel_width) || !get_integer(device, GIP_PIXEL_D, &pixel_height) || !get_integer(device, GIP_MAX_BINNING_X, &max_bin_x) || !get_integer(device, GIP_MAX_BINNING_Y, &max_bin_y)) {
		return false;
	}
	if (width <= 0 || height <= 0 || pixel_width <= 0 || pixel_height <= 0 || max_bin_x <= 0 || max_bin_y <= 0 || (size_t)width > (SIZE_MAX - FITS_HEADER_SIZE) / 2 / (size_t)height) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid camera geometry or limits");
		return false;
	}
	PRIVATE_DATA->asymmetric_binning = false;
	if (gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_ASYMMETRIC_BINNING, &PRIVATE_DATA->asymmetric_binning) != 0) {
		PRIVATE_DATA->asymmetric_binning = false;
	}
	CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = width;
	CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = height;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = 0;
	CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(pixel_width / 10.0) / 100.0;
	CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(pixel_height / 10.0) / 100.0;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = 16;
	CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
	CCD_BIN_HORIZONTAL_ITEM->number.min = CCD_BIN_VERTICAL_ITEM->number.min = 1;
	CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin_x;
	CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin_y;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = 1;
	CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = 1;
	int min_exposure = 0;
	if (gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MINIMAL_EXPOSURE, &min_exposure) == 0 && min_exposure > 0) {
		CCD_EXPOSURE_ITEM->number.min = min_exposure / 1000000.0;
	}
	int max_exposure = 0;
	if (gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MAXIMAL_EXPOSURE, &max_exposure) == 0 && max_exposure > 0) {
		CCD_EXPOSURE_ITEM->number.max = max_exposure / 1000.0;
	}
	CCD_MODE_PROPERTY->count = 0;
	const int bins[] = { 1, 2, 3, 4, 8 };
	for (unsigned i = 0; i < sizeof(bins) / sizeof(bins[0]); i++) {
		int bin = bins[i];
		if (bin <= max_bin_x && bin <= max_bin_y) {
			char item_name[32], item_label[32];
			snprintf(item_name, sizeof(item_name), "BIN_%dx%d", bin, bin);
			snprintf(item_label, sizeof(item_label), "RAW 16 %dx%d", width / bin, height / bin);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + CCD_MODE_PROPERTY->count, item_name, item_label, bin == 1);
			CCD_MODE_PROPERTY->count++;
		}
	}
	CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
	int read_modes = 0;
	if (gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_READ_MODES, &read_modes) != 0) {
		read_modes = 0;
	}
	if (read_modes < 0 || read_modes > 128) {
		return false;
	}
	CCD_READ_MODE_PROPERTY->hidden = read_modes == 0;
	CCD_READ_MODE_PROPERTY = indigo_resize_property(CCD_READ_MODE_PROPERTY, read_modes);
	if (read_modes == 0) {
		PRIVATE_DATA->read_mode = 0;
	} else {
		PRIVATE_DATA->read_mode = 0;
		if (gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_DEFAULT_READ_MODE, &PRIVATE_DATA->read_mode) != 0 || PRIVATE_DATA->read_mode < 0 || PRIVATE_DATA->read_mode >= read_modes) {
			PRIVATE_DATA->read_mode = 0;
		}
		for (int i = 0; i < read_modes; i++) {
			char item_name[32], item_label[INDIGO_VALUE_SIZE] = { 0 };
			if (gxccd_enumerate_read_modes(PRIVATE_DATA->camera, i, item_label, sizeof(item_label)) != 0) {
				snprintf(item_label, sizeof(item_label), "Read mode %d", i);
			}
			snprintf(item_name, sizeof(item_name), "READ_MODE%d", i);
			indigo_init_switch_item(CCD_READ_MODE_ITEM + i, item_name, item_label, i == PRIVATE_DATA->read_mode);
		}
	}
	PRIVATE_DATA->has_cooler = PRIVATE_DATA->has_power_utilization = PRIVATE_DATA->has_gain = false;
	if (gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_COOLER, &PRIVATE_DATA->has_cooler) != 0) {
		PRIVATE_DATA->has_cooler = false;
	}
	if (gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_POWER_UTILIZATION, &PRIVATE_DATA->has_power_utilization) != 0) {
		PRIVATE_DATA->has_power_utilization = false;
	}
	if (gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_GAIN, &PRIVATE_DATA->has_gain) != 0) {
		PRIVATE_DATA->has_gain = false;
	}
	CCD_COOLER_PROPERTY->hidden = !PRIVATE_DATA->has_cooler;
	CCD_COOLER_POWER_PROPERTY->hidden = !PRIVATE_DATA->has_power_utilization;
	CCD_TEMPERATURE_PROPERTY->hidden = false;
	CCD_TEMPERATURE_PROPERTY->perm = PRIVATE_DATA->has_cooler ? INDIGO_RW_PERM : INDIGO_RO_PERM;
	CCD_GAIN_PROPERTY->hidden = CCD_EGAIN_PROPERTY->hidden = true;
	if (PRIVATE_DATA->has_gain) {
		int max_gain = 0;
		if (gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MAX_GAIN, &max_gain) != 0) {
			max_gain = 0;
		}
		if (max_gain > 0) {
			CCD_GAIN_PROPERTY->hidden = false;
			CCD_GAIN_ITEM->number.min = 0;
			CCD_GAIN_ITEM->number.max = max_gain;
			CCD_GAIN_ITEM->number.step = 1;
			CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = 0;
		}
		float egain = 0;
		if (gxccd_get_value(PRIVATE_DATA->camera, GV_ADC_GAIN, &egain) == 0) {
			CCD_EGAIN_PROPERTY->hidden = false;
			CCD_EGAIN_ITEM->number.value = egain;
		}
	}
	float temperature = 0;
	if (gxccd_get_value(PRIVATE_DATA->camera, GV_CHIP_TEMPERATURE, &temperature) == 0) {
		PRIVATE_DATA->current_temperature = temperature;
		CCD_TEMPERATURE_ITEM->number.value = temperature;
	}
	PRIVATE_DATA->cooler_on = false;
	PRIVATE_DATA->target_temperature = TEMP_COOLER_OFF;
	indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
	if (PRIVATE_DATA->has_power_utilization) {
		float power = 0;
		if (gxccd_get_value(PRIVATE_DATA->camera, GV_POWER_UTILIZATION, &power) == 0) {
			CCD_COOLER_POWER_ITEM->number.value = round(power * 1000) / 10;
			if (PRIVATE_DATA->has_cooler && power > 0.0001) {
				PRIVATE_DATA->cooler_on = true;
				PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature;
				CCD_TEMPERATURE_ITEM->number.target = PRIVATE_DATA->target_temperature;
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				if (gxccd_set_temperature(PRIVATE_DATA->camera, PRIVATE_DATA->target_temperature) != 0) {
					return false;
				}
			}
		}
	}
	PRIVATE_DATA->buffer_size = (size_t)width * (size_t)height * 2 + FITS_HEADER_SIZE;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	if (PRIVATE_DATA->buffer == NULL) {
		return false;
	}
	PRIVATE_DATA->acquisition_active = false;
	return true;
}

static void exposure_failed(indigo_device *device, const char *message) {
	PRIVATE_DATA->acquisition_active = false;
	indigo_ccd_failure_cleanup(device);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "%s", message);
}

static void exposure_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->acquisition_active) {
		return;
	}
	double now = indigo_monotonic_time();
	if (now < PRIVATE_DATA->exposure_end) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, fmin(1, PRIVATE_DATA->exposure_end - now), exposure_finalizer);
		return;
	}
	bool ready = false;
	if (gxccd_image_ready(PRIVATE_DATA->camera, &ready) != 0) {
		exposure_failed(device, "Exposure readiness check failed");
		return;
	}
	if (!ready) {
		if (now < PRIVATE_DATA->readout_deadline) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, EXPOSURE_POLL_PERIOD, exposure_finalizer);
		} else {
			gxccd_abort_exposure(PRIVATE_DATA->camera, false);
			exposure_failed(device, "Exposure readiness timed out");
		}
		return;
	}
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	size_t image_size = (size_t)PRIVATE_DATA->image_width * (size_t)PRIVATE_DATA->image_height * 2;
	if (gxccd_read_image(PRIVATE_DATA->camera, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, image_size) != 0) {
		exposure_failed(device, "Image readout failed");
		return;
	}
	if (!CONNECTION_CONNECTED_ITEM->sw.value || CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		return;
	}
	indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->image_width, PRIVATE_DATA->image_height, 16, true, true, NULL, false);
	PRIVATE_DATA->acquisition_active = false;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

//- ccd.code

//+ guider.code

static int16_t remaining_pulse(double end, int16_t duration) {
	double remaining = end - indigo_monotonic_time();
	if (remaining <= 0 || duration == 0) {
		return 0;
	}
	int milliseconds = (int)ceil(remaining * 1000);
	if (milliseconds > INT16_MAX) {
		milliseconds = INT16_MAX;
	}
	return duration > 0 ? (int16_t)milliseconds : (int16_t)-milliseconds;
}

static void guider_ra_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	PRIVATE_DATA->guider_ra_duration = 0;
	PRIVATE_DATA->guider_ra_end = 0;
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	PRIVATE_DATA->guider_dec_duration = 0;
	PRIVATE_DATA->guider_dec_end = 0;
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

//- guider.code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	if (!PRIVATE_DATA->acquisition_active) {
		float temperature = 0;
		if (gxccd_get_value(PRIVATE_DATA->camera, GV_CHIP_TEMPERATURE, &temperature) == 0) {
			PRIVATE_DATA->current_temperature = temperature;
			CCD_TEMPERATURE_ITEM->number.value = round(temperature * 10) / 10;
			CCD_TEMPERATURE_PROPERTY->state = PRIVATE_DATA->cooler_on && fabs(temperature - PRIVATE_DATA->target_temperature) > 1 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		} else {
			report_error(device, CCD_TEMPERATURE_PROPERTY, "gxccd_get_value(GV_CHIP_TEMPERATURE)");
		}
		if (PRIVATE_DATA->has_power_utilization) {
			float power = 0;
			if (gxccd_get_value(PRIVATE_DATA->camera, GV_POWER_UTILIZATION, &power) == 0) {
				CCD_COOLER_POWER_ITEM->number.value = round(power * 1000) / 10;
				CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			} else {
				report_error(device, CCD_COOLER_POWER_PROPERTY, "gxccd_get_value(GV_POWER_UTILIZATION)");
			}
		}
	}
	indigo_execute_handler_in(device, TEMP_PERIOD, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = mi_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
			indigo_lock_master_device(device);
			connection_result = initialize_ccd(device);
			if (!connection_result && PRIVATE_DATA->buffer != NULL) {
				free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
				PRIVATE_DATA->buffer_size = 0;
			}
			indigo_unlock_master_device(device);
			//- ccd.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				mi_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		indigo_lock_master_device(device);
		if (PRIVATE_DATA->acquisition_active) {
			gxccd_abort_exposure(PRIVATE_DATA->camera, false);
			PRIVATE_DATA->acquisition_active = false;
			indigo_ccd_abort_exposure_cleanup(device);
		}
		free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
		PRIVATE_DATA->buffer_size = 0;
		indigo_unlock_master_device(device);
		//- ccd.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			CCD_READ_MODE_PROPERTY,
			CCD_COOLER_PROPERTY,
			CCD_TEMPERATURE_PROPERTY,
			CCD_COOLER_POWER_PROPERTY,
			CCD_GAIN_PROPERTY,
			CCD_EGAIN_PROPERTY,
			CCD_EXPOSURE_PROPERTY,
			CCD_ABORT_EXPOSURE_PROPERTY,
			CCD_BIN_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			mi_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, ccd_timer_callback);
	}
}

static void ccd_read_mode_handler(indigo_device *device) {
	CCD_READ_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_READ_MODE.on_change
	int selected = -1;
	for (int i = 0; i < CCD_READ_MODE_PROPERTY->count; i++) {
		if (CCD_READ_MODE_PROPERTY->items[i].sw.value) {
			selected = i;
			break;
		}
	}
	if (selected < 0 || gxccd_set_read_mode(PRIVATE_DATA->camera, selected) != 0) {
		indigo_set_switch(CCD_READ_MODE_PROPERTY, CCD_READ_MODE_ITEM + PRIVATE_DATA->read_mode, true);
		CCD_READ_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->read_mode = selected;
	}
	//- ccd.CCD_READ_MODE.on_change
	indigo_update_property(device, CCD_READ_MODE_PROPERTY, NULL);
}

static void ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_COOLER.on_change
	bool requested_on = CCD_COOLER_ON_ITEM->sw.value;
	float target = requested_on ? CCD_TEMPERATURE_ITEM->number.target : TEMP_COOLER_OFF;
	if (gxccd_set_temperature(PRIVATE_DATA->camera, target) != 0) {
		indigo_set_switch(CCD_COOLER_PROPERTY, PRIVATE_DATA->cooler_on ? CCD_COOLER_ON_ITEM : CCD_COOLER_OFF_ITEM, true);
		CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->cooler_on = requested_on;
		PRIVATE_DATA->target_temperature = target;
		CCD_TEMPERATURE_PROPERTY->state = requested_on ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	float target = CCD_TEMPERATURE_ITEM->number.target;
	if (gxccd_set_temperature(PRIVATE_DATA->camera, target) != 0) {
		CCD_TEMPERATURE_ITEM->number.target = PRIVATE_DATA->target_temperature;
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->target_temperature = target;
		PRIVATE_DATA->cooler_on = target < TEMP_COOLER_OFF;
		indigo_set_switch(CCD_COOLER_PROPERTY, PRIVATE_DATA->cooler_on ? CCD_COOLER_ON_ITEM : CCD_COOLER_OFF_ITEM, true);
		CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		CCD_TEMPERATURE_PROPERTY->state = PRIVATE_DATA->cooler_on ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	}
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	uint16_t gain = (uint16_t)CCD_GAIN_ITEM->number.target;
	if (gxccd_set_gain(PRIVATE_DATA->camera, gain) != 0) {
		CCD_GAIN_ITEM->number.target = CCD_GAIN_ITEM->number.value;
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_GAIN_ITEM->number.value = gain;
		float egain = 0;
		if (!CCD_EGAIN_PROPERTY->hidden && gxccd_get_value(PRIVATE_DATA->camera, GV_ADC_GAIN, &egain) == 0) {
			CCD_EGAIN_ITEM->number.value = egain;
			CCD_EGAIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EGAIN_PROPERTY, NULL);
		} else if (!CCD_EGAIN_PROPERTY->hidden) {
			CCD_EGAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EGAIN_PROPERTY, "Electronic gain readback failed");
		}
	}
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	int bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	int left = (int)CCD_FRAME_LEFT_ITEM->number.value / bin_x;
	int top = (int)CCD_FRAME_TOP_ITEM->number.value / bin_y;
	PRIVATE_DATA->image_width = (int)CCD_FRAME_WIDTH_ITEM->number.value / bin_x;
	PRIVATE_DATA->image_height = (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin_y;
	int result = gxccd_set_binning(PRIVATE_DATA->camera, bin_x, bin_y);
	if (result == 0 && !CCD_READ_MODE_PROPERTY->hidden) {
		result = gxccd_set_read_mode(PRIVATE_DATA->camera, PRIVATE_DATA->read_mode);
	}
	if (result == 0) {
		bool use_shutter = !(CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value);
		result = gxccd_start_exposure(PRIVATE_DATA->camera, CCD_EXPOSURE_ITEM->number.target, use_shutter, left, top, PRIVATE_DATA->image_width, PRIVATE_DATA->image_height);
	}
	if (result != 0) {
		exposure_failed(device, "Exposure setup failed");
		return;
	}
	PRIVATE_DATA->acquisition_active = true;
	PRIVATE_DATA->exposure_end = indigo_monotonic_time() + CCD_EXPOSURE_ITEM->number.target;
	PRIVATE_DATA->readout_deadline = PRIVATE_DATA->exposure_end + EXPOSURE_READOUT_TIMEOUT;
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, fmin(1, CCD_EXPOSURE_ITEM->number.target), exposure_finalizer);
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, ccd_exposure_handler);
	indigo_cancel_pending_handler(device, exposure_finalizer);
	int result = PRIVATE_DATA->acquisition_active ? gxccd_abort_exposure(PRIVATE_DATA->camera, false) : 0;
	PRIVATE_DATA->acquisition_active = false;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_ccd_abort_exposure_cleanup(device);
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	CCD_ABORT_EXPOSURE_PROPERTY->state = result == 0 ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	int horizontal = (int)CCD_BIN_HORIZONTAL_ITEM->number.target;
	int vertical = (int)CCD_BIN_VERTICAL_ITEM->number.target;
	bool horizontal_supported = horizontal == 1 || horizontal == 2 || horizontal == 3 || horizontal == 4 || horizontal == 8;
	bool vertical_supported = vertical == 1 || vertical == 2 || vertical == 3 || vertical == 4 || vertical == 8;
	if (!horizontal_supported || !vertical_supported || (!PRIVATE_DATA->asymmetric_binning && horizontal != vertical)) {
		CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value;
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_BIN_HORIZONTAL_ITEM->number.value = horizontal;
		CCD_BIN_VERTICAL_ITEM->number.value = vertical;
	}
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		//- ccd.on_attach
		CCD_READ_MODE_PROPERTY->hidden = true;
		CCD_COOLER_PROPERTY->hidden = true;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_COOLER_POWER_PROPERTY->hidden = true;
		CCD_GAIN_PROPERTY->hidden = true;
		CCD_EGAIN_PROPERTY->hidden = true;
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
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
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_READ_MODE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_READ_MODE_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_READ_MODE_PROPERTY, ccd_read_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_GAIN_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_BIN_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
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
			connection_result = mi_open(device->master_device);
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
				mi_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_lock_master_device(device);
		gxccd_move_telescope(PRIVATE_DATA->camera, 0, 0);
		PRIVATE_DATA->guider_ra_duration = PRIVATE_DATA->guider_dec_duration = 0;
		PRIVATE_DATA->guider_ra_end = PRIVATE_DATA->guider_dec_end = 0;
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_unlock_master_device(device);
		//- guider.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			GUIDER_GUIDE_RA_PROPERTY,
			GUIDER_GUIDE_DEC_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			mi_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	// A new request replaces the running pulse, so the finaliser of the superseded
	// one must not end the new pulse on the old deadline.
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value > 0 ? GUIDER_GUIDE_EAST_ITEM->number.value : -(int)GUIDER_GUIDE_WEST_ITEM->number.value;
	int16_t ra = (int16_t)duration;
	int16_t dec = remaining_pulse(PRIVATE_DATA->guider_dec_end, PRIVATE_DATA->guider_dec_duration);
	if (gxccd_move_telescope(PRIVATE_DATA->camera, ra, dec) != 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (ra != 0) {
		PRIVATE_DATA->guider_ra_duration = ra;
		PRIVATE_DATA->guider_ra_end = indigo_monotonic_time() + abs(ra) / 1000.0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, abs(ra) / 1000.0, guider_ra_finalizer);
	} else {
		PRIVATE_DATA->guider_ra_duration = 0;
		PRIVATE_DATA->guider_ra_end = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	// A new request replaces the running pulse, so the finaliser of the superseded
	// one must not end the new pulse on the old deadline.
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? GUIDER_GUIDE_NORTH_ITEM->number.value : -(int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
	int16_t ra = remaining_pulse(PRIVATE_DATA->guider_ra_end, PRIVATE_DATA->guider_ra_duration);
	int16_t dec = (int16_t)duration;
	if (gxccd_move_telescope(PRIVATE_DATA->camera, ra, dec) != 0) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (dec != 0) {
		PRIVATE_DATA->guider_dec_duration = dec;
		PRIVATE_DATA->guider_dec_end = indigo_monotonic_time() + abs(dec) / 1000.0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, abs(dec) / 1000.0, guider_dec_finalizer);
	} else {
		PRIVATE_DATA->guider_dec_duration = 0;
		PRIVATE_DATA->guider_dec_end = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		GUIDER_GUIDE_EAST_ITEM->number.max = GUIDER_GUIDE_WEST_ITEM->number.max = INT16_MAX;
		GUIDER_GUIDE_NORTH_ITEM->number.max = GUIDER_GUIDE_SOUTH_ITEM->number.max = INT16_MAX;
		//- guider.on_attach
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
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
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

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = mi_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ wheel.on_connect
			indigo_lock_master_device(device);
			int filters = 0;
			connection_result = get_integer(device, GIP_FILTERS, &filters) && filters > 0 && filters <= WHEEL_SLOT_NAME_PROPERTY->allocated_count;
			if (connection_result) {
				WHEEL_SLOT_ITEM->number.min = 1;
				WHEEL_SLOT_ITEM->number.max = filters;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->wheel_slot = 1;
				WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = filters;
			}
			indigo_unlock_master_device(device);
			//- wheel.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				mi_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			WHEEL_SLOT_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			mi_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.WHEEL_SLOT.on_change
	int slot = (int)WHEEL_SLOT_ITEM->number.target;
	if (slot == PRIVATE_DATA->wheel_slot) {
		WHEEL_SLOT_ITEM->number.value = slot;
	} else if (gxccd_set_filter(PRIVATE_DATA->camera, slot - 1) != 0) {
		WHEEL_SLOT_ITEM->number.target = WHEEL_SLOT_ITEM->number.value;
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->wheel_slot = slot;
		WHEEL_SLOT_ITEM->number.value = slot;
	}
	//- wheel.WHEEL_SLOT.on_change
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, wheel_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

#define SDK_DISCOVERY_RETRIES (6)
typedef struct sdk_discovery_retry {
	libusb_device *dev;
	int remaining;
	bool active, queued;
	struct sdk_discovery_retry *next;
} sdk_discovery_retry;

static sdk_discovery_retry *sdk_discovery_retries;
static bool sdk_discovery_stopping;
static void process_plug_event_handler(indigo_device *device, void *data);
static void process_sdk_retry_handler(indigo_device *device, void *data);

static void update_sdk_discovery_retry(libusb_device *dev, bool retry) {
	sdk_discovery_retry *entry = sdk_discovery_retries;
	while (entry && entry->dev != dev) {
		entry = entry->next;
	}
	if (!retry || sdk_discovery_stopping) {
		if (entry) {
			entry->active = false;
		}
		return;
	}
	if (!entry && SDK_DISCOVERY_RETRIES <= 0) {
		return;
	}
	if (!entry) {
		entry = (sdk_discovery_retry *)indigo_safe_malloc(sizeof(*entry));
		entry->dev = libusb_ref_device(dev);
		entry->remaining = SDK_DISCOVERY_RETRIES;
		entry->active = true;
		entry->next = sdk_discovery_retries;
		sdk_discovery_retries = entry;
	}
	if (entry->active && !entry->queued && entry->remaining > 0) {
		entry->remaining--;
		entry->queued = true;
		indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0.5, process_sdk_retry_handler, entry, &driver_queue_mutex);
	}
}

static void process_sdk_retry_handler(indigo_device *device, void *data) {
	sdk_discovery_retry *entry = (sdk_discovery_retry *)data;
	entry->queued = false;
	if (entry->active && !sdk_discovery_stopping) {
		process_plug_event_handler(NULL, libusb_ref_device(entry->dev));
	}
	if (!entry->queued) {
		sdk_discovery_retry **link = &sdk_discovery_retries;
		while (*link != entry) {
			link = &(*link)->next;
		}
		*link = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}

static void clear_sdk_discovery_retries(void) {
	while (sdk_discovery_retries) {
		sdk_discovery_retry *entry = sdk_discovery_retries;
		sdk_discovery_retries = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}
static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	if (sdk_discovery_stopping) {
		libusb_unref_device(dev);
		return;
	}
	bool dev_ref_transferred = false;
	mi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (mi_private_data *)indigo_safe_malloc(sizeof(mi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == MI_VID)) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		int count = enumerate_cameras();
		for (int i = 0; i < count; i++) {
			int eid = enumerated_ids[i];
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && ((mi_private_data *)devices[slot]->private_data)->eid == eid) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			camera_t *camera = gxccd_initialize_usb(eid);
			if (camera == NULL) {
				continue;
			}
			char description[INDIGO_NAME_SIZE] = "Camera";
			bool has_guider = false;
			bool has_wheel = false;
			if (gxccd_get_string_parameter(camera, GSP_CAMERA_DESCRIPTION, description, sizeof(description)) != 0) {
				snprintf(description, sizeof(description), "Camera");
			}
			if (gxccd_get_boolean_parameter(camera, GBP_GUIDE, &has_guider) != 0) {
				has_guider = false;
			}
			if (gxccd_get_boolean_parameter(camera, GBP_FILTERS, &has_wheel) != 0) {
				has_wheel = false;
			}
			gxccd_release(camera);
			int free_slots = 0;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] == NULL) {
					free_slots++;
				}
			}
			if (free_slots < 1 + (has_guider ? 1 : 0) + (has_wheel ? 1 : 0)) {
				continue;
			}
			description[sizeof(description) - 1] = 0;
			trim_trailing_space(description);
			private_data->eid = eid;
			private_data->has_guider = has_guider;
			private_data->has_wheel = has_wheel;
			snprintf(private_data->model, sizeof(private_data->model), "MI %s", description[0] ? description : "Camera");
			snprintf(name, INDIGO_NAME_SIZE, "%s", private_data->model);
			indigo_make_name_unique(name, "%d", eid);
			snprintf(private_data->guider_name, sizeof(private_data->guider_name), "%.*s (guider)", INDIGO_NAME_SIZE - 11, private_data->model);
			indigo_make_name_unique(private_data->guider_name, "%d", eid);
			snprintf(private_data->wheel_name, sizeof(private_data->wheel_name), "%.*s (wheel)", INDIGO_NAME_SIZE - 10, private_data->model);
			indigo_make_name_unique(private_data->wheel_name, "%d", eid);
			plug_result = true;
			break;
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
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
		if (ccd_attached && private_data->has_guider) {
		indigo_device *guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		guider->private_data = private_data;
		guider->master_device = ccd;
		snprintf(guider->name, INDIGO_NAME_SIZE, "%s", private_data->guider_name);
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
		if (ccd_attached && private_data->has_wheel) {
		indigo_device *wheel = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		wheel->private_data = private_data;
		wheel->master_device = ccd;
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s", private_data->wheel_name);
		bool wheel_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = wheel;
				if (indigo_attach_device(wheel) == INDIGO_OK) {
					dev_ref_transferred = true;
					wheel_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!wheel_attached) {
			indigo_safe_free(wheel);
		}
		}
	}
	update_sdk_discovery_retry(dev, discovery_eligible && !dev_ref_transferred);
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	update_sdk_discovery_retry(dev, false);
	mi_private_data *private_data = NULL;
	mi_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (!unplug_result && last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				unplug_result = !camera_is_enumerated(private_data->eid);
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
				bool recorded = false;
				for (int k = 0; k < removed_count; k++) {
					if (removed[k] == private_data) {
						recorded = true;
						break;
					}
				}
				if (!recorded) {
					removed[removed_count++] = private_data;
				}
			}
		}
	}
	for (int k = 0; k < removed_count; k++) {
		libusb_unref_device(removed[k]->usbdev);
		indigo_safe_free(removed[k]);
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

indigo_result indigo_ccd_mi(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			sdk_discovery_stopping = false;
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, MI_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
			if (shutdown_result == INDIGO_OK) {
				sdk_discovery_stopping = true;
			}
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_remove(driver_queue, NULL, (indigo_timer_callback)process_sdk_retry_handler);
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			clear_sdk_discovery_retries();
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
