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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Atik filter wheel driver
 \file indigo_ccd_atik.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_wheel_atik"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <libatik/libatik.h>

#include "indigo_driver_xml.h"
#include "indigo_wheel_atik.h"

// -------------------------------------------------------------------------------- ATIK USB interface implementation

#define ATIK_VENDOR_ID                  0x04D8
#define ATIK_PRODUC_ID                  0x003F

#define PRIVATE_DATA        ((atik_private_data *)device->private_data)

typedef struct {
	hid_device *handle;
	int slot_count, current_slot;
} atik_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void wheel_timer_callback(indigo_device *device) {
	libatik_wheel_query(PRIVATE_DATA->handle, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (WHEEL_SLOT_ITEM->number.value == WHEEL_SLOT_ITEM->number.target) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if ((PRIVATE_DATA->handle = hid_open(ATIK_VENDOR_ID, ATIK_PRODUC_ID, NULL)) != NULL) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_open ->  ok");
				while (true) {
					libatik_wheel_query(PRIVATE_DATA->handle, &PRIVATE_DATA->slot_count, &PRIVATE_DATA->current_slot);
					if (PRIVATE_DATA->slot_count > 0 && PRIVATE_DATA->slot_count <= 9)
						break;
					sleep(1);
				}
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->slot_count;
				WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_open ->  failed");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			hid_close(PRIVATE_DATA->handle);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			libatik_wheel_set(PRIVATE_DATA->handle, (int)WHEEL_SLOT_ITEM->number.value);
			indigo_set_timer(device, 0.5, wheel_timer_callback);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static indigo_device *device = NULL;

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device wheel_template = {
		"ATIK Filter Wheel", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		wheel_detach
	};

	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			if (device != NULL) {
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}
			device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &wheel_template, sizeof(indigo_device));
			atik_private_data *private_data = malloc(sizeof(atik_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(atik_private_data));
			device->private_data = private_data;
			indigo_attach_device(device);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			if (device == NULL) {
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
			device = NULL;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_atik(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Atik Filter Wheel", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		device = NULL;
		hid_init();
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ATIK_VENDOR_ID, ATIK_PRODUC_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		if (device)
			hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		hid_exit();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
