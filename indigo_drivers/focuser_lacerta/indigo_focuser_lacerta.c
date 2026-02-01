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

/** INDIGO LACERTA Motorfocus focuser driver
 \file indigo_focuser_lacerta.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_lacerta"

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

#include "indigo_focuser_lacerta.h"

#define PRIVATE_DATA								((lacerta_private_data *)device->private_data)

typedef enum { UNKNOWN = 0, FMC, MFOC } model_type;

typedef struct {
	int handle;
	indigo_timer *timer;
	pthread_mutex_t port_mutex;
	indigo_property *type_property[2];
	model_type model;
} lacerta_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool lacerta_command(indigo_device *device, char *command, char *response, int max, char wait_for) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	int count = 100;
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (response != NULL) {
		while (count > 0) {
			int index = 0;
			*response = 0;
			while (index < max) {
				tv.tv_usec = 500000;
				tv.tv_sec = 0;
				fd_set readout;
				FD_ZERO(&readout);
				FD_SET(PRIVATE_DATA->handle, &readout);
				long result = select(PRIVATE_DATA->handle + 1, &readout, NULL, NULL, &tv);
				if (result <= 0) {
					break;
				}
				result = read(PRIVATE_DATA->handle, &c, 1);
				if (result < 1) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					return false;
				}
				if (c == '\r') {
					break;
				}
				response[index++] = c;
			}
			response[index] = 0;
			if (*response == wait_for) {
				break;
			}
			count--;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command '%s' -> '%s'", command, response != NULL ? response : "");
	return true;
}

static bool lacerta_open(indigo_device *device) {
	char response[32];
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle > 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
	}
	if (PRIVATE_DATA->handle > 0) {
		if (lacerta_command(device, ": i #", response, sizeof(response), 'i')) {
			if (!strcmp(response + 2, "FMC")) {
				PRIVATE_DATA->model = FMC;
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, response + 2);
			} else if (!strcmp(response + 2, "MFOC")) {
				PRIVATE_DATA->model = MFOC;
			} else {
				PRIVATE_DATA->model = UNKNOWN;
			}
			strcpy(INFO_DEVICE_MODEL_ITEM->text.value, response + 2);
			if (lacerta_command(device, ": v #", response, sizeof(response), 'v')) {
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 1);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
		} else {
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
	}
	if (PRIVATE_DATA->handle > 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Detected %s %s", INFO_DEVICE_MODEL_ITEM->text.value, INFO_DEVICE_FW_REVISION_ITEM->text.value);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void lacerta_close(indigo_device *device) {
	close(PRIVATE_DATA->handle);
	PRIVATE_DATA->handle = 0;
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

static void focuser_timer_callback(indigo_device *device) {
	char response[32];
	if (!IS_CONNECTED) {
		return;
	}
	if (lacerta_command(device, ": t #", response, sizeof(response), 't')) {
		double temp = indigo_atod(response + 1);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	if (lacerta_command(device, ": q #", response, sizeof(response), 'p')) {
		double pos = indigo_atod(response + 1);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = FOCUSER_POSITION_ITEM->number.target == FOCUSER_POSITION_ITEM->number.value ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->timer);
}

static void focuser_connection_handler(indigo_device *device) {
	char response[32];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (lacerta_open(device)) {
			if (lacerta_command(device, ": r #", response, sizeof(response), 'r')) {
				if (atoi(response + 2) == 1) {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, FOCUSER_REVERSE_MOTION_ENABLED_ITEM, true);
				} else {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
			}
			if (lacerta_command(device, ": q #", response, sizeof(response), 'p')) {
				FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value = atoi(response + 2);
			}
			if (lacerta_command(device, ": b #", response, sizeof(response), 'b')) {
				FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = atoi(response + 2);
			}
			if (lacerta_command(device, ": g #", response, sizeof(response), 'g')) {
				FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
				FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = atoi(response + 2);
			}
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		lacerta_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	char command[32], response[32];
	int position = FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		snprintf(command, sizeof(command), ": M %d#", position);
		if (lacerta_command(device, command, NULL, 0, 0)) {
			FOCUSER_POSITION_ITEM->number.target = position;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	} else {
		snprintf(command, sizeof(command), ": P %d#", position);
		if (lacerta_command(device, command, response, sizeof(response), 'p')) {
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value = atoi(response + 2);
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
}

static void focuser_steps_handler(indigo_device *device) {
	char command[32];
	int position = FOCUSER_POSITION_ITEM->number.value + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? (int)-FOCUSER_STEPS_ITEM->number.value : (int)FOCUSER_STEPS_ITEM->number.value);
	if (position < 0) {
		position = 0;
	} else if (position > FOCUSER_POSITION_ITEM->number.max) {
		position = FOCUSER_POSITION_ITEM->number.max;
	}
	snprintf(command, sizeof(command), ": M %d#", position);
	if (lacerta_command(device, command, NULL, 0, 0)) {
		FOCUSER_POSITION_ITEM->number.target = position;
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

static void focuser_abort_handler(indigo_device *device) {
	char response[32];
	if (lacerta_command(device, ": H #", response, sizeof(response), 'H') && response[2] == '1') {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	}
}

static void focuser_limits_handler(indigo_device *device) {
	char command[32], response[32];
	snprintf(command, sizeof(command), ": G %d#", (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target);
	if (lacerta_command(device, command, response, sizeof(response), 'g')) {
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = atoi(response + 2);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	} else {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	}
}

static void focuser_backlash_handler(indigo_device *device) {
	char command[32], response[32];
	snprintf(command, sizeof(command), ": B %d#", (int)FOCUSER_BACKLASH_ITEM->number.target);
	if (lacerta_command(device, command, response, sizeof(response), 'b')) {
		FOCUSER_BACKLASH_ITEM->number.value = atoi(response + 2);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	}
}

static void focuser_reverse_handler(indigo_device *device) {
	char response[32];
	if (lacerta_command(device, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? ": R 1#" : ": R 0#" , response, sizeof(response), 'r')) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
	} else {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
	}
}


// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
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
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 250000;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 250000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 300;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, client, property);
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
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		int value = FOCUSER_POSITION_ITEM->number.value;
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_ITEM->number.value = value;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_limits_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_backlash_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_reverse_handler, NULL);
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
	pthread_mutex_destroy(&PRIVATE_DATA->port_mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_lacerta(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static lacerta_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"LACERTA Motorfocus",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "LACERTA Motorfocus Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(lacerta_private_data));
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
