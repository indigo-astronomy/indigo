// Copyright (c) 2020-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_wheel_qhy.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_wheel_qhy.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000A
#define DRIVER_NAME          "indigo_wheel_qhy"
#define DRIVER_LABEL         "QHY CFW Filter Wheel"
#define WHEEL_DEVICE_NAME    "CFW Filter Wheel"
#define PRIVATE_DATA         ((qhy_private_data *)device->private_data)

#pragma mark - Property definitions

#define X_MODEL_PROPERTY               (PRIVATE_DATA->x_model_property)
#define X_MODEL_1_ITEM                 (X_MODEL_PROPERTY->items + 0)
#define X_MODEL_2_ITEM                 (X_MODEL_PROPERTY->items + 1)
#define X_MODEL_3_ITEM                 (X_MODEL_PROPERTY->items + 2)

#define X_MODEL_PROPERTY_NAME          "X_MODEL"
#define X_MODEL_1_ITEM_NAME            "1"
#define X_MODEL_2_ITEM_NAME            "2"
#define X_MODEL_3_ITEM_NAME            "3"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_model_property;
	//+ data
	int current_slot;
	indigo_property *model;
	//- data
} qhy_private_data;

#pragma mark - Low level code

//+ code

static bool qhy_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	return PRIVATE_DATA->handle != NULL;
}

static bool qhy_command(indigo_device *device, char *command, char *reply, int reply_length, int read_timeout) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0) {
			for (int i = 0; i < read_timeout; i++) {
				if (indigo_uni_read(PRIVATE_DATA->handle, reply, reply_length) > 0) {
					return true;
				}
				indigo_usleep(INDIGO_DELAY(1));
			}
		}
	}
	return false;
}

static void qhy_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = qhy_open(device);
		if (connection_result) {
			//+ wheel.on_connect
			if (X_MODEL_3_ITEM->sw.value) {
				char reply[8];
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "QHY CFW3");
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
				bool result = qhy_command(device, "VRS", INFO_DEVICE_FW_REVISION_ITEM->text.value, 8, 1);
				if (!result) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed, retrying...");
					result = qhy_command(device, "VRS", INFO_DEVICE_FW_REVISION_ITEM->text.value, 8, 1);
				}
				if (result) {
					if (qhy_command(device, "MXP", reply, 1, 1)) {
						WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = isdigit(reply[0]) ? reply[0] - '0' : reply[0] - 'A' + 10;
					}
					if (qhy_command(device, "NOW", reply, 1, 1)) {
						WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 	isdigit(reply[0]) ? reply[0] - '0' + 1 : reply[0] - 'A' + 11;
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed, fallback to CFW1");
					indigo_set_switch(X_MODEL_PROPERTY, X_MODEL_1_ITEM, true);
					indigo_update_property(device, X_MODEL_PROPERTY, "Failed to connect to CFW3, trying to fallback to CFW1");
				}
			} else if (X_MODEL_2_ITEM->sw.value) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "QHY CFW2");
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
				if (!qhy_command(device, "0", NULL, 0, 0)) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = 7;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = 1;
			} else if (X_MODEL_1_ITEM->sw.value) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "QHY CFW1");
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
				if (!qhy_command(device, "0", NULL, 0, 0)) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = 7;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = 1;
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			//- wheel.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, CONNECTION_PROPERTY, "Connected to %s on %s", WHEEL_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, CONNECTION_PROPERTY, "Failed to connect to %s on %s", WHEEL_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		qhy_close(device);
		indigo_send_message(device, CONNECTION_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.WHEEL_SLOT.on_change
	int slot = (int)WHEEL_SLOT_ITEM->number.target;
	if (PRIVATE_DATA->current_slot != slot) {
		char command[2] = { '0' + slot - 1, 0 };
		char reply[3] = { 0 };
		if (qhy_command(device, command, reply, 1, 15)) {
			if (X_MODEL_1_ITEM->sw.value) {
				WHEEL_SLOT_PROPERTY->state = (reply[0] == '-' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
			} else if (X_MODEL_2_ITEM->sw.value || X_MODEL_3_ITEM->sw.value) {
				WHEEL_SLOT_PROPERTY->state = (reply[0] == command[0] ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
			}
			PRIVATE_DATA->current_slot = slot;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- wheel.WHEEL_SLOT.on_change
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_x_model_handler(indigo_device *device) {
	X_MODEL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_MODEL_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ wheel.on_attach
		INFO_PROPERTY->count = 6;
		WHEEL_SLOT_ITEM->number.value = 1;
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		X_MODEL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MODEL_PROPERTY_NAME, MAIN_GROUP, "Device type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_MODEL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_MODEL_1_ITEM, X_MODEL_1_ITEM_NAME, "CFW 1", false);
		indigo_init_switch_item(X_MODEL_2_ITEM, X_MODEL_2_ITEM_NAME, "CFW 2", false);
		indigo_init_switch_item(X_MODEL_3_ITEM, X_MODEL_3_ITEM_NAME, "CFW 3", true);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(X_MODEL_PROPERTY);
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, wheel_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MODEL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MODEL_PROPERTY, wheel_x_model_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_MODEL_PROPERTY);
		}
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	indigo_release_property(X_MODEL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Main code

indigo_result indigo_wheel_qhy(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static qhy_private_data *private_data = NULL;
	static indigo_device *wheel = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(qhy_private_data));
			wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
			wheel->private_data = private_data;
			indigo_attach_device(wheel);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(wheel);
			last_action = action;
			if (wheel != NULL) {
				indigo_detach_device(wheel);
				free(wheel);
				wheel = NULL;
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
