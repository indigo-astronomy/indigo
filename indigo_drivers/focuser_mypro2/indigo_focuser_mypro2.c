// Copyright (C) 2020-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_focuser_mypro2.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <stdarg.h>
#include <indigo/indigo_client.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_mypro2.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000E
#define DRIVER_NAME          "indigo_focuser_mypro2"
#define DRIVER_LABEL         "myFocuserPro2 Focuser"
#define FOCUSER_DEVICE_NAME  "myFocuserPro2"
#define PRIVATE_DATA         ((mypro2_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define MFP_CMD_LEN          100
#define MFP_BAUDRATE         "9600"
#define MFP_TIMEOUT          0.3
#define MFP_POLL_DELAY       0.5
#define MFP_MIN_POSITION     0
#define MFP_MAX_POSITION     2000000
#define NO_TEMP_READING      (-127)
// the DS18B20 probe measures -55 to 125 C, anything else is not a reading
#define MFP_MIN_TEMPERATURE  (-55)
#define MFP_MAX_TEMPERATURE  125
// polls without progress while the controller reports a move before it counts as stalled
#define MFP_STALL_POLLS      6

//- define

#pragma mark - Property definitions

#define X_STEP_MODE_PROPERTY           (PRIVATE_DATA->x_step_mode_property)
#define X_STEP_MODE_FULL_ITEM          (X_STEP_MODE_PROPERTY->items + 0)
#define X_STEP_MODE_HALF_ITEM          (X_STEP_MODE_PROPERTY->items + 1)
#define X_STEP_MODE_FOURTH_ITEM        (X_STEP_MODE_PROPERTY->items + 2)
#define X_STEP_MODE_EIGTH_ITEM         (X_STEP_MODE_PROPERTY->items + 3)
#define X_STEP_MODE_16TH_ITEM          (X_STEP_MODE_PROPERTY->items + 4)
#define X_STEP_MODE_32TH_ITEM          (X_STEP_MODE_PROPERTY->items + 5)
#define X_STEP_MODE_64TH_ITEM          (X_STEP_MODE_PROPERTY->items + 6)
#define X_STEP_MODE_128TH_ITEM         (X_STEP_MODE_PROPERTY->items + 7)

#define X_STEP_MODE_PROPERTY_NAME      "X_STEP_MODE"
#define X_STEP_MODE_FULL_ITEM_NAME     "FULL"
#define X_STEP_MODE_HALF_ITEM_NAME     "HALF"
#define X_STEP_MODE_FOURTH_ITEM_NAME   "FOURTH"
#define X_STEP_MODE_EIGTH_ITEM_NAME    "EIGTH"
#define X_STEP_MODE_16TH_ITEM_NAME     "16TH"
#define X_STEP_MODE_32TH_ITEM_NAME     "32TH"
#define X_STEP_MODE_64TH_ITEM_NAME     "64TH"
#define X_STEP_MODE_128TH_ITEM_NAME    "128TH"

#define X_COILS_MODE_PROPERTY            (PRIVATE_DATA->x_coils_mode_property)
#define X_COILS_MODE_IDLE_OFF_ITEM       (X_COILS_MODE_PROPERTY->items + 0)
#define X_COILS_MODE_ALWAYS_ON_ITEM      (X_COILS_MODE_PROPERTY->items + 1)

#define X_COILS_MODE_PROPERTY_NAME       "X_COILS_MODE"
#define X_COILS_MODE_IDLE_OFF_ITEM_NAME  "OFF_WHEN_IDLE"
#define X_COILS_MODE_ALWAYS_ON_ITEM_NAME "ALWAYS_ON"

#define X_SETTLE_TIME_PROPERTY         (PRIVATE_DATA->x_settle_time_property)
#define X_SETTLE_TIME_ITEM             (X_SETTLE_TIME_PROPERTY->items + 0)

#define X_SETTLE_TIME_PROPERTY_NAME    "X_SETTLE_TIME"
#define X_SETTLE_TIME_ITEM_NAME        "SETTLE_TIME"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_step_mode_property;
	indigo_property *x_coils_mode_property;
	indigo_property *x_settle_time_property;
	//+ data
	char response[MFP_CMD_LEN];
	int current_position, target_position, max_position, sync_target, stall_polls;
	// the settings the controller last confirmed, restored when a readback fails
	int step_mode, coils_mode, settle_time, reversed, backlash;
	double prev_temp;
	bool has_temperature_sensor, disconnection_queued;
	// motion_active: a move runs; external_motion: the driver did not command it;
	// stop_pending: an abort could not confirm the stop; poll_failed: the ALERT on
	// FOCUSER_POSITION comes from a failed idle poll, not from a failed move
	bool motion_active, external_motion, stop_pending, poll_failed;
	//- data
} mypro2_private_data;

#pragma mark - Low level code

//+ code

static void focuser_connection_handler(indigo_device *device);
static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void motion_finalizer(indigo_device *device);
static void sync_finalizer(indigo_device *device);

// A controller reached over the network cannot report a broken link, so
// a failed transaction on a TCP handle disconnects the device.
static void mypro2_network_disconnection(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
		// the alert state signals the unexpected disconnection
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_send_message(device, ALERT_PROPERTY, "Device disconnected unexpectedly");
	}
}

// Requests are ":<code><arguments>#", replies are "<letter><value>#".
// Setting commands are not acknowledged at all, so expect_response is
// false for them.
static bool mypro2_vcommand(indigo_device *device, bool expect_response, const char *format, va_list args) {
	bool failed = indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_vprintf(PRIVATE_DATA->handle, format, args) <= 0;
	if (!failed && expect_response) {
		long length = indigo_uni_read_section(PRIVATE_DATA->handle, RESPONSE, sizeof(PRIVATE_DATA->response) - 1, "#", "", INDIGO_DELAY(MFP_TIMEOUT));
		// a reply that fills the buffer or times out before its '#' is not a reply
		failed = length <= 0 || RESPONSE[length - 1] != '#';
		if (!failed) {
			// the Arduino firmware needs a gap between transactions
			indigo_usleep(50000);
		}
	}
	if (failed && PRIVATE_DATA->handle != NULL && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE && !PRIVATE_DATA->disconnection_queued) {
		PRIVATE_DATA->disconnection_queued = true;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unexpected disconnection from %s", DEVICE_PORT_ITEM->text.value);
		indigo_execute_handler(device, mypro2_network_disconnection);
	}
	return !failed;
}

static bool mypro2_command(indigo_device *device, bool expect_response, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = mypro2_vcommand(device, expect_response, format, args);
	va_end(args);
	return result;
}

static bool mypro2_get_int(indigo_device *device, const char *command, char expect, int *value) {
	char format[8];
	snprintf(format, sizeof(format), "%c%%d#", expect);
	if (mypro2_command(device, true, "%s", command) && sscanf(RESPONSE, format, value) == 1) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s = %d", command, RESPONSE, *value);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
	return false;
}

static bool mypro2_get_position(indigo_device *device, int *position) {
	return mypro2_get_int(device, ":00#", 'P', position);
}

// -127 is the no-sensor sentinel, anything else outside the probe range is rejected
static bool mypro2_get_temperature(indigo_device *device, double *temperature) {
	double value = 0;
	if (mypro2_command(device, true, ":06#") && sscanf(RESPONSE, "Z%lf#", &value) == 1 && (value <= NO_TEMP_READING || (value >= MFP_MIN_TEMPERATURE && value <= MFP_MAX_TEMPERATURE))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, ":06# -> %s = %lf", RESPONSE, value);
		*temperature = value;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, ":06# failed");
	return false;
}

// The firmware answers "F<board><separator><version>#"; the separators
// are a newline and a carriage return on the boards seen so far.
static bool mypro2_get_info(indigo_device *device, char *board, char *firmware) {
	if (!mypro2_command(device, true, ":04#")) {
		return false;
	}
	for (char *c = RESPONSE; *c; c++) {
		if (*c == '\n' || *c == '\r') {
			*c = ' ';
		} else if (*c == '#') {
			*c = 0;
			break;
		}
	}
	if (sscanf(RESPONSE, "F%63s %63s", board, firmware) != 2) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":04# failed");
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, ":04# -> %s %s", board, firmware);
	return true;
}

static bool mypro2_set_backlashes(indigo_device *device, int backlash_in, int backlash_out) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set backlash_in = %d, backlash_out = %d", backlash_in, backlash_out);
	return mypro2_command(device, false, ":77%02d#", backlash_in) && mypro2_command(device, false, ":79%02d#", backlash_out) && mypro2_command(device, false, ":73%1d#", backlash_in > 0 ? 1 : 0) && mypro2_command(device, false, ":75%1d#", backlash_out > 0 ? 1 : 0);
}

static bool mypro2_get_backlashes(indigo_device *device, int *backlash_in, int *backlash_out) {
	int in_steps = 0, out_steps = 0, in_enabled = 0, out_enabled = 0;
	if (!mypro2_get_int(device, ":78#", '6', &in_steps) || !mypro2_get_int(device, ":80#", '7', &out_steps) || !mypro2_get_int(device, ":74#", '4', &in_enabled) || !mypro2_get_int(device, ":76#", '5', &out_enabled)) {
		return false;
	}
	*backlash_in = in_enabled ? in_steps : 0;
	*backlash_out = out_enabled ? out_steps : 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Get backlash_in = %d, backlash_out = %d", *backlash_in, *backlash_out);
	return true;
}

// The update helpers publish what the controller reports; when the readback
// fails they restore the value it last confirmed instead of the request.
static void mypro2_show_step_mode(indigo_device *device) {
	for (int i = 0; i < X_STEP_MODE_PROPERTY->count; i++) {
		if (PRIVATE_DATA->step_mode == 1 << i) {
			indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_PROPERTY->items + i, true);
		}
	}
}

static bool mypro2_update_step_mode(indigo_device *device) {
	int mode = 0;
	bool result = false;
	if (mypro2_get_int(device, ":29#", 'S', &mode)) {
		for (int i = 0; i < X_STEP_MODE_PROPERTY->count; i++) {
			if (mode == 1 << i) {
				PRIVATE_DATA->step_mode = mode;
				result = true;
			}
		}
		if (!result) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, ":29# wrong value %d", mode);
		}
	}
	mypro2_show_step_mode(device);
	return result;
}

static bool mypro2_update_coils_mode(indigo_device *device) {
	int mode = 0;
	bool result = false;
	if (mypro2_get_int(device, ":11#", 'O', &mode)) {
		if (mode < 0 || mode >= X_COILS_MODE_PROPERTY->count) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, ":11# wrong value %d", mode);
		} else {
			PRIVATE_DATA->coils_mode = mode;
			result = true;
		}
	}
	indigo_set_switch(X_COILS_MODE_PROPERTY, X_COILS_MODE_PROPERTY->items + PRIVATE_DATA->coils_mode, true);
	return result;
}

static bool mypro2_update_settle_time(indigo_device *device) {
	int settle_time = 0;
	bool result = mypro2_get_int(device, ":72#", '3', &settle_time);
	if (result) {
		PRIVATE_DATA->settle_time = settle_time;
	}
	X_SETTLE_TIME_ITEM->number.value = X_SETTLE_TIME_ITEM->number.target = PRIVATE_DATA->settle_time;
	return result;
}

static bool mypro2_update_reverse(indigo_device *device) {
	int reversed = 0;
	bool result = mypro2_get_int(device, ":13#", 'R', &reversed);
	if (result) {
		PRIVATE_DATA->reversed = reversed ? 1 : 0;
	}
	indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reversed ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
	return result;
}

// FOCUSER_POSITION and FOCUSER_STEPS cover the travel the controller reports;
// a running session republishes them, because ranges travel only with a definition.
static void mypro2_update_ranges(indigo_device *device, bool redefine) {
	FOCUSER_POSITION_ITEM->number.max = FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->max_position;
	if (redefine) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
}

// Requests that move the focuser or change its geometry wait for the running motion.
static bool mypro2_motion_busy(indigo_device *device) {
	return PRIVATE_DATA->motion_active || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE;
}

static int mypro2_clamp_position(indigo_device *device, long long position) {
	if (position > FOCUSER_POSITION_ITEM->number.max) {
		position = (long long)FOCUSER_POSITION_ITEM->number.max;
	} else if (position < FOCUSER_POSITION_ITEM->number.min) {
		position = (long long)FOCUSER_POSITION_ITEM->number.min;
	}
	return (int)position;
}

// An ALERT publishes the real position as both value and target, so a failed or
// aborted move is never shown at the requested position.
static void mypro2_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if (state == INDIGO_ALERT_STATE) {
		FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->current_position;
	}
	if (state != INDIGO_BUSY_STATE) {
		PRIVATE_DATA->motion_active = PRIVATE_DATA->external_motion = false;
	}
	PRIVATE_DATA->poll_failed = false;
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

// A failed status read or a motor that reports a move without progress stops
// the controller and ends the move ALERT; neither is ever taken for arrival.
static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int moving = 0, position = 0;
	if (!mypro2_get_int(device, ":01#", 'I', &moving) || !mypro2_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Motion status read failed, stopping the focuser");
		mypro2_command(device, false, ":27#");
		mypro2_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (moving != 0 && position == PRIVATE_DATA->current_position && position != PRIVATE_DATA->target_position) {
		if (++PRIVATE_DATA->stall_polls >= MFP_STALL_POLLS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser stalled at %d, stopping it", position);
			mypro2_command(device, false, ":27#");
			mypro2_motion_state(device, INDIGO_ALERT_STATE);
			return;
		}
	} else {
		PRIVATE_DATA->stall_polls = 0;
	}
	PRIVATE_DATA->current_position = position;
	if (moving == 0 || PRIVATE_DATA->current_position == PRIVATE_DATA->target_position) {
		if (PRIVATE_DATA->external_motion) {
			// a move the driver did not command ends where the controller stopped
			FOCUSER_POSITION_ITEM->number.target = position;
		}
		mypro2_motion_state(device, INDIGO_OK_STATE);
	} else {
		// progress keeps the BUSY state of the running move
		FOCUSER_POSITION_ITEM->number.value = position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_execute_handler_in(device, MFP_POLL_DELAY, motion_finalizer);
	}
}

// :31# updates the controller's counter without moving, but the new
// coordinate is only readable a moment later.
static void sync_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int position = 0;
	if (!mypro2_get_position(device, &position)) {
		mypro2_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->current_position = position;
	if (position != PRIVATE_DATA->sync_target) {
		// :31# is not acknowledged, the readback is the only proof it was applied
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Sync to %d not applied, the controller reports %d", PRIVATE_DATA->sync_target, position);
		mypro2_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->target_position = position;
	FOCUSER_POSITION_ITEM->number.target = position;
	mypro2_motion_state(device, INDIGO_OK_STATE);
}

// A target equal to the current position ends at once without a command.
static bool mypro2_start_motion(indigo_device *device, int target) {
	PRIVATE_DATA->target_position = target;
	PRIVATE_DATA->external_motion = false;
	PRIVATE_DATA->stall_polls = 0;
	FOCUSER_POSITION_ITEM->number.target = target;
	if (target == PRIVATE_DATA->current_position) {
		mypro2_motion_state(device, INDIGO_OK_STATE);
		return true;
	}
	if (mypro2_command(device, false, ":05%06d#", target)) {
		PRIVATE_DATA->motion_active = true;
		mypro2_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, MFP_POLL_DELAY, motion_finalizer);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, ":05%06d# failed", target);
	mypro2_motion_state(device, INDIGO_ALERT_STATE);
	return false;
}

// The poll reads the position the driver does not command itself: a hand
// controller move is followed to its end, and a failed read is an ALERT
// that the next good read clears.
static void mypro2_poll_position(indigo_device *device) {
	if (mypro2_motion_busy(device)) {
		return;
	}
	int moving = 0, position = 0;
	bool read = mypro2_get_int(device, ":01#", 'I', &moving) && mypro2_get_position(device, &position);
	if (mypro2_motion_busy(device)) {
		// a request accepted while the poll was in flight owns the motion properties now
		return;
	}
	if (!read) {
		if (FOCUSER_POSITION_PROPERTY->state != INDIGO_ALERT_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->poll_failed = true;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Position read failed");
		}
		return;
	}
	if (moving) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "The focuser moves on its own from %d", position);
		PRIVATE_DATA->current_position = position;
		PRIVATE_DATA->target_position = -1;
		PRIVATE_DATA->stall_polls = 0;
		PRIVATE_DATA->motion_active = PRIVATE_DATA->external_motion = true;
		FOCUSER_POSITION_ITEM->number.target = position;
		mypro2_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, MFP_POLL_DELAY, motion_finalizer);
		return;
	}
	bool recovered = FOCUSER_POSITION_PROPERTY->state == INDIGO_ALERT_STATE && PRIVATE_DATA->poll_failed;
	if (position != PRIVATE_DATA->current_position || recovered) {
		PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
		FOCUSER_POSITION_ITEM->number.target = position;
		// an ALERT left by a failed or aborted move stays, only a failed poll is cleared
		mypro2_motion_state(device, recovered || FOCUSER_POSITION_PROPERTY->state != INDIGO_ALERT_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
	}
}

static void mypro2_compensate_focus(indigo_device *device, double new_temp) {
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;
	// we do not have a previous temperature reading
	if (PRIVATE_DATA->prev_temp <= NO_TEMP_READING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}
	// we do not have a current temperature reading or the focuser is moving
	if (new_temp <= NO_TEMP_READING || FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}
	if (fabs(temp_difference) < FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value || fabs(temp_difference) >= 100) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %.2f, threshold = %.2f", temp_difference, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
		return;
	}
	int compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, compensation = %d, steps/degC = %.0f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value);
	int position = 0;
	if (!mypro2_get_position(device, &position)) {
		// the reference is kept, so the next reading retries the correction
		return;
	}
	PRIVATE_DATA->current_position = position;
	if (mypro2_start_motion(device, mypro2_clamp_position(device, (long long)position + compensation))) {
		PRIVATE_DATA->prev_temp = new_temp;
	}
}

static bool mypro2_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (indigo_uni_is_url(name, "mfp")) {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, 8080, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value), INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", name);
		return false;
	}
	PRIVATE_DATA->disconnection_queued = false;
	// myFocuserPro2 resets on RTS, which is manipulated on connect
	indigo_usleep(INDIGO_DELAY(2));
	int position = 0;
	if (mypro2_get_position(device, &position)) {
		PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: MyFP2 AF did not respond");
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void mypro2_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	mypro2_poll_position(device);
	double temperature = NO_TEMP_READING;
	bool read = mypro2_get_temperature(device, &temperature);
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	if (read) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
	} else {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (FOCUSER_TEMPERATURE_ITEM->number.value <= NO_TEMP_READING) {
		// -127 is returned when the DS18B20 is not connected
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
		// reset the reference so compensation starts when automatic mode is selected
		PRIVATE_DATA->prev_temp = NO_TEMP_READING;
	} else if (read) {
		mypro2_compensate_focus(device, temperature);
	}
	indigo_execute_handler_in(device, 2, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = mypro2_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			// board-dependent capabilities and the identity are restored before every detection
			X_STEP_MODE_PROPERTY->count = 8;
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, device->name);
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
			char board[64] = "N/A", firmware[64] = "N/A";
			if (mypro2_get_info(device, board, firmware)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, board);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			if (strstr(INFO_DEVICE_MODEL_ITEM->text.value, "Gemini") != NULL) {
				// Gemini supports full and half step only
				X_STEP_MODE_PROPERTY->count = 2;
			}
			// every query below is part of the protocol subset, so a controller that
			// does not answer one of them is not a usable myFocuserPro2
			int position = 0, moving = 0, backlash_in = 0, backlash_out = 0;
			connection_result = mypro2_get_position(device, &position) && mypro2_get_int(device, ":01#", 'I', &moving) && mypro2_get_backlashes(device, &backlash_in, &backlash_out);
			if (connection_result && backlash_in != backlash_out) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "backlash_in != backlash_out, using backlash_in as backlash");
				connection_result = mypro2_set_backlashes(device, backlash_in, backlash_in);
			}
			connection_result = connection_result && mypro2_get_int(device, ":08#", 'M', &PRIVATE_DATA->max_position) && mypro2_command(device, false, ":15%d#", (int)FOCUSER_SPEED_ITEM->number.value) && mypro2_update_reverse(device) && mypro2_update_coils_mode(device) && mypro2_update_step_mode(device) && mypro2_update_settle_time(device);
			if (connection_result) {
				PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
				PRIVATE_DATA->motion_active = PRIVATE_DATA->external_motion = PRIVATE_DATA->stop_pending = PRIVATE_DATA->poll_failed = false;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->backlash = backlash_in;
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = backlash_in;
				FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = PRIVATE_DATA->max_position;
				mypro2_update_ranges(device, false);
				FOCUSER_SPEED_ITEM->number.target = FOCUSER_SPEED_ITEM->number.value;
				PRIVATE_DATA->has_temperature_sensor = true;
				PRIVATE_DATA->prev_temp = NO_TEMP_READING;
				double temperature = NO_TEMP_READING;
				if (mypro2_get_temperature(device, &temperature)) {
					FOCUSER_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->prev_temp = temperature;
				}
				if (moving) {
					// a move the driver did not command is followed until the controller stops
					INDIGO_DRIVER_LOG(DRIVER_NAME, "The focuser is moving at connect");
					PRIVATE_DATA->target_position = -1;
					PRIVATE_DATA->stall_polls = 0;
					PRIVATE_DATA->motion_active = PRIVATE_DATA->external_motion = true;
					FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_execute_handler_in(device, MFP_POLL_DELAY, motion_finalizer);
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "The controller did not answer the connect sequence");
				mypro2_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_STEP_MODE_PROPERTY, NULL);
			indigo_define_property(device, X_COILS_MODE_PROPERTY, NULL);
			indigo_define_property(device, X_SETTLE_TIME_PROPERTY, NULL);
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
		// a running move is stopped before the port closes
		mypro2_command(device, false, ":27#");
		mypro2_command(device, false, ":48#");
		PRIVATE_DATA->motion_active = PRIVATE_DATA->external_motion = PRIVATE_DATA->stop_pending = PRIVATE_DATA->poll_failed = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			FOCUSER_TEMPERATURE_PROPERTY,
			FOCUSER_ON_POSITION_SET_PROPERTY,
			FOCUSER_MODE_PROPERTY,
			FOCUSER_COMPENSATION_PROPERTY,
			FOCUSER_SPEED_PROPERTY,
			FOCUSER_REVERSE_MOTION_PROPERTY,
			FOCUSER_BACKLASH_PROPERTY,
			FOCUSER_LIMITS_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
			X_STEP_MODE_PROPERTY,
			X_COILS_MODE_PROPERTY,
			X_SETTLE_TIME_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_STEP_MODE_PROPERTY, NULL);
		indigo_delete_property(device, X_COILS_MODE_PROPERTY, NULL);
		indigo_delete_property(device, X_SETTLE_TIME_PROPERTY, NULL);
		mypro2_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_timer_callback);
	}
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

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!mypro2_command(device, false, ":15%d#", (int)FOCUSER_SPEED_ITEM->number.target)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	int requested = FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? 1 : 0;
	// :14# is not acknowledged, so the readback decides
	if (!mypro2_command(device, false, ":14%d#", requested) || !mypro2_update_reverse(device) || PRIVATE_DATA->reversed != requested) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	int backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;
	int backlash_in = 0, backlash_out = 0;
	// the writes are not acknowledged, so the readback decides
	bool written = mypro2_set_backlashes(device, backlash, backlash);
	if (mypro2_get_backlashes(device, &backlash_in, &backlash_out)) {
		PRIVATE_DATA->backlash = backlash_in;
		if (!written || backlash_in != backlash || backlash_out != backlash) {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int requested = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	int max_position = PRIVATE_DATA->max_position;
	if (requested < PRIVATE_DATA->current_position) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, ALERT_PROPERTY, "The focuser at %d is beyond the requested maximum %d", PRIVATE_DATA->current_position, requested);
	} else if (!mypro2_command(device, false, ":07%d#", requested) || !mypro2_get_int(device, ":08#", 'M', &max_position) || max_position != requested) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = MFP_MIN_POSITION;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = max_position;
	if (max_position != PRIVATE_DATA->max_position) {
		PRIVATE_DATA->max_position = max_position;
		mypro2_update_ranges(device, true);
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		// a sync reaches the controller even for the published value
		PRIVATE_DATA->sync_target = target;
		if (mypro2_command(device, false, ":31%06d#", target)) {
			mypro2_motion_state(device, INDIGO_BUSY_STATE);
			indigo_execute_handler_in(device, MFP_POLL_DELAY, sync_finalizer);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, ":31%06d# failed", target);
			mypro2_motion_state(device, INDIGO_ALERT_STATE);
		}
	} else {
		mypro2_start_motion(device, target);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	int position = 0;
	if (mypro2_get_position(device, &position)) {
		PRIVATE_DATA->current_position = position;
		long long steps = (long long)FOCUSER_STEPS_ITEM->number.value;
		long long target = (long long)position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -steps : steps);
		// a move past either end of travel is sent to that end
		mypro2_start_motion(device, mypro2_clamp_position(device, target));
	} else {
		// without the start position the move is not sent at all
		mypro2_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	// a request with the item OFF and an abort with nothing moving send nothing
	bool active = PRIVATE_DATA->motion_active || PRIVATE_DATA->stop_pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value && active) {
		// an urgent abort can overtake a move that is still queued
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		indigo_cancel_pending_handler(device, sync_finalizer);
		int position = 0;
		if (mypro2_command(device, false, ":27#") && mypro2_get_position(device, &position)) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
			PRIVATE_DATA->stop_pending = false;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "abort failed");
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->stop_pending = true;
		}
		// an aborted move ends ALERT at the position where it stopped
		mypro2_motion_state(device, INDIGO_ALERT_STATE);
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_x_step_mode_handler(indigo_device *device) {
	X_STEP_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_STEP_MODE.on_change
	int mode = 1;
	for (int i = 0; i < X_STEP_MODE_PROPERTY->count; i++) {
		if (X_STEP_MODE_PROPERTY->items[i].sw.value) {
			mode = 1 << i;
			break;
		}
	}
	// :30# is not acknowledged, so the readback decides
	if (!mypro2_command(device, false, ":30%d#", mode) || !mypro2_update_step_mode(device) || PRIVATE_DATA->step_mode != mode) {
		X_STEP_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_STEP_MODE.on_change
	indigo_update_property(device, X_STEP_MODE_PROPERTY, NULL);
}

static void focuser_x_coils_mode_handler(indigo_device *device) {
	X_COILS_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_COILS_MODE.on_change
	int mode = X_COILS_MODE_ALWAYS_ON_ITEM->sw.value ? 1 : 0;
	if (!mypro2_command(device, false, ":12%d#", mode) || !mypro2_update_coils_mode(device) || PRIVATE_DATA->coils_mode != mode) {
		X_COILS_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_COILS_MODE.on_change
	indigo_update_property(device, X_COILS_MODE_PROPERTY, NULL);
}

static void focuser_x_settle_time_handler(indigo_device *device) {
	X_SETTLE_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_SETTLE_TIME.on_change
	int settle_time = (int)X_SETTLE_TIME_ITEM->number.target;
	if (!mypro2_command(device, false, ":71%d#", settle_time) || !mypro2_update_settle_time(device) || PRIVATE_DATA->settle_time != settle_time) {
		X_SETTLE_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_SETTLE_TIME.on_change
	indigo_update_property(device, X_SETTLE_TIME_PROPERTY, NULL);
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
		INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, MFP_BAUDRATE);
		//- focuser.on_attach
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 2;
		FOCUSER_SPEED_ITEM->number.step = 0;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 255;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = MFP_MIN_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = MFP_MIN_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = MFP_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = MFP_MAX_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 1000;
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = MFP_MIN_POSITION;
		FOCUSER_POSITION_ITEM->number.max = MFP_MAX_POSITION;
		FOCUSER_POSITION_ITEM->number.step = 100;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_STEP_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_STEP_MODE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 8);
		if (X_STEP_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_STEP_MODE_FULL_ITEM, X_STEP_MODE_FULL_ITEM_NAME, "Full step", false);
		indigo_init_switch_item(X_STEP_MODE_HALF_ITEM, X_STEP_MODE_HALF_ITEM_NAME, "1/2 step", false);
		indigo_init_switch_item(X_STEP_MODE_FOURTH_ITEM, X_STEP_MODE_FOURTH_ITEM_NAME, "1/4 step", false);
		indigo_init_switch_item(X_STEP_MODE_EIGTH_ITEM, X_STEP_MODE_EIGTH_ITEM_NAME, "1/8 step", false);
		indigo_init_switch_item(X_STEP_MODE_16TH_ITEM, X_STEP_MODE_16TH_ITEM_NAME, "1/16 step", false);
		indigo_init_switch_item(X_STEP_MODE_32TH_ITEM, X_STEP_MODE_32TH_ITEM_NAME, "1/32 step", false);
		indigo_init_switch_item(X_STEP_MODE_64TH_ITEM, X_STEP_MODE_64TH_ITEM_NAME, "1/64 step", false);
		indigo_init_switch_item(X_STEP_MODE_128TH_ITEM, X_STEP_MODE_128TH_ITEM_NAME, "1/128 step", false);
		X_COILS_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_COILS_MODE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Coils Power", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_COILS_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_COILS_MODE_IDLE_OFF_ITEM, X_COILS_MODE_IDLE_OFF_ITEM_NAME, "OFF when idle", false);
		indigo_init_switch_item(X_COILS_MODE_ALWAYS_ON_ITEM, X_COILS_MODE_ALWAYS_ON_ITEM_NAME, "Always ON", false);
		X_SETTLE_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, X_SETTLE_TIME_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Settle time", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_SETTLE_TIME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_SETTLE_TIME_ITEM, X_SETTLE_TIME_ITEM_NAME, "Settle time (ms)", 0, 255, 10, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STEP_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_COILS_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SETTLE_TIME_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_MODE_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_COMPENSATION_PROPERTY, "The focuser is moving");
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_REVERSE_MOTION_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_BACKLASH_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_LIMITS_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_STEP_MODE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(mypro2_motion_busy(device), X_STEP_MODE_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_STEP_MODE_PROPERTY, focuser_x_step_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_COILS_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_COILS_MODE_PROPERTY, focuser_x_coils_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SETTLE_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SETTLE_TIME_PROPERTY, focuser_x_settle_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_STEP_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_COILS_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_SETTLE_TIME_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_STEP_MODE_PROPERTY);
	indigo_release_property(X_COILS_MODE_PROPERTY);
	indigo_release_property(X_SETTLE_TIME_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_mypro2(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static mypro2_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (mypro2_private_data *)indigo_safe_malloc(sizeof(mypro2_private_data));
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
