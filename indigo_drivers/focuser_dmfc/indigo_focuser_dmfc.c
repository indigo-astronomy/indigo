// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_dmfc.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_dmfc.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000F
#define DRIVER_NAME          "indigo_focuser_dmfc"
#define DRIVER_LABEL         "PegasusAstro DMFC Focuser"
#define FOCUSER_DEVICE_NAME  "Pegasus DMFC"
#define PRIVATE_DATA         ((dmfc_private_data *)device->private_data)

#pragma mark - Property definitions

#define X_FOCUSER_MOTOR_TYPE_PROPERTY          (PRIVATE_DATA->x_focuser_motor_type_property)
#define X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM      (X_FOCUSER_MOTOR_TYPE_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_TYPE_DC_ITEM           (X_FOCUSER_MOTOR_TYPE_PROPERTY->items + 1)

#define X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME     "X_FOCUSER_MOTOR_TYPE"
#define X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME "STEPPER"
#define X_FOCUSER_MOTOR_TYPE_DC_ITEM_NAME      "DC"

#define X_FOCUSER_ENCODER_PROPERTY           (PRIVATE_DATA->x_focuser_encoder_property)
#define X_FOCUSER_ENCODER_ENABLED_ITEM       (X_FOCUSER_ENCODER_PROPERTY->items + 0)
#define X_FOCUSER_ENCODER_DISABLED_ITEM      (X_FOCUSER_ENCODER_PROPERTY->items + 1)

#define X_FOCUSER_ENCODER_PROPERTY_NAME      "X_FOCUSER_ENCODER"
#define X_FOCUSER_ENCODER_ENABLED_ITEM_NAME  "ENABLED"
#define X_FOCUSER_ENCODER_DISABLED_ITEM_NAME "DISABLED"

#define X_FOCUSER_LED_PROPERTY           (PRIVATE_DATA->x_focuser_led_property)
#define X_FOCUSER_LED_ENABLED_ITEM       (X_FOCUSER_LED_PROPERTY->items + 0)
#define X_FOCUSER_LED_DISABLED_ITEM      (X_FOCUSER_LED_PROPERTY->items + 1)

#define X_FOCUSER_LED_PROPERTY_NAME      "X_FOCUSER_LED"
#define X_FOCUSER_LED_ENABLED_ITEM_NAME  "ENABLED"
#define X_FOCUSER_LED_DISABLED_ITEM_NAME "DISABLED"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_motor_type_property;
	indigo_property *x_focuser_encoder_property;
	indigo_property *x_focuser_led_property;
	//+ data
	char response[128];
	//- data
} dmfc_private_data;

#pragma mark - Low level code

//+ code

static bool dmfc_command(indigo_device *device, char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
		if (result > 0 && command[0] != 'C' && command[0] != 'H' && command[0] != 'M' && command[0] != 'G' && command[0] != 'S' && command[0] != 'W') {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool dmfc_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (dmfc_command(device, "#") && !strncmp(PRIVATE_DATA->response, "OK_", 3)) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value ,"DMFC Focuser");
			if (dmfc_command(device, "V")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void dmfc_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (dmfc_command(device, "T")) {
		double temp = indigo_atod(PRIVATE_DATA->response);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	bool update = false;
	if (dmfc_command(device, "P")) {
		int pos = atoi(PRIVATE_DATA->response);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			update = true;
		}
	}
	if (dmfc_command(device, "I")) {
		if (PRIVATE_DATA->response[0] == '0') {
			if (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				update = true;
			}
		} else {
			if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				update = true;
			}
		}
	}
	if (update) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = dmfc_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			if (dmfc_command(device, "A") && !strncmp(PRIVATE_DATA->response, "OK_", 3)) {
				char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, token + 3);
				token = strtok_r(NULL, ":", &pnt); // status
				if (token) { // version
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, token);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // motor
					indigo_set_switch(X_FOCUSER_MOTOR_TYPE_PROPERTY, *token == '1' ? X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM : X_FOCUSER_MOTOR_TYPE_DC_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // temperature
					FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = indigo_atod(token);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // position
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atoi(token);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // moving status
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // led status
					indigo_set_switch(X_FOCUSER_LED_PROPERTY, *token == '1' ? X_FOCUSER_LED_ENABLED_ITEM : X_FOCUSER_LED_DISABLED_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // reverse
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, *token == '1' ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // encoder
					indigo_set_switch(X_FOCUSER_ENCODER_PROPERTY, *token == '1' ? X_FOCUSER_ENCODER_DISABLED_ITEM: X_FOCUSER_ENCODER_ENABLED_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // backlash
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = atoi(token);
				}
			}
			//- focuser.on_connect
			indigo_define_property(device, X_FOCUSER_MOTOR_TYPE_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_ENCODER_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_LED_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, X_FOCUSER_MOTOR_TYPE_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_ENCODER_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_LED_PROPERTY, NULL);
		dmfc_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_x_focuser_motor_type_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_MOTOR_TYPE.on_change
	if (!dmfc_command(device, "R:%d", X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM->sw.value ? 1 : 0)) {
		X_FOCUSER_MOTOR_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_MOTOR_TYPE.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_TYPE_PROPERTY, NULL);
}

static void focuser_x_focuser_encoder_handler(indigo_device *device) {
	X_FOCUSER_ENCODER_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_ENCODER.on_change
	if (!dmfc_command(device, "E:%d", X_FOCUSER_ENCODER_DISABLED_ITEM->sw.value ? 1 : 0)) {
		X_FOCUSER_ENCODER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_ENCODER.on_change
	indigo_update_property(device, X_FOCUSER_ENCODER_PROPERTY, NULL);
}

static void focuser_x_focuser_led_handler(indigo_device *device) {
	X_FOCUSER_LED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_LED.on_change
	if (!dmfc_command(device, "L:%d", X_FOCUSER_LED_ENABLED_ITEM->sw.value ? 1 : 0)) {
		X_FOCUSER_LED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_LED.on_change
	indigo_update_property(device, X_FOCUSER_LED_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!dmfc_command(device, "C:%d", (int)FOCUSER_BACKLASH_ITEM->number.value)) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	if (!dmfc_command(device, "N:%d", FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 0 : 1)) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_temperature_handler(indigo_device *device) {
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!dmfc_command(device, "S:%d", (int)FOCUSER_SPEED_ITEM->number.value)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	if (dmfc_command(device, "G:%d", (int)FOCUSER_STEPS_ITEM->number.target * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1))) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_on_position_set_handler(indigo_device *device) {
	FOCUSER_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value) {
		position = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	}
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value) {
		position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	}
	FOCUSER_POSITION_ITEM->number.target = position;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		if (dmfc_command(device, "M:%d", position)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		if (!dmfc_command(device, "W:%d", position)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (dmfc_command(device, "H")) {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- focuser.on_attach
		X_FOCUSER_MOTOR_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_MOTOR_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM, X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME, "Stepper motor", false);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_DC_ITEM, X_FOCUSER_MOTOR_TYPE_DC_ITEM_NAME, "DC Motor", false);
		X_FOCUSER_ENCODER_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_ENCODER_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Encoder state", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_ENCODER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_ENCODER_ENABLED_ITEM, X_FOCUSER_ENCODER_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_FOCUSER_ENCODER_DISABLED_ITEM, X_FOCUSER_ENCODER_DISABLED_ITEM_NAME, "Disabled", false);
		X_FOCUSER_LED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_LED_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "LED status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_LED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_LED_ENABLED_ITEM, X_FOCUSER_LED_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_FOCUSER_LED_DISABLED_ITEM, X_FOCUSER_LED_DISABLED_ITEM_NAME, "Disabled", false);
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = -9999999;
		strcpy(FOCUSER_LIMITS_MIN_POSITION_ITEM->number.format, "%.0f");
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 9999999;
		strcpy(FOCUSER_LIMITS_MAX_POSITION_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_LIMITS.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_TYPE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_ENCODER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_LED_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_TYPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_TYPE_PROPERTY, focuser_x_focuser_motor_type_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_ENCODER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_ENCODER_PROPERTY, focuser_x_focuser_encoder_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_LED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_LED_PROPERTY, focuser_x_focuser_led_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_TEMPERATURE_PROPERTY, focuser_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ON_POSITION_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ON_POSITION_SET_PROPERTY, focuser_on_position_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_MOTOR_TYPE_PROPERTY);
	indigo_release_property(X_FOCUSER_ENCODER_PROPERTY);
	indigo_release_property(X_FOCUSER_LED_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_dmfc(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static dmfc_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[2] = { 0 };
			strcpy(patterns[0].product_string, "DMFC");
			strcpy(patterns[0].vendor_string, "Pegasus Astro");
			strcpy(patterns[1].product_string, "FocusCube");
			strcpy(patterns[1].vendor_string, "Pegasus Astro");
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 2);
			private_data = indigo_safe_malloc(sizeof(dmfc_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				free(focuser);
				focuser = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
