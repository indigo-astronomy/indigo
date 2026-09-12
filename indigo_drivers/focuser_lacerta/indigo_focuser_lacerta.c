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

// This file generated from indigo_focuser_lacerta.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_lacerta.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_focuser_lacerta"
#define DRIVER_LABEL         "LACERTA Motorfocus Focuser"
#define FOCUSER_DEVICE_NAME  "LACERTA Motorfocus"
#define PRIVATE_DATA         ((lacerta_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)

//- define

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	char response[96];
	int maximum, capability_maximum;
	int last_position, stalled;
	bool moving, motion_uncertain, reverse;
	//- data
} lacerta_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

static double lacerta_now(void) {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (double)time.tv_sec + time.tv_usec / 1000000.0;
}

static bool lacerta_command(indigo_device *device, char expected, const char *command, ...) {
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
	if (!expected) {
		return true;
	}
	double deadline = lacerta_now() + 2;
	for (int frames = 0; frames < 16; frames++) {
		double remaining = deadline - lacerta_now();
		if (remaining <= 0) {
			return false;
		}
		long count = indigo_uni_read_section2(PRIVATE_DATA->handle, RESPONSE, sizeof(PRIVATE_DATA->response) - 1, "\r", "\n", INDIGO_DELAY(remaining), INDIGO_DELAY(0.1));
		if (count <= 0 || RESPONSE[count - 1] != '\r' || (long)strlen(RESPONSE) != count) {
			return false;
		}
		RESPONSE[count - 1] = 0;
		if (RESPONSE[0] == expected) {
			return count > 2;
		}
		if (count > 1 && RESPONSE[0] != 'D' && RESPONSE[0] != 'M' && RESPONSE[0] != 'p') {
			return false;
		}
	}
	return false;
}

static bool lacerta_open(indigo_device *device) {
	char tail;
	int major, minor, revision;
	PRIVATE_DATA->capability_maximum = 250000;
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
	if (!PRIVATE_DATA->handle) {
		return false;
	}
	if (lacerta_command(device, 'i', ": i #") && (!strcmp(RESPONSE, "i FMC") || !strcmp(RESPONSE, "i MFOC"))) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, RESPONSE + 2);
		if (lacerta_command(device, 'v', ": v #") && sscanf(RESPONSE, "v%2d.%2d.%6d%c", &major, &minor, &revision, &tail) == 3 && major >= 1 && major <= 3 && minor >= 0 && revision >= 0) {
			PRIVATE_DATA->capability_maximum = major == 1 ? 65535 : 250000;
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, RESPONSE + 1);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		}
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	PRIVATE_DATA->capability_maximum = 250000;
	return false;
}

static void lacerta_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static bool lacerta_number(indigo_device *device, double minimum, double maximum, double *value) {
	char *end;
	if (!isspace((unsigned char)RESPONSE[1])) {
		return false;
	}
	errno = 0;
	*value = strtod(RESPONSE + 1, &end);
	if (end == RESPONSE + 1 || errno || !isfinite(*value) || *value < minimum || *value > maximum) {
		return false;
	}
	while (isspace((unsigned char)*end)) {
		end++;
	}
	return !*end;
}

static bool lacerta_integer(indigo_device *device, int minimum, int maximum, int *value) {
	double number;
	if (!lacerta_number(device, minimum, maximum, &number) || floor(number) != number) {
		return false;
	}
	*value = (int)number;
	return true;
}

static void lacerta_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static bool lacerta_halt(indigo_device *device) {
	int stopped;
	return (lacerta_command(device, 'H', ": H #") && lacerta_integer(device, 0, 1, &stopped)) && stopped == 1;
}

static void motion_finalizer(indigo_device *device) {
	int position = 0;
	if (!(lacerta_command(device, 'p', ": q #") && lacerta_integer(device, 0, PRIVATE_DATA->maximum, &position))) {
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->motion_uncertain = !lacerta_halt(device);
		lacerta_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = position;
		PRIVATE_DATA->stalled = 0;
	}
	FOCUSER_POSITION_ITEM->number.value = position;
	if (position == (int)FOCUSER_POSITION_ITEM->number.target) {
		PRIVATE_DATA->moving = false;
		lacerta_motion_state(device, INDIGO_OK_STATE);
	} else if (++PRIVATE_DATA->stalled >= 100) {
		// No progress for about ten seconds: stop before releasing the operation.
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->motion_uncertain = !lacerta_halt(device);
		lacerta_motion_state(device, INDIGO_ALERT_STATE);
	} else {
		lacerta_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
}

static void lacerta_start_motion(indigo_device *device, int position) {
	if (!IS_CONNECTED || PRIVATE_DATA->motion_uncertain) {
		lacerta_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	position = position < 0 ? 0 : position > PRIVATE_DATA->maximum ? PRIVATE_DATA->maximum : position;
	FOCUSER_POSITION_ITEM->number.target = position;
	if (position == (int)FOCUSER_POSITION_ITEM->number.value) {
		lacerta_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	if (!lacerta_command(device, 0, ": M %d#", position)) {
		PRIVATE_DATA->motion_uncertain = true;
		lacerta_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->last_position = (int)FOCUSER_POSITION_ITEM->number.value;
	PRIVATE_DATA->stalled = 0;
	PRIVATE_DATA->moving = true;
	lacerta_motion_state(device, INDIGO_BUSY_STATE);
}

static void lacerta_limits(indigo_device *device, int maximum) {
	PRIVATE_DATA->maximum = maximum;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = maximum;
	FOCUSER_POSITION_ITEM->number.max = maximum;
	FOCUSER_STEPS_ITEM->number.max = maximum;
}

static void lacerta_temperature(indigo_device *device) {
	double temperature;
	if ((lacerta_command(device, 't', ": t #") && lacerta_number(device, -100, 100, &temperature)) && temperature != 99.9 && temperature >= FOCUSER_TEMPERATURE_ITEM->number.min && temperature <= FOCUSER_TEMPERATURE_ITEM->number.max) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	lacerta_temperature(device);
	if (!PRIVATE_DATA->moving && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		int position = 0;
		if ((lacerta_command(device, 'p', ": q #") && lacerta_integer(device, 0, PRIVATE_DATA->maximum, &position))) {
			if (position != (int)FOCUSER_POSITION_ITEM->number.value) {
				FOCUSER_POSITION_ITEM->number.value = position;
				if (!PRIVATE_DATA->motion_uncertain) {
					FOCUSER_POSITION_ITEM->number.target = position;
				}
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		} else {
			lacerta_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = lacerta_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int reverse = 0, position = 0, backlash = 0, maximum = 0;
			connection_result = (lacerta_command(device, 'r', ": r #") && lacerta_integer(device, 0, 1, &reverse)) && (lacerta_command(device, 'g', ": g #") && lacerta_integer(device, 300, PRIVATE_DATA->capability_maximum, &maximum)) && (lacerta_command(device, 'p', ": q #") && lacerta_integer(device, 0, maximum, &position)) && (lacerta_command(device, 'b', ": b #") && lacerta_integer(device, 0, 255, &backlash));
			if (connection_result) {
				PRIVATE_DATA->reverse = reverse;
				indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, reverse ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = backlash;
				FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = PRIVATE_DATA->capability_maximum;
				lacerta_limits(device, maximum);
				PRIVATE_DATA->moving = PRIVATE_DATA->motion_uncertain = false;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				// Single-device generated connection failure does not close a successful open.
				lacerta_close(device);
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
		if (PRIVATE_DATA->moving || PRIVATE_DATA->motion_uncertain) {
			lacerta_halt(device);
		}
		PRIVATE_DATA->moving = PRIVATE_DATA->motion_uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		lacerta_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		lacerta_start_motion(device, position);
	} else {
		int actual;
		if (IS_CONNECTED && !PRIVATE_DATA->motion_uncertain && (lacerta_command(device, 'p', ": P %d#", position) && lacerta_integer(device, 0, PRIVATE_DATA->maximum, &actual)) && actual == position) {
			FOCUSER_POSITION_ITEM->number.value = actual;
			lacerta_motion_state(device, INDIGO_OK_STATE);
		} else {
			lacerta_motion_state(device, INDIGO_ALERT_STATE);
		}
	}
	if (PRIVATE_DATA->moving) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int direction = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? -1 : 1;
	int target = (int)FOCUSER_POSITION_ITEM->number.value + direction * (int)FOCUSER_STEPS_ITEM->number.value;
	lacerta_start_motion(device, target);
	if (PRIVATE_DATA->moving) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value && IS_CONNECTED) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		if (!PRIVATE_DATA->moving && (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
			lacerta_motion_state(device, INDIGO_ALERT_STATE);
		}
		if (lacerta_halt(device)) {
			indigo_cancel_pending_handler(device, motion_finalizer);
			PRIVATE_DATA->moving = PRIVATE_DATA->motion_uncertain = false;
			int position = 0;
			if ((lacerta_command(device, 'p', ": q #") && lacerta_integer(device, 0, PRIVATE_DATA->maximum, &position))) {
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				lacerta_motion_state(device, INDIGO_OK_STATE);
			} else {
				PRIVATE_DATA->motion_uncertain = true;
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
				lacerta_motion_state(device, INDIGO_ALERT_STATE);
			}
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = IS_CONNECTED ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	int actual, requested = (int)FOCUSER_BACKLASH_ITEM->number.target;
	if (IS_CONNECTED && !PRIVATE_DATA->moving && (lacerta_command(device, 'b', ": B %d#", requested) && lacerta_integer(device, 0, 255, &actual)) && actual == requested) {
		FOCUSER_BACKLASH_ITEM->number.value = actual;
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	int actual, requested = FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? 1 : 0;
	if (IS_CONNECTED && !PRIVATE_DATA->moving && (lacerta_command(device, 'r', requested ? ": R 1#" : ": R 0#") && lacerta_integer(device, 0, 1, &actual)) && actual == requested) {
		PRIVATE_DATA->reverse = actual;
	} else {
		indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, PRIVATE_DATA->reverse ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int actual, requested = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	if (IS_CONNECTED && !PRIVATE_DATA->moving && requested >= FOCUSER_POSITION_ITEM->number.value && (lacerta_command(device, 'g', ": G %d#", requested) && lacerta_integer(device, 300, PRIVATE_DATA->capability_maximum, &actual)) && actual == requested) {
		lacerta_limits(device, actual);
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
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
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 250000;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.max = 250000;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.max = 255;
		FOCUSER_BACKLASH_ITEM->number.step = 1;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 300;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 250000;
		//- focuser.FOCUSER_LIMITS.on_attach
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
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "A relative move is unfinished");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "An absolute move is unfinished");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
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

indigo_result indigo_focuser_lacerta(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static lacerta_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (lacerta_private_data *)indigo_safe_malloc(sizeof(lacerta_private_data));
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
