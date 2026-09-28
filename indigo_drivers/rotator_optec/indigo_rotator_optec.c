// Copyright (c) 2022-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_rotator_optec.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_rotator_optec.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_rotator_optec"
#define DRIVER_LABEL         "Optec Pyxis Rotator"
#define ROTATOR_DEVICE_NAME  "Optec Pyxis"
#define PRIVATE_DATA         ((optec_private_data *)device->private_data)

//+ define

#define OPTEC_RESPONSE_SIZE  64
#define OPTEC_MOTION_POLL    0.05
#define OPTEC_MOTION_MARGIN  5.0
#define OPTEC_NO_MOTION      0
#define OPTEC_POSITION_MOTION 1
#define OPTEC_HOME_MOTION    2

//- define

#pragma mark - Property definitions

#define X_HOME_PROPERTY                (PRIVATE_DATA->x_home_property)
#define X_HOME_ITEM                    (X_HOME_PROPERTY->items + 0)

#define X_HOME_PROPERTY_NAME           "X_HOME"
#define X_HOME_ITEM_NAME               "HOME"

#define X_RATE_PROPERTY                (PRIVATE_DATA->x_rate_property)
#define X_RATE_ITEM                    (X_RATE_PROPERTY->items + 0)

#define X_RATE_PROPERTY_NAME           "X_RATE"
#define X_RATE_ITEM_NAME               "RATE"

#define X_ROTATE_PROPERTY              (PRIVATE_DATA->x_rotate_property)
#define X_ROTATE_ITEM                  (X_ROTATE_PROPERTY->items + 0)

#define X_ROTATE_PROPERTY_NAME         "X_ROTATE"
#define X_ROTATE_ITEM_NAME             "ROTATE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_home_property;
	indigo_property *x_rate_property;
	indigo_property *x_rotate_property;
	//+ data
	char response[OPTEC_RESPONSE_SIZE];
	int motion;
	int motion_steps;
	double motion_deadline;
	bool position_invalidated;
	bool previous_direction_normal;
	//- data
} optec_private_data;

#pragma mark - Low level code

//+ code

static bool optec_write(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	long result = indigo_uni_vprintf(PRIVATE_DATA->handle, format, args);
	va_end(args);
	return result > 0;
}

static bool optec_read_line(indigo_device *device) {
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count <= 1 || count >= (long)sizeof(PRIVATE_DATA->response) - 1 || PRIVATE_DATA->response[count - 1] != '\n') {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	return true;
}

static bool optec_line_command(indigo_device *device, const char *command) {
	return indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && optec_write(device, "%s", command) && optec_read_line(device);
}

static bool optec_ack_command(indigo_device *device, const char *format, ...) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0) {
		return false;
	}
	va_list args;
	va_start(args, format);
	long result = indigo_uni_vprintf(PRIVATE_DATA->handle, format, args);
	va_end(args);
	if (result <= 0 || indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(1)) <= 0) {
		return false;
	}
	return indigo_uni_read(PRIVATE_DATA->handle, PRIVATE_DATA->response, 1) == 1 && PRIVATE_DATA->response[0] == '!';
}

static bool optec_wakeup(indigo_device *device) {
	if (!optec_line_command(device, "CWAKUP") || strcmp(PRIVATE_DATA->response, "!")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to wake up");
		return false;
	}
	return true;
}

static bool optec_sleep(indigo_device *device) {
	return optec_write(device, "CSLEEP");
}

static bool optec_read_integer(indigo_device *device, const char *command, int minimum, int maximum, int *value) {
	if (!optec_line_command(device, command)) {
		return false;
	}
	char *end = NULL;
	long parsed = strtol(PRIVATE_DATA->response, &end, 10);
	if (end == PRIVATE_DATA->response || *end != 0 || parsed < minimum || parsed > maximum) {
		return false;
	}
	*value = (int)parsed;
	return true;
}

static bool optec_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL && optec_wakeup(device) && optec_line_command(device, "CCLINK") && !strcmp(PRIVATE_DATA->response, "!")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void optec_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

//- code

//+ rotator.code

static void motion_finalizer(indigo_device *device) {
	bool complete = false;
	bool failed = false;
	for (int pass = 0; pass < 4 && indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.001)) > 0; pass++) {
		long count = indigo_uni_read_available(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1);
		if (count <= 0) {
			failed = true;
			break;
		}
		for (long i = 0; i < count; i++) {
			if (PRIVATE_DATA->response[i] == '!') {
				PRIVATE_DATA->motion_steps++;
			} else if (PRIVATE_DATA->response[i] == 'F') {
				complete = true;
			} else {
				failed = true;
			}
		}
	}
	if (!complete && !failed && indigo_monotonic_time() < PRIVATE_DATA->motion_deadline) {
		indigo_execute_handler_in(device, OPTEC_MOTION_POLL, motion_finalizer);
		return;
	}
	int motion = PRIVATE_DATA->motion;
	PRIVATE_DATA->motion = OPTEC_NO_MOTION;
	if (complete && !failed) {
		int position;
		if (optec_read_integer(device, "CGETPA", 0, 359, &position)) {
			ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = position;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->position_invalidated = false;
		} else {
			failed = true;
		}
	}
	if (failed || !complete) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator motion failed after %d steps", PRIVATE_DATA->motion_steps);
	}
	if (!optec_sleep(device)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		failed = true;
	}
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, failed || !complete ? "Rotator motion failed" : NULL);
	if (motion == OPTEC_HOME_MOTION) {
		X_HOME_PROPERTY->state = failed || !complete ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, X_HOME_PROPERTY, failed || !complete ? "Homing failed" : NULL);
	}
}

static bool optec_start_motion(indigo_device *device, int motion, const char *command) {
	if (!optec_wakeup(device) || indigo_uni_discard(PRIVATE_DATA->handle) < 0 || !optec_write(device, "%s", command)) {
		optec_sleep(device);
		return false;
	}
	PRIVATE_DATA->motion = motion;
	PRIVATE_DATA->motion_steps = 0;
	PRIVATE_DATA->motion_deadline = indigo_monotonic_time() + OPTEC_MOTION_MARGIN + 0.36 * (X_RATE_ITEM->number.value > 0 ? X_RATE_ITEM->number.value : 1);
	indigo_execute_handler_in(device, OPTEC_MOTION_POLL, motion_finalizer);
	return true;
}

//- rotator.code

#pragma mark - High level code (rotator)

static void rotator_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = optec_open(device);
		if (connection_result) {
			//+ rotator.on_connect
			int direction = 0;
			int position = 0;
			connection_result = optec_read_integer(device, "CMREAD", 0, 1, &direction) && optec_read_integer(device, "CGETPA", 0, 359, &position) && optec_ack_command(device, "CTxx%02d", (int)X_RATE_ITEM->number.target) && optec_sleep(device);
			if (connection_result) {
				indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, direction == 0 ? ROTATOR_DIRECTION_NORMAL_ITEM : ROTATOR_DIRECTION_REVERSED_ITEM, true);
				ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = position;
				ROTATOR_POSITION_PROPERTY->state = PRIVATE_DATA->position_invalidated ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
				X_RATE_ITEM->number.value = X_RATE_ITEM->number.target;
				X_RATE_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->motion = OPTEC_NO_MOTION;
			}
			if (!connection_result) {
				optec_close(device);
			}
			//- rotator.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_HOME_PROPERTY, NULL);
			indigo_define_property(device, X_RATE_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator.on_disconnect
		if (PRIVATE_DATA->motion != OPTEC_NO_MOTION) {
			PRIVATE_DATA->motion = OPTEC_NO_MOTION;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			X_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		//- rotator.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			X_HOME_PROPERTY,
			X_RATE_PROPERTY,
			X_ROTATE_PROPERTY,
			ROTATOR_ON_POSITION_SET_PROPERTY,
			ROTATOR_ABORT_MOTION_PROPERTY,
			ROTATOR_DIRECTION_PROPERTY,
			ROTATOR_POSITION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_HOME_PROPERTY, NULL);
		indigo_delete_property(device, X_RATE_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATE_PROPERTY, NULL);
		optec_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_x_home_handler(indigo_device *device) {
	//+ rotator.X_HOME.on_change
	bool requested = X_HOME_ITEM->sw.value;
	X_HOME_ITEM->sw.value = false;
	if (!requested) {
		X_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_HOME_PROPERTY, NULL);
	} else if (optec_start_motion(device, OPTEC_HOME_MOTION, "CHOMES")) { // motion_finalizer owns completion
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		X_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_update_property(device, X_HOME_PROPERTY, NULL);
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		X_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Failed to start homing");
		indigo_update_property(device, X_HOME_PROPERTY, "Failed to start homing");
	}
	//- rotator.X_HOME.on_change
}

static void rotator_x_rate_handler(indigo_device *device) {
	X_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.X_RATE.on_change
	if (optec_wakeup(device) && optec_ack_command(device, "CTxx%02d", (int)X_RATE_ITEM->number.target) && optec_sleep(device)) {
		X_RATE_ITEM->number.value = X_RATE_ITEM->number.target;
	} else {
		optec_sleep(device);
		X_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.X_RATE.on_change
	indigo_update_property(device, X_RATE_PROPERTY, NULL);
}

static void rotator_x_rotate_handler(indigo_device *device) {
	X_ROTATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.X_ROTATE.on_change
	int steps = (int)X_ROTATE_ITEM->number.target;
	if (steps == 0) {
		X_ROTATE_ITEM->number.value = X_ROTATE_ITEM->number.target = 0;
	} else {
		int encoded = steps > 0 ? steps : 10 - steps;
		if (optec_wakeup(device) && optec_ack_command(device, "CXxx%02d", encoded) && optec_sleep(device)) {
			X_ROTATE_ITEM->number.value = X_ROTATE_ITEM->number.target = 0;
			PRIVATE_DATA->position_invalidated = true;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Absolute position is unknown; home the rotator before an absolute move");
		} else {
			optec_sleep(device);
			X_ROTATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- rotator.X_ROTATE.on_change
	indigo_update_property(device, X_ROTATE_PROPERTY, NULL);
}

static void rotator_direction_handler(indigo_device *device) {
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_DIRECTION.on_change
	if (!optec_wakeup(device) || !optec_write(device, "CD%dxxx", ROTATOR_DIRECTION_NORMAL_ITEM->sw.value ? 0 : 1) || !optec_sleep(device)) {
		optec_sleep(device);
		indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, PRIVATE_DATA->previous_direction_normal ? ROTATOR_DIRECTION_NORMAL_ITEM : ROTATOR_DIRECTION_REVERSED_ITEM, true);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
}

static void rotator_position_handler(indigo_device *device) {
	//+ rotator.ROTATOR_POSITION.on_change
	int target = (int)ROTATOR_POSITION_ITEM->number.target;
	if (target == (int)ROTATOR_POSITION_ITEM->number.value) {
		ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value;
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	} else {
		char command[7];
		snprintf(command, sizeof(command), "CPA%03d", target);
		if (optec_start_motion(device, OPTEC_POSITION_MOTION, command)) { // motion_finalizer owns completion
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Failed to start motion");
		}
	}
	//- rotator.ROTATOR_POSITION.on_change
}

#pragma mark - Device API (rotator)

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		X_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, X_HOME_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_HOME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_HOME_ITEM, X_HOME_ITEM_NAME, "Find home", false);
		X_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, X_RATE_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_RATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RATE_ITEM, X_RATE_ITEM_NAME, "Rotational rate", 0, 99, 1, 8);
		X_ROTATE_PROPERTY = indigo_init_number_property(NULL, device->name, X_ROTATE_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Rotate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_ROTATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_ROTATE_ITEM, X_ROTATE_ITEM_NAME, "Steps", -9, 9, 1, 0);
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = true;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = true;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_POSITION_PROPERTY->hidden = false;
		//+ rotator.ROTATOR_POSITION.on_attach
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.max = 359;
		ROTATOR_POSITION_ITEM->number.step = 1;
		//- rotator.ROTATOR_POSITION.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_HOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATE_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(rotator_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_HOME_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->motion != OPTEC_NO_MOTION || ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_HOME_PROPERTY->state == INDIGO_BUSY_STATE, X_HOME_PROPERTY, "Rotator operation is already in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_HOME_PROPERTY, rotator_x_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RATE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->motion != OPTEC_NO_MOTION || ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_HOME_PROPERTY->state == INDIGO_BUSY_STATE, X_RATE_PROPERTY, "Rotator operation is already in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_RATE_PROPERTY, rotator_x_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->motion != OPTEC_NO_MOTION || ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_HOME_PROPERTY->state == INDIGO_BUSY_STATE, X_ROTATE_PROPERTY, "Rotator operation is already in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_ROTATE_PROPERTY, rotator_x_rotate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->motion != OPTEC_NO_MOTION || ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_HOME_PROPERTY->state == INDIGO_BUSY_STATE, ROTATOR_DIRECTION_PROPERTY, "Rotator operation is already in progress");
		//+ rotator.ROTATOR_DIRECTION.on_change_request
		PRIVATE_DATA->previous_direction_normal = ROTATOR_DIRECTION_NORMAL_ITEM->sw.value;
		//- rotator.ROTATOR_DIRECTION.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->motion != OPTEC_NO_MOTION || ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_HOME_PROPERTY->state == INDIGO_BUSY_STATE, ROTATOR_POSITION_PROPERTY, "Rotator operation is already in progress");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->position_invalidated, ROTATOR_POSITION_PROPERTY, "Absolute position is unknown; home the rotator first");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_RATE_PROPERTY);
		}
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	indigo_release_property(X_HOME_PROPERTY);
	indigo_release_property(X_RATE_PROPERTY);
	indigo_release_property(X_ROTATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - Device templates

static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_DEVICE_NAME, rotator_attach, rotator_enumerate_properties, rotator_change_property, NULL, rotator_detach);

#pragma mark - Main code

indigo_result indigo_rotator_optec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static optec_private_data *private_data = NULL;
	static indigo_device *rotator = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (optec_private_data *)indigo_safe_malloc(sizeof(optec_private_data));
			rotator = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
			rotator->private_data = private_data;
			indigo_attach_device(rotator);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(rotator);
			last_action = action;
			if (rotator != NULL) {
				indigo_detach_device(rotator);
				indigo_safe_free(rotator);
				rotator = NULL;
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
