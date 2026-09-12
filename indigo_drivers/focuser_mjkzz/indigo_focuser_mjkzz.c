// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_mjkzz.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdint.h>
#include "mjkzz_def.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_mjkzz.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_focuser_mjkzz"
#define DRIVER_LABEL         "MJKZZ Rail Focuser"
#define FOCUSER_DEVICE_NAME  "MJKZZ Rail"
#define PRIVATE_DATA         ((mjkzz_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define MJKZZ_MIN_POSITION   (-32768)
#define MJKZZ_MAX_POSITION   32767

//- define

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	mjkzz_message response;
	int32_t position, expected_position, last_position;
	int stalled;
	bool active, uncertain;
	//- data
} mjkzz_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static int32_t mjkzz_get_int(const mjkzz_message *message) {
	uint32_t value = ((uint32_t)message->ucMSG[0] << 24) | ((uint32_t)message->ucMSG[1] << 16) | ((uint32_t)message->ucMSG[2] << 8) | message->ucMSG[3];
	return (int32_t)value;
}

static void mjkzz_set_int(mjkzz_message *message, int32_t value) {
	uint32_t encoded = (uint32_t)value;
	message->ucMSG[0] = (uint8_t)(encoded >> 24);
	message->ucMSG[1] = (uint8_t)(encoded >> 16);
	message->ucMSG[2] = (uint8_t)(encoded >> 8);
	message->ucMSG[3] = (uint8_t)encoded;
}

static uint8_t mjkzz_checksum(const mjkzz_message *message) {
	return (uint8_t)(message->ucADD + message->ucCMD + message->ucIDX + message->ucMSG[0] + message->ucMSG[1] + message->ucMSG[2] + message->ucMSG[3]);
}

static bool mjkzz_read(indigo_device *device) {
	uint8_t *buffer = (uint8_t *)&RESPONSE;
	long received = 0;
	while (received < (long)sizeof(RESPONSE)) {
		long timeout = received ? INDIGO_DELAY(0.2) : INDIGO_DELAY(1);
		if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, timeout) <= 0) {
			return false;
		}
		long count = indigo_uni_read_available(PRIVATE_DATA->handle, buffer + received, (long)sizeof(RESPONSE) - received);
		if (count <= 0) {
			return false;
		}
		received += count;
	}
	if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.01)) > 0) {
		indigo_uni_discard(PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static bool mjkzz_command(indigo_device *device, uint8_t command, uint8_t index, int32_t value, int32_t *result) {
	mjkzz_message request = { 0x01, command, index, { 0 }, 0 };
	mjkzz_set_int(&request, value);
	request.ucSUM = mjkzz_checksum(&request);
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_write(PRIVATE_DATA->handle, (const char *)&request, sizeof(request)) != sizeof(request) || !mjkzz_read(device)) {
		return false;
	}
	if (RESPONSE.ucADD != (uint8_t)(request.ucADD | 0x80) || RESPONSE.ucCMD != (uint8_t)(request.ucCMD | 0x80) || RESPONSE.ucIDX != request.ucIDX || RESPONSE.ucSUM != mjkzz_checksum(&RESPONSE)) {
		return false;
	}
	if (result) {
		*result = mjkzz_get_int(&RESPONSE);
	}
	return true;
}

static bool mjkzz_write_register(indigo_device *device, uint8_t index, int32_t value) {
	int32_t actual = 0;
	return mjkzz_command(device, CMD_SREG, index, value, &actual) && actual == value;
}

static bool mjkzz_position(indigo_device *device, int32_t *position) {
	int32_t actual = 0;
	if (!mjkzz_command(device, CMD_GPOS, 0, 0, &actual) || actual < MJKZZ_MIN_POSITION || actual > MJKZZ_MAX_POSITION) {
		return false;
	}
	PRIVATE_DATA->position = actual;
	FOCUSER_POSITION_ITEM->number.value = actual;
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain) {
		FOCUSER_POSITION_ITEM->number.target = actual;
	}
	if (position) {
		*position = actual;
	}
	return true;
}

static bool mjkzz_stop(indigo_device *device) {
	int32_t position = 0;
	if (!mjkzz_command(device, CMD_STOP, 0, 0, &position) || position < MJKZZ_MIN_POSITION || position > MJKZZ_MAX_POSITION) {
		return false;
	}
	PRIVATE_DATA->position = position;
	FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
	return true;
}

static bool mjkzz_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG | BINARY_LOG);
	int32_t version = 0;
	if (PRIVATE_DATA->handle && mjkzz_command(device, CMD_GVER, 0, 0, &version)) {
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%u.%u.%u.%u", RESPONSE.ucMSG[0], RESPONSE.ucMSG[1], RESPONSE.ucMSG[2], RESPONSE.ucMSG[3]);
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void mjkzz_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void mjkzz_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	int32_t position = 0;
	if (!mjkzz_position(device, &position)) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = !mjkzz_stop(device);
		mjkzz_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (position == PRIVATE_DATA->expected_position) {
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_POSITION_ITEM->number.target = position;
		mjkzz_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	if (position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = position;
		PRIVATE_DATA->stalled = 0;
	} else if (++PRIVATE_DATA->stalled >= 50) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = !mjkzz_stop(device);
		mjkzz_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	mjkzz_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, 0.1, motion_finalizer);
}

static void mjkzz_start_motion(indigo_device *device, int32_t target) {
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		mjkzz_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	target = target < MJKZZ_MIN_POSITION ? MJKZZ_MIN_POSITION : target > MJKZZ_MAX_POSITION ? MJKZZ_MAX_POSITION : target;
	FOCUSER_POSITION_ITEM->number.target = target;
	if (target == PRIVATE_DATA->position) {
		mjkzz_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	int32_t accepted = 0;
	if (!mjkzz_command(device, CMD_SPOS, 0, target, &accepted) || accepted != target) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = !mjkzz_stop(device);
		mjkzz_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->expected_position = target;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	PRIVATE_DATA->stalled = 0;
	PRIVATE_DATA->active = true;
	mjkzz_motion_state(device, INDIGO_BUSY_STATE);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		FOCUSER_POSITION_PROPERTY->state = mjkzz_position(device, NULL) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = mjkzz_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int32_t position = 0, speed = 0;
			connection_result = mjkzz_write_register(device, reg_HPWR, 12) && mjkzz_write_register(device, reg_LPWR, 2) && mjkzz_write_register(device, reg_MSTEP, MOTOR_4STEP) && mjkzz_position(device, &position) && mjkzz_command(device, CMD_GSPD, 0, 0, &speed) && speed >= 0 && speed <= 255;
			if (connection_result) {
				PRIVATE_DATA->position = position;
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				speed = speed > 3 ? 3 : speed;
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = speed;
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				mjkzz_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
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
		indigo_cancel_pending_handler(device, motion_finalizer);
		if (PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
			mjkzz_stop(device);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		mjkzz_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	int32_t requested = (int32_t)FOCUSER_SPEED_ITEM->number.target;
	int32_t actual = 0;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !mjkzz_command(device, CMD_SSPD, 0, requested, &actual) || actual != requested) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_SPEED_ITEM->number.value = actual;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int64_t delta = (int64_t)FOCUSER_STEPS_ITEM->number.target;
	int64_t target = PRIVATE_DATA->position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -delta : delta);
	target = target < MJKZZ_MIN_POSITION ? MJKZZ_MIN_POSITION : target > MJKZZ_MAX_POSITION ? MJKZZ_MAX_POSITION : target;
	mjkzz_start_motion(device, (int32_t)target);
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	mjkzz_start_motion(device, (int32_t)FOCUSER_POSITION_ITEM->number.target);
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		PRIVATE_DATA->active = false;
		if (IS_CONNECTED && mjkzz_stop(device)) {
			PRIVATE_DATA->uncertain = false;
			mjkzz_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			mjkzz_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "MJKZZ Serial Camera Motion Controller");
		//- focuser.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 3;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 1000;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = MJKZZ_MIN_POSITION;
		FOCUSER_POSITION_ITEM->number.max = MJKZZ_MAX_POSITION;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
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
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_mjkzz(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static mjkzz_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (mjkzz_private_data *)indigo_safe_malloc(sizeof(mjkzz_private_data));
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
