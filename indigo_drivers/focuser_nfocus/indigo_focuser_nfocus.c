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

// This file generated from indigo_focuser_nfocus.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_nfocus.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_focuser_nfocus"
#define DRIVER_LABEL         "Rigel Systems nFOCUS Focuser"
#define FOCUSER_DEVICE_NAME  "nFOCUS"
#define PRIVATE_DATA         ((nfocus_private_data *)device->private_data)

//+ define

#define RESPONSE             (PRIVATE_DATA->response)
#define NFOCUS_STATUS_STALL_LIMIT 12

//- define

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	char response[16];
	int stalled;
	bool active, uncertain;
	//- data
} nfocus_private_data;

#pragma mark - Low level code

//+ code

static void focuser_steps_handler(indigo_device *device);

static bool nfocus_command(indigo_device *device, int reply_length, const char *command, ...) {
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
	if (reply_length == 0) {
		return true;
	}
	if (reply_length < 0 || reply_length >= (int)sizeof(PRIVATE_DATA->response)) {
		return false;
	}
	long count = indigo_uni_read_section(PRIVATE_DATA->handle, RESPONSE, reply_length, "", "", INDIGO_DELAY(1));
	if (count != reply_length) {
		RESPONSE[0] = 0;
		return false;
	}
	if (indigo_uni_discard(PRIVATE_DATA->handle) > 0) {
		RESPONSE[0] = 0;
		return false;
	}
	return true;
}

static bool nfocus_integer(const char *text, int minimum, int maximum, int *value) {
	if (!text || !*text || isspace((unsigned char)*text)) {
		return false;
	}
	char *end;
	errno = 0;
	long number = strtol(text, &end, 10);
	if (errno || end == text || *end || number < minimum || number > maximum) {
		return false;
	}
	*value = (int)number;
	return true;
}

static bool nfocus_speed(indigo_device *device, int *speed) {
	int raw = 0;
	if (!nfocus_command(device, 3, ":RO") || !nfocus_integer(RESPONSE, 5, 254, &raw)) {
		return false;
	}
	*speed = 255 - raw;
	return true;
}

static bool nfocus_temperature(indigo_device *device, bool *present) {
	int tenths = 0;
	if (!nfocus_command(device, 4, ":RT")) {
		return false;
	}
	if (!strcmp(RESPONSE, "-888")) {
		*present = false;
		return true;
	}
	if (!nfocus_integer(RESPONSE, -999, 999, &tenths)) {
		return false;
	}
	*present = true;
	FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = tenths / 10.0;
	return true;
}

static bool nfocus_moving(indigo_device *device, bool *moving) {
	if (!nfocus_command(device, 1, "S") || (RESPONSE[0] != '0' && RESPONSE[0] != '1')) {
		return false;
	}
	*moving = RESPONSE[0] == '1';
	return true;
}

static bool nfocus_stop(indigo_device *device) {
	return nfocus_command(device, 0, ":F11000#");
}

static void nfocus_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	bool moving = false;
	if (!nfocus_moving(device, &moving)) {
		if (++PRIVATE_DATA->stalled < NFOCUS_STATUS_STALL_LIMIT) {
			indigo_execute_handler_in(device, 0.5, motion_finalizer);
			return;
		}
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		nfocus_stop(device);
		nfocus_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->stalled = 0;
	if (moving) {
		nfocus_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.5, motion_finalizer);
	} else {
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		nfocus_motion_state(device, INDIGO_OK_STATE);
	}
}

static bool nfocus_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle && nfocus_command(device, 1, "%c", 0x06) && !strcmp(RESPONSE, "n")) {
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void nfocus_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && !FOCUSER_TEMPERATURE_PROPERTY->hidden) {
		bool temperature_present = true;
		FOCUSER_TEMPERATURE_PROPERTY->state = nfocus_temperature(device, &temperature_present) && temperature_present ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, PRIVATE_DATA->active ? 0.5 : 5, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = nfocus_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int speed = 0;
			bool temperature_present = true;
			connection_result = nfocus_temperature(device, &temperature_present) && nfocus_speed(device, &speed) && nfocus_command(device, 0, ":CS001#");
			if (connection_result) {
				PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
				PRIVATE_DATA->stalled = 0;
				FOCUSER_TEMPERATURE_PROPERTY->hidden = !temperature_present;
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = speed;
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				nfocus_close(device);
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
			nfocus_stop(device);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		nfocus_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	int requested = (int)FOCUSER_SPEED_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain || !nfocus_command(device, 0, ":CF%03d#", 255 - requested)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_SPEED_ITEM->number.value = requested;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	if (!IS_CONNECTED || PRIVATE_DATA->active || PRIVATE_DATA->uncertain) {
		nfocus_motion_state(device, INDIGO_ALERT_STATE);
	} else if (steps == 0) {
		nfocus_motion_state(device, INDIGO_OK_STATE);
	} else if (nfocus_command(device, 0, ":F%d1%03d#", FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : 0, steps)) {
		PRIVATE_DATA->active = true;
		PRIVATE_DATA->uncertain = false;
		PRIVATE_DATA->stalled = 0;
		nfocus_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.5, motion_finalizer);
	} else {
		PRIVATE_DATA->uncertain = true;
		nfocus_motion_state(device, INDIGO_ALERT_STATE);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		PRIVATE_DATA->active = false;
		if (IS_CONNECTED && nfocus_stop(device)) {
			PRIVATE_DATA->uncertain = false;
			nfocus_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			nfocus_motion_state(device, INDIGO_ALERT_STATE);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Rigel Systems nFOCUS");
		//- focuser.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 250;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Abort is unfinished");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
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

indigo_result indigo_focuser_nfocus(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nfocus_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (nfocus_private_data *)indigo_safe_malloc(sizeof(nfocus_private_data));
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
