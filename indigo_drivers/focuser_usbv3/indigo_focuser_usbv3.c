// Copyright (c) 2017-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_usbv3.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_usbv3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000C
#define DRIVER_NAME          "indigo_focuser_usbv3"
#define DRIVER_LABEL         "USB_Focus v3 Focuser"
#define FOCUSER_DEVICE_NAME  "USB_Focus v3"
#define PRIVATE_DATA         ((usbv3_private_data *)device->private_data)

#pragma mark - Property definitions

#define X_FOCUSER_STEP_SIZE_PROPERTY      (PRIVATE_DATA->x_focuser_step_size_property)
#define X_FOCUSER_FULL_STEP_ITEM          (X_FOCUSER_STEP_SIZE_PROPERTY->items + 0)
#define X_FOCUSER_HALF_STEP_ITEM          (X_FOCUSER_STEP_SIZE_PROPERTY->items + 1)

#define X_FOCUSER_STEP_SIZE_PROPERTY_NAME "X_FOCUSER_STEP_SIZE"
#define X_FOCUSER_FULL_STEP_ITEM_NAME     "FULL_STEP"
#define X_FOCUSER_HALF_STEP_ITEM_NAME     "HALF_STEP"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_step_size_property;
	//+ data
	char response[128];
	// INDIGO_COPY_VALUE() clears a whole INDIGO_VALUE_SIZE buffer, so this one has that size.
	char configuration[INDIGO_VALUE_SIZE];
	bool moving;
	bool abort;
	int motion_polls;
	int position_digits;
	// the settings the controller last confirmed, shown again when a write fails
	int speed, max_position, compensation, threshold, stepmode;
	// external_motion: the idle poll follows a move the driver did not command;
	// poll_failed: the ALERT on FOCUSER_POSITION comes from a failed idle poll
	bool external_motion, poll_failed;
	int timer_ticks;
	//- data
} usbv3_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);

// FTxxxA is answered with a bare "A=0" or "A=1", without the line ending every other
// reply of this device carries, so a read that only knows a terminator has to wait for the
// inter-byte timeout instead. With a blocking second read it waited five seconds for the
// port itself to give up, returned a failure and left the buffer unterminated, so the
// caller parsed the new reply followed by the tail of the previous one ("A=10306" from
// "A=1" over "P=00306") and read the compensation sign as a large positive number.
// The reply ends with LF followed by CR, so the read has to end on the CR. Ending it on
// the LF leaves the CR in the input, and the next read then spends its first byte on that
// CR and drops to the inter-byte timeout for the reply it is actually waiting for - which
// lost every position readback the unit did not answer within a tenth of a second.
// The first byte is given three seconds rather than one: while the motor steps, the unit
// answers a position request in well under a millisecond most of the time, but it was
// measured stalling for more than a second on a long move, and a position readback the
// driver gives up on is published as a failed move.
// Verified against a USB_Focus v3 with firmware 1321 on 2026-09-22.
static bool usbv3_command(indigo_device *device, char *command, int response, ...) {
	va_list args;
	va_start(args, response);
	*PRIVATE_DATA->response = 0;
	long result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
	va_end(args);
	if (response && result > 0) {
		result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r\n", INDIGO_DELAY(3), INDIGO_DELAY(0.1));
		if (*PRIVATE_DATA->response == '*') {
			PRIVATE_DATA->moving = false;
			result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r\n", INDIGO_DELAY(3), INDIGO_DELAY(0.1));
		}
	}
	return result > 0;
}

// The device answers FQUITx with the same bare "*" it sends on its own when a move ends,
// so that reply has to be taken here. A star left in the input is read by the next motion
// poll instead, which then reports the new move as finished at the position it had a
// tenth of a second in - the abort of a move followed by a move back ended 15 steps off
// for exactly that reason.
static bool usbv3_quit(indigo_device *device) {
	*PRIVATE_DATA->response = 0;
	if (indigo_uni_printf(PRIVATE_DATA->handle, "FQUITx") <= 0) {
		return false;
	}
	// only the stop marker confirms the stop; without it the motor may still run
	if (indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r\n", INDIGO_DELAY(1), INDIGO_DELAY(0.1)) > 0 && *PRIVATE_DATA->response == '*') {
		PRIVATE_DATA->moving = false;
		return true;
	}
	return false;
}

// Setting commands are answered with a fixed acknowledgement ("DONE", "A", "!"); any
// other reply, or none, is a failure.
static bool usbv3_acknowledged(indigo_device *device, const char *acknowledgement, char *command, int value) {
	if (usbv3_command(device, command, true, value) && !strcmp(PRIVATE_DATA->response, acknowledgement)) {
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s not acknowledged", command);
	return false;
}

static bool usbv3_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle) {
		// A session that ended while the device was still talking, or a device that was
		// already moving, leaves lines queued. Drop them before the identity handshake.
		indigo_uni_discard(PRIVATE_DATA->handle);
		if (usbv3_command(device, "SWHOIS", true) && !strcmp(PRIVATE_DATA->response, "UFO")) {
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

// The link drops a byte from a long reply every so often - about one read in twenty against
// firmware 1321, measured through this driver's own reader and through a plain POSIX one,
// so the loss is on the link and not in the framework. A lost digit turns a reply into a
// shorter but perfectly parsable one, "P=00306" into "P=0030" and "C=0-1-2-012-002-1321-
// 30000" into "C=0-1-2-02-002-1321-30000", so a value the driver keeps has to be checked
// instead of just parsed. A reply that is read once and only used to be displayed, like
// the temperature, is left alone.
//
// A configuration line is read twice and accepted only when both reads agree.
static bool usbv3_read_configuration(indigo_device *device) {
	for (int attempt = 0; attempt < 3; attempt++) {
		if (!usbv3_command(device, "SGETAL", true)) {
			continue;
		}
		INDIGO_COPY_VALUE(PRIVATE_DATA->configuration, PRIVATE_DATA->response);
		if (usbv3_command(device, "SGETAL", true) && !strcmp(PRIVATE_DATA->configuration, PRIVATE_DATA->response)) {
			return true;
		}
	}
	return false;
}

// A position reply is "P=" followed by a fixed number of digits. The width the unit uses is
// taken from the first reading of a session, which is itself confirmed by a second read,
// and every later reading has to match it.
static bool usbv3_read_position(indigo_device *device, int *position) {
	for (int attempt = 0; attempt < 3; attempt++) {
		if (!usbv3_command(device, "FPOSRO", true) || strncmp(PRIVATE_DATA->response, "P=", 2)) {
			continue;
		}
		char *digits = PRIVATE_DATA->response + 2;
		size_t length = strspn(digits, "0123456789");
		if (length == 0 || digits[length] != 0 || (PRIVATE_DATA->position_digits != 0 && (int)length != PRIVATE_DATA->position_digits)) {
			continue;
		}
		*position = atoi(digits);
		if (*position < 0 || *position > 65535) {
			continue;
		}
		PRIVATE_DATA->position_digits = (int)length;
		return true;
	}
	return false;
}

// FTxxxA reports the sign of the compensation as a bare "A=0" or "A=1".
static bool usbv3_read_sign(indigo_device *device, int *sign) {
	for (int attempt = 0; attempt < 3; attempt++) {
		if (usbv3_command(device, "FTxxxA", true) && (!strcmp(PRIVATE_DATA->response, "A=0") || !strcmp(PRIVATE_DATA->response, "A=1"))) {
			*sign = PRIVATE_DATA->response[2] == '1';
			return true;
		}
	}
	return false;
}

// The settings the controller holds: the configuration line and the compensation sign.
static bool usbv3_read_settings(indigo_device *device, int *direction, int *firmware) {
	int stepmode, speed, stepsdeg, threshold, maxpos, sign;
	if (!usbv3_read_configuration(device) || sscanf(PRIVATE_DATA->configuration, "C=%d-%d-%d-%d-%d-%d-%d", direction, &stepmode, &speed, &stepsdeg, &threshold, firmware, &maxpos) != 7 || !usbv3_read_sign(device, &sign)) {
		return false;
	}
	PRIVATE_DATA->stepmode = stepmode % 2;
	PRIVATE_DATA->speed = speed;
	PRIVATE_DATA->compensation = sign ? stepsdeg : -stepsdeg;
	PRIVATE_DATA->threshold = threshold;
	PRIVATE_DATA->max_position = maxpos;
	return true;
}

static void usbv3_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ focuser.code

// Requests that move the focuser or change its geometry wait for the running motion.
static bool usbv3_motion_busy(indigo_device *device) {
	return PRIVATE_DATA->moving || PRIVATE_DATA->external_motion || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE;
}

// FOCUSER_POSITION and FOCUSER_STEPS cover the travel the controller reports; a
// running session republishes them, because ranges travel only with a definition.
static void usbv3_update_ranges(indigo_device *device, bool redefine) {
	FOCUSER_POSITION_ITEM->number.min = 0;
	FOCUSER_POSITION_ITEM->number.max = FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->max_position;
	if (redefine) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

// Every setting item shows what the controller confirmed last.
static void usbv3_show_settings(indigo_device *device) {
	FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = PRIVATE_DATA->speed;
	FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = PRIVATE_DATA->compensation;
	FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value = FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target = PRIVATE_DATA->threshold;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = PRIVATE_DATA->max_position;
	indigo_set_switch(X_FOCUSER_STEP_SIZE_PROPERTY, X_FOCUSER_STEP_SIZE_PROPERTY->items + PRIVATE_DATA->stepmode, true);
}

// A setting write is confirmed by reading the settings back; a failed write shows the
// settings the controller reports, or the last confirmed ones when that read fails too.
static bool usbv3_confirm_settings(indigo_device *device, bool written) {
	int direction, firmware;
	bool read = usbv3_read_settings(device, &direction, &firmware);
	usbv3_show_settings(device);
	return written && read;
}

static void focuser_motion_finalizer(indigo_device *device) {
	int position;
	if (!usbv3_read_position(device, &position)) {
		// a lost position stops the motor rather than letting it run unobserved
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Position readback failed, stopping the focuser");
		usbv3_quit(device);
		PRIVATE_DATA->moving = false;
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_POSITION_ITEM->number.value = position;
		if (!PRIVATE_DATA->moving) {
			FOCUSER_POSITION_PROPERTY->state = PRIVATE_DATA->abort ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
			FOCUSER_POSITION_ITEM->number.target = position;
		} else if (--PRIVATE_DATA->motion_polls <= 0) {
			usbv3_quit(device);
			PRIVATE_DATA->moving = false;
			FOCUSER_POSITION_ITEM->number.target = position;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
		}
	}
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void usbv3_start_motion(indigo_device *device, int steps) {
	indigo_cancel_pending_handler(device, focuser_motion_finalizer);
	// A move the driver did not start - one that was already running when it
	// connected - ends with a star of its own at a time nothing here can predict.
	// Dropping whatever is pending keeps that star out of the poll of this move.
	indigo_uni_discard(PRIVATE_DATA->handle);
	PRIVATE_DATA->abort = false;
	PRIVATE_DATA->moving = false;
	PRIVATE_DATA->poll_failed = false;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	if (steps != 0) {
		if (usbv3_command(device, "%c%05u", false, steps > 0 ? 'O' : 'I', abs(steps))) {
			PRIVATE_DATA->moving = true;
			PRIVATE_DATA->motion_polls = 600;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

// The idle poll reads the position the driver does not command itself. A position
// that changed is a move of the hand controller, followed BUSY until two readings
// agree; a failed read is an ALERT that the next good read clears.
static void usbv3_poll_position(indigo_device *device) {
	bool idle = !PRIVATE_DATA->moving && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE;
	if (!PRIVATE_DATA->external_motion && !idle) {
		return;
	}
	int position;
	bool read = usbv3_read_position(device, &position);
	if (!PRIVATE_DATA->external_motion && (PRIVATE_DATA->moving || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		// a request accepted while the poll was in flight owns the motion properties
		return;
	}
	if (!read) {
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_OK_STATE) {
			PRIVATE_DATA->poll_failed = true;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Position read failed");
		}
		return;
	}
	bool changed = position != (int)FOCUSER_POSITION_ITEM->number.value;
	FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
	if (changed && FOCUSER_POSITION_PROPERTY->state == INDIGO_ALERT_STATE && !PRIVATE_DATA->poll_failed && !PRIVATE_DATA->external_motion) {
		// a failed or aborted move stays ALERT, the poll only shows where it stopped
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else if (changed) {
		if (!PRIVATE_DATA->external_motion) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The focuser moves on its own");
		}
		PRIVATE_DATA->external_motion = true;
		PRIVATE_DATA->poll_failed = false;
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else if (PRIVATE_DATA->external_motion) {
		PRIVATE_DATA->external_motion = false;
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else if (PRIVATE_DATA->poll_failed) {
		// only an ALERT left by a failed poll is cleared, never one of a failed move
		PRIVATE_DATA->poll_failed = false;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
}

// The probe reading is "T=" and a signed value; a failed, malformed or implausible
// one is ALERT with the last valid reading kept.
static void usbv3_poll_temperature(indigo_device *device) {
	double temperature = 0;
	char *end = NULL;
	bool valid = usbv3_command(device, "FTMPRO", true) && !strncmp(PRIVATE_DATA->response, "T=", 2);
	if (valid) {
		temperature = strtod(PRIVATE_DATA->response + 2, &end);
		valid = end != PRIVATE_DATA->response + 2 && *end == 0 && temperature >= -55 && temperature <= 125;
	}
	if (valid) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	} else if (FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_ALERT_STATE) {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "Temperature read failed");
	}
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	// the position is polled every two seconds, the temperature every ten
	usbv3_poll_position(device);
	if (PRIVATE_DATA->timer_ticks++ % 5 == 0) {
		usbv3_poll_temperature(device);
	}
	indigo_execute_handler_in(device, 2, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = usbv3_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			// every query is mandatory: a controller that does not answer one is refused
			int direction = 0, firmware = 0, position = 0;
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
			indigo_uni_discard(PRIVATE_DATA->handle);
			PRIVATE_DATA->position_digits = 0;
			PRIVATE_DATA->moving = PRIVATE_DATA->abort = PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_failed = false;
			PRIVATE_DATA->timer_ticks = 0;
			// The first reading of the session also establishes the digit width every later one is
			// checked against, so it is confirmed by a second read of its own.
			connection_result = usbv3_read_configuration(device) && sscanf(PRIVATE_DATA->configuration, "C=%d-%*d-%*d-%*d-%*d-%d-%*d", &direction, &firmware) == 2 && usbv3_read_position(device, &position) && usbv3_read_position(device, &position) && usbv3_acknowledged(device, "!", "FMANUA", 0) && usbv3_read_settings(device, &direction, &firmware);
			if (connection_result) {
				indigo_set_switch(FOCUSER_DIRECTION_PROPERTY, FOCUSER_DIRECTION_PROPERTY->items + direction % 2, true);
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DRIVER_LABEL);
				snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, sizeof(INFO_DEVICE_FW_REVISION_ITEM->text.value), "%d", firmware);
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
				FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
				indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_MANUAL_ITEM, true);
				usbv3_show_settings(device);
				usbv3_update_ranges(device, false);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "The controller did not answer the connect sequence");
				usbv3_close(device);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
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
		// a running move is stopped before the port closes
		usbv3_quit(device);
		PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = false;
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			X_FOCUSER_STEP_SIZE_PROPERTY,
			FOCUSER_REVERSE_MOTION_PROPERTY,
			FOCUSER_TEMPERATURE_PROPERTY,
			FOCUSER_COMPENSATION_PROPERTY,
			FOCUSER_MODE_PROPERTY,
			FOCUSER_SPEED_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
			FOCUSER_LIMITS_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
		usbv3_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_timer_callback);
	}
}

static void focuser_x_focuser_step_size_handler(indigo_device *device) {
	X_FOCUSER_STEP_SIZE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_STEP_SIZE.on_change
	// the step size commands are not answered, so the settings readback decides
	int stepmode = X_FOCUSER_FULL_STEP_ITEM->sw.value ? 0 : 1;
	bool written = usbv3_command(device, stepmode == 0 ? "SMSTPF" : "SMSTPD", false);
	if (!usbv3_confirm_settings(device, written) || PRIVATE_DATA->stepmode != stepmode) {
		X_FOCUSER_STEP_SIZE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_STEP_SIZE.on_change
	indigo_update_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	int compensation = (int)FOCUSER_COMPENSATION_ITEM->number.target;
	int threshold = (int)FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target;
	bool written = usbv3_acknowledged(device, "DONE", "FLX%03d", abs(compensation)) && usbv3_acknowledged(device, "DONE", "FZSIG%d", compensation < 0 ? 0 : 1) && usbv3_acknowledged(device, "DONE", "SMA%03d", threshold);
	if (!usbv3_confirm_settings(device, written) || PRIVATE_DATA->compensation != compensation || PRIVATE_DATA->threshold != threshold) {
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_timer_callback);
		if (usbv3_acknowledged(device, "A", "FAUTOM", 0)) {
			// the controller moves on its own now, the position is only reported
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		} else {
			indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_MANUAL_ITEM, true);
			FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_execute_priority_handler(device, 100, focuser_timer_callback);
		}
	} else {
		int position;
		// The unit answers FMANUA with an exclamation mark, so the position it stands
		// at after the mode change is read with the command that reports it.
		if (usbv3_acknowledged(device, "!", "FMANUA", 0) && usbv3_read_position(device, &position)) {
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
		} else {
			indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_AUTOMATIC_ITEM, true);
			FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		// the temperature is read again right away
		PRIVATE_DATA->timer_ticks = 0;
		indigo_execute_priority_handler(device, 100, focuser_timer_callback);
	}
	indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	// The device answers a speed change with DONE. Reading it here keeps it from
	// being delivered to the next command, which used to publish that DONE as a
	// failed position readback.
	int speed = (int)FOCUSER_SPEED_ITEM->number.target;
	bool written = usbv3_acknowledged(device, "DONE", "SMO%03u", speed);
	if (!usbv3_confirm_settings(device, written) || PRIVATE_DATA->speed != speed) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	if (!(FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value)) {
		steps = -steps;
	}
	int target = position + steps;
	int min = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	int max = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	if (target < min) {
		target = min;
	} else if (target > max) {
		target = max;
	}
	FOCUSER_POSITION_ITEM->number.target = target;
	steps = target - position;
	usbv3_start_motion(device, steps);
	if (PRIVATE_DATA->moving) {
		indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	int target = (int)FOCUSER_POSITION_ITEM->number.target;
	int min = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	int max = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	if (target < min) {
		target = min;
	} else if (target > max) {
		target = max;
	}
	FOCUSER_POSITION_ITEM->number.target = target;
	int steps = FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? target - position : position - target;
	usbv3_start_motion(device, steps);
	if (PRIVATE_DATA->moving) {
		indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	// a request with the item OFF, and an abort with nothing moving, send nothing
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		if (PRIVATE_DATA->moving) {
			// the finalizer ends the move ALERT at the stopped position; without the
			// stop marker the move stays BUSY and the next abort stops it again
			if (usbv3_quit(device)) {
				PRIVATE_DATA->abort = true;
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			PRIVATE_DATA->external_motion = false;
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
			INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
			INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_STEPS_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	// The device answers a travel limit change with DONE, which has to be read for the
	// same reason as the speed change above. A limit below the focuser is refused.
	int max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	int confirmed = PRIVATE_DATA->max_position;
	if (max_position < (int)FOCUSER_POSITION_ITEM->number.value) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		usbv3_show_settings(device);
		indigo_send_message(device, ALERT_PROPERTY, "The focuser is beyond the requested maximum %d", max_position);
	} else {
		bool written = usbv3_acknowledged(device, "DONE", "M%05u", max_position);
		if (!usbv3_confirm_settings(device, written) || PRIVATE_DATA->max_position != max_position) {
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
	if (confirmed != PRIVATE_DATA->max_position) {
		usbv3_update_ranges(device, true);
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
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- focuser.on_attach
		X_FOCUSER_STEP_SIZE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_STEP_SIZE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Step size", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEP_SIZE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_FULL_STEP_ITEM, X_FOCUSER_FULL_STEP_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_FOCUSER_HALF_STEP_ITEM, X_FOCUSER_HALF_STEP_ITEM_NAME, "Half step", false);
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		FOCUSER_COMPENSATION_ITEM->number.min = -999;
		FOCUSER_COMPENSATION_ITEM->number.max = 999;
		FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.min = 0;
		FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.max = 5;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = 4;
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 8;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 65535;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 65535;
		//- focuser.FOCUSER_LIMITS.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_STEP_SIZE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_STEP_SIZE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(usbv3_motion_busy(device), X_FOCUSER_STEP_SIZE_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_STEP_SIZE_PROPERTY, focuser_x_focuser_step_size_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(usbv3_motion_busy(device), FOCUSER_REVERSE_MOTION_PROPERTY, "The focuser is moving");
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(usbv3_motion_busy(device), FOCUSER_COMPENSATION_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->moving || PRIVATE_DATA->external_motion || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_MODE_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(usbv3_motion_busy(device) || FOCUSER_MODE_AUTOMATIC_ITEM->sw.value, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(usbv3_motion_busy(device) || FOCUSER_MODE_AUTOMATIC_ITEM->sw.value, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(usbv3_motion_busy(device), FOCUSER_LIMITS_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_REVERSE_MOTION_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEP_SIZE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_usbv3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbv3_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "SERIAL DEMO");
			strcpy(patterns[0].vendor_string, "CCS");
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = (usbv3_private_data *)indigo_safe_malloc(sizeof(usbv3_private_data));
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
