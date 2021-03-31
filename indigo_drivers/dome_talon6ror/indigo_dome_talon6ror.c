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

/** INDIGO Talon6 driver
 \file indigo_dome_talon6ror.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_dome_talon6ror"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include <indigo/indigo_io.h>
#include <indigo/indigo_dome_driver.h>

#include "indigo_dome_talon6ror.h"

#define PRIVATE_DATA												((talon6ror_private_data *)device->private_data)

#define X_SENSORS_PROPERTY_NAME								"X_SENSORS"
#define X_SENSORS_PWL_CONDITION_ITEM_NAME			"PWL_CONDITION"
#define X_SENSORS_CLW_CONDITION_ITEM_NAME			"CLW_CONDITION"
#define X_SENSORS_TOUT_CONDITION_ITEM_NAME		"TOUT_CONDITION"
#define X_SENSORS_MAP_SENSOR_ITEM_NAME				"MAP_SENSOR"
#define X_SENSORS_TOP_SENSOR_ITEM_NAME				"TOP_SENSOR"
#define X_SENSORS_TCP_SENSOR_ITEM_NAME				"TCP_SENSOR"
#define X_SENSORS_OPEN_BUTTON_ITEM_NAME				"OPEN_BUTTON"
#define X_SENSORS_CLOSE_BUTTON_ITEM_NAME			"CLOSE_BUTTON"
#define X_SENSORS_STOP_BUTTON_ITEM_NAME				"STOP_BUTTON"
#define X_SENSORS_MGM_INPUT_ITEM_NAME					"MGM_INPUT"
#define X_SENSORS_COM_INPUT_ITEM_NAME					"COM_INPUT"

#define X_SENSORS_PROPERTY    								(PRIVATE_DATA->status_property)
#define X_SENSORS_PWL_CONDITION_ITEM					(X_SENSORS_PROPERTY->items + 0)
#define X_SENSORS_CLW_CONDITION_ITEM					(X_SENSORS_PROPERTY->items + 1)
#define X_SENSORS_TOUT_CONDITION_ITEM					(X_SENSORS_PROPERTY->items + 2)
#define X_SENSORS_MAP_SENSOR_ITEM							(X_SENSORS_PROPERTY->items + 3)
#define X_SENSORS_TOP_SENSOR_ITEM							(X_SENSORS_PROPERTY->items + 4)
#define X_SENSORS_TCP_SENSOR_ITEM							(X_SENSORS_PROPERTY->items + 5)
#define X_SENSORS_OPEN_BUTTON_ITEM						(X_SENSORS_PROPERTY->items + 6)
#define X_SENSORS_CLOSE_BUTTON_ITEM						(X_SENSORS_PROPERTY->items + 7)
#define X_SENSORS_STOP_BUTTON_ITEM						(X_SENSORS_PROPERTY->items + 8)
#define X_SENSORS_MGM_INPUT_ITEM							(X_SENSORS_PROPERTY->items + 9)
#define X_SENSORS_COM_INPUT_ITEM							(X_SENSORS_PROPERTY->items + 10)

#define RESPONSE_LENGTH		32

typedef struct {
	int handle;
	pthread_mutex_t mutex;
	indigo_property *status_property;
	indigo_timer *state_timer;
} talon6ror_private_data;

// -------------------------------------------------------------------------------- serial interface

static bool talon6ror_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
	if (PRIVATE_DATA->handle < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
	return true;
}

static bool talon6ror_command(indigo_device *device, const char *command, char *response) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int result = indigo_printf(PRIVATE_DATA->handle, "&%s#", command);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d <- \"&%s#\" (%s)", PRIVATE_DATA->handle, command, result ? "OK" : strerror(errno));
	if (result) {
		char c, *pnt = response;
		*pnt = 0;
		result = false;
		bool start = false;
		while (pnt - response < RESPONSE_LENGTH) {
			if (indigo_read(PRIVATE_DATA->handle, &c, 1) < 1) {
				if (pnt)
					*pnt = 0;
				break;
			}
			if (c == '&') {
				start = true;
				continue;
			}
			if (!start) {
				continue;
			}
			if (c == '#') {
				if (pnt)
					*pnt = 0;
				result = true;
				break;
			}
			if (pnt)
				*pnt++ = c;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> \"&%s#\" (%s)", PRIVATE_DATA->handle, response, result ? "OK" : strerror(errno));
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return result;
}

static void talon6ror_close(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
	}
}

static void talon6ror_get_status(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "G%", response) && (response[0] == 'G' || response[0] == 'T')) {
		switch (response[1] & 0x70) {
			case 0x00:
				if (DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE || !DOME_SHUTTER_OPENED_ITEM->sw.value) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
					indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
					indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof opened");
				}
				break;
			case 0x10:
				if (DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE || !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
					indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
					indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof closed");
				}
				break;
			case 0x20:
				if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE || !DOME_SHUTTER_OPENED_ITEM->sw.value) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
					indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof opening");
				}
				break;
			case 0x30:
				if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE || !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
					indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof closing");
				}
				break;
			default:
				if (DOME_SHUTTER_PROPERTY->state != INDIGO_ALERT_STATE) {
					DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Error reported");
				}
				break;
		}
		int position = ((response[2] & 0x7F) << 14) | ((response[3] & 0x7F) << 7) | (response[4] & 0x7F);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "position = %d", position);
		double voltage = (((response[5] & 0x7F) << 7) | (response[6] & 0x7F)) * 1024 / 15.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "voltage = %g", voltage);
		bool changed = false;
		if (response[17] & (1 << 0)) {
			if (X_SENSORS_PWL_CONDITION_ITEM->light.value != INDIGO_ALERT_STATE) {
				X_SENSORS_PWL_CONDITION_ITEM->light.value = INDIGO_ALERT_STATE;
				changed = true;
			}
		} else {
			if (X_SENSORS_PWL_CONDITION_ITEM->light.value != INDIGO_IDLE_STATE) {
				X_SENSORS_PWL_CONDITION_ITEM->light.value = INDIGO_IDLE_STATE;
				changed = true;
			}
		}
		if (response[17] & (1 << 1)) {
			if (X_SENSORS_CLW_CONDITION_ITEM->light.value != INDIGO_ALERT_STATE) {
				X_SENSORS_CLW_CONDITION_ITEM->light.value = INDIGO_ALERT_STATE;
				changed = true;
			}
		} else {
			if (X_SENSORS_CLW_CONDITION_ITEM->light.value != INDIGO_IDLE_STATE) {
				X_SENSORS_CLW_CONDITION_ITEM->light.value = INDIGO_IDLE_STATE;
				changed = true;
			}
		}
		if (response[17] & (1 << 2)) {
			if (X_SENSORS_MAP_SENSOR_ITEM->light.value != INDIGO_OK_STATE) {
				X_SENSORS_MAP_SENSOR_ITEM->light.value = INDIGO_OK_STATE;
				changed = true;
			}
		} else {
			if (X_SENSORS_MAP_SENSOR_ITEM->light.value != INDIGO_IDLE_STATE) {
				X_SENSORS_MAP_SENSOR_ITEM->light.value = INDIGO_IDLE_STATE;
				changed = true;
			}
		}
		if (response[17] & (1 << 3)) {
			if (X_SENSORS_TOP_SENSOR_ITEM->light.value != INDIGO_OK_STATE) {
				X_SENSORS_TOP_SENSOR_ITEM->light.value = INDIGO_OK_STATE;
				changed = true;
			}
		} else {
			if (X_SENSORS_TOP_SENSOR_ITEM->light.value != INDIGO_IDLE_STATE) {
				X_SENSORS_TOP_SENSOR_ITEM->light.value = INDIGO_IDLE_STATE;
				changed = true;
			}
		}
		if (response[17] & (1 << 4)) {
			if (X_SENSORS_TCP_SENSOR_ITEM->light.value != INDIGO_OK_STATE) {
				X_SENSORS_TCP_SENSOR_ITEM->light.value = INDIGO_OK_STATE;
				changed = true;
			}
		} else {
			if (X_SENSORS_TCP_SENSOR_ITEM->light.value != INDIGO_IDLE_STATE) {
				X_SENSORS_TCP_SENSOR_ITEM->light.value = INDIGO_IDLE_STATE;
				changed = true;
			}
		}
		if (changed)
			indigo_update_property(device, X_SENSORS_PROPERTY, NULL);
		// TBD parse state
	}
	indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->state_timer);
}

// -------------------------------------------------------------------------------- async handlers

static void dome_connect_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!talon6ror_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (talon6ror_command(device, "V%", response) && response[0] == 'V') {
				strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 1, 7);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			}
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (talon6ror_command(device, "p%", response)) {
				// TBD parse configuration
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			}
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, X_SENSORS_PROPERTY, NULL);
			// TBD
			indigo_set_timer(device, 0, talon6ror_get_status, &PRIVATE_DATA->state_timer);
		} else {
			talon6ror_close(device);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device,  &PRIVATE_DATA->state_timer);
		if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		talon6ror_close(device);
		indigo_delete_property(device, X_SENSORS_PROPERTY, NULL);
		// TBD
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_open_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "O%", response)) {
		while (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_usleep(500000);
		}
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_close_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "C%", response)) {
		while (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_usleep(500000);
		}
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_abort_handler(indigo_device *device) {
	char response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "S%", response))
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	else
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO dome device implementation

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Talon6");
		// -------------------------------------------------------------------------------- standard properties
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Talon6 ROR");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
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
		// -------------------------------------------------------------------------------- X_SENSORS
		X_SENSORS_PROPERTY = indigo_init_light_property(NULL, device->name, X_SENSORS_PROPERTY_NAME, DOME_MAIN_GROUP, "Sensors", INDIGO_OK_STATE, 11);
		if (X_SENSORS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(X_SENSORS_PWL_CONDITION_ITEM, X_SENSORS_PWL_CONDITION_ITEM_NAME, "Power lost / Power fail", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_CLW_CONDITION_ITEM, X_SENSORS_CLW_CONDITION_ITEM_NAME, "Cloudwatcher relay closed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_TOUT_CONDITION_ITEM, X_SENSORS_TOUT_CONDITION_ITEM_NAME, "Closing countdown active", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_MAP_SENSOR_ITEM, X_SENSORS_MAP_SENSOR_ITEM_NAME, "Mount at park position", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_TOP_SENSOR_ITEM, X_SENSORS_TOP_SENSOR_ITEM_NAME, "Roof open", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_TCP_SENSOR_ITEM, X_SENSORS_TCP_SENSOR_ITEM_NAME, "Roof closed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_OPEN_BUTTON_ITEM, X_SENSORS_OPEN_BUTTON_ITEM_NAME, "Open button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_CLOSE_BUTTON_ITEM, X_SENSORS_CLOSE_BUTTON_ITEM_NAME, "Close button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_STOP_BUTTON_ITEM, X_SENSORS_STOP_BUTTON_ITEM_NAME, "Stop button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_MGM_INPUT_ITEM, X_SENSORS_MGM_INPUT_ITEM_NAME, "Closed by management input active", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_COM_INPUT_ITEM, X_SENSORS_COM_INPUT_ITEM_NAME, "Direct command input active", INDIGO_IDLE_STATE);
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_SENSORS_PROPERTY, property))
			indigo_define_property(device, X_SENSORS_PROPERTY, NULL);
		// TBD
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
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
			if (DOME_SHUTTER_OPENED_ITEM->sw.value && DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, dome_open_handler, NULL);
			} else if (DOME_SHUTTER_CLOSED_ITEM->sw.value && DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, dome_close_handler, NULL);
			}
		}
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
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
	indigo_release_property(X_SENSORS_PROPERTY);
	// TBD
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static talon6ror_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_talon6ror(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_SIMULATOR_NAME,
		dome_attach,
		dome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Talon6 ROR", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(talon6ror_private_data));
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
