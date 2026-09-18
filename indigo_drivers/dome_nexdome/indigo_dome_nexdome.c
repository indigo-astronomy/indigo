// Copyright (c) 2019-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_dome_nexdome.driver

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
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_nexdome.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_dome_nexdome"
#define DRIVER_LABEL         "NexDome"
#define DOME_DEVICE_NAME     "NexDome"
#define PRIVATE_DATA         ((nexdome_private_data *)device->private_data)

//+ define

#define NEXDOME_NETWORK_PORT 8080
#define NEXDOME_RESET_DELAY  1
#define NEXDOME_FLUSH_DELAY  100000
#define NEXDOME_WRITE_DELAY  100
#define NEXDOME_FIRST_BYTE_TIMEOUT 3.1
#define NEXDOME_NEXT_BYTE_TIMEOUT 0.1
#define NEXDOME_FIRST_POLL_DELAY 0.5
#define NEXDOME_POLL_DELAY   1
#define NEXDOME_WIRELESS_RESET_DELAY 2
#define NEXDOME_SHUTTER_TIMEOUT 30
#define NEXDOME_VOLT_THRESHOLD 7.5

//- define

#pragma mark - Property definitions

#define X_REVERSED_PROPERTY            (PRIVATE_DATA->x_reversed_property)
#define X_REVERSED_YES_ITEM            (X_REVERSED_PROPERTY->items + 0)
#define X_REVERSED_NO_ITEM             (X_REVERSED_PROPERTY->items + 1)

#define X_REVERSED_PROPERTY_NAME       "X_REVERSED"
#define X_REVERSED_YES_ITEM_NAME       "YES"
#define X_REVERSED_NO_ITEM_NAME        "NO"

#define X_RESET_SHUTTER_COMM_PROPERTY      (PRIVATE_DATA->x_reset_shutter_comm_property)
#define X_RESET_SHUTTER_COMM_ITEM          (X_RESET_SHUTTER_COMM_PROPERTY->items + 0)

#define X_RESET_SHUTTER_COMM_PROPERTY_NAME "X_RESET_SHUTTER_COMM"
#define X_RESET_SHUTTER_COMM_ITEM_NAME     "RESET"

#define X_FIND_HOME_PROPERTY           (PRIVATE_DATA->x_find_home_property)
#define X_FIND_HOME_ITEM               (X_FIND_HOME_PROPERTY->items + 0)

#define X_FIND_HOME_PROPERTY_NAME      "X_FIND_HOME"
#define X_FIND_HOME_ITEM_NAME          "FIND_HOME"

#define X_CALIBRATE_PROPERTY           (PRIVATE_DATA->x_calibrate_property)
#define X_CALIBRATE_ITEM               (X_CALIBRATE_PROPERTY->items + 0)

#define X_CALIBRATE_PROPERTY_NAME      "X_CALIBRATE"
#define X_CALIBRATE_ITEM_NAME          "CALIBRATE"

#define X_POWER_PROPERTY               (PRIVATE_DATA->x_power_property)
#define X_POWER_ROTATOR_ITEM           (X_POWER_PROPERTY->items + 0)
#define X_POWER_SHUTTER_ITEM           (X_POWER_PROPERTY->items + 1)

#define X_POWER_PROPERTY_NAME          "X_POWER"
#define X_POWER_ROTATOR_ITEM_NAME      "ROTATOR_VOLTAGE"
#define X_POWER_SHUTTER_ITEM_NAME      "SHUTTER_VOLTAGE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_reversed_property;
	indigo_property *x_reset_shutter_comm_property;
	indigo_property *x_find_home_property;
	indigo_property *x_calibrate_property;
	indigo_property *x_power_property;
	//+ data
	char command[32];
	char response[INDIGO_VALUE_SIZE];
	double current_position, target_position, park_azimuth;
	int dome_state, shutter_state, prev_shutter_state, shutter_target;
	double shutter_started;
	bool reversed, need_update, low_voltage;
	bool rotation_active, rotation_observed, park_requested, home_active, calibration_active, abort_requested, shutter_active, shutter_observed;
	//- data
} nexdome_private_data;

#pragma mark - Low level code

//+ code

typedef enum {
	DOME_STOPPED = 0,
	DOME_GOTO = 1,
	DOME_FINDING_HOME = 2,
	DOME_CALIBRATING = 3
} nexdome_dome_state_t;

typedef enum {
	SHUTTER_STATE_NOT_CONNECTED = 0,
	SHUTTER_STATE_OPEN = 1,
	SHUTTER_STATE_OPENING = 2,
	SHUTTER_STATE_CLOSED = 3,
	SHUTTER_STATE_CLOSING = 4,
	SHUTTER_STATE_UNKNOWN = 5
} nexdome_shutter_state_t;

// sends a command and reads one reply line starting with the expected letter; returns the reply after the letter or NULL
static char *nexdome_vcommand(indigo_device *device, char reply, const char *format, va_list args) {
	char *command = PRIVATE_DATA->command, *response = PRIVATE_DATA->response;
	vsnprintf(command, sizeof(PRIVATE_DATA->command), format, args);
	// the original driver waited for 100 ms of silence before every command
	indigo_usleep(NEXDOME_FLUSH_DELAY);
	long length = -1;
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) > 0) {
		indigo_usleep(NEXDOME_WRITE_DELAY);
		// an empty line left from a previous reply is skipped
		for (int i = 0; i < 2; i++) {
			length = indigo_uni_read_section2(PRIVATE_DATA->handle, response, sizeof(PRIVATE_DATA->response) - 1, "\r\n", "", INDIGO_DELAY(NEXDOME_FIRST_BYTE_TIMEOUT), INDIGO_DELAY(NEXDOME_NEXT_BYTE_TIMEOUT));
			if (length != 1 || (*response != '\r' && *response != '\n')) {
				break;
			}
		}
	}
	if (length <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response to %s", command);
		return NULL;
	}
	if (response[length - 1] != '\r' && response[length - 1] != '\n') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unterminated reply to %s: '%s'", command, response);
		return NULL;
	}
	response[length - 1] = 0;
	if (*response != reply) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid reply to %s: '%s'", command, response);
		return NULL;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s", command, response);
	return response + 1;
}

static bool nexdome_command(indigo_device *device, char reply, const char *format, ...) {
	va_list args;
	va_start(args, format);
	char *result = nexdome_vcommand(device, reply, format, args);
	va_end(args);
	return result != NULL;
}

// reads count numbers of a reply; exact rejects further fields
static bool nexdome_numbers(indigo_device *device, char reply, double *values, int count, bool exact, const char *format, ...) {
	va_list args;
	va_start(args, format);
	char *result = nexdome_vcommand(device, reply, format, args);
	va_end(args);
	if (result == NULL) {
		return false;
	}
	char *end = result;
	for (int i = 0; i < count; i++) {
		char *start = end;
		values[i] = strtod(start, &end);
		if (end == start || !isfinite(values[i])) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid reply to %s: '%s'", PRIVATE_DATA->command, PRIVATE_DATA->response);
			return false;
		}
	}
	while (*end == ' ') {
		end++;
	}
	if (exact && *end) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid reply to %s: '%s'", PRIVATE_DATA->command, PRIVATE_DATA->response);
		return false;
	}
	return true;
}

static bool nexdome_get_azimuth(indigo_device *device, double *azimuth, char reply, const char *command) {
	double value;
	if (!nexdome_numbers(device, reply, &value, 1, true, command) || value < 0 || value > 360) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
		return false;
	}
	*azimuth = value;
	return true;
}

static bool nexdome_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "nexdome")) {
		PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			// the controller resets when the serial port opens
			indigo_sleep(NEXDOME_RESET_DELAY);
		}
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, NEXDOME_NETWORK_PORT, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", name);
		indigo_global_unlock(device);
		return false;
	}
	char model[32] = "", firmware[32] = "";
	if (nexdome_command(device, 'V', "v") && sscanf(PRIVATE_DATA->response, "V%31s V %31s", model, firmware) == 2) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, model);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
		indigo_update_property(device, INFO_PROPERTY, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with firmware V.%s connected.", model, firmware);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: NexDome did not respond. Are you using the correct firmware?");
	indigo_send_message(device, CONNECTION_PROPERTY, "NexDome did not respond. Are you using the correct firmware?");
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	return false;
}

static void nexdome_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

//- code

//+ dome.code

static void dome_horizontal_coordinates_handler(indigo_device *device);
static void dome_steps_handler(indigo_device *device);
static void dome_park_handler(indigo_device *device);
static void dome_shutter_handler(indigo_device *device);
static void dome_x_find_home_handler(indigo_device *device);
static void dome_x_calibrate_handler(indigo_device *device);

// a closed shutter selects CLOSED, any other reported motion or open state OPENED
static void nexdome_update_shutter_switches(indigo_device *device) {
	if (PRIVATE_DATA->shutter_state == SHUTTER_STATE_CLOSED) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
	} else if (PRIVATE_DATA->shutter_state == SHUTTER_STATE_OPEN || PRIVATE_DATA->shutter_state == SHUTTER_STATE_OPENING || PRIVATE_DATA->shutter_state == SHUTTER_STATE_CLOSING) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
	}
}

static void reset_shutter_comm_finalizer(indigo_device *device) {
	X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(X_RESET_SHUTTER_COMM_PROPERTY, X_RESET_SHUTTER_COMM_ITEM, false);
	indigo_update_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
}

static void nexdome_update_power(indigo_device *device) {
	double volts[2];
	if (!nexdome_numbers(device, 'K', volts, 2, false, "k")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_voltages(): returned error");
		return;
	}
	// the controller reports hundredths of volt
	float rotator = (float)volts[0] / 100, shutter = (float)volts[1] / 100;
	/* Threshold taken from INDI driver */
	if (rotator < NEXDOME_VOLT_THRESHOLD || shutter < NEXDOME_VOLT_THRESHOLD) {
		if (!PRIVATE_DATA->low_voltage) {
			indigo_send_message(device, ALERT_PROPERTY, "Dome power is low! (U_rotator = %.2fV, U_shutter = %.2fV)", rotator, shutter);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Dome power is low! (U_rotator = %.2fV, U_shutter = %.2fV)", rotator, shutter);
		}
		PRIVATE_DATA->low_voltage = true;
	} else {
		if (PRIVATE_DATA->low_voltage) {
			indigo_send_message(device, IDLE_PROPERTY, "Dome power is normal! (U_rotator = %.2fV, U_shutter = %.2fV)", rotator, shutter);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Dome power is normal! (U_rotator = %.2fV, U_shutter = %.2fV)", rotator, shutter);
		}
		PRIVATE_DATA->low_voltage = false;
	}
	if (fabs((rotator - X_POWER_ROTATOR_ITEM->number.value) * 100) >= 1 || fabs((shutter - X_POWER_SHUTTER_ITEM->number.value) * 100) >= 1) {
		X_POWER_ROTATOR_ITEM->number.value = rotator;
		X_POWER_SHUTTER_ITEM->number.value = shutter;
		indigo_update_property(device, X_POWER_PROPERTY, NULL);
	}
}

static void nexdome_update_rotation(indigo_device *device) {
	double values[1];
	if (nexdome_numbers(device, 'M', values, 1, true, "m") && values[0] >= DOME_STOPPED && values[0] <= DOME_CALIBRATING && values[0] == floor(values[0])) {
		PRIVATE_DATA->dome_state = (int)values[0];
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_dome_state(): returned error");
	}
	if (!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position, 'Q', "q")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
	}
	// a property is BUSY without an active operation while its request waits in the queue
	bool queued = (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) && !PRIVATE_DATA->rotation_active && !PRIVATE_DATA->rotation_observed;
	if (queued) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Rotation request queued");
	} else {
		if (PRIVATE_DATA->rotation_active || PRIVATE_DATA->rotation_observed || PRIVATE_DATA->home_active || PRIVATE_DATA->calibration_active) {
			PRIVATE_DATA->need_update = true;
		}
		if (PRIVATE_DATA->dome_state != DOME_STOPPED) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->rotation_observed = true;
			PRIVATE_DATA->need_update = true;
		} else if (PRIVATE_DATA->need_update) {
			bool operation = PRIVATE_DATA->home_active || PRIVATE_DATA->calibration_active;
			if (!operation && !PRIVATE_DATA->abort_requested && (indigo_azimuth_distance(PRIVATE_DATA->target_position, PRIVATE_DATA->current_position) * 10) >= 1) {
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
				// a park stopped away from the park position has failed
				if (PRIVATE_DATA->park_requested) {
					PRIVATE_DATA->park_requested = false;
					DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
				}
			} else {
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			}
			if (PRIVATE_DATA->home_active) {
				X_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
				indigo_update_property(device, X_FIND_HOME_PROPERTY, "Home Found.");
			}
			if (PRIVATE_DATA->calibration_active) {
				X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(X_CALIBRATE_PROPERTY, X_CALIBRATE_ITEM, false);
				indigo_update_property(device, X_CALIBRATE_PROPERTY, "Callibration complete.");
			}
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_observed = PRIVATE_DATA->home_active = PRIVATE_DATA->calibration_active = false;
			PRIVATE_DATA->need_update = false;
		}
	}
	if (PRIVATE_DATA->park_requested && (indigo_azimuth_distance(PRIVATE_DATA->park_azimuth, PRIVATE_DATA->current_position) * 10) <= 1) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_requested = false;
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	}
	PRIVATE_DATA->abort_requested = false;
}

static void nexdome_update_shutter(indigo_device *device) {
	double values[2];
	if (!nexdome_numbers(device, 'U', values, 2, true, "u") || values[0] < SHUTTER_STATE_NOT_CONNECTED || values[0] > SHUTTER_STATE_UNKNOWN || values[0] != floor(values[0])) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_shutter_state(): returned error");
		return;
	}
	int state = PRIVATE_DATA->shutter_state = (int)values[0];
	bool queued = DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->shutter_active && !PRIVATE_DATA->shutter_observed;
	if (queued || (state == PRIVATE_DATA->prev_shutter_state && DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE)) {
		return;
	}
	PRIVATE_DATA->prev_shutter_state = state;
	// the controller reports the new state only after the shutter answered over the wireless link
	if (PRIVATE_DATA->shutter_active && ((PRIVATE_DATA->shutter_target == SHUTTER_STATE_OPEN && state == SHUTTER_STATE_CLOSED) || (PRIVATE_DATA->shutter_target == SHUTTER_STATE_CLOSED && state == SHUTTER_STATE_OPEN))) {
		if (indigo_monotonic_time() - PRIVATE_DATA->shutter_started < NEXDOME_SHUTTER_TIMEOUT) {
			return;
		}
		PRIVATE_DATA->shutter_active = false;
		nexdome_update_shutter_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter did not respond");
		return;
	}
	switch (state) {
		case SHUTTER_STATE_NOT_CONNECTED:
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		case SHUTTER_STATE_OPEN:
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			break;
		case SHUTTER_STATE_CLOSED:
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			break;
		case SHUTTER_STATE_OPENING:
		case SHUTTER_STATE_CLOSING:
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			break;
		default:
			DOME_SHUTTER_PROPERTY->state = INDIGO_IDLE_STATE;
			break;
	}
	PRIVATE_DATA->shutter_observed = DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE;
	if (!PRIVATE_DATA->shutter_observed) {
		PRIVATE_DATA->shutter_active = false;
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_status_poll(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	/* Check dome power */
	nexdome_update_power(device);
	/* Handle dome rotation */
	nexdome_update_rotation(device);
	/* Handle dome shutter */
	nexdome_update_shutter(device);
	indigo_execute_handler_in(device, NEXDOME_POLL_DELAY, dome_status_poll);
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = nexdome_open(device);
		if (connection_result) {
			//+ dome.on_connect
			double value;
			if (nexdome_numbers(device, 'Y', &value, 1, true, "y") && (value == 0 || value == 1)) {
				PRIVATE_DATA->reversed = value == 1;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_reversed_flag(): returned error");
				PRIVATE_DATA->reversed = false;
			}
			indigo_set_switch(X_REVERSED_PROPERTY, PRIVATE_DATA->reversed ? X_REVERSED_YES_ITEM : X_REVERSED_NO_ITEM, true);
			// operations interrupted by a disconnection are not resumed
			X_FIND_HOME_PROPERTY->state = X_CALIBRATE_PROPERTY->state = X_RESET_SHUTTER_COMM_PROPERTY->state = X_REVERSED_PROPERTY->state = INDIGO_OK_STATE;
			X_FIND_HOME_ITEM->sw.value = X_CALIBRATE_ITEM->sw.value = X_RESET_SHUTTER_COMM_ITEM->sw.value = false;
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_observed = PRIVATE_DATA->home_active = PRIVATE_DATA->calibration_active = PRIVATE_DATA->abort_requested = PRIVATE_DATA->shutter_active = PRIVATE_DATA->shutter_observed = false;
			PRIVATE_DATA->need_update = true;
			PRIVATE_DATA->low_voltage = false;
			PRIVATE_DATA->dome_state = DOME_STOPPED;
			PRIVATE_DATA->shutter_state = -1;
			PRIVATE_DATA->prev_shutter_state = SHUTTER_STATE_UNKNOWN;
			if (!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position, 'Q', "q")) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
			}
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			if (!nexdome_get_azimuth(device, &PRIVATE_DATA->park_azimuth, 'N', "n")) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_park_azimuth(): returned error");
			}
			if ((indigo_azimuth_distance(PRIVATE_DATA->park_azimuth, PRIVATE_DATA->current_position) * 100) <= 1) {
				indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
			} else {
				indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			}
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
			indigo_execute_handler_in(device, NEXDOME_FIRST_POLL_DELAY, dome_status_poll);
			//- dome.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_REVERSED_PROPERTY, NULL);
			indigo_define_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
			indigo_define_property(device, X_FIND_HOME_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);
			indigo_define_property(device, X_POWER_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_REVERSED_PROPERTY, NULL);
		indigo_delete_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
		indigo_delete_property(device, X_FIND_HOME_PROPERTY, NULL);
		indigo_delete_property(device, X_CALIBRATE_PROPERTY, NULL);
		indigo_delete_property(device, X_POWER_PROPERTY, NULL);
		nexdome_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_horizontal_coordinates_handler(indigo_device *device) {
	//+ dome.DOME_HORIZONTAL_COORDINATES.on_change
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device commands run
	PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		if (!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position, 'Q', "q")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
		}
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked");
		return;
	}
	if (DOME_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if (!nexdome_command(device, 'S', "s %.2f", PRIVATE_DATA->target_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_sync_azimuth(): returned error");
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			return;
		}
	} else if (!nexdome_command(device, 'G', "g %.2f", PRIVATE_DATA->target_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(): returned error");
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->rotation_active = true;
	//- dome.DOME_HORIZONTAL_COORDINATES.on_change
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
}

static void dome_steps_handler(indigo_device *device) {
	//+ dome.DOME_STEPS.on_change
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device commands run
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
		return;
	}
	if (!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position, 'Q', "q")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
	}
	// the step is a whole number of degrees, the current heading keeps its fraction
	DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
	if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
		PRIVATE_DATA->target_position = fmod(PRIVATE_DATA->current_position - DOME_STEPS_ITEM->number.value + 360, 360);
	} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
		PRIVATE_DATA->target_position = fmod(PRIVATE_DATA->current_position + DOME_STEPS_ITEM->number.value + 360, 360);
	}
	if (!nexdome_command(device, 'G', "g %.2f", PRIVATE_DATA->target_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(): returned error");
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed.");
		return;
	}
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	PRIVATE_DATA->rotation_active = true;
	//- dome.DOME_STEPS.on_change
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}

static void dome_park_handler(indigo_device *device) {
	//+ dome.DOME_PARK.on_change
	DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device commands run
	if (DOME_PARK_UNPARKED_ITEM->sw.value) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_requested = false;
	} else {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		if (!nexdome_get_azimuth(device, &PRIVATE_DATA->park_azimuth, 'N', "n")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_park_azimuth(): returned error");
		}
		if (!nexdome_command(device, 'G', "g %.2f", PRIVATE_DATA->park_azimuth)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(): returned error");
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->target_position = PRIVATE_DATA->park_azimuth;
			PRIVATE_DATA->park_requested = PRIVATE_DATA->rotation_active = true;
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		}
	}
	//- dome.DOME_PARK.on_change
	indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
}

static void dome_abort_motion_handler(indigo_device *device) {
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	// urgent abort can overtake queued requests: settle the properties they left BUSY
	indigo_cancel_pending_handler(device, dome_horizontal_coordinates_handler);
	indigo_cancel_pending_handler(device, dome_steps_handler);
	indigo_cancel_pending_handler(device, dome_park_handler);
	indigo_cancel_pending_handler(device, dome_shutter_handler);
	indigo_cancel_pending_handler(device, dome_x_find_home_handler);
	indigo_cancel_pending_handler(device, dome_x_calibrate_handler);
	if (!PRIVATE_DATA->rotation_active && !PRIVATE_DATA->rotation_observed && (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->current_position;
		INDIGO_UPDATE_PROPERTY_STATE(DOME_HORIZONTAL_COORDINATES_PROPERTY, INDIGO_OK_STATE, NULL);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_STEPS_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->park_requested && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_PARK_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->shutter_active && !PRIVATE_DATA->shutter_observed && DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		nexdome_update_shutter_switches(device);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->home_active && X_FIND_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_FIND_HOME_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(X_FIND_HOME_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->calibration_active && X_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_CALIBRATE_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!nexdome_command(device, 'A', "a")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_abort(): returned error");
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
	} else {
		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}
		PRIVATE_DATA->park_requested = false;
		if (PRIVATE_DATA->home_active) {
			PRIVATE_DATA->home_active = false;
			indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
			INDIGO_UPDATE_PROPERTY_STATE(X_FIND_HOME_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
		if (PRIVATE_DATA->calibration_active) {
			PRIVATE_DATA->calibration_active = false;
			indigo_set_switch(X_CALIBRATE_PROPERTY, X_CALIBRATE_ITEM, false);
			INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
		// the next status poll publishes the stopped shutter
		PRIVATE_DATA->shutter_target = -1;
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		PRIVATE_DATA->abort_requested = true;
	}
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	bool open = DOME_SHUTTER_OPENED_ITEM->sw.value;
	if (!nexdome_command(device, 'D', open ? "d" : "e")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_%s_shutter(): returned error", open ? "open" : "close");
		nexdome_update_shutter_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->shutter_active = true;
		PRIVATE_DATA->shutter_target = open ? SHUTTER_STATE_OPEN : SHUTTER_STATE_CLOSED;
		PRIVATE_DATA->shutter_started = indigo_monotonic_time();
	}
	//- dome.DOME_SHUTTER.on_change
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_x_reversed_handler(indigo_device *device) {
	//+ dome.X_REVERSED.on_change
	X_REVERSED_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	bool reversed = X_REVERSED_YES_ITEM->sw.value;
	if (!nexdome_command(device, 'Y', "y %d", (int)reversed)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_set_reversed_flag(%d): returned error", reversed);
		indigo_set_switch(X_REVERSED_PROPERTY, PRIVATE_DATA->reversed ? X_REVERSED_YES_ITEM : X_REVERSED_NO_ITEM, true);
		X_REVERSED_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->reversed = reversed;
		X_REVERSED_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- dome.X_REVERSED.on_change
	indigo_update_property(device, X_REVERSED_PROPERTY, NULL);
}

static void dome_x_reset_shutter_comm_handler(indigo_device *device) {
	//+ dome.X_RESET_SHUTTER_COMM.on_change
	X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (!X_RESET_SHUTTER_COMM_ITEM->sw.value) {
		X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		if (!nexdome_command(device, 'W', "w")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_restart_shutter_communication(): returned error");
			indigo_set_switch(X_RESET_SHUTTER_COMM_PROPERTY, X_RESET_SHUTTER_COMM_ITEM, false);
			X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			// wait for the XBee link to reinitialize
			indigo_execute_handler_in(device, NEXDOME_WIRELESS_RESET_DELAY, reset_shutter_comm_finalizer);
		}
	}
	indigo_update_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
	//- dome.X_RESET_SHUTTER_COMM.on_change
}

static void dome_x_find_home_handler(indigo_device *device) {
	//+ dome.X_FIND_HOME.on_change
	X_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (!X_FIND_HOME_ITEM->sw.value) {
		X_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		if (!nexdome_command(device, 'H', "h")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_find_home(): returned error");
			indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
			X_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->home_active = true;
		}
	}
	//- dome.X_FIND_HOME.on_change
	indigo_update_property(device, X_FIND_HOME_PROPERTY, NULL);
}

static void dome_x_calibrate_handler(indigo_device *device) {
	//+ dome.X_CALIBRATE.on_change
	X_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (!X_CALIBRATE_ITEM->sw.value) {
		X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		if (!nexdome_command(device, 'C', "c")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_callibrate(): returned error.");
			indigo_set_switch(X_CALIBRATE_PROPERTY, X_CALIBRATE_ITEM, false);
			X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CALIBRATE_PROPERTY, "Callibration failed. Is the dome in home position?");
			return;
		}
		PRIVATE_DATA->calibration_active = true;
	}
	//- dome.X_CALIBRATE.on_change
	indigo_update_property(device, X_CALIBRATE_PROPERTY, NULL);
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ dome.on_attach
		INFO_PROPERTY->count = 6;
		//- dome.on_attach
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_ON_COORDINATES_SET_PROPERTY->hidden = false;
		//+ dome.DOME_ON_COORDINATES_SET.on_attach
		DOME_ON_COORDINATES_SET_PROPERTY->count = 2;
		//- dome.DOME_ON_COORDINATES_SET.on_attach
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = false;
		//+ dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		//- dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_STEPS_PROPERTY->hidden = false;
		DOME_PARK_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		DOME_SHUTTER_PROPERTY->hidden = false;
		X_REVERSED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_REVERSED_PROPERTY_NAME, "Settings", "Reversed dome directions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_REVERSED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_REVERSED_YES_ITEM, X_REVERSED_YES_ITEM_NAME, "Yes", false);
		indigo_init_switch_item(X_REVERSED_NO_ITEM, X_REVERSED_NO_ITEM_NAME, "No", false);
		X_RESET_SHUTTER_COMM_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RESET_SHUTTER_COMM_PROPERTY_NAME, "Settings", "Reset shutter communication", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_RESET_SHUTTER_COMM_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RESET_SHUTTER_COMM_ITEM, X_RESET_SHUTTER_COMM_ITEM_NAME, "Reset", false);
		X_FIND_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FIND_HOME_PROPERTY_NAME, "Settings", "Find home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_FIND_HOME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FIND_HOME_ITEM, X_FIND_HOME_ITEM_NAME, "Find home", false);
		X_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_PROPERTY_NAME, "Settings", "Callibrate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_ITEM, X_CALIBRATE_ITEM_NAME, "Callibrate", false);
		X_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, X_POWER_PROPERTY_NAME, "Settings", "Power status", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (X_POWER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_POWER_ROTATOR_ITEM, X_POWER_ROTATOR_ITEM_NAME, "Rotator (Volts)", 0, 500, 1, 0);
		strcpy(X_POWER_ROTATOR_ITEM->number.format, "%.2f");
		indigo_init_number_item(X_POWER_SHUTTER_ITEM, X_POWER_SHUTTER_ITEM_NAME, "Shutter (Volts)", 0, 500, 1, 0);
		strcpy(X_POWER_SHUTTER_ITEM->number.format, "%.2f");
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_REVERSED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RESET_SHUTTER_COMM_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FIND_HOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_POWER_PROPERTY);
	}
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, dome_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_HORIZONTAL_COORDINATES_PROPERTY, dome_horizontal_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_STEPS_PROPERTY, property)) {
		if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < DOME_STEPS_PROPERTY->count; i++) {
				DOME_STEPS_PROPERTY->items[i].do_update = true;
			}
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_STEPS_PROPERTY, dome_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_PARK_PROPERTY, dome_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_REVERSED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_REVERSED_PROPERTY, dome_x_reversed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RESET_SHUTTER_COMM_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RESET_SHUTTER_COMM_PROPERTY, dome_x_reset_shutter_comm_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FIND_HOME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FIND_HOME_PROPERTY, dome_x_find_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_PROPERTY, dome_x_calibrate_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(X_REVERSED_PROPERTY);
	indigo_release_property(X_RESET_SHUTTER_COMM_PROPERTY);
	indigo_release_property(X_FIND_HOME_PROPERTY);
	indigo_release_property(X_CALIBRATE_PROPERTY);
	indigo_release_property(X_POWER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_nexdome(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nexdome_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (nexdome_private_data *)indigo_safe_malloc(sizeof(nexdome_private_data));
			dome = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				indigo_safe_free(dome);
				dome = NULL;
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
