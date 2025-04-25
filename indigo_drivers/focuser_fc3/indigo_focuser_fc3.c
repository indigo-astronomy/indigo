// Copyright (c) 2024 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
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

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO PegasusAstro FocusCube focuser driver
 \file indigo_focuser_fc3.c
 */

#define DRIVER_VERSION 0x03000002
#define DRIVER_NAME "indigo_focuser_fc3"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_fc3.h"

#define PRIVATE_DATA	((focuscube_private_data *)device->private_data)

typedef struct {
	indigo_uni_handle *handle;
	indigo_timer *timer;
	pthread_mutex_t mutex;
} focuscube_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool fc3_command(indigo_device *device, char *command, char *response, int max) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) > 0) {
			if (response != NULL) {
				if (indigo_uni_read_line(PRIVATE_DATA->handle, response, max) > 0) {
					return true;
				}
			}
		}
	}
	return false;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyFC");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Undefined");
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	char response[32];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	bool update = false;
	if (fc3_command(device, "FA", response, sizeof(response)) && !strncmp(response, "FC3:", 4)) {
		char *pnt, *token = strtok_r(response, ":", &pnt);
		token = strtok_r(NULL, ":", &pnt); // position
		if (token) {
			int pos = atoi(token);
			if (FOCUSER_POSITION_ITEM->number.value != pos) {
				FOCUSER_POSITION_ITEM->number.value = pos;
				update = true;
			}
		}
		token = strtok_r(NULL, ":", &pnt); // is_running
		if (token) {
			if (*token == '0') {
				if (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
					FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					update = true;
				}
			} else {
				if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
					update = true;
				}
			}
		}
		token = strtok_r(NULL, ":", &pnt); // temperature
		if (token) {
			double temp = indigo_atod(token);
			if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		}
	}
	if (update) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			if (fc3_command(device, "##", response, sizeof(response)) && !strncmp(response, "FC3_", 4)) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "FocusCube v3");
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%s OK", response + 4);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser not detected");
				indigo_uni_close(&PRIVATE_DATA->handle);
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			if (fc3_command(device, "FA", response, sizeof(response)) && !strncmp(response, "FC3:", 4)) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				token = strtok_r(NULL, ":", &pnt); // position
				if (token) {
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atoi(token);
				}
				token = strtok_r(NULL, ":", &pnt); // is_running
				if (token) {
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				}
				token = strtok_r(NULL, ":", &pnt); // temperature
				if (token) {
					FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = indigo_atod(token);
				}
				token = strtok_r(NULL, ":", &pnt); // direction
				if (token) {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, *token == '1' ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // backlash
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = atoi(token);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'FA' response");
					indigo_uni_close(&PRIVATE_DATA->handle);
				}
			}
			if (fc3_command(device, "FV", response, sizeof(response)) && !strncmp(response, "FV:", 3)) {
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 3);
			}
			if (fc3_command(device, "SP", response, sizeof(response)) && !strncmp(response, "SP:", 3)) {
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = atoi(response + 3);
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			indigo_update_property(device, INFO_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle != NULL) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			fc3_command(device, "FH", response, sizeof(response));
			strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Undefined");
			indigo_update_property(device, INFO_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_speed_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	snprintf(command, sizeof(command), "SP:%d", (int)FOCUSER_SPEED_ITEM->number.value);
	if (fc3_command(device, command, response, sizeof(response))) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	snprintf(command, sizeof(command), "FG:%d", (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1));
	if (fc3_command(device, command, NULL, 0)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		snprintf(command, sizeof(command), "FM:%d", (int)FOCUSER_POSITION_ITEM->number.value);
		if (fc3_command(device, command, response, sizeof(response))) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		snprintf(command, sizeof(command), "FN:%d", (int)FOCUSER_POSITION_ITEM->number.value);
		if (fc3_command(device, command, response, sizeof(response))) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (fc3_command(device, "FH", response, sizeof(response))) {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	snprintf(command, sizeof(command), "FD:%d", (int)FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value? 0 : 1);
	if (fc3_command(device, command, response, sizeof(response))) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	snprintf(command, sizeof(command), "BL:%d", (int)FOCUSER_BACKLASH_ITEM->number.value);
	if (fc3_command(device, command, response, sizeof(response))) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_speed_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_reverse_motion_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_backlash_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
			return indigo_device_change_property(device, client, property);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_fc3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static focuscube_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus FocusCube3",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "PegasusAstro FocusCube v3 Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(focuscube_private_data));
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
