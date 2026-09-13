// Copyright (c) 2020-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_robofocus.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdint.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_robofocus.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000002
#define DRIVER_NAME          "indigo_focuser_robofocus"
#define DRIVER_LABEL         "RoboFocus Focuser"
#define FOCUSER_DEVICE_NAME  "RoboFocus"
#define PRIVATE_DATA         ((robofocus_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define ROBOFOCUS_MIN_POSITION 1
#define ROBOFOCUS_MAX_POSITION 65535

//- define

#pragma mark - Property definitions

#define X_FOCUSER_POWER_CHANNELS_PROPERTY      (PRIVATE_DATA->x_focuser_power_channels_property)
#define X_FOCUSER_POWER_CHANNEL_1_ITEM         (X_FOCUSER_POWER_CHANNELS_PROPERTY->items + 0)
#define X_FOCUSER_POWER_CHANNEL_2_ITEM         (X_FOCUSER_POWER_CHANNELS_PROPERTY->items + 1)
#define X_FOCUSER_POWER_CHANNEL_3_ITEM         (X_FOCUSER_POWER_CHANNELS_PROPERTY->items + 2)
#define X_FOCUSER_POWER_CHANNEL_4_ITEM         (X_FOCUSER_POWER_CHANNELS_PROPERTY->items + 3)

#define X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME "X_FOCUSER_POWER_CHANNELS"
#define X_FOCUSER_POWER_CHANNEL_1_ITEM_NAME    "1"
#define X_FOCUSER_POWER_CHANNEL_2_ITEM_NAME    "2"
#define X_FOCUSER_POWER_CHANNEL_3_ITEM_NAME    "3"
#define X_FOCUSER_POWER_CHANNEL_4_ITEM_NAME    "4"

#define X_FOCUSER_CONFIG_PROPERTY             (PRIVATE_DATA->x_focuser_config_property)
#define X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM      (X_FOCUSER_CONFIG_PROPERTY->items + 0)
#define X_FOCUSER_CONFIG_STEP_DELAY_ITEM      (X_FOCUSER_CONFIG_PROPERTY->items + 1)
#define X_FOCUSER_CONFIG_STEP_SIZE_ITEM       (X_FOCUSER_CONFIG_PROPERTY->items + 2)
#define X_FOCUSER_CONFIG_BACKLASH_ITEM        (X_FOCUSER_CONFIG_PROPERTY->items + 3)

#define X_FOCUSER_CONFIG_PROPERTY_NAME        "X_FOCUSER_CONFIG"
#define X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM_NAME "DUTY_CYCLE"
#define X_FOCUSER_CONFIG_STEP_DELAY_ITEM_NAME "STEP_DELAY"
#define X_FOCUSER_CONFIG_STEP_SIZE_ITEM_NAME  "STEP_SIZE"
#define X_FOCUSER_CONFIG_BACKLASH_ITEM_NAME   "BACKLASH"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_power_channels_property;
	indigo_property *x_focuser_config_property;
	//+ data
	uint8_t response[9];
	int position, target, last_position, stalled, maximum;
	bool active, uncertain;
	//- data
} robofocus_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static uint8_t robofocus_checksum(const uint8_t *frame) {
	unsigned sum = 0;
	for (int i = 0; i < 8; i++) {
		sum += frame[i];
	}
	return (uint8_t)sum;
}

static bool robofocus_write(indigo_device *device, const uint8_t payload[8], bool discard) {
	uint8_t frame[9];
	memcpy(frame, payload, 8);
	frame[8] = robofocus_checksum(frame);
	return (!discard || indigo_uni_discard(PRIVATE_DATA->handle) >= 0) && indigo_uni_write(PRIVATE_DATA->handle, (const char *)frame, sizeof(frame)) == sizeof(frame);
}

static bool robofocus_read_byte(indigo_device *device, uint8_t *value, double timeout) {
	return indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(timeout)) > 0 && indigo_uni_read_available(PRIVATE_DATA->handle, value, 1) == 1;
}

static bool robofocus_read_frame(indigo_device *device, double timeout, int *inward, int *outward) {
	uint8_t value;
	double next_timeout = timeout;
	for (int count = 0; count < ROBOFOCUS_MAX_POSITION + 16; count++) {
		if (!robofocus_read_byte(device, &value, next_timeout)) {
			return false;
		}
		next_timeout = 0.2;
		if (value == 'I') {
			if (inward) {
				(*inward)++;
			}
			continue;
		}
		if (value == 'O') {
			if (outward) {
				(*outward)++;
			}
			continue;
		}
		if (value != 'F') {
			return false;
		}
		RESPONSE[0] = value;
		for (int i = 1; i < 9; i++) {
			if (!robofocus_read_byte(device, RESPONSE + i, 0.2)) {
				return false;
			}
		}
		return RESPONSE[8] == robofocus_checksum(RESPONSE);
	}
	return false;
}

static int robofocus_read_motion(indigo_device *device, int *direction) {
	uint8_t value;
	if (!robofocus_read_byte(device, &value, 0.05)) {
		return 0;
	}
	if (value == 'I' || value == 'O') {
		*direction = value == 'I' ? -1 : 1;
		return 1;
	}
	if (value != 'F') {
		return -1;
	}
	RESPONSE[0] = value;
	for (int i = 1; i < 9; i++) {
		if (!robofocus_read_byte(device, RESPONSE + i, 0.2)) {
			return -1;
		}
	}
	return RESPONSE[8] == robofocus_checksum(RESPONSE) ? 2 : -1;
}

static bool robofocus_command(indigo_device *device, const uint8_t payload[8]) {
	if (!robofocus_write(device, payload, true) || !robofocus_read_frame(device, 1, NULL, NULL)) {
		return false;
	}
	if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.01)) > 0) {
		indigo_uni_discard(PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static bool robofocus_ascii_value(indigo_device *device, int offset, int count, int minimum, int maximum, int *value) {
	int result = 0;
	for (int i = 0; i < count; i++) {
		uint8_t c = RESPONSE[offset + i];
		if (c < '0' || c > '9') {
			return false;
		}
		result = result * 10 + c - '0';
	}
	if (result < minimum || result > maximum) {
		return false;
	}
	*value = result;
	return true;
}

static void robofocus_format(uint8_t payload[8], char command, int value) {
	payload[0] = 'F';
	payload[1] = (uint8_t)command;
	for (int i = 7; i >= 2; i--) {
		payload[i] = (uint8_t)('0' + value % 10);
		value /= 10;
	}
}

static bool robofocus_value(indigo_device *device, char command, char response_command, int sent, int minimum, int maximum, int *value) {
	uint8_t payload[8];
	robofocus_format(payload, command, sent);
	return robofocus_command(device, payload) && RESPONSE[0] == 'F' && RESPONSE[1] == response_command && robofocus_ascii_value(device, 2, 6, minimum, maximum, value);
}

static bool robofocus_position(indigo_device *device, bool update_target) {
	int position;
	if (!robofocus_value(device, 'G', 'D', 0, ROBOFOCUS_MIN_POSITION, ROBOFOCUS_MAX_POSITION, &position)) {
		return false;
	}
	PRIVATE_DATA->position = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	if (update_target) {
		FOCUSER_POSITION_ITEM->number.target = position;
	}
	return true;
}

static bool robofocus_maximum(indigo_device *device, int requested) {
	int maximum;
	if (!robofocus_value(device, 'L', 'L', requested, ROBOFOCUS_MIN_POSITION, ROBOFOCUS_MAX_POSITION, &maximum)) {
		return false;
	}
	PRIVATE_DATA->maximum = maximum;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = maximum;
	return true;
}

static bool robofocus_temperature(indigo_device *device) {
	int raw;
	if (!robofocus_value(device, 'T', 'T', 0, 0, 1200, &raw)) {
		return false;
	}
	FOCUSER_TEMPERATURE_ITEM->number.value = raw / 2.0 - 273.15;
	return FOCUSER_TEMPERATURE_ITEM->number.value >= FOCUSER_TEMPERATURE_ITEM->number.min && FOCUSER_TEMPERATURE_ITEM->number.value <= FOCUSER_TEMPERATURE_ITEM->number.max;
}

static bool robofocus_power(indigo_device *device, bool write) {
	uint8_t payload[8] = { 'F', 'P', '0', '0', '0', '0', '0', '0' };
	if (write) {
		for (int i = 0; i < 4; i++) {
			payload[i + 4] = X_FOCUSER_POWER_CHANNELS_PROPERTY->items[i].sw.value ? '2' : '1';
		}
	}
	if (!robofocus_command(device, payload) || RESPONSE[0] != 'F' || RESPONSE[1] != 'P' || RESPONSE[2] != '0' || RESPONSE[3] != '0') {
		return false;
	}
	for (int i = 0; i < 4; i++) {
		if (RESPONSE[i + 4] != '1' && RESPONSE[i + 4] != '2') {
			return false;
		}
		X_FOCUSER_POWER_CHANNELS_PROPERTY->items[i].sw.value = RESPONSE[i + 4] == '2';
	}
	return true;
}

static bool robofocus_config(indigo_device *device, bool write) {
	uint8_t payload[8] = { 'F', 'C', 0, 0, 0, 0, 0, 0 };
	if (write) {
		payload[5] = (uint8_t)X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.target;
		payload[6] = (uint8_t)X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.target;
		payload[7] = (uint8_t)X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.target;
	}
	if (!robofocus_command(device, payload) || RESPONSE[0] != 'F' || RESPONSE[1] != 'C' || RESPONSE[2] != 0 || RESPONSE[3] != 0 || RESPONSE[4] != 0 || RESPONSE[5] > 250 || RESPONSE[6] < 1 || RESPONSE[6] > 64 || RESPONSE[7] < 1 || RESPONSE[7] > 64) {
		return false;
	}
	X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.value = X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.target = RESPONSE[5];
	X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.value = X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.target = RESPONSE[6];
	X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.value = X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.target = RESPONSE[7];
	return true;
}

static bool robofocus_backlash(indigo_device *device, bool write) {
	uint8_t payload[8] = { 'F', 'B', '0', '0', '0', '0', '0', '0' };
	if (write) {
		int requested = (int)X_FOCUSER_CONFIG_BACKLASH_ITEM->number.target;
		payload[2] = requested < 0 ? '3' : requested > 0 ? '2' : '1';
		int magnitude = requested < 0 ? -requested : requested;
		for (int i = 7; i >= 3; i--) {
			payload[i] = (uint8_t)('0' + magnitude % 10);
			magnitude /= 10;
		}
	}
	if (!robofocus_command(device, payload) || RESPONSE[0] != 'F' || RESPONSE[1] != 'B' || (RESPONSE[2] != '1' && RESPONSE[2] != '2' && RESPONSE[2] != '3')) {
		return false;
	}
	int magnitude;
	if (!robofocus_ascii_value(device, 3, 5, 0, 255, &magnitude) || (RESPONSE[2] == '1' && magnitude != 0)) {
		return false;
	}
	int backlash = RESPONSE[2] == '3' ? -magnitude : RESPONSE[2] == '2' ? magnitude : 0;
	X_FOCUSER_CONFIG_BACKLASH_ITEM->number.value = X_FOCUSER_CONFIG_BACKLASH_ITEM->number.target = backlash;
	return true;
}

static bool robofocus_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG | BINARY_LOG);
	int version;
	if (PRIVATE_DATA->handle && robofocus_value(device, 'V', 'V', 0, 0, 999999, &version)) {
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%06d", version);
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void robofocus_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void robofocus_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, 0) <= 0) {
		if (++PRIVATE_DATA->stalled >= 30) {
			PRIVATE_DATA->active = false;
			PRIVATE_DATA->uncertain = true;
			indigo_uni_write(PRIVATE_DATA->handle, "\r", 1);
			robofocus_motion_state(device, INDIGO_ALERT_STATE);
		} else {
			indigo_execute_handler_in(device, 0.1, motion_finalizer);
		}
		return;
	}
	int state = 0;
	for (int i = 0; i < 4096 && indigo_uni_wait_for_data(PRIVATE_DATA->handle, 0) > 0; i++) {
		int direction = 0;
		state = robofocus_read_motion(device, &direction);
		if (state < 0) {
			PRIVATE_DATA->active = false;
			PRIVATE_DATA->uncertain = true;
			indigo_uni_write(PRIVATE_DATA->handle, "\r", 1);
			robofocus_motion_state(device, INDIGO_ALERT_STATE);
			return;
		}
		if (state == 1) {
			PRIVATE_DATA->position += direction;
		} else if (state == 2) {
			break;
		}
	}
	if (PRIVATE_DATA->position < ROBOFOCUS_MIN_POSITION) {
		PRIVATE_DATA->position = ROBOFOCUS_MIN_POSITION;
	} else if (PRIVATE_DATA->position > PRIVATE_DATA->maximum) {
		PRIVATE_DATA->position = PRIVATE_DATA->maximum;
	}
	if (state == 2) {
		int position;
		if (RESPONSE[1] != 'D' || !robofocus_ascii_value(device, 2, 6, ROBOFOCUS_MIN_POSITION, ROBOFOCUS_MAX_POSITION, &position)) {
			PRIVATE_DATA->active = false;
			PRIVATE_DATA->uncertain = true;
			robofocus_motion_state(device, INDIGO_ALERT_STATE);
			return;
		}
		PRIVATE_DATA->position = position;
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_POSITION_ITEM->number.value = position;
		FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->target;
		robofocus_motion_state(device, position == PRIVATE_DATA->target ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
		return;
	}
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->position;
	PRIVATE_DATA->stalled = 0;
	robofocus_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, 0.01, motion_finalizer);
}

static void robofocus_start_motion(indigo_device *device, int target) {
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		robofocus_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	target = target < (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value ? (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value : target > PRIVATE_DATA->maximum ? PRIVATE_DATA->maximum : target;
	FOCUSER_POSITION_ITEM->number.target = target;
	PRIVATE_DATA->target = target;
	if (target == PRIVATE_DATA->position) {
		robofocus_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	uint8_t payload[8];
	robofocus_format(payload, 'G', target);
	if (!robofocus_write(device, payload, true)) {
		PRIVATE_DATA->uncertain = true;
		robofocus_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->active = true;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	PRIVATE_DATA->stalled = 0;
	robofocus_motion_state(device, INDIGO_BUSY_STATE);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		FOCUSER_POSITION_PROPERTY->state = robofocus_position(device, true) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		FOCUSER_TEMPERATURE_PROPERTY->state = robofocus_temperature(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = robofocus_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			connection_result = robofocus_position(device, true) && robofocus_maximum(device, 0) && robofocus_power(device, false) && robofocus_config(device, false) && robofocus_backlash(device, false) && robofocus_temperature(device);
			if (connection_result) {
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
				FOCUSER_POSITION_PROPERTY->state = FOCUSER_LIMITS_PROPERTY->state = FOCUSER_TEMPERATURE_PROPERTY->state = X_FOCUSER_POWER_CHANNELS_PROPERTY->state = X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				robofocus_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
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
			indigo_uni_write(PRIVATE_DATA->handle, "\r", 1);
			indigo_uni_discard(PRIVATE_DATA->handle);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
		robofocus_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int requested = (int)FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		int actual;
		if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !robofocus_value(device, 'S', 'D', requested, ROBOFOCUS_MIN_POSITION, ROBOFOCUS_MAX_POSITION, &actual) || actual != requested) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->position = actual;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = actual;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else {
		robofocus_start_motion(device, requested);
		if (PRIVATE_DATA->active) {
			indigo_execute_handler_in(device, 0.01, motion_finalizer);
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int delta = (int)FOCUSER_STEPS_ITEM->number.value;
	bool inward = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value != FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	robofocus_start_motion(device, PRIVATE_DATA->position + (inward ? -delta : delta));
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.01, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		bool pending = FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE;
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		if (PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
			int inward = 0, outward = 0, position = 0;
			bool stopped = indigo_uni_write(PRIVATE_DATA->handle, "\r", 1) == 1 && robofocus_read_frame(device, 1, &inward, &outward) && RESPONSE[1] == 'D' && robofocus_ascii_value(device, 2, 6, ROBOFOCUS_MIN_POSITION, ROBOFOCUS_MAX_POSITION, &position);
			if (!stopped) {
				indigo_uni_discard(PRIVATE_DATA->handle);
				stopped = robofocus_position(device, true);
				position = PRIVATE_DATA->position;
			}
			PRIVATE_DATA->active = false;
			PRIVATE_DATA->uncertain = !stopped;
			if (stopped) {
				PRIVATE_DATA->position = position;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				robofocus_motion_state(device, INDIGO_OK_STATE);
			} else {
				robofocus_motion_state(device, INDIGO_ALERT_STATE);
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (pending) {
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			robofocus_motion_state(device, INDIGO_OK_STATE);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int minimum = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
	int maximum = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || minimum > maximum || !robofocus_maximum(device, maximum)) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = minimum;
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_x_focuser_power_channels_handler(indigo_device *device) {
	X_FOCUSER_POWER_CHANNELS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_POWER_CHANNELS.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !robofocus_power(device, true)) {
		X_FOCUSER_POWER_CHANNELS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_POWER_CHANNELS.on_change
	indigo_update_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
}

static void focuser_x_focuser_config_handler(indigo_device *device) {
	X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_CONFIG.on_change
	bool result = IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->uncertain && robofocus_config(device, true);
	result = robofocus_backlash(device, true) && result;
	if (!result) {
		robofocus_config(device, false);
		robofocus_backlash(device, false);
		X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_CONFIG.on_change
	indigo_update_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "RoboFocus Focuser");
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = ROBOFOCUS_MIN_POSITION;
		FOCUSER_POSITION_ITEM->number.max = ROBOFOCUS_MAX_POSITION;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = ROBOFOCUS_MAX_POSITION;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = ROBOFOCUS_MIN_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = ROBOFOCUS_MAX_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = ROBOFOCUS_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = ROBOFOCUS_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = ROBOFOCUS_MAX_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = ROBOFOCUS_MAX_POSITION;
		//- focuser.FOCUSER_LIMITS.on_attach
		X_FOCUSER_POWER_CHANNELS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Power channels", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (X_FOCUSER_POWER_CHANNELS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_1_ITEM, X_FOCUSER_POWER_CHANNEL_1_ITEM_NAME, "Channel #1", false);
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_2_ITEM, X_FOCUSER_POWER_CHANNEL_2_ITEM_NAME, "Channel #2", false);
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_3_ITEM, X_FOCUSER_POWER_CHANNEL_3_ITEM_NAME, "Channel #3", false);
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_4_ITEM, X_FOCUSER_POWER_CHANNEL_4_ITEM_NAME, "Channel #4", false);
		X_FOCUSER_CONFIG_PROPERTY = indigo_init_number_property(NULL, device->name, X_FOCUSER_CONFIG_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (X_FOCUSER_CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM, X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM_NAME, "Duty cycle", 0, 250, 1, 0);
		indigo_init_number_item(X_FOCUSER_CONFIG_STEP_DELAY_ITEM, X_FOCUSER_CONFIG_STEP_DELAY_ITEM_NAME, "Step delay", 1, 64, 1, 1);
		indigo_init_number_item(X_FOCUSER_CONFIG_STEP_SIZE_ITEM, X_FOCUSER_CONFIG_STEP_SIZE_ITEM_NAME, "Step size", 1, 64, 1, 1);
		indigo_init_number_item(X_FOCUSER_CONFIG_BACKLASH_ITEM, X_FOCUSER_CONFIG_BACKLASH_ITEM_NAME, "Backlash", -255, 255, 1, 20);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_POWER_CHANNELS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_CONFIG_PROPERTY);
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
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ON_POSITION_SET_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ON_POSITION_SET_PROPERTY, property, false);
		FOCUSER_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
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
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_POWER_CHANNELS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_POWER_CHANNELS_PROPERTY, focuser_x_focuser_power_channels_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_CONFIG_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_CONFIG_PROPERTY, focuser_x_focuser_config_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_POWER_CHANNELS_PROPERTY);
	indigo_release_property(X_FOCUSER_CONFIG_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_robofocus(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static robofocus_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (robofocus_private_data *)indigo_safe_malloc(sizeof(robofocus_private_data));
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
