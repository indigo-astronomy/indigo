// Copyright (c) 2017 Rumen G. Bogdanovski
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


/** INDIGO ZWO ASI USB2ST4 guider driver
 \file indigo_guider_asi.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_guider_asi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_guider_asi.h"

#if !(defined(__APPLE__) && defined(__arm64__))

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "USB2ST4_Conv.h"

#define ASI_VENDOR_ID              0x03c3

#define PRIVATE_DATA               ((asi_private_data *)device->private_data)

#define PIXEL_FORMAT_PROPERTY      (PRIVATE_DATA->pixel_format_property)

// gp_bits is used as boolean
#define is_connected               gp_bits

// -------------------------------------------------------------------------------- ZWO ASI USB interface implementation

#define us2s(s) ((s) / 1000000.0)
#define s2us(us) ((us) * 1000000)


typedef struct {
	int dev_id;
	indigo_timer *guider_timer_ra, *guider_timer_dec;
	bool guide_relays[4];
	pthread_mutex_t usb_mutex;
} asi_private_data;


static bool asi_open(indigo_device *device) {
	int id = PRIVATE_DATA->dev_id;
	USB2ST4_ERROR_CODE res;

	if (device->is_connected) return false;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	res = USB2ST4Open(id);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB2ST4Open(%d) = %d", id, res);
		return false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static void asi_close(indigo_device *device) {

	if (!device->is_connected) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	USB2ST4Close(PRIVATE_DATA->dev_id);
	indigo_global_unlock(device);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}


static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	int id = PRIVATE_DATA->dev_id;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	USB2ST4PulseGuide(id, USB2ST4_EAST, false);
	USB2ST4PulseGuide(id, USB2ST4_WEST, false);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->guide_relays[USB2ST4_EAST] || PRIVATE_DATA->guide_relays[USB2ST4_WEST]) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	PRIVATE_DATA->guide_relays[USB2ST4_EAST] = false;
	PRIVATE_DATA->guide_relays[USB2ST4_WEST] = false;
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;
	int id = PRIVATE_DATA->dev_id;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	USB2ST4PulseGuide(id, USB2ST4_SOUTH, false);
	USB2ST4PulseGuide(id, USB2ST4_NORTH, false);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	if (PRIVATE_DATA->guide_relays[USB2ST4_NORTH] || PRIVATE_DATA->guide_relays[USB2ST4_SOUTH]) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	PRIVATE_DATA->guide_relays[USB2ST4_SOUTH] = false;
	PRIVATE_DATA->guide_relays[USB2ST4_NORTH] = false;
}


// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (asi_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
				GUIDER_GUIDE_RA_PROPERTY->hidden = false;
				device->is_connected = true;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
			asi_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	USB2ST4_ERROR_CODE res;
	int id = PRIVATE_DATA->dev_id;

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = USB2ST4PulseGuide(id, USB2ST4_NORTH, true);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

			if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB2ST4PulseGuide(%d, USB2ST4_NORTH) = %d", id, res);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
			PRIVATE_DATA->guide_relays[USB2ST4_NORTH] = true;
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = USB2ST4PulseGuide(id, USB2ST4_SOUTH, true);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB2ST4PulseGuide(%d, USB2ST4_SOUTH) = %d", id, res);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
				PRIVATE_DATA->guide_relays[USB2ST4_SOUTH] = true;
			}
		}

		if (PRIVATE_DATA->guide_relays[USB2ST4_SOUTH] || PRIVATE_DATA->guide_relays[USB2ST4_NORTH])
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = USB2ST4PulseGuide(id, USB2ST4_EAST, true);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

			if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB2ST4PulseGuide(%d, USB2ST4_EAST) = %d", id, res);
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
			PRIVATE_DATA->guide_relays[USB2ST4_EAST] = true;
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = USB2ST4PulseGuide(id, USB2ST4_WEST, true);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				if (res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB2ST4PulseGuide(%d, USB2ST4_WEST) = %d", id, res);
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
				PRIVATE_DATA->guide_relays[USB2ST4_WEST] = true;
			}
		}

		if (PRIVATE_DATA->guide_relays[USB2ST4_EAST] || PRIVATE_DATA->guide_relays[USB2ST4_WEST])
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int asi_products[100];
static int asi_id_count = 0;

static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[USB2ST4_ID_MAX] = {false};


static int find_index_by_device_id(int id) {
	int cid;
	int count = USB2ST4GetNum();
	for(int index = 0; index < count; index++) {
		USB2ST4GetID(index, &cid);
		if (cid == id) return index;
	}
	return -1;
}


static int find_plugged_device_id() {
	int i, id = NO_DEVICE, new_id = NO_DEVICE;

	int count = USB2ST4GetNum();
	for(i = 0; i < count; i++) {
		USB2ST4GetID(i, &id);
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
	bool dev_tmp[USB2ST4_ID_MAX] = {false};
	int i, id;

	int count = USB2ST4GetNum();
	for(i = 0; i < count; i++) {
		USB2ST4GetID(i, &id);
		dev_tmp[id] = true;
	}

	id = -1;
	for(i = 0; i < USB2ST4_ID_MAX; i++) {
		if(connected_ids[i] && !dev_tmp[i]){
			id = i;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}

static void process_plug_event(indigo_device *unused) {
	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		 "",
		 guider_attach,
		 indigo_guider_enumerate_properties,
		 guider_change_property,
		 NULL,
		 guider_detach
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
	int index = find_index_by_device_id(id);
	if (index < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No index of plugged device found.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
	sprintf(device->name, "ASI USB-St4 Guider #%d", id);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->dev_id = id;
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
}

static void process_unplug_event(indigo_device *unused) {
	int id, slot;
	bool removed = false;
	asi_private_data *private_data = NULL;
	pthread_mutex_lock(&device_mutex);
	while ((id = find_unplugged_device_id()) != -1) {
		slot = find_device_slot(id);
		while (slot >= 0) {
			indigo_device **device = &devices[slot];
			if (*device == NULL) {
				pthread_mutex_unlock(&device_mutex);
				return;
			}
			indigo_detach_device(*device);
			if ((*device)->private_data) {
				private_data = (*device)->private_data;
			}
			free(*device);
			*device = NULL;
			removed = true;
			slot = find_device_slot(id);
		}

		if (private_data) {
			USB2ST4Close(id);
			free(private_data);
			private_data = NULL;
		}
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI USB-St4 Guider unplugged (maybe other ASI device)!");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < asi_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || asi_products[i] != descriptor.idProduct)
					continue;
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
}


static void remove_all_devices() {
	int i;
	asi_private_data *pds[USB2ST4_ID_MAX] = { NULL };
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) pds[PRIVATE_DATA->dev_id] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		devices[i] = NULL;
	}
	/* free private data */
	for(i = 0; i < USB2ST4_ID_MAX; i++) {
		if (pds[i]) free(pds[i]);
	}
	for(i = 0; i < USB2ST4_ID_MAX; i++)
		connected_ids[i] = false;
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_guider_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI USB-St4 Guider", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			asi_id_count = USB2ST4GetProductIDs(asi_products);
			if (asi_id_count <= 0) {
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

indigo_result indigo_guider_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI USB-St4 Guider", __FUNCTION__, DRIVER_VERSION, true, last_action);

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
