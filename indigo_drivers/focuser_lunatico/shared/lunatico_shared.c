// Copyright (C) 2020-2026 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
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

// version history
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>
// 3.0 refactoring to indigo_generator by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Lunatico Astronomia shared implementation
 \file lunatico_shared.c
 */

// Transport, identity, port ownership, stepper, temperature and powerbox
// implementation shared by indigo_focuser_lunatico and indigo_rotator_lunatico.
// Both .driver definitions include this file from their code { } block, so it is
// emitted into the generated low level code section where the private data type
// and the inherited property macros are already known.
//
// Each physical port is represented by a focuser, a rotator and a powerbox
// logical device, because the generator attaches every declared device block
// statically. The hardware has one stepper and one DB9 connector per port, so
// the three devices of a port share one lunatico_port_state and only the one
// that claimed the port may talk to it.
//
// The driver-specific properties of a port live in the shared private data of
// the generated driver, so each logical device has to declare its own handles;
// the helpers below therefore take the property as an argument instead of using
// a macro, which keeps one implementation for all of them.

// The port index is stored in the general purpose bits of the logical device by
// its on_attach block.
#define PORT_INDEX                  ((int)(device->gp_bits & 0x0F))
#define PORT_STATE                  (PRIVATE_DATA->port[PORT_INDEX])

static const char *lunatico_port_name[LUNATICO_PORTS] = { "Main", "Exp", "Third" };

// --------------------------------------------------------------------------------- Transport

// Send one SLP request and read its reply. A serial reply is delimited by '#',
// which indigo_uni_read_section2() keeps in the buffer, so the reply can be
// matched against the echoed request. A UDP reply is always exactly one
// datagram and a read on a UDP handle consumes the whole datagram no matter how
// few bytes were requested, so the delimited reader cannot be used there.
static bool lunatico_vcommand(indigo_device *device, const char *format, va_list args) {
	vsnprintf(PRIVATE_DATA->command, sizeof(PRIVATE_DATA->command), format, args);
	*PRIVATE_DATA->response = 0;
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		result = indigo_uni_write(PRIVATE_DATA->handle, PRIVATE_DATA->command, (long)strlen(PRIVATE_DATA->command));
	}
	if (result > 0) {
		if (PRIVATE_DATA->handle->type == INDIGO_UDP_HANDLE) {
			result = indigo_uni_wait_for_data(PRIVATE_DATA->handle, LUNATICO_REPLY_TIMEOUT);
			if (result > 0) {
				result = indigo_uni_read_available(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1);
				if (result > 0) {
					PRIVATE_DATA->response[result] = 0;
				}
			}
		} else {
			result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "#", NULL, LUNATICO_REPLY_TIMEOUT, LUNATICO_BYTE_TIMEOUT);
		}
	}
	if (result <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' -> no reply", PRIVATE_DATA->command);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' -> '%s'", PRIVATE_DATA->command, PRIVATE_DATA->response);
	return true;
}

// The controller echoes the request without its trailing '#' and appends
// ':<value>#', so the expected reply is derived from the request itself.
static bool lunatico_parse_value(indigo_device *device, int32_t *value) {
	char format[LUNATICO_CMD_LEN + 8];
	int length = (int)strlen(PRIVATE_DATA->command);
	snprintf(format, sizeof(format), "%.*s:%%d#", length > 0 ? length - 1 : 0, PRIVATE_DATA->command);
	if (sscanf(PRIVATE_DATA->response, format, value) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' -> '%s' is not a valid reply", PRIVATE_DATA->command, PRIVATE_DATA->response);
		return false;
	}
	return true;
}

static bool lunatico_command_value(indigo_device *device, int32_t *value, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = lunatico_vcommand(device, format, args);
	va_end(args);
	return result && lunatico_parse_value(device, value);
}

// Every setting and motion command answers ':0#' when it was accepted and
// ':-1#' when the controller rejected it.
static bool lunatico_command_ok(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = lunatico_vcommand(device, format, args);
	va_end(args);
	int32_t value = -1;
	return result && lunatico_parse_value(device, &value) && value == 0;
}

// --------------------------------------------------------------------------------- Identity

// !seletek version:<MOPFF># where M is the operating mode (2 is the boot
// loader), O the model, P the firmware major and FF the firmware minor version.
static bool lunatico_version(indigo_device *device, int *mode, int *model) {
	static const char *models[] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet" };
	int32_t value = 0;
	if (!lunatico_command_value(device, &value, "!seletek version#")) {
		return false;
	}
	*mode = value / 10000;
	if (*mode >= 2) {
		*mode = 2;
	}
	*model = (value / 1000) % 10;
	if (*model > 5 || *model < 0) {
		*model = 0;
	}
	snprintf(PRIVATE_DATA->board, sizeof(PRIVATE_DATA->board), "%s", models[*model]);
	snprintf(PRIVATE_DATA->firmware, sizeof(PRIVATE_DATA->firmware), "%d.%d", (value / 100) % 10, value % 100);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' -> %s %s", PRIVATE_DATA->response, PRIVATE_DATA->board, PRIVATE_DATA->firmware);
	return true;
}

// Read the identity once for the whole controller. Each logical device copies it
// into its own INFO property when it connects.
static bool lunatico_identify(indigo_device *device) {
	int mode = 0, model = 0;
	if (!lunatico_version(device, &mode, &model)) {
		return false;
	}
	PRIVATE_DATA->model = model;
	return true;
}

static void lunatico_publish_identity(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->board);
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
	indigo_update_property(device, INFO_PROPERTY, NULL);
}

// How many ports the reported model has. A controller in boot loader mode has
// none, so the connection is refused.
static bool lunatico_port_exists(indigo_device *device) {
	int mode = 0, model = 0;
	if (!lunatico_version(device, &mode, &model) || mode == 2) {
		return false;
	}
	int ports = 0;
	switch (model) {
		case MODEL_SELETEK:
		case MODEL_ARMADILLO:
			ports = 2;
			break;
		case MODEL_PLATYPUS:
			ports = 3;
			break;
		case MODEL_LIMPET:
			ports = 1;
			break;
		default:
			ports = 0;
			break;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' -> %d ports, %s port exists = %d", PRIVATE_DATA->response, ports, lunatico_port_name[PORT_INDEX], PORT_INDEX < ports);
	return PORT_INDEX < ports;
}

// --------------------------------------------------------------------------------- Connection

static bool lunatico_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (indigo_uni_is_url(name, "lunatico")) {
		char url[INDIGO_VALUE_SIZE];
		const char *host = strstr(name, "://");
		snprintf(url, sizeof(url), "udp://%s", host == NULL ? name : host + 3);
		PRIVATE_DATA->handle = indigo_uni_open_url(url, LUNATICO_UDP_PORT, INDIGO_UDP_HANDLE, INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value), INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle != NULL) {
		if (lunatico_identify(device)) {
			return true;
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' does not answer as a Lunatico controller", name);
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void lunatico_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	*PRIVATE_DATA->board = 0;
	*PRIVATE_DATA->firmware = 0;
	PRIVATE_DATA->model = 0;
}

// A port carries one stepper and one DB9 connector, so the focuser, rotator and
// powerbox device of a port are mutually exclusive. The port also has to exist
// on the controller that actually answered.
static bool lunatico_claim_port(indigo_device *device) {
	if (PORT_STATE.owner != NULL && PORT_STATE.owner != device) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s port is already used by %s", lunatico_port_name[PORT_INDEX], PORT_STATE.owner->name);
		indigo_send_message(device, ALERT_PROPERTY, "The %s port is already used by %s", lunatico_port_name[PORT_INDEX], PORT_STATE.owner->name);
		return false;
	}
	if (!lunatico_port_exists(device)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response or the %s port does not exist on this hardware", lunatico_port_name[PORT_INDEX]);
		indigo_send_message(device, ALERT_PROPERTY, "No response or the %s port does not exist on this hardware", lunatico_port_name[PORT_INDEX]);
		return false;
	}
	PORT_STATE.owner = device;
	return true;
}

static void lunatico_release_port(indigo_device *device) {
	if (PORT_STATE.owner == device) {
		PORT_STATE.owner = NULL;
	}
}

// --------------------------------------------------------------------------------- Stepper commands

static bool lunatico_get_position(indigo_device *device, int32_t *position) {
	if (!lunatico_command_value(device, position, "!step getpos %d#", PORT_INDEX)) {
		return false;
	}
	return *position >= 0;
}

static bool lunatico_sync_position(indigo_device *device, int32_t position) {
	return lunatico_command_ok(device, "!step setpos %d %d#", PORT_INDEX, position);
}

static bool lunatico_goto_position(indigo_device *device, int32_t position, int32_t backlash) {
	return lunatico_command_ok(device, "!step goto %d %d %d#", PORT_INDEX, position, backlash);
}

static bool lunatico_stop(indigo_device *device) {
	return lunatico_command_ok(device, "!step stop %d#", PORT_INDEX);
}

static bool lunatico_is_moving(indigo_device *device, bool *is_moving) {
	int32_t value = 0;
	if (!lunatico_command_value(device, &value, "!step ismoving %d#", PORT_INDEX) || value < 0) {
		return false;
	}
	*is_moving = value != 0;
	return true;
}

static bool lunatico_set_step(indigo_device *device, lunatico_step_mode mode) {
	return lunatico_command_ok(device, "!step halfstep %d %d#", PORT_INDEX, mode == STEP_MODE_HALF ? 1 : 0);
}

static bool lunatico_set_wiring(indigo_device *device, lunatico_wiring wiring) {
	return lunatico_command_ok(device, "!step wiremode %d %d#", PORT_INDEX, wiring);
}

static bool lunatico_set_motor_type(indigo_device *device, lunatico_motor_type type) {
	return lunatico_command_ok(device, "!step model %d %d#", PORT_INDEX, type);
}

static bool lunatico_set_move_power(indigo_device *device, double percent) {
	return lunatico_command_ok(device, "!step movepow %d %d#", PORT_INDEX, (int)(percent * LUNATICO_POWER_SCALE));
}

static bool lunatico_set_stop_power(indigo_device *device, double percent) {
	return lunatico_command_ok(device, "!step stoppow %d %d#", PORT_INDEX, (int)(percent * LUNATICO_POWER_SCALE));
}

static bool lunatico_set_limits(indigo_device *device, int32_t min, int32_t max) {
	return lunatico_command_ok(device, "!step setswlimits %d %d %d#", PORT_INDEX, min, max);
}

static bool lunatico_delete_limits(indigo_device *device) {
	return lunatico_command_ok(device, "!step delswlimits %d#", PORT_INDEX);
}

// The controller is configured in microseconds per step; the driver exposes a
// step rate in kHz.
static bool lunatico_set_speed(indigo_device *device, double speed_khz) {
	if (speed_khz <= 0.00001) {
		return false;
	}
	int speed_us = (int)(1000 / speed_khz);
	if (speed_us < LUNATICO_MIN_SPEED_US || speed_us > LUNATICO_MAX_SPEED_US) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Speed out of range %.3f", speed_khz);
		return false;
	}
	return lunatico_command_ok(device, "!step speedrangeus %d %d %d#", PORT_INDEX, speed_us, speed_us);
}

// idC1, idC2 and idF are the manufacturer's conversion constants; the internal
// and the external sensor use different ones.
static bool lunatico_get_temperature(indigo_device *device, int sensor, double *temperature) {
	int32_t value = 0;
	if (!lunatico_command_value(device, &value, "!read temps %d#", sensor)) {
		return false;
	}
	double idC1 = sensor == 0 ? 261 : 192;
	double idC2 = sensor == 0 ? 250 : 0;
	double idF = sensor == 0 ? 1.8 : 1.7;
	*temperature = (((value - idC1) * idF) - idC2) / 10;
	return true;
}

// --------------------------------------------------------------------------------- Powerbox commands

static bool lunatico_enable_power_outlet(indigo_device *device, int pin, bool enable) {
	if (pin < 1 || pin > 4) {
		return false;
	}
	return lunatico_command_ok(device, "!write dig %d %d %d#", PORT_INDEX, pin, enable ? 1 : 0);
}

static bool lunatico_read_sensor(indigo_device *device, int pin, int32_t *value) {
	if (pin < 5 || pin > 8) {
		return false;
	}
	if (!lunatico_command_value(device, value, "!read an %d %d#", PORT_INDEX, pin)) {
		return false;
	}
	return *value >= 0;
}

// --------------------------------------------------------------------------------- Settings

// The driver-specific settings of a port are declared once per logical device,
// so every helper takes the property it belongs to. The item order of each
// property is fixed by the .driver definitions.

static bool lunatico_apply_step_mode(indigo_device *device, indigo_property *step_mode) {
	return lunatico_set_step(device, (step_mode->items + 1)->sw.value ? STEP_MODE_HALF : STEP_MODE_FULL);
}

static bool lunatico_apply_power_control(indigo_device *device, indigo_property *power_control) {
	if (!lunatico_set_move_power(device, (power_control->items + 0)->number.value)) {
		return false;
	}
	return lunatico_set_stop_power(device, (power_control->items + 1)->number.value);
}

static bool lunatico_apply_motor_type(indigo_device *device, indigo_property *motor_type) {
	for (int i = 0; i < motor_type->count; i++) {
		if ((motor_type->items + i)->sw.value) {
			return lunatico_set_motor_type(device, (lunatico_motor_type)i);
		}
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported motor type");
	return false;
}

// Item 0 of the wiring property is the Lunatico wiring, item 1 the RF/Moonlite
// one; the reversed flag comes from FOCUSER_REVERSE_MOTION or ROTATOR_DIRECTION.
static bool lunatico_apply_wiring(indigo_device *device, indigo_property *wiring, bool reversed) {
	if ((wiring->items + 0)->sw.value) {
		return lunatico_set_wiring(device, reversed ? MW_LUNATICO_REVERSED : MW_LUNATICO_NORMAL);
	}
	if ((wiring->items + 1)->sw.value) {
		return lunatico_set_wiring(device, reversed ? MW_MOONLITE_REVERSED : MW_MOONLITE_NORMAL);
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported motor wiring");
	return false;
}

static void lunatico_apply_temperature_sensor(indigo_device *device, indigo_property *sensor) {
	PORT_STATE.temperature_sensor = (sensor->items + 1)->sw.value ? 1 : 0;
}

// The configuration the original driver applied from lunatico_init_device(), in
// the same order: delete the software limits, write both coil currents, the
// motor type and the step mode. Each failure is logged and does not abort the
// connection, which is the behaviour of the driver this replaces.
static void lunatico_configure_port(indigo_device *device, indigo_property *power_control, indigo_property *motor_type, indigo_property *step_mode) {
	if (!lunatico_delete_limits(device)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_delete_limits() failed");
	}
	if (!lunatico_apply_power_control(device, power_control)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_apply_power_control() failed");
	}
	if (!lunatico_apply_motor_type(device, motor_type)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_apply_motor_type() failed");
	}
	if (!lunatico_apply_step_mode(device, step_mode)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_apply_step_mode() failed");
	}
}

// --------------------------------------------------------------------------------- Focuser device

static void lunatico_focuser_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = state;
	FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

// Bounded progress check of a focuser move. It reschedules itself while the
// controller reports motion and publishes the outcome once, so no handler ever
// waits for the motor.
static void focuser_motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	bool moving = false;
	int32_t position = 0;
	if (!lunatico_is_moving(device, &moving)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_is_moving() failed");
		lunatico_focuser_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
		lunatico_focuser_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	PORT_STATE.focuser_position = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	if (!moving || position == PORT_STATE.focuser_target) {
		lunatico_focuser_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	lunatico_focuser_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
}

// Start an absolute move to an already clamped target. Returns true when the
// move started and its completion still has to be polled.
static bool lunatico_focuser_start_move(indigo_device *device, int32_t target, int32_t backlash) {
	PORT_STATE.focuser_target = target;
	FOCUSER_POSITION_ITEM->number.value = PORT_STATE.focuser_position;
	lunatico_focuser_motion_state(device, INDIGO_BUSY_STATE);
	if (!lunatico_goto_position(device, target, backlash)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_goto_position(%d, %d) failed", target, backlash);
		// The move never started, so it must not be polled: the poll would find
		// the focuser idle at the unchanged position and overwrite the failure.
		lunatico_focuser_motion_state(device, INDIGO_ALERT_STATE);
		return false;
	}
	return true;
}

static int32_t lunatico_focuser_clamp(indigo_device *device, double target) {
	if (target > FOCUSER_POSITION_ITEM->number.max) {
		return (int32_t)FOCUSER_POSITION_ITEM->number.max;
	}
	if (target < FOCUSER_POSITION_ITEM->number.min) {
		return (int32_t)FOCUSER_POSITION_ITEM->number.min;
	}
	return (int32_t)target;
}

// FOCUSER_POSITION either starts a move or, in SYNC mode, writes the
// controller's position counter and reads it back. Returns true when a move
// started and still has to be polled.
static bool lunatico_focuser_position(indigo_device *device) {
	int32_t target = (int32_t)FOCUSER_POSITION_ITEM->number.target;
	if (target == PORT_STATE.focuser_position) {
		lunatico_focuser_motion_state(device, INDIGO_OK_STATE);
		return false;
	}
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		return lunatico_focuser_start_move(device, target, (int32_t)FOCUSER_BACKLASH_ITEM->number.value);
	}
	indigo_property_state state = INDIGO_OK_STATE;
	if (!lunatico_sync_position(device, target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_sync_position(%d) failed", target);
		state = INDIGO_ALERT_STATE;
	}
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
		state = INDIGO_ALERT_STATE;
	} else {
		PORT_STATE.focuser_position = position;
		FOCUSER_POSITION_ITEM->number.value = position;
	}
	lunatico_focuser_motion_state(device, state);
	return false;
}

// A relative move is issued as an absolute move to a target derived from the
// measured position, with no backlash compensation. Returns true when the move
// started and still has to be polled.
static bool lunatico_focuser_steps(indigo_device *device) {
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
	} else {
		PORT_STATE.focuser_position = position;
	}
	double steps = FOCUSER_STEPS_ITEM->number.value;
	double target = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? PORT_STATE.focuser_position - steps : PORT_STATE.focuser_position + steps;
	return lunatico_focuser_start_move(device, lunatico_focuser_clamp(device, target), 0);
}

static void lunatico_focuser_abort(indigo_device *device) {
	indigo_property_state state = INDIGO_OK_STATE;
	if (!lunatico_stop(device)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_stop() failed");
		state = INDIGO_ALERT_STATE;
	}
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
		state = INDIGO_ALERT_STATE;
	} else {
		PORT_STATE.focuser_position = position;
	}
	FOCUSER_POSITION_ITEM->number.value = PORT_STATE.focuser_position;
	lunatico_focuser_motion_state(device, INDIGO_OK_STATE);
	// The generated handler suppresses its epilogue because this block schedules
	// a finalizer, so the abort publishes its own outcome.
	FOCUSER_ABORT_MOTION_PROPERTY->state = state;
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

// FOCUSER_LIMITS are the controller's software limits; the full range removes
// them instead of writing a degenerate one.
static void lunatico_focuser_limits(indigo_device *device) {
	int32_t min = (int32_t)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;
	int32_t max = (int32_t)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	if (max < min) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, "Minimum value can not be bigger then maximum");
		return;
	}
	bool result;
	if (FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target == FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max && FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target == FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min) {
		result = lunatico_delete_limits(device);
	} else {
		result = lunatico_set_limits(device, min, max);
	}
	if (!result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits() failed");
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
}

// Temperature compensation. It is driven by the temperature poll and only moves
// the focuser when the reading is valid, the focuser is idle and the change is
// at least one degree.
static void lunatico_focuser_compensate(indigo_device *device, double temperature) {
	if (PORT_STATE.previous_temperature <= NO_TEMP_READING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: no previous temperature");
		PORT_STATE.previous_temperature = temperature;
		return;
	}
	if (temperature <= NO_TEMP_READING || FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: temperature = %f, FOCUSER_POSITION state = %d", temperature, FOCUSER_POSITION_PROPERTY->state);
		return;
	}
	double difference = temperature - PORT_STATE.previous_temperature;
	if (fabs(difference) < 1.0 || fabs(difference) >= 100) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: difference = %f", difference);
		return;
	}
	int compensation = (int)(difference * FOCUSER_COMPENSATION_ITEM->number.value);
	int32_t target = lunatico_focuser_clamp(device, PORT_STATE.focuser_position - compensation);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: difference = %.2f, compensation = %d, target = %d", difference, compensation, target);
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
	} else {
		PORT_STATE.focuser_position = position;
	}
	PORT_STATE.previous_temperature = temperature;
	if (lunatico_focuser_start_move(device, target, (int32_t)FOCUSER_BACKLASH_ITEM->number.value)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
	}
}

// The focuser temperature poll. A reading at or below NO_TEMP_READING means no
// sensor is connected, which is reported as IDLE and announced once.
static void lunatico_focuser_poll_temperature(indigo_device *device) {
	double temperature = NO_TEMP_READING;
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	if (!lunatico_get_temperature(device, PORT_STATE.temperature_sensor, &temperature)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_temperature() failed");
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
	}
	if (FOCUSER_TEMPERATURE_ITEM->number.value <= NO_TEMP_READING) {
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (PORT_STATE.has_temperature_sensor) {
			PORT_STATE.has_temperature_sensor = false;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected.");
		}
	} else {
		PORT_STATE.has_temperature_sensor = true;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		lunatico_focuser_compensate(device, temperature);
	} else {
		// Reset the reference so compensation starts from the reading that
		// follows the switch to automatic mode.
		PORT_STATE.previous_temperature = NO_TEMP_READING;
	}
}

// FOCUSER_MODE takes the manual controls away in automatic mode and makes the
// position read-only.
static void lunatico_focuser_mode(indigo_device *device) {
	if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
		indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else {
		indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
}

// The focuser connection sequence, in the order the original driver used.
static bool lunatico_focuser_connect(indigo_device *device, indigo_property *power_control, indigo_property *motor_type, indigo_property *step_mode, indigo_property *wiring, indigo_property *temperature_sensor) {
	if (!lunatico_claim_port(device)) {
		return false;
	}
	lunatico_publish_identity(device);
	lunatico_apply_temperature_sensor(device, temperature_sensor);
	lunatico_configure_port(device, power_control, motor_type, step_mode);
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
	}
	PORT_STATE.focuser_position = PORT_STATE.focuser_target = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	if (!lunatico_set_speed(device, FOCUSER_SPEED_ITEM->number.target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_speed() failed");
	}
	if (!lunatico_apply_wiring(device, wiring, !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_apply_wiring() failed");
	}
	bool result;
	if (FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value == FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max && FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value == FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min) {
		result = lunatico_delete_limits(device);
	} else {
		result = lunatico_set_limits(device, (int32_t)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value, (int32_t)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value);
	}
	if (!result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits() failed");
	}
	double temperature = NO_TEMP_READING;
	if (lunatico_get_temperature(device, 0, &temperature)) {
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
	}
	PORT_STATE.previous_temperature = FOCUSER_TEMPERATURE_ITEM->number.value;
	PORT_STATE.has_temperature_sensor = true;
	// The original driver published the initial position from a one-shot poll
	// scheduled 0.5 s after the connection; the same poll is kept so an axis
	// that is already moving when the driver connects is still followed.
	indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
	return true;
}

// --------------------------------------------------------------------------------- Rotator device

static int lunatico_degrees_to_steps(double degrees, int steps_per_revolution, double minimum) {
	double deg = degrees;
	while (deg >= (360 - minimum)) {
		deg -= 360;
	}
	deg -= minimum;
	int steps = (int)(deg * steps_per_revolution / 360.0);
	while (steps < 0) {
		steps += steps_per_revolution;
	}
	while (steps >= steps_per_revolution) {
		steps -= steps_per_revolution;
	}
	return steps;
}

static double lunatico_steps_to_degrees(int steps, int steps_per_revolution, double minimum) {
	if (steps_per_revolution == 0) {
		return 0;
	}
	int st = steps;
	while (st >= steps_per_revolution) {
		st -= steps_per_revolution;
	}
	st += (int)(steps_per_revolution * minimum / 360);
	double degrees = st * 360.0 / steps_per_revolution;
	while (degrees < 0) {
		degrees += 360;
	}
	while (degrees >= 360) {
		degrees -= 360;
	}
	return degrees;
}

static int lunatico_rotator_steps(indigo_device *device, double degrees) {
	return lunatico_degrees_to_steps(degrees, (int)ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value, ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value);
}

static double lunatico_rotator_degrees(indigo_device *device, int steps) {
	return lunatico_steps_to_degrees(steps, (int)ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value, ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value);
}

// Both the limits and the steps per revolution change the degree to step
// mapping, so the controller's counter has to be re-synced to the angle the
// driver is showing.
static void lunatico_rotator_resync(indigo_device *device) {
	if (!lunatico_sync_position(device, lunatico_rotator_steps(device, ROTATOR_POSITION_ITEM->number.value))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_sync_position() failed");
	}
}

static void rotator_motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	bool moving = false;
	int32_t position = 0;
	if (!lunatico_is_moving(device, &moving)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_is_moving() failed");
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		return;
	}
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		return;
	}
	PORT_STATE.rotator_position = lunatico_rotator_degrees(device, position);
	ROTATOR_POSITION_ITEM->number.value = PORT_STATE.rotator_position;
	ROTATOR_POSITION_PROPERTY->state = !moving || PORT_STATE.rotator_position == PORT_STATE.rotator_target ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, rotator_motion_finalizer);
	}
}

// Returns true when a move started and its completion still has to be polled.
static bool lunatico_rotator_position(indigo_device *device) {
	double current = PORT_STATE.rotator_position;
	int minimum = lunatico_rotator_steps(device, ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value);
	int maximum = lunatico_rotator_steps(device, ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value);
	int target = lunatico_rotator_steps(device, ROTATOR_POSITION_ITEM->number.target);
	if (minimum != maximum && (target > maximum || target < minimum)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		ROTATOR_POSITION_ITEM->number.value = current;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Requested position is not in the limits.");
		return false;
	}
	if (ROTATOR_POSITION_ITEM->number.target == current) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		return false;
	}
	if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		PORT_STATE.rotator_target = ROTATOR_POSITION_ITEM->number.target;
		ROTATOR_POSITION_ITEM->number.value = current;
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		if (!lunatico_goto_position(device, target, (int32_t)ROTATOR_BACKLASH_ITEM->number.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_goto_position(%d) failed", target);
			// The move never started, so it must not be polled.
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			return false;
		}
		return true;
	}
	indigo_property_state state = INDIGO_OK_STATE;
	if (!lunatico_sync_position(device, target)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_sync_position(%d) failed", target);
		state = INDIGO_ALERT_STATE;
	}
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
		state = INDIGO_ALERT_STATE;
	} else {
		PORT_STATE.rotator_position = lunatico_rotator_degrees(device, position);
		ROTATOR_POSITION_ITEM->number.value = PORT_STATE.rotator_position;
	}
	ROTATOR_POSITION_PROPERTY->state = state;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	return false;
}

static void lunatico_rotator_abort(indigo_device *device) {
	indigo_property_state state = INDIGO_OK_STATE;
	if (!lunatico_stop(device)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_stop() failed");
		state = INDIGO_ALERT_STATE;
	}
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
		state = INDIGO_ALERT_STATE;
	} else {
		PORT_STATE.rotator_position = lunatico_rotator_degrees(device, position);
	}
	ROTATOR_POSITION_ITEM->number.value = PORT_STATE.rotator_position;
	ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	// The generated handler suppresses its epilogue because this block schedules
	// a finalizer, so the abort publishes its own outcome.
	ROTATOR_ABORT_MOTION_PROPERTY->state = state;
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
}

static void lunatico_rotator_limits(indigo_device *device) {
	int minimum = lunatico_rotator_steps(device, ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value);
	int maximum = lunatico_rotator_steps(device, ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value);
	bool result;
	if (minimum == maximum) {
		result = lunatico_delete_limits(device);
	} else {
		result = lunatico_set_limits(device, minimum, maximum);
	}
	if (!result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits() failed");
		ROTATOR_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	lunatico_rotator_resync(device);
}

static bool lunatico_rotator_connect(indigo_device *device, indigo_property *power_control, indigo_property *motor_type, indigo_property *step_mode, indigo_property *wiring) {
	if (!lunatico_claim_port(device)) {
		return false;
	}
	lunatico_publish_identity(device);
	lunatico_configure_port(device, power_control, motor_type, step_mode);
	int32_t position = 0;
	if (!lunatico_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position() failed");
	}
	// The current angle has to follow what the controller reports: the position
	// handler skips a request that equals it, so a stale 0 here would silently
	// swallow the first request for 0 deg.
	PORT_STATE.rotator_position = PORT_STATE.rotator_target = lunatico_rotator_degrees(device, position);
	ROTATOR_POSITION_ITEM->number.value = PORT_STATE.rotator_position;
	lunatico_rotator_resync(device);
	if (!lunatico_set_speed(device, 0.1)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_speed() failed");
	}
	if (!lunatico_apply_wiring(device, wiring, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_apply_wiring() failed");
	}
	int minimum = lunatico_rotator_steps(device, ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value);
	int maximum = lunatico_rotator_steps(device, ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value);
	bool result;
	if (minimum == maximum) {
		result = lunatico_delete_limits(device);
	} else {
		result = lunatico_set_limits(device, minimum, maximum);
	}
	if (!result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits() failed");
	}
	indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, rotator_motion_finalizer);
	return true;
}

// --------------------------------------------------------------------------------- Powerbox device

// The DB9 pins are horizontally flipped on the devices with a female connector,
// so they are reversed here and the user sees outlet 1 on pin 1 of the male
// pinout: outlet item 1 writes pin 4 and sensor item 1 reads pin 8.
static bool lunatico_apply_outlets(indigo_device *device, indigo_property *outlets) {
	bool result = true;
	for (int i = 0; i < outlets->count; i++) {
		if (!lunatico_enable_power_outlet(device, 4 - i, (outlets->items + i)->sw.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_enable_power_outlet(%d) failed", 4 - i);
			result = false;
		}
	}
	return result;
}

static void lunatico_poll_sensors(indigo_device *device, indigo_property *sensors) {
	sensors->state = INDIGO_OK_STATE;
	for (int i = 0; i < sensors->count; i++) {
		int32_t value = 0;
		if (!lunatico_read_sensor(device, 8 - i, &value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_sensor(%d) failed", 8 - i);
			sensors->state = INDIGO_ALERT_STATE;
			break;
		}
		(sensors->items + i)->number.value = value;
	}
	indigo_update_property(device, sensors, NULL);
}

// Renaming an outlet or a sensor relabels the corresponding item, so the
// property has to be redefined for the new labels to reach the client.
static void lunatico_apply_names(indigo_device *device, indigo_property *names, indigo_property *target) {
	if (IS_CONNECTED) {
		indigo_delete_property(device, target, NULL);
	}
	for (int i = 0; i < target->count && i < names->count; i++) {
		snprintf((target->items + i)->label, INDIGO_NAME_SIZE, "%s", (names->items + i)->text.value);
	}
	if (IS_CONNECTED) {
		indigo_define_property(device, target, NULL);
	}
}

static bool lunatico_aux_connect(indigo_device *device, indigo_property *outlets) {
	if (!lunatico_claim_port(device)) {
		return false;
	}
	lunatico_publish_identity(device);
	lunatico_apply_outlets(device, outlets);
	return true;
}
