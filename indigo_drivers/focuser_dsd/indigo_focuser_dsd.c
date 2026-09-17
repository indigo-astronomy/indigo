// Copyright (C) 2019-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_focuser_dsd.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_dsd.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000011
#define DRIVER_NAME          "indigo_focuser_dsd"
#define DRIVER_LABEL         "Deep Sky Dad Focuser"
#define FOCUSER_DEVICE_NAME  "Focuser DSD AF"
#define PRIVATE_DATA         ((dsd_private_data *)device->private_data)

//+ define

#define DSD_AF1_AF2_BAUDRATE "9600"
#define DSD_AF3_BAUDRATE     "115200"
#define DSD_CMD_LEN          100
#define DSD_TIMEOUT          3
#define DSD_POLL_DELAY       0.5
#define DSD_MAX_POSITION     1000000
#define NO_TEMP_READING      (-127)

//- define

#pragma mark - Property definitions

#define X_DSD_MODEL_HINT_PROPERTY      (PRIVATE_DATA->x_dsd_model_hint_property)
#define X_DSD_MODEL_AF1_2_ITEM         (X_DSD_MODEL_HINT_PROPERTY->items + 0)
#define X_DSD_MODEL_AF3_ITEM           (X_DSD_MODEL_HINT_PROPERTY->items + 1)

#define X_DSD_MODEL_HINT_PROPERTY_NAME "X_DSD_MODEL_HINT"
#define X_DSD_MODEL_AF1_2_ITEM_NAME    "AF1_2"
#define X_DSD_MODEL_AF3_ITEM_NAME      "AF3"

#define X_DSD_STEP_MODE_PROPERTY         (PRIVATE_DATA->x_dsd_step_mode_property)
#define X_DSD_STEP_MODE_FULL_ITEM        (X_DSD_STEP_MODE_PROPERTY->items + 0)
#define X_DSD_STEP_MODE_HALF_ITEM        (X_DSD_STEP_MODE_PROPERTY->items + 1)
#define X_DSD_STEP_MODE_FOURTH_ITEM      (X_DSD_STEP_MODE_PROPERTY->items + 2)
#define X_DSD_STEP_MODE_EIGTH_ITEM       (X_DSD_STEP_MODE_PROPERTY->items + 3)
#define X_DSD_STEP_MODE_16TH_ITEM        (X_DSD_STEP_MODE_PROPERTY->items + 4)
#define X_DSD_STEP_MODE_32TH_ITEM        (X_DSD_STEP_MODE_PROPERTY->items + 5)
#define X_DSD_STEP_MODE_64TH_ITEM        (X_DSD_STEP_MODE_PROPERTY->items + 6)
#define X_DSD_STEP_MODE_128TH_ITEM       (X_DSD_STEP_MODE_PROPERTY->items + 7)
#define X_DSD_STEP_MODE_256TH_ITEM       (X_DSD_STEP_MODE_PROPERTY->items + 8)

#define X_DSD_STEP_MODE_PROPERTY_NAME    "X_DSD_STEP_MODE"
#define X_DSD_STEP_MODE_FULL_ITEM_NAME   "FULL"
#define X_DSD_STEP_MODE_HALF_ITEM_NAME   "HALF"
#define X_DSD_STEP_MODE_FOURTH_ITEM_NAME "FOURTH"
#define X_DSD_STEP_MODE_EIGTH_ITEM_NAME  "EIGTH"
#define X_DSD_STEP_MODE_16TH_ITEM_NAME   "16TH"
#define X_DSD_STEP_MODE_32TH_ITEM_NAME   "32TH"
#define X_DSD_STEP_MODE_64TH_ITEM_NAME   "64TH"
#define X_DSD_STEP_MODE_128TH_ITEM_NAME  "128TH"
#define X_DSD_STEP_MODE_256TH_ITEM_NAME  "256TH"

#define X_DSD_COILS_MODE_PROPERTY            (PRIVATE_DATA->x_dsd_coils_mode_property)
#define X_DSD_COILS_MODE_IDLE_OFF_ITEM       (X_DSD_COILS_MODE_PROPERTY->items + 0)
#define X_DSD_COILS_MODE_ALWAYS_ON_ITEM      (X_DSD_COILS_MODE_PROPERTY->items + 1)
#define X_DSD_COILS_MODE_TIMEOUT_ITEM        (X_DSD_COILS_MODE_PROPERTY->items + 2)

#define X_DSD_COILS_MODE_PROPERTY_NAME       "X_DSD_COILS_MODE"
#define X_DSD_COILS_MODE_IDLE_OFF_ITEM_NAME  "OFF_WHEN_IDLE"
#define X_DSD_COILS_MODE_ALWAYS_ON_ITEM_NAME "ALWAYS_ON"
#define X_DSD_COILS_MODE_TIMEOUT_ITEM_NAME   "TIMEOUT_OFF"

#define X_DSD_CURRENT_CONTROL_PROPERTY       (PRIVATE_DATA->x_dsd_current_control_property)
#define X_DSD_CURRENT_CONTROL_MOVE_ITEM      (X_DSD_CURRENT_CONTROL_PROPERTY->items + 0)
#define X_DSD_CURRENT_CONTROL_HOLD_ITEM      (X_DSD_CURRENT_CONTROL_PROPERTY->items + 1)

#define X_DSD_CURRENT_CONTROL_PROPERTY_NAME  "X_DSD_CURRENT_CONTROL"
#define X_DSD_CURRENT_CONTROL_MOVE_ITEM_NAME "MOVE_CURRENT"
#define X_DSD_CURRENT_CONTROL_HOLD_ITEM_NAME "HOLD_CURRENT"

#define X_DSD_TIMINGS_PROPERTY             (PRIVATE_DATA->x_dsd_timings_property)
#define X_DSD_TIMINGS_SETTLE_ITEM          (X_DSD_TIMINGS_PROPERTY->items + 0)
#define X_DSD_TIMINGS_COILS_TOUT_ITEM      (X_DSD_TIMINGS_PROPERTY->items + 1)

#define X_DSD_TIMINGS_PROPERTY_NAME        "X_DSD_TIMINGS"
#define X_DSD_TIMINGS_SETTLE_ITEM_NAME     "SETTLE_TIME"
#define X_DSD_TIMINGS_COILS_TOUT_ITEM_NAME "COILS_POWER_TIMEOUT"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_dsd_model_hint_property;
	indigo_property *x_dsd_step_mode_property;
	indigo_property *x_dsd_coils_mode_property;
	indigo_property *x_dsd_current_control_property;
	indigo_property *x_dsd_timings_property;
	//+ data
	int focuser_version;
	int current_position, target_position, max_position;
	bool positive_last_move, reverse;
	double prev_temp;
	bool has_temperature_sensor;
	//- data
} dsd_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static bool dsd_vcommand(indigo_device *device, char *response, int size, const char *format, va_list args) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_vprintf(PRIVATE_DATA->handle, format, args) <= 0) {
		return false;
	}
	if (response == NULL) {
		return true;
	}
	long length = indigo_uni_read_section(PRIVATE_DATA->handle, response, size - 1, ")", "", INDIGO_DELAY(DSD_TIMEOUT));
	if (length <= 0 || response[length - 1] != ')') {
		response[0] = 0;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No valid response");
		return false;
	}
	response[length] = 0;
	indigo_usleep(50000);
	return true;
}

static bool dsd_command(indigo_device *device, char *response, int size, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = dsd_vcommand(device, response, size, format, args);
	va_end(args);
	return result;
}

static bool dsd_command_ok(indigo_device *device, const char *format, ...) {
	char response[DSD_CMD_LEN];
	va_list args;
	va_start(args, format);
	bool result = dsd_vcommand(device, response, sizeof(response), format, args);
	va_end(args);
	return result && !strcmp(response, "(OK)");
}

static bool dsd_get_int(indigo_device *device, const char *command, int *value) {
	char response[DSD_CMD_LEN];
	int result, consumed = 0;
	if (dsd_command(device, response, sizeof(response), "%s", command) && sscanf(response, "(%d%n", &result, &consumed) == 1 && (!strcmp(response + consumed, ")") || !strcmp(response + consumed, "%)"))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s = %d", command, response, result);
		*value = result;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
	return false;
}

static bool dsd_get_temperature(indigo_device *device, double *temperature) {
	char response[DSD_CMD_LEN];
	double result;
	int consumed = 0;
	if (dsd_command(device, response, sizeof(response), "[GTMC]") && sscanf(response, "(%lf%n", &result, &consumed) == 1 && !strcmp(response + consumed, ")")) {
		*temperature = result;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "[GTMC] failed");
	return false;
}

static bool dsd_get_info(indigo_device *device, char *board, char *firmware) {
	char response[DSD_CMD_LEN];
	int consumed = 0;
	if (dsd_command(device, response, sizeof(response), "[GFRM]") && sscanf(response, "(Board=%63[^,], Version=%63[^)])%n", board, firmware, &consumed) == 2 && consumed > 0 && response[consumed] == 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "[GFRM] -> %s = %s %s", response, board, firmware);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "[GFRM] failed");
	return false;
}

static bool dsd_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, atoi(DEVICE_BAUDRATE_ITEM->text.value), INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		// DSD resets on RTS, which is manipulated on connect! Wait for 2 seconds to recover!
		indigo_usleep(INDIGO_DELAY(2));
		int position;
		if (dsd_get_int(device, "[GPOS]", &position)) {
			return true;
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: Deep Sky Dad AF did not respond");
		indigo_uni_close(&PRIVATE_DATA->handle);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
	}
	indigo_global_unlock(device);
	return false;
}

static void dsd_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

static int dsd_clamp_position(indigo_device *device, long long position) {
	if (position > FOCUSER_POSITION_ITEM->number.max) {
		position = (long long)FOCUSER_POSITION_ITEM->number.max;
	} else if (position < FOCUSER_POSITION_ITEM->number.min) {
		position = (long long)FOCUSER_POSITION_ITEM->number.min;
	}
	return (int)position;
}

static void dsd_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int moving = 0, position = 0;
	bool moving_read = dsd_get_int(device, "[GMOV]", &moving);
	bool position_read = dsd_get_int(device, "[GPOS]", &position);
	if (position_read) {
		PRIVATE_DATA->current_position = position;
	}
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if (!moving_read || !position_read) {
		dsd_motion_state(device, INDIGO_ALERT_STATE);
	} else if (moving == 0 || PRIVATE_DATA->current_position == PRIVATE_DATA->target_position) {
		dsd_motion_state(device, INDIGO_OK_STATE);
	} else {
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_execute_handler_in(device, DSD_POLL_DELAY, motion_finalizer);
	}
}

static bool dsd_start_motion(indigo_device *device, int position) {
	int target = indigo_compensate_backlash(position, PRIVATE_DATA->current_position, (int)FOCUSER_BACKLASH_ITEM->number.value, &PRIVATE_DATA->positive_last_move);
	if (target < 0) {
		target = 0;
	}
	if (!dsd_command_ok(device, "[STRG%06d]", target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[STRG%06d] failed", target);
		return false;
	}
	if (!dsd_command(device, NULL, 0, "[SMOV]")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SMOV] failed");
		return false;
	}
	// a started move replaces the pending connection or compensation poll
	indigo_cancel_pending_handler(device, motion_finalizer);
	return true;
}

static bool dsd_update_step_mode(indigo_device *device) {
	int mode;
	if (dsd_get_int(device, "[GSTP]", &mode)) {
		for (int i = 0; i < X_DSD_STEP_MODE_PROPERTY->count; i++) {
			if (mode == 1 << i) {
				indigo_set_switch(X_DSD_STEP_MODE_PROPERTY, X_DSD_STEP_MODE_PROPERTY->items + i, true);
				return true;
			}
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[GSTP] wrong value %d", mode);
	}
	return false;
}

static bool dsd_update_coils_mode(indigo_device *device) {
	int mode;
	if (dsd_get_int(device, "[GCLM]", &mode)) {
		if (mode >= 0 && mode < X_DSD_COILS_MODE_PROPERTY->count) {
			indigo_set_switch(X_DSD_COILS_MODE_PROPERTY, X_DSD_COILS_MODE_PROPERTY->items + mode, true);
			return true;
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[GCLM] wrong value %d", mode);
	}
	return false;
}

static bool dsd_update_number(indigo_device *device, const char *command, indigo_item *item) {
	int value;
	if (dsd_get_int(device, command, &value)) {
		item->number.value = item->number.target = value;
		return true;
	}
	return false;
}

static bool dsd_update_currents(indigo_device *device) {
	bool result = dsd_update_number(device, PRIVATE_DATA->focuser_version < 3 ? "[GCMV%]" : "[GMMM]", X_DSD_CURRENT_CONTROL_MOVE_ITEM);
	return dsd_update_number(device, PRIVATE_DATA->focuser_version < 3 ? "[GCHD%]" : "[GMHM]", X_DSD_CURRENT_CONTROL_HOLD_ITEM) && result;
}

static void dsd_compensate_focus(indigo_device *device, double new_temp) {
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;
	// we do not have previous temperature reading
	if (PRIVATE_DATA->prev_temp <= NO_TEMP_READING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PRIVATE_DATA->prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}
	// we do not have current temperature reading or focuser is moving
	if (new_temp <= NO_TEMP_READING || FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}
	// temperature difference if more than 1 degree so compensation needed
	if (fabs(temp_difference) < FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value || fabs(temp_difference) >= 100) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %.2f, threshold = %.2f", temp_difference, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
		return;
	}
	int compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.0f, threshold = %.2f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
	long long target = (long long)PRIVATE_DATA->current_position + compensation;
	int position;
	if (dsd_get_int(device, "[GPOS]", &position)) {
		PRIVATE_DATA->current_position = position;
	}
	// Make sure we do not attempt to go beyond the limits
	PRIVATE_DATA->target_position = dsd_clamp_position(device, target);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if (dsd_start_motion(device, PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, DSD_POLL_DELAY, motion_finalizer);
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	PRIVATE_DATA->prev_temp = new_temp;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void dsd_temperature_poll(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	double temperature = NO_TEMP_READING;
	bool read = dsd_get_temperature(device, &temperature);
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	if (read) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
	} else {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (FOCUSER_TEMPERATURE_ITEM->number.value <= NO_TEMP_READING) {
		// -127 is returned when the sensor is not connected
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (PRIVATE_DATA->has_temperature_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected.");
			PRIVATE_DATA->has_temperature_sensor = false;
		}
	} else {
		PRIVATE_DATA->has_temperature_sensor = true;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (!FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		// reset temp so that the compensation starts when auto mode is selected
		PRIVATE_DATA->prev_temp = NO_TEMP_READING;
	} else if (read) {
		dsd_compensate_focus(device, temperature);
	}
	indigo_execute_handler_in(device, 2, dsd_temperature_poll);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = dsd_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			// model-dependent capabilities are restored before every detection
			PRIVATE_DATA->focuser_version = 0;
			FOCUSER_SPEED_ITEM->number.max = 5;
			X_DSD_STEP_MODE_PROPERTY->count = 9;
			X_DSD_COILS_MODE_PROPERTY->hidden = false;
			X_DSD_TIMINGS_PROPERTY->count = 2;
			X_DSD_CURRENT_CONTROL_MOVE_ITEM->number.min = X_DSD_CURRENT_CONTROL_HOLD_ITEM->number.min = 10;
			INDIGO_COPY_VALUE(X_DSD_CURRENT_CONTROL_MOVE_ITEM->label, "Move current (%)");
			INDIGO_COPY_VALUE(X_DSD_CURRENT_CONTROL_HOLD_ITEM->label, "Hold current (%)");
			FOCUSER_MODE_PROPERTY->hidden = FOCUSER_TEMPERATURE_PROPERTY->hidden = FOCUSER_COMPENSATION_PROPERTY->hidden = true;
			char board[64] = "N/A", firmware[64] = "N/A";
			if (dsd_get_info(device, board, firmware)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, board);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				if (strstr(board, "AF1")) {
					PRIVATE_DATA->focuser_version = 1;
				} else if (strstr(board, "AF2")) {
					PRIVATE_DATA->focuser_version = 2;
				} else if (strstr(board, "AF3")) {
					PRIVATE_DATA->focuser_version = 3;
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "version = %d", PRIVATE_DATA->focuser_version);
			}
			if (PRIVATE_DATA->focuser_version < 3) {
				// DSD version < 3 supports speeds from 1 to 3 and steps from full to 1/8
				FOCUSER_SPEED_ITEM->number.max = 3;
				X_DSD_STEP_MODE_PROPERTY->count = 4;
			} else {
				// DSD version 3 does not have coils mode and coils timeout, current multipliers are in range 1-100
				X_DSD_COILS_MODE_PROPERTY->hidden = true;
				X_DSD_TIMINGS_PROPERTY->count = 1;
				X_DSD_CURRENT_CONTROL_MOVE_ITEM->number.min = X_DSD_CURRENT_CONTROL_HOLD_ITEM->number.min = 1;
				INDIGO_COPY_VALUE(X_DSD_CURRENT_CONTROL_MOVE_ITEM->label, "Move current multiplier (%)");
				INDIGO_COPY_VALUE(X_DSD_CURRENT_CONTROL_HOLD_ITEM->label, "Hold current multiplier (%)");
			}
			int value;
			if (dsd_get_int(device, "[GPOS]", &value)) {
				FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = value;
			}
			if (dsd_get_int(device, "[GMXP]", &value)) {
				FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = PRIVATE_DATA->max_position = value;
			}
			if (dsd_get_int(device, "[GSPD]", &value)) {
				FOCUSER_SPEED_ITEM->number.value = value;
			}
			// While we do not have max move property hardcode it to max position
			dsd_command_ok(device, "[SMXM%d]", (int)FOCUSER_POSITION_ITEM->number.max);
			// DSD does not report reverse motion, so we set it to be sure we know its state
			PRIVATE_DATA->reverse = FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
			dsd_command_ok(device, "[SREV%01d]", PRIVATE_DATA->reverse ? 1 : 0);
			dsd_update_step_mode(device);
			if (PRIVATE_DATA->focuser_version < 3) {
				dsd_update_coils_mode(device);
			}
			dsd_update_currents(device);
			dsd_update_number(device, "[GBUF]", X_DSD_TIMINGS_SETTLE_ITEM);
			if (PRIVATE_DATA->focuser_version < 3) {
				dsd_update_number(device, "[GIDC]", X_DSD_TIMINGS_COILS_TOUT_ITEM);
			}
			indigo_execute_handler_in(device, DSD_POLL_DELAY, motion_finalizer);
			if (PRIVATE_DATA->focuser_version > 1) {
				FOCUSER_MODE_PROPERTY->hidden = false;
				FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
				dsd_get_temperature(device, &FOCUSER_TEMPERATURE_ITEM->number.value);
				PRIVATE_DATA->prev_temp = FOCUSER_TEMPERATURE_ITEM->number.value;
				FOCUSER_COMPENSATION_PROPERTY->hidden = false;
				FOCUSER_COMPENSATION_ITEM->number.min = -10000;
				FOCUSER_COMPENSATION_ITEM->number.max = 10000;
				FOCUSER_COMPENSATION_PROPERTY->count = 2;
				PRIVATE_DATA->has_temperature_sensor = true;
				indigo_execute_handler_in(device, 1, dsd_temperature_poll);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_DSD_STEP_MODE_PROPERTY, NULL);
			indigo_define_property(device, X_DSD_COILS_MODE_PROPERTY, NULL);
			indigo_define_property(device, X_DSD_CURRENT_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, X_DSD_TIMINGS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		dsd_command(device, NULL, 0, "[STOP]");
		//- focuser.on_disconnect
		indigo_delete_property(device, X_DSD_STEP_MODE_PROPERTY, NULL);
		indigo_delete_property(device, X_DSD_COILS_MODE_PROPERTY, NULL);
		indigo_delete_property(device, X_DSD_CURRENT_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, X_DSD_TIMINGS_PROPERTY, NULL);
		dsd_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	if (!dsd_command_ok(device, "[SMXP%d]", (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SMXP] failed");
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	int value;
	if (dsd_get_int(device, "[GMXP]", &value)) {
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = PRIVATE_DATA->max_position = value;
	} else {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!dsd_command_ok(device, "[SSPD%d]", (int)FOCUSER_SPEED_ITEM->number.target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SSPD] failed");
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	int value;
	if (dsd_get_int(device, "[GSPD]", &value)) {
		FOCUSER_SPEED_ITEM->number.value = value;
	} else {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	if (dsd_command_ok(device, "[SREV%01d]", FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? 1 : 0)) {
		PRIVATE_DATA->reverse = FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SREV] failed");
		indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reverse ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	if (target == PRIVATE_DATA->current_position) {
		dsd_motion_state(device, INDIGO_OK_STATE);
	} else {
		PRIVATE_DATA->target_position = target;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		dsd_motion_state(device, INDIGO_BUSY_STATE);
		if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
			if (dsd_start_motion(device, target)) {
				indigo_execute_handler_in(device, DSD_POLL_DELAY, motion_finalizer);
			} else {
				dsd_motion_state(device, INDIGO_ALERT_STATE);
			}
		} else {
			indigo_property_state state = INDIGO_OK_STATE;
			if (!dsd_command_ok(device, "[SPOS%06d]", target)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SPOS%06d] failed", target);
				state = INDIGO_ALERT_STATE;
			}
			int position;
			if (dsd_get_int(device, "[GPOS]", &position)) {
				FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = position;
			} else {
				state = INDIGO_ALERT_STATE;
			}
			dsd_motion_state(device, state);
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	dsd_motion_state(device, INDIGO_BUSY_STATE);
	int position;
	if (dsd_get_int(device, "[GPOS]", &position)) {
		PRIVATE_DATA->current_position = position;
	}
	long long steps = (long long)FOCUSER_STEPS_ITEM->number.value;
	PRIVATE_DATA->target_position = dsd_clamp_position(device, PRIVATE_DATA->current_position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -steps : steps));
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if (dsd_start_motion(device, PRIVATE_DATA->target_position)) {
		indigo_execute_handler_in(device, DSD_POLL_DELAY, motion_finalizer);
	} else {
		dsd_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	// urgent abort can overtake a queued move and its completion poll
	indigo_cancel_pending_handler(device, focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_steps_handler);
	indigo_cancel_pending_handler(device, motion_finalizer);
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	if (!dsd_command(device, NULL, 0, "[STOP]")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[STOP] failed");
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	int position;
	if (dsd_get_int(device, "[GPOS]", &position)) {
		PRIVATE_DATA->current_position = position;
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
		indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else {
		indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_x_dsd_model_hint_handler(indigo_device *device) {
	X_DSD_MODEL_HINT_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_DSD_MODEL_HINT.on_change
	INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, X_DSD_MODEL_AF3_ITEM->sw.value ? DSD_AF3_BAUDRATE : DSD_AF1_AF2_BAUDRATE);
	indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, NULL);
	//- focuser.X_DSD_MODEL_HINT.on_change
	indigo_update_property(device, X_DSD_MODEL_HINT_PROPERTY, NULL);
}

static void focuser_x_dsd_step_mode_handler(indigo_device *device) {
	X_DSD_STEP_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_DSD_STEP_MODE.on_change
	int mode = 1;
	for (int i = 0; i < X_DSD_STEP_MODE_PROPERTY->count; i++) {
		if (X_DSD_STEP_MODE_PROPERTY->items[i].sw.value) {
			mode = 1 << i;
			break;
		}
	}
	if (!dsd_command_ok(device, "[SSTP%d]", mode)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SSTP%d] failed", mode);
		X_DSD_STEP_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (!dsd_update_step_mode(device)) {
		X_DSD_STEP_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_DSD_STEP_MODE.on_change
	indigo_update_property(device, X_DSD_STEP_MODE_PROPERTY, NULL);
}

static void focuser_x_dsd_coils_mode_handler(indigo_device *device) {
	X_DSD_COILS_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_DSD_COILS_MODE.on_change
	int mode = X_DSD_COILS_MODE_ALWAYS_ON_ITEM->sw.value ? 1 : X_DSD_COILS_MODE_TIMEOUT_ITEM->sw.value ? 2 : 0;
	if (!dsd_command_ok(device, "[SCLM%d]", mode)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SCLM%d] failed", mode);
		X_DSD_COILS_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (!dsd_update_coils_mode(device)) {
		X_DSD_COILS_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_DSD_COILS_MODE.on_change
	indigo_update_property(device, X_DSD_COILS_MODE_PROPERTY, NULL);
}

static void focuser_x_dsd_current_control_handler(indigo_device *device) {
	X_DSD_CURRENT_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_DSD_CURRENT_CONTROL.on_change
	bool af3 = PRIVATE_DATA->focuser_version >= 3;
	if (!dsd_command_ok(device, af3 ? "[SMMM%d]" : "[SCMV%d%%]", (int)X_DSD_CURRENT_CONTROL_MOVE_ITEM->number.target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Move current setting failed");
		X_DSD_CURRENT_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (!dsd_command_ok(device, af3 ? "[SMHM%d]" : "[SCHD%d%%]", (int)X_DSD_CURRENT_CONTROL_HOLD_ITEM->number.target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hold current setting failed");
		X_DSD_CURRENT_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (!dsd_update_currents(device)) {
		X_DSD_CURRENT_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_DSD_CURRENT_CONTROL.on_change
	indigo_update_property(device, X_DSD_CURRENT_CONTROL_PROPERTY, NULL);
}

static void focuser_x_dsd_timings_handler(indigo_device *device) {
	X_DSD_TIMINGS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_DSD_TIMINGS.on_change
	if (!dsd_command_ok(device, "[SBUF%06d]", (int)X_DSD_TIMINGS_SETTLE_ITEM->number.target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SBUF] failed");
		X_DSD_TIMINGS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (!dsd_update_number(device, "[GBUF]", X_DSD_TIMINGS_SETTLE_ITEM)) {
		X_DSD_TIMINGS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (PRIVATE_DATA->focuser_version < 3) {
		if (!dsd_command_ok(device, "[SIDC%06d]", (int)X_DSD_TIMINGS_COILS_TOUT_ITEM->number.target)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "[SIDC] failed");
			X_DSD_TIMINGS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!dsd_update_number(device, "[GIDC]", X_DSD_TIMINGS_COILS_TOUT_ITEM)) {
			X_DSD_TIMINGS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.X_DSD_TIMINGS.on_change
	indigo_update_property(device, X_DSD_TIMINGS_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, DSD_AF1_AF2_BAUDRATE);
		//- focuser.on_attach
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 10000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = DSD_MAX_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 5;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = true;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 100;
		FOCUSER_POSITION_ITEM->number.max = DSD_MAX_POSITION;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = true;
		X_DSD_MODEL_HINT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_DSD_MODEL_HINT_PROPERTY_NAME, MAIN_GROUP, "Focuser model hint", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_DSD_MODEL_HINT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_DSD_MODEL_AF1_2_ITEM, X_DSD_MODEL_AF1_2_ITEM_NAME, "AF1/AF2", true);
		indigo_init_switch_item(X_DSD_MODEL_AF3_ITEM, X_DSD_MODEL_AF3_ITEM_NAME, "AF3", false);
		X_DSD_STEP_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_DSD_STEP_MODE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 9);
		if (X_DSD_STEP_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_DSD_STEP_MODE_FULL_ITEM, X_DSD_STEP_MODE_FULL_ITEM_NAME, "Full step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_HALF_ITEM, X_DSD_STEP_MODE_HALF_ITEM_NAME, "1/2 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_FOURTH_ITEM, X_DSD_STEP_MODE_FOURTH_ITEM_NAME, "1/4 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_EIGTH_ITEM, X_DSD_STEP_MODE_EIGTH_ITEM_NAME, "1/8 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_16TH_ITEM, X_DSD_STEP_MODE_16TH_ITEM_NAME, "1/16 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_32TH_ITEM, X_DSD_STEP_MODE_32TH_ITEM_NAME, "1/32 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_64TH_ITEM, X_DSD_STEP_MODE_64TH_ITEM_NAME, "1/64 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_128TH_ITEM, X_DSD_STEP_MODE_128TH_ITEM_NAME, "1/128 step", false);
		indigo_init_switch_item(X_DSD_STEP_MODE_256TH_ITEM, X_DSD_STEP_MODE_256TH_ITEM_NAME, "1/256 step", false);
		X_DSD_COILS_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_DSD_COILS_MODE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Coils Power", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_DSD_COILS_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_DSD_COILS_MODE_IDLE_OFF_ITEM, X_DSD_COILS_MODE_IDLE_OFF_ITEM_NAME, "OFF when idle", false);
		indigo_init_switch_item(X_DSD_COILS_MODE_ALWAYS_ON_ITEM, X_DSD_COILS_MODE_ALWAYS_ON_ITEM_NAME, "Always ON", false);
		indigo_init_switch_item(X_DSD_COILS_MODE_TIMEOUT_ITEM, X_DSD_COILS_MODE_TIMEOUT_ITEM_NAME, "OFF after timeout", false);
		X_DSD_CURRENT_CONTROL_PROPERTY = indigo_init_number_property(NULL, device->name, X_DSD_CURRENT_CONTROL_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_DSD_CURRENT_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_DSD_CURRENT_CONTROL_MOVE_ITEM, X_DSD_CURRENT_CONTROL_MOVE_ITEM_NAME, "Move current (%)", 10, 100, 1, 50);
		indigo_init_number_item(X_DSD_CURRENT_CONTROL_HOLD_ITEM, X_DSD_CURRENT_CONTROL_HOLD_ITEM_NAME, "Hold current (%)", 10, 100, 1, 50);
		X_DSD_TIMINGS_PROPERTY = indigo_init_number_property(NULL, device->name, X_DSD_TIMINGS_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Timing settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_DSD_TIMINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_DSD_TIMINGS_SETTLE_ITEM, X_DSD_TIMINGS_SETTLE_ITEM_NAME, "Settle time (ms)", 0, 99999, 100, 0);
		indigo_init_number_item(X_DSD_TIMINGS_COILS_TOUT_ITEM, X_DSD_TIMINGS_COILS_TOUT_ITEM_NAME, "Coils power timeout (ms)", 9, 999999, 1000, 60000);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_DSD_STEP_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_DSD_COILS_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_DSD_CURRENT_CONTROL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_DSD_TIMINGS_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(X_DSD_MODEL_HINT_PROPERTY);
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DSD_MODEL_HINT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(X_DSD_MODEL_HINT_PROPERTY, focuser_x_dsd_model_hint_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DSD_STEP_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_DSD_STEP_MODE_PROPERTY, focuser_x_dsd_step_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DSD_COILS_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_DSD_COILS_MODE_PROPERTY, focuser_x_dsd_coils_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DSD_CURRENT_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_DSD_CURRENT_CONTROL_PROPERTY, focuser_x_dsd_current_control_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DSD_TIMINGS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_DSD_TIMINGS_PROPERTY, focuser_x_dsd_timings_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_DSD_MODEL_HINT_PROPERTY);
			indigo_save_property(device, NULL, X_DSD_STEP_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_DSD_COILS_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_DSD_CURRENT_CONTROL_PROPERTY);
			indigo_save_property(device, NULL, X_DSD_TIMINGS_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_DSD_MODEL_HINT_PROPERTY);
	indigo_release_property(X_DSD_STEP_MODE_PROPERTY);
	indigo_release_property(X_DSD_COILS_MODE_PROPERTY);
	indigo_release_property(X_DSD_CURRENT_CONTROL_PROPERTY);
	indigo_release_property(X_DSD_TIMINGS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_dsd(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static dsd_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (dsd_private_data *)indigo_safe_malloc(sizeof(dsd_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
