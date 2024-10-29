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

/** INDIGO Astroasis filter wheel driver
 \file indigo_wheel_astroasis.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_wheel_astroasis"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_wheel_astroasis.h"

#if !defined(__i386__)

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <OasisFilterWheel.h>

#define ADVANCED_GROUP                 "Advanced"

#define X_CALIBRATE_PROPERTY           (PRIVATE_DATA->calibrate_property)
#define X_CALIBRATE_START_ITEM         (X_CALIBRATE_PROPERTY->items+0)
#define X_CALIBRATE_PROPERTY_NAME      "X_CALIBRATE"
#define X_CALIBRATE_START_ITEM_NAME    "START"

#define X_CUSTOM_SUFFIX_PROPERTY     (PRIVATE_DATA->custom_suffix_property)
#define X_CUSTOM_SUFFIX_ITEM         (X_CUSTOM_SUFFIX_PROPERTY->items+0)
#define X_CUSTOM_SUFFIX_PROPERTY_NAME   "X_CUSTOM_SUFFIX"
#define X_CUSTOM_SUFFIX_NAME         "SUFFIX"

#define X_BLUETOOTH_PROPERTY			(PRIVATE_DATA->bluetooth_property)
#define X_BLUETOOTH_ON_ITEM			(X_BLUETOOTH_PROPERTY->items+0)
#define X_BLUETOOTH_OFF_ITEM			(X_BLUETOOTH_PROPERTY->items+1)
#define X_BLUETOOTH_PROPERTY_NAME			"X_BLUETOOTH_PROPERTY"
#define X_BLUETOOTH_ON_ITEM_NAME			"ENABLED"
#define X_BLUETOOTH_OFF_ITEM_NAME			"DISABLED"

#define X_BLUETOOTH_NAME_PROPERTY			(PRIVATE_DATA->bluetooth_name_property)
#define X_BLUETOOTH_NAME_ITEM			(X_BLUETOOTH_NAME_PROPERTY->items+0)
#define X_BLUETOOTH_NAME_PROPERTY_NAME		"X_BLUETOOTH_NAME_PROPERTY"
#define X_BLUETOOTH_NAME_NAME			"BLUETOOTH_NAME"

#define X_FACTORY_RESET_PROPERTY			(PRIVATE_DATA->factory_reset_property)
#define X_FACTORY_RESET_ITEM				(X_FACTORY_RESET_PROPERTY->items+0)
#define X_FACTORY_RESET_PROPERTY_NAME		"X_FACTORY_RESET"
#define X_FACTORY_RESET_ITEM_NAME			"RESET"

// gp_bits is used as boolean
#define is_connected                    gp_bits

#define ASTROASIS_VENDOR_ID			0x338f
#define ASTROASIS_PRODUCT_WHEEL_ID		0x0fe0

#define PRIVATE_DATA				((astroasis_private_data *)device->private_data)

typedef struct {
	int dev_id;
	OFWConfig config;
	OFWStatus status;
	char sdk_version[OFW_VERSION_LEN + 1];
	char firmware_version[OFW_VERSION_LEN + 1];
	char model[OFW_NAME_LEN + 1];
	char custom_suffix[OFW_NAME_LEN + 1];
	char bluetooth_name[OFW_NAME_LEN + 1];
	int current_slot, target_slot;
	int count;
	indigo_timer *wheel_timer;
	indigo_property *calibrate_property;
	indigo_property *custom_suffix_property;
	indigo_property *bluetooth_property;
	indigo_property *bluetooth_name_property;
	indigo_property *factory_reset_property;
} astroasis_private_data;

// -------------------------------------------------------------------------------- INDIGO Wheel device implementation

static bool wheel_config(indigo_device *device, unsigned int mask, int value) {
	OFWConfig *config = &PRIVATE_DATA->config;

	config->mask = mask;

	switch (mask) {
	case MASK_SPEED:
		config->speed = value;
		break;
	case MASK_BLUETOOTH:
		config->bluetoothOn = value;
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid Oasis wheel configuration mask %08X\n", mask);
		return false;
	}

	int ret = OFWSetConfig(PRIVATE_DATA->dev_id, config);

	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set Oasis wheel configuration, ret = %d\n", ret);
		return false;
	}

	return true;
}


static void wheel_timer_callback(indigo_device *device) {
	OFWStatus status;
	int res = OFWGetStatus(PRIVATE_DATA->dev_id, &status);
	PRIVATE_DATA->current_slot = status.filterPosition;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWGetStatus(%d, -> .filterPosition = %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_slot, res);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->wheel_timer));
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void calibrate_callback(indigo_device *device) {
	int res = OFWCalibrate(PRIVATE_DATA->dev_id, 0);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWCalibrate(%d) = %d", PRIVATE_DATA->dev_id, res);
	if (res == AO_SUCCESS) {

		OFWStatus status = { 0 };
		do {
			indigo_usleep(ONE_SECOND_DELAY);
			int res = OFWGetStatus(PRIVATE_DATA->dev_id, &status);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWGetStatus(%d, -> .filterPosition = %d .filterStatus = %d) = %d", PRIVATE_DATA->dev_id, status.filterPosition, status.filterStatus, res);
		} while (status.filterStatus != STATUS_IDLE);

		WHEEL_SLOT_ITEM->number.value =
		WHEEL_SLOT_ITEM->number.target =
		PRIVATE_DATA->current_slot =
		PRIVATE_DATA->target_slot = status.filterPosition;

		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);

		X_CALIBRATE_START_ITEM->sw.value=false;
		X_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration finished");
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		X_CALIBRATE_START_ITEM->sw.value=false;
		X_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration failed");
	}
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		char sdk_version[OFW_VERSION_LEN + 1];
		OFWGetSDKVersion(sdk_version);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");

		// --------------------------------------------------------------------------------- X_CALIBRATE
		X_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_PROPERTY_NAME, ADVANCED_GROUP, "Calibrate filter wheel", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_CALIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CALIBRATE_START_ITEM, X_CALIBRATE_START_ITEM_NAME, "Start", false);
		// --------------------------------------------------------------------------------- X_CUSTOM_SUFFIX
		X_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, "X_CUSTOM_SUFFIX", WHEEL_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_CUSTOM_SUFFIX_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(X_CUSTOM_SUFFIX_ITEM, X_CUSTOM_SUFFIX_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		// --------------------------------------------------------------------------------- BLUETOOTH
		X_BLUETOOTH_PROPERTY = indigo_init_switch_property(NULL, device->name, X_BLUETOOTH_PROPERTY_NAME, "Advanced", "Bluetooth", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_BLUETOOTH_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(X_BLUETOOTH_ON_ITEM, X_BLUETOOTH_ON_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_BLUETOOTH_OFF_ITEM, X_BLUETOOTH_OFF_ITEM_NAME, "Disabled", true);
		// ---------------------------------------------------------------------------------- X_BLUETOOTH_NAME
		X_BLUETOOTH_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, X_BLUETOOTH_NAME_PROPERTY_NAME, "Advanced", "Bluetooth name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_BLUETOOTH_NAME_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(X_BLUETOOTH_NAME_ITEM, X_BLUETOOTH_NAME_NAME, "Bluetooth name", PRIVATE_DATA->bluetooth_name);
		// ---------------------------------------------------------------------------------- X_FACTORY_RESET
		X_FACTORY_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FACTORY_RESET_PROPERTY_NAME, "Advanced", "Factory reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FACTORY_RESET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FACTORY_RESET_ITEM, X_FACTORY_RESET_ITEM_NAME, "Reset", false);
		sprintf(X_FACTORY_RESET_ITEM->hints, "warn_on_set:\"Clear all alignment points?\";");
		// --------------------------------------------------------------------------

		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	if (device->is_connected) {
		if (indigo_property_match(X_CALIBRATE_PROPERTY, property))
			indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);
		if (indigo_property_match(X_CUSTOM_SUFFIX_PROPERTY, property))
			indigo_define_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		if (indigo_property_match(X_BLUETOOTH_PROPERTY, property));
			indigo_define_property(device, X_BLUETOOTH_PROPERTY, NULL);
		if (indigo_property_match(X_BLUETOOTH_NAME_PROPERTY, property));
			indigo_define_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
		if (indigo_property_match(X_FACTORY_RESET_PROPERTY, property));
			indigo_define_property(device, X_FACTORY_RESET_PROPERTY, NULL);
	}
	return indigo_wheel_enumerate_properties(device, client, property);
}

static void wheel_connect_callback(indigo_device *device) {
	//EFW_INFO info;
	int index;
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				int res = OFWOpen(PRIVATE_DATA->dev_id);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
				if (!res) {
					int slot_num;
					int res = OFWGetSlotNum(PRIVATE_DATA->dev_id, &slot_num);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWGetSlotNum(%d, -> %d) = %d", PRIVATE_DATA->dev_id, slot_num, res);
					WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->count = slot_num;

					OFWStatus status;
					res = OFWGetStatus(PRIVATE_DATA->dev_id, &status);
					PRIVATE_DATA->target_slot = status.filterPosition;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWGetStatus(%d, -> .filterPosition = %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot, res);
					WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->target_slot;

					res = OFWGetConfig(PRIVATE_DATA->dev_id, &PRIVATE_DATA->config);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWGetConfig(%d, -> .speed = %d .bluetoothOn = %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->config.speed, PRIVATE_DATA->config.bluetoothOn, res);
					X_BLUETOOTH_ON_ITEM->sw.value = PRIVATE_DATA->config.bluetoothOn ? true : false;;
					X_BLUETOOTH_OFF_ITEM->sw.value = !X_BLUETOOTH_ON_ITEM->sw.value;

					res = OFWGetBluetoothName(PRIVATE_DATA->dev_id, PRIVATE_DATA->bluetooth_name);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWGetBluetoothName(%d, -> \"%s\") = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->bluetooth_name, res);
					indigo_set_text_item_value(X_BLUETOOTH_NAME_ITEM, PRIVATE_DATA->bluetooth_name);

					indigo_define_property(device, X_CALIBRATE_PROPERTY, NULL);
					indigo_define_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
					indigo_define_property(device, X_BLUETOOTH_PROPERTY, NULL);
					indigo_define_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
					indigo_define_property(device, X_FACTORY_RESET_PROPERTY, NULL);

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;
					indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWOpen(%d) = %d", index, res);
					indigo_global_unlock(device);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				}
			}
		}
	} else {
		if (device->is_connected) {
			int res = OFWClose(PRIVATE_DATA->dev_id);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWClose(%d) = %d", PRIVATE_DATA->dev_id, res);
			indigo_delete_property(device, X_CALIBRATE_PROPERTY, NULL);
			indigo_delete_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_delete_property(device, X_BLUETOOTH_PROPERTY, NULL);
			indigo_delete_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
			indigo_delete_property(device, X_FACTORY_RESET_PROPERTY, NULL);
			indigo_global_unlock(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
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
			int res = OFWSetPosition(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWSetPosition(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot, res);
			indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALIBRATE
		indigo_property_copy_values(X_CALIBRATE_PROPERTY, property, false);
		if (X_CALIBRATE_START_ITEM->sw.value) {
			X_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_CALIBRATE_PROPERTY, "Calibration started");
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_set_timer(device, 0.5, calibrate_callback, &PRIVATE_DATA->wheel_timer);
		}
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- X_CUSTOM_SUFFIX
	} else if (indigo_property_match_changeable(X_CUSTOM_SUFFIX_PROPERTY, property)) {
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(X_CUSTOM_SUFFIX_PROPERTY, property, false);
		
		if (strlen(X_CUSTOM_SUFFIX_ITEM->text.value) > OFW_NAME_LEN) {
			X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Suffix is too long");
			return INDIGO_OK;
		}

		strcpy(PRIVATE_DATA->custom_suffix, X_CUSTOM_SUFFIX_ITEM->text.value);

		int res = OFWSetFriendlyName(PRIVATE_DATA->dev_id, PRIVATE_DATA->custom_suffix);

		if (res != AO_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFWSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, res);
			X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFWSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, X_CUSTOM_SUFFIX_ITEM->text.value, res);
			X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
			if (strlen(X_CUSTOM_SUFFIX_ITEM->text.value) > 0) {
				indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Filter wheel name suffix '#%s' will be used on replug", X_CUSTOM_SUFFIX_ITEM->text.value);
			} else {
				indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Filter wheel name suffix cleared, will be used on replug");
			}
		}
		// -------------------------------------------------------------------------------- X_BLUETOOTH
	} else if (indigo_property_match_changeable(X_BLUETOOTH_PROPERTY, property)) {
		indigo_property_copy_values(X_BLUETOOTH_PROPERTY, property, false);
		if (wheel_config(device, MASK_BLUETOOTH, X_BLUETOOTH_ON_ITEM->sw.value))
			X_BLUETOOTH_PROPERTY->state = INDIGO_OK_STATE;
		else
			X_BLUETOOTH_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_BLUETOOTH_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_BLUETOOTH_NAME
	} else if (indigo_property_match_changeable(X_BLUETOOTH_NAME_PROPERTY, property)) {
		indigo_property_copy_values(X_BLUETOOTH_NAME_PROPERTY, property, false);
		if (strlen(X_BLUETOOTH_NAME_ITEM->text.value) > OFW_VERSION_LEN) {
			X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_BLUETOOTH_NAME_PROPERTY, "Bluetooth name is too long");
			return INDIGO_OK;
		}
		strcpy(PRIVATE_DATA->bluetooth_name, X_BLUETOOTH_NAME_ITEM->text.value);
		int ret = OFWSetBluetoothName(PRIVATE_DATA->dev_id, PRIVATE_DATA->bluetooth_name);
		if (ret == AO_SUCCESS) {
			X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set the Bluetooth name for the Oasis Filter Wheel, ret = %d\n", ret);
			X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FACTORY_RESET
	} else if (indigo_property_match_changeable(X_FACTORY_RESET_PROPERTY, property)) {
		indigo_property_copy_values(X_FACTORY_RESET_PROPERTY, property, false);
		if (X_FACTORY_RESET_ITEM->sw.value) {
			X_FACTORY_RESET_ITEM->sw.value = false;
			X_FACTORY_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
			int res = OFWFactoryReset(PRIVATE_DATA->dev_id);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OFWFactoryReset(%d) = %d", PRIVATE_DATA->dev_id, res);
			if (res == AO_SUCCESS) {
				X_FACTORY_RESET_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_FACTORY_RESET_PROPERTY, "Factory reset completed");
			} else {
				X_FACTORY_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_FACTORY_RESET_PROPERTY, "Factory reset failed");
			}
		}
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
	indigo_release_property(X_CALIBRATE_PROPERTY);
	indigo_release_property(X_CUSTOM_SUFFIX_PROPERTY);
	indigo_release_property(X_BLUETOOTH_PROPERTY);
	indigo_release_property(X_BLUETOOTH_NAME_PROPERTY);
	indigo_release_property(X_FACTORY_RESET_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support
typedef struct _WHEEL_LIST {
	indigo_device *device[OFW_MAX_NUM];
	int count;
} WHEEL_LIST;

static WHEEL_LIST gWheels = { {}, 0 };

static int wheel_get_index(int id) {
	int i;

	for (i = 0; i < gWheels.count; i++) {
		indigo_device *device = gWheels.device[i];

		if (device && (PRIVATE_DATA->dev_id == id))
			return i;
	}

	return -1;
}

static indigo_device *wheel_create(int id) {
	OFWVersion version;
	OFWConfig config;
	char model[OFW_NAME_LEN + 1];
	char custom_suffix[OFW_NAME_LEN + 1];
	char bluetooth_name[OFW_NAME_LEN + 1];
	indigo_device *device = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	AOReturn ret = OFWOpen(id);
	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWOpen() failed, ret = %d", ret);
		return NULL;
	}

	ret = OFWGetVersion(id, &version);
	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWGetVersion() failed, ret = %d", ret);
		goto out;
	}

	ret = OFWGetProductModel(id, model);
	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWGetProductModel() failed, ret = %d", ret);
		goto out;
	}

	ret = OFWGetFriendlyName(id, custom_suffix);
	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWGetFriendlyName() failed, ret = %d", ret);
		goto out;
	}

	ret = OFWGetBluetoothName(id, bluetooth_name);
	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWGetBluetoothName() failed, ret = %d", ret);
		goto out;
	}

	ret = OFWGetConfig(id, &config);
	if (ret != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "OFWGetConfig() failed, ret = %d", ret);
		goto out;
	}

	device = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);

	astroasis_private_data *private_data = indigo_safe_malloc(sizeof(astroasis_private_data));

	private_data->dev_id = id;

	OFWGetSDKVersion(private_data->sdk_version);

	sprintf(private_data->firmware_version, "%d.%d.%d", version.firmware >> 24, (version.firmware & 0x00FF0000) >> 16, (version.firmware & 0x0000FF00) >> 8);

	strcpy(private_data->model, model);
	strcpy(private_data->custom_suffix, custom_suffix);
	strcpy(private_data->bluetooth_name, bluetooth_name);

	if (strlen(private_data->custom_suffix) > 0)
		sprintf(device->name, "%s #%s", "Oasis Filter Wheel", private_data->custom_suffix);
	else
		sprintf(device->name, "%s", "Oasis Filter Wheel");

	memcpy(&private_data->config, &config, sizeof(OFWConfig));

	device->private_data = private_data;

	indigo_make_name_unique(device->name, "%d", id);

	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);

	indigo_attach_device(device);

out:
	OFWClose(id);

	return device;
}

static void wheel_refresh(void) {
	WHEEL_LIST wheels = { {}, 0 };
	int number, ids[OFW_MAX_NUM];
	int i;

	OFWScan(&number, ids);

	pthread_mutex_lock(&indigo_device_enumeration_mutex);

	for (i = 0; i < number; i++) {
		int pos = wheel_get_index(ids[i]);

		if (pos == -1) {
			// This wheel is not found in previous scan. Create wheel device instance for it
			wheels.device[wheels.count] = wheel_create(ids[i]);
			wheels.count += wheels.device[wheels.count] ? 1 : 0;
		} else {
			// This wheel is found in previous scan
			wheels.device[wheels.count] = gWheels.device[pos];
			wheels.count++;

			gWheels.device[pos] = 0;
		}
	}

	for (int i = 0; i < gWheels.count; i++) {
		indigo_device *device = gWheels.device[i];

		if (device) {
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
		}
	}

	memcpy(&gWheels, &wheels, sizeof(WHEEL_LIST));

	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static void process_plug_event(indigo_device *unused) {
	wheel_refresh();
}

static void process_unplug_event(indigo_device *unused) {
	wheel_refresh();
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device plugged has PID:VID = %x:%x", descriptor.idVendor, descriptor.idProduct);
			indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
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
	pthread_mutex_lock(&indigo_device_enumeration_mutex);

	for (int i = 0; i < gWheels.count; i++) {
		indigo_device *device = gWheels.device[i];

		if (device) {
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
		}
	}

	memset(&gWheels, 0, sizeof(WHEEL_LIST));

	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_astroasis(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Astroasis Oasis Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		char sdk_version[OFW_VERSION_LEN];

		OFWGetSDKVersion(sdk_version);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Oasis Focuser SDK version: %s", sdk_version);

		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASTROASIS_VENDOR_ID, ASTROASIS_PRODUCT_WHEEL_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < gWheels.count; i++)
			VERIFY_NOT_CONNECTED(gWheels.device[i]);
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
indigo_result indigo_focuser_astroasis(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Astroasis Oasis Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

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
