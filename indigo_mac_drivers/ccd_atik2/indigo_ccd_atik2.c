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

// This file generated from indigo_ccd_atik2.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <indigo/indigo_client.h>
#include "wheel_atik/bin_externals/libatik/include/libatik.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_atik2.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000009
#define DRIVER_NAME          "indigo_ccd_atik2"
#define DRIVER_LABEL         "Atik (legacy) Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s (guider)"
#define WHEEL_DEVICE_NAME    "%s (wheel)"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((atik2_private_data *)device->private_data)

//+ define

#define ATIK2_READOUT_TIMEOUT 120
#define ATIK2_WHEEL_TIMEOUT  60

//- define

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	//+ data
	libatik_device_context *device_context;
	char guider_name[INDIGO_NAME_SIZE], wheel_name[INDIGO_NAME_SIZE];
	bool has_guider, has_wheel, acquisition_active, exposure_started;
	unsigned short relay_mask;
	unsigned char *buffer;
	size_t buffer_size;
	double exposure_duration, exposure_deadline;
	int exp_left, exp_top, exp_width, exp_height, exp_bx, exp_by;
	double cooler_power, current_temperature;
	int target_slot, current_slot;
	double wheel_deadline;
	//- data
} atik2_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

static void exposure_finalizer(indigo_device *device);

static bool atik2_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		return false;
	}
	if (!libatik_open(PRIVATE_DATA->usbdev, &PRIVATE_DATA->device_context) || !PRIVATE_DATA->device_context) {
		PRIVATE_DATA->device_context = NULL;
		indigo_global_unlock(device);
		return false;
	}
	return true;
}

static void atik2_close(indigo_device *device) {
	libatik_close(PRIVATE_DATA->device_context);
	PRIVATE_DATA->device_context = NULL;
	indigo_global_unlock(device);
}

static bool atik2_initialize_ccd(indigo_device *device) {
	libatik_device_context *context = PRIVATE_DATA->device_context;
	if (!context || context->width <= 0 || context->height <= 0 || !isfinite(context->pixel_width) || !isfinite(context->pixel_height) || context->pixel_width <= 0 || context->pixel_height <= 0 || !isfinite(context->min_exposure) || context->min_exposure <= 0 || context->max_bin_hor < 1 || context->max_bin_vert < 1 || context->max_bin_hor > context->width || context->max_bin_vert > context->height || (size_t)context->width > (SIZE_MAX - FITS_HEADER_SIZE) / 2 / context->height) {
		return false;
	}
	CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = context->width;
	CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = context->height;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(context->pixel_width * 100) / 100;
	CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(context->pixel_height * 100) / 100;
	CCD_EXPOSURE_ITEM->number.min = context->min_exposure;
	CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = context->max_bin_hor;
	CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = context->max_bin_vert;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = 1;
	int max_bin = context->max_bin_hor < context->max_bin_vert ? context->max_bin_hor : context->max_bin_vert;
	int modes = 0;
	for (int bin = 1; bin <= max_bin; bin *= 2) {
		modes++;
		if (bin > INT_MAX / 2) {
			break;
		}
	}
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, modes);
	CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
	for (int i = 0, bin = 1; i < modes; i++, bin *= 2) {
		char name[32], label[64];
		snprintf(name, sizeof(name), "BIN_%dx%d", bin, bin);
		snprintf(label, sizeof(label), "RAW 16 %dx%d", context->width / bin, context->height / bin);
		indigo_init_switch_item(CCD_MODE_PROPERTY->items + i, name, label, i == 0);
	}
	CCD_TEMPERATURE_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = !context->has_cooler;
	if (context->has_cooler) {
		bool status = false;
		if (!libatik_check_cooler(context, &status, &PRIVATE_DATA->cooler_power, &PRIVATE_DATA->current_temperature) || !isfinite(PRIVATE_DATA->cooler_power) || PRIVATE_DATA->cooler_power < 0 || PRIVATE_DATA->cooler_power > 100 || !isfinite(PRIVATE_DATA->current_temperature)) {
			return false;
		}
		indigo_set_switch(CCD_COOLER_PROPERTY, status ? CCD_COOLER_ON_ITEM : CCD_COOLER_OFF_ITEM, true);
		CCD_COOLER_PROPERTY->state = CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
		CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_TEMPERATURE_ITEM->number.value = CCD_TEMPERATURE_ITEM->number.target = round(PRIVATE_DATA->current_temperature * 10) / 10;
		CCD_COOLER_POWER_ITEM->number.value = round(PRIVATE_DATA->cooler_power);
	}
	PRIVATE_DATA->buffer_size = 2 * (size_t)context->width * context->height + FITS_HEADER_SIZE;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	return PRIVATE_DATA->buffer != NULL;
}

static void atik2_exposure_failure(indigo_device *device, const char *message) {
	if (PRIVATE_DATA->exposure_started) {
		libatik_abort_exposure(PRIVATE_DATA->device_context);
	}
	PRIVATE_DATA->exposure_started = PRIVATE_DATA->acquisition_active = false;
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_ccd_failure_cleanup(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "%s", message);
}

static bool atik2_read_image(indigo_device *device, double delay) {
	int width = 0, height = 0;
	if (!libatik_read_pixels(PRIVATE_DATA->device_context, delay, CCD_READ_MODE_HIGH_SPEED_ITEM->sw.value, PRIVATE_DATA->exp_left, PRIVATE_DATA->exp_top, PRIVATE_DATA->exp_width, PRIVATE_DATA->exp_height, PRIVATE_DATA->exp_bx, PRIVATE_DATA->exp_by, (unsigned short *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), &width, &height) || width <= 0 || height <= 0 || width != PRIVATE_DATA->exp_width / PRIVATE_DATA->exp_bx || height != PRIVATE_DATA->exp_height / PRIVATE_DATA->exp_by || (size_t)width > (PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) / 2 / height) {
		return false;
	}
	indigo_process_image(device, PRIVATE_DATA->buffer, width, height, 16, true, true, NULL, false);
	return true;
}

static void exposure_finalizer(indigo_device *device) {
	if (!PRIVATE_DATA->acquisition_active) {
		return;
	}
	if (!IS_CONNECTED || indigo_monotonic_time() >= PRIVATE_DATA->exposure_deadline || !atik2_read_image(device, 0) || indigo_monotonic_time() >= PRIVATE_DATA->exposure_deadline) {
		atik2_exposure_failure(device, "Exposure failed or timed out");
		return;
	}
	PRIVATE_DATA->exposure_started = PRIVATE_DATA->acquisition_active = false;
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

//- code

//+ guider.code

static void guider_ra_finalizer(indigo_device *device) {
	unsigned short mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_EAST | ATIK_GUIDE_WEST);
	GUIDER_GUIDE_RA_PROPERTY->state = libatik_guide_relays(PRIVATE_DATA->device_context, mask) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_OK_STATE) {
		PRIVATE_DATA->relay_mask = mask;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	unsigned short mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH);
	GUIDER_GUIDE_DEC_PROPERTY->state = libatik_guide_relays(PRIVATE_DATA->device_context, mask) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_OK_STATE) {
		PRIVATE_DATA->relay_mask = mask;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

//- guider.code

//+ wheel.code

static bool atik2_wheel_position(indigo_device *device, int *position, bool allow_moving) {
	return libatik_check_filter_wheel(PRIVATE_DATA->device_context, position) && ((*position >= 1 && *position <= PRIVATE_DATA->device_context->filter_count) || (allow_moving && *position == 0));
}

static void wheel_move_finalizer(indigo_device *device) {
	int position = 0;
	if (!atik2_wheel_position(device, &position, true) || indigo_monotonic_time() >= PRIVATE_DATA->wheel_deadline) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		if (position > 0) {
			PRIVATE_DATA->current_slot = WHEEL_SLOT_ITEM->number.value = position;
		}
		if (position == PRIVATE_DATA->target_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, .5, wheel_move_finalizer);
		}
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

//- wheel.code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	if (PRIVATE_DATA->acquisition_active) {
		indigo_execute_handler_in(device, 5, ccd_timer_callback);
		return;
	}
	if (!CCD_TEMPERATURE_PROPERTY->hidden) {
		bool status = false;
		double power = 0, temperature = 0;
		if (libatik_check_cooler(PRIVATE_DATA->device_context, &status, &power, &temperature) && isfinite(power) && power >= 0 && power <= 100 && isfinite(temperature)) {
			PRIVATE_DATA->cooler_power = power;
			PRIVATE_DATA->current_temperature = temperature;
			CCD_TEMPERATURE_ITEM->number.value = round(temperature * 10) / 10;
			CCD_COOLER_POWER_ITEM->number.value = round(power);
			CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value && fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 1 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_PROPERTY->state = status == CCD_COOLER_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_POWER_PROPERTY->state = CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 5, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = atik2_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
			indigo_lock_master_device(device);
			connection_result = atik2_initialize_ccd(device);
			indigo_unlock_master_device(device);
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, ccd_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				atik2_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		indigo_lock_master_device(device);
		if (PRIVATE_DATA->exposure_started) {
			libatik_abort_exposure(PRIVATE_DATA->device_context);
		}
		PRIVATE_DATA->exposure_started = PRIVATE_DATA->acquisition_active = false;
		indigo_safe_free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
		PRIVATE_DATA->buffer_size = 0;
		indigo_unlock_master_device(device);
		//- ccd.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			atik2_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	int horizontal = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical = CCD_BIN_VERTICAL_ITEM->number.value;
	if (horizontal != vertical || horizontal < 1 || (horizontal & (horizontal - 1)) != 0 || horizontal > PRIVATE_DATA->device_context->max_bin_hor || vertical > PRIVATE_DATA->device_context->max_bin_vert) {
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		char name[32];
		snprintf(name, sizeof(name), "BIN_%dx%d", horizontal, vertical);
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			CCD_MODE_PROPERTY->items[i].sw.value = !strcmp(name, CCD_MODE_PROPERTY->items[i].name);
		}
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	}
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	if (PRIVATE_DATA->exposure_started && !libatik_abort_exposure(PRIVATE_DATA->device_context)) {
		atik2_exposure_failure(device, "Previous exposure could not be stopped");
		return;
	}
	indigo_use_shortest_exposure_if_bias(device);
	PRIVATE_DATA->exposure_duration = fmax(CCD_EXPOSURE_ITEM->number.target, PRIVATE_DATA->device_context->min_exposure);
	PRIVATE_DATA->exp_bx = CCD_BIN_HORIZONTAL_ITEM->number.value;
	PRIVATE_DATA->exp_by = CCD_BIN_VERTICAL_ITEM->number.value;
	PRIVATE_DATA->exp_left = ((int)CCD_FRAME_LEFT_ITEM->number.value / PRIVATE_DATA->exp_bx) * PRIVATE_DATA->exp_bx;
	PRIVATE_DATA->exp_top = ((int)CCD_FRAME_TOP_ITEM->number.value / PRIVATE_DATA->exp_by) * PRIVATE_DATA->exp_by;
	PRIVATE_DATA->exp_width = ((int)CCD_FRAME_WIDTH_ITEM->number.value / PRIVATE_DATA->exp_bx) * PRIVATE_DATA->exp_bx;
	PRIVATE_DATA->exp_height = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / PRIVATE_DATA->exp_by) * PRIVATE_DATA->exp_by;
	PRIVATE_DATA->acquisition_active = true;
	PRIVATE_DATA->exposure_started = false;
	CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = PRIVATE_DATA->exposure_duration;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	if (PRIVATE_DATA->exposure_duration < 1) {
		if (!atik2_read_image(device, PRIVATE_DATA->exposure_duration)) {
			atik2_exposure_failure(device, "Exposure failed");
		} else {
			PRIVATE_DATA->acquisition_active = false;
			CCD_EXPOSURE_ITEM->number.value = 0;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		}
	} else if (!libatik_start_exposure(PRIVATE_DATA->device_context, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value)) {
		atik2_exposure_failure(device, "Exposure start failed");
	} else {
		PRIVATE_DATA->exposure_started = true;
		PRIVATE_DATA->exposure_deadline = indigo_monotonic_time() + PRIVATE_DATA->exposure_duration + ATIK2_READOUT_TIMEOUT;
		indigo_execute_handler_in(device, PRIVATE_DATA->exposure_duration, exposure_finalizer);
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, ccd_exposure_handler);
	indigo_cancel_pending_handler(device, exposure_finalizer);
	bool result = !PRIVATE_DATA->exposure_started || libatik_abort_exposure(PRIVATE_DATA->device_context);
	PRIVATE_DATA->exposure_started = !result;
	PRIVATE_DATA->acquisition_active = false;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_ccd_failure_cleanup(device);
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure aborted");
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	CCD_ABORT_EXPOSURE_PROPERTY->state = result ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_COOLER.on_change
	if (!libatik_set_cooler(PRIVATE_DATA->device_context, CCD_COOLER_ON_ITEM->sw.value, CCD_TEMPERATURE_ITEM->number.target)) {
		CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	if (libatik_set_cooler(PRIVATE_DATA->device_context, true, CCD_TEMPERATURE_ITEM->number.target)) {
		indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
		CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = 16;
		//- ccd.on_attach
		CCD_READ_MODE_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
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
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		//+ ccd.CCD_BIN.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, "Exposure in progress");
			return INDIGO_OK;
		}
		//- ccd.CCD_BIN.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		//+ ccd.CCD_TEMPERATURE.on_change_request
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		//- ccd.CCD_TEMPERATURE.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
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
			connection_result = atik2_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			indigo_lock_master_device(device);
			connection_result = libatik_guide_relays(PRIVATE_DATA->device_context, 0);
			PRIVATE_DATA->relay_mask = 0;
			indigo_unlock_master_device(device);
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				atik2_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_lock_master_device(device);
		libatik_guide_relays(PRIVATE_DATA->device_context, 0);
		PRIVATE_DATA->relay_mask = 0;
		indigo_unlock_master_device(device);
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			atik2_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	unsigned short mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_EAST | ATIK_GUIDE_WEST);
	double duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	if (duration > 0) {
		mask |= ATIK_GUIDE_EAST;
	} else if ((duration = GUIDER_GUIDE_WEST_ITEM->number.value) > 0) {
		mask |= ATIK_GUIDE_WEST;
	}
	if (!libatik_guide_relays(PRIVATE_DATA->device_context, mask)) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->relay_mask = mask;
		GUIDER_GUIDE_RA_PROPERTY->state = duration > 0 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (duration > 0) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_ra_finalizer);
		}
	}
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	unsigned short mask = PRIVATE_DATA->relay_mask & ~(ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH);
	double duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	if (duration > 0) {
		mask |= ATIK_GUIDE_NORTH;
	} else if ((duration = GUIDER_GUIDE_SOUTH_ITEM->number.value) > 0) {
		mask |= ATIK_GUIDE_SOUTH;
	}
	if (!libatik_guide_relays(PRIVATE_DATA->device_context, mask)) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->relay_mask = mask;
		GUIDER_GUIDE_DEC_PROPERTY->state = duration > 0 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (duration > 0) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_dec_finalizer);
		}
	}
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
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
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
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

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = atik2_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ wheel.on_connect
			indigo_lock_master_device(device);
			int count = PRIVATE_DATA->device_context->filter_count;
			int position = 0;
			connection_result = count >= 1 && count <= 64 && atik2_wheel_position(device, &position, false);
			if (connection_result) {
				WHEEL_SLOT_NAME_PROPERTY = indigo_resize_property(WHEEL_SLOT_NAME_PROPERTY, count);
				WHEEL_SLOT_OFFSET_PROPERTY = indigo_resize_property(WHEEL_SLOT_OFFSET_PROPERTY, count);
				for (int i = 0; i < count; i++) {
					if (!WHEEL_SLOT_NAME_PROPERTY->items[i].name[0]) {
						char name[32], label[32];
						snprintf(name, sizeof(name), WHEEL_SLOT_NAME_ITEM_NAME, i + 1);
						snprintf(label, sizeof(label), "Slot %d", i + 1);
						indigo_init_text_item(WHEEL_SLOT_NAME_PROPERTY->items + i, name, label, "Filter #%d", i + 1);
						snprintf(name, sizeof(name), WHEEL_SLOT_OFFSET_ITEM_NAME, i + 1);
						indigo_init_number_item(WHEEL_SLOT_OFFSET_PROPERTY->items + i, name, label, -1000000, 1000000, 1, 0);
					}
				}
				WHEEL_SLOT_ITEM->number.max = count;
				PRIVATE_DATA->target_slot = PRIVATE_DATA->current_slot = position;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = position;
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
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
				atik2_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			atik2_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.target;
	if (PRIVATE_DATA->target_slot == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
	(void)libatik_set_filter_wheel(PRIVATE_DATA->device_context, PRIVATE_DATA->target_slot);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->wheel_deadline = indigo_monotonic_time() + ATIK2_WHEEL_TIMEOUT;
		indigo_execute_handler_in(device, .5, wheel_move_finalizer);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	//- wheel.WHEEL_SLOT.on_change
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
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
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, wheel_connection_handler, &driver_queue_mutex);
		}
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

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	atik2_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (atik2_private_data *)indigo_safe_malloc(sizeof(atik2_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (libusb_get_device_descriptor(dev, &descriptor) != LIBUSB_SUCCESS) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		if (descriptor.idVendor == ATIK_VID1 || descriptor.idVendor == ATIK_VID2) {
			bool duplicate = false;
			int available = 0;
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (!devices[i]) {
					available++;
				} else if (((atik2_private_data *)devices[i]->private_data)->usbdev == dev) {
					duplicate = true;
				}
			}
			libatik_camera_type type;
			const char *model = NULL;
			bool has_guider = false, has_wheel = false;
			if (!duplicate && libatik_camera(dev, &type, &model, &has_guider, &has_wheel) && model && model[0] && available >= 1 + has_guider + has_wheel) {
				private_data->has_guider = has_guider;
				private_data->has_wheel = has_wheel;
				snprintf(name, INDIGO_NAME_SIZE, "%s", model);
				snprintf(private_data->guider_name, INDIGO_NAME_SIZE, "%.*s (guider)", INDIGO_NAME_SIZE - 10, model);
				snprintf(private_data->wheel_name, INDIGO_NAME_SIZE, "%.*s (wheel)", INDIGO_NAME_SIZE - 9, model);
				char usb_path[INDIGO_NAME_SIZE] = { 0 };
				indigo_get_usb_path(dev, usb_path);
				indigo_make_name_unique(name, "%s", usb_path);
				indigo_make_name_unique(private_data->guider_name, "%s", usb_path);
				indigo_make_name_unique(private_data->wheel_name, "%s", usb_path);
				plug_result = true;
			}
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
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	atik2_private_data *private_data = NULL;
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

indigo_result indigo_ccd_atik2(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			if (indigo_driver_initialized((char *)"indigo_ccd_atik")) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Conflicting driver indigo_ccd_atik is already loaded");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			atik_log = indigo_debug;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "libatik %s (%s/%s)", libatik_version, libatik_os, libatik_arch);
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
