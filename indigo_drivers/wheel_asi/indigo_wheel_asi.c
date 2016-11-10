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
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** INDIGO ASI filter wheel driver
 \file indigo_wheel_asi.c
 */

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

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((asi_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
	int dev_id;
	int current_slot, target_slot;
	int count;
} asi_private_data;

static int find_index_by_device_id(int id);
// -------------------------------------------------------------------------------- INDIGO Wheel device implementation


static void wheel_timer_callback(indigo_device *device) {
	EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	PRIVATE_DATA->current_slot++;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}


static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	asi_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_wheel_attach(device, INDIGO_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
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
			int res = EFWOpen(index);
			if (!res) {
				EFWGetID(index, &(PRIVATE_DATA->dev_id)); /* id's may change on connect and disconnect */
				EFWGetProperty(PRIVATE_DATA->dev_id, &info);
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->count = info.slotNum;
				EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_slot));
				PRIVATE_DATA->target_slot++;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_timer(device, 0.5, wheel_timer_callback);
			} else {
				INDIGO_LOG(indigo_log("indigo_wheel_asi: EFWOpen(%d) = %d", index, res));
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				return INDIGO_FAILED;
			}
		} else {
			EFWClose(PRIVATE_DATA->dev_id);
			EFWGetID(index, &(PRIVATE_DATA->dev_id)); /* id's may change on connect and disconnect */
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
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
			EFWSetPosition(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot-1);
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
	indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("indigo_wheel_asi: '%s' detached.", device->name));
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int efw_products[100];
static int efw_id_count = 0;


static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
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
	bool dev_tmp[EFW_ID_MAX] = {false};
	int i, id = NO_DEVICE, new_id = NO_DEVICE;

	int count = EFWGetNum();
	for(i = 0; i < count; i++) {
		EFWGetID(i, &id);
		dev_tmp[id] = true;
		if(!connected_ids[id]) new_id = id;
	}

	for(i = 0; i < EFW_ID_MAX; i++)
		connected_ids[i] = dev_tmp[i];

	return new_id;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_unplugged_device_slot() {
	bool dev_tmp[EFW_ID_MAX] = {false};
	int i, id = -1;

	int count = EFWGetNum();
	for(i = 0; i < count; i++) {
		EFWGetID(i, &id);
		dev_tmp[id] = true;
	}

	id = -1;
	for(i = 0; i < EFW_ID_MAX; i++) {
		if(connected_ids[i] && !dev_tmp[i] && id == -1) id = i;
		connected_ids[i] = dev_tmp[i];
	}
	return id;
}


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	int id;
	int index = 0;
	EFW_INFO info;

	static indigo_device wheel_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		wheel_detach
	};

	struct libusb_device_descriptor descriptor;

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			int rc = libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < efw_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || efw_products[i] != descriptor.idProduct) continue;

				int slot = find_available_device_slot();
				if (slot < 0) {
					INDIGO_LOG(indigo_log("indigo_wheel_asi: No available device slots available."));
					return 0;
				}

				int id = find_plugged_device_id();
				if (id == NO_DEVICE) {
					INDIGO_LOG(indigo_log("indigo_wheel_asi: No plugged device found."));
				}

				indigo_device *device = malloc(sizeof(indigo_device));
				EFWGetProperty(id, &info);
				assert(device != NULL);
				memcpy(device, &wheel_template, sizeof(indigo_device));
				sprintf(device->name, "%s %d", info.Name, id);
				INDIGO_LOG(indigo_log("indigo_wheel_asi: '%s' attached.", device->name));
				device->device_context = malloc(sizeof(asi_private_data));
				assert(device->device_context);
				memset(device->device_context, 0, sizeof(asi_private_data));
				((asi_private_data*)device->device_context)->dev_id = id;
				indigo_attach_device(device);
				devices[slot]=device;
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			int slot;
			bool removed = false;
			while ((slot = find_unplugged_device_slot()) != -1) {
				indigo_device **device = &devices[slot];
				if (*device == NULL)
					return 0;
				indigo_detach_device(*device);
				free((*device)->device_context);
				free(*device);
				*device = NULL;
				removed = true;
			}
			if (!removed) {
				INDIGO_LOG(indigo_log("indigo_wheel_asi: No ASI EFW device unplugged (maybe ASI camera)!"));
			}
		}
	}
	return 0;
};


void remove_all_devices() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL) continue;
		indigo_detach_device(*device);
		free((*device)->device_context);
		free(*device);
		*device = NULL;
	}
	for(i = 0; i < EFW_ID_MAX; i++)
		connected_ids[i] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_asi(bool state) {
	static bool current_state = false;
	if (state == current_state)
		return INDIGO_OK;
	if ((current_state = state)) {
		efw_id_count = EFWGetProductIDs(efw_products);
		if (efw_id_count <= 0) {
			INDIGO_LOG(indigo_log("indigo_wheel_asi: Can not get the list of supported IDs."));
			return INDIGO_FAILED;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_wheel_asi: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
	} else {
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_wheel_asi: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_all_devices();
		return INDIGO_OK;
	}
}
