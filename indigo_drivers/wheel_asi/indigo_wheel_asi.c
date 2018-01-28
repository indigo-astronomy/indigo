// Copyright (C) 2016 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO ASI filter wheel driver
 \file indigo_wheel_asi.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_wheel_asi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <asi_efw/EFW_filter.h>
#include "indigo_driver_xml.h"
#include "indigo_wheel_asi.h"

#define ASI_VENDOR_ID                   0x03c3

#define PRIVATE_DATA        ((asi_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	int dev_id;
	int current_slot, target_slot;
	int count;
	indigo_timer *wheel_timer;
	pthread_mutex_t usb_mutex;
} asi_private_data;

static int find_index_by_device_id(int id);
// -------------------------------------------------------------------------------- INDIGO Wheel device implementation


static void wheel_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	PRIVATE_DATA->current_slot++;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->wheel_timer));
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}


static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	EFW_INFO info;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);

		int index = find_index_by_device_id(PRIVATE_DATA->dev_id);
		if (index < 0) {
			return INDIGO_NOT_FOUND;
		}

		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				EFWGetID(index, &(PRIVATE_DATA->dev_id));
				int res = EFWOpen(PRIVATE_DATA->dev_id);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (!res) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					EFWGetProperty(PRIVATE_DATA->dev_id, &info);
					WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->count = info.slotNum;
					EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_slot));
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					PRIVATE_DATA->target_slot++;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;
					PRIVATE_DATA->wheel_timer = indigo_set_timer(device, 0.5, wheel_timer_callback);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFWOpen(%d) = %d", index, res);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					return INDIGO_FAILED;
				}
			}
		} else {
			if (device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				EFWClose(PRIVATE_DATA->dev_id);
				EFWGetID(index, &(PRIVATE_DATA->dev_id));
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}

		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
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
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			EFWSetPosition(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot-1);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			PRIVATE_DATA->wheel_timer = indigo_set_timer(device, 0.5, wheel_timer_callback);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}


static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int efw_products[100];
static int efw_id_count = 0;


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[EFW_ID_MAX];


static int find_index_by_device_id(int id) {
	int count = EFWGetNum();
	int cur_id;
	for(int index = 0; index < count; index++) {
		EFWGetID(index,&cur_id);
		if (cur_id == id) return index;
	}
	return -1;
}


static int find_plugged_device_id() {
	int i, id = NO_DEVICE, new_id = NO_DEVICE;

	int count = EFWGetNum();
	for(i = 0; i < count; i++) {
		EFWGetID(i, &id);
		if(!connected_ids[id]) {
			new_id = id;
			connected_ids[id] = true;
			break;
		}
	}

	return new_id;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(int id) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (PRIVATE_DATA->dev_id == id) return slot;
	}
	return -1;
}


static int find_unplugged_device_id() {
	bool dev_tmp[EFW_ID_MAX] = {false};
	int i, id = -1;

	int count = EFWGetNum();
	for(i = 0; i < count; i++) {
		EFWGetID(i, &id);
		dev_tmp[id] = true;
	}

	id = -1;
	for(i = 0; i < EFW_ID_MAX; i++) {
		if(connected_ids[i] && !dev_tmp[i]){
			id = i;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	EFW_INFO info;

	static indigo_device wheel_template = {
		"", 0, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
	};

	struct libusb_device_descriptor descriptor;

	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < efw_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || efw_products[i] != descriptor.idProduct) continue;

				int slot = find_available_device_slot();
				if (slot < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
					pthread_mutex_unlock(&device_mutex);
					return 0;
				}

				int id = find_plugged_device_id();
				if (id == NO_DEVICE) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "No plugged device found.");
					pthread_mutex_unlock(&device_mutex);
					return 0;
				}

				indigo_device *device = malloc(sizeof(indigo_device));
				EFWGetProperty(id, &info);
				assert(device != NULL);
				memcpy(device, &wheel_template, sizeof(indigo_device));
				sprintf(device->name, "%s #%d", info.Name, id);
				INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
				asi_private_data *private_data = malloc(sizeof(asi_private_data));
				assert(private_data != NULL);
				memset(private_data, 0, sizeof(asi_private_data));
				private_data->dev_id = id;
				device->private_data = private_data;
				indigo_async((void *)(void *)indigo_attach_device, device);
				devices[slot]=device;
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			int slot, id;
			bool removed = false;
			while ((id = find_unplugged_device_id()) != -1) {
				slot = find_device_slot(id);
				if (slot < 0) continue;
				indigo_device **device = &devices[slot];
				if (*device == NULL) {
					pthread_mutex_unlock(&device_mutex);
					return 0;
				}
				indigo_detach_device(*device);
				free((*device)->private_data);
				free(*device);
				libusb_unref_device(dev);
				*device = NULL;
				removed = true;
			}
			if (!removed) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "No ASI EFW device unplugged (maybe ASI Camera)!");
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};


static void remove_all_devices() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
	for(i = 0; i < EFW_ID_MAX; i++)
		connected_ids[i] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Filter Wheel", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		efw_id_count = EFWGetProductIDs(efw_products);
		if (efw_id_count <= 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get the list of supported IDs.");
			return INDIGO_FAILED;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
