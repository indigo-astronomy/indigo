// Copyright (c) 2018-2026 CloudMakers, s. r. o.
// All rights reserved.

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

// This file generated from indigo_focuser_nstep.driver

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

#include "indigo_focuser_nstep.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_focuser_nstep"
#define DRIVER_LABEL         "Rigel Systems nSTEP Focuser"
#define FOCUSER_DEVICE_NAME  "nSTEP"
#define PRIVATE_DATA         ((nstep_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define NSTEP_STATUS_STALL_LIMIT 12

//- define

#pragma mark - Property definitions

#define X_FOCUSER_STEPPING_MODE_PROPERTY       (PRIVATE_DATA->x_focuser_stepping_mode_property)
#define X_FOCUSER_STEPPING_MODE_WAVE_ITEM      (X_FOCUSER_STEPPING_MODE_PROPERTY->items + 0)
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM      (X_FOCUSER_STEPPING_MODE_PROPERTY->items + 1)
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM      (X_FOCUSER_STEPPING_MODE_PROPERTY->items + 2)

#define X_FOCUSER_STEPPING_MODE_PROPERTY_NAME  "X_FOCUSER_STEPPING_MODE"
#define X_FOCUSER_STEPPING_MODE_WAVE_ITEM_NAME "WAVE"
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME "HALF"
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM_NAME "FULL"

#define X_FOCUSER_PHASE_WIRING_PROPERTY      (PRIVATE_DATA->x_focuser_phase_wiring_property)
#define X_FOCUSER_PHASE_WIRING_0_ITEM        (X_FOCUSER_PHASE_WIRING_PROPERTY->items + 0)
#define X_FOCUSER_PHASE_WIRING_1_ITEM        (X_FOCUSER_PHASE_WIRING_PROPERTY->items + 1)
#define X_FOCUSER_PHASE_WIRING_2_ITEM        (X_FOCUSER_PHASE_WIRING_PROPERTY->items + 2)

#define X_FOCUSER_PHASE_WIRING_PROPERTY_NAME "X_FOCUSER_PHASE_WIRING"
#define X_FOCUSER_PHASE_WIRING_0_ITEM_NAME   "0"
#define X_FOCUSER_PHASE_WIRING_1_ITEM_NAME   "1"
#define X_FOCUSER_PHASE_WIRING_2_ITEM_NAME   "2"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_stepping_mode_property;
	indigo_property *x_focuser_phase_wiring_property;
	//+ data
	char response[16];
	int stalled;
	bool active, uncertain;
	//- data
} nstep_private_data;

#pragma mark - Low level code

//+ code

static void focuser_steps_handler(indigo_device *device);

static bool nstep_command(indigo_device *device, int reply_length, const char *command, ...) {
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
	if (reply_length == 0) {
		return true;
	}
	if (reply_length < 0 || reply_length >= (int)sizeof(PRIVATE_DATA->response)) {
		return false;
	}
	long count = indigo_uni_read_section(PRIVATE_DATA->handle, RESPONSE, reply_length, "", "", INDIGO_DELAY(1));
	if (count != reply_length) {
		RESPONSE[0] = 0;
		return false;
	}
	if (indigo_uni_discard(PRIVATE_DATA->handle) > 0) {
		RESPONSE[0] = 0;
		return false;
	}
	return true;
}

static bool nstep_integer(const char *text, int minimum, int maximum, int *value) {
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

static bool nstep_temperature(indigo_device *device, bool *present) {
	int tenths = 0;
	if (!nstep_command(device, 4, ":RT")) {
		return false;
	}
	if (!strcmp(RESPONSE, "-888")) {
		*present = false;
		return true;
	}
	if (!nstep_integer(RESPONSE, -999, 999, &tenths)) {
		return false;
	}
	*present = true;
	FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = tenths / 10.0;
	return true;
}

static bool nstep_speed(indigo_device *device, int *speed) {
	int raw = 0;
	if (!nstep_command(device, 3, ":RO") || !nstep_integer(RESPONSE, 1, 254, &raw)) {
		return false;
	}
	*speed = 255 - raw;
	return true;
}

static bool nstep_position(indigo_device *device, int *position) {
	if (!nstep_command(device, 7, ":RP")) {
		return false;
	}
	return nstep_integer(RESPONSE, -999999, 999999, position);
}

static bool nstep_moving(indigo_device *device, bool *moving) {
	if (!nstep_command(device, 1, "S") || (RESPONSE[0] != '0' && RESPONSE[0] != '1')) {
		return false;
	}
	*moving = RESPONSE[0] == '1';
	return true;
}

static bool nstep_stop(indigo_device *device) {
	return nstep_command(device, 0, ":F10000#");
}

static void nstep_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_STEPS_PROPERTY->state = state;
	FOCUSER_POSITION_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	bool moving = false;
	if (!nstep_moving(device, &moving)) {
		if (++PRIVATE_DATA->stalled < NSTEP_STATUS_STALL_LIMIT) {
			indigo_execute_handler_in(device, 0.5, motion_finalizer);
			return;
		}
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		nstep_stop(device);
		nstep_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->stalled = 0;
	int position = 0;
	if (nstep_position(device, &position)) {
		FOCUSER_POSITION_ITEM->number.value = position;
	}
	if (moving) {
		nstep_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.5, motion_finalizer);
	} else {
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		nstep_motion_state(device, INDIGO_OK_STATE);
	}
}

static bool nstep_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle && nstep_command(device, 1, "%c", 0x06) && !strcmp(RESPONSE, "S")) {
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void nstep_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && !FOCUSER_TEMPERATURE_PROPERTY->hidden) {
		bool temperature_present = true;
		FOCUSER_TEMPERATURE_PROPERTY->state = nstep_temperature(device, &temperature_present) && temperature_present ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (!PRIVATE_DATA->active) {
		int position = 0;
		if (nstep_position(device, &position) && FOCUSER_POSITION_ITEM->number.value != position) {
			FOCUSER_POSITION_ITEM->number.value = position;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, PRIVATE_DATA->active ? 0.5 : 5, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = nstep_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int speed = 0, position = 0;
			bool temperature_present = true;
			char phase = '0';
			connection_result = nstep_temperature(device, &temperature_present);
			if (connection_result && temperature_present) {
				int coefficient = 0, steps = 0, backlash = 0;
				char mode = '0';
				connection_result = nstep_command(device, 4, ":RA") && nstep_integer(RESPONSE, -999, 999, &coefficient);
				if (connection_result) {
					connection_result = nstep_command(device, 3, ":RB") && nstep_integer(RESPONSE, 0, 999, &steps);
				}
				if (connection_result) {
					connection_result = nstep_command(device, 1, ":RG") && (RESPONSE[0] == '0' || RESPONSE[0] == '1' || RESPONSE[0] == '2');
					mode = RESPONSE[0];
				}
				if (connection_result) {
					connection_result = nstep_command(device, 3, ":RE") && nstep_integer(RESPONSE, 0, 999, &backlash);
				}
				if (connection_result) {
					double tt = coefficient / 10.0;
					FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = tt == 0 ? 0 : steps / tt;
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = backlash;
					indigo_set_switch(FOCUSER_MODE_PROPERTY, mode == '2' ? FOCUSER_MODE_AUTOMATIC_ITEM : FOCUSER_MODE_MANUAL_ITEM, true);
				}
			}
			if (connection_result) {
				connection_result = nstep_position(device, &position) && nstep_speed(device, &speed);
			}
			if (connection_result) {
				connection_result = nstep_command(device, 1, ":RW") && (RESPONSE[0] == '0' || RESPONSE[0] == '1' || RESPONSE[0] == '2');
				phase = RESPONSE[0];
			}
			if (connection_result) {
				connection_result = nstep_command(device, 0, ":CC1") && nstep_command(device, 0, ":CS001#");
			}
			if (connection_result) {
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
				PRIVATE_DATA->stalled = 0;
				FOCUSER_TEMPERATURE_PROPERTY->hidden = FOCUSER_COMPENSATION_PROPERTY->hidden = FOCUSER_MODE_PROPERTY->hidden = !temperature_present;
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = speed;
				FOCUSER_POSITION_ITEM->number.value = position;
				indigo_set_switch(X_FOCUSER_PHASE_WIRING_PROPERTY, phase == '2' ? X_FOCUSER_PHASE_WIRING_2_ITEM : (phase == '1' ? X_FOCUSER_PHASE_WIRING_1_ITEM : X_FOCUSER_PHASE_WIRING_0_ITEM), true);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				nstep_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
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
		indigo_cancel_pending_handler(device, motion_finalizer);
		if (PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
			nstep_stop(device);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
		nstep_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	int requested = (int)FOCUSER_SPEED_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !nstep_command(device, 0, ":CO%03d#", 255 - requested)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_SPEED_ITEM->number.value = requested;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	int mode = X_FOCUSER_STEPPING_MODE_WAVE_ITEM->sw.value ? 0 : (X_FOCUSER_STEPPING_MODE_HALF_ITEM->sw.value ? 1 : 2);
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		nstep_motion_state(device, INDIGO_ALERT_STATE);
	} else if (steps == 0) {
		nstep_motion_state(device, INDIGO_OK_STATE);
	} else if (nstep_command(device, 0, ":F%d%d%03d#", FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : 0, mode, steps)) {
		PRIVATE_DATA->active = true;
		PRIVATE_DATA->uncertain = false;
		PRIVATE_DATA->stalled = 0;
		nstep_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.5, motion_finalizer);
	} else {
		PRIVATE_DATA->uncertain = true;
		nstep_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		PRIVATE_DATA->active = false;
		if (IS_CONNECTED && nstep_stop(device)) {
			PRIVATE_DATA->uncertain = false;
			nstep_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			nstep_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!IS_CONNECTED || !nstep_command(device, 0, ":TB%03d#", (int)FOCUSER_BACKLASH_ITEM->number.target)) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	int compensation = (int)FOCUSER_COMPENSATION_ITEM->number.target;
	if (!IS_CONNECTED || !nstep_command(device, 0, compensation > 0 ? ":TT+010#:TS%03d#" : ":TT-010#:TS%03d#", compensation)) {
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	if (!IS_CONNECTED || !nstep_command(device, 0, FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? ":TA2:TC30#" : ":TA0")) {
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_x_focuser_phase_wiring_handler(indigo_device *device) {
	X_FOCUSER_PHASE_WIRING_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_PHASE_WIRING.on_change
	int wiring = X_FOCUSER_PHASE_WIRING_1_ITEM->sw.value ? 1 : (X_FOCUSER_PHASE_WIRING_2_ITEM->sw.value ? 2 : 0);
	if (!IS_CONNECTED || !nstep_command(device, 0, ":CW%d#", wiring)) {
		X_FOCUSER_PHASE_WIRING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_PHASE_WIRING.on_change
	indigo_update_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Rigel Systems nSTEP");
		FOCUSER_POSITION_ITEM->number.min = -999999;
		FOCUSER_POSITION_ITEM->number.max = 999999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		//- focuser.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 254;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 999;
		FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = 0;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_MODE.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		//- focuser.FOCUSER_MODE.on_attach
		X_FOCUSER_STEPPING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Stepping mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_STEPPING_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_WAVE_ITEM, X_FOCUSER_STEPPING_MODE_WAVE_ITEM_NAME, "Wave", false);
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_HALF_ITEM, X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME, "Half", false);
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_FULL_ITEM, X_FOCUSER_STEPPING_MODE_FULL_ITEM_NAME, "Full", true);
		X_FOCUSER_PHASE_WIRING_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_PHASE_WIRING_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Phase wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_PHASE_WIRING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_PHASE_WIRING_0_ITEM, X_FOCUSER_PHASE_WIRING_0_ITEM_NAME, "0", true);
		indigo_init_switch_item(X_FOCUSER_PHASE_WIRING_1_ITEM, X_FOCUSER_PHASE_WIRING_1_ITEM_NAME, "1", false);
		indigo_init_switch_item(X_FOCUSER_PHASE_WIRING_2_ITEM, X_FOCUSER_PHASE_WIRING_2_ITEM_NAME, "2", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_STEPPING_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_PHASE_WIRING_PROPERTY);
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
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_STEPPING_MODE_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_STEPPING_MODE_PROPERTY, property, false);
		X_FOCUSER_STEPPING_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_PHASE_WIRING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_PHASE_WIRING_PROPERTY, focuser_x_focuser_phase_wiring_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEPPING_MODE_PROPERTY);
	indigo_release_property(X_FOCUSER_PHASE_WIRING_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_nstep(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nstep_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (nstep_private_data *)indigo_safe_malloc(sizeof(nstep_private_data));
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
