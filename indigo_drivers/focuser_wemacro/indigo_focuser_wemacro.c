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

// This file generated from indigo_focuser_wemacro.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_wemacro.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_focuser_wemacro"
#define DRIVER_LABEL         "WeMacro Rail Focuser"
#define FOCUSER_DEVICE_NAME  "WeMacro Rail"
#define PRIVATE_DATA         ((wemacro_private_data *)device->private_data)

//+ define

#define X_RAIL_BATCH         "Batch"
#define WEMACRO_FRAME_SIZE   12
#define WEMACRO_STATUS_SIZE  3
#define WEMACRO_STATUS_INITIAL 0xF0
#define WEMACRO_STATUS_FORWARD 0xF5
#define WEMACRO_STATUS_BACKWARD 0xF6
#define WEMACRO_STATUS_BATCH 0xF7

//- define

#pragma mark - Property definitions

#define X_RAIL_CONFIG_PROPERTY         (PRIVATE_DATA->x_rail_config_property)
#define X_RAIL_CONFIG_BACK_ITEM        (X_RAIL_CONFIG_PROPERTY->items + 0)
#define X_RAIL_CONFIG_BEEP_ITEM        (X_RAIL_CONFIG_PROPERTY->items + 1)

#define X_RAIL_CONFIG_PROPERTY_NAME    "X_RAIL_CONFIG"
#define X_RAIL_CONFIG_BACK_ITEM_NAME   "BACK"
#define X_RAIL_CONFIG_BEEP_ITEM_NAME   "BEEP"

#define X_RAIL_SHUTTER_PROPERTY        (PRIVATE_DATA->x_rail_shutter_property)
#define X_RAIL_SHUTTER_ITEM            (X_RAIL_SHUTTER_PROPERTY->items + 0)

#define X_RAIL_SHUTTER_PROPERTY_NAME   "X_RAIL_SHUTTER"
#define X_RAIL_SHUTTER_ITEM_NAME       "SHUTTER"

#define X_RAIL_EXECUTE_PROPERTY              (PRIVATE_DATA->x_rail_execute_property)
#define X_RAIL_EXECUTE_SETTLE_TIME_ITEM      (X_RAIL_EXECUTE_PROPERTY->items + 0)
#define X_RAIL_EXECUTE_PER_STEP_ITEM         (X_RAIL_EXECUTE_PROPERTY->items + 1)
#define X_RAIL_EXECUTE_INTERVAL_ITEM         (X_RAIL_EXECUTE_PROPERTY->items + 2)
#define X_RAIL_EXECUTE_LENGTH_ITEM           (X_RAIL_EXECUTE_PROPERTY->items + 3)
#define X_RAIL_EXECUTE_COUNT_ITEM            (X_RAIL_EXECUTE_PROPERTY->items + 4)

#define X_RAIL_EXECUTE_PROPERTY_NAME         "X_RAIL_EXECUTE"
#define X_RAIL_EXECUTE_SETTLE_TIME_ITEM_NAME "SETTLE_TIME"
#define X_RAIL_EXECUTE_PER_STEP_ITEM_NAME    "SHUTTER_PER_STEP"
#define X_RAIL_EXECUTE_INTERVAL_ITEM_NAME    "SHUTTER_INTERVAL"
#define X_RAIL_EXECUTE_LENGTH_ITEM_NAME      "LENGTH"
#define X_RAIL_EXECUTE_COUNT_ITEM_NAME       "COUNT"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_rail_config_property;
	indigo_property *x_rail_shutter_property;
	indigo_property *x_rail_execute_property;
	//+ data
	uint8_t command[WEMACRO_FRAME_SIZE];
	uint8_t response[WEMACRO_STATUS_SIZE + 1];
	uint8_t expected_status;
	uint32_t batch_remaining;
	uint8_t batch_command, batch_settle, batch_per_step, batch_interval;
	uint32_t batch_length, batch_count;
	double deadline;
	bool motion_active, batch_active, batch_start_pending, uncertain;
	//- data
} wemacro_private_data;

#pragma mark - Low level code

//+ code

static void focuser_steps_handler(indigo_device *device);
static void focuser_x_rail_execute_handler(indigo_device *device);

static uint16_t wemacro_crc(const uint8_t *buffer, size_t length) {
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < length; i++) {
		crc ^= buffer[i];
		for (int bit = 0; bit < 8; bit++) {
			crc = crc & 1 ? (crc >> 1) ^ 0xA001 : crc >> 1;
		}
	}
	return crc;
}

static bool wemacro_write(indigo_device *device, uint8_t command, uint8_t a, uint8_t b, uint8_t c, uint32_t value) {
	uint8_t *frame = PRIVATE_DATA->command;
	frame[0] = 0xA5;
	frame[1] = 0x5A;
	frame[2] = command;
	frame[3] = a;
	frame[4] = b;
	frame[5] = c;
	frame[6] = (uint8_t)(value >> 24);
	frame[7] = (uint8_t)(value >> 16);
	frame[8] = (uint8_t)(value >> 8);
	frame[9] = (uint8_t)value;
	uint16_t crc = wemacro_crc(frame, 10);
	frame[10] = (uint8_t)crc;
	frame[11] = (uint8_t)(crc >> 8);
	return indigo_uni_write(PRIVATE_DATA->handle, (const char *)frame, WEMACRO_FRAME_SIZE) == WEMACRO_FRAME_SIZE;
}

static int wemacro_read_status(indigo_device *device, double timeout, uint8_t *status) {
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, (char *)PRIVATE_DATA->response, WEMACRO_STATUS_SIZE, "", "", INDIGO_DELAY(timeout), INDIGO_DELAY(0.1));
	if (count == 0) {
		return 0;
	}
	if (count != WEMACRO_STATUS_SIZE || PRIVATE_DATA->response[0] != 0xA5 || PRIVATE_DATA->response[1] != 0x5A) {
		return -1;
	}
	*status = PRIVATE_DATA->response[2];
	return 1;
}

static uint8_t wemacro_options(indigo_device *device, bool include_flags) {
	uint8_t options = 0x80;
	if (include_flags && X_RAIL_CONFIG_BEEP_ITEM->sw.value) {
		options |= 0x02;
	}
	if (include_flags && X_RAIL_CONFIG_BACK_ITEM->sw.value) {
		options |= 0x08;
	}
	return options;
}

static uint8_t wemacro_speed(indigo_device *device) {
	return FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0;
}

static void wemacro_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void wemacro_batch_state(indigo_device *device, indigo_property_state state) {
	X_RAIL_EXECUTE_PROPERTY->state = state;
	indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
}

static bool wemacro_stop(indigo_device *device) {
	return wemacro_write(device, 0x20, 0, 0, 0, 0);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->motion_active) {
		return;
	}
	uint8_t status = 0;
	int result = wemacro_read_status(device, 0.1, &status);
	if (result > 0) {
		PRIVATE_DATA->motion_active = false;
		if (status == PRIVATE_DATA->expected_status) {
			PRIVATE_DATA->uncertain = false;
			wemacro_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			wemacro_stop(device);
			wemacro_motion_state(device, INDIGO_ALERT_STATE);
		}
	} else if (result == 0 && indigo_monotonic_time() < PRIVATE_DATA->deadline) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	} else {
		PRIVATE_DATA->motion_active = false;
		PRIVATE_DATA->uncertain = true;
		wemacro_stop(device);
		wemacro_motion_state(device, INDIGO_ALERT_STATE);
	}
}

static void batch_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->batch_active) {
		return;
	}
	uint8_t status = 0;
	int result = wemacro_read_status(device, 0.1, &status);
	if (result > 0) {
		if (status == WEMACRO_STATUS_BATCH && PRIVATE_DATA->batch_remaining > 0) {
			PRIVATE_DATA->batch_remaining--;
			X_RAIL_EXECUTE_COUNT_ITEM->number.value = PRIVATE_DATA->batch_remaining;
			wemacro_batch_state(device, INDIGO_BUSY_STATE);
			if (PRIVATE_DATA->batch_remaining == 0 && !X_RAIL_CONFIG_BACK_ITEM->sw.value) {
				PRIVATE_DATA->batch_active = PRIVATE_DATA->uncertain = false;
				wemacro_batch_state(device, INDIGO_OK_STATE);
			} else {
				indigo_execute_handler_in(device, 0.1, batch_finalizer);
			}
		} else if (status == WEMACRO_STATUS_BACKWARD && PRIVATE_DATA->batch_remaining == 0 && X_RAIL_CONFIG_BACK_ITEM->sw.value) {
			PRIVATE_DATA->batch_active = PRIVATE_DATA->uncertain = false;
			wemacro_batch_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->batch_active = false;
			PRIVATE_DATA->uncertain = true;
			wemacro_stop(device);
			wemacro_batch_state(device, INDIGO_ALERT_STATE);
		}
	} else if (result == 0 && indigo_monotonic_time() < PRIVATE_DATA->deadline) {
		indigo_execute_handler_in(device, 0.1, batch_finalizer);
	} else {
		PRIVATE_DATA->batch_active = false;
		PRIVATE_DATA->uncertain = true;
		wemacro_stop(device);
		wemacro_batch_state(device, INDIGO_ALERT_STATE);
	}
}

static void batch_start_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->batch_start_pending) {
		return;
	}
	PRIVATE_DATA->batch_start_pending = false;
	if (wemacro_write(device, PRIVATE_DATA->batch_command, PRIVATE_DATA->batch_settle, PRIVATE_DATA->batch_per_step, PRIVATE_DATA->batch_interval, PRIVATE_DATA->batch_count - 1)) {
		PRIVATE_DATA->batch_active = true;
		indigo_execute_handler_in(device, 0.1, batch_finalizer);
	} else {
		PRIVATE_DATA->batch_active = false;
		PRIVATE_DATA->uncertain = true;
		wemacro_batch_state(device, INDIGO_ALERT_STATE);
	}
}

static bool wemacro_initialise(indigo_device *device) {
	uint8_t status = 0;
	if (wemacro_read_status(device, 2, &status) > 0 && status == WEMACRO_STATUS_INITIAL) {
		return wemacro_write(device, wemacro_options(device, true), wemacro_speed(device), 0, 0, 0);
	}
	indigo_uni_discard(PRIVATE_DATA->handle);
	return wemacro_write(device, 0x40, 0, 0, 0, 0) && wemacro_write(device, 0x20, 0, 0, 0, 0) && wemacro_write(device, 0x40, 0, 0, 0, 0) && wemacro_read_status(device, 2, &status) > 0 && status == WEMACRO_STATUS_FORWARD && wemacro_write(device, wemacro_options(device, true), wemacro_speed(device), 0, 0, 0);
}

static bool wemacro_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG | BINARY_LOG);
	if (PRIVATE_DATA->handle && wemacro_initialise(device)) {
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void wemacro_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = wemacro_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			PRIVATE_DATA->motion_active = PRIVATE_DATA->batch_active = PRIVATE_DATA->batch_start_pending = PRIVATE_DATA->uncertain = false;
			indigo_update_property(device, INFO_PROPERTY, NULL);
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
			indigo_define_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
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
		indigo_cancel_pending_handler(device, batch_start_finalizer);
		indigo_cancel_pending_handler(device, batch_finalizer);
		if (PRIVATE_DATA->motion_active || PRIVATE_DATA->batch_active || PRIVATE_DATA->batch_start_pending || PRIVATE_DATA->uncertain) {
			wemacro_stop(device);
		}
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			wemacro_motion_state(device, INDIGO_ALERT_STATE);
		}
		if (X_RAIL_EXECUTE_PROPERTY->state == INDIGO_BUSY_STATE) {
			wemacro_batch_state(device, INDIGO_ALERT_STATE);
		}
		PRIVATE_DATA->motion_active = PRIVATE_DATA->batch_active = PRIVATE_DATA->batch_start_pending = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		indigo_delete_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
		indigo_delete_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
		wemacro_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	int requested = (int)FOCUSER_SPEED_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->motion_active || PRIVATE_DATA->batch_active || PRIVATE_DATA->batch_start_pending || PRIVATE_DATA->uncertain || !wemacro_write(device, wemacro_options(device, true), requested == 2 ? 0xFF : 0, 0, 0, 0)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_SPEED_ITEM->number.value = requested;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	uint32_t steps = (uint32_t)FOCUSER_STEPS_ITEM->number.target;
	bool inward = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value != FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	uint8_t command = inward ? 0x40 : 0x41;
	if (!IS_CONNECTED || PRIVATE_DATA->motion_active || PRIVATE_DATA->batch_active || PRIVATE_DATA->batch_start_pending || PRIVATE_DATA->uncertain) {
		wemacro_motion_state(device, INDIGO_ALERT_STATE);
	} else if (steps == 0) {
		wemacro_motion_state(device, INDIGO_OK_STATE);
	} else if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && wemacro_write(device, command, 0, 0, 0, steps)) {
		PRIVATE_DATA->motion_active = true;
		PRIVATE_DATA->expected_status = command == 0x40 ? WEMACRO_STATUS_FORWARD : WEMACRO_STATUS_BACKWARD;
		PRIVATE_DATA->deadline = indigo_monotonic_time() + 5 + steps / (FOCUSER_SPEED_ITEM->number.value == 2 ? 4000.0 : 2000.0);
		wemacro_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	} else {
		PRIVATE_DATA->uncertain = true;
		wemacro_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		bool pending = FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || X_RAIL_EXECUTE_PROPERTY->state == INDIGO_BUSY_STATE;
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, focuser_x_rail_execute_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		indigo_cancel_pending_handler(device, batch_start_finalizer);
		indigo_cancel_pending_handler(device, batch_finalizer);
		bool active = PRIVATE_DATA->motion_active || PRIVATE_DATA->batch_active || PRIVATE_DATA->batch_start_pending || PRIVATE_DATA->uncertain;
		PRIVATE_DATA->motion_active = PRIVATE_DATA->batch_active = PRIVATE_DATA->batch_start_pending = false;
		if (active && (!IS_CONNECTED || !wemacro_stop(device))) {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->uncertain = false;
		}
		if (pending) {
			if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
				wemacro_motion_state(device, INDIGO_ALERT_STATE);
			}
			if (X_RAIL_EXECUTE_PROPERTY->state == INDIGO_BUSY_STATE) {
				wemacro_batch_state(device, INDIGO_ALERT_STATE);
			}
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_x_rail_shutter_handler(indigo_device *device) {
	X_RAIL_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RAIL_SHUTTER.on_change
	bool requested = X_RAIL_SHUTTER_ITEM->sw.value;
	X_RAIL_SHUTTER_ITEM->sw.value = false;
	if (requested && (!IS_CONNECTED || !wemacro_write(device, 0x04, 0, 0, 0, 0))) {
		X_RAIL_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_RAIL_SHUTTER.on_change
	indigo_update_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
}

static void focuser_x_rail_execute_handler(indigo_device *device) {
	//+ focuser.X_RAIL_EXECUTE.on_change
	PRIVATE_DATA->batch_settle = (uint8_t)X_RAIL_EXECUTE_SETTLE_TIME_ITEM->number.target;
	PRIVATE_DATA->batch_per_step = (uint8_t)X_RAIL_EXECUTE_PER_STEP_ITEM->number.target;
	PRIVATE_DATA->batch_interval = (uint8_t)X_RAIL_EXECUTE_INTERVAL_ITEM->number.target;
	PRIVATE_DATA->batch_length = (uint32_t)X_RAIL_EXECUTE_LENGTH_ITEM->number.target;
	PRIVATE_DATA->batch_count = (uint32_t)X_RAIL_EXECUTE_COUNT_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->motion_active || PRIVATE_DATA->batch_active || PRIVATE_DATA->batch_start_pending || PRIVATE_DATA->uncertain || indigo_uni_discard(PRIVATE_DATA->handle) < 0 || !wemacro_write(device, wemacro_options(device, false), wemacro_speed(device), 0, 0, PRIVATE_DATA->batch_length)) {
		wemacro_batch_state(device, INDIGO_ALERT_STATE);
	} else {
		for (int i = 0; i < X_RAIL_EXECUTE_PROPERTY->count; i++) {
			X_RAIL_EXECUTE_PROPERTY->items[i].number.value = X_RAIL_EXECUTE_PROPERTY->items[i].number.target;
		}
		PRIVATE_DATA->batch_remaining = PRIVATE_DATA->batch_count;
		PRIVATE_DATA->batch_command = 0x10 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0);
		PRIVATE_DATA->deadline = indigo_monotonic_time() + 10 + PRIVATE_DATA->batch_count * (PRIVATE_DATA->batch_settle + PRIVATE_DATA->batch_per_step * PRIVATE_DATA->batch_interval + 1);
		PRIVATE_DATA->batch_start_pending = true;
		PRIVATE_DATA->uncertain = false;
		wemacro_batch_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, batch_start_finalizer);
	}
	//- focuser.X_RAIL_EXECUTE.on_change
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "WeMacro Rail controller");
		//- focuser.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 2;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 0xFFFFFF;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_RAIL_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RAIL_CONFIG_PROPERTY_NAME, X_RAIL_BATCH, "Set configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (X_RAIL_CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RAIL_CONFIG_BACK_ITEM, X_RAIL_CONFIG_BACK_ITEM_NAME, "Return back when done", false);
		indigo_init_switch_item(X_RAIL_CONFIG_BEEP_ITEM, X_RAIL_CONFIG_BEEP_ITEM_NAME, "Beep when done", false);
		X_RAIL_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RAIL_SHUTTER_PROPERTY_NAME, X_RAIL_BATCH, "Fire shutter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_RAIL_SHUTTER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RAIL_SHUTTER_ITEM, X_RAIL_SHUTTER_ITEM_NAME, "Fire shutter", false);
		X_RAIL_EXECUTE_PROPERTY = indigo_init_number_property(NULL, device->name, X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_BATCH, "Execute batch", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (X_RAIL_EXECUTE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RAIL_EXECUTE_SETTLE_TIME_ITEM, X_RAIL_EXECUTE_SETTLE_TIME_ITEM_NAME, "Settle time", 0, 99, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_PER_STEP_ITEM, X_RAIL_EXECUTE_PER_STEP_ITEM_NAME, "Shutter per step", 1, 9, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_INTERVAL_ITEM, X_RAIL_EXECUTE_INTERVAL_ITEM_NAME, "Shutter interval", 1, 99, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_LENGTH_ITEM, X_RAIL_EXECUTE_LENGTH_ITEM_NAME, "Step size", 1, 0xFFFFFF, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_COUNT_ITEM, X_RAIL_EXECUTE_COUNT_ITEM_NAME, "Step count", 1, 0xFFFFFF, 1, 1);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAIL_CONFIG_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAIL_SHUTTER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAIL_EXECUTE_PROPERTY);
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
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (X_RAIL_EXECUTE_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Another rail operation is in progress");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RAIL_CONFIG_PROPERTY, property)) {
		indigo_property_copy_values(X_RAIL_CONFIG_PROPERTY, property, false);
		X_RAIL_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RAIL_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RAIL_SHUTTER_PROPERTY, focuser_x_rail_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RAIL_EXECUTE_PROPERTY, property)) {
		//+ focuser.X_RAIL_EXECUTE.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, "Another rail operation is in progress");
			return INDIGO_OK;
		}
		//- focuser.X_RAIL_EXECUTE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RAIL_EXECUTE_PROPERTY, focuser_x_rail_execute_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_RAIL_CONFIG_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_RAIL_CONFIG_PROPERTY);
	indigo_release_property(X_RAIL_SHUTTER_PROPERTY);
	indigo_release_property(X_RAIL_EXECUTE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_wemacro(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static wemacro_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "USB2.0-Serial");
			patterns[0].vendor_id = 0x1A86;
			patterns[0].product_id = 0x7523;
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = (wemacro_private_data *)indigo_safe_malloc(sizeof(wemacro_private_data));
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
