// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_wheel_xagyl.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_wheel_xagyl.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_wheel_xagyl"
#define DRIVER_LABEL         "Xagyl Filter Wheel"
#define WHEEL_DEVICE_NAME    "Xagyl Filter Wheel"
#define PRIVATE_DATA         ((xagyl_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	int slot;
	//- data
} xagyl_private_data;

#pragma mark - Low level code

//+ code

static bool xagyl_query(indigo_device *device) {
	if (indigo_uni_printf(PRIVATE_DATA->handle, "I2") > 0 && indigo_uni_scanf_line(PRIVATE_DATA->handle, "P%d", &PRIVATE_DATA->slot) == 1) {
		return true;
	}
	return false;
}

static bool xagyl_goto(indigo_device *device, int slot) {
	return indigo_uni_printf(PRIVATE_DATA->handle, "G%d", slot) > 0;
}

static bool xagyl_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		char buffer[128];
		if (indigo_uni_printf(PRIVATE_DATA->handle, "I0") > 0 && indigo_uni_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer)) > 0) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, buffer);
			if (indigo_uni_printf(PRIVATE_DATA->handle, "I1") > 0 && indigo_uni_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer)) > 0) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, buffer);
				if (indigo_uni_printf(PRIVATE_DATA->handle, "I3") > 0 && indigo_uni_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer)) > 0) {
					INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, buffer);
					if (indigo_uni_printf(PRIVATE_DATA->handle, "I8") > 0 && indigo_uni_scanf_line(PRIVATE_DATA->handle, "FilterSlots %d", &PRIVATE_DATA->slot) == 1) {
						WHEEL_SLOT_ITEM->number.min = 1;
						WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->slot;
						if (xagyl_query(device)) {
							WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->slot;
							return true;
						}
					}
				}
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void xagyl_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

//- code

//+ wheel.code

static void wheel_move_finalizer(indigo_device *device) {
	if (xagyl_query(device)) {
		if (PRIVATE_DATA->slot != WHEEL_SLOT_ITEM->number.target) {
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		} else {
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot;
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

//- wheel.code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = xagyl_open(device);
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", WHEEL_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", WHEEL_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		xagyl_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
	int slot = (int)WHEEL_SLOT_ITEM->number.target;
	if (slot == PRIVATE_DATA->slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else  if (xagyl_goto(device, slot)) {
		WHEEL_SLOT_ITEM->number.value = 0;
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- wheel.WHEEL_SLOT.on_change
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
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
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Main code

indigo_result indigo_wheel_xagyl(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static xagyl_private_data *private_data = NULL;
	static indigo_device *wheel = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(xagyl_private_data));
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
