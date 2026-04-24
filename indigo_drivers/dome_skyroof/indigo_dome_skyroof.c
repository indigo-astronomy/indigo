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
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_skyroof.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_dome_skyroof"
#define DRIVER_LABEL         "Interactive Astronomy SkyRoof"
#define DOME_DEVICE_NAME     "SkyRoof"
#define PRIVATE_DATA         ((skyroof_private_data *)device->private_data)

#pragma mark - Property definitions

#define HEATER_CONTROL_PROPERTY        (PRIVATE_DATA->heater_control_property)
#define HEATER_CONTROL_OFF_ITEM        (HEATER_CONTROL_PROPERTY->items + 0)
#define HEATER_CONTROL_ON_ITEM         (HEATER_CONTROL_PROPERTY->items + 1)

#define HEATER_CONTROL_PROPERTY_NAME   "HEATER_CONTROL"
#define HEATER_CONTROL_OFF_ITEM_NAME   "OFF"
#define HEATER_CONTROL_ON_ITEM_NAME    "ON"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *heater_control_property;
	//+ data
	char response[128];
	//- data
} skyroof_private_data;

#pragma mark - Low level code

//+ code

static bool skyroof_write(indigo_device *device, char *command)	{
	return indigo_uni_printf(PRIVATE_DATA->handle, "%s\r", command) > 0;
}

static bool	skyroof_read(indigo_device *device) {
	return indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\r", "\r", INDIGO_DELAY(1)) > 0;
}

static bool skyroof_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (skyroof_write(device, "Status#") && skyroof_read(device)) {
			if (!strcmp(PRIVATE_DATA->response, "RoofOpen#")) {
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			} else if (!strcmp(PRIVATE_DATA->response, "RoofClosed#")) {
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			return true;
		} else {
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
	}
	return false;
}

static void skyroof_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ dome.code

static void dome_shutter_finalizer(indigo_device *device) {
	if (DOME_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, NULL);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_ABORT_MOTION_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		if (skyroof_write(device, "Status#") && skyroof_read(device)) {
			if (DOME_SHUTTER_OPENED_ITEM->sw.value && !strcmp(PRIVATE_DATA->response, "RoofOpen#")) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			} else if (DOME_SHUTTER_CLOSED_ITEM->sw.value && !strcmp(PRIVATE_DATA->response, "RoofClosed#")) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				indigo_execute_handler_in(device, 0.5, dome_shutter_finalizer);
			}
		}
	}
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = skyroof_open(device);
		if (connection_result) {
			indigo_define_property(device, HEATER_CONTROL_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, HEATER_CONTROL_PROPERTY, NULL);
		skyroof_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	if (skyroof_write(device, DOME_SHUTTER_OPENED_ITEM->sw.value ? "Open#" : "Close#") && skyroof_read(device) && !strcmp(PRIVATE_DATA->response, "0#")) {
		indigo_execute_handler_in(device, 0.5, dome_shutter_finalizer);
	} else {
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	//- dome.DOME_SHUTTER.on_change
}

static void dome_abort_motion_handler(indigo_device *device) {
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (skyroof_write(device, "Stop#") && skyroof_read(device) && !strcmp(PRIVATE_DATA->response, "0#")) {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void dome_heater_control_handler(indigo_device *device) {
	HEATER_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.HEATER_CONTROL.on_change
	if (!skyroof_write(device, HEATER_CONTROL_ON_ITEM->sw.value ? "HeaterOn#" : "HeaterOff#")) {
		HEATER_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- dome.HEATER_CONTROL.on_change
	indigo_update_property(device, HEATER_CONTROL_PROPERTY, NULL);
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DOME_SHUTTER_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		HEATER_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, HEATER_CONTROL_PROPERTY_NAME, DOME_MAIN_GROUP, "Heater control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (HEATER_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(HEATER_CONTROL_OFF_ITEM, HEATER_CONTROL_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(HEATER_CONTROL_ON_ITEM, HEATER_CONTROL_ON_ITEM_NAME, "On", false);
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_EQUATORIAL_COORDINATES_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(HEATER_CONTROL_PROPERTY);
	}
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, dome_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(HEATER_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(HEATER_CONTROL_PROPERTY, dome_heater_control_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(HEATER_CONTROL_PROPERTY);
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
