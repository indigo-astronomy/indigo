// Copyright (c) 2017-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_guider_asi.driver

// supported_architecture: !defined(INDIGO_WINDOWS) && (!defined(INDIGO_MACOS) || defined(__x86_64__))
#if !defined(INDIGO_WINDOWS) && (!defined(INDIGO_MACOS) || defined(__x86_64__))

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdbool.h>
#include "USB2ST4_Conv.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_guider_asi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_guider_asi"
#define DRIVER_LABEL         "ZWO ASI USB-St4 Guider"
#define GUIDER_DEVICE_NAME   "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((asi_private_data *)device->private_data)

//+ define

#define ASI_VENDOR_ID        0x03c3
#define NO_DIRECTION         (-1)

//- define

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	//+ data
	int dev_id;
	int active_ra, active_dec;
	double deadline_ra, deadline_dec;
	//- data
} asi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static pthread_mutex_t sdk_mutex = PTHREAD_MUTEX_INITIALIZER;
static int asi_products[USB2ST4_ID_MAX];
static int asi_product_count;

static int asi_get_num(void) {
	pthread_mutex_lock(&sdk_mutex);
	int count = USB2ST4GetNum();
	pthread_mutex_unlock(&sdk_mutex);
	return count;
}

static USB2ST4_ERROR_CODE asi_get_id(int index, int *id) {
	pthread_mutex_lock(&sdk_mutex);
	USB2ST4_ERROR_CODE result = USB2ST4GetID(index, id);
	pthread_mutex_unlock(&sdk_mutex);
	return result;
}

static bool asi_load_products(void) {
	pthread_mutex_lock(&sdk_mutex);
	int capacity = USB2ST4GetProductIDs(NULL);
	if (capacity <= 0 || capacity > USB2ST4_ID_MAX) {
		pthread_mutex_unlock(&sdk_mutex);
		return false;
	}
	int count = USB2ST4GetProductIDs(asi_products);
	pthread_mutex_unlock(&sdk_mutex);
	if (count <= 0 || count > capacity || count > USB2ST4_ID_MAX) {
		return false;
	}
	asi_product_count = count;
	return true;
}

static bool asi_pid_supported(uint16_t pid) {
	for (int index = 0; index < asi_product_count; index++) {
		if (asi_products[index] == pid) {
			return true;
		}
	}
	return false;
}

static USB2ST4_ERROR_CODE asi_set_relay(indigo_device *device, int direction, bool enabled) {
	pthread_mutex_lock(&sdk_mutex);
	USB2ST4_ERROR_CODE result = USB2ST4PulseGuide(PRIVATE_DATA->dev_id, (USB2ST4_DIRECTION)direction, enabled);
	pthread_mutex_unlock(&sdk_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "USB2ST4PulseGuide(%d, %d, %s) = %d", PRIVATE_DATA->dev_id, direction, enabled ? "ON" : "OFF", result);
	return result;
}

static bool asi_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock");
		return false;
	}
	pthread_mutex_lock(&sdk_mutex);
	USB2ST4_ERROR_CODE result = USB2ST4Open(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&sdk_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "USB2ST4Open(%d) = %d", PRIVATE_DATA->dev_id, result);
	if (result != USB2ST4_SUCCESS) {
		indigo_global_unlock(device);
		return false;
	}
	PRIVATE_DATA->active_ra = PRIVATE_DATA->active_dec = NO_DIRECTION;
	PRIVATE_DATA->deadline_ra = PRIVATE_DATA->deadline_dec = 0;
	return true;
}

static void asi_close(indigo_device *device) {
	pthread_mutex_lock(&sdk_mutex);
	USB2ST4_ERROR_CODE result = USB2ST4Close(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&sdk_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "USB2ST4Close(%d) = %d", PRIVATE_DATA->dev_id, result);
	indigo_global_unlock(device);
}

static void asi_clear_axis_items(indigo_device *device, bool ra) {
	if (ra) {
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
	} else {
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
	}
}

static void guider_guide_ra_finalizer(indigo_device *device);
static void guider_guide_dec_finalizer(indigo_device *device);

static bool asi_stop_axis(indigo_device *device, bool ra) {
	int *active = ra ? &PRIVATE_DATA->active_ra : &PRIVATE_DATA->active_dec;
	double *deadline = ra ? &PRIVATE_DATA->deadline_ra : &PRIVATE_DATA->deadline_dec;
	indigo_property *property = ra ? GUIDER_GUIDE_RA_PROPERTY : GUIDER_GUIDE_DEC_PROPERTY;
	asi_clear_axis_items(device, ra);
	if (*active == NO_DIRECTION) {
		*deadline = 0;
		property->state = INDIGO_OK_STATE;
		return true;
	}
	USB2ST4_ERROR_CODE result = asi_set_relay(device, *active, false);
	*deadline = 0;
	if (result != USB2ST4_SUCCESS) {
		property->state = INDIGO_ALERT_STATE;
		return false;
	}
	*active = NO_DIRECTION;
	property->state = INDIGO_OK_STATE;
	return true;
}

static void asi_axis_finalizer(indigo_device *device, bool ra) {
	if (!IS_CONNECTED) {
		return;
	}
	double deadline = ra ? PRIVATE_DATA->deadline_ra : PRIVATE_DATA->deadline_dec;
	double remaining = deadline - indigo_monotonic_time();
	if (deadline > 0 && remaining > 0.0005) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, remaining, ra ? guider_guide_ra_finalizer : guider_guide_dec_finalizer);
		return;
	}
	asi_stop_axis(device, ra);
	indigo_update_property(device, ra ? GUIDER_GUIDE_RA_PROPERTY : GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	asi_axis_finalizer(device, true);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	asi_axis_finalizer(device, false);
}

static void asi_change_axis(indigo_device *device, bool ra, int direction, int duration, indigo_timer_callback finalizer) {
	int *active = ra ? &PRIVATE_DATA->active_ra : &PRIVATE_DATA->active_dec;
	double *deadline = ra ? &PRIVATE_DATA->deadline_ra : &PRIVATE_DATA->deadline_dec;
	indigo_property *property = ra ? GUIDER_GUIDE_RA_PROPERTY : GUIDER_GUIDE_DEC_PROPERTY;
	if (!asi_stop_axis(device, ra)) {
		indigo_update_property(device, property, "Failed to stop the active guide relay");
		return;
	}
	if (duration <= 0) {
		indigo_update_property(device, property, NULL);
		return;
	}
	USB2ST4_ERROR_CODE result = asi_set_relay(device, direction, true);
	if (result != USB2ST4_SUCCESS) {
		property->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, property, "Failed to start the guide relay");
		return;
	}
	*active = direction;
	*deadline = indigo_monotonic_time() + duration / 1000.0;
	property->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, property, NULL);
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, finalizer);
}

//- code

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = asi_open(device);
		if (connection_result) {
			//+ guider.on_connect
			GUIDER_GUIDE_RA_PROPERTY->hidden = false;
			GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
			//- guider.on_connect
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
		//+ guider.on_disconnect
		bool ra_stopped = asi_stop_axis(device, true);
		bool dec_stopped = asi_stop_axis(device, false);
		if (!ra_stopped || !dec_stopped) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to stop every guide relay before disconnecting device %d", PRIVATE_DATA->dev_id);
		}
		GUIDER_GUIDE_RA_PROPERTY->hidden = true;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = true;
		//- guider.on_disconnect
		asi_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	int direction = USB2ST4_NORTH;
	if (duration <= 0) {
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
		direction = USB2ST4_SOUTH;
	}
	asi_change_axis(device, false, direction, duration, guider_guide_dec_finalizer);
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	int direction = USB2ST4_EAST;
	if (duration <= 0) {
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
		direction = USB2ST4_WEST;
	}
	asi_change_axis(device, true, direction, duration, guider_guide_ra_finalizer);
	//- guider.GUIDER_GUIDE_RA.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = true;
		GUIDER_GUIDE_RA_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

#define SDK_DISCOVERY_RETRIES (6)
typedef struct sdk_discovery_retry {
	libusb_device *dev;
	int remaining;
	bool active, queued;
	struct sdk_discovery_retry *next;
} sdk_discovery_retry;

static sdk_discovery_retry *sdk_discovery_retries;
static bool sdk_discovery_stopping;
static void process_plug_event_handler(indigo_device *device, void *data);
static void process_sdk_retry_handler(indigo_device *device, void *data);

static void update_sdk_discovery_retry(libusb_device *dev, bool retry) {
	sdk_discovery_retry *entry = sdk_discovery_retries;
	while (entry && entry->dev != dev) {
		entry = entry->next;
	}
	if (!retry || sdk_discovery_stopping) {
		if (entry) {
			entry->active = false;
		}
		return;
	}
	if (!entry && SDK_DISCOVERY_RETRIES <= 0) {
		return;
	}
	if (!entry) {
		entry = (sdk_discovery_retry *)indigo_safe_malloc(sizeof(*entry));
		entry->dev = libusb_ref_device(dev);
		entry->remaining = SDK_DISCOVERY_RETRIES;
		entry->active = true;
		entry->next = sdk_discovery_retries;
		sdk_discovery_retries = entry;
	}
	if (entry->active && !entry->queued && entry->remaining > 0) {
		entry->remaining--;
		entry->queued = true;
		indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0.5, process_sdk_retry_handler, entry, &driver_queue_mutex);
	}
}

static void process_sdk_retry_handler(indigo_device *device, void *data) {
	sdk_discovery_retry *entry = (sdk_discovery_retry *)data;
	entry->queued = false;
	if (entry->active && !sdk_discovery_stopping) {
		process_plug_event_handler(NULL, libusb_ref_device(entry->dev));
	}
	if (!entry->queued) {
		sdk_discovery_retry **link = &sdk_discovery_retries;
		while (*link != entry) {
			link = &(*link)->next;
		}
		*link = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}

static void clear_sdk_discovery_retries(void) {
	while (sdk_discovery_retries) {
		sdk_discovery_retry *entry = sdk_discovery_retries;
		sdk_discovery_retries = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}
static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	if (sdk_discovery_stopping) {
		libusb_unref_device(dev);
		return;
	}
	bool dev_ref_transferred = false;
	asi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (asi_private_data *)indigo_safe_malloc(sizeof(asi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == ASI_VENDOR_ID)) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = asi_pid_supported(descriptor.idProduct);
		if (plug_result) {
			plug_result = false;
			int count = asi_get_num();
			for (int index = 0; index < count && index < USB2ST4_ID_MAX; index++) {
				int id = NO_DIRECTION;
				if (asi_get_id(index, &id) != USB2ST4_SUCCESS || id < 0 || id >= USB2ST4_ID_MAX) {
					continue;
				}
				bool attached = false;
				for (int slot = 0; slot < MAX_DEVICES; slot++) {
					if (devices[slot] != NULL && ((asi_private_data *)devices[slot]->private_data)->dev_id == id) {
						attached = true;
						break;
					}
				}
				if (!attached) {
					private_data->dev_id = id;
					private_data->active_ra = private_data->active_dec = NO_DIRECTION;
					snprintf(name, INDIGO_NAME_SIZE, "ASI USB-St4 Guider #%d", id);
					plug_result = true;
					break;
				}
			}
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		guider->private_data = private_data;
		snprintf(guider->name, INDIGO_NAME_SIZE, "%s", name);
		bool guider_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = guider;
				if (indigo_attach_device(guider) == INDIGO_OK) {
					dev_ref_transferred = true;
					guider_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!guider_attached) {
			indigo_safe_free(guider);
		}
	}
	update_sdk_discovery_retry(dev, discovery_eligible && !dev_ref_transferred);
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	update_sdk_discovery_retry(dev, false);
	asi_private_data *private_data = NULL;
	asi_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				int count = asi_get_num();
				bool complete = count >= 0 && count <= USB2ST4_ID_MAX;
				bool present = false;
				for (int index = 0; complete && index < count; index++) {
					int id = NO_DIRECTION;
					if (asi_get_id(index, &id) != USB2ST4_SUCCESS || id < 0 || id >= USB2ST4_ID_MAX) {
						complete = false;
					} else if (id == private_data->dev_id) {
						present = true;
					}
				}
				unplug_result = complete && !present;
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
				bool recorded = false;
				for (int k = 0; k < removed_count; k++) {
					if (removed[k] == private_data) {
						recorded = true;
						break;
					}
				}
				if (!recorded) {
					removed[removed_count++] = private_data;
				}
			}
		}
	}
	for (int k = 0; k < removed_count; k++) {
		libusb_unref_device(removed[k]->usbdev);
		indigo_safe_free(removed[k]);
	}
	libusb_unref_device(dev);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_plug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_unplug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_guider_asi(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			if (!asi_load_products()) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get the list of supported IDs");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			sdk_discovery_stopping = false;
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			if (shutdown_result == INDIGO_OK) {
				sdk_discovery_stopping = true;
			}
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_remove(driver_queue, NULL, (indigo_timer_callback)process_sdk_retry_handler);
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			clear_sdk_discovery_retries();
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
#else
#include "indigo_guider_asi.h"

indigo_result indigo_guider_asi(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, "ZWO ASI USB-St4 Guider", __FUNCTION__, 0x03000007, false, INDIGO_DRIVER_SHUTDOWN);
	return action == INDIGO_DRIVER_INFO ? INDIGO_OK : INDIGO_UNSUPPORTED_ARCH;
}
#endif
