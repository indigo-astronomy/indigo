// Copyright (c) 2017-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_usbv3.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_usbv3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_focuser_usbv3"
#define DRIVER_LABEL         "USB_Focus v3 Focuser"
#define FOCUSER_DEVICE_NAME  "USB_Focus v3"
#define PRIVATE_DATA         ((usbv3_private_data *)device->private_data)

#pragma mark - Property definitions

#define X_FOCUSER_STEP_SIZE_PROPERTY      (PRIVATE_DATA->x_focuser_step_size_property)
#define X_FOCUSER_FULL_STEP_ITEM          (X_FOCUSER_STEP_SIZE_PROPERTY->items + 0)
#define X_FOCUSER_HALF_STEP_ITEM          (X_FOCUSER_STEP_SIZE_PROPERTY->items + 1)

#define X_FOCUSER_STEP_SIZE_PROPERTY_NAME "X_FOCUSER_STEP_SIZE"
#define X_FOCUSER_FULL_STEP_ITEM_NAME     "FULL_STEP"
#define X_FOCUSER_HALF_STEP_ITEM_NAME     "HALF_STEP"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_step_size_property;
	//+ data
	char response[128];
	bool moving;
	bool abort;
	//- data
} usbv3_private_data;

#pragma mark - Low level code

//+ code

static bool usbv3_command(indigo_device *device, char *command, int response, ...) {
	va_list args;
	va_start(args, response);
	long result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
	va_end(args);
	if (response && result > 0) {
		result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		if (*PRIVATE_DATA->response == '*') {
			PRIVATE_DATA->moving = false;
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool usbv3_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle) {
		if (usbv3_command(device, "SWHOIS", true) && !strcmp(PRIVATE_DATA->response, "UFO")) {
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void usbv3_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (usbv3_command(device, "FTMPRO", true)) {
		if (sscanf(PRIVATE_DATA->response, "T=%lf", &FOCUSER_TEMPERATURE_ITEM->number.value) == 1) {
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, 10, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = usbv3_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int direction, stepmode, speed, stepsdeg = 0, threshold = 0, firmware, maxpos, sign;
			indigo_uni_discard(PRIVATE_DATA->handle);
			if (usbv3_command(device, "SGETAL", true)) {
				if (sscanf(PRIVATE_DATA->response, "C=%u-%u-%u-%u-%u-%u-%u", &direction, &stepmode, &speed, &stepsdeg, &threshold, &firmware, &maxpos) == 7) {
					indigo_set_switch(FOCUSER_DIRECTION_PROPERTY, FOCUSER_DIRECTION_PROPERTY->items + direction % 2, true);
					indigo_set_switch(X_FOCUSER_STEP_SIZE_PROPERTY, X_FOCUSER_STEP_SIZE_PROPERTY->items + stepmode % 2, true);
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = speed;
					INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DRIVER_LABEL);
					snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, sizeof(INFO_DEVICE_FW_REVISION_ITEM->text.value), "%d", firmware);
					FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = maxpos;
					indigo_update_property(device, INFO_PROPERTY, NULL);
				}
			}
			if (usbv3_command(device, "FPOSRO", true)) {
				if (sscanf(PRIVATE_DATA->response, "P=%lf", &FOCUSER_POSITION_ITEM->number.value) == 1) {
					indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				}
			}
			usbv3_command(device, "FMANUA", true);
			usbv3_command(device, "FTxxxA", true);
			if (sscanf(PRIVATE_DATA->response, "A=%d", &sign) == 1) {
				if (sign == 0)
					stepsdeg = -stepsdeg;
				FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = stepsdeg;
				FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value = FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target = threshold;
			}
			//- focuser.on_connect
			indigo_define_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		usbv3_command(device, "FQUITx", false);
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
		usbv3_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_x_focuser_step_size_handler(indigo_device *device) {
	X_FOCUSER_STEP_SIZE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_STEP_SIZE.on_change
	if (X_FOCUSER_FULL_STEP_ITEM->sw.value) {
		usbv3_command(device, "SMSTPF", false);
	} else {
		usbv3_command(device, "SMSTPD", false);
	}
	//- focuser.X_FOCUSER_STEP_SIZE.on_change
	indigo_update_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_temperature_handler(indigo_device *device) {
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	usbv3_command(device, "FLX%03d", true, abs((int)FOCUSER_COMPENSATION_ITEM->number.target));
	usbv3_command(device, "FZSIG%d", true, FOCUSER_COMPENSATION_ITEM->number.target < 0 ? 0 : 1);
	usbv3_command(device, "SMA%03d", true, (int)FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target);
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_timer_callback);
		usbv3_command(device, "FAUTOM", true);
	} else {
		usbv3_command(device, "FMANUA", true);					
		if (sscanf(PRIVATE_DATA->response, "P=%lf", &FOCUSER_POSITION_ITEM->number.value) == 1) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		indigo_execute_priority_handler(device, 100, focuser_timer_callback);
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	usbv3_command(device, "SMO%03u", false, (int)FOCUSER_SPEED_ITEM->number.target);
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	PRIVATE_DATA->abort = false;
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value) {
		int max = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
		if (position + steps > max) {
			steps = max - position;
		}
		if (steps > 0) {
			usbv3_command(device, "O%05u", false, steps);
		}
	} else {
		int min = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
		if (position - steps < min) {
			steps = position - min;
		}
		if (steps > 0) {
			usbv3_command(device, "I%05u", false, steps);
		}
	}
	if (steps > 0) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		PRIVATE_DATA->moving = true;
		int value;
		while (PRIVATE_DATA->moving) {
			if (PRIVATE_DATA->abort) {
				usbv3_command(device, "FQUITx", false);
			}
			usbv3_command(device, "FPOSRO", true);
			if (sscanf(PRIVATE_DATA->response, "P=%d", &value) == 1) {
				if (position != value) {
					FOCUSER_POSITION_ITEM->number.value = position = value;
					indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				}
			}
		}
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	PRIVATE_DATA->abort = false;
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	int min = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	int max = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	if (target < min) {
		target = min;
	}
	if (target > max) {
		target = max;
	}
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	int steps =  FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? target - position : position - target;
	if (steps > 0) {
		usbv3_command(device, "O%05u", false, steps);
	} else if (steps < 0) {
		usbv3_command(device, "I%05u", false, -steps);
	}
	if (steps != 0) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		PRIVATE_DATA->moving = true;
		int value;
		while (PRIVATE_DATA->moving) {
			if (PRIVATE_DATA->abort) {
				usbv3_command(device, "FQUITx", false);
			}
			usbv3_command(device, "FPOSRO", true);
			if (sscanf(PRIVATE_DATA->response, "P=%d", &value) == 1) {
				if (position != value) {
					FOCUSER_POSITION_ITEM->number.value = position = value;
					indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				}
			}
		}
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	PRIVATE_DATA->abort = true;
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	usbv3_command(device, "M%05u", false, (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target);
	//- focuser.FOCUSER_LIMITS.on_change
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
		X_FOCUSER_STEP_SIZE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_STEP_SIZE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Step size", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEP_SIZE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_FULL_STEP_ITEM, X_FOCUSER_FULL_STEP_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_FOCUSER_HALF_STEP_ITEM, X_FOCUSER_HALF_STEP_ITEM_NAME, "Half step", false);
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		FOCUSER_COMPENSATION_ITEM->number.min = -999;
		FOCUSER_COMPENSATION_ITEM->number.max = 999;
		FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.min = 0;
		FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.max = 5;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = 4;
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 8;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 65535;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 65535;
		//- focuser.FOCUSER_LIMITS.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_STEP_SIZE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
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
	} else if (indigo_property_match_changeable(X_FOCUSER_STEP_SIZE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_STEP_SIZE_PROPERTY, focuser_x_focuser_step_size_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_TEMPERATURE_PROPERTY, focuser_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_REVERSE_MOTION_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEP_SIZE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_usbv3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbv3_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "SERIAL DEMO");
			strcpy(patterns[0].vendor_string, "CCS");
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(usbv3_private_data));
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
