// Copyright (C) 2016 Rumen G. Bogdanovski
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

// This file generated from indigo_wheel_asi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

//+ include

#include <EFW_filter.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_asi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000E
#define DRIVER_NAME          "indigo_wheel_asi"
#define DRIVER_LABEL         "ZWO ASI Filter Wheel"
#define WHEEL_DEVICE_NAME    "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((asi_private_data *)device->private_data)

//+ define

#define ASI_VENDOR_ID        0x03c3
#define NO_DEVICE            -1
#define ADVANCED_GROUP       "Advanced"

//- define

#pragma mark - Property definitions

#define X_CALIBRATE_PROPERTY           (PRIVATE_DATA->x_calibrate_property)
#define X_CALIBRATE_START_ITEM         (X_CALIBRATE_PROPERTY->items + 0)

#define X_CALIBRATE_PROPERTY_NAME      "X_CALIBRATE"
#define X_CALIBRATE_START_ITEM_NAME    "START"

#define X_CUSTOM_SUFFIX_PROPERTY       (PRIVATE_DATA->x_custom_suffix_property)
#define X_CUSTOM_SUFFIX_ITEM           (X_CUSTOM_SUFFIX_PROPERTY->items + 0)

#define X_CUSTOM_SUFFIX_PROPERTY_NAME  "X_CUSTOM_SUFFIX"
#define X_CUSTOM_SUFFIX_ITEM_NAME      "SUFFIX"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *x_calibrate_property;
	indigo_property *x_custom_suffix_property;
	//+ data
	int dev_id;
	char model[64];
	char custom_suffix[9];
	int current_slot, target_slot;
	int slot_count;
	//- data
} asi_private_data;

#pragma mark - Low level code

//+ code

static int efw_products[100];
static int efw_id_count = 0;
static bool connected_ids[EFW_ID_MAX];
static pthread_mutex_t indigo_device_enumeration_mutex = PTHREAD_MUTEX_INITIALIZER;

static void split_device_name(const char *name, char *model, char *custom_suffix) {
	snprintf(model, 64, "%s", name);
	custom_suffix[0] = 0;
	char *suffix = strrchr(model, '#');
	if (suffix == NULL) {
		return;
	}
	*suffix++ = 0;
	size_t length = strlen(model);
	while (length > 0 && model[length - 1] == ' ') {
		model[--length] = 0;
	}
	strncpy(custom_suffix, suffix, 8);
	custom_suffix[8] = 0;
}

static bool asi_open(indigo_device *device) {
	bool result = false;
	bool global_locked = false;
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
	} else {
		global_locked = true;
		int res = EFWOpen(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
		result = res == EFW_SUCCESS;
	}
	if (!result && global_locked) {
		indigo_global_unlock(device);
	}
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
	return result;
}

static void asi_close(indigo_device *device) {
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	if (PRIVATE_DATA->dev_id >= 0 && PRIVATE_DATA->dev_id < EFW_ID_MAX) {
		int res = EFWClose(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWClose(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_global_unlock(device);
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

//- code

//+ wheel.code

static void wheel_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int res = EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_slot, res);
	PRIVATE_DATA->current_slot++;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_calibrate_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	int pos = 0;
	int res = EFWGetPosition(PRIVATE_DATA->dev_id, &pos);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, pos, res);
	if (pos == -1) {
		indigo_execute_handler_in(device, 1, wheel_calibrate_finalizer);
		return;
	}
	WHEEL_SLOT_ITEM->number.value =
	WHEEL_SLOT_ITEM->number.target =
	PRIVATE_DATA->current_slot =
	PRIVATE_DATA->target_slot = ++pos;
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	X_CALIBRATE_START_ITEM->sw.value = false;
	X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration finished");
}

//- wheel.code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = asi_open(device);
		if (connection_result) {
			//+ wheel.on_connect
			EFW_INFO info;
			EFWGetProperty(PRIVATE_DATA->dev_id, &info);
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->slot_count = info.slotNum;
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
			INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
			int res = EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_slot, res);
			PRIVATE_DATA->current_slot++;
			PRIVATE_DATA->target_slot = PRIVATE_DATA->current_slot;
			WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot;
			//- wheel.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);
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
		indigo_delete_property(device, X_CALIBRATE_PROPERTY, NULL);
		indigo_delete_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		asi_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	if(WHEEL_SLOT_PROPERTY->state == INDIGO_BUSY_STATE) {
		return;
	}
	if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->target_slot = (int)WHEEL_SLOT_ITEM->number.value;
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
		int res = EFWSetPosition(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot - 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWSetPosition(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot - 1, res);
		if (res == EFW_SUCCESS) {
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		}
	}
	//- wheel.WHEEL_SLOT.on_change
}

static void wheel_x_calibrate_handler(indigo_device *device) {
	//+ wheel.X_CALIBRATE.on_change
	if (X_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE || WHEEL_SLOT_PROPERTY->state == INDIGO_BUSY_STATE) {
		return;
	}
	if (X_CALIBRATE_START_ITEM->sw.value) {
		X_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration started");
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		int res = EFWCalibrate(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWCalibrate(%d) = %d", PRIVATE_DATA->dev_id, res);
		if (res == EFW_SUCCESS) {
			indigo_execute_handler_in(device, 1, wheel_calibrate_finalizer);
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			X_CALIBRATE_START_ITEM->sw.value = false;
			X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration failed");
		}
	}
	//- wheel.X_CALIBRATE.on_change
}

static void wheel_x_custom_suffix_handler(indigo_device *device) {
	//+ wheel.X_CUSTOM_SUFFIX.on_change
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
	if (strlen(X_CUSTOM_SUFFIX_ITEM->text.value) > 8) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Custom suffix too long");
		return;
	}
	EFW_ID efw_id = {0};
	memcpy(efw_id.id, X_CUSTOM_SUFFIX_ITEM->text.value, 8);
	int res = EFWSetID(PRIVATE_DATA->dev_id, efw_id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFWSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, res);
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		if (res == EFW_ERROR_NOT_SUPPORTED) {
			indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Custom suffix is not supported by this filter wheel firmware");
		} else {
			indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		}
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, res);
	memset(PRIVATE_DATA->custom_suffix, 0, sizeof(PRIVATE_DATA->custom_suffix));
	strncpy(PRIVATE_DATA->custom_suffix, X_CUSTOM_SUFFIX_ITEM->text.value, sizeof(PRIVATE_DATA->custom_suffix) - 1);
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	if (strlen(X_CUSTOM_SUFFIX_ITEM->text.value) > 0) {
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Filter wheel name suffix '#%s' will be used on replug", X_CUSTOM_SUFFIX_ITEM->text.value);
	} else {
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Filter wheel name suffix cleared, will be used on replug");
	}
	return;
	//- wheel.X_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 6;
		const char *sdk_version = EFWGetSDKVersion();
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		X_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_PROPERTY_NAME, ADVANCED_GROUP, "Calibrate filter wheel", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_START_ITEM, X_CALIBRATE_START_ITEM_NAME, "Start", false);
		X_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, X_CUSTOM_SUFFIX_PROPERTY_NAME, WHEEL_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
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
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CUSTOM_SUFFIX_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, wheel_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_PROPERTY, wheel_x_calibrate_handler);
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
	indigo_release_property(X_CALIBRATE_PROPERTY);
	indigo_release_property(X_CUSTOM_SUFFIX_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Hot-plug code

static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *devices[MAX_DEVICES];

static void process_plug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->usbdev = dev;
	libusb_ref_device(dev);
	//+ sdk.plug
	plug_result = false;
	struct libusb_device_descriptor descriptor;
	if (libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == ASI_VENDOR_ID) {
		for (int i = 0; i < efw_id_count; i++) {
			if (efw_products[i] == descriptor.idProduct) {
				plug_result = true;
				break;
			}
		}
	}
	if (plug_result) {
		plug_result = false;
		private_data->dev_id = NO_DEVICE;
		pthread_mutex_lock(&indigo_device_enumeration_mutex);
		int count = EFWGetNum();
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetNum() = %d", count);
		for (int index = 0; index < count; index++) {
			int id = NO_DEVICE;
			int res = EFWGetID(index, &id);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetID(%d, -> %d) = %d", index, id, res);
			if (res != EFW_SUCCESS || id < 0 || id >= EFW_ID_MAX || connected_ids[id]) {
				continue;
			}
			res = EFWOpen(id);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWOpen(%d) = %d", id, res);
			if (res != EFW_SUCCESS) {
				continue;
			}
			EFW_INFO info = {0};
			res = EFWGetProperty(id, &info);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetProperty(%d) = %d", id, res);
			EFWClose(id);
			if (res == EFW_SUCCESS) {
				private_data->dev_id = id;
				connected_ids[id] = true;
				split_device_name(info.Name, private_data->model, private_data->custom_suffix);
				snprintf(name, INDIGO_NAME_SIZE, "%s", info.Name);
				plug_result = true;
				break;
			}
		}
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
	}
	//- sdk.plug
	if (plug_result) {
		indigo_device *wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		wheel->private_data = private_data;
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s", name);
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				indigo_async((void *)(void *)indigo_attach_device, devices[j] = wheel);
				break;
			}
		}
	} else {
		libusb_unref_device(dev);
		free(private_data);
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	asi_private_data *private_data = NULL;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				//+ sdk.unplug
				if (private_data->dev_id >= 0 && private_data->dev_id < EFW_ID_MAX) {
					connected_ids[private_data->dev_id] = false;
				}
				//- sdk.unplug
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		free(private_data);
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_wheel_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			const char *sdk_version = EFWGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "EFW SDK v. %s", sdk_version);

			for (int index = 0; index < EFW_ID_MAX; index++) {
				connected_ids[index] = false;
			}
			efw_id_count = EFWGetProductIDs(efw_products);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EFWGetProductIDs(-> [ %d, %d, ... ]) = %d", efw_products[0], efw_products[1], efw_id_count);
			if (efw_id_count <= 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get the list of supported IDs.");
				return INDIGO_FAILED;
			}
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
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
					hotplug_callback(NULL, PRIVATE_DATA->usbdev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
				}
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
