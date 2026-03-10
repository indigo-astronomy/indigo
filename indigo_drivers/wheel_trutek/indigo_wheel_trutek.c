// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_wheel_trutek.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_wheel_trutek.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_wheel_trutek"
#define DRIVER_LABEL         "Trutek Filter Wheel"
#define WHEEL_DEVICE_NAME    "Trutek Filter Wheel"
#define PRIVATE_DATA         ((trutek_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	int slot;
	//- data
} trutek_private_data;

#pragma mark - Low level code

//+ code

static bool trutek_query(indigo_device *device) {
	unsigned char buffer[4] = { 0xA5, 0x02, 0x20, 0xA5 + 0x02 + 0x20 };
	if (indigo_uni_write(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 && indigo_uni_read(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 && buffer[0] == 0xA5 && buffer[1] == 0x82) {
		PRIVATE_DATA->slot = buffer[2] - 0x30;
		return true;
	}
	return false;
}

static bool trutek_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG | BINARY_LOG);
	if (PRIVATE_DATA->handle != NULL) {
		unsigned char buffer[4] = { 0xA5, 0x03, 0x20, 0xA5 + 0x03 + 0x20 };
		if (indigo_uni_write(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 && indigo_uni_read(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 && buffer[0] == 0xA5 && buffer[1] == 0x83) {
			WHEEL_SLOT_ITEM->number.min = 1;
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = buffer[2] - 0x30;
			if (trutek_query(device)) {
				WHEEL_SLOT_ITEM->number.target = WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot;
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void trutek_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

//- code

//+ wheel.code

static void wheel_move_finalizer(indigo_device *device) {
	if (trutek_query(device)) {
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot;
		if (PRIVATE_DATA->slot == 0) {
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		} else {
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
		connection_result = trutek_open(device);
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
		trutek_close(device);
		indigo_send_message(device, CONNECTION_PROPERTY, "Disconnected from %s", device->name);
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
	} else {
		unsigned char buffer[4] = { 0xA5, 0x01, slot, 0xA5 + 0x01 + slot };
		if (indigo_uni_write(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 && indigo_uni_read(PRIVATE_DATA->handle, (char *)buffer, 4) == 4 && buffer[0] == 0xA5 && buffer[1] == 0x81) {
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot = buffer[2];
			if (PRIVATE_DATA->slot == 0) {
				indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
			}
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
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

indigo_result indigo_wheel_trutek(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static trutek_private_data *private_data = NULL;
	static indigo_device *wheel = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(trutek_private_data));
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
