// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO Interactive Astronomy SkyRoof driver
 \file indigo_dome_skyroof.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME	"indigo_dome_skyroof"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include <indigo/indigo_io.h>
#include <indigo/indigo_dome_driver.h>

#include "indigo_dome_skyroof.h"

#define PRIVATE_DATA												((skyroof_private_data *)device->private_data)

#define X_MOUNT_PARK_STATUS_PROPERTY_NAME		"X_MOUNT_PARK_STATUS"
#define X_MOUNT_PARK_STATUS_ITEM_NAME				"STATUS"

#define X_MOUNT_PARK_STATUS_PROPERTY    		(PRIVATE_DATA->mount_park_status_property)
#define X_MOUNT_PARK_STATUS_ITEM         		(X_MOUNT_PARK_STATUS_PROPERTY->items + 0)

#define X_HEATER_CONTROL_PROPERTY_NAME			"X_HEATER_CONTROL"
#define X_HEATER_CONTROL_ON_ITEM_NAME				"ON"
#define X_HEATER_CONTROL_OFF_ITEM_NAME			"OFF"

#define X_HEATER_CONTROL_PROPERTY    				(PRIVATE_DATA->heater_control_property)
#define X_HEATER_CONTROL_OFF_ITEM         	(X_HEATER_CONTROL_PROPERTY->items + 0)
#define X_HEATER_CONTROL_ON_ITEM         		(X_HEATER_CONTROL_PROPERTY->items + 1)

#define RESPONSE_LENGTH											16

typedef struct {
	int handle;
	bool closed;
	pthread_mutex_t mutex;
	indigo_property *mount_park_status_property;
	indigo_property *heater_control_property;
} skyroof_private_data;

// -------------------------------------------------------------------------------- serial interface

static bool skyroof_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
	if (PRIVATE_DATA->handle < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
	return true;
}

static bool skyroof_command(indigo_device *device, const char *command, char *response) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int result = indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	result |= indigo_write(PRIVATE_DATA->handle, "\r", 1);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d <- \"%s\" (%s)", PRIVATE_DATA->handle, command, result ? "OK" : strerror(errno));
	if (result && response) {
		char c, *pnt = response;
		*pnt = 0;
		result = false;
		while (pnt - response < RESPONSE_LENGTH) {
			if (indigo_read(PRIVATE_DATA->handle, &c, 1) < 1) {
				*pnt = 0;
				break;
			}
			if (c == '\r') {
				*pnt = 0;
				result = true;
				break;
			}
			*pnt++ = c;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> \"%s\" (%s)", PRIVATE_DATA->handle, response, result ? "OK" : strerror(errno));
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return result;
}

static void skyroof_close(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
	}
}

// -------------------------------------------------------------------------------- async handlers

static void dome_connect_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!skyroof_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (skyroof_command(device, "Status#", response)) {
				if (!strcmp(response, "RoofOpen#")) {
					indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
					DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->closed = false;
				} else if (!strcmp(response, "RoofClosed#")) {
					indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
					DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->closed = true;
				} else if (!strcmp(response, "Safety#")) {
					DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
					DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
					PRIVATE_DATA->closed = false;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			}
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (skyroof_command(device, "Parkstatus#", response)) {
				if (!strcmp(response, "0#")) {
					X_MOUNT_PARK_STATUS_ITEM->light.value = INDIGO_OK_STATE;
					X_MOUNT_PARK_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				} else if (!strcmp(response, "1#")) {
					X_MOUNT_PARK_STATUS_ITEM->light.value = INDIGO_IDLE_STATE;
					X_MOUNT_PARK_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			}
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, X_MOUNT_PARK_STATUS_PROPERTY, NULL);
			indigo_set_switch(X_HEATER_CONTROL_PROPERTY, X_HEATER_CONTROL_OFF_ITEM, true);
			indigo_define_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
		} else {
			skyroof_close(device);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		skyroof_close(device);
		indigo_delete_property(device, X_MOUNT_PARK_STATUS_PROPERTY, NULL);
		indigo_delete_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_open_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	PRIVATE_DATA->closed = false;
	if (skyroof_command(device, "Open#", response) && !strcmp(response, "0#")) {
		while (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (skyroof_command(device, "Status#", response)) {
				if (!strcmp(response, "RoofOpen#")) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
					break;
				} else if (!strcmp(response, "RoofClosed#") || !strcmp(response, "Safety#")) {
					indigo_usleep(500000);
					continue;
				}
			}
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (DOME_SHUTTER_PROPERTY->state == INDIGO_ALERT_STATE)
		DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_close_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	PRIVATE_DATA->closed = false;
	if (skyroof_command(device, "Close#", response) && !strcmp(response, "0#")) {
		while (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (skyroof_command(device, "Status#", response)) {
				if (!strcmp(response, "RoofClosed#")) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->closed = true;
					break;
				} else if (!strcmp(response, "RoofOpen#") || !strcmp(response, "Safety#")) {
					indigo_usleep(500000);
					continue;
				}
			}
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (DOME_SHUTTER_PROPERTY->state == INDIGO_ALERT_STATE)
		DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_abort_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (skyroof_command(device, "Stop#", response) && !strcmp(response, "0#"))
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	else
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void heater_handler(indigo_device *device) {
	if (skyroof_command(device, X_HEATER_CONTROL_ON_ITEM->sw.value ? "HeaterOn#" : "HeaterOff#", NULL))
		X_HEATER_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	else
		X_HEATER_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO dome device implementation

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Interactive Astronomy SkyRoof");
		// -------------------------------------------------------------------------------- standard properties
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_EQUATORIAL_COORDINATES_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
		DOME_SHUTTER_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
		indigo_copy_value(DOME_SHUTTER_PROPERTY->label, "Roof state");
		indigo_copy_value(DOME_SHUTTER_OPENED_ITEM->label, "Roof opened");
		indigo_copy_value(DOME_SHUTTER_CLOSED_ITEM->label, "Roof closed");
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_MOUNT_PARKED
		X_MOUNT_PARK_STATUS_PROPERTY = indigo_init_light_property(NULL, device->name, X_MOUNT_PARK_STATUS_PROPERTY_NAME, DOME_MAIN_GROUP, "Mount park status", INDIGO_OK_STATE, 1);
		if (X_MOUNT_PARK_STATUS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(X_MOUNT_PARK_STATUS_ITEM, X_MOUNT_PARK_STATUS_ITEM_NAME, "Parked", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------------- X_MOUNT_PARKED
		X_HEATER_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_HEATER_CONTROL_PROPERTY_NAME, DOME_MAIN_GROUP, "Heater control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_HEATER_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_HEATER_CONTROL_OFF_ITEM, X_HEATER_CONTROL_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_HEATER_CONTROL_ON_ITEM, X_HEATER_CONTROL_ON_ITEM_NAME, "On", false);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_MOUNT_PARK_STATUS_PROPERTY, property))
			indigo_define_property(device, X_MOUNT_PARK_STATUS_PROPERTY, NULL);
		if (indigo_property_match(X_HEATER_CONTROL_PROPERTY, property))
			indigo_define_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, dome_connect_handler, NULL);
		return INDIGO_OK;

	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, dome_abort_handler, NULL);
		} else {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
			if (DOME_SHUTTER_OPENED_ITEM->sw.value && (PRIVATE_DATA->closed || DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE)) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, dome_open_handler, NULL);
			} else if (DOME_SHUTTER_CLOSED_ITEM->sw.value && (!PRIVATE_DATA->closed || DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE)) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, dome_close_handler, NULL);
			}
		}
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_HEATER_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_HEATER_CONTROL
		indigo_property_copy_values(X_HEATER_CONTROL_PROPERTY, property, false);
		X_HEATER_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
		indigo_set_timer(device, 0, heater_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connect_handler(device);
	}
	indigo_release_property(X_MOUNT_PARK_STATUS_PROPERTY);
	indigo_release_property(X_HEATER_CONTROL_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static skyroof_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_skyroof(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		"SkyRoof",
		dome_attach,
		dome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Interactive Astronomy SkyRoof", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(skyroof_private_data));
			dome = indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				free(dome);
				dome = NULL;
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
