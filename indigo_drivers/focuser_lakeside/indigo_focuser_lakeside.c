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

// This file generated from indigo_focuser_lakeside.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_lakeside.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_focuser_lakeside"
#define DRIVER_LABEL         "LakesideAstro Focuser"
#define FOCUSER_DEVICE_NAME  "LakesideAstro Focuser"
#define PRIVATE_DATA         ((lakeside_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define X_FOCUSER_DEADBAND_ITEM (FOCUSER_COMPENSATION_PROPERTY->items + 1)
#define X_FOCUSER_PERIOD_ITEM (FOCUSER_COMPENSATION_PROPERTY->items + 2)

//- define

#pragma mark - Property definitions

#define X_FOCUSER_ACTIVE_SLOPE_PROPERTY      (PRIVATE_DATA->x_focuser_active_slope_property)
#define X_FOCUSER_ACTIVE_SLOPE_1_ITEM        (X_FOCUSER_ACTIVE_SLOPE_PROPERTY->items + 0)
#define X_FOCUSER_ACTIVE_SLOPE_2_ITEM        (X_FOCUSER_ACTIVE_SLOPE_PROPERTY->items + 1)

#define X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME "X_FOCUSER_ACTIVE_SLOPE"
#define X_FOCUSER_ACTIVE_SLOPE_1_ITEM_NAME   "1"
#define X_FOCUSER_ACTIVE_SLOPE_2_ITEM_NAME   "2"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_active_slope_property;
	//+ data
	char response[64];
	int position, expected_position, last_position, stalled, active_slope, abort_position;
	bool active, uncertain;
	//- data
} lakeside_private_data;

#pragma mark - Low level code

//+ code

static void focuser_steps_handler(indigo_device *device);

static bool lakeside_read(indigo_device *device, double timeout) {
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, RESPONSE, sizeof(PRIVATE_DATA->response) - 1, "#", "", INDIGO_DELAY(timeout), INDIGO_DELAY(0.1));
	if (count <= 1 || RESPONSE[count - 1] != '#' || (long)strlen(RESPONSE) != count) {
		return false;
	}
	RESPONSE[count - 1] = 0;
	return true;
}

static bool lakeside_command(indigo_device *device, bool reply, const char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result <= 0) {
		return false;
	}
	return !reply || lakeside_read(device, 1);
}

static bool lakeside_ack(indigo_device *device, const char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	return result > 0 && lakeside_read(device, 1) && !strcmp(RESPONSE, "OK");
}

static bool lakeside_integer(const char *text, int minimum, int maximum, int *value) {
	if (!text || !*text || isspace((unsigned char)*text)) {
		return false;
	}
	char *end;
	errno = 0;
	long number = strtol(text, &end, 10);
	if (errno || end == text || *end || number < minimum || number > maximum) {
		return false;
	}
	*value = (int)number;
	return true;
}

static bool lakeside_value(indigo_device *device, char prefix, int minimum, int maximum, int *value, const char *command) {
	return lakeside_command(device, true, "%s", command) && RESPONSE[0] == prefix && lakeside_integer(RESPONSE + 1, minimum, maximum, value);
}

static bool lakeside_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle && lakeside_command(device, true, "??#") && !strcmp(RESPONSE, "OK")) {
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void lakeside_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void lakeside_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static bool lakeside_position(indigo_device *device) {
	int position = 0;
	if (!lakeside_value(device, 'P', 0, 65535, &position, "?P#")) {
		return false;
	}
	PRIVATE_DATA->position = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain) {
		FOCUSER_POSITION_ITEM->number.target = position;
	}
	return true;
}

static bool lakeside_temperature(indigo_device *device) {
	int half_degrees = 0;
	if (!lakeside_value(device, 'T', -200, 200, &half_degrees, "?T#")) {
		return false;
	}
	FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = half_degrees / 2.0;
	return true;
}

static bool lakeside_read_slope(indigo_device *device, int profile) {
	int slope = 0, direction = 0, deadband = 0, period = 0;
	char magnitude = profile == 1 ? '1' : '2';
	char sign = profile == 1 ? 'a' : 'b';
	char deadband_prefix = profile == 1 ? 'c' : 'd';
	char period_prefix = profile == 1 ? 'e' : 'f';
	char command[4] = { '?', magnitude, '#', 0 };
	if (!lakeside_value(device, magnitude, 0, 127, &slope, command)) {
		return false;
	}
	command[1] = sign;
	if (!lakeside_value(device, sign, 0, 1, &direction, command)) {
		return false;
	}
	command[1] = deadband_prefix;
	if (!lakeside_value(device, deadband_prefix, 0, 65535, &deadband, command)) {
		return false;
	}
	command[1] = period_prefix;
	if (!lakeside_value(device, period_prefix, 0, 65535, &period, command)) {
		return false;
	}
	FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = direction ? -slope : slope;
	X_FOCUSER_DEADBAND_ITEM->number.value = X_FOCUSER_DEADBAND_ITEM->number.target = deadband;
	X_FOCUSER_PERIOD_ITEM->number.value = X_FOCUSER_PERIOD_ITEM->number.target = period;
	return true;
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	if (!lakeside_read(device, 0.2)) {
		if (++PRIVATE_DATA->stalled < 40) {
			indigo_execute_handler_in(device, 0.1, motion_finalizer);
			return;
		}
		lakeside_command(device, false, "CH#");
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		lakeside_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (!strcmp(RESPONSE, "DONE")) {
		PRIVATE_DATA->active = false;
		if (lakeside_position(device)) {
			PRIVATE_DATA->uncertain = false;
			lakeside_motion_state(device, PRIVATE_DATA->position == PRIVATE_DATA->expected_position ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			lakeside_motion_state(device, INDIGO_ALERT_STATE);
		}
		return;
	}
	int position = 0;
	if (RESPONSE[0] != 'P' || !lakeside_integer(RESPONSE + 1, 0, 65535, &position)) {
		lakeside_command(device, false, "CH#");
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		lakeside_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->position = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	if (position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = position;
		PRIVATE_DATA->stalled = 0;
	}
	lakeside_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler(device, motion_finalizer);
}

static void abort_finalizer(indigo_device *device) {
	if (IS_CONNECTED && lakeside_position(device) && PRIVATE_DATA->position == PRIVATE_DATA->abort_position) {
		PRIVATE_DATA->uncertain = false;
		FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		lakeside_motion_state(device, INDIGO_OK_STATE);
	} else {
		PRIVATE_DATA->uncertain = true;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		lakeside_command(device, false, "CH#");
		lakeside_motion_state(device, INDIGO_ALERT_STATE);
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		if (lakeside_position(device)) {
			PRIVATE_DATA->uncertain = false;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_TEMPERATURE_PROPERTY->state = lakeside_temperature(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = lakeside_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int position = 0, backlash = 0, reverse = 0;
			connection_result = lakeside_command(device, false, "CTF#") && lakeside_ack(device, "CRg1#") && lakeside_value(device, 'P', 0, 65535, &position, "?P#") && lakeside_value(device, 'B', 0, 65535, &backlash, "?B#") && lakeside_value(device, 'D', 0, 1, &reverse, "?D#") && lakeside_temperature(device) && lakeside_read_slope(device, 1);
			if (connection_result) {
				PRIVATE_DATA->position = position;
				PRIVATE_DATA->active_slope = 1;
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = backlash;
				indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, reverse ? FOCUSER_REVERSE_MOTION_DISABLED_ITEM : FOCUSER_REVERSE_MOTION_ENABLED_ITEM, true);
				indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_MANUAL_ITEM, true);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				lakeside_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
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
		indigo_cancel_pending_handler(device, abort_finalizer);
		if (PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
			lakeside_command(device, false, "CH#");
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
		lakeside_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	int target = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? PRIVATE_DATA->position - steps : PRIVATE_DATA->position + steps;
	target = target < 0 ? 0 : target > 65535 ? 65535 : target;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		lakeside_motion_state(device, INDIGO_ALERT_STATE);
	} else if (target == PRIVATE_DATA->position) {
		lakeside_motion_state(device, INDIGO_OK_STATE);
	} else if (lakeside_command(device, false, FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? "CI%d#" : "CO%d#", steps)) {
		PRIVATE_DATA->expected_position = target;
		PRIVATE_DATA->last_position = PRIVATE_DATA->position;
		PRIVATE_DATA->stalled = 0;
		PRIVATE_DATA->active = true;
		FOCUSER_POSITION_ITEM->number.target = target;
		lakeside_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	} else {
		PRIVATE_DATA->uncertain = true;
		lakeside_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		bool stopped = IS_CONNECTED && lakeside_command(device, false, "CH#");
		PRIVATE_DATA->active = false;
		if (stopped && lakeside_position(device)) {
			PRIVATE_DATA->abort_position = PRIVATE_DATA->position;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_execute_handler_in(device, 0.2, abort_finalizer);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			lakeside_motion_state(device, INDIGO_ALERT_STATE);
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		}
	} else {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	}
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	int requested = (int)FOCUSER_BACKLASH_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !lakeside_ack(device, "CRB%d#", requested)) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_BACKLASH_ITEM->number.value = requested;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	int profile = PRIVATE_DATA->active_slope;
	int slope = abs((int)FOCUSER_COMPENSATION_ITEM->number.target);
	int direction = FOCUSER_COMPENSATION_ITEM->number.target < 0 ? 1 : 0;
	int deadband = (int)X_FOCUSER_DEADBAND_ITEM->number.target;
	int period = (int)X_FOCUSER_PERIOD_ITEM->number.target;
	bool result = IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->uncertain;
	result = result && lakeside_ack(device, profile == 1 ? "CR1%d#" : "CR2%d#", slope);
	result = result && lakeside_ack(device, profile == 1 ? "CRa%d#" : "CRb%d#", direction);
	result = result && lakeside_ack(device, profile == 1 ? "CRc%d#" : "CRd%d#", deadband);
	result = result && lakeside_ack(device, profile == 1 ? "CRe%d#" : "CRf%d#", period);
	if (!result || !lakeside_read_slope(device, profile)) {
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !lakeside_command(device, false, FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? "CTN#" : "CTF#")) {
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_x_focuser_active_slope_handler(indigo_device *device) {
	X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_ACTIVE_SLOPE.on_change
	int profile = X_FOCUSER_ACTIVE_SLOPE_1_ITEM->sw.value ? 1 : 2;
	if (IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->uncertain && lakeside_ack(device, "CRg%d#", profile) && lakeside_read_slope(device, profile)) {
		PRIVATE_DATA->active_slope = profile;
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_switch(X_FOCUSER_ACTIVE_SLOPE_PROPERTY, PRIVATE_DATA->active_slope == 1 ? X_FOCUSER_ACTIVE_SLOPE_1_ITEM : X_FOCUSER_ACTIVE_SLOPE_2_ITEM, true);
		X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
	//- focuser.X_FOCUSER_ACTIVE_SLOPE.on_change
	indigo_update_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Lakeside Focuser");
		FOCUSER_COMPENSATION_PROPERTY = indigo_resize_property(FOCUSER_COMPENSATION_PROPERTY, 3);
		indigo_init_number_item(X_FOCUSER_DEADBAND_ITEM, "DEADBAND", "Deadband", 0, 65535, 1, 0);
		indigo_init_number_item(X_FOCUSER_PERIOD_ITEM, "PERIOD", "Period", 0, 65535, 1, 6);
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 65535;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 65535;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.max = 65535;
		FOCUSER_BACKLASH_ITEM->number.step = 1;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_ITEM->number.min = -127;
		FOCUSER_COMPENSATION_ITEM->number.max = 127;
		FOCUSER_COMPENSATION_ITEM->number.step = 1;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		X_FOCUSER_ACTIVE_SLOPE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Active slope", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_ACTIVE_SLOPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_ACTIVE_SLOPE_1_ITEM, X_FOCUSER_ACTIVE_SLOPE_1_ITEM_NAME, "Slope #1", true);
		indigo_init_switch_item(X_FOCUSER_ACTIVE_SLOPE_2_ITEM, X_FOCUSER_ACTIVE_SLOPE_2_ITEM_NAME, "Slope #2", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_ACTIVE_SLOPE_PROPERTY);
	}
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
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Abort is unfinished");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_ACTIVE_SLOPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_ACTIVE_SLOPE_PROPERTY, focuser_x_focuser_active_slope_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_ACTIVE_SLOPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_lakeside(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static lakeside_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (lakeside_private_data *)indigo_safe_malloc(sizeof(lakeside_private_data));
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
