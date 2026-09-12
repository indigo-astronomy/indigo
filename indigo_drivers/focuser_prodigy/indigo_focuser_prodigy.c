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

// This file generated from indigo_focuser_prodigy.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdarg.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_prodigy.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000003
#define DRIVER_NAME          "indigo_focuser_prodigy"
#define DRIVER_LABEL         "PegasusAstro Prodigy Microfocuser"
#define FOCUSER_DEVICE_NAME  "Pegasus Prodigy Focuser"
#define AUX_DEVICE_NAME      "Pegasus Prodigy Powerbox"
#define PRIVATE_DATA         ((prodigy_private_data *)device->private_data)

#pragma mark - Property definitions

#define X_FOCUSER_PARK_PROPERTY        (PRIVATE_DATA->x_focuser_park_property)
#define X_FOCUSER_PARK_ITEM            (X_FOCUSER_PARK_PROPERTY->items + 0)

#define X_FOCUSER_PARK_PROPERTY_NAME   "X_FOCUSER_PARK"
#define X_FOCUSER_PARK_ITEM_NAME       "PARK"

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_NAME_2_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_USB_PORT_NAME_1_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_USB_PORT_NAME_2_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)

#define AUX_USB_PORT_PROPERTY          (PRIVATE_DATA->aux_usb_port_property)
#define AUX_USB_PORT_1_ITEM            (AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM            (AUX_USB_PORT_PROPERTY->items + 1)

#define X_AUX_REBOOT_PROPERTY          (PRIVATE_DATA->x_aux_reboot_property)
#define X_AUX_REBOOT_ITEM              (X_AUX_REBOOT_PROPERTY->items + 0)

#define X_AUX_REBOOT_PROPERTY_NAME     "X_AUX_REBOOT"
#define X_AUX_REBOOT_ITEM_NAME         "REBOOT"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_focuser_park_property;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_power_outlet_property;
	indigo_property *aux_usb_port_property;
	indigo_property *x_aux_reboot_property;
	//+ data
	char response[128];
	int position, moving, speed, backlash, last_position, stalled, reboot_attempts;
	bool active, parking, uncertain, rebooting;
	bool outlets[4];
	//- data
} prodigy_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void focuser_x_focuser_park_handler(indigo_device *device);

static bool prodigy_command(indigo_device *device, bool reply, const char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
	}
	if (result <= 0) {
		return false;
	}
	if (!reply) {
		return true;
	}
	// Keep the terminator to distinguish a complete line from timeout/truncation.
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count <= 1 || PRIVATE_DATA->response[count - 1] != '\n' || (long)strlen(PRIVATE_DATA->response) != count) {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	return true;
}

static bool prodigy_number(const char *text, double min, double max, double *value, bool integer) {
	if (!text || !*text || isspace((unsigned char)*text)) {
		return false;
	}
	char *end;
	errno = 0;
	*value = strtod(text, &end);
	return !errno && !*end && isfinite(*value) && *value >= min && *value <= max && (!integer || floor(*value) == *value);
}

static bool prodigy_version(const char *text) {
	const char *dot = strchr(text, '.');
	return dot && dot != text && dot[1] && strspn(text, "0123456789") == (size_t)(dot - text) && strspn(dot + 1, "0123456789") == strlen(dot + 1);
}

static bool prodigy_echo(indigo_device *device, const char *command, const char *reply) {
	return prodigy_command(device, true, "%s", command) && !strcmp(PRIVATE_DATA->response, reply ? reply : command);
}

static bool prodigy_set(indigo_device *device, char command, int value) {
	double actual;
	return prodigy_command(device, true, "%c:%d", command, value) && PRIVATE_DATA->response[0] == command && PRIVATE_DATA->response[1] == ':' && prodigy_number(PRIVATE_DATA->response + 2, value, value, &actual, true);
}

static bool prodigy_status(indigo_device *device) {
	double position, moving;
	if (!prodigy_command(device, true, "P") || !prodigy_number(PRIVATE_DATA->response, -999999, 999999, &position, true) || !prodigy_command(device, true, "I") || !prodigy_number(PRIVATE_DATA->response, 0, 1, &moving, true)) {
		return false;
	}
	PRIVATE_DATA->position = (int)position;
	PRIVATE_DATA->moving = (int)moving;
	return true;
}

static bool prodigy_ports(indigo_device *device) {
	if (!prodigy_command(device, true, "D") || strlen(PRIVATE_DATA->response) != 9 || PRIVATE_DATA->response[0] != 'D') {
		return false;
	}
	for (int i = 0; i < 4; i++) {
		if (PRIVATE_DATA->response[2 * i + 1] != ':' || (PRIVATE_DATA->response[2 * i + 2] != '0' && PRIVATE_DATA->response[2 * i + 2] != '1')) {
			return false;
		}
	}
	for (int i = 0; i < 4; i++) {
		PRIVATE_DATA->outlets[i] = PRIVATE_DATA->response[2 * i + 2] == '1';
	}
	return true;
}

static bool prodigy_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle && prodigy_echo(device, "#", "OK_PRDG")) {
		PRIVATE_DATA->uncertain = PRIVATE_DATA->rebooting = false;
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void prodigy_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ focuser.code

static void prodigy_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->position;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	if (PRIVATE_DATA->parking) {
		X_FOCUSER_PARK_PROPERTY->state = state;
		indigo_update_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
		if (state != INDIGO_BUSY_STATE) {
			PRIVATE_DATA->parking = false;
		}
	}
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	if (!prodigy_status(device) || (PRIVATE_DATA->moving && ++PRIVATE_DATA->stalled >= 100) || (!PRIVATE_DATA->moving && PRIVATE_DATA->position != (int)FOCUSER_POSITION_ITEM->number.target)) {
		PRIVATE_DATA->active = false;
		PRIVATE_DATA->uncertain = true;
		prodigy_echo(device, "H", "0");
		prodigy_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (PRIVATE_DATA->last_position != PRIVATE_DATA->position) {
		PRIVATE_DATA->last_position = PRIVATE_DATA->position;
		PRIVATE_DATA->stalled = 0;
	}
	PRIVATE_DATA->active = PRIVATE_DATA->moving != 0;
	prodigy_motion_state(device, PRIVATE_DATA->active ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
	if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
}

static bool prodigy_start(indigo_device *device, int target, bool relative, bool park) {
	if (PRIVATE_DATA->active || PRIVATE_DATA->moving || PRIVATE_DATA->uncertain || PRIVATE_DATA->rebooting) {
		return false;
	}
	if (park && (FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value > 0 || FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value < 0)) {
		return false;
	}
	target = (int)fmax(FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value, fmin(FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value, target));
	FOCUSER_POSITION_ITEM->number.target = target;
	if (!park && target == PRIVATE_DATA->position) {
		prodigy_motion_state(device, INDIGO_OK_STATE);
		return true;
	}
	bool accepted = park ? prodigy_echo(device, "Z", "Z:1") : prodigy_set(device, relative ? 'G' : 'M', relative ? target - PRIVATE_DATA->position : target);
	if (!accepted) {
		PRIVATE_DATA->uncertain = true;
		prodigy_echo(device, "H", "0");
		return false;
	}
	PRIVATE_DATA->active = true;
	PRIVATE_DATA->parking = park;
	PRIVATE_DATA->stalled = 0;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	prodigy_motion_state(device, INDIGO_BUSY_STATE);
	return true;
}

static void prodigy_ranges(indigo_device *device) {
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

//- focuser.code

//+ aux.code

static void prodigy_publish_ports(indigo_device *device) {
	AUX_POWER_OUTLET_1_ITEM->sw.value = PRIVATE_DATA->outlets[0];
	AUX_POWER_OUTLET_2_ITEM->sw.value = PRIVATE_DATA->outlets[1];
	AUX_USB_PORT_1_ITEM->sw.value = PRIVATE_DATA->outlets[2];
	AUX_USB_PORT_2_ITEM->sw.value = PRIVATE_DATA->outlets[3];
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
}

static void reboot_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->rebooting) {
		return;
	}
	if (prodigy_echo(device, "#", "OK_PRDG") && prodigy_ports(device)) {
		PRIVATE_DATA->rebooting = false;
		X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
		prodigy_publish_ports(device);
	} else if (++PRIVATE_DATA->reboot_attempts >= 10) {
		PRIVATE_DATA->rebooting = false;
		PRIVATE_DATA->uncertain = true;
		X_AUX_REBOOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		indigo_execute_handler_in(device, 0.5, reboot_finalizer);
		return;
	}
	indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
}

//- aux.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->rebooting) {
		double temperature;
		if (prodigy_command(device, true, "T") && prodigy_number(PRIVATE_DATA->response, -100, 100, &temperature, false)) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		if (!PRIVATE_DATA->active && (PRIVATE_DATA->moving || (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE))) {
			if (prodigy_status(device)) {
				FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
				prodigy_motion_state(device, PRIVATE_DATA->uncertain ? INDIGO_ALERT_STATE : PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
			} else {
				prodigy_motion_state(device, INDIGO_ALERT_STATE);
			}
		}
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = prodigy_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser.on_connect
			connection_result = prodigy_command(device, true, "A");
			char *fields[10], *cursor = PRIVATE_DATA->response;
			for (int i = 0; i < 10 && connection_result; i++) {
				fields[i] = cursor;
				char *colon = strchr(cursor, ':');
				connection_result = i == 9 ? colon == NULL : colon != NULL;
				if (colon) {
					*colon = 0;
					cursor = colon + 1;
				}
			}
			double values[8] = { 0 };
			if (connection_result) {
				connection_result = !strcmp(fields[0], "OK_PRDG") && prodigy_version(fields[1]);
				for (int i = 2; i < 10 && connection_result; i++) {
					double min = i == 3 ? -100 : i == 4 ? -999999 : 0;
					double max = i == 3 ? 100 : i == 4 ? 999999 : i == 9 ? 9999 : 1;
					connection_result = prodigy_number(fields[i], min, max, values + i - 2, i != 3);
				}
			}
			if (connection_result) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Prodigy Microfocuser");
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, fields[1]);
				FOCUSER_TEMPERATURE_ITEM->number.value = values[1];
				PRIVATE_DATA->position = (int)values[2];
				PRIVATE_DATA->moving = (int)values[3];
				PRIVATE_DATA->backlash = (int)values[7];
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
				double speed = 0;
				connection_result = prodigy_command(device, true, "B") && !strncmp(PRIVATE_DATA->response, "B:", 2) && prodigy_number(PRIVATE_DATA->response + 2, 100, 1000, &speed, true);
				if (connection_result) {
					PRIVATE_DATA->speed = (int)speed;
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = speed;
					FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
					prodigy_ranges(device);
					prodigy_motion_state(device, PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
					indigo_update_property(device, INFO_PROPERTY, NULL);
				}
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				prodigy_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		if (PRIVATE_DATA->active || PRIVATE_DATA->moving || PRIVATE_DATA->uncertain) {
			PRIVATE_DATA->uncertain = !(prodigy_echo(device, "H", "0") && prodigy_status(device) && !PRIVATE_DATA->moving);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->parking = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			prodigy_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	int value = (int)FOCUSER_SPEED_ITEM->number.value;
	double readback;
	if (!PRIVATE_DATA->rebooting && prodigy_set(device, 'S', value) && prodigy_command(device, true, "B") && !strncmp(PRIVATE_DATA->response, "B:", 2) && prodigy_number(PRIVATE_DATA->response + 2, 100, 1000, &readback, true) && readback == value) {
		PRIVATE_DATA->speed = value;
	} else {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = PRIVATE_DATA->speed;
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!PRIVATE_DATA->rebooting && prodigy_set(device, 'C', (int)FOCUSER_BACKLASH_ITEM->number.value)) {
		PRIVATE_DATA->backlash = (int)FOCUSER_BACKLASH_ITEM->number.value;
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	if (PRIVATE_DATA->active || PRIVATE_DATA->moving || FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target >= FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		prodigy_ranges(device);
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		int target = (int)FOCUSER_POSITION_ITEM->number.target;
		bool ok = !PRIVATE_DATA->rebooting && !PRIVATE_DATA->uncertain && prodigy_set(device, 'W', target) && prodigy_status(device) && PRIVATE_DATA->position == target && !PRIVATE_DATA->moving;
		PRIVATE_DATA->uncertain = !ok;
		prodigy_motion_state(device, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
	} else if (!prodigy_start(device, (int)FOCUSER_POSITION_ITEM->number.target, false, false)) {
		prodigy_motion_state(device, INDIGO_ALERT_STATE);
	} else if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int target = PRIVATE_DATA->position + (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1);
	if (!prodigy_start(device, target, true, false)) {
		prodigy_motion_state(device, INDIGO_ALERT_STATE);
	} else if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		if (!PRIVATE_DATA->active && (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
			prodigy_motion_state(device, INDIGO_ALERT_STATE);
		}
		indigo_cancel_pending_handler(device, focuser_x_focuser_park_handler);
		if (X_FOCUSER_PARK_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->parking) {
			X_FOCUSER_PARK_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(X_FOCUSER_PARK_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
		if (!PRIVATE_DATA->rebooting && prodigy_echo(device, "H", "0") && prodigy_status(device) && !PRIVATE_DATA->moving) {
			indigo_cancel_pending_handler(device, motion_finalizer);
			PRIVATE_DATA->active = PRIVATE_DATA->uncertain = false;
			if (PRIVATE_DATA->parking) {
				PRIVATE_DATA->parking = false;
				X_FOCUSER_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_FOCUSER_PARK_PROPERTY, "Park aborted");
			}
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			prodigy_motion_state(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_x_focuser_park_handler(indigo_device *device) {
	//+ focuser.X_FOCUSER_PARK.on_change
	bool requested = X_FOCUSER_PARK_ITEM->sw.value;
	X_FOCUSER_PARK_ITEM->sw.value = false;
	if (!requested) {
		X_FOCUSER_PARK_PROPERTY->state = INDIGO_OK_STATE;
	} else if (!prodigy_start(device, 0, false, true)) {
		X_FOCUSER_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	X_FOCUSER_PARK_ITEM->sw.value = false;
	indigo_update_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
	//- focuser.X_FOCUSER_PARK.on_change
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 100; FOCUSER_SPEED_ITEM->number.max = 1000; FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0; FOCUSER_BACKLASH_ITEM->number.max = 9999; FOCUSER_BACKLASH_ITEM->number.step = 1;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = -999999;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 999999;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = -999999;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 999999;
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = -999999; FOCUSER_POSITION_ITEM->number.max = 999999; FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0; FOCUSER_STEPS_ITEM->number.max = 1999998; FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_PARK_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Park", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_FOCUSER_PARK_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_PARK_ITEM, X_FOCUSER_PARK_ITEM_NAME, "Park", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_PARK_PROPERTY);
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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || X_FOCUSER_PARK_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Another motion is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_FOCUSER_PARK_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Another motion is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_PARK_PROPERTY, property)) {
		//+ focuser.X_FOCUSER_PARK.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, X_FOCUSER_PARK_PROPERTY, "Another motion is pending");
			return INDIGO_OK;
		}
		//- focuser.X_FOCUSER_PARK.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_PARK_PROPERTY, focuser_x_focuser_park_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_PARK_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (aux)

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = prodigy_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux.on_connect
			connection_result = !PRIVATE_DATA->rebooting && prodigy_ports(device);
			if (connection_result) {
				prodigy_publish_ports(device);
			}
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				prodigy_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		PRIVATE_DATA->rebooting = false;
		//- aux.on_disconnect
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			prodigy_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	INDIGO_COPY_VALUE(AUX_POWER_OUTLET_1_ITEM->label, AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	INDIGO_COPY_VALUE(AUX_POWER_OUTLET_2_ITEM->label, AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	INDIGO_COPY_VALUE(AUX_USB_PORT_1_ITEM->label, AUX_USB_PORT_NAME_1_ITEM->text.value);
	INDIGO_COPY_VALUE(AUX_USB_PORT_2_ITEM->label, AUX_USB_PORT_NAME_2_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_power_outlet_handler(indigo_device *device) {
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	bool first = AUX_POWER_OUTLET_1_ITEM->sw.value, second = AUX_POWER_OUTLET_2_ITEM->sw.value;
	bool ok = !PRIVATE_DATA->rebooting && prodigy_set(device, 'X', first) && prodigy_set(device, 'Y', second);
	if (!PRIVATE_DATA->rebooting && prodigy_ports(device)) {
		ok = ok && PRIVATE_DATA->outlets[0] == first && PRIVATE_DATA->outlets[1] == second;
	} else {
		ok = false;
	}
	if (!ok) {
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	AUX_POWER_OUTLET_1_ITEM->sw.value = PRIVATE_DATA->outlets[0];
	AUX_POWER_OUTLET_2_ITEM->sw.value = PRIVATE_DATA->outlets[1];
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
}

static void aux_usb_port_handler(indigo_device *device) {
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_USB_PORT.on_change
	bool first = AUX_USB_PORT_1_ITEM->sw.value, second = AUX_USB_PORT_2_ITEM->sw.value;
	bool ok = !PRIVATE_DATA->rebooting && prodigy_set(device, 'U', first) && prodigy_set(device, 'J', second);
	if (!PRIVATE_DATA->rebooting && prodigy_ports(device)) {
		ok = ok && PRIVATE_DATA->outlets[2] == first && PRIVATE_DATA->outlets[3] == second;
	} else {
		ok = false;
	}
	if (!ok) {
		AUX_USB_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	AUX_USB_PORT_1_ITEM->sw.value = PRIVATE_DATA->outlets[2];
	AUX_USB_PORT_2_ITEM->sw.value = PRIVATE_DATA->outlets[3];
	//- aux.AUX_USB_PORT.on_change
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
}

static void aux_x_aux_reboot_handler(indigo_device *device) {
	//+ aux.X_AUX_REBOOT.on_change
	X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
	if (X_AUX_REBOOT_ITEM->sw.value) {
		if (!PRIVATE_DATA->active && !PRIVATE_DATA->moving && !PRIVATE_DATA->uncertain && prodigy_command(device, false, "Q")) {
			PRIVATE_DATA->rebooting = true;
			PRIVATE_DATA->reboot_attempts = 0;
			X_AUX_REBOOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, 1, reboot_finalizer);
		} else {
			X_AUX_REBOOT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	X_AUX_REBOOT_ITEM->sw.value = false;
	indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
	//- aux.X_AUX_REBOOT.on_change
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, "Powerbox", "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Outlet #1", "Outlet #1");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Outlet #2", "Outlet #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_1_ITEM, AUX_USB_PORT_NAME_1_ITEM_NAME, "Port #1", "Port #1");
		indigo_init_text_item(AUX_USB_PORT_NAME_2_ITEM, AUX_USB_PORT_NAME_2_ITEM_NAME, "Port #2", "Port #2");
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, "Powerbox", "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", false);
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, "Powerbox", "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_USB_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "Port #1", false);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "Port #2", false);
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_REBOOT_PROPERTY_NAME, "Powerbox", "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, X_AUX_REBOOT_ITEM_NAME, "Reboot", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_USB_PORT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_REBOOT_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_PROPERTY, aux_power_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_USB_PORT_PROPERTY, aux_usb_port_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_REBOOT_PROPERTY, aux_x_aux_reboot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_focuser_prodigy(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static prodigy_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (prodigy_private_data *)indigo_safe_malloc(sizeof(prodigy_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = focuser;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
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
