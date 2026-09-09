// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_efa.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_efa.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000012
#define DRIVER_NAME          "indigo_focuser_efa"
#define DRIVER_LABEL         "Celestron / PlaneWave EFA Focuser"
#define FOCUSER_DEVICE_NAME  "EFA Focuser"
#define PRIVATE_DATA         ((efa_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response + 5)

//- define

#pragma mark - Property definitions

#define X_FOCUSER_FANS_PROPERTY        (PRIVATE_DATA->x_focuser_fans_property)
#define X_FOCUSER_FANS_OFF_ITEM        (X_FOCUSER_FANS_PROPERTY->items + 0)
#define X_FOCUSER_FANS_ON_ITEM         (X_FOCUSER_FANS_PROPERTY->items + 1)

#define X_FOCUSER_FANS_PROPERTY_NAME   "X_FOCUSER_FANS"
#define X_FOCUSER_FANS_OFF_ITEM_NAME   "OFF"
#define X_FOCUSER_FANS_ON_ITEM_NAME    "ON"

#define X_FOCUSER_CALIBRATION_PROPERTY      (PRIVATE_DATA->x_focuser_calibration_property)
#define X_FOCUSER_CALIBRATION_ITEM          (X_FOCUSER_CALIBRATION_PROPERTY->items + 0)

#define X_FOCUSER_CALIBRATION_PROPERTY_NAME "X_FOCUSER_CALIBRATION"
#define X_FOCUSER_CALIBRATION_ITEM_NAME     "CALIBRATE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_fans_property;
	indigo_property *x_focuser_calibration_property;
	//+ data
	uint8_t response[32];
	bool celestron, flow, active, coarse, calibrating, uncertain, fans;
	int position, last_position, stalled;
	double calibration_deadline;
	//- data
} efa_private_data;

#pragma mark - Low level code

//+ code

static double efa_now(void) {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (double)time.tv_sec + time.tv_usec / 1000000.0;
}

static bool efa_read(indigo_device *device, uint8_t *data, int count, double deadline) {
	for (int i = 0; i < count; i++) {
		double remaining = deadline - efa_now();
		if (remaining <= 0 || indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(remaining)) <= 0 || indigo_uni_read_available(PRIVATE_DATA->handle, data + i, 1) != 1) {
			return false;
		}
	}
	return true;
}

static int efa_command(indigo_device *device, uint8_t destination, uint8_t command, const uint8_t *data, int size) {
	if (!PRIVATE_DATA->handle) {
		return -1;
	}
	for (int i = 0; indigo_uni_wait_for_data(PRIVATE_DATA->handle, 0) > 0; i++) {
		if (i == 32 || indigo_uni_read_available(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response)) <= 0) {
			return -1;
		}
	}
	if (PRIVATE_DATA->flow && !PRIVATE_DATA->celestron) {
		int state = 1;
		for (int i = 0; i < 50 && (state = indigo_uni_get_cts(PRIVATE_DATA->handle)) > 0; i++) {
			indigo_usleep(10000);
		}
		if (state != 0 || indigo_uni_set_rts(PRIVATE_DATA->handle, true) < 0) {
			return -1;
		}
	}
	uint8_t out[16] = { 0x3B, (uint8_t)(size + 3), 0x20, destination, command };
	if (size) {
		memcpy(out + 5, data, (size_t)size);
	}
	unsigned sum = 0;
	for (int i = 1; i < size + 5; i++) {
		sum += out[i];
	}
	out[size + 5] = (uint8_t)(0u - sum);
	bool written = indigo_uni_write(PRIVATE_DATA->handle, (const char *)out, size + 6) == size + 6;
	bool rts = PRIVATE_DATA->flow && !PRIVATE_DATA->celestron;
	int result = -1;
	if (!written) {
		goto cleanup;
	}
	double deadline = efa_now() + 1;
	for (int frames = 0; frames < 8; frames++) {
		uint8_t *in = PRIVATE_DATA->response;
		if (!efa_read(device, in, 2, deadline) || in[0] != 0x3B || in[1] < 3 || in[1] > sizeof(PRIVATE_DATA->response) - 3 || !efa_read(device, in + 2, in[1] + 1, deadline)) {
			goto cleanup;
		}
		sum = 0;
		for (int i = 1; i < in[1] + 3; i++) {
			sum += in[i];
		}
		if (sum & 255) {
			goto cleanup;
		}
		if (in[1] == out[1] && !memcmp(in, out, (size_t)size + 6)) {
			// Echo confirms physical transmission; release RTS before awaiting the response.
			if (rts) {
				if (indigo_uni_set_rts(PRIVATE_DATA->handle, false) < 0) {
					goto cleanup;
				}
				rts = false;
			}
			continue;
		}
		if (in[2] != destination || in[3] != 0x20 || in[4] != command) {
			goto cleanup;
		}
		result = in[1] - 3;
		break;
	}
cleanup:
	if (rts && indigo_uni_set_rts(PRIVATE_DATA->handle, false) < 0) {
		result = -1;
	}
	return result;
}

static bool efa_ack(indigo_device *device, uint8_t destination, uint8_t command, const uint8_t *data, int size) {
	int count = efa_command(device, destination, command, data, size);
	return (count == 1 && RESPONSE[0] == 1) || (count == 0 && (command == 0xEF || PRIVATE_DATA->celestron));
}

static bool efa_byte(indigo_device *device, uint8_t command, uint8_t value) {
	return efa_ack(device, 0x12, command, &value, 1);
}

static bool efa_position(indigo_device *device) {
	if (efa_command(device, 0x12, 0x01, NULL, 0) != 3) {
		return false;
	}
	int position = RESPONSE[0] * 65536 + RESPONSE[1] * 256 + RESPONSE[2];
	PRIVATE_DATA->position = position & 0x800000 ? position - 0x1000000 : position;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->position;
	return true;
}

static int efa_state(indigo_device *device) {
	if (efa_command(device, 0x12, 0x13, NULL, 0) != 1 || (RESPONSE[0] != 0 && RESPONSE[0] != 254 && RESPONSE[0] != 255)) {
		return -1;
	}
	return RESPONSE[0];
}

static bool efa_target(indigo_device *device, uint8_t command, int target) {
	uint32_t bits = (uint32_t)target;
	uint8_t data[3] = { (uint8_t)(bits >> 16), (uint8_t)(bits >> 8), (uint8_t)bits };
	return efa_ack(device, 0x12, command, data, 3);
}

static bool efa_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG | BINARY_LOG);
	if (!PRIVATE_DATA->handle) {
		return false;
	}
	PRIVATE_DATA->celestron = false;
	PRIVATE_DATA->flow = indigo_uni_get_cts(PRIVATE_DATA->handle) >= 0;
	if (PRIVATE_DATA->flow && indigo_uni_set_rts(PRIVATE_DATA->handle, false) < 0) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		return false;
	}
	int count = efa_command(device, 0x12, 0xFE, NULL, 0);
	if (count == 2 || count == 4) {
		PRIVATE_DATA->celestron = count == 4;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, count == 4 ? "Celestron Focus Motor" : "PlaneWave EFA");
		if (count == 4) {
			snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%u.%u.%u", RESPONSE[0], RESPONSE[1], RESPONSE[2] * 256u + RESPONSE[3]);
		} else {
			snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%u.%u", RESPONSE[0], RESPONSE[1]);
		}
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void efa_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void efa_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void efa_ranges(indigo_device *device) {
	FOCUSER_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	FOCUSER_STEPS_ITEM->number.max = FOCUSER_POSITION_ITEM->number.max - FOCUSER_POSITION_ITEM->number.min;
	if (IS_CONNECTED) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

static bool efa_limits(indigo_device *device) {
	if (efa_command(device, 0x12, 0x2C, NULL, 0) != 8) {
		return false;
	}
	uint32_t minimum = 0, maximum = 0;
	for (int i = 0; i < 4; i++) {
		minimum = minimum * 256 + RESPONSE[i];
		maximum = maximum * 256 + RESPONSE[i + 4];
	}
	if (minimum >= maximum || maximum > 0x7FFFFF) {
		return false;
	}
	FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = minimum;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = maximum;
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	efa_ranges(device);
	return true;
}

static void efa_temperature(indigo_device *device) {
	uint8_t address = 0;
	int count = efa_command(device, 0x12, 0x26, &address, 1);
	int raw = count == 3 && RESPONSE[0] == address ? RESPONSE[1] * 256 + RESPONSE[2] : count == 2 ? RESPONSE[1] * 256 + RESPONSE[0] : 0x7F7F;
	double temperature = (raw & 0x8000 ? raw - 65536 : raw) / 16.0;
	if (raw == 0x7F7F || temperature < FOCUSER_TEMPERATURE_ITEM->number.min || temperature > FOCUSER_TEMPERATURE_ITEM->number.max) {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
}

static void efa_fail_motion(indigo_device *device) {
	PRIVATE_DATA->active = false;
	PRIVATE_DATA->uncertain = true;
	efa_byte(device, 0x24, 0);
	efa_motion_state(device, INDIGO_ALERT_STATE);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	int state = efa_state(device);
	if (state < 0 || state == 254 || !efa_position(device)) {
		efa_fail_motion(device);
		return;
	}
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	if (PRIVATE_DATA->coarse && abs(target - PRIVATE_DATA->position) <= 50000) {
		if (!efa_byte(device, 0x24, 0) || !efa_target(device, 0x17, target)) {
			efa_fail_motion(device);
			return;
		}
		PRIVATE_DATA->coarse = false;
	} else if (state == 255) {
		PRIVATE_DATA->active = false;
		efa_motion_state(device, PRIVATE_DATA->position == target ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
		return;
	}
	if (PRIVATE_DATA->position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = PRIVATE_DATA->position;
		PRIVATE_DATA->stalled = 0;
	} else if (++PRIVATE_DATA->stalled >= 100) {
		efa_fail_motion(device);
		return;
	}
	efa_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, 0.1, motion_finalizer);
}

static void calibration_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->calibrating) {
		return;
	}
	bool read = efa_command(device, 0x12, 0x2B, NULL, 0) == 2 && RESPONSE[0] <= 1;
	int completed = read ? RESPONSE[0] : 0;
	int running = read ? RESPONSE[1] : 0;
	read = read && efa_position(device);
	if (read && completed) {
		PRIVATE_DATA->calibrating = false;
		if (efa_limits(device)) {
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
			efa_motion_state(device, INDIGO_OK_STATE);
		} else {
			X_FOCUSER_CALIBRATION_PROPERTY->state = FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
			efa_motion_state(device, INDIGO_ALERT_STATE);
		}
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	} else if (!read || !running || efa_now() >= PRIVATE_DATA->calibration_deadline) {
		PRIVATE_DATA->calibrating = false;
		efa_byte(device, 0x2A, 0);
		PRIVATE_DATA->uncertain = true;
		X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		efa_motion_state(device, INDIGO_ALERT_STATE);
	} else {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.1, calibration_finalizer);
	}
	indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
}

static void efa_start(indigo_device *device, int target) {
	if (!IS_CONNECTED || PRIVATE_DATA->uncertain || PRIVATE_DATA->calibrating) {
		efa_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	target = target < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value ? (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value : target > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value ? (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value : target;
	FOCUSER_POSITION_ITEM->number.target = target;
	if (target == PRIVATE_DATA->position) {
		efa_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	PRIVATE_DATA->coarse = !PRIVATE_DATA->celestron && abs(target - PRIVATE_DATA->position) > 50000;
	bool result = PRIVATE_DATA->coarse ? efa_byte(device, target > PRIVATE_DATA->position ? 0x24 : 0x25, 9) : efa_target(device, PRIVATE_DATA->celestron ? 0x02 : 0x17, target);
	if (!result) {
		efa_fail_motion(device);
		return;
	}
	PRIVATE_DATA->active = true;
	PRIVATE_DATA->stalled = 0;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	efa_motion_state(device, INDIGO_BUSY_STATE);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->calibrating && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		if (efa_position(device)) {
			if (!PRIVATE_DATA->uncertain) {
				FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			efa_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	if (!PRIVATE_DATA->celestron) {
		efa_temperature(device);
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = efa_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			uint8_t value = 0x40;
			int calibrated = -1;
			PRIVATE_DATA->active = PRIVATE_DATA->calibrating = PRIVATE_DATA->uncertain = false;
			connection_result = efa_position(device);
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			X_FOCUSER_FANS_PROPERTY->hidden = PRIVATE_DATA->celestron;
			X_FOCUSER_CALIBRATION_PROPERTY->hidden = !PRIVATE_DATA->celestron;
			FOCUSER_TEMPERATURE_PROPERTY->hidden = PRIVATE_DATA->celestron;
			FOCUSER_ON_POSITION_SET_PROPERTY->hidden = PRIVATE_DATA->celestron;
			FOCUSER_LIMITS_PROPERTY->perm = PRIVATE_DATA->celestron ? INDIGO_RO_PERM : INDIGO_RW_PERM;
			if (connection_result && PRIVATE_DATA->celestron) {
				connection_result = efa_command(device, 0x12, 0x2B, NULL, 0) == 2 && RESPONSE[0] <= 1;
				if (connection_result) {
					calibrated = RESPONSE[0];
					connection_result = efa_limits(device);
				}
			} else if (connection_result) {
				connection_result = efa_command(device, 0x12, 0x30, &value, 1) == 1 && RESPONSE[0] <= 1;
				if (connection_result) {
					calibrated = RESPONSE[0];
					connection_result = efa_byte(device, 0xEF, 1) && efa_command(device, 0x13, 0x28, NULL, 0) == 1 && (RESPONSE[0] == 0 || RESPONSE[0] == 3);
				}
				if (connection_result) {
					PRIVATE_DATA->fans = RESPONSE[0] == 0;
					indigo_set_switch(X_FOCUSER_FANS_PROPERTY, PRIVATE_DATA->fans ? X_FOCUSER_FANS_ON_ITEM : X_FOCUSER_FANS_OFF_ITEM, true);
					efa_ranges(device);
				}
			}
			if (!connection_result) {
				efa_close(device);
			} else {
				if (calibrated == 0) {
					indigo_send_message(device, BUSY_PROPERTY, "Focuser is not calibrated!");
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
				efa_motion_state(device, INDIGO_OK_STATE);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
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
		if (PRIVATE_DATA->calibrating) {
			efa_byte(device, 0x2A, 0);
		} else if (PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
			efa_byte(device, 0x24, 0);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->calibrating = PRIVATE_DATA->uncertain = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
		efa_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	double minimum = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
	double maximum = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	if (PRIVATE_DATA->celestron || PRIVATE_DATA->active || PRIVATE_DATA->calibrating || minimum >= maximum) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = minimum;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = maximum;
		efa_ranges(device);
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	if (!PRIVATE_DATA->celestron && FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		if (efa_target(device, 0x04, target) && efa_position(device) && PRIVATE_DATA->position == target) {
			efa_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			efa_motion_state(device, INDIGO_ALERT_STATE);
		}
	} else {
		efa_start(device, target);
		if (PRIVATE_DATA->active) {
			indigo_execute_handler_in(device, 0.1, motion_finalizer);
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int delta = (int)FOCUSER_STEPS_ITEM->number.value;
	efa_start(device, PRIVATE_DATA->position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -delta : delta));
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		if (IS_CONNECTED && efa_byte(device, PRIVATE_DATA->calibrating ? 0x2A : 0x24, 0) && efa_position(device) && efa_state(device) == 255) {
			indigo_cancel_pending_handler(device, motion_finalizer);
			indigo_cancel_pending_handler(device, calibration_finalizer);
			if (PRIVATE_DATA->calibrating) {
				X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, "Calibration aborted");
			}
			PRIVATE_DATA->active = PRIVATE_DATA->calibrating = PRIVATE_DATA->uncertain = false;
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			efa_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_x_focuser_fans_handler(indigo_device *device) {
	X_FOCUSER_FANS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_FANS.on_change
	uint8_t value = X_FOCUSER_FANS_ON_ITEM->sw.value ? 1 : 0;
	if (!PRIVATE_DATA->celestron && efa_ack(device, 0x13, 0x27, &value, 1) && efa_command(device, 0x13, 0x28, NULL, 0) == 1 && RESPONSE[0] == (value ? 0 : 3)) {
		PRIVATE_DATA->fans = value;
	} else {
		X_FOCUSER_FANS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_set_switch(X_FOCUSER_FANS_PROPERTY, PRIVATE_DATA->fans ? X_FOCUSER_FANS_ON_ITEM : X_FOCUSER_FANS_OFF_ITEM, true);
	//- focuser.X_FOCUSER_FANS.on_change
	indigo_update_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
}

static void focuser_x_focuser_calibration_handler(indigo_device *device) {
	//+ focuser.X_FOCUSER_CALIBRATION.on_change
	if (!X_FOCUSER_CALIBRATION_ITEM->sw.value) {
		X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->celestron && !PRIVATE_DATA->active && !PRIVATE_DATA->uncertain && efa_byte(device, 0x2A, 1)) {
		PRIVATE_DATA->calibrating = true;
		PRIVATE_DATA->calibration_deadline = efa_now() + 180;
		X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
		efa_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, calibration_finalizer);
	} else {
		X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	X_FOCUSER_CALIBRATION_ITEM->sw.value = false;
	indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
	//- focuser.X_FOCUSER_CALIBRATION.on_change
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
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = true;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 3799422;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 3799422;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 3799422;
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 3799422;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 3799422;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_FANS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_FANS_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Fans", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_FANS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_FANS_OFF_ITEM, X_FOCUSER_FANS_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_FOCUSER_FANS_ON_ITEM, X_FOCUSER_FANS_ON_ITEM_NAME, "On", false);
		X_FOCUSER_FANS_PROPERTY->hidden = true;
		X_FOCUSER_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_CALIBRATION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FOCUSER_CALIBRATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_CALIBRATION_ITEM, X_FOCUSER_CALIBRATION_ITEM_NAME, "Calibrate", false);
		X_FOCUSER_CALIBRATION_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_FANS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_CALIBRATION_PROPERTY);
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
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || X_FOCUSER_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_FOCUSER_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_FANS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_FANS_PROPERTY, focuser_x_focuser_fans_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_CALIBRATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_CALIBRATION_PROPERTY, focuser_x_focuser_calibration_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_FANS_PROPERTY);
	indigo_release_property(X_FOCUSER_CALIBRATION_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_efa(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static efa_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(efa_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
