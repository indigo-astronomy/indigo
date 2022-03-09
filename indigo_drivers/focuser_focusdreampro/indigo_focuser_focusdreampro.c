// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO AGadget FocusDream focuser driver
 \file indigo_focuser_focusdreampro.c
 */

#define DRIVER_VERSION 0x0004
#define DRIVER_NAME "indigo_focuser_focusdreampro"

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

#include "indigo_focuser_focusdreampro.h"

#define PRIVATE_DATA													((focusdreampro_private_data *)device->private_data)

#define X_FOCUSER_DUTY_CYCLE_PROPERTY					(PRIVATE_DATA->duty_cycle_property)
#define X_FOCUSER_DUTY_CYCLE_ITEM							(X_FOCUSER_DUTY_CYCLE_PROPERTY->items+0)

typedef struct {
	int handle;
	indigo_property *duty_cycle_property;
	indigo_timer *timer;
	pthread_mutex_t mutex;
	bool fdp, jolo;
} focusdreampro_private_data;

static int SPEED[] = { 500, 250, 110, 40, 10, 5 };

// -------------------------------------------------------------------------------- Low level communication routines

static bool focusdreampro_command(indigo_device *device, char *command, char *response, int length) {
	if (indigo_write(PRIVATE_DATA->handle, command, strlen(command)) && indigo_write(PRIVATE_DATA->handle, "\n", 1) && indigo_read_line(PRIVATE_DATA->handle, response, length) < 0) {
		*response = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s failed", command);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response);
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_FOCUSER_FREQUENCY
		X_FOCUSER_DUTY_CYCLE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_FOCUSER_DUTY_CYCLE", FOCUSER_MAIN_GROUP, "Duty cycle", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_FOCUSER_DUTY_CYCLE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_FOCUSER_DUTY_CYCLE_ITEM, "DUTY_CYCLE", "Duty cycle", 0, 100, 1, 20);
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 5;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 100000;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.max = 1000000;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_DUTY_CYCLE_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	if (!FOCUSER_TEMPERATURE_PROPERTY->hidden) {
		if (focusdreampro_command(device, "T", response, sizeof(response)) && *response == 'T') {
			double temp = atof(response + 2);
			if (FOCUSER_TEMPERATURE_ITEM->number.value != temp || FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_OK_STATE) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	bool moving = false;
	if (focusdreampro_command(device, "I", response, sizeof(response)) && *response == 'I') {
		moving = !strcmp(response, "I:true");
	}
	bool update = false;
	if (focusdreampro_command(device, "P", response, sizeof(response)) && *response == 'P') {
		int position = atoi(response + 2);
		if (FOCUSER_POSITION_ITEM->number.value != position) {
			FOCUSER_POSITION_ITEM->number.value = position;
			update = true;
		}
	}
	if (moving) {
		if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			update = true;
		}
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			update = true;
		}
	} else {
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			update = true;
		}
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			update = true;
		}
	}
	if (update) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, moving ? 0.5 : 1, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600);
		if (PRIVATE_DATA->handle > 0) {
			if (focusdreampro_command(device, "#", response, sizeof(response))) {
				if (!strcmp(response, "FD")) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "FocusDreamPro detected");
					PRIVATE_DATA->fdp = true;
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "AGadget FocusDreamPro");
				} else if (!strncmp(response, "Jolo", 4)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Astrojolo detected");
					PRIVATE_DATA->jolo = true;
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "ASCOM Jolo focuser");
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FocusDreamPro not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (focusdreampro_command(device, "T", response, sizeof(response)) && *response == 'T') {
				if (!strcmp(response, "T:false")) {
					FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
				} else {
					FOCUSER_TEMPERATURE_ITEM->number.value = atof(response + 2);
					FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				}
			} else {
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (focusdreampro_command(device, "P", response, sizeof(response)) && *response == 'P') {
				FOCUSER_POSITION_ITEM->number.value = atoi(response + 2);
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			snprintf(command, sizeof(command), "X:%d", (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target);
			if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
				FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			snprintf(command, sizeof(command), "S:%d", SPEED[(int)FOCUSER_SPEED_ITEM->number.target]);
			if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			snprintf(command, sizeof(command), "D:%d", (int)X_FOCUSER_DUTY_CYCLE_ITEM->number.target);
			if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
				X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
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
			focusdreampro_command(device, "H", response, sizeof(response));
			indigo_delete_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
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
	char command[16], response[16];
	snprintf(command, sizeof(command), "S:%d", SPEED[(int)FOCUSER_SPEED_ITEM->number.target]);
	if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target)
		position = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	FOCUSER_POSITION_ITEM->number.target = position;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	snprintf(command, sizeof(command), "%c:%d", FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value ? 'R': 'M', position);
	if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	int position = (int)FOCUSER_POSITION_ITEM->number.value + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -(int)FOCUSER_STEPS_ITEM->number.value : (int)FOCUSER_STEPS_ITEM->number.value);
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target)
		position = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	snprintf(command, sizeof(command), "M:%d", position);
	if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	char response[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (focusdreampro_command(device, "H", response, sizeof(response)) && *response == 'H') {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
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

static void duty_cycle_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	snprintf(command, sizeof(command), "D:%d", (int)X_FOCUSER_DUTY_CYCLE_ITEM->number.target);
	if (focusdreampro_command(device, command, response, sizeof(response)) && *response == *command) {
		X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
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
		if (IS_CONNECTED) {
			FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_speed_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		int position = FOCUSER_POSITION_ITEM->number.value;
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_ITEM->number.value = position;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_FOCUSER_DUTY_CYCLE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FOCUSER_DUTY_CYCLE
		indigo_property_copy_values(X_FOCUSER_DUTY_CYCLE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			X_FOCUSER_DUTY_CYCLE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_FOCUSER_DUTY_CYCLE_PROPERTY, NULL);
			indigo_set_timer(device, 0, duty_cycle_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_FOCUSER_DUTY_CYCLE_PROPERTY);
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
	indigo_release_property(X_FOCUSER_DUTY_CYCLE_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_focusdreampro(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static focusdreampro_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"FocusDreamPro",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "AGadget FocusDreamPro Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(focusdreampro_private_data));
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
