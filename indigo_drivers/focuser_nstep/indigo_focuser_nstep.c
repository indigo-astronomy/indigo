// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
//
// Thanks to Gene Nolan and Leon Palmer for their support.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Rigel Systems nSTEP focuser driver
 \file indigo_focuser_nstep.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_focuser_nstep"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_focuser_nstep.h"

#define PRIVATE_DATA													((nstep_private_data *)device->private_data)

#define X_FOCUSER_STEPPING_MODE_PROPERTY			(PRIVATE_DATA->stepping_mode_property)
#define X_FOCUSER_STEPPING_MODE_WAVE_ITEM			(X_FOCUSER_STEPPING_MODE_PROPERTY->items+0)
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM			(X_FOCUSER_STEPPING_MODE_PROPERTY->items+1)
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM			(X_FOCUSER_STEPPING_MODE_PROPERTY->items+2)

#define X_FOCUSER_PHASE_WIRING_PROPERTY				(PRIVATE_DATA->phase_wiring_property)
#define X_FOCUSER_PHASE_WIRING_0_ITEM					(X_FOCUSER_PHASE_WIRING_PROPERTY->items+0)
#define X_FOCUSER_PHASE_WIRING_1_ITEM					(X_FOCUSER_PHASE_WIRING_PROPERTY->items+1)
#define X_FOCUSER_PHASE_WIRING_2_ITEM					(X_FOCUSER_PHASE_WIRING_PROPERTY->items+2)

typedef struct {
	int handle;
	indigo_timer *timer;
	indigo_property *stepping_mode_property;
	indigo_property *phase_wiring_property;
	indigo_property *compensation_backlash_property;
	pthread_mutex_t mutex;
} nstep_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool nstep_command(indigo_device *device, char *command, char *response, int length) {
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (response != NULL) {
		if (indigo_read(PRIVATE_DATA->handle, response, length) != length) {
			*response = 0;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command failed %s", command);
			return false;
		}
		response[length] = 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		X_FOCUSER_STEPPING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_STEPPING_MODE", FOCUSER_MAIN_GROUP, "Stepping mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_STEPPING_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_WAVE_ITEM, "WAVE", "Wave", false);
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_HALF_ITEM, "HALF", "Half", false);
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_FULL_ITEM, "FULL", "Full", true);
		X_FOCUSER_PHASE_WIRING_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_PHASE_WIRING", FOCUSER_MAIN_GROUP, "Phase wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_PHASE_WIRING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_PHASE_WIRING_0_ITEM, "0", "0", true);
		indigo_init_switch_item(X_FOCUSER_PHASE_WIRING_1_ITEM, "1", "1", false);
		indigo_init_switch_item(X_FOCUSER_PHASE_WIRING_2_ITEM, "2", "2", false);
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 0;
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 254;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = -999999;
		FOCUSER_POSITION_ITEM->number.max = 999999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;

		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_STEPPING_MODE_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
		if (indigo_property_match(X_FOCUSER_PHASE_WIRING_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[8];
	if (!FOCUSER_TEMPERATURE_PROPERTY->hidden) {
		if (nstep_command(device, ":RT", response, 4)) {
			double temp = atoi(response) / 10.0;
			if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		}
	}
	bool moving = false;
	if (nstep_command(device, "S", response, 1)) {
		moving = *response == '1';
	}
	long position = 0;
	if (nstep_command(device, ":RP", response, 7)) {
		position = atol(response);
	}
	bool update = false;
	if (moving) {
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			update = true;
		}
		if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			update = true;
		}
	} else {
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			update = true;
		}
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			update = true;
		}
	}
	if (FOCUSER_POSITION_ITEM->number.value != position) {
		FOCUSER_POSITION_ITEM->number.value = position;
		update = true;
	}
	if (update) {
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, moving ? 0.5 : 5, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[8];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			if (nstep_command(device, "\006", response, 1) && *response == 'S') {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "nSTEP detected");
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nSTEP not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (nstep_command(device, ":RT", response, 4)) {
				if (strcmp(response, "-888") == 0) {
					FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
					FOCUSER_COMPENSATION_PROPERTY->hidden = true;
					FOCUSER_MODE_PROPERTY->hidden = true;
				} else {
					FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
					FOCUSER_TEMPERATURE_ITEM->number.value = atoi(response) / 10.0;
					FOCUSER_COMPENSATION_PROPERTY->hidden = false;
					double tt = 0;
					if (nstep_command(device, ":RA", response, 4)) {
						tt = atoi(response) / 10.0;
					}
					int ts = 0;
					if (nstep_command(device, ":RB", response, 3)) {
						ts = atoi(response);
					}
					if (tt == 0)
						FOCUSER_COMPENSATION_ITEM->number.value = 0;
					else
						FOCUSER_COMPENSATION_ITEM->number.value = ts / tt;
					FOCUSER_MODE_PROPERTY->hidden = false;
					if (nstep_command(device, ":RG", response, 1)) {
						indigo_set_switch(FOCUSER_MODE_PROPERTY, *response == '2' ? FOCUSER_MODE_AUTOMATIC_ITEM : FOCUSER_MODE_MANUAL_ITEM, true);
					}
					if (nstep_command(device, ":RE", response, 3)) {
						FOCUSER_BACKLASH_ITEM->number.value = atoi(response);
					}
				}
			}
			if (nstep_command(device, ":RP", response, 7)) {
				FOCUSER_POSITION_ITEM->number.value = atol(response);
			}
			if (nstep_command(device, ":RO", response, 3)) {
				FOCUSER_SPEED_ITEM->number.value = 255 - atoi(response);
			}
			if (nstep_command(device, ":RW", response, 1)) {
				switch (*response) {
					case '0':
						indigo_set_switch(X_FOCUSER_PHASE_WIRING_PROPERTY, X_FOCUSER_PHASE_WIRING_0_ITEM, true);
						break;
					case '1':
						indigo_set_switch(X_FOCUSER_PHASE_WIRING_PROPERTY, X_FOCUSER_PHASE_WIRING_1_ITEM, true);
						break;
					case '2':
						indigo_set_switch(X_FOCUSER_PHASE_WIRING_PROPERTY, X_FOCUSER_PHASE_WIRING_2_ITEM, true);
						break;
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
			nstep_command(device, ":CC1", NULL, 0);
			nstep_command(device, ":CS001#", NULL, 0);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			nstep_command(device, ":F10000#", NULL, 0);
			indigo_delete_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
			indigo_delete_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_speed_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (IS_CONNECTED) {
		snprintf(command, sizeof(command), ":CO%03d#", 255 - (int)FOCUSER_SPEED_ITEM->number.value);
		if (nstep_command(device, command, NULL, 0)) {
			FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	int direction = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : 0;
	int mode = X_FOCUSER_STEPPING_MODE_WAVE_ITEM->sw.value ? 0 : (X_FOCUSER_STEPPING_MODE_HALF_ITEM->sw.value ? 1 : 2);
	snprintf(command, sizeof(command), ":F%1d%1d%03d#", direction, mode, (int)FOCUSER_STEPS_ITEM->number.value);
	if (nstep_command(device, command, NULL, 0)) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (nstep_command(device, ":F10000#", NULL, 0)) {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_phase_wiring_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (X_FOCUSER_PHASE_WIRING_1_ITEM->sw.value)
		strcpy(command, ":CW1#");
	else if (X_FOCUSER_PHASE_WIRING_2_ITEM->sw.value)
		strcpy(command, ":CW2#");
	else
		strcpy(command, ":CW0#");
	if (nstep_command(device, command, NULL, 0)) {
		X_FOCUSER_PHASE_WIRING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_FOCUSER_PHASE_WIRING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_PHASE_WIRING_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_mode_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		strcpy(command, ":TA2:TC30#");
	} else {
		strcpy(command, ":TA0");
	}
	if (nstep_command(device, command, NULL, 0)) {
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_compensation_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (IS_CONNECTED) {
		if (FOCUSER_COMPENSATION_ITEM->number.value > 0)
			sprintf(command, ":TT+010#:TS%03d#", (int)FOCUSER_COMPENSATION_ITEM->number.value);
		else
			sprintf(command, ":TT-010#:TS%03d#", (int)FOCUSER_COMPENSATION_ITEM->number.value);
		if (nstep_command(device, command, NULL, 0)) {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (IS_CONNECTED) {
		snprintf(command, sizeof(command), ":TB%03d#", (int)FOCUSER_BACKLASH_ITEM->number.value);
		if (nstep_command(device, command, NULL, 0)) {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_speed_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_STEPPING_MODE
	} else if (indigo_property_match(X_FOCUSER_STEPPING_MODE_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_STEPPING_MODE_PROPERTY, property, false);
		X_FOCUSER_STEPPING_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_PHASE_WIRING
	} else if (indigo_property_match(X_FOCUSER_PHASE_WIRING_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_PHASE_WIRING_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_phase_wiring_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_mode_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_compensation_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_backlash_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEPPING_MODE_PROPERTY);
	indigo_release_property(X_FOCUSER_PHASE_WIRING_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_nstep(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nstep_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"nSTEP",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "Rigel Systems nSTEP Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(nstep_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				free(focuser);
				focuser = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
