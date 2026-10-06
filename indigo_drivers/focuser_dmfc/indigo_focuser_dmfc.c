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

// This file generated from indigo_focuser_dmfc.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_dmfc.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000014
#define DRIVER_NAME          "indigo_focuser_dmfc"
#define DRIVER_LABEL         "PegasusAstro DMFC Focuser"
#define FOCUSER_DEVICE_NAME  "Pegasus DMFC"
#define PRIVATE_DATA         ((dmfc_private_data *)device->private_data)

//+ define

// polls without progress while the controller reports a move before it counts as stalled
#define DMFC_STALL_POLLS     5
// consecutive failed polls during a move before the focuser is stopped
#define DMFC_POLL_FAILURES   2
#define DMFC_MIN_TEMPERATURE (-55)
#define DMFC_MAX_TEMPERATURE 125
#define DMFC_MAX_STEPS       9999999

//- define

#pragma mark - Property definitions

#define X_FOCUSER_MOTOR_TYPE_PROPERTY          (PRIVATE_DATA->x_focuser_motor_type_property)
#define X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM      (X_FOCUSER_MOTOR_TYPE_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_TYPE_DC_ITEM           (X_FOCUSER_MOTOR_TYPE_PROPERTY->items + 1)

#define X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME     "X_FOCUSER_MOTOR_TYPE"
#define X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME "STEPPER"
#define X_FOCUSER_MOTOR_TYPE_DC_ITEM_NAME      "DC"

#define X_FOCUSER_ENCODER_PROPERTY           (PRIVATE_DATA->x_focuser_encoder_property)
#define X_FOCUSER_ENCODER_ENABLED_ITEM       (X_FOCUSER_ENCODER_PROPERTY->items + 0)
#define X_FOCUSER_ENCODER_DISABLED_ITEM      (X_FOCUSER_ENCODER_PROPERTY->items + 1)

#define X_FOCUSER_ENCODER_PROPERTY_NAME      "X_FOCUSER_ENCODER"
#define X_FOCUSER_ENCODER_ENABLED_ITEM_NAME  "ENABLED"
#define X_FOCUSER_ENCODER_DISABLED_ITEM_NAME "DISABLED"

#define X_FOCUSER_LED_PROPERTY           (PRIVATE_DATA->x_focuser_led_property)
#define X_FOCUSER_LED_ENABLED_ITEM       (X_FOCUSER_LED_PROPERTY->items + 0)
#define X_FOCUSER_LED_DISABLED_ITEM      (X_FOCUSER_LED_PROPERTY->items + 1)

#define X_FOCUSER_LED_PROPERTY_NAME      "X_FOCUSER_LED"
#define X_FOCUSER_LED_ENABLED_ITEM_NAME  "ENABLED"
#define X_FOCUSER_LED_DISABLED_ITEM_NAME "DISABLED"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_motor_type_property;
	indigo_property *x_focuser_encoder_property;
	indigo_property *x_focuser_led_property;
	//+ data
	char response[128];
	// A motion the controller runs: the poll ends FOCUSER_POSITION / FOCUSER_STEPS BUSY only for it, not
	// for a request that is copied and queued but whose handler has not sent the move yet.
	bool moving;
	// external_motion: the driver did not command the move; stop_pending: the last abort could not be
	// sent; poll_failed: the ALERT on FOCUSER_POSITION comes from a failed idle poll, not a failed move
	bool external_motion, stop_pending, poll_failed;
	int poll_failures, stall_polls;
	// the settings the controller last confirmed and the limits the client last accepted
	int backlash, speed, reverse, motor, encoder_disabled, led, min_limit, max_limit;
	//- data
} dmfc_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

typedef struct {
	int motor, position, led, reverse, encoder_disabled, backlash;
	bool moving, temperature_valid;
	double temperature;
	char model[32], version[32];
} dmfc_status_data;

// Commands without a reply (C, H, M, G, S, W) are only written. A reply has to end with its
// line end; one cut off by the timeout or longer than the buffer is not a reply.
static bool dmfc_command(indigo_device *device, char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
		if (result > 0 && command[0] != 'C' && command[0] != 'H' && command[0] != 'M' && command[0] != 'G' && command[0] != 'S' && command[0] != 'W') {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(1));
			if (result > 0 && PRIVATE_DATA->response[result - 1] == '\n') {
				PRIVATE_DATA->response[result - 1] = 0;
			} else {
				result = -1;
			}
		}
	}
	return result > 0;
}

static bool dmfc_parse_int(const char *text, int *value) {
	char *end = NULL;
	long result = strtol(text, &end, 10);
	if (text == NULL || end == text || *end != 0) {
		return false;
	}
	*value = (int)result;
	return true;
}

static bool dmfc_parse_flag(const char *text, int *value) {
	if (text == NULL || (strcmp(text, "0") && strcmp(text, "1"))) {
		return false;
	}
	*value = *text == '1';
	return true;
}

static bool dmfc_parse_temperature(const char *text, double *value) {
	char *end = NULL;
	*value = text == NULL ? 0 : strtod(text, &end);
	return text != NULL && end != text && *end == 0 && *value >= DMFC_MIN_TEMPERATURE && *value <= DMFC_MAX_TEMPERATURE;
}

// "OK_<model>:<version>:<motor>:<temperature>:<position>:<moving>:<led>:<reverse>:<encoder>:<backlash>";
// a missing or malformed field rejects the line, except the temperature, which is only marked invalid.
static bool dmfc_status(indigo_device *device, dmfc_status_data *status) {
	if (!dmfc_command(device, "A") || strncmp(PRIVATE_DATA->response, "OK_", 3)) {
		return false;
	}
	char *fields[10], *pnt;
	fields[0] = strtok_r(PRIVATE_DATA->response, ":", &pnt);
	for (int i = 1; i < 10; i++) {
		fields[i] = strtok_r(NULL, ":", &pnt);
	}
	int moving = 0;
	if (fields[9] == NULL || strtok_r(NULL, ":", &pnt) != NULL || !dmfc_parse_flag(fields[2], &status->motor) || !dmfc_parse_int(fields[4], &status->position) || !dmfc_parse_flag(fields[5], &moving) || !dmfc_parse_flag(fields[6], &status->led) || !dmfc_parse_flag(fields[7], &status->reverse) || !dmfc_parse_flag(fields[8], &status->encoder_disabled) || !dmfc_parse_int(fields[9], &status->backlash)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Malformed status");
		return false;
	}
	status->moving = moving;
	status->temperature_valid = dmfc_parse_temperature(fields[3], &status->temperature);
	snprintf(status->model, sizeof(status->model), "%s", fields[0] + 3);
	snprintf(status->version, sizeof(status->version), "%s", fields[1]);
	return true;
}

// A setting command answered with "<reply prefix><value>" is confirmed only by that value.
static bool dmfc_set(indigo_device *device, const char *command, const char *prefix, int request, int expected, int *applied) {
	int value = 0;
	if (!dmfc_command(device, "%s:%d", command, request) || strncmp(PRIVATE_DATA->response, prefix, strlen(prefix)) || !dmfc_parse_flag(PRIVATE_DATA->response + strlen(prefix), &value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s:%d not acknowledged", command, request);
		return false;
	}
	*applied = value;
	if (value != expected) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s:%d applied as %d", command, request, value);
		return false;
	}
	return true;
}

static bool dmfc_open(indigo_device *device) {
	// the controller talks at 19200 baud
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (dmfc_command(device, "#") && !strncmp(PRIVATE_DATA->response, "OK_", 3)) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value ,"DMFC Focuser");
			if (dmfc_command(device, "V")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void dmfc_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ focuser.code

// Requests that move the focuser or change its geometry wait for the running motion.
static bool dmfc_motion_busy(indigo_device *device) {
	return PRIVATE_DATA->moving || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE;
}

// FOCUSER_POSITION and FOCUSER_STEPS cover the FOCUSER_LIMITS interval; a running
// session republishes them, because ranges travel only with a definition.
static void dmfc_update_ranges(indigo_device *device, bool redefine) {
	FOCUSER_POSITION_ITEM->number.min = PRIVATE_DATA->min_limit;
	FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->max_limit;
	long long span = (long long)PRIVATE_DATA->max_limit - PRIVATE_DATA->min_limit;
	FOCUSER_STEPS_ITEM->number.max = span < 1 ? 1 : span > DMFC_MAX_STEPS ? DMFC_MAX_STEPS : span;
	if (redefine) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

static void dmfc_start_motion(indigo_device *device) {
	PRIVATE_DATA->moving = true;
	PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_failed = false;
	PRIVATE_DATA->poll_failures = PRIVATE_DATA->stall_polls = 0;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
}

// A move ends with both motion properties in the same state; a failed or aborted move and a move
// the driver did not command end with the target at the measured position.
static void dmfc_end_motion(indigo_device *device, indigo_property_state state, const char *message) {
	if (state == INDIGO_ALERT_STATE || PRIVATE_DATA->external_motion) {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
	}
	PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_failed = false;
	PRIVATE_DATA->poll_failures = PRIVATE_DATA->stall_polls = 0;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, message);
}

static void dmfc_show_settings(indigo_device *device) {
	FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
	FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = PRIVATE_DATA->speed;
	indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reverse ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
	indigo_set_switch(X_FOCUSER_MOTOR_TYPE_PROPERTY, PRIVATE_DATA->motor ? X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM : X_FOCUSER_MOTOR_TYPE_DC_ITEM, true);
	indigo_set_switch(X_FOCUSER_ENCODER_PROPERTY, PRIVATE_DATA->encoder_disabled ? X_FOCUSER_ENCODER_DISABLED_ITEM : X_FOCUSER_ENCODER_ENABLED_ITEM, true);
	indigo_set_switch(X_FOCUSER_LED_PROPERTY, PRIVATE_DATA->led ? X_FOCUSER_LED_ENABLED_ITEM : X_FOCUSER_LED_DISABLED_ITEM, true);
}

static void dmfc_poll_failed(indigo_device *device) {
	if (PRIVATE_DATA->moving) {
		// a single lost poll is retried, a second one stops the move
		if (++PRIVATE_DATA->poll_failures >= DMFC_POLL_FAILURES) {
			dmfc_command(device, "H");
			dmfc_end_motion(device, INDIGO_ALERT_STATE, "Status read failed, the focuser was stopped");
		}
	} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_OK_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		PRIVATE_DATA->poll_failed = true;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Position read failed");
	}
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	double temperature;
	if (dmfc_command(device, "T") && dmfc_parse_temperature(PRIVATE_DATA->response, &temperature)) {
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temperature || FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_OK_STATE) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
			INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_TEMPERATURE_PROPERTY, INDIGO_OK_STATE, NULL);
		}
	} else if (FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_ALERT_STATE) {
		// the last valid reading stays published
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_TEMPERATURE_PROPERTY, INDIGO_ALERT_STATE, "Temperature read failed");
	}
	int position = 0, moving = 0;
	if (!dmfc_command(device, "P") || !dmfc_parse_int(PRIVATE_DATA->response, &position) || !dmfc_command(device, "I") || !dmfc_parse_flag(PRIVATE_DATA->response, &moving)) {
		dmfc_poll_failed(device);
	} else {
		bool changed = FOCUSER_POSITION_ITEM->number.value != position;
		PRIVATE_DATA->poll_failures = 0;
		if (PRIVATE_DATA->moving) {
			FOCUSER_POSITION_ITEM->number.value = position;
			if (!moving) {
				dmfc_end_motion(device, INDIGO_OK_STATE, NULL);
			} else if (!changed && ++PRIVATE_DATA->stall_polls >= DMFC_STALL_POLLS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser stalled at %d, stopping it", position);
				dmfc_command(device, "H");
				dmfc_end_motion(device, INDIGO_ALERT_STATE, "The focuser stalled and was stopped");
			} else if (changed) {
				// progress keeps the BUSY state of the running move
				PRIVATE_DATA->stall_polls = 0;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			// a request the bus accepted waits for its handler; the poll leaves it alone
		} else if (moving) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The focuser moves on its own from %d", position);
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			dmfc_start_motion(device);
			PRIVATE_DATA->external_motion = true;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (changed || PRIVATE_DATA->poll_failed) {
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			if (PRIVATE_DATA->poll_failed) {
				// only an ALERT left by a failed poll is cleared, never one of a failed move
				PRIVATE_DATA->poll_failed = false;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = dmfc_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			// the status line and the speed are mandatory: a controller that does not answer them is refused
			dmfc_status_data status;
			double speed = 0;
			char *end = NULL;
			connection_result = dmfc_status(device, &status) && dmfc_command(device, "B") && !strncmp(PRIVATE_DATA->response, "B:", 2) && (speed = strtod(PRIVATE_DATA->response + 2, &end), end != PRIVATE_DATA->response + 2 && *end == 0);
			if (connection_result) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, status.model);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, status.version);
				PRIVATE_DATA->motor = status.motor;
				PRIVATE_DATA->led = status.led;
				PRIVATE_DATA->reverse = status.reverse;
				PRIVATE_DATA->encoder_disabled = status.encoder_disabled;
				PRIVATE_DATA->backlash = status.backlash;
				PRIVATE_DATA->speed = (int)(speed + 0.5);
				if (status.temperature_valid) {
					FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = status.temperature;
					FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = status.position;
				PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = PRIVATE_DATA->stop_pending = PRIVATE_DATA->poll_failed = false;
				FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				if (status.moving) {
					// a move the driver did not command is followed until the controller stops
					dmfc_start_motion(device);
					PRIVATE_DATA->external_motion = true;
				}
				PRIVATE_DATA->min_limit = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
				PRIVATE_DATA->max_limit = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
				dmfc_update_ranges(device, false);
				dmfc_show_settings(device);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "The controller did not answer the connect sequence");
				dmfc_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_MOTOR_TYPE_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_ENCODER_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_LED_PROPERTY, NULL);
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
		if (PRIVATE_DATA->moving || PRIVATE_DATA->stop_pending) {
			dmfc_command(device, "H");
		}
		PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = PRIVATE_DATA->stop_pending = PRIVATE_DATA->poll_failed = false;
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			X_FOCUSER_MOTOR_TYPE_PROPERTY,
			X_FOCUSER_ENCODER_PROPERTY,
			X_FOCUSER_LED_PROPERTY,
			FOCUSER_BACKLASH_PROPERTY,
			FOCUSER_REVERSE_MOTION_PROPERTY,
			FOCUSER_TEMPERATURE_PROPERTY,
			FOCUSER_SPEED_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_ON_POSITION_SET_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
			FOCUSER_LIMITS_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_FOCUSER_MOTOR_TYPE_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_ENCODER_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_LED_PROPERTY, NULL);
		dmfc_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_timer_callback);
	}
}

static void focuser_x_focuser_motor_type_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_MOTOR_TYPE.on_change
	// R answers with the motor type the controller applied
	if (!dmfc_set(device, "R", "", X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM->sw.value ? 1 : 0, X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM->sw.value ? 1 : 0, &PRIVATE_DATA->motor)) {
		X_FOCUSER_MOTOR_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_set_switch(X_FOCUSER_MOTOR_TYPE_PROPERTY, PRIVATE_DATA->motor ? X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM : X_FOCUSER_MOTOR_TYPE_DC_ITEM, true);
	//- focuser.X_FOCUSER_MOTOR_TYPE.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_TYPE_PROPERTY, NULL);
}

static void focuser_x_focuser_encoder_handler(indigo_device *device) {
	X_FOCUSER_ENCODER_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_ENCODER.on_change
	// E:1 turns the encoder off, E:0 on; the reply repeats the setting
	int disabled = X_FOCUSER_ENCODER_DISABLED_ITEM->sw.value ? 1 : 0;
	if (!dmfc_set(device, "E", "E:", disabled, disabled, &PRIVATE_DATA->encoder_disabled)) {
		X_FOCUSER_ENCODER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_set_switch(X_FOCUSER_ENCODER_PROPERTY, PRIVATE_DATA->encoder_disabled ? X_FOCUSER_ENCODER_DISABLED_ITEM : X_FOCUSER_ENCODER_ENABLED_ITEM, true);
	//- focuser.X_FOCUSER_ENCODER.on_change
	indigo_update_property(device, X_FOCUSER_ENCODER_PROPERTY, NULL);
}

static void focuser_x_focuser_led_handler(indigo_device *device) {
	X_FOCUSER_LED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_LED.on_change
	// L:2 switches the LED on, L:1 off; the reply is the LED status, 1 on and 0 off
	int on = X_FOCUSER_LED_ENABLED_ITEM->sw.value ? 1 : 0;
	if (!dmfc_set(device, "L", "L:", on ? 2 : 1, on, &PRIVATE_DATA->led)) {
		X_FOCUSER_LED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_set_switch(X_FOCUSER_LED_PROPERTY, PRIVATE_DATA->led ? X_FOCUSER_LED_ENABLED_ITEM : X_FOCUSER_LED_DISABLED_ITEM, true);
	//- focuser.X_FOCUSER_LED.on_change
	indigo_update_property(device, X_FOCUSER_LED_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	// C is not answered, so the status line decides
	int backlash = (int)FOCUSER_BACKLASH_ITEM->number.value;
	dmfc_status_data status;
	bool written = dmfc_command(device, "C:%d", backlash);
	if (dmfc_status(device, &status)) {
		PRIVATE_DATA->backlash = status.backlash;
	} else {
		written = false;
	}
	if (!written || PRIVATE_DATA->backlash != backlash) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	int reverse = FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 0 : 1;
	if (!dmfc_set(device, "N", "N:", reverse, reverse, &PRIVATE_DATA->reverse)) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reverse ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	// S is not answered, so the maximum speed the controller reports with B decides
	int speed = (int)FOCUSER_SPEED_ITEM->number.value;
	double reported = 0;
	char *end = NULL;
	bool written = dmfc_command(device, "S:%d", speed);
	if (dmfc_command(device, "B") && !strncmp(PRIVATE_DATA->response, "B:", 2) && (reported = strtod(PRIVATE_DATA->response + 2, &end), end != PRIVATE_DATA->response + 2 && *end == 0)) {
		PRIVATE_DATA->speed = (int)(reported + 0.5);
	} else {
		written = false;
	}
	if (!written || PRIVATE_DATA->speed != speed) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = PRIVATE_DATA->speed;
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	// G counts inward positive; a move past the limit it approaches is sent to that limit
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	bool inward = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value;
	if (inward && position + steps > PRIVATE_DATA->max_limit) {
		steps = PRIVATE_DATA->max_limit - position;
	} else if (!inward && position - steps < PRIVATE_DATA->min_limit) {
		steps = position - PRIVATE_DATA->min_limit;
	}
	if (steps < 0) {
		// beyond the limit already (after a sync): never move further out
		steps = 0;
	}
	FOCUSER_STEPS_ITEM->number.value = FOCUSER_STEPS_ITEM->number.target = steps;
	int delta = inward ? steps : -steps;
	if (steps == 0) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else if (dmfc_command(device, "G:%d", delta)) {
		FOCUSER_POSITION_ITEM->number.target = position + delta;
		dmfc_start_motion(device);
	} else {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
		FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	if (position < PRIVATE_DATA->min_limit) {
		position = PRIVATE_DATA->min_limit;
	}
	if (position > PRIVATE_DATA->max_limit) {
		position = PRIVATE_DATA->max_limit;
	}
	FOCUSER_POSITION_ITEM->number.target = position;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		if (position == (int)FOCUSER_POSITION_ITEM->number.value) {
			// already there: OK at once, without a command
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		} else if (dmfc_command(device, "M:%d", position)) {
			dmfc_start_motion(device);
		} else {
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		// W is not answered, so the position read back decides; a sync reaches the
		// controller even for the published value
		int readback = 0;
		if (dmfc_command(device, "W:%d", position) && dmfc_command(device, "P") && dmfc_parse_int(PRIVATE_DATA->response, &readback) && readback == position) {
			FOCUSER_POSITION_ITEM->number.value = position;
			PRIVATE_DATA->poll_failed = false;
		} else {
			if (dmfc_parse_int(PRIVATE_DATA->response, &readback)) {
				FOCUSER_POSITION_ITEM->number.value = readback;
			}
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		// an abort with nothing moving sends nothing
		if (PRIVATE_DATA->moving || PRIVATE_DATA->stop_pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			// an urgent abort can overtake a move that is still queued
			indigo_cancel_pending_handler(device, focuser_position_handler);
			indigo_cancel_pending_handler(device, focuser_steps_handler);
			if (dmfc_command(device, "H")) {
				int position = 0;
				PRIVATE_DATA->stop_pending = false;
				if (dmfc_command(device, "P") && dmfc_parse_int(PRIVATE_DATA->response, &position)) {
					FOCUSER_POSITION_ITEM->number.value = position;
				}
				// an aborted move ends ALERT at the position where it stopped
				dmfc_end_motion(device, INDIGO_ALERT_STATE, NULL);
			} else {
				// the motor may still run, so the move stays BUSY
				PRIVATE_DATA->stop_pending = true;
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	// the limits belong to the driver, nothing is sent to the controller
	int min = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	int max = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	if (min > max || position < min || position > max) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, ALERT_PROPERTY, "Limits %d to %d are empty or exclude the focuser at %d", min, max, position);
	} else {
		PRIVATE_DATA->min_limit = min;
		PRIVATE_DATA->max_limit = max;
		dmfc_update_ranges(device, true);
	}
	FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = PRIVATE_DATA->min_limit;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = PRIVATE_DATA->max_limit;
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- focuser.on_attach
		X_FOCUSER_MOTOR_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_MOTOR_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM, X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME, "Stepper motor", false);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_DC_ITEM, X_FOCUSER_MOTOR_TYPE_DC_ITEM_NAME, "DC Motor", false);
		X_FOCUSER_ENCODER_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_ENCODER_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Encoder state", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_ENCODER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_ENCODER_ENABLED_ITEM, X_FOCUSER_ENCODER_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_FOCUSER_ENCODER_DISABLED_ITEM, X_FOCUSER_ENCODER_DISABLED_ITEM_NAME, "Disabled", false);
		X_FOCUSER_LED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_LED_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "LED status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_LED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_LED_ENABLED_ITEM, X_FOCUSER_LED_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_FOCUSER_LED_DISABLED_ITEM, X_FOCUSER_LED_DISABLED_ITEM_NAME, "Disabled", false);
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = DMFC_MAX_STEPS;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = -9999999;
		strcpy(FOCUSER_LIMITS_MIN_POSITION_ITEM->number.format, "%.0f");
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 9999999;
		strcpy(FOCUSER_LIMITS_MAX_POSITION_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_LIMITS.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_TYPE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_ENCODER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_LED_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_TYPE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), X_FOCUSER_MOTOR_TYPE_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_TYPE_PROPERTY, focuser_x_focuser_motor_type_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_ENCODER_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), X_FOCUSER_ENCODER_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_ENCODER_PROPERTY, focuser_x_focuser_encoder_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_LED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_LED_PROPERTY, focuser_x_focuser_led_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), FOCUSER_BACKLASH_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), FOCUSER_REVERSE_MOTION_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(dmfc_motion_busy(device), FOCUSER_LIMITS_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_MOTOR_TYPE_PROPERTY);
	indigo_release_property(X_FOCUSER_ENCODER_PROPERTY);
	indigo_release_property(X_FOCUSER_LED_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_dmfc(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static dmfc_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[2] = { 0 };
			strcpy(patterns[0].product_string, "DMFC");
			strcpy(patterns[0].vendor_string, "Pegasus Astro");
			strcpy(patterns[1].product_string, "FocusCube");
			strcpy(patterns[1].vendor_string, "Pegasus Astro");
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 2);
			private_data = (dmfc_private_data *)indigo_safe_malloc(sizeof(dmfc_private_data));
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
