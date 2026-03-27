// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_wheel_atik.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <libatik.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_atik.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_wheel_atik"
#define DRIVER_LABEL         "Atik Filter Wheel"
#define WHEEL_DEVICE_NAME    "Atik Filter Wheel"
#define PRIVATE_DATA         ((atik_private_data *)device->private_data)

//+ define

#define ATIK_VENDOR_ID       0x04D8
#define ATIK_PRODUC_ID       0x003F

//- define

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	int slot_count, current_slot, target_slot;
	//- data
} atik_private_data;

#pragma mark - Low level code

//+ code

static bool atik_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_hid(ATIK_VENDOR_ID, ATIK_PRODUC_ID, INDIGO_LOG_DEBUG | BINARY_LOG);
	return PRIVATE_DATA->handle != NULL;
}

static void atik_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ wheel.code

static void wheel_move_finalizer(indigo_device *device) {
	libatik_wheel_query((hid_device *)PRIVATE_DATA->handle->hid_device, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

//- wheel.code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = atik_open(device);
		if (connection_result) {
			//+ wheel.on_connect
			connection_result = false;
			for (int i = 0; i < 10; i++) {
				libatik_wheel_query((hid_device *)PRIVATE_DATA->handle->hid_device, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
				if (PRIVATE_DATA->slot_count > 0 && PRIVATE_DATA->slot_count <= 9) {
					connection_result = true;
					break;
				}
				indigo_sleep(1);
			}
			if (connection_result) {
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->slot_count;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot;
			}
			//- wheel.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		atik_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.WHEEL_SLOT.on_change
	if (WHEEL_SLOT_ITEM->number.value != PRIVATE_DATA->current_slot) {
		PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.value;
		libatik_wheel_set((hid_device *)PRIVATE_DATA->handle->hid_device, PRIVATE_DATA->target_slot);
		libatik_wheel_query((hid_device *)PRIVATE_DATA->handle->hid_device, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
	//- wheel.WHEEL_SLOT.on_change
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
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

#pragma mark - Hot-plug code

static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *wheel = NULL;

static void process_plug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	if (wheel == NULL) {
		char usb_path[INDIGO_NAME_SIZE];
		indigo_get_usb_path(dev, usb_path);
		atik_private_data *private_data = indigo_safe_malloc(sizeof(atik_private_data));
		wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s #%s", "Atik Filter Wheel", usb_path);
		wheel->private_data = private_data;
		indigo_attach_device(wheel);
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	if (wheel != NULL) {
		indigo_detach_device(wheel);
		free(wheel->private_data);
		free(wheel);
		wheel = NULL;
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_wheel_atik(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			wheel = NULL;
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ATIK_VENDOR_ID, ATIK_PRODUC_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(wheel);
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			process_unplug_event(NULL);
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
