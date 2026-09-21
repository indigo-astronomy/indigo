// Copyright (c) 2024-2026 Moravian Instruments
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

// This file generated from indigo_wheel_mi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <gxccd.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_mi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_wheel_mi"
#define DRIVER_LABEL         "Moravian Instruments SFW"
#define WHEEL_DEVICE_NAME    "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((mi_private_data *)device->private_data)

//+ define

#define MI_VID               0x1347
#define MI_MAX_ENUMERATED_IDS 64

//- define

#pragma mark - Property definitions

#define X_MI_SFW_COMMANDS_PROPERTY      (PRIVATE_DATA->x_mi_sfw_commands_property)
#define REINIT_ITEM                     (X_MI_SFW_COMMANDS_PROPERTY->items + 0)

#define X_MI_SFW_COMMANDS_PROPERTY_NAME "X_MI_SFW_COMMANDS"
#define REINIT_ITEM_NAME                "MI_SFW_REINIT"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *x_mi_sfw_commands_property;
	//+ data
	int eid;
	fwheel_t *wheel;
	int current_slot;
	int slot_count;
	bool move_pending;
	bool reinit_pending;
	char model[INDIGO_NAME_SIZE];
	//- data
} mi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static int enumerated_ids[MI_MAX_ENUMERATED_IDS];
static int enumerated_id_count;

static void enumerate_callback(int eid) {
	if (enumerated_id_count < MI_MAX_ENUMERATED_IDS) {
		enumerated_ids[enumerated_id_count++] = eid;
	}
}

static int enumerate_wheels(void) {
	enumerated_id_count = 0;
	gxfw_enumerate_usb(enumerate_callback);
	return enumerated_id_count;
}

static bool wheel_is_enumerated(int eid) {
	int count = enumerate_wheels();
	for (int i = 0; i < count; i++) {
		if (enumerated_ids[i] == eid) {
			return true;
		}
	}
	return false;
}

static void trim_trailing_space(char *text) {
	size_t length = strlen(text);
	while (length > 0 && isspace((unsigned char)text[length - 1])) {
		text[--length] = 0;
	}
}

static void report_error(indigo_device *device, indigo_property *property, const char *operation) {
	char message[128] = "Moravian Instruments SDK error";
	if (PRIVATE_DATA->wheel != NULL) {
		gxfw_get_last_error(PRIVATE_DATA->wheel, message, sizeof(message));
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed: %s", operation, message);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

static bool mi_open(indigo_device *device) {
	PRIVATE_DATA->wheel = gxfw_initialize_usb(PRIVATE_DATA->eid);
	if (PRIVATE_DATA->wheel == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "gxfw_initialize_usb(%d) failed", PRIVATE_DATA->eid);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gxfw_initialize_usb(%d) succeeded", PRIVATE_DATA->eid);
	return true;
}

static void mi_close(indigo_device *device) {
	indigo_lock_master_device(device);
	if (PRIVATE_DATA->wheel != NULL) {
		gxfw_release(PRIVATE_DATA->wheel);
		PRIVATE_DATA->wheel = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gxfw_release() succeeded");
	}
	indigo_unlock_master_device(device);
}

//- code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = mi_open(device);
		if (connection_result) {
			//+ wheel.on_connect
			int filters = 0;
			connection_result = gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_FILTERS, &filters) == 0 && filters > 0 && filters <= WHEEL_SLOT_NAME_PROPERTY->allocated_count && filters <= WHEEL_SLOT_OFFSET_PROPERTY->allocated_count;
			if (connection_result) {
				PRIVATE_DATA->slot_count = filters;
				WHEEL_SLOT_ITEM->number.min = 1;
				WHEEL_SLOT_ITEM->number.max = filters;
				WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = filters;
				char value[INDIGO_VALUE_SIZE] = { 0 };
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
				if (gxfw_get_string_parameter(PRIVATE_DATA->wheel, FW_GSP_DESCRIPTION, value, sizeof(value)) == 0) {
					value[sizeof(value) - 1] = 0;
					trim_trailing_space(value);
					if (value[0]) {
						INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, value);
					}
				}
				if (gxfw_get_string_parameter(PRIVATE_DATA->wheel, FW_GSP_SERIAL_NUMBER, value, sizeof(value)) == 0) {
					value[sizeof(value) - 1] = 0;
					INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, value);
				} else {
					INFO_DEVICE_SERIAL_NUM_ITEM->text.value[0] = 0;
				}
				int version[4] = { 0 };
				bool has_version = true;
				for (int i = 0; i < 4; i++) {
					if (gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_VERSION_1 + i, version + i) != 0) {
						has_version = false;
						break;
					}
				}
				if (has_version) {
					snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%d.%d.%d.%d", version[0], version[1], version[2], version[3]);
				} else {
					INFO_DEVICE_FW_REVISION_ITEM->text.value[0] = 0;
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
				REINIT_ITEM->sw.value = false;
				X_MI_SFW_COMMANDS_PROPERTY->state = INDIGO_OK_STATE;
				int result = gxfw_set_filter(PRIVATE_DATA->wheel, 0);
				if (result == 0) {
					PRIVATE_DATA->current_slot = 1;
					WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 1;
					WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					PRIVATE_DATA->current_slot = 0;
					WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} else {
				mi_close(device);
			}
			//- wheel.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_MI_SFW_COMMANDS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ wheel.on_disconnect
		PRIVATE_DATA->current_slot = 0;
		PRIVATE_DATA->slot_count = 0;
		PRIVATE_DATA->move_pending = false;
		PRIVATE_DATA->reinit_pending = false;
		REINIT_ITEM->sw.value = false;
		//- wheel.on_disconnect
		indigo_delete_property(device, X_MI_SFW_COMMANDS_PROPERTY, NULL);
		mi_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.WHEEL_SLOT.on_change
	int slot = (int)WHEEL_SLOT_ITEM->number.value;
	if (slot == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_ITEM->number.target = slot;
	} else {
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot > 0 ? PRIVATE_DATA->current_slot : 1;
		if (gxfw_set_filter(PRIVATE_DATA->wheel, slot - 1) != 0) {
			report_error(device, WHEEL_SLOT_PROPERTY, "gxfw_set_filter()");
			PRIVATE_DATA->move_pending = false;
			return;
		}
		PRIVATE_DATA->current_slot = slot;
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = slot;
	}
	PRIVATE_DATA->move_pending = false;
	//- wheel.WHEEL_SLOT.on_change
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_x_mi_sfw_commands_handler(indigo_device *device) {
	X_MI_SFW_COMMANDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_MI_SFW_COMMANDS.on_change
	if (REINIT_ITEM->sw.value) {
		int filters = 0;
		int result = gxfw_reinit_filter_wheel(PRIVATE_DATA->wheel, &filters);
		if (result != 0 || filters <= 0 || filters > WHEEL_SLOT_NAME_PROPERTY->allocated_count || filters > WHEEL_SLOT_OFFSET_PROPERTY->allocated_count) {
			REINIT_ITEM->sw.value = false;
			if (result != 0) {
				report_error(device, X_MI_SFW_COMMANDS_PROPERTY, "gxfw_reinit_filter_wheel()");
			} else {
				X_MI_SFW_COMMANDS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_MI_SFW_COMMANDS_PROPERTY, "Invalid filter count returned by SDK");
			}
			PRIVATE_DATA->reinit_pending = false;
			return;
		}
		PRIVATE_DATA->slot_count = filters;
		PRIVATE_DATA->current_slot = 1;
		WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_NAME_PROPERTY->allocated_count;
		WHEEL_SLOT_OFFSET_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->allocated_count;
		indigo_delete_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		indigo_delete_property(device, WHEEL_SLOT_OFFSET_PROPERTY, NULL);
		WHEEL_SLOT_ITEM->number.max = filters;
		WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = filters;
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 1;
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Filter count changed to %d", filters);
		indigo_define_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		indigo_define_property(device, WHEEL_SLOT_OFFSET_PROPERTY, NULL);
	}
	REINIT_ITEM->sw.value = false;
	PRIVATE_DATA->reinit_pending = false;
	//- wheel.X_MI_SFW_COMMANDS.on_change
	indigo_update_property(device, X_MI_SFW_COMMANDS_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		X_MI_SFW_COMMANDS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MI_SFW_COMMANDS_PROPERTY_NAME, MAIN_GROUP, "Commands", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_MI_SFW_COMMANDS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(REINIT_ITEM, REINIT_ITEM_NAME, "Reinit Filter Wheel", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MI_SFW_COMMANDS_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, wheel_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->reinit_pending, WHEEL_SLOT_PROPERTY, "Wheel reinitialization is in progress");
		//+ wheel.WHEEL_SLOT.on_change_request
		PRIVATE_DATA->move_pending = true;
		//- wheel.WHEEL_SLOT.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MI_SFW_COMMANDS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->move_pending, X_MI_SFW_COMMANDS_PROPERTY, "Wheel movement is in progress");
		//+ wheel.X_MI_SFW_COMMANDS.on_change_request
		PRIVATE_DATA->reinit_pending = true;
		//- wheel.X_MI_SFW_COMMANDS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MI_SFW_COMMANDS_PROPERTY, wheel_x_mi_sfw_commands_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	indigo_release_property(X_MI_SFW_COMMANDS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

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
	mi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (mi_private_data *)indigo_safe_malloc(sizeof(mi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == MI_VID)) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		int count = enumerate_wheels();
		for (int i = 0; i < count; i++) {
			int eid = enumerated_ids[i];
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && ((mi_private_data *)devices[slot]->private_data)->eid == eid) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			fwheel_t *wheel = gxfw_initialize_usb(eid);
			if (wheel == NULL) {
				continue;
			}
			char description[INDIGO_NAME_SIZE] = "SFW";
			if (gxfw_get_string_parameter(wheel, FW_GSP_DESCRIPTION, description, sizeof(description)) != 0) {
				snprintf(description, sizeof(description), "SFW");
			}
			gxfw_release(wheel);
			description[sizeof(description) - 1] = 0;
			trim_trailing_space(description);
			private_data->eid = eid;
			snprintf(private_data->model, sizeof(private_data->model), "%s", description[0] ? description : "SFW");
			snprintf(name, INDIGO_NAME_SIZE, "MI %s", private_data->model);
			indigo_make_name_unique(name, "%d", eid);
			plug_result = true;
			break;
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *wheel = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		wheel->private_data = private_data;
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s", name);
		bool wheel_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = wheel;
				if (indigo_attach_device(wheel) == INDIGO_OK) {
					dev_ref_transferred = true;
					wheel_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!wheel_attached) {
			indigo_safe_free(wheel);
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
	mi_private_data *private_data = NULL;
	mi_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				unplug_result = !wheel_is_enumerated(private_data->eid);
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

indigo_result indigo_wheel_mi(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, MI_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
