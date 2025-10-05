// Copyright (c) 2024-2025 CloudMakers, s. r. o.
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
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO iOptron iEAF focuser driver
 \file indigo_focuser_ioptron.c
 */

#define DRIVER_VERSION 0x03000003
#define DRIVER_NAME "indigo_focuser_ioptron"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_ioptron.h"

#define PRIVATE_DATA													((ioptron_private_data *)device->private_data)

#define X_FOCUSER_ZERO_SYNC_PROPERTY					(PRIVATE_DATA->zero_sync_property)
#define X_FOCUSER_ZERO_SYNC_ITEM							(X_FOCUSER_ZERO_SYNC_PROPERTY->items+0)


typedef struct {
	indigo_uni_handle *handle;
	int reversed;
	indigo_property *zero_sync_property;
	indigo_timer *timer;
	pthread_mutex_t port_mutex;
	char response[16];
} ioptron_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool ioptron_command(indigo_device *device, char *command, ...) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result >= 0;
}

static bool ioptron_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle >= 0) {
		int pos, model;
		indigo_sleep(2);
		if (ioptron_command(device, ":MountInfo#") && indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "#", "#", INDIGO_DELAY(1)) > 0 && sscanf(PRIVATE_DATA->response, "%6d%2d", &pos, &model) == 2 && (model == 2 || model == 3)) {
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = pos;
		} else {
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	return false;
}

static void ioptron_close(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- ZERO_SYNC
		X_FOCUSER_ZERO_SYNC_PROPERTY = indigo_init_switch_property(NULL, device->name, "ZERO_SYNC", FOCUSER_MAIN_GROUP, "Sync position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FOCUSER_ZERO_SYNC_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_ZERO_SYNC_ITEM, "SYNC", "Sync to 0", false);

		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
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
		FOCUSER_STEPS_ITEM->number.max = 99999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 99999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_ZERO_SYNC_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int pos, state, temp;
	if (ioptron_command(device, ":FI#") && indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "#", "#", INDIGO_DELAY(1)) > 0 && sscanf(PRIVATE_DATA->response, "%7d%1d%5d%1d", &pos, &state, &temp, &PRIVATE_DATA->reversed) == 4) {
		if (FOCUSER_POSITION_ITEM->number.value != pos || FOCUSER_POSITION_PROPERTY->state != (state ? INDIGO_OK_STATE : INDIGO_BUSY_STATE)) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		if (pos == 0 && X_FOCUSER_ZERO_SYNC_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_FOCUSER_ZERO_SYNC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
		}
		if (state == 0 && FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		}
		double temperature = temp / 100 - 273.15;
		if (fabs(temperature - FOCUSER_TEMPERATURE_ITEM->number.value) > 0.1) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
		if (FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value != (PRIVATE_DATA->reversed == 1)) {
			indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reversed == 1 ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		}
	} else {
		indigo_send_message(device, "Can't read focuser state");
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer);
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (ioptron_open(device)) {
			indigo_define_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle != NULL) {
			indigo_delete_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			ioptron_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	if (ioptron_command(device, ":FM%7d#", (int)FOCUSER_POSITION_ITEM->number.target)) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_POSITION_ITEM->number.target += (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -FOCUSER_STEPS_ITEM->number.value : FOCUSER_STEPS_ITEM->number.value);
	if (FOCUSER_POSITION_ITEM->number.target < 0) {
		FOCUSER_POSITION_ITEM->number.target = 0;
	}
	if (FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.max;
	}
	focuser_position_handler(device);
}

static void focuser_reverse_handler(indigo_device *device) {
	ioptron_command(device, ":FR#");
}

static void focuser_zero_sync_handler(indigo_device *device) {
	ioptron_command(device, ":FZ#");
}

static void focuser_abort_handler(indigo_device *device) {
	ioptron_command(device, ":FQ#");
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
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
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
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
				indigo_set_timer(device, 0, focuser_abort_handler, NULL);
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		if (FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value != (PRIVATE_DATA->reversed == 1)) {
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_reverse_handler, NULL);
		} else {
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_ZERO_SYNC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FOCUSER_ZERO_SYNC
		indigo_property_copy_values(X_FOCUSER_ZERO_SYNC_PROPERTY, property, false);
		X_FOCUSER_ZERO_SYNC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_zero_sync_handler, NULL);
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
	indigo_release_property(X_FOCUSER_ZERO_SYNC_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->port_mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_ioptron(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static ioptron_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"iOptron iEAF",
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "iOptron iEAF Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(ioptron_private_data));
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
