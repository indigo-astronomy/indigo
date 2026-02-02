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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASI focuser driver
 \file indigo_focuser_asi.c
 */

#define DRIVER_VERSION 0x0300001C
#define DRIVER_NAME "indigo_focuser_asi"
#define BT_DEVICE_NAME "EAF Pro (Bluetooth)"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_focuser_asi.h"

#include <EAF_focuser.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_uni_io.h>
#include <fcntl.h>

#define BT_CONFIG_KEY "selected_bt_device"

#define ASI_VENDOR_ID                   0x03c3
#define EAF_PRODUCT_ID                  0x1f10

#define PRIVATE_DATA                    ((asi_private_data *)device->private_data)

#define EAF_BEEP_PROPERTY               (PRIVATE_DATA->beep_property)
#define EAF_BEEP_ON_ITEM                (EAF_BEEP_PROPERTY->items+0)
#define EAF_BEEP_OFF_ITEM               (EAF_BEEP_PROPERTY->items+1)
#define EAF_BEEP_PROPERTY_NAME          "EAF_BEEP_ON_MOVE"
#define EAF_BEEP_ON_ITEM_NAME           "ON"
#define EAF_BEEP_OFF_ITEM_NAME          "OFF"

#define EAF_CUSTOM_SUFFIX_PROPERTY     (PRIVATE_DATA->custom_suffix_property)
#define EAF_CUSTOM_SUFFIX_ITEM         (EAF_CUSTOM_SUFFIX_PROPERTY->items+0)
#define EAF_CUSTOM_SUFFIX_PROPERTY_NAME   "EAF_CUSTOM_SUFFIX"
#define EAF_CUSTOM_SUFFIX_NAME         "SUFFIX"

#define EAF_SCAN_PROPERTY              (PRIVATE_DATA->scan_property)
#define EAF_SCAN_RESCAN_ITEM           (EAF_SCAN_PROPERTY->items+0)
#define EAF_SCAN_PROPERTY_NAME         "EAF_SCAN_BLUETOOTH"
#define EAF_SCAN_RESCAN_ITEM_NAME      "RESCAN"

#define EAF_BATTERY_INFO_PROPERTY      (PRIVATE_DATA->battery_info_property)
#define EAF_BATTERY_CHARGE_ITEM        (EAF_BATTERY_INFO_PROPERTY->items+0)
#define EAF_BATTERY_TEMP_ITEM          (EAF_BATTERY_INFO_PROPERTY->items+1)
#define EAF_BATTERY_VOLTAGE_ITEM       (EAF_BATTERY_INFO_PROPERTY->items+2)
#define EAF_BATTERY_CHARGE_CURR_ITEM   (EAF_BATTERY_INFO_PROPERTY->items+3)
#define EAF_BATTERY_DISCHARGE_CURR_ITEM (EAF_BATTERY_INFO_PROPERTY->items+4)
#define EAF_BATTERY_HEALTH_ITEM        (EAF_BATTERY_INFO_PROPERTY->items+5)
#define EAF_BATTERY_CHARGE_VOL_ITEM    (EAF_BATTERY_INFO_PROPERTY->items+6)
#define EAF_BATTERY_CYCLES_ITEM        (EAF_BATTERY_INFO_PROPERTY->items+7)
#define EAF_BATTERY_INFO_PROPERTY_NAME "EAF_BATTERY_INFO"

// gp_bits is used as boolean
#define is_connected                    gp_bits

#define MAX_BLE_DEVICES 64

typedef struct {
	int dev_id;
	EAF_INFO info;
	char model[64];
	char custom_suffix[9];
	int current_position, target_position, max_position, backlash;
	double prev_temp;
	indigo_timer *focuser_timer, *temperature_timer;
	pthread_mutex_t usb_mutex;
	indigo_property *beep_property;
	indigo_property *custom_suffix_property;
	indigo_property *battery_info_property;
	/* Bluetooth support */
	char selected_bt_name_address[INDIGO_NAME_SIZE];
	bool is_bluetooth;
	pthread_mutex_t bt_mutex;
	indigo_property *scan_property;
	int bt_device_count;
	BLE_DEVICE_INFO_T bt_devices[MAX_BLE_DEVICES];
	EAF_ALL_INFO all_info;
} asi_private_data;

static indigo_device *ble_device = NULL;
static int find_index_by_device_id(int id);
static void compensate_focus(indigo_device *device, double new_temp);

static void save_selected_bt_device_config(indigo_device *device, const char *name_address) {
	indigo_uni_handle *handle = indigo_open_config_file(device->name, 0, O_WRONLY | O_CREAT | O_TRUNC, ".bt");
	if (handle > 0) {
		indigo_uni_printf(handle, "%s=%s\n", BT_CONFIG_KEY, name_address);
		indigo_uni_close(&handle);
	}
}

static bool read_selected_bt_device_config(indigo_device *device, char *name_address, size_t len) {
	indigo_uni_handle *handle = indigo_open_config_file(device->name, 0, O_RDONLY, ".bt");
	if (handle <= 0) return false;
	char key[INDIGO_NAME_SIZE];
	char value[INDIGO_NAME_SIZE];
	while (indigo_uni_scanf_line(handle, "%63[^=]=%255[^\n]", key, value) == 2) {
		if (strcmp(key, BT_CONFIG_KEY) == 0) {
			strncpy(name_address, value, len);
			name_address[len-1] = '\0';
			indigo_uni_close(&handle);
			return true;
		}
	}
	indigo_uni_close(&handle);
	return false;
}

static void focuser_rebuild_scan_property(indigo_device *device) {
	/* delete existing if present */
	if (EAF_SCAN_PROPERTY) {
		indigo_delete_property(device, EAF_SCAN_PROPERTY, NULL);
		indigo_release_property(EAF_SCAN_PROPERTY);
		EAF_SCAN_PROPERTY = NULL;
	}

	int count = 1 + PRIVATE_DATA->bt_device_count;
	EAF_SCAN_PROPERTY = indigo_init_switch_property(NULL, device->name, EAF_SCAN_PROPERTY_NAME, "Main", "Scan Bluetooth", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, count);
	if (EAF_SCAN_PROPERTY) {
		/* init rescan item */
		indigo_init_switch_item(EAF_SCAN_RESCAN_ITEM, EAF_SCAN_RESCAN_ITEM_NAME, "Rescan", false);
		char item_name[128];
		char selected_name_address[128] = "";
		read_selected_bt_device_config(device, selected_name_address, sizeof(selected_name_address));
		for (int i = 0; i < PRIVATE_DATA->bt_device_count; i++) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device added: %s (%s)", PRIVATE_DATA->bt_devices[i].name, PRIVATE_DATA->bt_devices[i].address);
			snprintf(item_name, sizeof(item_name), "%s-%s", PRIVATE_DATA->bt_devices[i].name, PRIVATE_DATA->bt_devices[i].address);
			indigo_init_switch_item(EAF_SCAN_PROPERTY->items + 1 + i, item_name, PRIVATE_DATA->bt_devices[i].name, false);
			if (selected_name_address[0] != '\0' && strcmp(item_name, selected_name_address) == 0) {
				strncpy(PRIVATE_DATA->selected_bt_name_address, selected_name_address, sizeof(PRIVATE_DATA->selected_bt_name_address));
				EAF_SCAN_PROPERTY->items[1 + i].sw.value = true;
			}
		}
		EAF_SCAN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_define_property(device, EAF_SCAN_PROPERTY, NULL);
	}
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	bool moving, moving_HC;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = EAFIsMoving(PRIVATE_DATA->dev_id, &moving, &moving_HC);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFIsMoving(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_position));
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_position, res);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if ((!moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void temperature_timer_callback(indigo_device *device) {
	float temp;
	static bool has_sensor = true;
	int res;

	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = EAFGetTemp(PRIVATE_DATA->dev_id, &temp);
	FOCUSER_TEMPERATURE_ITEM->number.value = (double)temp;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if ((res != EAF_SUCCESS) && (FOCUSER_TEMPERATURE_ITEM->number.value > -270.0)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetTemp(%d, -> %f) = %d", PRIVATE_DATA->dev_id, FOCUSER_TEMPERATURE_ITEM->number.value, res);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetTemp(%d, -> %f) = %d", PRIVATE_DATA->dev_id, FOCUSER_TEMPERATURE_ITEM->number.value, res);
	}

	if (FOCUSER_TEMPERATURE_ITEM->number.value < -270.0) { /* -273 is returned when the sensor is not connected */
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (has_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected.");
			has_sensor = false;
		}
	} else {
		has_sensor = true;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		compensate_focus(device, temp);
	} else {
		/* reset temp so that the compensation starts when auto mode is selected */
		PRIVATE_DATA->prev_temp = -273;
	}

	/* Update battery info for devices with battery info support */
	if (!EAF_BATTERY_INFO_PROPERTY->hidden) {
		EAF_BATTERY_INFO battery_info;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = EAFGetBatteryInfo(PRIVATE_DATA->dev_id, &battery_info);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

		if (res == EAF_SUCCESS) {
			EAF_BATTERY_CHARGE_ITEM->number.value = (double)battery_info.battery_percentage;
			EAF_BATTERY_TEMP_ITEM->number.value = (double)battery_info.battery_temp;
			EAF_BATTERY_VOLTAGE_ITEM->number.value = (double)battery_info.battery_vol / 1000.0;
			EAF_BATTERY_CHARGE_CURR_ITEM->number.value = (double)battery_info.battery_charge_curr;
			EAF_BATTERY_DISCHARGE_CURR_ITEM->number.value = (double)battery_info.battery_discharge_curr;
			EAF_BATTERY_HEALTH_ITEM->number.value = (double)battery_info.battery_health;
			EAF_BATTERY_CHARGE_VOL_ITEM->number.value = (double)battery_info.battery_charge_vol / 1000.0;
			EAF_BATTERY_CYCLES_ITEM->number.value = (double)battery_info.battery_num_of_cycles;
			EAF_BATTERY_INFO_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetBatteryInfo(%d) = %d", PRIVATE_DATA->dev_id, res);
			EAF_BATTERY_INFO_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, EAF_BATTERY_INFO_PROPERTY, NULL);
	}

	indigo_reschedule_timer(device, 2, &(PRIVATE_DATA->temperature_timer));
}


static void compensate_focus(indigo_device *device, double new_temp) {
	int compensation;
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;

	/* we do not have previous temperature reading */
	if (PRIVATE_DATA->prev_temp < -270) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PRIVATE_DATA->prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}

	/* we do not have current temperature reading or focuser is moving */
	if ((new_temp < -270) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}

	/* temperature difference if more than 1 degree so compensation needed */
	if ((fabs(temp_difference) >= FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Compensation: temp_difference = %.2f, compensation = %d, steps/degC = %.0f, threshold = %.2f",
			temp_difference,
			compensation,
			FOCUSER_COMPENSATION_ITEM->number.value,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
	} else {
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Not compensating (not needed): temp_difference = %.2f, threshold = %.2f",
			temp_difference,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
		return;
	}

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
}


static indigo_result eaf_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_BEEP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_CUSTOM_SUFFIX_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_BATTERY_INFO_PROPERTY);
	}
	if (PRIVATE_DATA->is_bluetooth) {
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_SCAN_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		char *sdk_version = EAFGetSDKVersion();
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = PRIVATE_DATA->info.MaxStep;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "\'%s\' MaxStep = %d",device->name ,PRIVATE_DATA->info.MaxStep);

		FOCUSER_SPEED_PROPERTY->hidden = true;

		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 10000;
		FOCUSER_BACKLASH_ITEM->number.step = 1;

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;

		// -------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		// -------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------- BEEP_PROPERTY
		EAF_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, EAF_BEEP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (EAF_BEEP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}

		indigo_init_switch_item(EAF_BEEP_ON_ITEM, EAF_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(EAF_BEEP_OFF_ITEM, EAF_BEEP_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------------- EAF_CUSTOM_SUFFIX
		EAF_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, "EAF_CUSTOM_SUFFIX", FOCUSER_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (EAF_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(EAF_CUSTOM_SUFFIX_ITEM, EAF_CUSTOM_SUFFIX_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		// -------------------------------------------------------------------------- EAF_BATTERY_INFO
		EAF_BATTERY_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, EAF_BATTERY_INFO_PROPERTY_NAME, "Battery", "Battery information", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (EAF_BATTERY_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		EAF_BATTERY_INFO_PROPERTY->hidden = true;  /* Hidden by default, unhide if device supports battery info */
		indigo_init_number_item(EAF_BATTERY_CHARGE_ITEM, "CHARGE", "Charge (%)", 0, 100, 0, 0);
		indigo_init_number_item(EAF_BATTERY_TEMP_ITEM, "TEMPERATURE", "Temperature (°C)", -100, 100, 0, 0);
		indigo_init_number_item(EAF_BATTERY_VOLTAGE_ITEM, "VOLTAGE", "Voltage (V)", 0, 5, 0, 0);
		indigo_init_number_item(EAF_BATTERY_CHARGE_CURR_ITEM, "CHARGE_CURRENT", "Charge current (mA)", 0, 10000, 0, 0);
		indigo_init_number_item(EAF_BATTERY_DISCHARGE_CURR_ITEM, "DISCHARGE_CURRENT", "Discharge current (mA)", 0, 10000, 0, 0);
		indigo_init_number_item(EAF_BATTERY_HEALTH_ITEM, "HEALTH", "Health", 0, 100, 0, 0);
		indigo_init_number_item(EAF_BATTERY_CHARGE_VOL_ITEM, "CHARGE_VOLTAGE", "Charge voltage (V)", 0, 5, 0, 0);
		indigo_init_number_item(EAF_BATTERY_CYCLES_ITEM, "CYCLES", "Number of cycles", 0, 10000, 0, 0);
		// -------------------------------------------------------------------------- Bluetooth support
		if (PRIVATE_DATA->is_bluetooth) {
			pthread_mutex_init(&PRIVATE_DATA->bt_mutex, NULL);
			PRIVATE_DATA->scan_property = NULL;
			PRIVATE_DATA->bt_device_count = 0;

			BLE_DEVICE_INFO_T devs[MAX_BLE_DEVICES];
			int found = 0;
			int res = EAFBLEScan(1000, devs, MAX_BLE_DEVICES, &found);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFBLEScan(1000, devices, 64, &actual) = %d, actual = %d", res, found);

			if (res == EAF_SUCCESS && found > 0) {
				PRIVATE_DATA->bt_device_count = found > MAX_BLE_DEVICES ? MAX_BLE_DEVICES : found;
				for (int i = 0; i < PRIVATE_DATA->bt_device_count; i++) PRIVATE_DATA->bt_devices[i] = devs[i];
			} else {
				PRIVATE_DATA->bt_device_count = 0;
			}

			focuser_rebuild_scan_property(device);
		}
		// ------------------------------------------------------------------------------
		return eaf_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ble_connection_state_callback(bool state) {
	if (state) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Bluetooth connection established");
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Bluetooth connection lost");
		if (ble_device != NULL && ble_device->is_connected) {
			indigo_device *device = ble_device;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, "Bluetooth connection lost");
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			indigo_delete_property(device, EAF_BEEP_PROPERTY, NULL);
			indigo_delete_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_delete_property(device, EAF_BATTERY_INFO_PROPERTY, NULL);
			indigo_global_unlock(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
		}
	}
}

static int focuser_bt_open(indigo_device *device) {
	/* selected -> parse "name-address" from the item name */
	char dev_name[INDIGO_NAME_SIZE] = {0};
	char dev_addr[INDIGO_NAME_SIZE] = {0};

	if (PRIVATE_DATA->selected_bt_name_address[0] == '\0') {
		char *message = "No Bluetooth device selected.";
		indigo_send_message(device, message);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
		return INDIGO_FAILED;
	}

	char *sep = strrchr(PRIVATE_DATA->selected_bt_name_address, '-');
	if (sep) {
		size_t name_len = sep - PRIVATE_DATA->selected_bt_name_address;
		if (name_len >= sizeof(dev_name)) name_len = sizeof(dev_name) - 1;
		strncpy(dev_name, PRIVATE_DATA->selected_bt_name_address, name_len);
		dev_name[name_len] = '\0';
		strncpy(dev_addr, sep + 1, sizeof(dev_addr));
		dev_addr[sizeof(dev_addr)-1] = '\0';
	} else {
		/* use whole string as device name */
		strncpy(dev_name, PRIVATE_DATA->selected_bt_name_address, sizeof(dev_name));
		dev_name[sizeof(dev_name)-1] = '\0';
	}

	int id = -1;
	pthread_mutex_lock(&PRIVATE_DATA->bt_mutex);
	int res = EAFBLEConnect(dev_name, dev_addr, &id);
	if (res == EAF_SUCCESS) {
		if (PRIVATE_DATA->dev_id == id) {
			/* already paired, skip */
			INDIGO_DRIVER_LOG(DRIVER_NAME, "BLE device %s (%s) is already paired -> id=%d", dev_name, dev_addr, id);
			pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);
			return INDIGO_OK;
		}

		indigo_send_message(device, "Pairing to %s, (if it beeps press IN or OUT)...", dev_name);
		res = EAFBLEPair(id);
		if (res == EAF_SUCCESS) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Paired BLE device %s (%s) -> id=%d", dev_name, dev_addr, id);
			PRIVATE_DATA->dev_id = id;

			res = EAFBLERegConnStateCallback(id, ble_connection_state_callback);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFBLERegConnStateCallback(%d) = %d", id, res);
			}

			pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);
			indigo_send_message(device, "Paired");
			return INDIGO_OK;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFBLEPair(%d) = %d", id, res);
			EAFBLEDisconnect(PRIVATE_DATA->dev_id);
			PRIVATE_DATA->dev_id = -1;
			pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);
			indigo_send_message(device, "Pairing failed");
			return INDIGO_FAILED;
		}
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFBLEConnect(%s,%s) = %d", dev_name, dev_addr, res);
	PRIVATE_DATA->dev_id = -1;
	pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);
	return INDIGO_FAILED;
}

static int focuser_bt_close(indigo_device *device) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->bt_mutex);
	res = EAFBLEDisconnect(PRIVATE_DATA->dev_id);
	if (res == EAF_SUCCESS) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected Bluetooth device id=%d", PRIVATE_DATA->dev_id);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFBLEDisconnect(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	PRIVATE_DATA->dev_id = -1;
	pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);
	return INDIGO_OK;
}

static void focuser_connect_callback(indigo_device *device) {
	int index = 0;
	int res = EAF_SUCCESS;
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!PRIVATE_DATA->is_bluetooth) {
			index = find_index_by_device_id(PRIVATE_DATA->dev_id);
		}
		if (index >= 0 || PRIVATE_DATA->is_bluetooth) {
			if (!device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				} else {
					if (PRIVATE_DATA->is_bluetooth) {
						res = focuser_bt_open(device);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "focuser_bt_open(%d) = %d", PRIVATE_DATA->dev_id, res);
					} else {
						EAFGetID(index, &(PRIVATE_DATA->dev_id));
						res = EAFOpen(PRIVATE_DATA->dev_id);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
					}

					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (res == EAF_SUCCESS) {
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						/* Why? Why? Why BT is handleled differently? */
						if (PRIVATE_DATA->is_bluetooth) {
							res = EAFBLEgetAllInfo(PRIVATE_DATA->dev_id, &PRIVATE_DATA->all_info);
							if (res != EAF_SUCCESS) {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFBLEgetAllInfo(%d) = %d", PRIVATE_DATA->dev_id, res);
							}
							FOCUSER_STEPS_ITEM->number.max =
							FOCUSER_POSITION_ITEM->number.max =
							PRIVATE_DATA->max_position =
							FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->all_info.max_steps;

							FOCUSER_BACKLASH_ITEM->number.value = PRIVATE_DATA->backlash = PRIVATE_DATA->all_info.backlash_steps;
							FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position = PRIVATE_DATA->all_info.current_steps;

							FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value = PRIVATE_DATA->all_info.reverse_state;
							FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = !PRIVATE_DATA->all_info.reverse_state;

							EAF_BEEP_ON_ITEM->sw.value = PRIVATE_DATA->all_info.buzzer_state;
							EAF_BEEP_OFF_ITEM->sw.value = !PRIVATE_DATA->all_info.buzzer_state;
						} else {
							res = EAFGetBacklash(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->backlash));
							if (res != EAF_SUCCESS) {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
							}
							FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;

							res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_position));
							if (res != EAF_SUCCESS) {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
							}
							FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->target_position;

							res = EAFGetReverse(PRIVATE_DATA->dev_id, &(FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value));
							if (res != EAF_SUCCESS) {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetReverse(%d, -> %d) = %d", PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
							}
							FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;

							res = EAFGetBeep(PRIVATE_DATA->dev_id, &(EAF_BEEP_ON_ITEM->sw.value));
							if (res != EAF_SUCCESS) {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBeep(%d, -> %d) = %d", PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value, res);
							}
							EAF_BEEP_OFF_ITEM->sw.value = !EAF_BEEP_ON_ITEM->sw.value;
						}

						res = EAFGetMaxStep(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = (double)PRIVATE_DATA->max_position;

						int step_range = 0;
						res = EAFStepRange(PRIVATE_DATA->dev_id, &step_range);
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFStepRange(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = (double)step_range;

						/* Check for battery info control capability */
						EAF_BATTERY_INFO_PROPERTY->hidden = true;
						/*
						int num_controls = 0;
						res = EAFGetNumOfControls(PRIVATE_DATA->dev_id, &num_controls);
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetNumOfControls(%d, -> %d) = %d", PRIVATE_DATA->dev_id, num_controls, res);
						if (res == EAF_SUCCESS) {
							for (int i = 0; i < num_controls; i++) {
								EAF_CONTROL_CAPS caps;
								res = EAFGetControlCaps(PRIVATE_DATA->dev_id, i, &caps);
								INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetControlCaps(%d, %d) = %d: type=%d, supported=%d", PRIVATE_DATA->dev_id, i, res, caps.controlType, caps.isSupported);
								if (res == EAF_SUCCESS && caps.controlType == CONTROL_BAT_INFO && caps.isSupported) {
									EAF_BATTERY_INFO_PROPERTY->hidden = false;
									INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Battery info control is supported for device %d", PRIVATE_DATA->dev_id);
									break;
								}
							}
						}
						*/
						/* A simpler check for battery info support - the above should be the right way, but it does not work */
						EAF_BATTERY_INFO battery_info;
						res = EAFGetBatteryInfo(PRIVATE_DATA->dev_id, &battery_info);
						if (res == EAF_SUCCESS) {
							EAF_BATTERY_INFO_PROPERTY->hidden = false;
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Battery info is supported for device %d", PRIVATE_DATA->dev_id);
						}
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
						indigo_define_property(device, EAF_BEEP_PROPERTY, NULL);
						indigo_define_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
						indigo_define_property(device, EAF_BATTERY_INFO_PROPERTY, NULL);

						PRIVATE_DATA->prev_temp = -273;  /* we do not have previous temperature reading */
						device->is_connected = true;
						indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
						indigo_set_timer(device, 0.1, temperature_timer_callback, &PRIVATE_DATA->temperature_timer);
					} else {
						if (PRIVATE_DATA->is_bluetooth) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "focuser_bt_open(%d) = %d", PRIVATE_DATA->dev_id, res);
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						indigo_global_unlock(device);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					}
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			indigo_delete_property(device, EAF_BEEP_PROPERTY, NULL);
			indigo_delete_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_delete_property(device, EAF_BATTERY_INFO_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = EAFStop(PRIVATE_DATA->dev_id);
			if (PRIVATE_DATA->is_bluetooth) {
				res = focuser_bt_close(device);
			} else {
				res = EAFClose(PRIVATE_DATA->dev_id);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
				}
			}
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_bt_scan_callback(indigo_device *device) {
	/* rescan */
	if (EAF_SCAN_RESCAN_ITEM->sw.value) {
		BLE_DEVICE_INFO_T devs[MAX_BLE_DEVICES];
		int actual = 0;
		int res;
		pthread_mutex_lock(&PRIVATE_DATA->bt_mutex);
		res = EAFBLEScan(3500, devs, MAX_BLE_DEVICES, &actual);
		if (res == EAF_SUCCESS && actual > 0) {
			PRIVATE_DATA->bt_device_count = actual > MAX_BLE_DEVICES ? MAX_BLE_DEVICES : actual;
			for (int i = 0; i < PRIVATE_DATA->bt_device_count; i++) PRIVATE_DATA->bt_devices[i] = devs[i];
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFBLEScan(3500, devs, %d, &actual) = %d, actual = %d", MAX_BLE_DEVICES, res, actual);
			PRIVATE_DATA->bt_device_count = 0;
		}

		focuser_rebuild_scan_property(device);
		pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);
		return;
	}
	/* select device */
	if(device->is_connected) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Device is connected, cannot change selection");
		EAF_SCAN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, EAF_SCAN_PROPERTY, "Device is connected, cannot change selection");
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->bt_mutex);
	for (int i = 1; i < EAF_SCAN_PROPERTY->count; i++) { // skip 0 - the Rescan item
		PRIVATE_DATA->selected_bt_name_address[0] = '\0';
		if (EAF_SCAN_PROPERTY->items[i].sw.value) {
			strncpy(PRIVATE_DATA->selected_bt_name_address, EAF_SCAN_PROPERTY->items[i].name, sizeof(PRIVATE_DATA->selected_bt_name_address));
			PRIVATE_DATA->selected_bt_name_address[sizeof(PRIVATE_DATA->selected_bt_name_address) - 1] = '\0';
			break;
		}
	}
	save_selected_bt_device_config(device, PRIVATE_DATA->selected_bt_name_address);
	pthread_mutex_unlock(&PRIVATE_DATA->bt_mutex);

	if (EAF_SCAN_PROPERTY) {
		EAF_SCAN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, EAF_SCAN_PROPERTY, NULL);
	}
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFSetReverse(PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetReverse(%d, %d) = %d", PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_ITEM->number.target < 0 || FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (FOCUSER_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* RESET CURRENT POSITION - SYNC */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = EAFResetPostion(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFResetPostion(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
				FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFSetMaxStep(PRIVATE_DATA->dev_id, PRIVATE_DATA->max_position);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetMaxStep(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->max_position, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetMaxStep(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		PRIVATE_DATA->backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;
		int res = EAFSetBacklash(PRIVATE_DATA->dev_id, PRIVATE_DATA->backlash);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetBacklash(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->backlash, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetBacklash(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->backlash));
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
			}
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position - FOCUSER_STEPS_ITEM->number.value);
			} else {
				PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position + FOCUSER_STEPS_ITEM->number.value);
			}

			/* Make sure we do not attempt to go beyond the limits */
			if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.max;
			} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.min;
			}

			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
			}
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFStop(PRIVATE_DATA->dev_id);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFStop(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION_PROPERTY
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(EAF_BEEP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- EAF_BEEP_PROPERTY
		indigo_property_copy_values(EAF_BEEP_PROPERTY, property, false);
		EAF_BEEP_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFSetBeep(PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBeep(%d, %d) = %d", PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value, res);
			EAF_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, EAF_BEEP_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- EAF_CUSTOM_SUFFIX
	} else if (indigo_property_match_changeable(EAF_CUSTOM_SUFFIX_PROPERTY, property)) {
		EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(EAF_CUSTOM_SUFFIX_PROPERTY, property, false);
		if (strlen(EAF_CUSTOM_SUFFIX_ITEM->text.value) > 8) {
			EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Custom suffix too long");
			return INDIGO_OK;
		}
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		EAF_ID eaf_id = {0};
		memcpy(eaf_id.id, EAF_CUSTOM_SUFFIX_ITEM->text.value, 8);
		memcpy(PRIVATE_DATA->custom_suffix, EAF_CUSTOM_SUFFIX_ITEM->text.value, sizeof(PRIVATE_DATA->custom_suffix));
		int res = EAFSetID(PRIVATE_DATA->dev_id, eaf_id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, EAF_CUSTOM_SUFFIX_ITEM->text.value, res);
			EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, EAF_CUSTOM_SUFFIX_ITEM->text.value, res);
			EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
			if (strlen(EAF_CUSTOM_SUFFIX_ITEM->text.value) > 0) {
				indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Focuser name suffix '#%s' will be used on replug", EAF_CUSTOM_SUFFIX_ITEM->text.value);
			} else {
				indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Focuser name suffix cleared, will be used on replug");
			}
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (PRIVATE_DATA->is_bluetooth && indigo_property_match_changeable(EAF_SCAN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- EAF_SCAN_PROPERTY
		indigo_property_copy_values(EAF_SCAN_PROPERTY, property, false);
		EAF_SCAN_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, EAF_SCAN_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_bt_scan_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, EAF_BEEP_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}

	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(EAF_BEEP_PROPERTY);
	indigo_release_property(EAF_CUSTOM_SUFFIX_PROPERTY);
	if (EAF_SCAN_PROPERTY) {
		indigo_delete_property(device, EAF_SCAN_PROPERTY, NULL);
		indigo_release_property(EAF_SCAN_PROPERTY);
	}
	indigo_release_property(EAF_BATTERY_INFO_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)

static int eaf_products[100];
static int eaf_id_count = 0;

static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[EAF_ID_MAX] = {false};

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
		if (device == NULL) {
			continue;
		}
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
		if (res == EAF_SUCCESS) {
			dev_tmp[id] = true;
		}
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


static void split_device_name(const char *fill_device_name, char *device_name, char *suffix) {
	if (fill_device_name == NULL || device_name == NULL || suffix == NULL) {
		return;
	}

	char name_buf[64];
	strncpy(name_buf, fill_device_name, sizeof(name_buf));
	char *suffix_start = strchr(name_buf, '(');
	char *suffix_end = strrchr(name_buf, ')');

	if (suffix_start == NULL || suffix_end == NULL) {
		strncpy(device_name, name_buf, 64);
		suffix[0] = '\0';
		return;
	}
	suffix_start[0] = '\0';
	suffix_end[0] = '\0';
	suffix_start++;

	strncpy(device_name, name_buf, 64);
	strncpy(suffix, suffix_start, 9);
}

static pthread_mutex_t indigo_device_enumeration_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(indigo_device *unused) {
	EAF_INFO info;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"",
		focuser_attach,
		eaf_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
		);
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	}
	int id = find_plugged_device_id();
	if (id == NO_DEVICE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	}
	int res = EAFOpen(id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFOpen(%d}) = %d", id, res);
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
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
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
			return;
		}
		  indigo_sleep(1);
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
	char name[64] = {0};
	char suffix[9] = {0};
	char device_name[64] = {0};
	split_device_name(info.Name, name, suffix);
	if (suffix[0] != '\0') {
		sprintf(device_name, "%s #%s", name, suffix);
	} else {
		sprintf(device_name, "%s", name);
	}
	sprintf(device->name, "%s", device_name);
	indigo_make_name_unique(device->name, "%d", id);

	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->dev_id = id;
	private_data->info = info;
	strncpy(private_data->custom_suffix, suffix, 9);
	strncpy(private_data->model, name, 64);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	int slot, id;
	bool removed = false;
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	while ((id = find_unplugged_device_id()) != -1) {
		slot = find_device_slot(id);
		if (slot < 0) {
			continue;
		}
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
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
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device plugged has PID:VID = %x:%x", descriptor.idVendor, descriptor.idProduct);
			for (int i = 0; i < eaf_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || eaf_products[i] != descriptor.idProduct) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EAF device plugged (maybe ASI Camera)!");
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
		if (*device == NULL) {
			continue;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
	for (int index = 0; index < EAF_ID_MAX; index++) {
		connected_ids[index] = false;
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_focuser_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Focuser", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			const char *sdk_version = EAFGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "EAF SDK v. %s ", sdk_version);

			for(int index = 0; index < EAF_ID_MAX; index++) {
				connected_ids[index] = false;
			}
			//eaf_id_count = EAFGetProductIDs(eaf_products);
			//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetProductIDs(-> [ %d, %d, ... ]) = %d", eaf_products[0], eaf_products[1], eaf_id_count);
			eaf_products[0] = EAF_PRODUCT_ID;
			eaf_id_count = 1;

#if defined (INDIGO_MACOS) || defined(INDIGO_WINDOWS)
			/* create static Bluetooth pseudo device */
			pthread_mutex_lock(&indigo_device_enumeration_mutex);
			static indigo_device bt_template = INDIGO_DEVICE_INITIALIZER(
				"",
				focuser_attach,
				eaf_enumerate_properties,
				focuser_change_property,
				NULL,
				focuser_detach
			);
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &bt_template);
			strncpy(device->name, BT_DEVICE_NAME, sizeof(device->name));
			indigo_make_name_unique(device->name, "%d", 0);
			asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
			memset(private_data, 0, sizeof(asi_private_data));
			private_data->is_bluetooth = true;
			private_data->dev_id = -1;
			strncpy(private_data->model, BT_DEVICE_NAME, sizeof(private_data->model));
			device->private_data = private_data;
			indigo_attach_device(device);
			INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
			ble_device = device;
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
#endif

			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(ble_device);
			for (int i = 0; i < MAX_DEVICES; i++) {
				VERIFY_NOT_CONNECTED(devices[i]);
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");

#if defined(INDIGO_MACOS) || defined(INDIGO_WINDOWS)
		if (ble_device) {
			indigo_detach_device(ble_device);
			free(ble_device->private_data);
			free(ble_device);
			ble_device = NULL;
		}
#endif

			remove_all_devices();
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}

