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

// This file generated from indigo_focuser_optec.driver

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
#include <math.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_optec.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_focuser_optec"
#define DRIVER_LABEL         "Optec TCF-S Focuser"
#define FOCUSER_DEVICE_NAME  "Optec TCF-S"
#define PRIVATE_DATA         ((optec_private_data *)device->private_data)

//+ define

#define OPTEC_MIN_POSITION   0
#define OPTEC_MAX_POSITION   9999
#define OPTEC_STALL_POLLS    50

//- define

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	char response[64];
	int position, expected_position, last_position, stalled, recovery_position, recovery_samples;
	bool active, uncertain, automatic_mode;
	//- data
} optec_private_data;

#pragma mark - Low level code

//+ code

static void focuser_steps_handler(indigo_device *device);

static bool optec_command(indigo_device *device, int expected, const char *command, ...) {
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
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\n", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count != expected + 1 || PRIVATE_DATA->response[count - 1] != '\r' || (long)strlen(PRIVATE_DATA->response) != count) {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.01)) > 0) {
		indigo_uni_discard(PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static bool optec_exact(indigo_device *device, const char *expected, const char *command, ...) {
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
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\n", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	long length = (long)strlen(expected);
	if (count != length + 1 || PRIVATE_DATA->response[count - 1] != '\r' || (long)strlen(PRIVATE_DATA->response) != count) {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	return !strcmp(PRIVATE_DATA->response, expected) && indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.01)) == 0;
}

static bool optec_digits(const char *text, int digits, int min, int max, int *value) {
	if ((int)strlen(text) != digits) {
		return false;
	}
	int result = 0;
	for (int index = 0; index < digits; index++) {
		if (!isdigit((unsigned char)text[index])) {
			return false;
		}
		result = result * 10 + text[index] - '0';
	}
	if (result < min || result > max) {
		return false;
	}
	*value = result;
	return true;
}

static bool optec_position(indigo_device *device, int *position) {
	int value = 0;
	if (!optec_command(device, 6, "FPOSRO") || strncmp(PRIVATE_DATA->response, "P=", 2) || !optec_digits(PRIVATE_DATA->response + 2, 4, OPTEC_MIN_POSITION, OPTEC_MAX_POSITION, &value)) {
		return false;
	}
	PRIVATE_DATA->position = value;
	FOCUSER_POSITION_ITEM->number.value = value;
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain) {
		FOCUSER_POSITION_ITEM->number.target = value;
	}
	if (position) {
		*position = value;
	}
	return true;
}

static bool optec_temperature(indigo_device *device) {
	if (!optec_command(device, 7, "FTMPRO") || strncmp(PRIVATE_DATA->response, "T=", 2) || (PRIVATE_DATA->response[2] != '+' && PRIVATE_DATA->response[2] != '-') || !isdigit((unsigned char)PRIVATE_DATA->response[3]) || !isdigit((unsigned char)PRIVATE_DATA->response[4]) || PRIVATE_DATA->response[5] != '.' || !isdigit((unsigned char)PRIVATE_DATA->response[6])) {
		return false;
	}
	char *end;
	errno = 0;
	double value = strtod(PRIVATE_DATA->response + 2, &end);
	if (errno || *end || !isfinite(value) || value < -40 || value > 100) {
		return false;
	}
	FOCUSER_TEMPERATURE_ITEM->number.value = value;
	return true;
}

static bool optec_compensation(indigo_device *device, int *coefficient) {
	int magnitude = 0, sign = 0;
	if (!optec_command(device, 6, "FREADA") || strncmp(PRIVATE_DATA->response, "A=", 2) || !optec_digits(PRIVATE_DATA->response + 2, 4, 0, 999, &magnitude) || !optec_command(device, 3, "FTxxxA") || strncmp(PRIVATE_DATA->response, "A=", 2) || !optec_digits(PRIVATE_DATA->response + 2, 1, 0, 1, &sign)) {
		return false;
	}
	*coefficient = sign ? -magnitude : magnitude;
	FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = *coefficient;
	return true;
}

static bool optec_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle) {
		for (int attempt = 0; attempt < 3; attempt++) {
			if (optec_exact(device, "!", "FMMODE")) {
				return true;
			}
		}
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void optec_close(indigo_device *device) {
	if (PRIVATE_DATA->handle) {
		if (!optec_exact(device, "END", "FFMODE")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to release serial control");
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

static void optec_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	int position = 0;
	if (!optec_position(device, &position)) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		PRIVATE_DATA->recovery_samples = 0;
		optec_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (position == PRIVATE_DATA->expected_position) {
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_POSITION_ITEM->number.target = position;
		optec_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	if (position != PRIVATE_DATA->last_position) {
		PRIVATE_DATA->last_position = position;
		PRIVATE_DATA->stalled = 0;
	} else if (++PRIVATE_DATA->stalled >= OPTEC_STALL_POLLS) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		PRIVATE_DATA->recovery_position = position;
		PRIVATE_DATA->recovery_samples = 1;
		optec_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	optec_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, 0.1, motion_finalizer);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->automatic_mode && !PRIVATE_DATA->active) {
		int position = 0;
		if (optec_position(device, &position)) {
			if (PRIVATE_DATA->uncertain) {
				if (PRIVATE_DATA->recovery_samples > 0 && position == PRIVATE_DATA->recovery_position) {
					PRIVATE_DATA->uncertain = false;
					PRIVATE_DATA->recovery_samples = 0;
					FOCUSER_POSITION_ITEM->number.target = position;
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					PRIVATE_DATA->recovery_position = position;
					PRIVATE_DATA->recovery_samples = 1;
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} else {
				FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		FOCUSER_TEMPERATURE_PROPERTY->state = optec_temperature(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = optec_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int position = 0, coefficient = 0;
			connection_result = optec_position(device, &position) && optec_temperature(device) && optec_compensation(device, &coefficient);
			if (connection_result) {
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = PRIVATE_DATA->automatic_mode = false;
				PRIVATE_DATA->recovery_samples = 0;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_MANUAL_ITEM, true);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				optec_close(device);
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
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		PRIVATE_DATA->recovery_samples = 0;
		//- focuser.on_disconnect
		optec_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	bool inward = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value;
	bool physical_inward = inward != FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	int target = physical_inward ? PRIVATE_DATA->position - steps : PRIVATE_DATA->position + steps;
	target = target < OPTEC_MIN_POSITION ? OPTEC_MIN_POSITION : target > OPTEC_MAX_POSITION ? OPTEC_MAX_POSITION : target;
	int actual_steps = abs(target - PRIVATE_DATA->position);
	if (!IS_CONNECTED || PRIVATE_DATA->automatic_mode || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		optec_motion_state(device, INDIGO_ALERT_STATE);
	} else if (actual_steps == 0) {
		optec_motion_state(device, INDIGO_OK_STATE);
	} else if (optec_exact(device, "*", physical_inward ? "FI%04d" : "FO%04d", actual_steps)) {
		PRIVATE_DATA->expected_position = target;
		PRIVATE_DATA->last_position = PRIVATE_DATA->position;
		PRIVATE_DATA->stalled = 0;
		PRIVATE_DATA->active = true;
		FOCUSER_POSITION_ITEM->number.target = target;
		optec_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	} else {
		PRIVATE_DATA->uncertain = true;
		PRIVATE_DATA->recovery_samples = 0;
		optec_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	int requested = (int)FOCUSER_COMPENSATION_ITEM->number.target;
	bool written = IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->uncertain && optec_exact(device, "DONE", "FLA%03d", abs(requested));
	written = written && optec_exact(device, "DONE", "FZAxx%d", requested < 0 ? 1 : 0);
	int actual = 0;
	bool read = IS_CONNECTED && optec_compensation(device, &actual);
	if (!written || !read || actual != requested) {
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	bool requested = FOCUSER_MODE_AUTOMATIC_ITEM->sw.value;
	bool changed = IS_CONNECTED && !PRIVATE_DATA->active && !PRIVATE_DATA->uncertain;
	if (changed && requested) {
		changed = optec_exact(device, "DONE", "FQUIT1") && optec_command(device, -1, "FAMODE");
	} else if (changed) {
		changed = optec_exact(device, "!", "FMMODE");
	}
	if (changed) {
		PRIVATE_DATA->automatic_mode = requested;
		if (requested) {
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	} else {
		indigo_set_switch(FOCUSER_MODE_PROPERTY, PRIVATE_DATA->automatic_mode ? FOCUSER_MODE_AUTOMATIC_ITEM : FOCUSER_MODE_MANUAL_ITEM, true);
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Optec TCF-S/TCF-S3");
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = true;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = true;
		FOCUSER_LIMITS_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		FOCUSER_POSITION_ITEM->number.min = OPTEC_MIN_POSITION;
		FOCUSER_POSITION_ITEM->number.max = OPTEC_MAX_POSITION;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = OPTEC_MAX_POSITION;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_ITEM->number.min = -999;
		FOCUSER_COMPENSATION_ITEM->number.max = 999;
		FOCUSER_COMPENSATION_ITEM->number.step = 1;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
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
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
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

indigo_result indigo_focuser_optec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static optec_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (optec_private_data *)indigo_safe_malloc(sizeof(optec_private_data));
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
