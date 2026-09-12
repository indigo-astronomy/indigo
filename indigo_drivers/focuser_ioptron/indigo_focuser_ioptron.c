// Copyright (c) 2024-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_ioptron.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_ioptron.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_focuser_ioptron"
#define DRIVER_LABEL         "iOptron iEAF Focuser"
#define FOCUSER_DEVICE_NAME  "iOptron iEAF"
#define PRIVATE_DATA         ((ioptron_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)

//- define

#pragma mark - Property definitions

#define X_FOCUSER_ZERO_SYNC_PROPERTY      (PRIVATE_DATA->x_focuser_zero_sync_property)
#define X_FOCUSER_ZERO_SYNC_ITEM          (X_FOCUSER_ZERO_SYNC_PROPERTY->items + 0)

#define X_FOCUSER_ZERO_SYNC_PROPERTY_NAME "X_FOCUSER_ZERO_SYNC"
#define X_FOCUSER_ZERO_SYNC_ITEM_NAME     "SYNC"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_zero_sync_property;
	//+ data
	char response[32];
	int position, temperature, last_position, stalled;
	bool reversed, moving, active, uncertain;
	//- data
} ioptron_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static bool ioptron_command(indigo_device *device, bool reply, const char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result <= 0) {
		return false;
	}
	if (!reply) {
		return true;
	}
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, RESPONSE, sizeof(PRIVATE_DATA->response) - 1, "#", "", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count <= 1 || RESPONSE[count - 1] != '#' || (long)strlen(RESPONSE) != count) {
		return false;
	}
	RESPONSE[count - 1] = 0;
	return true;
}

static bool ioptron_field(const char *text, int count, int *value) {
	*value = 0;
	for (int i = 0; i < count; i++) {
		if (text[i] < '0' || text[i] > '9') {
			return false;
		}
		*value = *value * 10 + text[i] - '0';
	}
	return true;
}

static bool ioptron_status(indigo_device *device) {
	int position = 0, moving = 0, temperature = 0, direction = 0;
	if (!ioptron_command(device, true, ":FI#") || strlen(RESPONSE) != 14 || !ioptron_field(RESPONSE, 7, &position) || !ioptron_field(RESPONSE + 7, 1, &moving) || !ioptron_field(RESPONSE + 8, 5, &temperature) || !ioptron_field(RESPONSE + 13, 1, &direction) || position > 99999 || moving > 1 || direction > 1) {
		return false;
	}
	PRIVATE_DATA->position = position;
	PRIVATE_DATA->moving = moving;
	PRIVATE_DATA->temperature = temperature;
	PRIVATE_DATA->reversed = direction == 0;
	return true;
}

static bool ioptron_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
	if (!PRIVATE_DATA->handle) {
		return false;
	}
	// Preserve the hardware's legacy USB startup settling interval, on the handler queue.
	indigo_sleep(2);
	int position = 0, model = 0, firmware = 0;
	if (ioptron_command(device, true, ":DeviceInfo#") && strlen(RESPONSE) == 12 && ioptron_field(RESPONSE, 6, &position) && ioptron_field(RESPONSE + 6, 2, &model) && ioptron_field(RESPONSE + 8, 4, &firmware) && position <= 99999 && (model == 2 || model == 3)) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, model == 2 ? "iEAF" : "iAFS");
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%04d", firmware);
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void ioptron_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void ioptron_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void ioptron_publish(indigo_device *device) {
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->position;
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain) {
		FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
	}
	double temperature = PRIVATE_DATA->temperature / 100.0 - 273.15;
	if (temperature >= FOCUSER_TEMPERATURE_ITEM->number.min && temperature <= FOCUSER_TEMPERATURE_ITEM->number.max) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reversed ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void ioptron_read_error(indigo_device *device) {
	ioptron_motion_state(device, INDIGO_ALERT_STATE);
	FOCUSER_TEMPERATURE_PROPERTY->state = FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	if (!ioptron_status(device)) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		ioptron_command(device, false, ":FQ#");
		ioptron_read_error(device);
		return;
	}
	ioptron_publish(device);
	if (!PRIVATE_DATA->moving) {
		PRIVATE_DATA->active = false;
		ioptron_motion_state(device, PRIVATE_DATA->position == (int)FOCUSER_POSITION_ITEM->number.target ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
	} else if (PRIVATE_DATA->position == PRIVATE_DATA->last_position && ++PRIVATE_DATA->stalled >= 100) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		ioptron_command(device, false, ":FQ#");
		ioptron_motion_state(device, INDIGO_ALERT_STATE);
	} else {
		if (PRIVATE_DATA->position != PRIVATE_DATA->last_position) {
			PRIVATE_DATA->last_position = PRIVATE_DATA->position;
			PRIVATE_DATA->stalled = 0;
		}
		ioptron_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
}

static void ioptron_start_motion(indigo_device *device, int target) {
	if (!IS_CONNECTED || PRIVATE_DATA->moving || PRIVATE_DATA->uncertain) {
		ioptron_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	target = target < 0 ? 0 : target > 99999 ? 99999 : target;
	FOCUSER_POSITION_ITEM->number.target = target;
	if (target == PRIVATE_DATA->position) {
		ioptron_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	if (!ioptron_command(device, false, ":FM%7d#", target)) {
		PRIVATE_DATA->uncertain = true;
		ioptron_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->active = true;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	PRIVATE_DATA->stalled = 0;
	ioptron_motion_state(device, INDIGO_BUSY_STATE);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && (PRIVATE_DATA->moving || (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE))) {
		if (ioptron_status(device)) {
			if (!PRIVATE_DATA->moving) {
				PRIVATE_DATA->uncertain = false;
			}
			ioptron_publish(device);
			ioptron_motion_state(device, PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
		} else {
			ioptron_read_error(device);
		}
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = ioptron_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			connection_result = ioptron_status(device);
			if (connection_result) {
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
				ioptron_publish(device);
				ioptron_motion_state(device, PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				// Single-device generated connection failure does not close a successful open.
				ioptron_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		if (PRIVATE_DATA->active || PRIVATE_DATA->moving || PRIVATE_DATA->uncertain) {
			ioptron_command(device, false, ":FQ#");
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = PRIVATE_DATA->moving = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
		ioptron_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	ioptron_start_motion(device, (int)FOCUSER_POSITION_ITEM->number.target);
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int delta = (int)FOCUSER_STEPS_ITEM->number.value;
	ioptron_start_motion(device, PRIVATE_DATA->position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -delta : delta));
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	bool requested = FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	bool result = IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->moving && !PRIVATE_DATA->uncertain && ioptron_status(device);
	if (result && requested != PRIVATE_DATA->reversed) {
		result = ioptron_command(device, false, ":FR#") && ioptron_status(device) && PRIVATE_DATA->reversed == requested;
	}
	indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reversed ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
	if (!result) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		if (!PRIVATE_DATA->active && (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
			ioptron_motion_state(device, INDIGO_ALERT_STATE);
		}
		if (IS_CONNECTED && ioptron_command(device, false, ":FQ#") && ioptron_status(device) && !PRIVATE_DATA->moving) {
			indigo_cancel_pending_handler(device, motion_finalizer);
			PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
			ioptron_publish(device);
			ioptron_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_x_focuser_zero_sync_handler(indigo_device *device) {
	X_FOCUSER_ZERO_SYNC_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_ZERO_SYNC.on_change
	if (X_FOCUSER_ZERO_SYNC_ITEM->sw.value) {
		if (IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->moving && !PRIVATE_DATA->uncertain) {
			if (ioptron_command(device, false, ":FZ#") && ioptron_status(device)) {
				ioptron_publish(device);
				if (PRIVATE_DATA->moving || PRIVATE_DATA->position != 0) {
					X_FOCUSER_ZERO_SYNC_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					ioptron_motion_state(device, INDIGO_OK_STATE);
				}
			} else {
				PRIVATE_DATA->uncertain = true;
				X_FOCUSER_ZERO_SYNC_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			X_FOCUSER_ZERO_SYNC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	X_FOCUSER_ZERO_SYNC_ITEM->sw.value = false;
	//- focuser.X_FOCUSER_ZERO_SYNC.on_change
	indigo_update_property(device, X_FOCUSER_ZERO_SYNC_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 99999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 99999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_ZERO_SYNC_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_ZERO_SYNC_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Sync position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FOCUSER_ZERO_SYNC_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_ZERO_SYNC_ITEM, X_FOCUSER_ZERO_SYNC_ITEM_NAME, "Sync to 0", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_ZERO_SYNC_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || X_FOCUSER_ZERO_SYNC_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_FOCUSER_ZERO_SYNC_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_ZERO_SYNC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_ZERO_SYNC_PROPERTY, focuser_x_focuser_zero_sync_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_ZERO_SYNC_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_ioptron(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static ioptron_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (ioptron_private_data *)indigo_safe_malloc(sizeof(ioptron_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
