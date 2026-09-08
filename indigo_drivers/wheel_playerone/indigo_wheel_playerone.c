// Copyright (C) 2023-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_wheel_playerone.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <PlayerOnePW.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_playerone.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000A
#define DRIVER_NAME          "indigo_wheel_playerone"
#define DRIVER_LABEL         "Player One Filter Wheel"
#define WHEEL_DEVICE_NAME    "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((playerone_private_data *)device->private_data)

//+ define

#define PONE_VENDOR_ID       0xa0a0

//- define

#pragma mark - Property definitions

#define X_RESET_PROPERTY               (PRIVATE_DATA->x_reset_property)
#define X_RESET_ITEM                   (X_RESET_PROPERTY->items + 0)

#define X_RESET_PROPERTY_NAME          "X_RESET"
#define X_RESET_ITEM_NAME              "RESET"

#define X_CUSTOM_SUFFIX_PROPERTY       (PRIVATE_DATA->x_custom_suffix_property)
#define X_CUSTOM_SUFFIX_ITEM           (X_CUSTOM_SUFFIX_PROPERTY->items + 0)

#define X_CUSTOM_SUFFIX_PROPERTY_NAME  "X_CUSTOM_SUFFIX"
#define X_CUSTOM_SUFFIX_ITEM_NAME      "SUFFIX"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *x_reset_property;
	indigo_property *x_custom_suffix_property;
	//+ data
	int dev_handle;
	char model[64];
	char custom_suffix[MAX_NAME_LEN + 1];
	int current_slot, target_slot, slot_count;
	int initialization_polls;
	int count;
	//- data
} playerone_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static void split_device_name(const char *name, char *model, char *suffix) {
	snprintf(model, 64, "%s", name);
	suffix[0] = 0;
	char *start = strchr(model, '[');
	char *end = strrchr(model, ']');
	if (start == NULL || end == NULL || end < start || end[1] != 0 || end - start - 1 > MAX_NAME_LEN) {
		return;
	}
	*end = 0;
	snprintf(suffix, MAX_NAME_LEN + 1, "%s", start + 1);
	*start = 0;
	while (start > model && start[-1] == ' ') {
		*--start = 0;
	}
}

static bool playerone_open(indigo_device *device) {
	if (PRIVATE_DATA->count > 0) {
		PRIVATE_DATA->count++;
		return true;
	}
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		return false;
	}
	int res = POAOpenPW(PRIVATE_DATA->dev_handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAOpenPW(%d) = %d", PRIVATE_DATA->dev_handle, res);
	if (res != PW_OK) {
		indigo_global_unlock(device);
		return false;
	}
	PRIVATE_DATA->count++;
	return true;
}

static void playerone_close(indigo_device *device) {
	if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
		int res = POAClosePW(PRIVATE_DATA->dev_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAClosePW(%d) = %d", PRIVATE_DATA->dev_handle, res);
		indigo_global_unlock(device);
	}
}

//- code

//+ wheel.code

static void wheel_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int position = -1;
	int res = POAGetCurrentPosition(PRIVATE_DATA->dev_handle, &position);
	if (res == PW_ERROR_IS_MOVING) {
		if (PRIVATE_DATA->target_slot == 0 && --PRIVATE_DATA->initialization_polls == 0) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Initial positioning timed out after 15 seconds");
		} else {
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		}
		return;
	}
	if (res != PW_OK || position < 0 || position >= PRIVATE_DATA->slot_count) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Failed to read filter position (%d)", res);
		return;
	}
	PRIVATE_DATA->current_slot = position + 1;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->target_slot == 0) {
		WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->target_slot = PRIVATE_DATA->current_slot;
	}
	WHEEL_SLOT_PROPERTY->state = PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

//- wheel.code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = playerone_open(device);
		if (connection_result) {
			//+ wheel.on_connect
			PWProperties info = { 0 };
			int res = POAGetPWPropertiesByHandle(PRIVATE_DATA->dev_handle, &info);
			connection_result = res == PW_OK && info.Handle == PRIVATE_DATA->dev_handle && info.PositionCount > 0 && info.PositionCount <= WHEEL_SLOT_NAME_PROPERTY->allocated_count && info.PositionCount <= WHEEL_SLOT_OFFSET_PROPERTY->allocated_count;
			int position = -1;
			if (connection_result) {
				res = POAGetCurrentPosition(PRIVATE_DATA->dev_handle, &position);
				connection_result = res == PW_ERROR_IS_MOVING || (res == PW_OK && position >= 0 && position < info.PositionCount);
			}
			if (connection_result) {
				char suffix[MAX_NAME_LEN + 1] = { 0 };
				connection_result = POAGetPWCustomName(PRIVATE_DATA->dev_handle, suffix, sizeof(suffix)) == PW_OK;
				suffix[MAX_NAME_LEN] = 0;
				if (connection_result) {
					snprintf(PRIVATE_DATA->custom_suffix, sizeof(PRIVATE_DATA->custom_suffix), "%s", suffix);
					INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, suffix);
					X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
			if (connection_result) {
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->slot_count = info.PositionCount;
				X_RESET_ITEM->sw.value = false;
				X_RESET_PROPERTY->state = INDIGO_OK_STATE;
				if (res == PW_ERROR_IS_MOVING) {
					PRIVATE_DATA->target_slot = 0;
					PRIVATE_DATA->initialization_polls = 30;
					WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
				} else {
					WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = PRIVATE_DATA->target_slot = position + 1;
					WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				}
			} else {
				playerone_close(device);
			}
			//- wheel.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_RESET_PROPERTY, NULL);
			indigo_define_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_RESET_PROPERTY, NULL);
		indigo_delete_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		playerone_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	double target = WHEEL_SLOT_ITEM->number.value;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (!isfinite(target) || target != floor(target) || target < 1 || target > PRIVATE_DATA->slot_count || X_RESET_PROPERTY->state == INDIGO_BUSY_STATE) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Invalid slot or wheel reset in progress");
	} else if (target == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	} else {
		int res = POAGotoPosition(PRIVATE_DATA->dev_handle, (int)target - 1);
		if (res != PW_OK) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Set filter failed (%d)", res);
		} else {
			PRIVATE_DATA->target_slot = (int)target;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		}
	}
	//- wheel.WHEEL_SLOT.on_change
}

static void wheel_x_reset_handler(indigo_device *device) {
	X_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_RESET.on_change
	if (X_RESET_ITEM->sw.value) {
		X_RESET_ITEM->sw.value = false;
		if (WHEEL_SLOT_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_RESET_PROPERTY, "Wheel is busy");
			return;
		}
		int res = POAResetPW(PRIVATE_DATA->dev_handle);
		if (res != PW_OK) {
			X_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			indigo_update_property(device, X_RESET_PROPERTY, "Filter wheel reset successful, disconnecting...");
			indigo_change_switch_property_1(NULL, device->name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
			return;
		}
	}
	//- wheel.X_RESET.on_change
	indigo_update_property(device, X_RESET_PROPERTY, NULL);
}

static void wheel_x_custom_suffix_handler(indigo_device *device) {
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_CUSTOM_SUFFIX.on_change
	size_t length = strlen(X_CUSTOM_SUFFIX_ITEM->text.value);
	if (length > MAX_NAME_LEN) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Custom suffix is too long");
		return;
	}
	int res = POASetPWCustomName(PRIVATE_DATA->dev_handle, X_CUSTOM_SUFFIX_ITEM->text.value, (int)length);
	if (res != PW_OK) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
	} else {
		snprintf(PRIVATE_DATA->custom_suffix, sizeof(PRIVATE_DATA->custom_suffix), "%s", X_CUSTOM_SUFFIX_ITEM->text.value);
		indigo_send_message(device, X_CUSTOM_SUFFIX_PROPERTY, "Filter wheel name suffix will be used on replug");
	}
	//- wheel.X_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, POAGetPWSDKVer());
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		X_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RESET_PROPERTY_NAME, WHEEL_ADVANCED_GROUP, "Reset filter wheel", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RESET_ITEM, X_RESET_ITEM_NAME, "Reset", false);
		X_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, X_CUSTOM_SUFFIX_PROPERTY_NAME, WHEEL_MAIN_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_CUSTOM_SUFFIX_ITEM, X_CUSTOM_SUFFIX_ITEM_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CUSTOM_SUFFIX_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, wheel_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RESET_PROPERTY, wheel_x_reset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CUSTOM_SUFFIX_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CUSTOM_SUFFIX_PROPERTY, wheel_x_custom_suffix_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	indigo_release_property(X_RESET_PROPERTY);
	indigo_release_property(X_CUSTOM_SUFFIX_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	playerone_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = indigo_safe_malloc(sizeof(playerone_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == PONE_VENDOR_ID && descriptor.idProduct == 0xf001)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		bool duplicate = false;
		for (int slot = 0; slot < MAX_DEVICES; slot++) {
			if (devices[slot] != NULL && ((playerone_private_data *)devices[slot]->private_data)->usbdev == dev) {
				duplicate = true;
				break;
			}
		}
		int count = duplicate ? 0 : POAGetPWCount();
		for (int index = 0; index < count; index++) {
			PWProperties info = { 0 };
			int res = POAGetPWProperties(index, &info);
			if (res != PW_OK || info.Handle < 0) {
				continue;
			}
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && ((playerone_private_data *)devices[slot]->private_data)->dev_handle == info.Handle) {
					attached = true;
					break;
				}
			}
			if (!attached) {
				info.Name[sizeof(info.Name) - 1] = 0;
				private_data->dev_handle = info.Handle;
				split_device_name(info.Name, private_data->model, private_data->custom_suffix);
				if (private_data->custom_suffix[0]) {
					snprintf(name, INDIGO_NAME_SIZE, "%.*s #%s", INDIGO_NAME_SIZE - MAX_NAME_LEN - 3, private_data->model, private_data->custom_suffix);
				} else {
					snprintf(name, INDIGO_NAME_SIZE, "%s", private_data->model);
				}
				indigo_make_name_unique(name, "%d", info.Handle);
				plug_result = true;
				break;
			}
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
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
			free(wheel);
		}
	}
	if (!dev_ref_transferred) {
		free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	playerone_private_data *private_data = NULL;
	playerone_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				int count = POAGetPWCount();
				unplug_result = count >= 0;
				for (int index = 0; index < count; index++) {
					PWProperties info = { 0 };
					if (POAGetPWProperties(index, &info) != PW_OK || info.Handle == private_data->dev_handle) {
						unplug_result = false;
						break;
					}
				}
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				free(device);
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
		free(removed[k]);
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

indigo_result indigo_wheel_playerone(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Player One filter wheel SDK v. %s", POAGetPWSDKVer());
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, PONE_VENDOR_ID, 0xf001, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++) {
				VERIFY_NOT_CONNECTED(devices[i]);
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
