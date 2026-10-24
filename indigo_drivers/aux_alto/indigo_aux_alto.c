// Copyright (c) 2026 Rumen G. Bogdanovski
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

// This file generated from indigo_aux_alto.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include "../shared/primalucelib/primalucelib.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_alto.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000001
#define DRIVER_NAME          "indigo_aux_alto"
#define DRIVER_LABEL         "PrimaluceLab ALTO Cover"
#define AUX_DEVICE_NAME      "PrimaluceLab ALTO"
#define PRIVATE_DATA         ((alto_private_data *)device->private_data)

//+ define

// The controller connection, see indigo_drivers/shared/primalucelib (linked as a static library).
#define LINK                 (&PRIVATE_DATA->link)
// Idle poll period (seconds); the temperature and the supplies are read every ENVIRONMENT_TICKS polls.
#define POLL_PERIOD          2
#define ENVIRONMENT_TICKS    5
// A move is polled this often (seconds).
#define MOTION_POLL          0.5
// A cover that reports motion without progress for this many polls has stalled.
#define STALL_POLLS          10
// Consecutive failed readbacks tolerated during a move.
#define MAX_POLL_FAILURES    3
// A full stroke takes about 34 s on an ALTO1; a move still running after this long is stopped (seconds).
#define MOTION_TIMEOUT       60
// Polls after the start during which a stopped motor that is not at the target is still starting.
#define START_POLLS          2

//- define

#pragma mark - Property definitions

#define AUX_COVER_PROPERTY             (PRIVATE_DATA->aux_cover_property)
#define AUX_COVER_OPEN_ITEM            (AUX_COVER_PROPERTY->items + 0)
#define AUX_COVER_CLOSE_ITEM           (AUX_COVER_PROPERTY->items + 1)

#define X_COVER_POSITION_PROPERTY      (PRIVATE_DATA->x_cover_position_property)
#define X_COVER_POSITION_ITEM          (X_COVER_POSITION_PROPERTY->items + 0)

#define X_COVER_POSITION_PROPERTY_NAME "X_COVER_POSITION"
#define X_COVER_POSITION_ITEM_NAME     "POSITION"

#define X_COVER_ABORT_PROPERTY         (PRIVATE_DATA->x_cover_abort_property)
#define X_COVER_ABORT_ITEM             (X_COVER_ABORT_PROPERTY->items + 0)

#define X_COVER_ABORT_PROPERTY_NAME    "X_COVER_ABORT"
#define X_COVER_ABORT_ITEM_NAME        "ABORT"

#define X_STATE_PROPERTY               (PRIVATE_DATA->x_state_property)
#define X_STATE_TEMPERATURE_ITEM       (X_STATE_PROPERTY->items + 0)
#define X_STATE_VIN_12V_ITEM           (X_STATE_PROPERTY->items + 1)
#define X_STATE_VIN_USB_ITEM           (X_STATE_PROPERTY->items + 2)

#define X_STATE_PROPERTY_NAME          "X_STATE"
#define X_STATE_TEMPERATURE_ITEM_NAME  "TEMPERATURE"
#define X_STATE_VIN_12V_ITEM_NAME      "VIN_12V"
#define X_STATE_VIN_USB_ITEM_NAME      "VIN_USB"

#define X_PRESETS_PROPERTY             (PRIVATE_DATA->x_presets_property)
#define X_PRESETS_1_ITEM               (X_PRESETS_PROPERTY->items + 0)
#define X_PRESETS_2_ITEM               (X_PRESETS_PROPERTY->items + 1)
#define X_PRESETS_3_ITEM               (X_PRESETS_PROPERTY->items + 2)

#define X_PRESETS_PROPERTY_NAME        "X_PRESETS"
#define X_PRESETS_1_ITEM_NAME          "PRESET_1"
#define X_PRESETS_2_ITEM_NAME          "PRESET_2"
#define X_PRESETS_3_ITEM_NAME          "PRESET_3"

#define X_PRESET_NAMES_PROPERTY        (PRIVATE_DATA->x_preset_names_property)
#define X_PRESET_NAMES_1_ITEM          (X_PRESET_NAMES_PROPERTY->items + 0)
#define X_PRESET_NAMES_2_ITEM          (X_PRESET_NAMES_PROPERTY->items + 1)
#define X_PRESET_NAMES_3_ITEM          (X_PRESET_NAMES_PROPERTY->items + 2)

#define X_PRESET_NAMES_PROPERTY_NAME   "X_PRESET_NAMES"
#define X_PRESET_NAMES_1_ITEM_NAME     "PRESET_1"
#define X_PRESET_NAMES_2_ITEM_NAME     "PRESET_2"
#define X_PRESET_NAMES_3_ITEM_NAME     "PRESET_3"

#define X_PRESET_RECALL_PROPERTY       (PRIVATE_DATA->x_preset_recall_property)
#define X_PRESET_RECALL_1_ITEM         (X_PRESET_RECALL_PROPERTY->items + 0)
#define X_PRESET_RECALL_2_ITEM         (X_PRESET_RECALL_PROPERTY->items + 1)
#define X_PRESET_RECALL_3_ITEM         (X_PRESET_RECALL_PROPERTY->items + 2)

#define X_PRESET_RECALL_PROPERTY_NAME  "X_PRESET_RECALL"
#define X_PRESET_RECALL_1_ITEM_NAME    "PRESET_1"
#define X_PRESET_RECALL_2_ITEM_NAME    "PRESET_2"
#define X_PRESET_RECALL_3_ITEM_NAME    "PRESET_3"

#define X_CALIBRATION_PROPERTY         (PRIVATE_DATA->x_calibration_property)
#define X_CALIBRATION_START_ITEM       (X_CALIBRATION_PROPERTY->items + 0)
#define X_CALIBRATION_STORE_ITEM       (X_CALIBRATION_PROPERTY->items + 1)
#define X_CALIBRATION_CANCEL_ITEM      (X_CALIBRATION_PROPERTY->items + 2)

#define X_CALIBRATION_PROPERTY_NAME    "X_CALIBRATION"
#define X_CALIBRATION_START_ITEM_NAME  "START"
#define X_CALIBRATION_STORE_ITEM_NAME  "STORE"
#define X_CALIBRATION_CANCEL_ITEM_NAME "CANCEL"

#define X_CALIBRATION_MOTION_PROPERTY             (PRIVATE_DATA->x_calibration_motion_property)
#define X_CALIBRATION_MOTION_CLOSE_FAST_ITEM      (X_CALIBRATION_MOTION_PROPERTY->items + 0)
#define X_CALIBRATION_MOTION_CLOSE_SLOW_ITEM      (X_CALIBRATION_MOTION_PROPERTY->items + 1)
#define X_CALIBRATION_MOTION_OPEN_FAST_ITEM       (X_CALIBRATION_MOTION_PROPERTY->items + 2)
#define X_CALIBRATION_MOTION_OPEN_SLOW_ITEM       (X_CALIBRATION_MOTION_PROPERTY->items + 3)
#define X_CALIBRATION_MOTION_STOP_ITEM            (X_CALIBRATION_MOTION_PROPERTY->items + 4)

#define X_CALIBRATION_MOTION_PROPERTY_NAME        "X_CALIBRATION_MOTION"
#define X_CALIBRATION_MOTION_CLOSE_FAST_ITEM_NAME "CLOSE_FAST"
#define X_CALIBRATION_MOTION_CLOSE_SLOW_ITEM_NAME "CLOSE_SLOW"
#define X_CALIBRATION_MOTION_OPEN_FAST_ITEM_NAME  "OPEN_FAST"
#define X_CALIBRATION_MOTION_OPEN_SLOW_ITEM_NAME  "OPEN_SLOW"
#define X_CALIBRATION_MOTION_STOP_ITEM_NAME       "STOP"

#define X_LEDS_PROPERTY                (PRIVATE_DATA->x_leds_property)
#define X_LEDS_OFF_ITEM                (X_LEDS_PROPERTY->items + 0)
#define X_LEDS_DIM_ITEM                (X_LEDS_PROPERTY->items + 1)
#define X_LEDS_MIDDLE_ITEM             (X_LEDS_PROPERTY->items + 2)
#define X_LEDS_ON_ITEM                 (X_LEDS_PROPERTY->items + 3)

#define X_LEDS_PROPERTY_NAME           "X_LEDS"
#define X_LEDS_OFF_ITEM_NAME           "OFF"
#define X_LEDS_DIM_ITEM_NAME           "DIM"
#define X_LEDS_MIDDLE_ITEM_NAME        "MIDDLE"
#define X_LEDS_ON_ITEM_NAME            "ON"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_cover_property;
	indigo_property *x_cover_position_property;
	indigo_property *x_cover_abort_property;
	indigo_property *x_state_property;
	indigo_property *x_presets_property;
	indigo_property *x_preset_names_property;
	indigo_property *x_preset_recall_property;
	indigo_property *x_calibration_property;
	indigo_property *x_calibration_motion_property;
	indigo_property *x_leds_property;
	//+ data
	primalucelib_link link;
	// The travel in steps (CAL_MAXPOS); 0 when the cover is not calibrated.
	int travel;
	int target, last_position;
	int stalled_polls, poll_failures, motion_polls;
	double motion_started;
	bool moving, external_motion, abort_requested, poll_alert;
	// 0 outside a calibration, 1 while the closed position is searched, 2 while the open position is searched.
	int calibration_phase;
	int preset_steps[PRIMALUCE_COVER_PRESET_COUNT];
	char preset_names[PRIMALUCE_COVER_PRESET_COUNT][INDIGO_VALUE_SIZE];
	indigo_item *leds_item;
	int environment_ticks;
	//- data
} alto_private_data;

#pragma mark - Low level code

//+ code

static void cover_motion_finalizer(indigo_device *device);

static indigo_item *selected_item(indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value) {
			return property->items + i;
		}
	}
	return NULL;
}

// Publishes property with the reason the controller gave for the last refusal, if it gave one.
static void update_with_refusal(indigo_device *device, indigo_property *property) {
	const char *reason = primalucelib_last_error(LINK);
	if (*reason) {
		indigo_update_property(device, property, "%s", reason);
	} else {
		indigo_update_property(device, property, NULL);
	}
}

static int steps_to_percent(indigo_device *device, int steps) {
	return PRIVATE_DATA->travel > 0 ? (int)lround(100.0 * steps / PRIVATE_DATA->travel) : 0;
}

static int percent_to_steps(indigo_device *device, int percent) {
	return (int)lround(percent * (double)PRIVATE_DATA->travel / 100.0);
}

// AUX_COVER shows OPEN at 100 % and CLOSE at 0 %; between them neither item is on.
static void cover_show_ends(indigo_device *device, int position) {
	AUX_COVER_OPEN_ITEM->sw.value = position == PRIMALUCE_COVER_OPEN;
	AUX_COVER_CLOSE_ITEM->sw.value = position == PRIMALUCE_COVER_CLOSED;
}

// Ends a move: the value shows the position reached, the target keeps the request.
static void cover_end_motion(indigo_device *device, indigo_property_state state, const char *message) {
	PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = PRIVATE_DATA->abort_requested = false;
	cover_show_ends(device, PRIVATE_DATA->last_position);
	X_COVER_POSITION_ITEM->number.value = PRIVATE_DATA->last_position;
	AUX_COVER_PROPERTY->state = X_COVER_POSITION_PROPERTY->state = state;
	indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
	if (message != NULL) {
		indigo_update_property(device, X_COVER_POSITION_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
	}
}

// Refuses a move request: the target goes back to the position, the reason is published.
static void cover_refuse(indigo_device *device, indigo_property *property, const char *message) {
	X_COVER_POSITION_ITEM->number.target = X_COVER_POSITION_ITEM->number.value = PRIVATE_DATA->last_position;
	cover_show_ends(device, PRIVATE_DATA->last_position);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

// Publishes a move the controller accepted, to target %; the caller schedules cover_motion_finalizer.
static void cover_follow_motion(indigo_device *device, int target) {
	PRIVATE_DATA->target = target;
	PRIVATE_DATA->moving = true;
	PRIVATE_DATA->abort_requested = false;
	PRIVATE_DATA->stalled_polls = PRIVATE_DATA->poll_failures = PRIVATE_DATA->motion_polls = 0;
	PRIVATE_DATA->motion_started = indigo_monotonic_time();
	X_COVER_POSITION_ITEM->number.target = target;
	AUX_COVER_OPEN_ITEM->sw.value = target == PRIMALUCE_COVER_OPEN;
	AUX_COVER_CLOSE_ITEM->sw.value = target == PRIMALUCE_COVER_CLOSED;
	AUX_COVER_PROPERTY->state = X_COVER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
	indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
}

// Why a move cannot start now, or NULL.
static const char *cover_busy_reason(indigo_device *device) {
	if (PRIVATE_DATA->calibration_phase) {
		return "A calibration is running";
	}
	if (PRIVATE_DATA->travel <= 0) {
		return "The cover needs calibration";
	}
	if (PRIVATE_DATA->moving || PRIVATE_DATA->external_motion) {
		return "The cover is moving";
	}
	return NULL;
}

// Starts a move to target % (0 to 100; the controller acknowledges any value, so nothing else is sent) and
// publishes BUSY, or publishes the refusal. Returns whether the cover moves.
static bool cover_move(indigo_device *device, indigo_property *property, int target) {
	const char *reason = cover_busy_reason(device);
	if (reason != NULL) {
		cover_refuse(device, property, reason);
		return false;
	}
	if (!primalucelib_cover_set_position(LINK, target)) {
		const char *refusal = primalucelib_last_error(LINK);
		cover_refuse(device, property, *refusal ? refusal : "The cover did not accept the move");
		return false;
	}
	cover_follow_motion(device, target);
	return true;
}

static void cover_motion_finalizer(indigo_device *device) {
	int position;
	char *state;
	PRIVATE_DATA->motion_polls++;
	if (!primalucelib_cover_get_motion(LINK, &position, &state)) {
		if (++PRIVATE_DATA->poll_failures < MAX_POLL_FAILURES) {
			// A single lost readback is retried.
			indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
			return;
		}
		// The cover must not run on unobserved.
		primalucelib_cover_stop(LINK);
		cover_end_motion(device, INDIGO_ALERT_STATE, "The cover position could not be read");
		return;
	}
	PRIVATE_DATA->poll_failures = 0;
	bool stopped = primalucelib_is_motor_stopped(state);
	if (stopped && position != PRIVATE_DATA->target && PRIVATE_DATA->motion_polls <= START_POLLS && !PRIVATE_DATA->abort_requested && !PRIVATE_DATA->external_motion) {
		// The motor may not have started yet.
		indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
		return;
	}
	if (!stopped) {
		if (position != PRIVATE_DATA->last_position) {
			PRIVATE_DATA->last_position = position;
			PRIVATE_DATA->stalled_polls = 0;
			X_COVER_POSITION_ITEM->number.value = position;
			indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
		} else if (++PRIVATE_DATA->stalled_polls >= STALL_POLLS) {
			primalucelib_cover_stop(LINK);
			cover_end_motion(device, INDIGO_ALERT_STATE, "The cover stalled");
			return;
		}
		if (indigo_monotonic_time() - PRIVATE_DATA->motion_started > MOTION_TIMEOUT) {
			primalucelib_cover_stop(LINK);
			cover_end_motion(device, INDIGO_ALERT_STATE, "The cover did not reach its target in time");
			return;
		}
		indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
		return;
	}
	PRIVATE_DATA->last_position = position;
	if (PRIVATE_DATA->external_motion) {
		// Motion started elsewhere ends where the cover stopped.
		X_COVER_POSITION_ITEM->number.target = position;
		cover_end_motion(device, INDIGO_OK_STATE, NULL);
	} else if (PRIVATE_DATA->abort_requested) {
		cover_end_motion(device, INDIGO_ALERT_STATE, NULL);
	} else if (position == PRIVATE_DATA->target) {
		cover_end_motion(device, INDIGO_OK_STATE, NULL);
	} else {
		char message[INDIGO_VALUE_SIZE];
		snprintf(message, sizeof(message), "The cover stopped at %d %% before reaching the target", position);
		cover_end_motion(device, INDIGO_ALERT_STATE, message);
	}
}

static bool alto_open(indigo_device *device, const char **error_message) {
	if (primalucelib_open(LINK, DRIVER_NAME, DEVICE_PORT_ITEM->text.value, error_message)) {
		char *text = primalucelib_get_model_name(LINK);
		if (text != NULL && primalucelib_model_from_name(text) == PRIMALUCELIB_MODEL_ALTO) {
			// Diagnostic output would interleave with the replies on the serial line.
			primalucelib_disable_log_output(LINK);
			// The full state stays in the link for on_connect.
			if (primalucelib_get_state(LINK) && primalucelib_reply_has(LINK, PRIMALUCE_REPLY_COVER_POSITION)) {
				return true;
			}
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No cover position in the controller state");
			*error_message = "The controller reports no cover position";
		} else if (text != NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' is not an ALTO", text);
			*error_message = "The device is not an ALTO";
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No reply to the model request");
		}
		primalucelib_close(LINK);
	}
	return false;
}

static void alto_close(indigo_device *device) {
	if (primalucelib_is_open(LINK)) {
		primalucelib_close(LINK);
	}
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	int position;
	char *state;
	if (!PRIVATE_DATA->moving && !PRIVATE_DATA->external_motion) {
		if (primalucelib_cover_get_motion(LINK, &position, &state)) {
			if (PRIVATE_DATA->poll_alert || position != PRIVATE_DATA->last_position) {
				PRIVATE_DATA->poll_alert = false;
				PRIVATE_DATA->last_position = position;
				X_COVER_POSITION_ITEM->number.value = X_COVER_POSITION_ITEM->number.target = position;
				cover_show_ends(device, position);
				AUX_COVER_PROPERTY->state = X_COVER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
				indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
			}
			if (!primalucelib_is_motor_stopped(state) && !PRIVATE_DATA->calibration_phase) {
				// Motion started elsewhere (the web page, a preset recalled by another client) is followed.
				PRIVATE_DATA->external_motion = true;
				PRIVATE_DATA->target = position;
				PRIVATE_DATA->stalled_polls = PRIVATE_DATA->poll_failures = 0;
				PRIVATE_DATA->motion_polls = START_POLLS;
				PRIVATE_DATA->motion_started = indigo_monotonic_time();
				AUX_COVER_PROPERTY->state = X_COVER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
				indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
				indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
			}
		} else if (!PRIVATE_DATA->poll_alert) {
			// A failed poll keeps the last values; the next good poll restores OK.
			PRIVATE_DATA->poll_alert = true;
			AUX_COVER_PROPERTY->state = X_COVER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
			indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
		}
	}
	if (++PRIVATE_DATA->environment_ticks % ENVIRONMENT_TICKS == 0) {
		double temperature, supply, usb;
		if (primalucelib_cover_get_environment(LINK, &temperature, &supply, &usb)) {
			X_STATE_TEMPERATURE_ITEM->number.value = temperature;
			X_STATE_VIN_12V_ITEM->number.value = supply;
			X_STATE_VIN_USB_ITEM->number.value = usb;
			X_STATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			X_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_STATE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, POLL_PERIOD, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		connection_result = alto_open(device, &error_message);
		if (connection_result) {
			//+ aux.on_connect
			// alto_open() left the full state in the link.
			char *text;
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_MODEL_NAME))) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, text);
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_APPLICATION_FIRMWARE))) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_WEB_FIRMWARE))) {
					strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, " / ");
					strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
				}
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_SERIAL_NUMBER))) {
				INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, text);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_FOCUSER_ERROR)) && *text) {
				indigo_send_message(device, ALERT_PROPERTY, "%s", text);
			}
			PRIVATE_DATA->travel = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX);
			if (PRIVATE_DATA->travel <= 0 || primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_RESTART)) {
				indigo_send_message(device, BUSY_PROPERTY, "%s needs calibration", INFO_DEVICE_MODEL_ITEM->text.value);
			}
			PRIVATE_DATA->last_position = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_COVER_POSITION);
			X_COVER_POSITION_ITEM->number.value = X_COVER_POSITION_ITEM->number.target = PRIVATE_DATA->last_position;
			cover_show_ends(device, PRIVATE_DATA->last_position);
			AUX_COVER_PROPERTY->state = X_COVER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			for (int i = 0; i < PRIMALUCE_COVER_PRESET_COUNT; i++) {
				PRIVATE_DATA->preset_steps[i] = (int)primalucelib_reply_stored_position(LINK, i + 1);
				X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = steps_to_percent(device, PRIVATE_DATA->preset_steps[i]);
				INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], (text = primalucelib_reply_stored_position_name(LINK, i + 1)) ? text : "");
				INDIGO_COPY_VALUE(X_PRESET_NAMES_PROPERTY->items[i].text.value, PRIVATE_DATA->preset_names[i]);
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_LEDS))) {
				if (!strcmp(text, PRIMALUCE_LEDS_ON)) {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_ON_ITEM, true);
				} else if (!strcmp(text, PRIMALUCE_LEDS_LOW)) {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_DIM_ITEM, true);
				} else if (!strcmp(text, PRIMALUCE_LEDS_MIDDLE)) {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_MIDDLE_ITEM, true);
				} else {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_OFF_ITEM, true);
				}
			}
			PRIVATE_DATA->leds_item = selected_item(X_LEDS_PROPERTY);
			X_STATE_TEMPERATURE_ITEM->number.value = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_MOTOR_TEMPERATURE);
			X_STATE_VIN_12V_ITEM->number.value = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_SUPPLY_VOLTAGE);
			X_STATE_VIN_USB_ITEM->number.value = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_USB_VOLTAGE);
			PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = PRIVATE_DATA->abort_requested = PRIVATE_DATA->poll_alert = false;
			PRIVATE_DATA->calibration_phase = 0;
			PRIVATE_DATA->environment_ticks = 0;
			indigo_set_switch(X_CALIBRATION_PROPERTY, X_CALIBRATION_START_ITEM, false);
			X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_COVER_PROPERTY, NULL);
			indigo_define_property(device, X_COVER_POSITION_PROPERTY, NULL);
			indigo_define_property(device, X_COVER_ABORT_PROPERTY, NULL);
			indigo_define_property(device, X_STATE_PROPERTY, NULL);
			indigo_define_property(device, X_PRESETS_PROPERTY, NULL);
			indigo_define_property(device, X_PRESET_NAMES_PROPERTY, NULL);
			indigo_define_property(device, X_PRESET_RECALL_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATION_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATION_MOTION_PROPERTY, NULL);
			indigo_define_property(device, X_LEDS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		// A move still running is stopped before the connection closes.
		if (PRIVATE_DATA->moving || PRIVATE_DATA->external_motion || PRIVATE_DATA->calibration_phase) {
			if (!primalucelib_cover_stop(LINK)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to stop the cover");
			}
		}
		PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = PRIVATE_DATA->abort_requested = false;
		PRIVATE_DATA->calibration_phase = 0;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		//- aux.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			AUX_COVER_PROPERTY,
			X_COVER_POSITION_PROPERTY,
			X_COVER_ABORT_PROPERTY,
			X_STATE_PROPERTY,
			X_PRESETS_PROPERTY,
			X_PRESET_NAMES_PROPERTY,
			X_PRESET_RECALL_PROPERTY,
			X_CALIBRATION_PROPERTY,
			X_CALIBRATION_MOTION_PROPERTY,
			X_LEDS_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, AUX_COVER_PROPERTY, NULL);
		indigo_delete_property(device, X_COVER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, X_COVER_ABORT_PROPERTY, NULL);
		indigo_delete_property(device, X_STATE_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESETS_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESET_NAMES_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESET_RECALL_PROPERTY, NULL);
		indigo_delete_property(device, X_CALIBRATION_PROPERTY, NULL);
		indigo_delete_property(device, X_CALIBRATION_MOTION_PROPERTY, NULL);
		indigo_delete_property(device, X_LEDS_PROPERTY, NULL);
		alto_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_timer_callback);
	}
}

static void aux_cover_handler(indigo_device *device) {
	//+ aux.AUX_COVER.on_change
	if (cover_move(device, AUX_COVER_PROPERTY, AUX_COVER_OPEN_ITEM->sw.value ? PRIMALUCE_COVER_OPEN : PRIMALUCE_COVER_CLOSED)) {
		indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
	}
	//- aux.AUX_COVER.on_change
}

static void aux_x_cover_position_handler(indigo_device *device) {
	//+ aux.X_COVER_POSITION.on_change
	if (cover_move(device, X_COVER_POSITION_PROPERTY, (int)X_COVER_POSITION_ITEM->number.target)) {
		indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
	}
	//- aux.X_COVER_POSITION.on_change
}

static void aux_x_cover_abort_handler(indigo_device *device) {
	X_COVER_ABORT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_COVER_ABORT.on_change
	X_COVER_ABORT_ITEM->sw.value = false;
	if (PRIVATE_DATA->moving || PRIVATE_DATA->external_motion) {
		// The finalizer ends the move ALERT once the cover has stopped; the stop decelerates, so the
		// swinging arm is not jerked.
		if (primalucelib_cover_stop(LINK)) {
			PRIVATE_DATA->abort_requested = true;
			PRIVATE_DATA->external_motion = false;
		} else {
			X_COVER_ABORT_PROPERTY->state = INDIGO_ALERT_STATE;
			update_with_refusal(device, X_COVER_ABORT_PROPERTY);
			return;
		}
	}
	//- aux.X_COVER_ABORT.on_change
	indigo_update_property(device, X_COVER_ABORT_PROPERTY, NULL);
}

static void aux_x_presets_handler(indigo_device *device) {
	X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_PRESETS.on_change
	// Only changed presets are written, converted to steps; a preset without a name refuses a position.
	char reason_text[INDIGO_VALUE_SIZE];
	const char *reason = NULL;
	for (int i = 0; i < PRIMALUCE_COVER_PRESET_COUNT; i++) {
		int percent = (int)X_PRESETS_PROPERTY->items[i].number.target;
		if (percent == steps_to_percent(device, PRIVATE_DATA->preset_steps[i])) {
			continue;
		}
		int steps = percent_to_steps(device, percent);
		if (PRIVATE_DATA->travel <= 0) {
			X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
			reason = "The cover needs calibration";
		} else if (primalucelib_cover_set_preset_position(LINK, i + 1, steps)) {
			PRIVATE_DATA->preset_steps[i] = steps;
		} else {
			X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
			if (reason == NULL && *primalucelib_last_error(LINK) && PRIVATE_DATA->preset_names[i][0] == 0) {
				snprintf(reason_text, sizeof(reason_text), "Preset #%d needs a name before its position can be set", i + 1);
				reason = reason_text;
			} else if (reason == NULL && *primalucelib_last_error(LINK)) {
				INDIGO_COPY_VALUE(reason_text, primalucelib_last_error(LINK));
				reason = reason_text;
			}
		}
		X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = steps_to_percent(device, PRIVATE_DATA->preset_steps[i]);
	}
	if (reason != NULL) {
		indigo_update_property(device, X_PRESETS_PROPERTY, "%s", reason);
		return;
	}
	//- aux.X_PRESETS.on_change
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
}

static void aux_x_preset_names_handler(indigo_device *device) {
	X_PRESET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_PRESET_NAMES.on_change
	// An empty name deletes the preset. The first refusal is reported.
	char reason_text[INDIGO_VALUE_SIZE];
	const char *reason = NULL;
	for (int i = 0; i < PRIMALUCE_COVER_PRESET_COUNT; i++) {
		char *name = X_PRESET_NAMES_PROPERTY->items[i].text.value;
		if (!strcmp(name, PRIVATE_DATA->preset_names[i])) {
			continue;
		}
		primalucelib_clear_last_error(LINK);
		if (primalucelib_cover_set_preset_name(LINK, i + 1, name)) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], name);
		} else {
			X_PRESET_NAMES_PROPERTY->state = INDIGO_ALERT_STATE;
			if (reason == NULL && *primalucelib_last_error(LINK)) {
				INDIGO_COPY_VALUE(reason_text, primalucelib_last_error(LINK));
				reason = reason_text;
			} else if (reason == NULL && (strchr(name, '"') != NULL || strchr(name, '\\') != NULL)) {
				// The protocol has no escaping, so such a name is not sent.
				reason = "A preset name cannot contain \" or \\";
			}
			INDIGO_COPY_VALUE(name, PRIVATE_DATA->preset_names[i]);
		}
	}
	if (reason != NULL) {
		indigo_update_property(device, X_PRESET_NAMES_PROPERTY, "%s", reason);
		return;
	}
	//- aux.X_PRESET_NAMES.on_change
	indigo_update_property(device, X_PRESET_NAMES_PROPERTY, NULL);
}

static void aux_x_preset_recall_handler(indigo_device *device) {
	//+ aux.X_PRESET_RECALL.on_change
	int index = -1;
	for (int i = 0; i < PRIMALUCE_COVER_PRESET_COUNT; i++) {
		if (X_PRESET_RECALL_PROPERTY->items[i].sw.value) {
			index = i;
		}
		X_PRESET_RECALL_PROPERTY->items[i].sw.value = false;
	}
	if (index >= 0) {
		const char *reason = cover_busy_reason(device);
		if (reason != NULL) {
			X_PRESET_RECALL_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_PRESET_RECALL_PROPERTY, "%s", reason);
			return;
		}
		// The cover moves to the preset; the move is followed like any other.
		if (!primalucelib_cover_recall_preset(LINK, index + 1)) {
			X_PRESET_RECALL_PROPERTY->state = INDIGO_ALERT_STATE;
			update_with_refusal(device, X_PRESET_RECALL_PROPERTY);
			return;
		}
		cover_follow_motion(device, steps_to_percent(device, PRIVATE_DATA->preset_steps[index]));
		indigo_execute_handler_in(device, MOTION_POLL, cover_motion_finalizer);
	}
	X_PRESET_RECALL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_PRESET_RECALL_PROPERTY, NULL);
	//- aux.X_PRESET_RECALL.on_change
}

static void aux_x_calibration_handler(indigo_device *device) {
	X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_CALIBRATION.on_change
	// The INDI sequence (not verified on hardware): Init, drive the cover to closed and store, drive it to
	// fully open and store, which ends the calibration. Between the steps the property is OK, so that the
	// next STORE or CANCEL is accepted; the message says what comes next.
	bool start = X_CALIBRATION_START_ITEM->sw.value, store = X_CALIBRATION_STORE_ITEM->sw.value, cancel = X_CALIBRATION_CANCEL_ITEM->sw.value;
	X_CALIBRATION_START_ITEM->sw.value = X_CALIBRATION_STORE_ITEM->sw.value = X_CALIBRATION_CANCEL_ITEM->sw.value = false;
	if (start) {
		if (PRIVATE_DATA->moving || PRIVATE_DATA->external_motion) {
			X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CALIBRATION_PROPERTY, "The cover is moving");
		} else if (primalucelib_cover_calibration_step(LINK, PRIMALUCE_COVER_CALIBRATION_INIT)) {
			PRIVATE_DATA->calibration_phase = 1;
			X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_CALIBRATION_PROPERTY, "Close the cover fully with X_CALIBRATION_MOTION, stop it and press STORE");
		} else {
			X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
			update_with_refusal(device, X_CALIBRATION_PROPERTY);
		}
	} else if (store && PRIVATE_DATA->calibration_phase == 1) {
		if (primalucelib_cover_stop(LINK) && primalucelib_cover_calibration_step(LINK, PRIMALUCE_COVER_CALIBRATION_STORE_CLOSED)) {
			PRIVATE_DATA->calibration_phase = 2;
			X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_CALIBRATION_PROPERTY, "Closed position stored; open the cover fully, stop it and press STORE");
		} else {
			X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
			update_with_refusal(device, X_CALIBRATION_PROPERTY);
		}
	} else if (store && PRIVATE_DATA->calibration_phase == 2) {
		if (primalucelib_cover_stop(LINK) && primalucelib_cover_calibration_step(LINK, PRIMALUCE_COVER_CALIBRATION_STORE_OPEN)) {
			PRIVATE_DATA->calibration_phase = 0;
			if (primalucelib_get_state(LINK)) {
				PRIVATE_DATA->travel = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX);
				PRIVATE_DATA->last_position = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_COVER_POSITION);
			}
			X_COVER_POSITION_ITEM->number.value = X_COVER_POSITION_ITEM->number.target = PRIVATE_DATA->last_position;
			cover_show_ends(device, PRIVATE_DATA->last_position);
			indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
			indigo_update_property(device, X_COVER_POSITION_PROPERTY, NULL);
			X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_CALIBRATION_PROPERTY, "Calibration complete, travel %d steps", PRIVATE_DATA->travel);
		} else {
			X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
			update_with_refusal(device, X_CALIBRATION_PROPERTY);
		}
	} else if (store) {
		X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATION_PROPERTY, "Start a calibration first");
	} else if (cancel && PRIVATE_DATA->calibration_phase) {
		primalucelib_cover_stop(LINK);
		PRIVATE_DATA->calibration_phase = 0;
		X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATION_PROPERTY, "Calibration cancelled");
	} else {
		X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CALIBRATION_PROPERTY, NULL);
	}
	return;
	//- aux.X_CALIBRATION.on_change
	indigo_update_property(device, X_CALIBRATION_PROPERTY, NULL);
}

static void aux_x_calibration_motion_handler(indigo_device *device) {
	X_CALIBRATION_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_CALIBRATION_MOTION.on_change
	const char *step = NULL;
	bool stop = X_CALIBRATION_MOTION_STOP_ITEM->sw.value;
	if (X_CALIBRATION_MOTION_CLOSE_FAST_ITEM->sw.value) {
		step = PRIMALUCE_COVER_CALIBRATION_CLOSE_FAST;
	} else if (X_CALIBRATION_MOTION_CLOSE_SLOW_ITEM->sw.value) {
		step = PRIMALUCE_COVER_CALIBRATION_CLOSE_SLOW;
	} else if (X_CALIBRATION_MOTION_OPEN_FAST_ITEM->sw.value) {
		step = PRIMALUCE_COVER_CALIBRATION_OPEN_FAST;
	} else if (X_CALIBRATION_MOTION_OPEN_SLOW_ITEM->sw.value) {
		step = PRIMALUCE_COVER_CALIBRATION_OPEN_SLOW;
	}
	for (int i = 0; i < X_CALIBRATION_MOTION_PROPERTY->count; i++) {
		X_CALIBRATION_MOTION_PROPERTY->items[i].sw.value = false;
	}
	if (!PRIVATE_DATA->calibration_phase) {
		// Outside a calibration these steps are not defined; the cover moves with AUX_COVER.
		X_CALIBRATION_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATION_MOTION_PROPERTY, "Start a calibration first");
		return;
	}
	if ((stop && !primalucelib_cover_stop(LINK)) || (step != NULL && !primalucelib_cover_calibration_step(LINK, step))) {
		X_CALIBRATION_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		update_with_refusal(device, X_CALIBRATION_MOTION_PROPERTY);
		return;
	}
	//- aux.X_CALIBRATION_MOTION.on_change
	indigo_update_property(device, X_CALIBRATION_MOTION_PROPERTY, NULL);
}

static void aux_x_leds_handler(indigo_device *device) {
	X_LEDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_LEDS.on_change
	bool result = false;
	if (X_LEDS_OFF_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_OFF);
	} else if (X_LEDS_DIM_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_LOW);
	} else if (X_LEDS_MIDDLE_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_MIDDLE);
	} else if (X_LEDS_ON_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_ON);
	}
	if (result) {
		PRIVATE_DATA->leds_item = selected_item(X_LEDS_PROPERTY);
	} else {
		if (PRIVATE_DATA->leds_item != NULL) {
			indigo_set_switch(X_LEDS_PROPERTY, PRIVATE_DATA->leds_item, true);
		}
		X_LEDS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.X_LEDS.on_change
	indigo_update_property(device, X_LEDS_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_DUSTCAP) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_COVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_COVER_PROPERTY_NAME, AUX_MAIN_GROUP, "Cover (open/close)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_COVER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_COVER_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(AUX_COVER_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Close", false);
		X_COVER_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, X_COVER_POSITION_PROPERTY_NAME, AUX_MAIN_GROUP, "Cover position", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_COVER_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_COVER_POSITION_ITEM, X_COVER_POSITION_ITEM_NAME, "Position (%, 0 = closed)", 0, 100, 1, 0);
		strcpy(X_COVER_POSITION_ITEM->number.format, "%g");
		X_COVER_ABORT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_COVER_ABORT_PROPERTY_NAME, AUX_MAIN_GROUP, "Abort cover motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_COVER_ABORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_COVER_ABORT_ITEM, X_COVER_ABORT_ITEM_NAME, "Abort", false);
		X_STATE_PROPERTY = indigo_init_number_property(NULL, device->name, X_STATE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "State", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_STATE_TEMPERATURE_ITEM, X_STATE_TEMPERATURE_ITEM_NAME, "Motor temperature (°C)", -50, 100, 0, 0);
		indigo_init_number_item(X_STATE_VIN_12V_ITEM, X_STATE_VIN_12V_ITEM_NAME, "12V power (V)", 0, 50, 0, 0);
		indigo_init_number_item(X_STATE_VIN_USB_ITEM, X_STATE_VIN_USB_ITEM_NAME, "USB power (V)", 0, 10, 0, 0);
		X_PRESETS_PROPERTY = indigo_init_number_property(NULL, device->name, X_PRESETS_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Preset positions", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (X_PRESETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_PRESETS_1_ITEM, X_PRESETS_1_ITEM_NAME, "Preset #1 (%)", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_2_ITEM, X_PRESETS_2_ITEM_NAME, "Preset #2 (%)", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_3_ITEM, X_PRESETS_3_ITEM_NAME, "Preset #3 (%)", 0, 100, 1, 0);
		X_PRESET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, X_PRESET_NAMES_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Preset names", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (X_PRESET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_PRESET_NAMES_1_ITEM, X_PRESET_NAMES_1_ITEM_NAME, "Preset #1", "");
		indigo_init_text_item(X_PRESET_NAMES_2_ITEM, X_PRESET_NAMES_2_ITEM_NAME, "Preset #2", "");
		indigo_init_text_item(X_PRESET_NAMES_3_ITEM, X_PRESET_NAMES_3_ITEM_NAME, "Preset #3", "");
		X_PRESET_RECALL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PRESET_RECALL_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Go to preset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (X_PRESET_RECALL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_PRESET_RECALL_1_ITEM, X_PRESET_RECALL_1_ITEM_NAME, "Preset #1", false);
		indigo_init_switch_item(X_PRESET_RECALL_2_ITEM, X_PRESET_RECALL_2_ITEM_NAME, "Preset #2", false);
		indigo_init_switch_item(X_PRESET_RECALL_3_ITEM, X_PRESET_RECALL_3_ITEM_NAME, "Preset #3", false);
		X_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATION_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (X_CALIBRATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATION_START_ITEM, X_CALIBRATION_START_ITEM_NAME, "Start", false);
		indigo_init_switch_item(X_CALIBRATION_STORE_ITEM, X_CALIBRATION_STORE_ITEM_NAME, "Store position", false);
		indigo_init_switch_item(X_CALIBRATION_CANCEL_ITEM, X_CALIBRATION_CANCEL_ITEM_NAME, "Cancel", false);
		X_CALIBRATION_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATION_MOTION_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Calibration motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 5);
		if (X_CALIBRATION_MOTION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATION_MOTION_CLOSE_FAST_ITEM, X_CALIBRATION_MOTION_CLOSE_FAST_ITEM_NAME, "Close fast", false);
		indigo_init_switch_item(X_CALIBRATION_MOTION_CLOSE_SLOW_ITEM, X_CALIBRATION_MOTION_CLOSE_SLOW_ITEM_NAME, "Close slow", false);
		indigo_init_switch_item(X_CALIBRATION_MOTION_OPEN_FAST_ITEM, X_CALIBRATION_MOTION_OPEN_FAST_ITEM_NAME, "Open fast", false);
		indigo_init_switch_item(X_CALIBRATION_MOTION_OPEN_SLOW_ITEM, X_CALIBRATION_MOTION_OPEN_SLOW_ITEM_NAME, "Open slow", false);
		indigo_init_switch_item(X_CALIBRATION_MOTION_STOP_ITEM, X_CALIBRATION_MOTION_STOP_ITEM_NAME, "Stop", false);
		X_LEDS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_LEDS_PROPERTY_NAME, AUX_ADVANCED_GROUP, "LEDs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_LEDS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_LEDS_OFF_ITEM, X_LEDS_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(X_LEDS_DIM_ITEM, X_LEDS_DIM_ITEM_NAME, "Dim", false);
		indigo_init_switch_item(X_LEDS_MIDDLE_ITEM, X_LEDS_MIDDLE_ITEM_NAME, "Middle", false);
		indigo_init_switch_item(X_LEDS_ON_ITEM, X_LEDS_ON_ITEM_NAME, "On", true);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_COVER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_COVER_POSITION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_COVER_ABORT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESET_NAMES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESET_RECALL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATION_MOTION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_LEDS_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_COVER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_COVER_PROPERTY, aux_cover_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_COVER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_COVER_POSITION_PROPERTY, aux_x_cover_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_COVER_ABORT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_COVER_ABORT_PROPERTY, aux_x_cover_abort_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESETS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESETS_PROPERTY, aux_x_presets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESET_NAMES_PROPERTY, aux_x_preset_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESET_RECALL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESET_RECALL_PROPERTY, aux_x_preset_recall_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATION_PROPERTY, aux_x_calibration_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATION_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATION_MOTION_PROPERTY, aux_x_calibration_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_LEDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_LEDS_PROPERTY, aux_x_leds_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_COVER_PROPERTY);
	indigo_release_property(X_COVER_POSITION_PROPERTY);
	indigo_release_property(X_COVER_ABORT_PROPERTY);
	indigo_release_property(X_STATE_PROPERTY);
	indigo_release_property(X_PRESETS_PROPERTY);
	indigo_release_property(X_PRESET_NAMES_PROPERTY);
	indigo_release_property(X_PRESET_RECALL_PROPERTY);
	indigo_release_property(X_CALIBRATION_PROPERTY);
	indigo_release_property(X_CALIBRATION_MOTION_PROPERTY);
	indigo_release_property(X_LEDS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_alto(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static alto_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "CP2102N");
			patterns[0].vendor_id = 0x10c4;
			patterns[0].product_id = 0xea60;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = (alto_private_data *)indigo_safe_malloc(sizeof(alto_private_data));
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
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
