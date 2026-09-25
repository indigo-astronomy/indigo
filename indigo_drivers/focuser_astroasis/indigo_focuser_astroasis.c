// Copyright (C) 2024-2026 Astroasis Vision Technology, Inc.
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

// This file generated from indigo_focuser_astroasis.driver

// supported_architecture: !defined(__i386__)
#if !defined(__i386__)

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <AOFocus.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_focuser_astroasis.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000C
#define DRIVER_NAME          "indigo_focuser_astroasis"
#define DRIVER_LABEL         "Astroasis Oasis Focuser"
#define FOCUSER_DEVICE_NAME  "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((astroasis_private_data *)device->private_data)

//+ define

#define ASTROASIS_VENDOR_ID  0x338f
#define ASTROASIS_PRODUCT_FOCUSER_ID 0xa0f0

//- define

#pragma mark - Property definitions

#define X_BEEP_ON_POWER_UP_PROPERTY      (PRIVATE_DATA->x_beep_on_power_up_property)
#define X_BEEP_ON_POWER_UP_ON_ITEM       (X_BEEP_ON_POWER_UP_PROPERTY->items + 0)
#define X_BEEP_ON_POWER_UP_OFF_ITEM      (X_BEEP_ON_POWER_UP_PROPERTY->items + 1)

#define X_BEEP_ON_POWER_UP_PROPERTY_NAME "X_BEEP_ON_POWER_UP_PROPERTY"
#define X_BEEP_ON_POWER_UP_ON_ITEM_NAME  "ON"
#define X_BEEP_ON_POWER_UP_OFF_ITEM_NAME "OFF"

#define X_BEEP_ON_MOVE_PROPERTY        (PRIVATE_DATA->x_beep_on_move_property)
#define X_BEEP_ON_MOVE_ON_ITEM         (X_BEEP_ON_MOVE_PROPERTY->items + 0)
#define X_BEEP_ON_MOVE_OFF_ITEM        (X_BEEP_ON_MOVE_PROPERTY->items + 1)

#define X_BEEP_ON_MOVE_PROPERTY_NAME   "X_BEEP_ON_MOVE_PROPERTY"
#define X_BEEP_ON_MOVE_ON_ITEM_NAME    "ON"
#define X_BEEP_ON_MOVE_OFF_ITEM_NAME   "OFF"

#define X_BACKLASH_DIRECTION_PROPERTY      (PRIVATE_DATA->x_backlash_direction_property)
#define X_BACKLASH_DIRECTION_IN_ITEM       (X_BACKLASH_DIRECTION_PROPERTY->items + 0)
#define X_BACKLASH_DIRECTION_OUT_ITEM      (X_BACKLASH_DIRECTION_PROPERTY->items + 1)

#define X_BACKLASH_DIRECTION_PROPERTY_NAME "X_BACKLASH_DIRECTION_PROPERTY"
#define X_BACKLASH_DIRECTION_IN_ITEM_NAME  "INWARD"
#define X_BACKLASH_DIRECTION_OUT_ITEM_NAME "OUTWARD"

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

#define X_FACTORY_RESET_PROPERTY_NAME  "X_FACTORY_RESET_PROPERTY"
#define X_FACTORY_RESET_ITEM_NAME      "RESET"

#define X_BOARD_TEMPERATURE_PROPERTY      (PRIVATE_DATA->x_board_temperature_property)
#define X_BOARD_TEMPERATURE_ITEM          (X_BOARD_TEMPERATURE_PROPERTY->items + 0)

#define X_BOARD_TEMPERATURE_PROPERTY_NAME "X_BOARD_TEMPERATURE_PROPERTY"
#define X_BOARD_TEMPERATURE_ITEM_NAME     "Internal Temp."

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *x_beep_on_power_up_property;
	indigo_property *x_beep_on_move_property;
	indigo_property *x_backlash_direction_property;
	indigo_property *x_custom_suffix_property;
	indigo_property *x_bluetooth_property;
	indigo_property *x_bluetooth_name_property;
	indigo_property *x_factory_reset_property;
	indigo_property *x_board_temperature_property;
	//+ data
	int dev_id;
	AOFocuserConfig config;
	AOFocuserStatus status;
	char sdk_version[AO_FOCUSER_VERSION_LEN + 1];
	char firmware_version[AO_FOCUSER_VERSION_LEN + 1];
	char model[AO_FOCUSER_NAME_LEN + 1];
	char custom_suffix[AO_FOCUSER_NAME_LEN + 1];
	char bluetooth_name[AO_FOCUSER_NAME_LEN + 1];
	double compensation_last_temp;
	bool has_temperature_sensor;
	//- data
} astroasis_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

static bool astroasis_open(indigo_device *device) {
	int res = AOFocuserOpen(PRIVATE_DATA->dev_id);
	if (res != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserOpen() failed, ret = %d", res);
		return false;
	}
	return true;
}

static void astroasis_close(indigo_device *device) {
	AOFocuserClose(PRIVATE_DATA->dev_id);
}

static bool astroasis_config(indigo_device *device, unsigned int mask, int value) {
	AOFocuserConfig config = PRIVATE_DATA->config;
	config.mask = mask;
	switch (mask) {
		case MASK_MAX_STEP:
			config.maxStep = value;
			break;
		case MASK_BACKLASH:
			config.backlash = value;
			break;
		case MASK_BACKLASH_DIRECTION:
			config.backlashDirection = value;
			break;
		case MASK_REVERSE_DIRECTION:
			config.reverseDirection = value;
			break;
		case MASK_BEEP_ON_MOVE:
			config.beepOnMove = value;
			break;
		case MASK_BEEP_ON_STARTUP:
			config.beepOnStartup = value;
			break;
		case MASK_BLUETOOTH:
			config.bluetoothOn = value;
			break;
		default:
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid Oasis Focuser configuration mask %08X", mask);
			return false;
	}
	int res = AOFocuserSetConfig(PRIVATE_DATA->dev_id, &config);
	if (res != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set Oasis Focuser configuration, ret = %d", res);
		return false;
	}
	PRIVATE_DATA->config = config;
	return true;
}

//- code

//+ focuser.code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static void focuser_move_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	int res = AOFocuserGetStatus(PRIVATE_DATA->dev_id, &PRIVATE_DATA->status);
	if (res == AO_SUCCESS) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving = %d, Position = %d", PRIVATE_DATA->status.moving, PRIVATE_DATA->status.position);
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->status.position;
		if (!PRIVATE_DATA->status.moving) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserGetStatus() failed, ret = %d", res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_move_failed(indigo_device *device, int res) {
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to move Oasis Focuser, ret = %d", res);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_update_limits(indigo_device *device, int maximum) {
	bool changed = FOCUSER_POSITION_ITEM->number.max != maximum || FOCUSER_STEPS_ITEM->number.max != maximum;
	if (changed && IS_CONNECTED) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = maximum;
	FOCUSER_POSITION_ITEM->number.max = FOCUSER_STEPS_ITEM->number.max = maximum;
	if (changed && IS_CONNECTED) {
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
}

static void focuser_compensation(indigo_device *device, double curr_temp) {
	int compensation;
	double temp_diff = curr_temp - PRIVATE_DATA->compensation_last_temp;
	/* Last compensation temperature is invalid */
	if (PRIVATE_DATA->compensation_last_temp < -270) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation not started yet, last temperature = %f", PRIVATE_DATA->compensation_last_temp);
		PRIVATE_DATA->compensation_last_temp = curr_temp;
		return;
	}
	/* Current temperature is invalid or focuser is moving */
	if ((curr_temp < -270) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation not started: curr_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", curr_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}
	/* Temperature difference is big enough to do compensation */
	if ((fabs(temp_diff) >= FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) && (fabs(temp_diff) < 100)) {
		compensation = (int)(temp_diff * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temperature difference = %.2f, compensation = %d, steps/degC = %.0f, threshold = %.2f", temp_diff, compensation, FOCUSER_COMPENSATION_ITEM->number.value, FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
	} else {
		return;
	}
	int res = AOFocuserMove(PRIVATE_DATA->dev_id, compensation);
	PRIVATE_DATA->compensation_last_temp = curr_temp;
	if (res != AO_SUCCESS) {
		focuser_move_failed(device, res);
		return;
	}
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->status.position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
}

static void focuser_temperature_poll(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		return;
	}
	const char *property_message = NULL;
	int res = AOFocuserGetStatus(PRIVATE_DATA->dev_id, &PRIVATE_DATA->status);
	if (res == AO_SUCCESS) {
		X_BOARD_TEMPERATURE_ITEM->number.value = (double)PRIVATE_DATA->status.temperatureInt / 100;
		X_BOARD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		if (PRIVATE_DATA->status.temperatureDetection && ((unsigned int)PRIVATE_DATA->status.temperatureExt != TEMPERATURE_INVALID)) {
			FOCUSER_TEMPERATURE_ITEM->number.value = (double)PRIVATE_DATA->status.temperatureExt / 100;
			if (!PRIVATE_DATA->has_temperature_sensor) {
				property_message = "Temperature sensor connected.";
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%s", property_message);
				PRIVATE_DATA->has_temperature_sensor = true;
			}
		} else {
			FOCUSER_TEMPERATURE_ITEM->number.value = X_BOARD_TEMPERATURE_ITEM->number.value;
			if (PRIVATE_DATA->has_temperature_sensor) {
				property_message = "No temperature sensor connected. Using board temperature as ambient.";
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%s", property_message);
				PRIVATE_DATA->has_temperature_sensor = false;
			}
		}
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
			focuser_compensation(device, FOCUSER_TEMPERATURE_ITEM->number.value);
		} else {
			/* reset temp so that the compensation starts when auto mode is selected */
			PRIVATE_DATA->compensation_last_temp = -273.15;
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserGetStatus() failed, ret = %d", res);
		X_BOARD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_BOARD_TEMPERATURE_PROPERTY, NULL);
	if (property_message != NULL) {
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "%s", property_message);
	} else {
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 2, focuser_temperature_poll);
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = astroasis_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int res = AOFocuserGetConfig(PRIVATE_DATA->dev_id, &PRIVATE_DATA->config);
			if (res != AO_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserGetConfig() failed, ret = %d", res);
				connection_result = false;
				astroasis_close(device);
			} else {
				focuser_update_limits(device, PRIVATE_DATA->config.maxStep);
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->config.backlash;
				indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->config.reverseDirection ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				indigo_set_switch(X_BEEP_ON_POWER_UP_PROPERTY, PRIVATE_DATA->config.beepOnStartup ? X_BEEP_ON_POWER_UP_ON_ITEM : X_BEEP_ON_POWER_UP_OFF_ITEM, true);
				indigo_set_switch(X_BEEP_ON_MOVE_PROPERTY, PRIVATE_DATA->config.beepOnMove ? X_BEEP_ON_MOVE_ON_ITEM : X_BEEP_ON_MOVE_OFF_ITEM, true);
				indigo_set_switch(X_BACKLASH_DIRECTION_PROPERTY, PRIVATE_DATA->config.backlashDirection ? X_BACKLASH_DIRECTION_OUT_ITEM : X_BACKLASH_DIRECTION_IN_ITEM, true);
				res = AOFocuserGetConfig(PRIVATE_DATA->dev_id, &PRIVATE_DATA->config);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AOFocuserGetConfig(%d, -> .speed = %d .bluetoothOn = %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->config.speed, PRIVATE_DATA->config.bluetoothOn, res);
				indigo_set_switch(X_BLUETOOTH_PROPERTY, PRIVATE_DATA->config.bluetoothOn ? X_BLUETOOTH_ON_ITEM : X_BLUETOOTH_OFF_ITEM, true);
				res = AOFocuserGetBluetoothName(PRIVATE_DATA->dev_id, PRIVATE_DATA->bluetooth_name);
				PRIVATE_DATA->bluetooth_name[AO_FOCUSER_NAME_LEN] = 0;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AOFocuserGetBluetoothName(%d, -> \"%s\") = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->bluetooth_name, res);
				indigo_set_text_item_value(X_BLUETOOTH_NAME_ITEM, PRIVATE_DATA->bluetooth_name);
				INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
				PRIVATE_DATA->compensation_last_temp = -273.15;
				indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
				indigo_execute_handler_in(device, 0.1, focuser_temperature_poll);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_BEEP_ON_POWER_UP_PROPERTY, NULL);
			indigo_define_property(device, X_BEEP_ON_MOVE_PROPERTY, NULL);
			indigo_define_property(device, X_BACKLASH_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
			indigo_define_property(device, X_BLUETOOTH_PROPERTY, NULL);
			indigo_define_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
			indigo_define_property(device, X_FACTORY_RESET_PROPERTY, NULL);
			indigo_define_property(device, X_BOARD_TEMPERATURE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		int res = AOFocuserStopMove(PRIVATE_DATA->dev_id);
		if (res != AO_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserStopMove() failed, ret = %d", res);
		}
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			FOCUSER_REVERSE_MOTION_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
			FOCUSER_LIMITS_PROPERTY,
			FOCUSER_BACKLASH_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
			FOCUSER_COMPENSATION_PROPERTY,
			FOCUSER_MODE_PROPERTY,
			X_BEEP_ON_POWER_UP_PROPERTY,
			X_BEEP_ON_MOVE_PROPERTY,
			X_BACKLASH_DIRECTION_PROPERTY,
			X_CUSTOM_SUFFIX_PROPERTY,
			X_BLUETOOTH_PROPERTY,
			X_BLUETOOTH_NAME_PROPERTY,
			X_FACTORY_RESET_PROPERTY,
			X_BOARD_TEMPERATURE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_BEEP_ON_POWER_UP_PROPERTY, NULL);
		indigo_delete_property(device, X_BEEP_ON_MOVE_PROPERTY, NULL);
		indigo_delete_property(device, X_BACKLASH_DIRECTION_PROPERTY, NULL);
		indigo_delete_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
		indigo_delete_property(device, X_BLUETOOTH_PROPERTY, NULL);
		indigo_delete_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
		indigo_delete_property(device, X_FACTORY_RESET_PROPERTY, NULL);
		indigo_delete_property(device, X_BOARD_TEMPERATURE_PROPERTY, NULL);
		astroasis_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	if (!astroasis_config(device, MASK_REVERSE_DIRECTION, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value)) {
		indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->config.reverseDirection ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	if (target == PRIVATE_DATA->status.position) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->status.position;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		int res = AOFocuserMoveTo(PRIVATE_DATA->dev_id, target);
		if (res != AO_SUCCESS) {
			focuser_move_failed(device, res);
		} else {
			indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
		}
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->status.position;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		int res = AOFocuserSyncPosition(PRIVATE_DATA->dev_id, target);
		if (res != AO_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to sync Oasis Focuser, ret = %d", res);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = AOFocuserGetStatus(PRIVATE_DATA->dev_id, &PRIVATE_DATA->status);
		if (res != AO_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserGetStatus() failed, ret = %d", res);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->status.position;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int maximum = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	if (astroasis_config(device, MASK_MAX_STEP, maximum)) {
		focuser_update_limits(device, maximum);
	} else {
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	int backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;
	if (astroasis_config(device, MASK_BACKLASH, backlash)) {
		FOCUSER_BACKLASH_ITEM->number.value = backlash;
	} else {
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value;
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	int step = (int)(FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -FOCUSER_STEPS_ITEM->number.value : FOCUSER_STEPS_ITEM->number.value);
	int res = AOFocuserMove(PRIVATE_DATA->dev_id, step);
	if (res != AO_SUCCESS) {
		focuser_move_failed(device, res);
	} else {
		indigo_execute_handler_in(device, 0.5, focuser_move_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_steps_handler);
	indigo_cancel_pending_handler(device, focuser_move_finalizer);
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	int res = AOFocuserStopMove(PRIVATE_DATA->dev_id);
	if (res != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to stop Oasis Focuser, ret = %d", res);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	/* AOFocuserGetStatus() sometimes fails after a stop with comm error, so retry */
	for (int retry = 0; retry < 3; retry++) {
		res = AOFocuserGetStatus(PRIVATE_DATA->dev_id, &PRIVATE_DATA->status);
		if (res == AO_SUCCESS || res != AO_ERROR_COMMUNICATION) {
			break;
		}
		indigo_usleep(0.05 * ONE_SECOND_DELAY);
	}
	if (res != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get Oasis Focuser status, ret = %d", res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->status.position;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
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

static void focuser_x_beep_on_power_up_handler(indigo_device *device) {
	X_BEEP_ON_POWER_UP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_BEEP_ON_POWER_UP.on_change
	if (!astroasis_config(device, MASK_BEEP_ON_STARTUP, X_BEEP_ON_POWER_UP_ON_ITEM->sw.value)) {
		indigo_set_switch(X_BEEP_ON_POWER_UP_PROPERTY, PRIVATE_DATA->config.beepOnStartup ? X_BEEP_ON_POWER_UP_ON_ITEM : X_BEEP_ON_POWER_UP_OFF_ITEM, true);
		X_BEEP_ON_POWER_UP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_BEEP_ON_POWER_UP.on_change
	indigo_update_property(device, X_BEEP_ON_POWER_UP_PROPERTY, NULL);
}

static void focuser_x_beep_on_move_handler(indigo_device *device) {
	X_BEEP_ON_MOVE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_BEEP_ON_MOVE.on_change
	if (!astroasis_config(device, MASK_BEEP_ON_MOVE, X_BEEP_ON_MOVE_ON_ITEM->sw.value)) {
		indigo_set_switch(X_BEEP_ON_MOVE_PROPERTY, PRIVATE_DATA->config.beepOnMove ? X_BEEP_ON_MOVE_ON_ITEM : X_BEEP_ON_MOVE_OFF_ITEM, true);
		X_BEEP_ON_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_BEEP_ON_MOVE.on_change
	indigo_update_property(device, X_BEEP_ON_MOVE_PROPERTY, NULL);
}

static void focuser_x_backlash_direction_handler(indigo_device *device) {
	X_BACKLASH_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_BACKLASH_DIRECTION.on_change
	if (!astroasis_config(device, MASK_BACKLASH_DIRECTION, X_BACKLASH_DIRECTION_OUT_ITEM->sw.value)) {
		indigo_set_switch(X_BACKLASH_DIRECTION_PROPERTY, PRIVATE_DATA->config.backlashDirection ? X_BACKLASH_DIRECTION_OUT_ITEM : X_BACKLASH_DIRECTION_IN_ITEM, true);
		X_BACKLASH_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_BACKLASH_DIRECTION.on_change
	indigo_update_property(device, X_BACKLASH_DIRECTION_PROPERTY, NULL);
}

static void focuser_x_custom_suffix_handler(indigo_device *device) {
	X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_CUSTOM_SUFFIX.on_change
	if (strlen(X_CUSTOM_SUFFIX_ITEM->text.value) > AO_FOCUSER_NAME_LEN) {
		INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, "Custom suffix is too long");
		return;
	}
	char suffix[AO_FOCUSER_NAME_LEN + 1] = { 0 };
	snprintf(suffix, sizeof(suffix), "%s", X_CUSTOM_SUFFIX_ITEM->text.value);
	int res = AOFocuserSetFriendlyName(PRIVATE_DATA->dev_id, suffix);
	if (res != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set Oasis Focuser custom suffix, ret = %d", res);
		INDIGO_COPY_VALUE(X_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		X_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		snprintf(PRIVATE_DATA->custom_suffix, sizeof(PRIVATE_DATA->custom_suffix), "%s", suffix);
	}
	//- focuser.X_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, X_CUSTOM_SUFFIX_PROPERTY, NULL);
}

static void focuser_x_bluetooth_handler(indigo_device *device) {
	X_BLUETOOTH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_BLUETOOTH.on_change
	if (!astroasis_config(device, MASK_BLUETOOTH, X_BLUETOOTH_ON_ITEM->sw.value)) {
		indigo_set_switch(X_BLUETOOTH_PROPERTY, PRIVATE_DATA->config.bluetoothOn ? X_BLUETOOTH_ON_ITEM : X_BLUETOOTH_OFF_ITEM, true);
		X_BLUETOOTH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_BLUETOOTH.on_change
	indigo_update_property(device, X_BLUETOOTH_PROPERTY, NULL);
}

static void focuser_x_bluetooth_name_handler(indigo_device *device) {
	X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_BLUETOOTH_NAME.on_change
	if (strlen(X_BLUETOOTH_NAME_ITEM->text.value) > AO_FOCUSER_NAME_LEN) {
		INDIGO_COPY_VALUE(X_BLUETOOTH_NAME_ITEM->text.value, PRIVATE_DATA->bluetooth_name);
		X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_BLUETOOTH_NAME_PROPERTY, "Bluetooth name is too long");
		return;
	}
	char bluetooth_name[AO_FOCUSER_NAME_LEN + 1] = { 0 };
	snprintf(bluetooth_name, sizeof(bluetooth_name), "%s", X_BLUETOOTH_NAME_ITEM->text.value);
	int res = AOFocuserSetBluetoothName(PRIVATE_DATA->dev_id, bluetooth_name);
	if (res != AO_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set the Bluetooth name for the Oasis Focuser, ret = %d", res);
		INDIGO_COPY_VALUE(X_BLUETOOTH_NAME_ITEM->text.value, PRIVATE_DATA->bluetooth_name);
		X_BLUETOOTH_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		snprintf(PRIVATE_DATA->bluetooth_name, sizeof(PRIVATE_DATA->bluetooth_name), "%s", bluetooth_name);
	}
	//- focuser.X_BLUETOOTH_NAME.on_change
	indigo_update_property(device, X_BLUETOOTH_NAME_PROPERTY, NULL);
}

static void focuser_x_factory_reset_handler(indigo_device *device) {
	X_FACTORY_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FACTORY_RESET.on_change
	if (X_FACTORY_RESET_ITEM->sw.value) {
		X_FACTORY_RESET_ITEM->sw.value = false;
		int res = AOFocuserFactoryReset(PRIVATE_DATA->dev_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "AOFocuserFactoryReset(%d) = %d", PRIVATE_DATA->dev_id, res);
		if (res != AO_SUCCESS) {
			X_FACTORY_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_FACTORY_RESET_PROPERTY, "Factory reset failed");
		} else {
			indigo_update_property(device, X_FACTORY_RESET_PROPERTY, "Factory reset completed");
		}
		return;
	}
	//- focuser.X_FACTORY_RESET.on_change
	indigo_update_property(device, X_FACTORY_RESET_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser.on_attach
		INFO_PROPERTY->count = 7;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware_version);
		INDIGO_COPY_VALUE(INFO_DEVICE_HW_REVISION_ITEM->text.value, PRIVATE_DATA->sdk_version);
		INDIGO_COPY_VALUE(INFO_DEVICE_HW_REVISION_ITEM->label, "SDK version");
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 0x7fffffff;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 10000;
		FOCUSER_BACKLASH_ITEM->number.step = 1;
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->config.maxStep;
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->config.maxStep;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		INDIGO_COPY_VALUE(FOCUSER_TEMPERATURE_PROPERTY->label, "Temperature 2 (Ambient)");
		//- focuser.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_STEPS_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
		X_BEEP_ON_POWER_UP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_BEEP_ON_POWER_UP_PROPERTY_NAME, "Advanced", "Beep on power up", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_BEEP_ON_POWER_UP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_BEEP_ON_POWER_UP_ON_ITEM, X_BEEP_ON_POWER_UP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(X_BEEP_ON_POWER_UP_OFF_ITEM, X_BEEP_ON_POWER_UP_OFF_ITEM_NAME, "Off", true);
		X_BEEP_ON_MOVE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_BEEP_ON_MOVE_PROPERTY_NAME, "Advanced", "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_BEEP_ON_MOVE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_BEEP_ON_MOVE_ON_ITEM, X_BEEP_ON_MOVE_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(X_BEEP_ON_MOVE_OFF_ITEM, X_BEEP_ON_MOVE_OFF_ITEM_NAME, "Off", true);
		X_BACKLASH_DIRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_BACKLASH_DIRECTION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Backlash compensation overshot direction", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_BACKLASH_DIRECTION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_BACKLASH_DIRECTION_IN_ITEM, X_BACKLASH_DIRECTION_IN_ITEM_NAME, "Inward", false);
		indigo_init_switch_item(X_BACKLASH_DIRECTION_OUT_ITEM, X_BACKLASH_DIRECTION_OUT_ITEM_NAME, "Outward", true);
		X_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, X_CUSTOM_SUFFIX_PROPERTY_NAME, "Advanced", "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
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
		X_BLUETOOTH_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, X_BLUETOOTH_NAME_PROPERTY_NAME, "Advanced", "Bluetooth name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_BLUETOOTH_NAME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_BLUETOOTH_NAME_ITEM, X_BLUETOOTH_NAME_ITEM_NAME, "Bluetooth name", PRIVATE_DATA->bluetooth_name);
		X_FACTORY_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FACTORY_RESET_PROPERTY_NAME, "Advanced", "Factory reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FACTORY_RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FACTORY_RESET_ITEM, X_FACTORY_RESET_ITEM_NAME, "Reset", false);
		//+ focuser.X_FACTORY_RESET.on_attach
		INDIGO_COPY_VALUE(X_FACTORY_RESET_ITEM->hints, "warn_on_set:\"Confirm focuser factory reset?\";");
		//- focuser.X_FACTORY_RESET.on_attach
		X_BOARD_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device->name, X_BOARD_TEMPERATURE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Temperature 1 (Board)", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_BOARD_TEMPERATURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_BOARD_TEMPERATURE_ITEM, X_BOARD_TEMPERATURE_ITEM_NAME, "Temperature (°C)", -50, 50, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BEEP_ON_POWER_UP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BEEP_ON_MOVE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BACKLASH_DIRECTION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CUSTOM_SUFFIX_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BLUETOOTH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BLUETOOTH_NAME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FACTORY_RESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BOARD_TEMPERATURE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BEEP_ON_POWER_UP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BEEP_ON_POWER_UP_PROPERTY, focuser_x_beep_on_power_up_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BEEP_ON_MOVE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BEEP_ON_MOVE_PROPERTY, focuser_x_beep_on_move_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BACKLASH_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BACKLASH_DIRECTION_PROPERTY, focuser_x_backlash_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CUSTOM_SUFFIX_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CUSTOM_SUFFIX_PROPERTY, focuser_x_custom_suffix_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BLUETOOTH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BLUETOOTH_PROPERTY, focuser_x_bluetooth_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_BLUETOOTH_NAME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_BLUETOOTH_NAME_PROPERTY, focuser_x_bluetooth_name_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FACTORY_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FACTORY_RESET_PROPERTY, focuser_x_factory_reset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_BEEP_ON_MOVE_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_BEEP_ON_POWER_UP_PROPERTY);
	indigo_release_property(X_BEEP_ON_MOVE_PROPERTY);
	indigo_release_property(X_BACKLASH_DIRECTION_PROPERTY);
	indigo_release_property(X_CUSTOM_SUFFIX_PROPERTY);
	indigo_release_property(X_BLUETOOTH_PROPERTY);
	indigo_release_property(X_BLUETOOTH_NAME_PROPERTY);
	indigo_release_property(X_FACTORY_RESET_PROPERTY);
	indigo_release_property(X_BOARD_TEMPERATURE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

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
	astroasis_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (astroasis_private_data *)indigo_safe_malloc(sizeof(astroasis_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == ASTROASIS_VENDOR_ID && descriptor.idProduct == ASTROASIS_PRODUCT_FOCUSER_ID)) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		bool duplicate = false;
		for (int slot = 0; slot < MAX_DEVICES; slot++) {
			if (devices[slot] != NULL && ((astroasis_private_data *)devices[slot]->private_data)->usbdev == dev) {
				duplicate = true;
				break;
			}
		}
		int number = 0, ids[AO_FOCUSER_MAX_NUM] = { 0 };
		if (!duplicate && AOFocuserScan(&number, ids) == AO_SUCCESS && number >= 0 && number <= AO_FOCUSER_MAX_NUM) {
			for (int index = 0; index < number && !plug_result; index++) {
				int id = ids[index];
				bool attached = false;
				for (int slot = 0; slot < MAX_DEVICES; slot++) {
					if (devices[slot] != NULL && ((astroasis_private_data *)devices[slot]->private_data)->dev_id == id) {
						attached = true;
						break;
					}
				}
				if (attached) {
					continue;
				}
				int res = AOFocuserOpen(id);
				if (res != AO_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "AOFocuserOpen() failed, ret = %d", res);
					continue;
				}
				AOFocuserVersion version = { 0 };
				AOFocuserConfig config = { 0 };
				char model[AO_FOCUSER_NAME_LEN + 1] = { 0 };
				char suffix[AO_FOCUSER_NAME_LEN + 1] = { 0 };
				char bluetooth_name[AO_FOCUSER_NAME_LEN + 1] = { 0 };
				res = AOFocuserGetVersion(id, &version);
				if (res == AO_SUCCESS) {
					res = AOFocuserGetProductModel(id, model);
				}
				if (res == AO_SUCCESS) {
					res = AOFocuserGetFriendlyName(id, suffix);
				}
				if (res == AO_SUCCESS) {
					res = AOFocuserGetBluetoothName(id, bluetooth_name);
				}
				if (res == AO_SUCCESS) {
					res = AOFocuserGetConfig(id, &config);
				}
				if (res == AO_SUCCESS) {
					model[AO_FOCUSER_NAME_LEN] = suffix[AO_FOCUSER_NAME_LEN] = bluetooth_name[AO_FOCUSER_NAME_LEN] = 0;
					private_data->dev_id = id;
					private_data->config = config;
					private_data->has_temperature_sensor = true;
					AOFocuserGetSDKVersion(private_data->sdk_version);
					snprintf(private_data->firmware_version, sizeof(private_data->firmware_version), "%u.%u.%u", version.firmware >> 24, (version.firmware & 0x00FF0000) >> 16, (version.firmware & 0x0000FF00) >> 8);
					snprintf(private_data->model, sizeof(private_data->model), "%s", model);
					snprintf(private_data->custom_suffix, sizeof(private_data->custom_suffix), "%s", suffix);
					snprintf(private_data->bluetooth_name, sizeof(private_data->bluetooth_name), "%s", bluetooth_name);
					snprintf(name, INDIGO_NAME_SIZE, "Oasis Focuser%s%s", suffix[0] ? " #" : "", suffix);
					indigo_make_name_unique(name, "%d", id);
					plug_result = true;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Oasis Focuser %d probe failed, ret = %d", id, res);
				}
				AOFocuserClose(id);
			}
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
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
			indigo_safe_free(focuser);
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
	astroasis_private_data *private_data = NULL;
	astroasis_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (!unplug_result && last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				int number = 0, ids[AO_FOCUSER_MAX_NUM] = { 0 };
				unplug_result = AOFocuserScan(&number, ids) == AO_SUCCESS && number >= 0 && number <= AO_FOCUSER_MAX_NUM;
				for (int index = 0; unplug_result && index < number; index++) {
					if (ids[index] == private_data->dev_id) {
						unplug_result = false;
					}
				}
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

indigo_result indigo_focuser_astroasis(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			char sdk_version[AO_FOCUSER_VERSION_LEN + 1] = { 0 };
			AOFocuserGetSDKVersion(sdk_version);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Oasis Focuser SDK version: %s", sdk_version);
			AOFocuserSetLogLevel(indigo_get_log_level() >= INDIGO_LOG_DEBUG ? AO_LOG_LEVEL_DEBUG : AO_LOG_LEVEL_QUIET);
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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, ASTROASIS_VENDOR_ID, ASTROASIS_PRODUCT_FOCUSER_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
#include "indigo_focuser_astroasis.h"

indigo_result indigo_focuser_astroasis(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, "Astroasis Oasis Focuser", __FUNCTION__, 0x0300000C, false, INDIGO_DRIVER_SHUTDOWN);
	return action == INDIGO_DRIVER_INFO ? INDIGO_OK : INDIGO_UNSUPPORTED_ARCH;
}
#endif
