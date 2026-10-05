// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_focusdreampro.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_focusdreampro.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000C
#define DRIVER_NAME          "indigo_focuser_focusdreampro"
#define DRIVER_LABEL         "AGadget FocusDreamPro Focuser"
#define FOCUSER_DEVICE_NAME  "FocusDreamPro"
#define PRIVATE_DATA         ((focusdreampro_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define FOCUSDREAMPRO_MIN_POSITION 0
#define FOCUSDREAMPRO_MAX_POSITION 1000000
#define FOCUSDREAMPRO_MAX_STEPS 100000
#define FOCUSDREAMPRO_POLL_RETRIES 3
#define FOCUSDREAMPRO_STALL_POLLS 6

//- define

#pragma mark - Property definitions

#define X_FOCUSER_DUTY_CYCLE_PROPERTY      (PRIVATE_DATA->x_focuser_duty_cycle_property)
#define X_FOCUSER_DUTY_CYCLE_ITEM          (X_FOCUSER_DUTY_CYCLE_PROPERTY->items + 0)

#define X_FOCUSER_DUTY_CYCLE_PROPERTY_NAME "X_FOCUSER_DUTY_CYCLE"
#define X_FOCUSER_DUTY_CYCLE_ITEM_NAME     "DUTY_CYCLE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_duty_cycle_property;
	//+ data
	char response[32];
	bool moving, external, aborted, poll_alert, temperature_absent;
	int poll_failures, stalled_polls, last_polled, speed, duty_cycle;
	//- data
} focusdreampro_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

// FOCUSER_SPEED is an index into the controller's per step delay, so a
// higher index is a faster focuser. One unit of the value the S:
// command carries is about 44 us of real delay, measured on an
// AstroGadget FocusDreamPro, on top of about 0.15 ms of fixed per step
// overhead.
static const int FOCUSDREAMPRO_SPEED[] = { 500, 250, 110, 40, 10, 5 };

// Requests and replies are single lines. The controller only acts on a
// request once it has seen the terminating newline; without it the
// request is never answered, whatever the gap between the bytes. Value
// setting commands echo the request, but only the leading command
// letter is guaranteed, so that is what `expected` verifies; pass 0 to
// accept any reply.
static bool focusdreampro_command(indigo_device *device, char expected, const char *format, ...) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0) {
		return false;
	}
	va_list args;
	va_start(args, format);
	long result = indigo_uni_vtprintf(PRIVATE_DATA->handle, format, args, "\n");
	va_end(args);
	if (result <= 0) {
		return false;
	}
	if (indigo_uni_read_line(PRIVATE_DATA->handle, RESPONSE, sizeof(PRIVATE_DATA->response) - 1) <= 0) {
		return false;
	}
	return expected == 0 || RESPONSE[0] == expected;
}

// "<letter>:<integer>" with nothing after the number
static bool focusdreampro_integer(const char *text, int *value) {
	char *end = NULL;
	if (text[0] == 0 || text[1] != ':' || text[2] == 0) {
		return false;
	}
	long result = strtol(text + 2, &end, 10);
	if (*end != 0) {
		return false;
	}
	*value = (int)result;
	return true;
}

static bool focusdreampro_position(indigo_device *device, int *position) {
	return focusdreampro_command(device, 'P', "P") && focusdreampro_integer(RESPONSE, position);
}

static bool focusdreampro_moving(indigo_device *device, bool *moving) {
	if (!focusdreampro_command(device, 'I', "I") || (strcmp(RESPONSE, "I:true") && strcmp(RESPONSE, "I:false"))) {
		return false;
	}
	*moving = !strcmp(RESPONSE, "I:true");
	return true;
}

// T:false is the no-probe sentinel; a malformed or implausible reading is a failed reading, never a value
static bool focusdreampro_temperature(indigo_device *device, bool *absent) {
	*absent = false;
	if (!focusdreampro_command(device, 'T', "T")) {
		return false;
	}
	if (!strcmp(RESPONSE, "T:false")) {
		*absent = true;
		return true;
	}
	char *end = NULL;
	if (RESPONSE[1] != ':' || RESPONSE[2] == 0) {
		return false;
	}
	double temperature = strtod(RESPONSE + 2, &end);
	if (*end != 0 || temperature < FOCUSER_TEMPERATURE_ITEM->number.min || temperature > FOCUSER_TEMPERATURE_ITEM->number.max) {
		return false;
	}
	FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
	return true;
}

static int focusdreampro_clamp(indigo_device *device, int position) {
	int minimum = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	int maximum = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	return position < minimum ? minimum : position > maximum ? maximum : position;
}

static void focusdreampro_ranges(indigo_device *device, bool redefine) {
	FOCUSER_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	double travel = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value - FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	FOCUSER_STEPS_ITEM->number.max = travel < FOCUSDREAMPRO_MAX_STEPS ? travel : FOCUSDREAMPRO_MAX_STEPS;
	if (redefine) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

static void focusdreampro_motion_state(indigo_device *device, indigo_property_state state, const char *message) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, message);
}

static void focusdreampro_end_motion(indigo_device *device, indigo_property_state state, const char *message) {
	PRIVATE_DATA->moving = PRIVATE_DATA->external = PRIVATE_DATA->aborted = false;
	if (state != INDIGO_OK_STATE || FOCUSER_POSITION_ITEM->number.value != FOCUSER_POSITION_ITEM->number.target) {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
	}
	focusdreampro_motion_state(device, state, message);
}

// a goto (M:) to a position other than the current one; the poll completes it
static void focusdreampro_start_motion(indigo_device *device, int position) {
	FOCUSER_POSITION_ITEM->number.target = position;
	if (position == PRIVATE_DATA->last_polled) {
		// nothing to move: no command, no waiting for a poll
		focusdreampro_motion_state(device, INDIGO_OK_STATE, NULL);
	} else if (focusdreampro_command(device, 'M', "M:%d", position)) {
		PRIVATE_DATA->moving = true;
		PRIVATE_DATA->external = PRIVATE_DATA->aborted = false;
		PRIVATE_DATA->poll_failures = PRIVATE_DATA->stalled_polls = 0;
		focusdreampro_motion_state(device, INDIGO_BUSY_STATE, NULL);
	} else {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
		focusdreampro_motion_state(device, INDIGO_ALERT_STATE, "Move refused by the controller");
	}
}

// sends the halt and confirms it with the motion status and a position readback
static bool focusdreampro_halt(indigo_device *device) {
	bool moving = true;
	int position = 0;
	bool halted = focusdreampro_command(device, 'H', "H") && focusdreampro_moving(device, &moving) && !moving && focusdreampro_position(device, &position);
	if (halted) {
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->last_polled = position;
	}
	return halted;
}

static bool focusdreampro_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (!focusdreampro_command(device, 0, "#")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FocusDreamPro not detected");
		indigo_uni_close(&PRIVATE_DATA->handle);
		return false;
	}
	// The Astrojolo controller answers the same command set with its
	// own banner; any other single line answer is accepted as an
	// unknown but compatible controller.
	if (!strcmp(RESPONSE, "FD")) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "AGadget FocusDreamPro");
	} else if (!strncmp(RESPONSE, "Jolo", 4)) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "ASCOM Jolo focuser");
	} else {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detected", INFO_DEVICE_MODEL_ITEM->text.value);
	indigo_update_property(device, INFO_PROPERTY, NULL);
	return true;
}

static void focusdreampro_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!FOCUSER_TEMPERATURE_PROPERTY->hidden) {
		bool absent = false;
		if (!focusdreampro_temperature(device, &absent)) {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		} else if (absent) {
			// a probe that went away is published IDLE once
			if (!PRIVATE_DATA->temperature_absent) {
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature probe is not connected");
			}
		} else {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
		PRIVATE_DATA->temperature_absent = absent;
	}
	// a request accepted but not yet handled owns the motion properties
	bool tracking = PRIVATE_DATA->moving || PRIVATE_DATA->external;
	if (tracking || (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE)) {
		bool moving = false;
		int position = 0;
		bool read = focusdreampro_moving(device, &moving) && focusdreampro_position(device, &position);
		if (!tracking && (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
			// a request was accepted while this poll was in flight
		} else if (!read) {
			if (tracking) {
				// a single lost readback is retried, a persistent failure halts the move
				if (++PRIVATE_DATA->poll_failures >= FOCUSDREAMPRO_POLL_RETRIES) {
					focusdreampro_command(device, 'H', "H");
					focusdreampro_end_motion(device, INDIGO_ALERT_STATE, "Status or position read failed during the move");
				}
			} else if (!PRIVATE_DATA->poll_alert) {
				PRIVATE_DATA->poll_alert = true;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		} else {
			bool changed = position != PRIVATE_DATA->last_polled;
			PRIVATE_DATA->poll_failures = 0;
			PRIVATE_DATA->stalled_polls = PRIVATE_DATA->moving && moving && !changed ? PRIVATE_DATA->stalled_polls + 1 : 0;
			PRIVATE_DATA->last_polled = position;
			FOCUSER_POSITION_ITEM->number.value = position;
			if (PRIVATE_DATA->stalled_polls >= FOCUSDREAMPRO_STALL_POLLS) {
				// the controller reports motion but the position does not change
				focusdreampro_halt(device);
				focusdreampro_end_motion(device, INDIGO_ALERT_STATE, "The focuser stalled");
			} else if (moving || (!tracking && changed)) {
				if (!PRIVATE_DATA->moving) {
					// motion the driver did not command is published BUSY at the measured position
					PRIVATE_DATA->external = true;
					FOCUSER_POSITION_ITEM->number.target = position;
				}
				focusdreampro_motion_state(device, INDIGO_BUSY_STATE, NULL);
			} else if (tracking) {
				if (PRIVATE_DATA->external) {
					FOCUSER_POSITION_ITEM->number.target = position;
				}
				focusdreampro_end_motion(device, PRIVATE_DATA->aborted ? INDIGO_ALERT_STATE : INDIGO_OK_STATE, NULL);
			} else if (PRIVATE_DATA->poll_alert) {
				PRIVATE_DATA->poll_alert = false;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
	}
	indigo_execute_handler_in(device, PRIVATE_DATA->moving || PRIVATE_DATA->external ? 0.5 : 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = focusdreampro_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			// A controller without a probe answers T:false, so the property has
			// to be unhidden again for every connection. Every connect-time
			// command has to succeed, otherwise the connection is refused.
			bool absent = false;
			int position = 0;
			FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
			connection_result = focusdreampro_temperature(device, &absent) && focusdreampro_position(device, &position);
			connection_result = connection_result && focusdreampro_command(device, 'X', "X:%d", (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value);
			connection_result = connection_result && focusdreampro_command(device, 'S', "S:%d", FOCUSDREAMPRO_SPEED[(int)FOCUSER_SPEED_ITEM->number.target]);
			connection_result = connection_result && focusdreampro_command(device, 'D', "D:%d", (int)X_FOCUSER_DUTY_CYCLE_ITEM->number.target);
			if (connection_result) {
				FOCUSER_TEMPERATURE_PROPERTY->hidden = absent;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->temperature_absent = absent;
				PRIVATE_DATA->moving = PRIVATE_DATA->external = PRIVATE_DATA->aborted = PRIVATE_DATA->poll_alert = false;
				PRIVATE_DATA->poll_failures = PRIVATE_DATA->stalled_polls = 0;
				PRIVATE_DATA->speed = (int)FOCUSER_SPEED_ITEM->number.target;
				PRIVATE_DATA->duty_cycle = (int)X_FOCUSER_DUTY_CYCLE_ITEM->number.target;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->last_polled = position;
				FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = FOCUSER_LIMITS_PROPERTY->state = FOCUSER_SPEED_PROPERTY->state = X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_OK_STATE;
				focusdreampro_ranges(device, false);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "A connect-time command failed");
				focusdreampro_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
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
		focusdreampro_command(device, 'H', "H");
		PRIVATE_DATA->moving = PRIVATE_DATA->external = PRIVATE_DATA->aborted = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			FOCUSER_TEMPERATURE_PROPERTY,
			FOCUSER_ON_POSITION_SET_PROPERTY,
			FOCUSER_SPEED_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
			FOCUSER_LIMITS_PROPERTY,
			X_FOCUSER_DUTY_CYCLE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
		focusdreampro_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_timer_callback);
	}
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	// S: has no readback: a refused write keeps the speed the controller was last given
	if (focusdreampro_command(device, 'S', "S:%d", FOCUSDREAMPRO_SPEED[(int)FOCUSER_SPEED_ITEM->number.target])) {
		PRIVATE_DATA->speed = (int)FOCUSER_SPEED_ITEM->number.target;
	} else {
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = PRIVATE_DATA->speed;
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	int position = focusdreampro_clamp(device, (int)FOCUSER_POSITION_ITEM->number.target);
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		// SYNC always reaches the controller and is confirmed by the position readback
		int readback = 0;
		bool synced = focusdreampro_command(device, 'R', "R:%d", position) && focusdreampro_position(device, &readback);
		if (synced) {
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->last_polled = readback;
			synced = readback == position;
		}
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
		focusdreampro_motion_state(device, synced ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, synced ? NULL : "Sync refused by the controller");
	} else {
		focusdreampro_start_motion(device, position);
	}
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)FOCUSER_STEPS_ITEM->number.value;
	// a relative move past either limit is sent as a move to that limit
	focusdreampro_start_motion(device, focusdreampro_clamp(device, PRIVATE_DATA->last_polled + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -steps : steps)));
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	bool motion = PRIVATE_DATA->moving || PRIVATE_DATA->external || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value && motion) {
		// An urgent abort can overtake a move that is still queued.
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		if (focusdreampro_halt(device)) {
			// an aborted move ends ALERT where the focuser actually stopped
			focusdreampro_end_motion(device, INDIGO_ALERT_STATE, NULL);
		} else {
			// the halt was not confirmed: the move is not reported completed and the poll keeps tracking it
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->aborted = true;
			PRIVATE_DATA->external = !PRIVATE_DATA->moving;
			PRIVATE_DATA->poll_failures = PRIVATE_DATA->stalled_polls = 0;
			focusdreampro_motion_state(device, INDIGO_BUSY_STATE, NULL);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int minimum = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
	int maximum = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	// an empty interval or one excluding the position is refused without a command; the controller stores the maximum
	// a change accepted while a move was being started is refused here, once the move runs
	if (PRIVATE_DATA->moving || PRIVATE_DATA->external || minimum > maximum || PRIVATE_DATA->last_polled < minimum || PRIVATE_DATA->last_polled > maximum || !focusdreampro_command(device, 'X', "X:%d", maximum)) {
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = minimum;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = maximum;
		focusdreampro_ranges(device, true);
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_x_focuser_duty_cycle_handler(indigo_device *device) {
	X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_DUTY_CYCLE.on_change
	// D: has no readback: a refused write keeps the duty cycle the controller was last given
	if (focusdreampro_command(device, 'D', "D:%d", (int)X_FOCUSER_DUTY_CYCLE_ITEM->number.target)) {
		PRIVATE_DATA->duty_cycle = (int)X_FOCUSER_DUTY_CYCLE_ITEM->number.target;
	} else {
		X_FOCUSER_DUTY_CYCLE_ITEM->number.value = X_FOCUSER_DUTY_CYCLE_ITEM->number.target = PRIVATE_DATA->duty_cycle;
		X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_DUTY_CYCLE.on_change
	indigo_update_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
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
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		// DEVICE_PORT keeps the framework's auto:// default, so the port is chosen by the CP2102
		// pattern above. A fixed /dev/ttyUSB0 or first /dev/cu.usbmodem* disabled that selection and
		// connected to whatever adapter enumerated first, e.g. a Pegasus UPB.
		//- focuser.on_attach
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 5;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = FOCUSDREAMPRO_MIN_POSITION;
		FOCUSER_POSITION_ITEM->number.max = FOCUSDREAMPRO_MAX_POSITION;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = FOCUSDREAMPRO_MAX_STEPS;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSDREAMPRO_MIN_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSDREAMPRO_MAX_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSDREAMPRO_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = FOCUSDREAMPRO_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = FOCUSDREAMPRO_MAX_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSDREAMPRO_MAX_POSITION;
		//- focuser.FOCUSER_LIMITS.on_attach
		X_FOCUSER_DUTY_CYCLE_PROPERTY = indigo_init_number_property(NULL, device->name, X_FOCUSER_DUTY_CYCLE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Duty cycle", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_FOCUSER_DUTY_CYCLE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_FOCUSER_DUTY_CYCLE_ITEM, X_FOCUSER_DUTY_CYCLE_ITEM_NAME, "Duty cycle", 0, 100, 1, 20);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_DUTY_CYCLE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_LIMITS_PROPERTY, "Motion in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_DUTY_CYCLE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_DUTY_CYCLE_PROPERTY, focuser_x_focuser_duty_cycle_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_FOCUSER_DUTY_CYCLE_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_DUTY_CYCLE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_focusdreampro(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static focusdreampro_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			patterns[0].vendor_id = 0x10c4;
			patterns[0].product_id = 0xea60;
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = (focusdreampro_private_data *)indigo_safe_malloc(sizeof(focusdreampro_private_data));
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
