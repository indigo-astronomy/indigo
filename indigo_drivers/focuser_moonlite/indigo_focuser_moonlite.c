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

// This file generated from indigo_focuser_moonlite.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_moonlite.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_focuser_moonlite"
#define DRIVER_LABEL         "MoonLite Focuser"
#define FOCUSER_DEVICE_NAME  "MoonLite"
#define PRIVATE_DATA         ((moonlite_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define MOONLITE_MIN_POSITION 0
#define MOONLITE_MAX_POSITION 65535

//- define

#pragma mark - Property definitions

#define X_FOCUSER_STEPPING_MODE_PROPERTY       (PRIVATE_DATA->x_focuser_stepping_mode_property)
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM      (X_FOCUSER_STEPPING_MODE_PROPERTY->items + 0)
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM      (X_FOCUSER_STEPPING_MODE_PROPERTY->items + 1)

#define X_FOCUSER_STEPPING_MODE_PROPERTY_NAME  "X_FOCUSER_STEPPING_MODE"
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME "HALF"
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM_NAME "FULL"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_stepping_mode_property;
	//+ data
	char response[64];
	int position, expected_position, last_position;
	int stalled;
	bool active, uncertain, temperature_pending;
	//- data
} moonlite_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static bool moonlite_command(indigo_device *device, int expected, const char *command, ...) {
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
	if (expected < 0) {
		return true;
	}
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, RESPONSE, sizeof(PRIVATE_DATA->response) - 1, "#", "", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count != expected + 1 || RESPONSE[count - 1] != '#' || (long)strlen(RESPONSE) != count) {
		return false;
	}
	RESPONSE[count - 1] = 0;
	if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.01)) > 0) {
		indigo_uni_discard(PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static bool moonlite_hex(indigo_device *device, int digits, unsigned *value) {
	unsigned result = 0;
	for (int index = 0; index < digits; index++) {
		unsigned char c = (unsigned char)RESPONSE[index];
		if (!isxdigit(c)) {
			return false;
		}
		result = result * 16 + (unsigned)(isdigit(c) ? c - '0' : toupper(c) - 'A' + 10);
	}
	if (RESPONSE[digits]) {
		return false;
	}
	*value = result;
	return true;
}

static bool moonlite_query(indigo_device *device, const char *command, int digits, unsigned *value) {
	return moonlite_command(device, digits, command) && moonlite_hex(device, digits, value);
}

static bool moonlite_position(indigo_device *device, int *position) {
	unsigned value = 0;
	if (!moonlite_query(device, ":GP#", 4, &value)) {
		return false;
	}
	PRIVATE_DATA->position = (int)value;
	FOCUSER_POSITION_ITEM->number.value = value;
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain) {
		FOCUSER_POSITION_ITEM->number.target = value;
	}
	if (position) {
		*position = (int)value;
	}
	return true;
}

static bool moonlite_moving(indigo_device *device, bool *moving) {
	unsigned value = 0;
	if (!moonlite_query(device, ":GI#", 2, &value) || value > 1) {
		return false;
	}
	*moving = value == 1;
	return true;
}

static bool moonlite_stop(indigo_device *device) {
	bool moving = true;
	int position = 0;
	if (!moonlite_command(device, -1, ":FQ#") || !moonlite_moving(device, &moving) || moving || !moonlite_position(device, &position)) {
		return false;
	}
	FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
	return true;
}

static bool moonlite_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
	unsigned version = 0;
	if (PRIVATE_DATA->handle && moonlite_query(device, ":GV#", 2, &version) && RESPONSE[0] >= '0' && RESPONSE[0] <= '9' && RESPONSE[1] >= '0' && RESPONSE[1] <= '9') {
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%c.%c", RESPONSE[0], RESPONSE[1]);
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void moonlite_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void moonlite_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	int position = 0;
	bool moving = false;
	if (!moonlite_position(device, &position) || !moonlite_moving(device, &moving)) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = !moonlite_stop(device);
		moonlite_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (!moving) {
		if (position != PRIVATE_DATA->expected_position && !moonlite_position(device, &position)) {
			PRIVATE_DATA->active = false;
			PRIVATE_DATA->uncertain = !moonlite_stop(device);
			moonlite_motion_state(device, INDIGO_ALERT_STATE);
			return;
		}
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = position != PRIVATE_DATA->expected_position;
		FOCUSER_POSITION_ITEM->number.target = position;
		moonlite_motion_state(device, PRIVATE_DATA->uncertain ? INDIGO_ALERT_STATE : INDIGO_OK_STATE);
		return;
	}
	if (position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = position;
		PRIVATE_DATA->stalled = 0;
	} else if (++PRIVATE_DATA->stalled >= 50) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = !moonlite_stop(device);
		moonlite_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	moonlite_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, 0.1, motion_finalizer);
}

static void moonlite_start_motion(indigo_device *device, int target) {
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		moonlite_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	int minimum = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	int maximum = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	target = target < minimum ? minimum : target > maximum ? maximum : target;
	FOCUSER_POSITION_ITEM->number.target = target;
	if (target == PRIVATE_DATA->position) {
		moonlite_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	unsigned accepted = 0;
	if (!moonlite_command(device, -1, ":SN%04X#", target) || !moonlite_query(device, ":GN#", 4, &accepted) || accepted != (unsigned)target || !moonlite_command(device, -1, ":FG#")) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = !moonlite_stop(device);
		moonlite_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->expected_position = target;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	PRIVATE_DATA->stalled = 0;
	PRIVATE_DATA->active = true;
	moonlite_motion_state(device, INDIGO_BUSY_STATE);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		FOCUSER_POSITION_PROPERTY->state = moonlite_position(device, NULL) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	unsigned temperature = 0;
	if (PRIVATE_DATA->temperature_pending) {
		if (moonlite_query(device, ":GT#", 4, &temperature)) {
			FOCUSER_TEMPERATURE_ITEM->number.value = (int16_t)temperature / 2.0;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		PRIVATE_DATA->temperature_pending = false;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.25, focuser_timer_callback);
	} else {
		PRIVATE_DATA->temperature_pending = moonlite_command(device, -1, ":C#");
		if (!PRIVATE_DATA->temperature_pending) {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
		indigo_execute_handler_in(device, PRIVATE_DATA->temperature_pending ? 0.75 : 1, focuser_timer_callback);
	}
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = moonlite_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			unsigned temperature = 0, position = 0, coefficient = 0, speed = 0, stepping = 0;
			bool moving = true;
			connection_result = moonlite_command(device, -1, ":FQ#") && moonlite_moving(device, &moving) && !moving && moonlite_command(device, -1, ":-#") && moonlite_query(device, ":GP#", 4, &position) && moonlite_query(device, ":GC#", 2, &coefficient) && moonlite_query(device, ":GD#", 2, &speed) && moonlite_query(device, ":GH#", 2, &stepping) && moonlite_command(device, -1, ":C#");
			if (connection_result) {
				indigo_usleep(750000);
				connection_result = moonlite_query(device, ":GT#", 4, &temperature);
			}
			if (connection_result && (speed == 2 || speed == 4 || speed == 8 || speed == 16 || speed == 32) && (stepping == 0 || stepping == 255)) {
				PRIVATE_DATA->position = (int)position;
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = PRIVATE_DATA->temperature_pending = false;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = speed == 2 ? 1 : speed == 4 ? 2 : speed == 8 ? 3 : speed == 16 ? 4 : 5;
				FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = (int8_t)coefficient;
				FOCUSER_TEMPERATURE_ITEM->number.value = (int16_t)temperature / 2.0;
				indigo_set_switch(X_FOCUSER_STEPPING_MODE_PROPERTY, stepping == 255 ? X_FOCUSER_STEPPING_MODE_HALF_ITEM : X_FOCUSER_STEPPING_MODE_FULL_ITEM, true);
				indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_MANUAL_ITEM, true);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				connection_result = false;
				moonlite_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
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
			moonlite_stop(device);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = PRIVATE_DATA->temperature_pending = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
		moonlite_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	int requested = (int)FOCUSER_SPEED_ITEM->number.target;
	unsigned actual = 0;
	unsigned encoded = 1U << requested;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !moonlite_command(device, -1, ":SD%02X#", encoded) || !moonlite_query(device, ":GD#", 2, &actual) || actual != encoded) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_SPEED_ITEM->number.value = requested;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int64_t steps = (int64_t)FOCUSER_STEPS_ITEM->number.target;
	int direction = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1;
	if (FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value) {
		direction = -direction;
	}
	int64_t target = (int64_t)PRIVATE_DATA->position + direction * steps;
	target = target < MOONLITE_MIN_POSITION ? MOONLITE_MIN_POSITION : target > MOONLITE_MAX_POSITION ? MOONLITE_MAX_POSITION : target;
	moonlite_start_motion(device, (int)target);
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	moonlite_start_motion(device, (int)FOCUSER_POSITION_ITEM->number.target);
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
		if (IS_CONNECTED && moonlite_stop(device)) {
			PRIVATE_DATA->uncertain = false;
			moonlite_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			moonlite_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	int requested = (int)FOCUSER_COMPENSATION_ITEM->number.target;
	unsigned actual = 0;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !moonlite_command(device, -1, ":SC%02X#", (unsigned)(uint8_t)requested) || !moonlite_query(device, ":GC#", 2, &actual) || (int8_t)actual != requested) {
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_COMPENSATION_ITEM->number.value = requested;
	}
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !moonlite_command(device, -1, FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? ":+#" : ":-#")) {
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	if (FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_x_focuser_stepping_mode_handler(indigo_device *device) {
	X_FOCUSER_STEPPING_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_STEPPING_MODE.on_change
	unsigned actual = 0;
	bool half = X_FOCUSER_STEPPING_MODE_HALF_ITEM->sw.value;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !moonlite_command(device, -1, half ? ":SH#" : ":SF#") || !moonlite_query(device, ":GH#", 2, &actual) || actual != (half ? 255 : 0)) {
		X_FOCUSER_STEPPING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_STEPPING_MODE.on_change
	indigo_update_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "MoonLite Focuser");
		//- focuser.on_attach
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
		FOCUSER_SPEED_ITEM->number.max = 5;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = MOONLITE_MAX_POSITION;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = MOONLITE_MIN_POSITION;
		FOCUSER_POSITION_ITEM->number.max = MOONLITE_MAX_POSITION;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_ITEM->number.min = -128;
		FOCUSER_COMPENSATION_ITEM->number.max = 127;
		FOCUSER_COMPENSATION_ITEM->number.step = 1;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = MOONLITE_MIN_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = MOONLITE_MAX_POSITION;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.step = 1;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = MOONLITE_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = MOONLITE_MIN_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = MOONLITE_MAX_POSITION;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = MOONLITE_MAX_POSITION;
		//- focuser.FOCUSER_LIMITS.on_attach
		X_FOCUSER_STEPPING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Stepping mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEPPING_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_HALF_ITEM, X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME, "Half", false);
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_FULL_ITEM, X_FOCUSER_STEPPING_MODE_FULL_ITEM_NAME, "Full", true);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_STEPPING_MODE_PROPERTY);
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
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_STEPPING_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_STEPPING_MODE_PROPERTY, focuser_x_focuser_stepping_mode_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEPPING_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_moonlite(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static moonlite_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (moonlite_private_data *)indigo_safe_malloc(sizeof(moonlite_private_data));
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
