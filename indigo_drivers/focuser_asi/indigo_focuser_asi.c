// Copyright (C) 2019 Rumen G. Bogdanovski
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

/** INDIGO ASI focuser driver
 \file indigo_focuser_asi.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_asi"

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

#include <EAF_focuser.h>
#include "indigo_driver_xml.h"
#include "indigo_focuser_asi.h"

#define ASI_VENDOR_ID                   0x03c3

#define PRIVATE_DATA        ((asi_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	int dev_id;
	int current_position, target_position;
	indigo_timer *focuser_timer;
	pthread_mutex_t usb_mutex;
} asi_private_data;

static int find_index_by_device_id(int id);
// -------------------------------------------------------------------------------- INDIGO focuser device implementation


static void focuser_timer_callback(indigo_device *device) {
	bool moving;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_position));
	res = EAFIsMoving(PRIVATE_DATA->dev_id, &moving);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_position, res);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if ((!moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	EAF_INFO info;
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

				if (indigo_try_global_lock(device) != INDIGO_OK) {
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				} else {
					EAFGetID(index, &(PRIVATE_DATA->dev_id));
					int res = EAFOpen(PRIVATE_DATA->dev_id);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (!res) {
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						EAFGetProperty(PRIVATE_DATA->dev_id, &info);
						FOCUSER_POSITION_ITEM->number.max = info.MaxStep;
						res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_position));
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
						device->is_connected = true;
						PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0.5, focuser_timer_callback);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFOpen(%d) = %d", index, res);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					}
				}
			}
		} else {
			if (device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = EAFClose(PRIVATE_DATA->dev_id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
				res = EAFGetID(index, &(PRIVATE_DATA->dev_id));
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, PRIVATE_DATA->dev_id, res);
				indigo_global_unlock(device);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_ITEM->number.value < 1 || FOCUSER_POSITION_ITEM->number.value > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (FOCUSER_POSITION_ITEM->number.value == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.value;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position-1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position-1, res);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0.5, focuser_timer_callback);
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(NULL, device->name);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int eaf_products[100];
static int eaf_id_count = 0;


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[EAF_ID_MAX];

static int find_index_by_device_id(int id) {
	int count = EAFGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetNum() = %d", count);
	int cur_id;
	for (int index = 0; index < count; index++) {
		int res = EAFGetID(index, &cur_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, cur_id, res);
		if (res == EAF_SUCCESS && cur_id == id) {
			return index;
		}
	}
	return -1;
}


static int find_plugged_device_id() {
	int id = NO_DEVICE, new_id = NO_DEVICE;
	int count = EAFGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = EAFGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, id, res);
		if (res == EAF_SUCCESS && !connected_ids[id]) {
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
	bool dev_tmp[EAF_ID_MAX] = { false };
	int id = -1;
	int count = EAFGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = EAFGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, id, res);
		if (res == EAF_SUCCESS)
			dev_tmp[id] = true;
	}
	id = -1;
	for (int index = 0; index < EAF_ID_MAX; index++) {
		if (connected_ids[index] && !dev_tmp[index]) {
			id = index;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}

static void process_plug_event(indigo_device *unused) {
	EAF_INFO info;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"",
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
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
	int res = EAFOpen(id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFOpen(%d}) = %d", id, res);
		pthread_mutex_unlock(&device_mutex);
		return;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFOpen(%d}) = %d", id, res);
	}
	while (true) {
		res = EAFGetProperty(id, &info);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetProperty(%d, -> { %d, '%s', %d }) = %d", id, info.ID, info.Name, info.MaxStep, res);
		if (res == EAF_SUCCESS) {
			EAFClose(id);
			break;
		}
		if (res != EAF_ERROR_MOVING) {
			EAFClose(id);
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		sleep(1);
	}
	indigo_device *device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &focuser_template, sizeof(indigo_device));
	sprintf(device->name, "%s #%d", info.Name, id);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	asi_private_data *private_data = malloc(sizeof(asi_private_data));
	assert(private_data != NULL);
	memset(private_data, 0, sizeof(asi_private_data));
	private_data->dev_id = id;
	device->private_data = private_data;
	indigo_async((void *)(void *)indigo_attach_device, device);
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EAF device unplugged (maybe ASI Camera)!");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < eaf_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || eaf_products[i] != descriptor.idProduct) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EAF device plugged (maybe ASI Camera)!");
					continue;
				}
				indigo_set_timer(NULL, 0.5, process_plug_event);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			indigo_set_timer(NULL, 0.5, process_unplug_event);
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
	for (int index = 0; index < EAF_ID_MAX; index++)
		connected_ids[index] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_focuser_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Focuser", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for(int index = 0; index < EAF_ID_MAX; index++)
			connected_ids[index] = false;
		eaf_id_count = EAFGetProductIDs(eaf_products);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetProductIDs(-> [ %d, %d, ... ]) = %d", eaf_products[0], eaf_products[1], eaf_id_count);
		if (eaf_id_count <= 0) {
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
