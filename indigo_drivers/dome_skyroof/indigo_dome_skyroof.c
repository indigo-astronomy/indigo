// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_dome_skyroof.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_skyroof.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_dome_skyroof"
#define DRIVER_LABEL         "Interactive Astronomy SkyRoof"
#define DOME_DEVICE_NAME     "SkyRoof"
#define PRIVATE_DATA         ((skyroof_private_data *)device->private_data)

//+ define

// a roof that travels longer than this without arriving is reported as failed
#define SKYROOF_MOTION_TIMEOUT 300

//- define

#pragma mark - Property definitions

#define X_MOUNT_PARK_STATUS_PROPERTY      (PRIVATE_DATA->x_mount_park_status_property)
#define X_MOUNT_PARK_STATUS_ITEM          (X_MOUNT_PARK_STATUS_PROPERTY->items + 0)

#define X_MOUNT_PARK_STATUS_PROPERTY_NAME "X_MOUNT_PARK_STATUS"
#define X_MOUNT_PARK_STATUS_ITEM_NAME     "STATUS"

#define X_HEATER_CONTROL_PROPERTY      (PRIVATE_DATA->x_heater_control_property)
#define X_HEATER_CONTROL_OFF_ITEM      (X_HEATER_CONTROL_PROPERTY->items + 0)
#define X_HEATER_CONTROL_ON_ITEM       (X_HEATER_CONTROL_PROPERTY->items + 1)

#define X_HEATER_CONTROL_PROPERTY_NAME "X_HEATER_CONTROL"
#define X_HEATER_CONTROL_OFF_ITEM_NAME "OFF"
#define X_HEATER_CONTROL_ON_ITEM_NAME  "ON"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_mount_park_status_property;
	indigo_property *x_heater_control_property;
	//+ data
	char response[128];
	double motion_started;
	//- data
} skyroof_private_data;

#pragma mark - Low level code

//+ code

static void dome_shutter_handler(indigo_device *device);

// every command is answered by one CR terminated line, except the heater commands, which have no answer
static bool skyroof_command(indigo_device *device, char *command, bool reply) {
	PRIVATE_DATA->response[0] = 0;
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_printf(PRIVATE_DATA->handle, "%s\r", command) <= 0) {
		return false;
	}
	return !reply || indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r", INDIGO_DELAY(1)) > 0;
}

// "RoofOpen#" and "RoofClosed#" end a motion, "Safety#" is reported while the roof travels
static bool skyroof_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (skyroof_command(device, "Status#", true)) {
			if (!strcmp(PRIVATE_DATA->response, "RoofOpen#")) {
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			} else if (!strcmp(PRIVATE_DATA->response, "RoofClosed#")) {
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			} else if (!strcmp(PRIVATE_DATA->response, "Safety#")) {
				DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed, Status# answered '%s'", PRIVATE_DATA->response);
				indigo_uni_close(&PRIVATE_DATA->handle);
				return false;
			}
			// "0#" and "1#" are the two states of the mount park sensor
			if (skyroof_command(device, "Parkstatus#", true) && (!strcmp(PRIVATE_DATA->response, "0#") || !strcmp(PRIVATE_DATA->response, "1#"))) {
				X_MOUNT_PARK_STATUS_ITEM->light.value = PRIVATE_DATA->response[0] == '0' ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
				X_MOUNT_PARK_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				return true;
			}
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed, Parkstatus# answered '%s'", PRIVATE_DATA->response);
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void skyroof_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ dome.code

static void skyroof_shutter_failed(indigo_device *device, const char *message) {
	DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
	INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, message);
}

static void dome_shutter_finalizer(indigo_device *device) {
	if (DOME_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		skyroof_shutter_failed(device, NULL);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_ABORT_MOTION_PROPERTY, INDIGO_OK_STATE, NULL);
	} else if (!skyroof_command(device, "Status#", true)) {
		skyroof_shutter_failed(device, "Roof does not respond");
	} else if (DOME_SHUTTER_OPENED_ITEM->sw.value && !strcmp(PRIVATE_DATA->response, "RoofOpen#")) {
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
	} else if (DOME_SHUTTER_CLOSED_ITEM->sw.value && !strcmp(PRIVATE_DATA->response, "RoofClosed#")) {
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
	} else if (strcmp(PRIVATE_DATA->response, "RoofOpen#") && strcmp(PRIVATE_DATA->response, "RoofClosed#") && strcmp(PRIVATE_DATA->response, "Safety#")) {
		skyroof_shutter_failed(device, "Unexpected roof status");
	} else if (indigo_monotonic_time() - PRIVATE_DATA->motion_started > SKYROOF_MOTION_TIMEOUT) {
		skyroof_shutter_failed(device, "Roof did not arrive");
	} else {
		indigo_execute_handler_in(device, 0.5, dome_shutter_finalizer);
	}
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = skyroof_open(device);
		if (connection_result) {
			indigo_define_property(device, X_MOUNT_PARK_STATUS_PROPERTY, NULL);
			indigo_define_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			DOME_SHUTTER_PROPERTY,
			DOME_ABORT_MOTION_PROPERTY,
			X_MOUNT_PARK_STATUS_PROPERTY,
			X_HEATER_CONTROL_PROPERTY,
			DOME_SPEED_PROPERTY,
			DOME_DIRECTION_PROPERTY,
			DOME_HORIZONTAL_COORDINATES_PROPERTY,
			DOME_STEPS_PROPERTY,
			DOME_PARK_PROPERTY,
			DOME_DIMENSION_PROPERTY,
			DOME_SLAVING_PARAMETERS_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_MOUNT_PARK_STATUS_PROPERTY, NULL);
		indigo_delete_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
		skyroof_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	bool open = indigo_get_switch_target(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM_NAME);
	if (skyroof_command(device, open ? "Open#" : "Close#", true) && !strcmp(PRIVATE_DATA->response, "0#")) {
		indigo_apply_switch_targets(DOME_SHUTTER_PROPERTY);
		PRIVATE_DATA->motion_started = indigo_monotonic_time();
		indigo_execute_handler_in(device, 0.5, dome_shutter_finalizer);
	} else {
		skyroof_shutter_failed(device, NULL);
	}
	//- dome.DOME_SHUTTER.on_change
}

static void dome_abort_motion_handler(indigo_device *device) {
	//+ dome.DOME_ABORT_MOTION.on_change
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, dome_shutter_handler);
		if (skyroof_command(device, "Stop#", true) && !strcmp(PRIVATE_DATA->response, "0#")) {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_cancel_pending_handler(device, dome_shutter_finalizer);
			indigo_execute_handler_in(device, 0, dome_shutter_finalizer);
		} else {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
	}
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
	//- dome.DOME_ABORT_MOTION.on_change
}

static void dome_x_heater_control_handler(indigo_device *device) {
	//+ dome.X_HEATER_CONTROL.on_change
	X_HEATER_CONTROL_PROPERTY->state = skyroof_command(device, X_HEATER_CONTROL_ON_ITEM->sw.value ? "HeaterOn#" : "HeaterOff#", false) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- dome.X_HEATER_CONTROL.on_change
	indigo_update_property(device, X_HEATER_CONTROL_PROPERTY, NULL);
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
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Interactive Astronomy SkyRoof");
		//- dome.on_attach
		DOME_SHUTTER_PROPERTY->hidden = false;
		//+ dome.DOME_SHUTTER.on_attach
		DOME_SHUTTER_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
		INDIGO_COPY_VALUE(DOME_SHUTTER_PROPERTY->label, "Roof state");
		INDIGO_COPY_VALUE(DOME_SHUTTER_OPENED_ITEM->label, "Roof opened");
		INDIGO_COPY_VALUE(DOME_SHUTTER_CLOSED_ITEM->label, "Roof closed");
		//- dome.DOME_SHUTTER.on_attach
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		X_MOUNT_PARK_STATUS_PROPERTY = indigo_init_light_property(NULL, device->name, X_MOUNT_PARK_STATUS_PROPERTY_NAME, DOME_MAIN_GROUP, "Mount park status", INDIGO_OK_STATE, 1);
		if (X_MOUNT_PARK_STATUS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_MOUNT_PARK_STATUS_ITEM, X_MOUNT_PARK_STATUS_ITEM_NAME, "Parked", INDIGO_IDLE_STATE);
		X_HEATER_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_HEATER_CONTROL_PROPERTY_NAME, DOME_MAIN_GROUP, "Heater control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_HEATER_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_HEATER_CONTROL_OFF_ITEM, X_HEATER_CONTROL_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_HEATER_CONTROL_ON_ITEM, X_HEATER_CONTROL_ON_ITEM_NAME, "On", false);
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MOUNT_PARK_STATUS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_HEATER_CONTROL_PROPERTY);
	}
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(dome_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_HEATER_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_HEATER_CONTROL_PROPERTY, dome_x_heater_control_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(X_MOUNT_PARK_STATUS_PROPERTY);
	indigo_release_property(X_HEATER_CONTROL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_skyroof(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static skyroof_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (skyroof_private_data *)indigo_safe_malloc(sizeof(skyroof_private_data));
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
