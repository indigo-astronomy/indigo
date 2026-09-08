// Copyright (C) 2016-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_wheel_astroasis.driver

// supported_architecture: !defined(__i386__)
#if !defined(__i386__)

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <OasisFilterWheel.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_astroasis.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_wheel_astroasis"
#define DRIVER_LABEL         "Astroasis Oasis Wheel"
#define WHEEL_DEVICE_NAME    "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((astroasis_private_data *)device->private_data)

//+ define

#define OPERATION_POLLS      240

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

#define X_BLUETOOTH_PROPERTY           (PRIVATE_DATA->x_bluetooth_property)
#define X_BLUETOOTH_ON_ITEM            (X_BLUETOOTH_PROPERTY->items + 0)
#define X_BLUETOOTH_OFF_ITEM           (X_BLUETOOTH_PROPERTY->items + 1)

#define X_BLUETOOTH_PROPERTY_NAME      "X_BLUETOOTH_PROPERTY"
#define X_BLUETOOTH_ON_ITEM_NAME       "ENABLED"
#define X_BLUETOOTH_OFF_ITEM_NAME      "DISABLED"

#define X_BLUETOOTH_NAME_PROPERTY      (PRIVATE_DATA->x_bluetooth_name_property)
#define X_BLUETOOTH_NAME_ITEM          (X_BLUETOOTH_NAME_PROPERTY->items + 0)

#define X_BLUETOOTH_NAME_PROPERTY_NAME "X_BLUETOOTH_NAME_PROPERTY"
#define X_BLUETOOTH_NAME_ITEM_NAME     "BLUETOOTH_NAME"

#define X_FACTORY_RESET_PROPERTY       (PRIVATE_DATA->x_factory_reset_property)
#define X_FACTORY_RESET_ITEM           (X_FACTORY_RESET_PROPERTY->items + 0)

#define X_FACTORY_RESET_PROPERTY_NAME  "X_FACTORY_RESET"
#define X_FACTORY_RESET_ITEM_NAME      "RESET"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *x_calibrate_property;
	indigo_property *x_custom_suffix_property;
	indigo_property *x_bluetooth_property;
	indigo_property *x_bluetooth_name_property;
	indigo_property *x_factory_reset_property;
	//+ data
	int dev_id, count, slot_count;
	int current_slot, target_slot, polls;
	char model[OFW_NAME_LEN + 1];
	char custom_suffix[OFW_NAME_LEN + 1];
	char bluetooth_name[OFW_NAME_LEN + 1];
	OFWConfig config;
	//- data
} astroasis_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static bool astroasis_open(indigo_device *device) {
	if (PRIVATE_DATA->count > 0) {
		PRIVATE_DATA->count++;
		return true;
	}
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		return false;
	}
	int res = OFWOpen(PRIVATE_DATA->dev_id);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
	if (res != AO_SUCCESS) {
		indigo_global_unlock(device);
		return false;
	}
	PRIVATE_DATA->count++;
	return true;
}

static void astroasis_close(indigo_device *device) {
	if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
		int res = OFWClose(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWClose(%d) = %d", PRIVATE_DATA->dev_id, res);
		indigo_global_unlock(device);
	}
}

static bool astroasis_status(indigo_device *device, OFWStatus *status) {
	int res = OFWGetStatus(PRIVATE_DATA->dev_id, status);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWGetStatus(%d) = %d", PRIVATE_DATA->dev_id, res);
	return res == AO_SUCCESS && status->filterStatus >= STATUS_IDLE && status->filterStatus <= STATUS_BENCHMARKING && status->filterPosition >= 0 && status->filterPosition <= PRIVATE_DATA->slot_count && (status->filterStatus != STATUS_IDLE || status->filterPosition > 0);
}

static bool astroasis_idle(indigo_device *device) {
	OFWStatus status = { 0 };
	if (!astroasis_status(device, &status) || status.filterStatus != STATUS_IDLE) {
		return false;
	}
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot = status.filterPosition;
	return true;
}

static bool astroasis_read_names(indigo_device *device) {
	char suffix[OFW_NAME_LEN + 1] = { 0 };
	if (OFWGetFriendlyName(PRIVATE_DATA->dev_id, suffix) != AO_SUCCESS) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		return false;
	}
	suffix[OFW_NAME_LEN] = 0;
	INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, suffix);
	snprintf(PRIVATE_DATA->custom_suffix, sizeof(PRIVATE_DATA->custom_suffix), "%s", suffix);
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	OFWConfig config = { 0 };
	int res = OFWGetConfig(PRIVATE_DATA->dev_id, &config);
	X_BLUETOOTH_PROPERTY->state = res == AO_SUCCESS ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (res == AO_SUCCESS) {
		PRIVATE_DATA->config = config;
		indigo_set_switch(X_BLUETOOTH_PROPERTY, config.bluetoothOn ? X_BLUETOOTH_ON_ITEM : X_BLUETOOTH_OFF_ITEM, true);
	} else if (res != AO_ERROR_NOT_IMPLEMENTED) {
		return false;
	}
	char name[OFW_NAME_LEN + 1] = { 0 };
	res = OFWGetBluetoothName(PRIVATE_DATA->dev_id, name);
	name[OFW_NAME_LEN] = 0;
	X_BLUETOOTH_NAME_PROPERTY->state = res == AO_SUCCESS ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (res == AO_SUCCESS) {
		snprintf(PRIVATE_DATA->bluetooth_name, sizeof(PRIVATE_DATA->bluetooth_name), "%s", name);
		INDIGO_COPY_VALUE(X_BLUETOOTH_NAME_ITEM->text.value, name);
	}
	return res == AO_SUCCESS || res == AO_ERROR_NOT_IMPLEMENTED;
}

//- code

//+ wheel.code

static void wheel_operation_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	OFWStatus status = { 0 };
	bool valid = astroasis_status(device, &status);
	if (valid && status.filterStatus != STATUS_IDLE && --PRIVATE_DATA->polls > 0) {
		indigo_execute_handler_in(device, 0.5, wheel_operation_finalizer);
		return;
	}
	bool success = valid && status.filterStatus == STATUS_IDLE && (PRIVATE_DATA->target_slot == 0 || status.filterPosition == PRIVATE_DATA->target_slot);
	if (valid && status.filterStatus == STATUS_IDLE) {
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot = status.filterPosition;
	}
	WHEEL_SLOT_PROPERTY->state = success ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (success) {
		WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, success ? NULL : "Positioning failed or timed out");
	if (X_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_CALIBRATE_START_ITEM->sw.value = false;
		X_CALIBRATE_PROPERTY->state = success ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, success ? "Calibration finished" : "Calibration failed or timed out");
	}
}

//- wheel.code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = astroasis_open(device);
		if (connection_result) {
			//+ wheel.on_connect
			int slots = 0;
			connection_result = OFWGetSlotNum(PRIVATE_DATA->dev_id, &slots) == AO_SUCCESS && slots > 0 && slots <= WHEEL_SLOT_NAME_PROPERTY->allocated_count && slots <= WHEEL_SLOT_OFFSET_PROPERTY->allocated_count;
			OFWStatus status = { 0 };
			if (connection_result) {
				PRIVATE_DATA->slot_count = slots;
				connection_result = astroasis_status(device, &status) && astroasis_read_names(device);
			}
			if (connection_result) {
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = slots;
				X_CALIBRATE_START_ITEM->sw.value = X_FACTORY_RESET_ITEM->sw.value = false;
				X_CALIBRATE_PROPERTY->state = X_FACTORY_RESET_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->target_slot = 0;
				if (status.filterPosition > 0) {
					WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = status.filterPosition;
				}
				if (status.filterStatus != STATUS_IDLE) {
					WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
					PRIVATE_DATA->polls = OPERATION_POLLS;
					indigo_execute_handler_in(device, 0.5, wheel_operation_finalizer);
				}
			} else {
				astroasis_close(device);
			}
			//- wheel.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);
			indigo_define_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_define_property(device, X_BLUETOOTH_PROPERTY, NULL);
			indigo_define_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
			indigo_define_property(device, X_FACTORY_RESET_PROPERTY, NULL);
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
		indigo_delete_property(device, X_BLUETOOTH_PROPERTY, NULL);
		indigo_delete_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
		indigo_delete_property(device, X_FACTORY_RESET_PROPERTY, NULL);
		astroasis_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	double target = WHEEL_SLOT_ITEM->number.value;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (!isfinite(target) || target != floor(target) || target < 1 || target > PRIVATE_DATA->slot_count || X_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE || X_FACTORY_RESET_PROPERTY->state == INDIGO_BUSY_STATE || !astroasis_idle(device)) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (target != PRIVATE_DATA->current_slot) {
		int res = OFWSetPosition(PRIVATE_DATA->dev_id, (int)target);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWSetPosition(%d, %d) = %d", PRIVATE_DATA->dev_id, (int)target, res);
		if (res != AO_SUCCESS) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->target_slot = (int)target;
			PRIVATE_DATA->polls = OPERATION_POLLS;
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_execute_handler_in(device, 0.5, wheel_operation_finalizer);
			return;
		}
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	//- wheel.WHEEL_SLOT.on_change
}

static void wheel_x_calibrate_handler(indigo_device *device) {
	//+ wheel.X_CALIBRATE.on_change
	X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	if (X_CALIBRATE_START_ITEM->sw.value) {
		X_CALIBRATE_START_ITEM->sw.value = false;
		if (WHEEL_SLOT_PROPERTY->state == INDIGO_BUSY_STATE || X_FACTORY_RESET_PROPERTY->state == INDIGO_BUSY_STATE || !astroasis_idle(device)) {
			X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			int res = OFWCalibrate(PRIVATE_DATA->dev_id, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWCalibrate(%d, 0) = %d", PRIVATE_DATA->dev_id, res);
			if (res != AO_SUCCESS) {
				X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				PRIVATE_DATA->target_slot = 0;
				PRIVATE_DATA->polls = OPERATION_POLLS;
				WHEEL_SLOT_PROPERTY->state = X_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration started");
				indigo_execute_handler_in(device, 0.5, wheel_operation_finalizer);
				return;
			}
		}
	}
	indigo_update_property(device, X_CALIBRATE_PROPERTY, NULL);
	//- wheel.X_CALIBRATE.on_change
}

static void wheel_x_custom_suffix_handler(indigo_device *device) {
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_CUSTOM_SUFFIX.on_change
	if (strlen(X_CUSTOM_SUFFIX_ITEM->text.value) > OFW_NAME_LEN || OFWSetFriendlyName(PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value) != AO_SUCCESS) {
		INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		snprintf(PRIVATE_DATA->custom_suffix, sizeof(PRIVATE_DATA->custom_suffix), "%s", X_CUSTOM_SUFFIX_ITEM->text.value);
		indigo_send_message(device, X_CUSTOM_SUFFIX_PROPERTY, "Filter wheel name suffix will be used on replug");
	}
	//- wheel.X_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
}

static void wheel_x_bluetooth_handler(indigo_device *device) {
	X_BLUETOOTH_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_BLUETOOTH.on_change
	OFWConfig config = PRIVATE_DATA->config;
	config.mask = MASK_BLUETOOTH;
	config.bluetoothOn = X_BLUETOOTH_ON_ITEM->sw.value;
	if (OFWSetConfig(PRIVATE_DATA->dev_id, &config) != AO_SUCCESS) {
		indigo_set_switch(X_BLUETOOTH_PROPERTY, PRIVATE_DATA->config.bluetoothOn ? X_BLUETOOTH_ON_ITEM : X_BLUETOOTH_OFF_ITEM, true);
		X_BLUETOOTH_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->config = config;
	}
	//- wheel.X_BLUETOOTH.on_change
	indigo_update_property(device, X_BLUETOOTH_PROPERTY, NULL);
}

static void wheel_x_bluetooth_name_handler(indigo_device *device) {
	X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_BLUETOOTH_NAME.on_change
	if (strlen(X_BLUETOOTH_NAME_ITEM->text.value) > OFW_NAME_LEN || OFWSetBluetoothName(PRIVATE_DATA->dev_id, X_BLUETOOTH_NAME_ITEM->text.value) != AO_SUCCESS) {
		INDIGO_COPY_VALUE(X_BLUETOOTH_NAME_ITEM->text.value, PRIVATE_DATA->bluetooth_name);
		X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		snprintf(PRIVATE_DATA->bluetooth_name, sizeof(PRIVATE_DATA->bluetooth_name), "%s", X_BLUETOOTH_NAME_ITEM->text.value);
	}
	//- wheel.X_BLUETOOTH_NAME.on_change
	indigo_update_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
}

static void wheel_x_factory_reset_handler(indigo_device *device) {
	X_FACTORY_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.X_FACTORY_RESET.on_change
	if (X_FACTORY_RESET_ITEM->sw.value) {
		X_FACTORY_RESET_ITEM->sw.value = false;
		if (WHEEL_SLOT_PROPERTY->state == INDIGO_BUSY_STATE || X_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE || !astroasis_idle(device) || OFWFactoryReset(PRIVATE_DATA->dev_id) != AO_SUCCESS) {
			X_FACTORY_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			OFWStatus status = { 0 };
			bool names_valid = astroasis_read_names(device);
			bool position_valid = astroasis_status(device, &status) && status.filterStatus == STATUS_IDLE;
			if (position_valid) {
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = status.filterPosition;
			}
			WHEEL_SLOT_PROPERTY->state = position_valid ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
			if (!names_valid || !position_valid) {
				X_FACTORY_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	//- wheel.X_FACTORY_RESET.on_change
	indigo_update_property(device, X_FACTORY_RESET_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 6;
		char version[OFW_VERSION_LEN + 1] = { 0 };
		if (OFWGetSDKVersion(version) == AO_SUCCESS) {
			version[OFW_VERSION_LEN] = 0;
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, version);
		}
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		X_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_PROPERTY_NAME, "Advanced", "Calibrate filter wheel", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_START_ITEM, X_CALIBRATE_START_ITEM_NAME, "Start", false);
		X_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, X_CUSTOM_SUFFIX_PROPERTY_NAME, WHEEL_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_CUSTOM_SUFFIX_ITEM, X_CUSTOM_SUFFIX_ITEM_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		X_BLUETOOTH_PROPERTY = indigo_init_switch_property(NULL, device->name, X_BLUETOOTH_PROPERTY_NAME, "Advanced", "Bluetooth", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_BLUETOOTH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_BLUETOOTH_ON_ITEM, X_BLUETOOTH_ON_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_BLUETOOTH_OFF_ITEM, X_BLUETOOTH_OFF_ITEM_NAME, "Disabled", true);
		X_BLUETOOTH_PROPERTY->hidden = true;
		X_BLUETOOTH_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, X_BLUETOOTH_NAME_PROPERTY_NAME, "Advanced", "Bluetooth name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_BLUETOOTH_NAME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_BLUETOOTH_NAME_ITEM, X_BLUETOOTH_NAME_ITEM_NAME, "Bluetooth name", "");
		X_BLUETOOTH_NAME_PROPERTY->hidden = true;
		X_FACTORY_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FACTORY_RESET_PROPERTY_NAME, "Advanced", "Factory reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FACTORY_RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FACTORY_RESET_ITEM, X_FACTORY_RESET_ITEM_NAME, "Reset", false);
		//+ wheel.X_FACTORY_RESET.on_attach
		INDIGO_COPY_VALUE(X_FACTORY_RESET_ITEM->hints, "warn_on_set:\"Confirm filter wheel factory reset?\";");
		//- wheel.X_FACTORY_RESET.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CUSTOM_SUFFIX_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BLUETOOTH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BLUETOOTH_NAME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FACTORY_RESET_PROPERTY);
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
	} else if (indigo_property_match_changeable(X_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_PROPERTY, wheel_x_calibrate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CUSTOM_SUFFIX_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CUSTOM_SUFFIX_PROPERTY, wheel_x_custom_suffix_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BLUETOOTH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BLUETOOTH_PROPERTY, wheel_x_bluetooth_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BLUETOOTH_NAME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BLUETOOTH_NAME_PROPERTY, wheel_x_bluetooth_name_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FACTORY_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FACTORY_RESET_PROPERTY, wheel_x_factory_reset_handler);
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
	indigo_release_property(X_BLUETOOTH_PROPERTY);
	indigo_release_property(X_BLUETOOTH_NAME_PROPERTY);
	indigo_release_property(X_FACTORY_RESET_PROPERTY);
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
	astroasis_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = indigo_safe_malloc(sizeof(astroasis_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == 0x338f && descriptor.idProduct == 0x0fe0)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		bool duplicate = false;
		for (int slot = 0; slot < MAX_DEVICES; slot++) {
			if (devices[slot] && ((astroasis_private_data *)devices[slot]->private_data)->usbdev == dev) {
				duplicate = true;
				break;
			}
		}
		int number = 0, ids[OFW_MAX_NUM] = { 0 };
		if (!duplicate && OFWScan(&number, ids) == AO_SUCCESS && number >= 0 && number <= OFW_MAX_NUM) {
			for (int index = 0; index < number; index++) {
				bool attached = false;
				for (int slot = 0; slot < MAX_DEVICES; slot++) {
					if (devices[slot] && ((astroasis_private_data *)devices[slot]->private_data)->dev_id == ids[index]) {
						attached = true;
						break;
					}
				}
				if (attached || OFWOpen(ids[index]) != AO_SUCCESS) {
					continue;
				}
				OFWVersion version = { 0 };
				char model[OFW_NAME_LEN + 1] = { 0 }, suffix[OFW_NAME_LEN + 1] = { 0 };
				plug_result = OFWGetVersion(ids[index], &version) == AO_SUCCESS && OFWGetProductModel(ids[index], model) == AO_SUCCESS && OFWGetFriendlyName(ids[index], suffix) == AO_SUCCESS;
				int res = OFWClose(ids[index]);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWClose(%d) after probe = %d", ids[index], res);
				if (plug_result) {
					model[OFW_NAME_LEN] = suffix[OFW_NAME_LEN] = 0;
					private_data->dev_id = ids[index];
					snprintf(private_data->model, sizeof(private_data->model), "%s", model);
					snprintf(private_data->custom_suffix, sizeof(private_data->custom_suffix), "%s", suffix);
					snprintf(name, INDIGO_NAME_SIZE, "Oasis Filter Wheel%s%s", suffix[0] ? " #" : "", suffix);
					indigo_make_name_unique(name, "%d", ids[index]);
					break;
				}
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
	astroasis_private_data *private_data = NULL;
	astroasis_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				int number = 0, ids[OFW_MAX_NUM] = { 0 };
				unplug_result = OFWScan(&number, ids) == AO_SUCCESS && number >= 0 && number <= OFW_MAX_NUM;
				if (unplug_result) {
					for (int index = 0; index < number; index++) {
						if (ids[index] == private_data->dev_id) {
							unplug_result = false;
							break;
						}
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

indigo_result indigo_wheel_astroasis(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			char version[OFW_VERSION_LEN + 1] = { 0 };
			if (OFWGetSDKVersion(version) == AO_SUCCESS) {
				version[OFW_VERSION_LEN] = 0;
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Oasis Filter Wheel SDK version: %s", version);
			}
			OFWSetLogLevel(indigo_get_log_level() >= INDIGO_LOG_DEBUG ? AO_LOG_LEVEL_DEBUG : AO_LOG_LEVEL_QUIET);
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
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, 0x338f, 0x0fe0, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
#else
#include "indigo_wheel_astroasis.h"

indigo_result indigo_wheel_astroasis(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, "Astroasis Oasis Wheel", __FUNCTION__, 0x03000004, false, INDIGO_DRIVER_SHUTDOWN);
	return action == INDIGO_DRIVER_INFO ? INDIGO_OK : INDIGO_UNSUPPORTED_ARCH;
}
#endif
