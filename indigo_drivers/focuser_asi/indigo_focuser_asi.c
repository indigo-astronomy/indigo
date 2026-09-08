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

// This file generated from indigo_focuser_asi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdbool.h>
#include <EAF_focuser.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_focuser_asi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300001E
#define DRIVER_NAME          "indigo_focuser_asi"
#define DRIVER_LABEL         "ZWO ASI Focuser"
#define FOCUSER_DEVICE_NAME  "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((asi_private_data *)device->private_data)

//+ define

#define ASI_VENDOR_ID        0x03c3
#define EAF_PRODUCT_ID       0x1f10

//- define

#pragma mark - Property definitions

#define EAF_BEEP_PROPERTY              (PRIVATE_DATA->eaf_beep_property)
#define EAF_BEEP_ON_ITEM               (EAF_BEEP_PROPERTY->items + 0)
#define EAF_BEEP_OFF_ITEM              (EAF_BEEP_PROPERTY->items + 1)

#define EAF_BEEP_PROPERTY_NAME         "EAF_BEEP_ON_MOVE"
#define EAF_BEEP_ON_ITEM_NAME          "ON"
#define EAF_BEEP_OFF_ITEM_NAME         "OFF"

#define EAF_CUSTOM_SUFFIX_PROPERTY      (PRIVATE_DATA->eaf_custom_suffix_property)
#define EAF_CUSTOM_SUFFIX_ITEM          (EAF_CUSTOM_SUFFIX_PROPERTY->items + 0)

#define EAF_CUSTOM_SUFFIX_PROPERTY_NAME "EAF_CUSTOM_SUFFIX"
#define EAF_CUSTOM_SUFFIX_ITEM_NAME     "SUFFIX"

#define EAF_BATTERY_INFO_PROPERTY            (PRIVATE_DATA->eaf_battery_info_property)
#define EAF_BATTERY_CHARGE_ITEM              (EAF_BATTERY_INFO_PROPERTY->items + 0)
#define EAF_BATTERY_TEMP_ITEM                (EAF_BATTERY_INFO_PROPERTY->items + 1)
#define EAF_BATTERY_VOLTAGE_ITEM             (EAF_BATTERY_INFO_PROPERTY->items + 2)
#define EAF_BATTERY_CHARGE_CURR_ITEM         (EAF_BATTERY_INFO_PROPERTY->items + 3)
#define EAF_BATTERY_DISCHARGE_CURR_ITEM      (EAF_BATTERY_INFO_PROPERTY->items + 4)
#define EAF_BATTERY_HEALTH_ITEM              (EAF_BATTERY_INFO_PROPERTY->items + 5)
#define EAF_BATTERY_CHARGE_VOL_ITEM          (EAF_BATTERY_INFO_PROPERTY->items + 6)
#define EAF_BATTERY_CYCLES_ITEM              (EAF_BATTERY_INFO_PROPERTY->items + 7)

#define EAF_BATTERY_INFO_PROPERTY_NAME       "EAF_BATTERY_INFO"
#define EAF_BATTERY_CHARGE_ITEM_NAME         "CHARGE"
#define EAF_BATTERY_TEMP_ITEM_NAME           "TEMPERATURE"
#define EAF_BATTERY_VOLTAGE_ITEM_NAME        "VOLTAGE"
#define EAF_BATTERY_CHARGE_CURR_ITEM_NAME    "CHARGE_CURRENT"
#define EAF_BATTERY_DISCHARGE_CURR_ITEM_NAME "DISCHARGE_CURRENT"
#define EAF_BATTERY_HEALTH_ITEM_NAME         "HEALTH"
#define EAF_BATTERY_CHARGE_VOL_ITEM_NAME     "CHARGE_VOLTAGE"
#define EAF_BATTERY_CYCLES_ITEM_NAME         "CYCLES"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *eaf_beep_property;
	indigo_property *eaf_custom_suffix_property;
	indigo_property *eaf_battery_info_property;
	//+ data
	int dev_id;
	EAF_INFO info;
	char model[64];
	char custom_suffix[9];
	int current_position, target_position, max_position, backlash;
	double prev_temp;
	bool has_temperature_sensor;
	bool moving;
	//- data
} asi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

static void split_device_name(const char *full_name, char *model, char *suffix) {
	snprintf(model, 64, "%s", full_name);
	suffix[0] = 0;
	char *suffix_start = strchr(model, '(');
	char *suffix_end = strrchr(model, ')');
	if (suffix_start == NULL || suffix_end == NULL || suffix_end <= suffix_start) {
		return;
	}
	*suffix_start = 0;
	*suffix_end = 0;
	suffix_start++;
	while (suffix_start[0] == ' ') {
		suffix_start++;
	}
	strncpy(suffix, suffix_start, 8);
	suffix[8] = 0;
	size_t length = strlen(model);
	while (length > 0 && model[length - 1] == ' ') {
		model[--length] = 0;
	}
}

static bool asi_open(indigo_device *device) {
	bool result = false;
	bool global_locked = false;
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
	} else {
		global_locked = true;
		int res = EAFOpen(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unable to open EAF device %d: %d", PRIVATE_DATA->dev_id, res);
		}
		result = res == EAF_SUCCESS;
	}
	if (!result && global_locked) {
		indigo_global_unlock(device);
	}
	return result;
}

static void asi_close(indigo_device *device) {
	int res = EAFStop(PRIVATE_DATA->dev_id);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFStop(%d) = %d", PRIVATE_DATA->dev_id, res);
	res = EAFClose(PRIVATE_DATA->dev_id);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_global_unlock(device);
}

//- code

//+ focuser.code

static void focuser_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	bool moving = false, moving_hc = false;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	int res = EAFIsMoving(PRIVATE_DATA->dev_id, &moving, &moving_hc);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFIsMoving(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	}
	PRIVATE_DATA->moving = false;
	if (FOCUSER_POSITION_PROPERTY->state != INDIGO_ALERT_STATE && (moving || moving_hc)) {
		PRIVATE_DATA->moving = true;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
	}
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
	if (FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
		if (moving_hc) {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, moving_hc ? "Release the hand controller to stop motion" : NULL);
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static bool focuser_motion_ready(indigo_device *device) {
	if (!IS_CONNECTED) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Focuser is not connected");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return false;
	}
	if (PRIVATE_DATA->moving) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Focuser is moving");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return false;
	}
	bool moving = false, moving_hc = false;
	int res = EAFIsMoving(PRIVATE_DATA->dev_id, &moving, &moving_hc);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFIsMoving(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Cannot verify focuser motion state");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return false;
	}
	if (moving || moving_hc) {
		PRIVATE_DATA->moving = true;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Focuser is moving");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
		return false;
	}
	return true;
}

static void focuser_update_limits(indigo_device *device, int maximum) {
	bool changed = FOCUSER_POSITION_ITEM->number.max != maximum;
	if (changed && IS_CONNECTED) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
	PRIVATE_DATA->max_position = maximum;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = maximum;
	FOCUSER_POSITION_ITEM->number.max = maximum;
	FOCUSER_STEPS_ITEM->number.max = maximum;
	if (changed && IS_CONNECTED) {
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
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
	if (!isfinite(new_temp) || (new_temp < -270) || PRIVATE_DATA->moving) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}
	/* temperature difference if more than 1 degree so compensation needed */
	if ((fabs(temp_difference) >= FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, compensation = %d, steps/degC = %.0f, threshold = %.2f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %.2f, threshold = %.2f", temp_difference, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
		return;
	}
	if (!focuser_motion_ready(device)) {
		return;
	}
	int res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
		return;
	}
	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = (int)FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);
	res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->moving = true;
	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	float temp = NAN;
	int res = EAFGetTemp(PRIVATE_DATA->dev_id, &temp);
	if (isfinite(temp) && temp < -270 && (res == EAF_SUCCESS || res == EAF_ERROR_GENERAL_ERROR)) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temp;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, PRIVATE_DATA->has_temperature_sensor ? "The temperature sensor is not connected" : NULL);
		PRIVATE_DATA->has_temperature_sensor = false;
	} else if (res != EAF_SUCCESS || !isfinite(temp)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetTemp(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "Failed to read temperature");
	} else {
		PRIVATE_DATA->has_temperature_sensor = true;
		FOCUSER_TEMPERATURE_ITEM->number.value = temp;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
			compensate_focus(device, temp);
		}
	}
	if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
		PRIVATE_DATA->prev_temp = -273;
	}
	/* Update battery info for devices with battery info support */
	if (!EAF_BATTERY_INFO_PROPERTY->hidden) {
		EAF_BATTERY_INFO battery_info;
		res = EAFGetBatteryInfo(PRIVATE_DATA->dev_id, &battery_info);
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
	indigo_execute_handler_in(device, 2, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = asi_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int res = EAFGetBacklash(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->backlash));
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;
			res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_position));
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
				connection_result = false;
			}
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->target_position;
			res = EAFGetReverse(PRIVATE_DATA->dev_id, &(FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value));
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetReverse(%d, -> %d) = %d", PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
				connection_result = false;
			}
			FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
			res = EAFGetBeep(PRIVATE_DATA->dev_id, &(EAF_BEEP_ON_ITEM->sw.value));
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBeep(%d, -> %d) = %d", PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value, res);
				connection_result = false;
			}
			EAF_BEEP_OFF_ITEM->sw.value = !EAF_BEEP_ON_ITEM->sw.value;
			res = EAFGetMaxStep(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = (double)PRIVATE_DATA->max_position;
			int step_range = 0;
			res = EAFStepRange(PRIVATE_DATA->dev_id, &step_range);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFStepRange(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			if (step_range > 0) {
				FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = (double)step_range;
			} else {
				connection_result = false;
			}
			EAF_BATTERY_INFO_PROPERTY->hidden = true;
			EAF_BATTERY_INFO battery_info;
			res = EAFGetBatteryInfo(PRIVATE_DATA->dev_id, &battery_info);
			if (res == EAF_SUCCESS) {
				EAF_BATTERY_INFO_PROPERTY->hidden = false;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Battery info is supported for device %d", PRIVATE_DATA->dev_id);
			}
			PRIVATE_DATA->prev_temp = -273;  /* we do not have previous temperature reading */
			PRIVATE_DATA->moving = false;
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->target_position;
			if (connection_result) {
				focuser_update_limits(device, PRIVATE_DATA->max_position);
			} else {
				asi_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, EAF_BEEP_PROPERTY, NULL);
			indigo_define_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_define_property(device, EAF_BATTERY_INFO_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, EAF_BEEP_PROPERTY, NULL);
		indigo_delete_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
		indigo_delete_property(device, EAF_BATTERY_INFO_PROPERTY, NULL);
		asi_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_eaf_beep_handler(indigo_device *device) {
	EAF_BEEP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.EAF_BEEP.on_change
	int res = EAFSetBeep(PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBeep(%d, %d) = %d", PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value, res);
		EAF_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.EAF_BEEP.on_change
	indigo_update_property(device, EAF_BEEP_PROPERTY, NULL);
}

static void focuser_eaf_custom_suffix_handler(indigo_device *device) {
	EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.EAF_CUSTOM_SUFFIX.on_change
	if (strlen(EAF_CUSTOM_SUFFIX_ITEM->text.value) > 8) {
		EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Custom suffix too long");
		return;
	}
	EAF_ID eaf_id = { 0 };
	memcpy(eaf_id.id, EAF_CUSTOM_SUFFIX_ITEM->text.value, 8);
	int res = EAFSetID(PRIVATE_DATA->dev_id, eaf_id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, EAF_CUSTOM_SUFFIX_ITEM->text.value, res);
		EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(EAF_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, EAF_CUSTOM_SUFFIX_ITEM->text.value, res);
	memset(PRIVATE_DATA->custom_suffix, 0, sizeof(PRIVATE_DATA->custom_suffix));
	strncpy(PRIVATE_DATA->custom_suffix, EAF_CUSTOM_SUFFIX_ITEM->text.value, sizeof(PRIVATE_DATA->custom_suffix) - 1);
	if (strlen(EAF_CUSTOM_SUFFIX_ITEM->text.value) > 0) {
		indigo_send_message(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Focuser name suffix '#%s' will be used on replug", EAF_CUSTOM_SUFFIX_ITEM->text.value);
	} else {
		indigo_send_message(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Focuser name suffix cleared, will be used on replug");
	}
	//- focuser.EAF_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	int res = EAFSetReverse(PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetReverse(%d, %d) = %d", PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	if (!focuser_motion_ready(device)) {
		return;
	}
	double target = FOCUSER_POSITION_ITEM->number.target;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	PRIVATE_DATA->target_position = (int)target;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		int res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->moving = true;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else {
		int res = EAFResetPostion(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
		if (res == EAF_SUCCESS) {
			res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
		}
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		if (res != EAF_SUCCESS) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	double target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	int res = EAFSetMaxStep(PRIVATE_DATA->dev_id, (int)target);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetMaxStep(%d, %d) = %d", PRIVATE_DATA->dev_id, (int)target, res);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	int confirmed = PRIVATE_DATA->max_position;
	res = EAFGetMaxStep(PRIVATE_DATA->dev_id, &confirmed);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		focuser_update_limits(device, confirmed);
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	double target = FOCUSER_BACKLASH_ITEM->number.target;
	int res = EAFSetBacklash(PRIVATE_DATA->dev_id, (int)target);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBacklash(%d, %d) = %d", PRIVATE_DATA->dev_id, (int)target, res);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	int confirmed = PRIVATE_DATA->backlash;
	res = EAFGetBacklash(PRIVATE_DATA->dev_id, &confirmed);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->backlash = confirmed;
		FOCUSER_BACKLASH_ITEM->number.value = confirmed;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	if (!focuser_motion_ready(device)) {
		return;
	}
	int res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
	if (res != EAF_SUCCESS) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Failed to read focuser position");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return;
	}
	int steps = (int)FOCUSER_STEPS_ITEM->number.value;
	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -steps : steps);
	PRIVATE_DATA->target_position = (int)fmax(FOCUSER_POSITION_ITEM->number.min, fmin(FOCUSER_POSITION_ITEM->number.max, PRIVATE_DATA->target_position));
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->target_position;
	res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->moving = true;
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	int res = EAFStop(PRIVATE_DATA->dev_id);
	if (res != EAF_SUCCESS) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, "Failed to stop focuser");
		return;
	}
	indigo_cancel_pending_handler(device, focuser_move_finalizer);
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
	focuser_move_finalizer(device);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
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
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		const char *sdk_version = EAFGetSDKVersion();
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
		//- focuser.on_attach
		EAF_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, EAF_BEEP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (EAF_BEEP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(EAF_BEEP_ON_ITEM, EAF_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(EAF_BEEP_OFF_ITEM, EAF_BEEP_OFF_ITEM_NAME, "Off", true);
		EAF_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, EAF_CUSTOM_SUFFIX_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (EAF_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(EAF_CUSTOM_SUFFIX_ITEM, EAF_CUSTOM_SUFFIX_ITEM_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		EAF_BATTERY_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, EAF_BATTERY_INFO_PROPERTY_NAME, "Battery", "Battery information", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (EAF_BATTERY_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(EAF_BATTERY_CHARGE_ITEM, EAF_BATTERY_CHARGE_ITEM_NAME, "Charge (%)", 0, 100, 0, 0);
		indigo_init_number_item(EAF_BATTERY_TEMP_ITEM, EAF_BATTERY_TEMP_ITEM_NAME, "Temperature (°C)", -100, 100, 0, 0);
		indigo_init_number_item(EAF_BATTERY_VOLTAGE_ITEM, EAF_BATTERY_VOLTAGE_ITEM_NAME, "Voltage (V)", 0, 5, 0, 0);
		indigo_init_number_item(EAF_BATTERY_CHARGE_CURR_ITEM, EAF_BATTERY_CHARGE_CURR_ITEM_NAME, "Charge current (mA)", 0, 10000, 0, 0);
		indigo_init_number_item(EAF_BATTERY_DISCHARGE_CURR_ITEM, EAF_BATTERY_DISCHARGE_CURR_ITEM_NAME, "Discharge current (mA)", 0, 10000, 0, 0);
		indigo_init_number_item(EAF_BATTERY_HEALTH_ITEM, EAF_BATTERY_HEALTH_ITEM_NAME, "Health", 0, 100, 0, 0);
		indigo_init_number_item(EAF_BATTERY_CHARGE_VOL_ITEM, EAF_BATTERY_CHARGE_VOL_ITEM_NAME, "Charge voltage (V)", 0, 5, 0, 0);
		indigo_init_number_item(EAF_BATTERY_CYCLES_ITEM, EAF_BATTERY_CYCLES_ITEM_NAME, "Number of cycles", 0, 10000, 0, 0);
		EAF_BATTERY_INFO_PROPERTY->hidden = true;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_STEPS_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_BEEP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_CUSTOM_SUFFIX_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(EAF_BATTERY_INFO_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, focuser_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(EAF_BEEP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(EAF_BEEP_PROPERTY, focuser_eaf_beep_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(EAF_CUSTOM_SUFFIX_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(EAF_CUSTOM_SUFFIX_PROPERTY, focuser_eaf_custom_suffix_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, EAF_BEEP_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(EAF_BEEP_PROPERTY);
	indigo_release_property(EAF_CUSTOM_SUFFIX_PROPERTY);
	indigo_release_property(EAF_BATTERY_INFO_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	asi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == ASI_VENDOR_ID && descriptor.idProduct == EAF_PRODUCT_ID)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		int count = EAFGetNum();
		for (int index = 0; index < count; index++) {
			int id = -1;
			int res = EAFGetID(index, &id);
			if (res != EAF_SUCCESS || id < 0 || id >= EAF_ID_MAX) {
				continue;
			}
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && ((asi_private_data *)devices[slot]->private_data)->dev_id == id) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			res = EAFOpen(id);
			if (res != EAF_SUCCESS) {
				continue;
			}
			EAF_INFO info = { 0 };
			res = EAFGetProperty(id, &info);
			EAFClose(id);
			if (res != EAF_SUCCESS) {
				continue;
			}
			private_data->dev_id = id;
			private_data->info = info;
			private_data->has_temperature_sensor = true;
			split_device_name(info.Name, private_data->model, private_data->custom_suffix);
			if (private_data->custom_suffix[0]) {
				snprintf(name, INDIGO_NAME_SIZE, "%s #%s", private_data->model, private_data->custom_suffix);
			} else {
				snprintf(name, INDIGO_NAME_SIZE, "%s", private_data->model);
			}
			indigo_make_name_unique(name, "%d", id);
			plug_result = true;
			break;
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
		focuser->private_data = private_data;
		snprintf(focuser->name, INDIGO_NAME_SIZE, "%s", name);
		bool focuser_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = focuser;
				if (indigo_attach_device(focuser) == INDIGO_OK) {
					dev_ref_transferred = true;
					focuser_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!focuser_attached) {
			free(focuser);
		}
	}
	if (!dev_ref_transferred) {
		free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	asi_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
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

indigo_result indigo_focuser_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			const char *sdk_version = EAFGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "EAF SDK v. %s ", sdk_version);
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
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, EAF_PRODUCT_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
