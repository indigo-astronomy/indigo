// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO StarlighXpress filter wheel driver
 \file indigo_ccd_sx.c
 */

#define DRIVER_VERSION 0x02000003
#define DRIVER_NAME "indigo_wheel_sx"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <hidapi/hidapi.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_sx.h"

// -------------------------------------------------------------------------------- SX USB interface implementation

#define SX_VENDOR_ID                  0x1278
#define SX_PRODUC_ID                  0x0920

#define PRIVATE_DATA        ((sx_private_data *)device->private_data)

typedef struct {
	hid_device *handle;
	int current_slot, target_slot;
	int count;
} sx_private_data;

static bool sx_message(indigo_device *device, int a, int b) {
	unsigned char buf[2] = { a, b };
	int rc = hid_write(PRIVATE_DATA->handle, buf, 2);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_write({ %02x, %02x }) ->  %d", buf[0], buf[1], rc);
	if (rc != 2)
		return false;
	indigo_usleep(100);
	rc = hid_read(PRIVATE_DATA->handle, buf, 2);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_read() ->  %d, { %02x, %02x }", rc, buf[0], buf[1]);
	PRIVATE_DATA->current_slot = buf[0];
	PRIVATE_DATA->count = buf[1];
	return rc == 2;
}

static bool sx_open(indigo_device *device) {
	if ((PRIVATE_DATA->handle = hid_open(SX_VENDOR_ID, SX_PRODUC_ID, NULL)) != NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_open ->  ok");
		sx_message(device, 0x81, 0);
		return true;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_open ->  failed");
	return false;
}

static void sx_close(indigo_device *device) {
	hid_close(PRIVATE_DATA->handle);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void wheel_timer_callback(indigo_device *device) {
	sx_message(device, 0, 0);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (sx_open(device)) {
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->count;
			PRIVATE_DATA->target_slot = 1;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		sx_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
			sx_message(device, 0x80 + PRIVATE_DATA->target_slot, 0);
			indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static indigo_device *wheel = NULL;

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
	);

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			if (wheel != NULL) {
				return 0;
			}
			wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
			char usb_path[INDIGO_NAME_SIZE];
			indigo_get_usb_path(dev, usb_path);
			snprintf(wheel->name, INDIGO_NAME_SIZE, "SX Filter Wheel #%s", usb_path);
			sx_private_data *private_data = indigo_safe_malloc(sizeof(sx_private_data));
			wheel->private_data = private_data;
			indigo_attach_device(wheel);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			if (wheel == NULL) {
				return 0;
			}
			indigo_detach_device(wheel);
			free(wheel->private_data);
			free(wheel);
			wheel = NULL;
		}
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_sx(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Starlight Xpress Filter Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		wheel = NULL;
		hid_init();
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SX_VENDOR_ID, SX_PRODUC_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(wheel);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		if (wheel)
			hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
