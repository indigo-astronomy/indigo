// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
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

/** INDIGO Moravian Instruments CCD driver
 \file indigo_ccd_mi.c
 */

#define DRIVER_VERSION 0x000A
#define DRIVER_NAME "indigo_ccd_mi"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_usb_utils.h>
#include <indigo/indigo_driver_xml.h>

#include "indigo_ccd_mi.h"

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <gxccd.h>

#define MI_VID							0x1347
#define PRIVATE_DATA				((mi_private_data *)device->private_data)
#define CCD_READ_MODE_ITEM	(CCD_READ_MODE_PROPERTY->items+0)

#define TEMP_PERIOD					5
#define TEMP_COOLER_OFF			100
#define POWER_UTIL_PERIOD		10

typedef struct {
	int eid;
	camera_t *camera;
	int device_count;
	indigo_timer *exposure_timer, *temperature_timer, *power_util_timer, *guider_timer;
	float target_temperature, current_temperature;
	unsigned char *buffer;
	int read_mode;
	int image_width;
	int image_height;
	bool downloading;
	bool enumerated;
} mi_private_data;

static void mi_report_error(indigo_device *device, indigo_property *property) {
	char buffer[128];
	gxccd_get_last_error(PRIVATE_DATA->camera, buffer, sizeof(buffer));
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, buffer);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;
	if (!IS_CONNECTED)
		return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->downloading = true;
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		int state = 0;
		bool ready = false;
		state = gxccd_image_ready(PRIVATE_DATA->camera, &ready);
		while (state != -1 && !ready) {
			indigo_usleep(200);
			state = gxccd_image_ready(PRIVATE_DATA->camera, &ready);
		}
		if (state != -1)
			state = gxccd_read_image(PRIVATE_DATA->camera, (char *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), PRIVATE_DATA->image_width * PRIVATE_DATA->image_height * 2);
		if (state != -1) {
			indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->image_width, PRIVATE_DATA->image_height, 16, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			mi_report_error(device, CCD_EXPOSURE_PROPERTY);
		}
		PRIVATE_DATA->downloading = false;
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	if (!PRIVATE_DATA->downloading) {
		int state = gxccd_get_value(PRIVATE_DATA->camera, GV_CHIP_TEMPERATURE, &PRIVATE_DATA->current_temperature);
		if (state != -1) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 1 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10) / 10;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		} else {
			mi_report_error(device, CCD_TEMPERATURE_PROPERTY);
		}
	}
	indigo_reschedule_timer(device, TEMP_PERIOD, &PRIVATE_DATA->temperature_timer);
}

static void ccd_power_util_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	if (!PRIVATE_DATA->downloading) {
		float float_value;
		int state = gxccd_get_value(PRIVATE_DATA->camera, GV_POWER_UTILIZATION, &float_value);
		if (state != -1) {
			CCD_COOLER_POWER_ITEM->number.value = round(float_value * 1000) / 10;
			indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		} else {
			mi_report_error(device, CCD_COOLER_POWER_PROPERTY);
		}
	}
	indigo_reschedule_timer(device, POWER_UTIL_PERIOD, &PRIVATE_DATA->power_util_timer);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->device_count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				PRIVATE_DATA->camera = NULL;
			} else {
				PRIVATE_DATA->camera = gxccd_initialize_usb(PRIVATE_DATA->eid);
			}
		}
		if (PRIVATE_DATA->camera) {
			int int_value;
			bool bool_value;
			float float_value;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_CHIP_W, &int_value);
			CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = int_value;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_CHIP_D, &int_value);
			CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = int_value;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_PIXEL_W, &int_value);
			CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(int_value / 100.0) / 10.0;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_PIXEL_D, &int_value);
			CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(int_value / 100.0) / 10.0;
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
			CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MAX_BINNING_X, &int_value);
			CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
			CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = int_value;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MAX_BINNING_Y, &int_value);
			CCD_BIN_VERTICAL_ITEM->number.min = 1;
			CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = int_value;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MINIMAL_EXPOSURE, &int_value);
			if (int_value > 0)
				CCD_EXPOSURE_ITEM->number.min = int_value / 1000000.0;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MAXIMAL_EXPOSURE, &int_value);
			if (int_value > 0)
				CCD_EXPOSURE_ITEM->number.max = int_value / 1000.0;

			int_value = 1;
			CCD_MODE_PROPERTY->count = 0;
			while (int_value <= CCD_BIN_HORIZONTAL_ITEM->number.max && int_value <= CCD_BIN_VERTICAL_ITEM->number.max) {
				char name[32], description[32];
				sprintf(name, "BIN_%dx%d", int_value, int_value);
				sprintf(description, "RAW 16 %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value / int_value, (int)CCD_INFO_HEIGHT_ITEM->number.value / int_value);
				indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, description, int_value == 1);
				CCD_MODE_PROPERTY->count++;
				int_value *= 2;
			}
			CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;

			int_value = 0;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_READ_MODES, &int_value);
			if (int_value == 0) {
				CCD_READ_MODE_PROPERTY->count = 0;
				CCD_READ_MODE_PROPERTY->hidden = true;
				PRIVATE_DATA->read_mode = 0;
			} else {
				CCD_READ_MODE_PROPERTY = indigo_resize_property(CCD_READ_MODE_PROPERTY, int_value);
				CCD_READ_MODE_PROPERTY->hidden = false;
				gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_DEFAULT_READ_MODE, &PRIVATE_DATA->read_mode);
				char name[32], description[32];
				for (int i = 0; i < int_value; i++) {
					gxccd_enumerate_read_modes(PRIVATE_DATA->camera, i, description, sizeof(description));
					sprintf(name, "READ_MODE%d", i);
					indigo_init_switch_item(CCD_READ_MODE_ITEM + i, name, description, i == PRIVATE_DATA->read_mode);
				}
			}

			gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_COOLER, &bool_value);
			if (bool_value) {
				CCD_COOLER_PROPERTY->hidden = false;
				PRIVATE_DATA->target_temperature = 0;
			} else {
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			}
			CCD_TEMPERATURE_PROPERTY->hidden = false;
			indigo_set_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);

			gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_POWER_UTILIZATION, &bool_value);
			if (bool_value) {
				CCD_COOLER_POWER_PROPERTY->hidden = false;
				indigo_set_timer(device, 0, ccd_power_util_callback, &PRIVATE_DATA->power_util_timer);
			}

			gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_GAIN, &bool_value);
			if (bool_value) {
				int_value = 0;
				gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_MAX_GAIN, &int_value);
				if (int_value > 0) { // it is possible to set camera gain
					CCD_GAIN_PROPERTY->hidden = false;
					CCD_GAIN_ITEM->number.min = 0.0;
					CCD_GAIN_ITEM->number.max = int_value;
					CCD_GAIN_ITEM->number.step = 1.0;
					CCD_GAIN_ITEM->number.value = 0.0;
				} else if (gxccd_get_value(PRIVATE_DATA->camera, GV_ADC_GAIN, &float_value) != -1) {
					CCD_GAIN_PROPERTY->hidden = false;
					CCD_GAIN_PROPERTY->perm = INDIGO_RO_PERM;
					CCD_GAIN_ITEM->number.min = 0.0;
					CCD_GAIN_ITEM->number.step = 0.1;
					CCD_GAIN_ITEM->number.value = float_value;
				}
			}

			PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
			assert(PRIVATE_DATA->buffer != NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->power_util_timer);
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			gxccd_abort_exposure(PRIVATE_DATA->camera, false);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
		}
		PRIVATE_DATA->downloading = false;
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		if (--PRIVATE_DATA->device_count == 0) {
			gxccd_release(PRIVATE_DATA->camera);
			PRIVATE_DATA->camera = NULL;
			indigo_global_unlock(device);
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
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE, CCD_BIN
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		int bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		int left = (int)CCD_FRAME_LEFT_ITEM->number.value / bin_x;
		int top = (int)CCD_FRAME_TOP_ITEM->number.value / bin_y;
		int width = PRIVATE_DATA->image_width = (int)CCD_FRAME_WIDTH_ITEM->number.value / bin_x;
		int height = PRIVATE_DATA->image_height = (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin_y;
		int state = gxccd_set_binning(PRIVATE_DATA->camera, bin_x, bin_y);
		if (state != -1 && !CCD_READ_MODE_PROPERTY->hidden) // do not set read mode to cameras without it
			state = gxccd_set_read_mode(PRIVATE_DATA->camera, PRIVATE_DATA->read_mode);
		if (state != -1)
			state = gxccd_start_exposure(PRIVATE_DATA->camera, CCD_EXPOSURE_ITEM->number.target, !(CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value), left, top, width, height);
		if (state != -1)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		else {
			mi_report_error(device, CCD_EXPOSURE_PROPERTY);
			return INDIGO_OK;
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			gxccd_abort_exposure(PRIVATE_DATA->camera, false);
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
		}
		PRIVATE_DATA->downloading = false;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_READ_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_READ_MODE
		indigo_property_copy_values(CCD_READ_MODE_PROPERTY, property, false);
		if (IS_CONNECTED && !CCD_READ_MODE_PROPERTY->hidden) {
			for (int i = 0; i < CCD_READ_MODE_PROPERTY->count; i++) {
				indigo_item *item = CCD_READ_MODE_ITEM + i;
				if (item && item->sw.value) {
					PRIVATE_DATA->read_mode = i;
					gxccd_set_read_mode(PRIVATE_DATA->camera, PRIVATE_DATA->read_mode);
					break;
				}
			}
			indigo_update_property(device, CCD_READ_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		int h = CCD_BIN_HORIZONTAL_ITEM->number.value;
		int v = CCD_BIN_VERTICAL_ITEM->number.value;
		bool bool_value;
		gxccd_get_boolean_parameter(PRIVATE_DATA->camera, GBP_ASYMMETRIC_BINNING, &bool_value);
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		if (!(CCD_BIN_HORIZONTAL_ITEM->number.value == 1 || CCD_BIN_HORIZONTAL_ITEM->number.value == 2 || CCD_BIN_HORIZONTAL_ITEM->number.value == 4 || CCD_BIN_HORIZONTAL_ITEM->number.value == 8) || (CCD_BIN_HORIZONTAL_ITEM->number.value != CCD_BIN_VERTICAL_ITEM->number.value && !bool_value)) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = h;
			CCD_BIN_VERTICAL_ITEM->number.value = v;
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			}
			return INDIGO_OK;
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (IS_CONNECTED && !CCD_COOLER_PROPERTY->hidden) {
			if (CCD_COOLER_ON_ITEM->sw.value) {
				PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
				CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10) / 10;
				gxccd_set_temperature(PRIVATE_DATA->camera, PRIVATE_DATA->target_temperature);
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			} else {
				gxccd_set_temperature(PRIVATE_DATA->camera, TEMP_COOLER_OFF);
			}
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (IS_CONNECTED && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10) / 10;
			gxccd_set_temperature(PRIVATE_DATA->camera, PRIVATE_DATA->target_temperature);
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		if (IS_CONNECTED && !CCD_GAIN_PROPERTY->hidden) {
			gxccd_set_gain(PRIVATE_DATA->camera, CCD_GAIN_ITEM->number.value);
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
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

static void guider_timer_callback(indigo_device *device) {
	PRIVATE_DATA->guider_timer = NULL;
	if (!IS_CONNECTED)
		return;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value != 0 || GUIDER_GUIDE_SOUTH_ITEM->number.value != 0) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	if (GUIDER_GUIDE_EAST_ITEM->number.value != 0 || GUIDER_GUIDE_WEST_ITEM->number.value != 0) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

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
		if (PRIVATE_DATA->device_count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				PRIVATE_DATA->camera = NULL;
			} else {
				PRIVATE_DATA->camera = gxccd_initialize_usb(PRIVATE_DATA->eid);
			}
		}
		if (PRIVATE_DATA->camera) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer);
		if (--PRIVATE_DATA->device_count == 0) {
			gxccd_release(PRIVATE_DATA->camera);
			indigo_global_unlock(device);
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
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			gxccd_move_telescope(PRIVATE_DATA->camera, 0, duration);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				gxccd_move_telescope(PRIVATE_DATA->camera, 0, -duration);
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			gxccd_move_telescope(PRIVATE_DATA->camera, duration, 0);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				gxccd_move_telescope(PRIVATE_DATA->camera, -duration, 0);
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
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

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->device_count++ == 0) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				PRIVATE_DATA->camera = NULL;
			} else {
				PRIVATE_DATA->camera = gxccd_initialize_usb(PRIVATE_DATA->eid);
			}
		}
		if (PRIVATE_DATA->camera) {
			int int_value;
			gxccd_get_integer_parameter(PRIVATE_DATA->camera, GIP_FILTERS, &int_value);
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = int_value;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			gxccd_release(PRIVATE_DATA->camera);
			indigo_global_unlock(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
			gxccd_set_filter(PRIVATE_DATA->camera, (int)WHEEL_SLOT_ITEM->number.value - 1);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10

static indigo_device *devices[MAX_DEVICES];
static int new_eid = -1;
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void callback(int eid) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device && PRIVATE_DATA->eid == eid) {
			PRIVATE_DATA->enumerated = true;
			return;
		}
	}
	new_eid = eid;
}

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
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);
	pthread_mutex_lock(&device_mutex);
	new_eid = -1;
	gxccd_enumerate_usb(callback);
	if (new_eid != -1) {
		camera_t *camera = gxccd_initialize_usb(new_eid);
		if (camera) {
			char name[128] = "MI ";
			bool is_guider, has_wheel;
			gxccd_get_string_parameter(camera, GSP_CAMERA_DESCRIPTION, name + 3, sizeof(name) - 3);
			gxccd_get_boolean_parameter(camera, GBP_GUIDE, &is_guider);
			gxccd_get_boolean_parameter(camera, GBP_FILTERS, &has_wheel);
			gxccd_release(camera);
			char *end = name + strlen(name) - 1;
			while(end > name && isspace((unsigned char)*end))
				end--;
			end[1] = '\0';
			mi_private_data *private_data = indigo_safe_malloc(sizeof(mi_private_data));
			private_data->eid = new_eid;
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			indigo_device *master_device = device;
			device->master_device = master_device;
			snprintf(device->name, INDIGO_NAME_SIZE, "%s #%d", name, new_eid);
			device->private_data = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
			if (is_guider) {
				device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
				device->master_device = master_device;
				snprintf(device->name, INDIGO_NAME_SIZE, "%s (guider) #%d", name, new_eid);
				device->private_data = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_attach_device(devices[j] = device);
						break;
					}
				}
			}
			if (has_wheel) {
				device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
				device->master_device = master_device;
				snprintf(device->name, INDIGO_NAME_SIZE, "%s (wheel) #%d", name, new_eid);
				device->private_data = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_attach_device(devices[j] = device);
						break;
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&device_mutex);
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device)
			PRIVATE_DATA->enumerated = false;
	}
	gxccd_enumerate_usb(callback);
	for (int i = MAX_DEVICES - 1; i >=0; i--) {
		indigo_device *device = devices[i];
		if (device && !PRIVATE_DATA->enumerated) {
			indigo_detach_device(device);
			if (device->master_device == device) {
				mi_private_data *private_data = PRIVATE_DATA;
				if (private_data->buffer != NULL)
					free(private_data->buffer);
				free(private_data);
			}
			free(device);
			devices[i] = NULL;
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_mi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Moravian Instruments Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, MI_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++)
				VERIFY_NOT_CONNECTED(devices[i]);
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");

			for (int i = MAX_DEVICES - 1; i >=0; i--) {
				indigo_device *device = devices[i];
				if (device) {
					indigo_detach_device(device);
					if (device->master_device == device) {
						mi_private_data *private_data = PRIVATE_DATA;
						if (private_data->buffer != NULL)
							free(private_data->buffer);
						free(private_data);
					}
					free(device);
					devices[i] = NULL;
				}
			}

			break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
