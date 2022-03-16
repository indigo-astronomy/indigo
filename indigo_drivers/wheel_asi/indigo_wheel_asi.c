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

#define DRIVER_VERSION 0x0008
#define DRIVER_NAME "indigo_wheel_asi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_wheel_asi.h"

#if !(defined(__APPLE__) && defined(__arm64__))

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <EFW_filter.h>

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
	int res = EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_slot, res);
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

	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		char *sdk_version = EFWGetSDKVersion();
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");

		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	EFW_INFO info;
	int index = find_index_by_device_id(PRIVATE_DATA->dev_id);
	if (index < 0) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

				if (indigo_try_global_lock(device) != INDIGO_OK) {
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				} else {
					EFWGetID(index, &(PRIVATE_DATA->dev_id));
					int res = EFWOpen(PRIVATE_DATA->dev_id);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (!res) {
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						EFWGetProperty(PRIVATE_DATA->dev_id, &info);
						WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->count = info.slotNum;
						res = EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_slot));
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot, res);
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
						PRIVATE_DATA->target_slot++;
						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
						device->is_connected = true;
						indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFWOpen(%d) = %d", index, res);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					}
				}
			}
		} else {
			if (device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = EFWClose(PRIVATE_DATA->dev_id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWClose(%d) = %d", PRIVATE_DATA->dev_id, res);
				res = EFWGetID(index, &(PRIVATE_DATA->dev_id));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetID(%d, -> %d) = %d", index, PRIVATE_DATA->dev_id, res);
				indigo_global_unlock(device);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
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
			int res = EFWSetPosition(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot-1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWSetPosition(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot-1, res);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
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

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int efw_products[100];
static int efw_id_count = 0;


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[EFW_ID_MAX];

static int find_index_by_device_id(int id) {
	int count = EFWGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetNum() = %d", count);
	int cur_id;
	for (int index = 0; index < count; index++) {
		int res = EFWGetID(index, &cur_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetID(%d, -> %d) = %d", index, cur_id, res);
		if (res == EFW_SUCCESS && cur_id == id) {
			return index;
		}
	}
	return -1;
}


static int find_plugged_device_id() {
	int id = NO_DEVICE, new_id = NO_DEVICE;
	int count = EFWGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = EFWGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetID(%d, -> %d) = %d", index, id, res);
		if (res == EFW_SUCCESS && !connected_ids[id]) {
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
	bool dev_tmp[EFW_ID_MAX] = { false };
	int id = -1;
	int count = EFWGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = EFWGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetID(%d, -> %d) = %d", index, id, res);
		if (res == EFW_SUCCESS)
			dev_tmp[id] = true;
	}
	id = -1;
	for (int index = 0; index < EFW_ID_MAX; index++) {
		if (connected_ids[index] && !dev_tmp[index]) {
			id = index;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}

static void process_plug_event(indigo_device *unused) {
	EFW_INFO info;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);
	pthread_mutex_lock(&device_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	int id = find_plugged_device_id();
	if (id == NO_DEVICE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	int res = EFWOpen(id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFWOpen(%d}) = %d", id, res);
		pthread_mutex_unlock(&device_mutex);
		return;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWOpen(%d}) = %d", id, res);
	}
	while (true) {
		res = EFWGetProperty(id, &info);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetProperty(%d, -> { %d, '%s', %d }) = %d", id, info.ID, info.Name, info.slotNum, res);
		if (res == EFW_SUCCESS) {
			EFWClose(id);
			break;
		}
		if (res != EFW_ERROR_MOVING) {
			EFWClose(id);
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		  indigo_usleep(ONE_SECOND_DELAY);
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
	sprintf(device->name, "%s #%d", info.Name, id);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->dev_id = id;
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	int slot, id;
	bool removed = false;
	pthread_mutex_lock(&device_mutex);
	while ((id = find_unplugged_device_id()) != -1) {
		slot = find_device_slot(id);
		if (slot < 0) continue;
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
		removed = true;
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EFW device unplugged (maybe ASI Camera)!");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < efw_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || efw_products[i] != descriptor.idProduct) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EFW device plugged (maybe ASI Camera)!");
					continue;
				}
				indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			indigo_set_timer(NULL, 0.5, process_unplug_event, NULL);
			break;
		}
	}
	return 0;
};


static void remove_all_devices() {
	for (int index = 0; index < MAX_DEVICES; index++) {
		indigo_device **device = &devices[index];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
	for (int index = 0; index < EFW_ID_MAX; index++)
		connected_ids[index] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Filter Wheel", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		char *sdk_version = EFWGetSDKVersion();
		INDIGO_DRIVER_LOG(DRIVER_NAME, "EFW SDK v. %s", sdk_version);

		for(int index = 0; index < EFW_ID_MAX; index++)
			connected_ids[index] = false;
		efw_id_count = EFWGetProductIDs(efw_products);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetProductIDs(-> [ %d, %d, ... ]) = %d", efw_products[0], efw_products[1], efw_id_count);
		if (efw_id_count <= 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get the list of supported IDs.");
			return INDIGO_FAILED;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
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

#else

indigo_result indigo_wheel_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Filter Wheel", __FUNCTION__, DRIVER_VERSION, true, last_action);

	switch(action) {
		case INDIGO_DRIVER_INIT:
		case INDIGO_DRIVER_SHUTDOWN:
			return INDIGO_UNSUPPORTED_ARCH;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}

#endif
