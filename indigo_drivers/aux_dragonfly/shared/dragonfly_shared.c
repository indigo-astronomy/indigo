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

/** INDIGO Lunatico Dragonfly shared implementation
 \file dragonfly_shared.c
 */

// Transport, identity, relay and sensor implementation shared by
// indigo_aux_dragonfly and indigo_dome_dragonfly. Both .driver definitions
// include this file from their code { } block, so it is emitted into the
// generated low level code section where the private data type and the
// property macros are already known.
//
// The AUX relay device of the two drivers exposes a different window of the
// controller's eight channels, so the shared code is parameterised by
// RELAY_FIRST / RELAY_COUNT and SENSOR_FIRST / SENSOR_COUNT, which each
// definition declares in its define { } block.

// --------------------------------------------------------------------------------- Transport

// Send one SLP request and read its reply datagram. A reply is always exactly
// one datagram, so it is read with a single indigo_uni_read_available(); the
// delimited readers cannot be used here because a read on a UDP handle
// consumes the whole datagram no matter how few bytes were requested.
static bool dragonfly_vcommand(indigo_device *device, const char *format, va_list args) {
	vsnprintf(PRIVATE_DATA->command, sizeof(PRIVATE_DATA->command), format, args);
	*PRIVATE_DATA->response = 0;
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		// The request is already formatted and must go out as one datagram.
		result = indigo_uni_write(PRIVATE_DATA->handle, PRIVATE_DATA->command, (long)strlen(PRIVATE_DATA->command));
	}
	if (result > 0) {
		result = indigo_uni_wait_for_data(PRIVATE_DATA->handle, DRAGONFLY_REPLY_TIMEOUT);
	}
	if (result > 0) {
		result = indigo_uni_read_available(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1);
		if (result > 0) {
			PRIVATE_DATA->response[result] = 0;
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
static bool dragonfly_parse_value(indigo_device *device, int32_t *value) {
	char format[DRAGONFLY_CMD_LEN + 8];
	int length = (int)strlen(PRIVATE_DATA->command);
	snprintf(format, sizeof(format), "%.*s:%%d#", length > 0 ? length - 1 : 0, PRIVATE_DATA->command);
	if (sscanf(PRIVATE_DATA->response, format, value) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' -> '%s' is not a valid reply", PRIVATE_DATA->command, PRIVATE_DATA->response);
		return false;
	}
	return true;
}

static bool dragonfly_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = dragonfly_vcommand(device, format, args);
	va_end(args);
	return result;
}

static bool dragonfly_command_value(indigo_device *device, int32_t *value, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = dragonfly_vcommand(device, format, args);
	va_end(args);
	return result && dragonfly_parse_value(device, value);
}

// A negative result code means the controller rejected the command, for
// example because the earned access level is too low.
static bool dragonfly_command_ok(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = dragonfly_vcommand(device, format, args);
	va_end(args);
	int32_t value = -1;
	return result && dragonfly_parse_value(device, &value) && value >= 0;
}

// --------------------------------------------------------------------------------- Device commands

// !seletek version:<MOPFF># where M is the operating mode, O the model,
// P the firmware major and FF the firmware minor version.
static bool dragonfly_identify(indigo_device *device) {
	static const char *models[] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet" };
	int32_t value = 0;
	if (!dragonfly_command_value(device, &value, "!seletek version#")) {
		return false;
	}
	int model = (value / 1000) % 10;
	if (model > 5 || model < 0) {
		model = 0;
	}
	snprintf(PRIVATE_DATA->board, sizeof(PRIVATE_DATA->board), "%s", models[model]);
	snprintf(PRIVATE_DATA->firmware, sizeof(PRIVATE_DATA->firmware), "%d.%d", (value / 100) % 10, value % 100);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' -> %s %s", PRIVATE_DATA->response, PRIVATE_DATA->board, PRIVATE_DATA->firmware);
	return true;
}

// Authentication expires about 30 s after the last command, so the dome device
// keeps it alive with an echo. The relay-only driver does not need it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static bool dragonfly_keep_alive(indigo_device *device) {
	return dragonfly_command_ok(device, "!seletek echo#");
}

#pragma GCC diagnostic pop

// A NULL password queries the current access level, a non-empty password earns
// a new one. An empty password is not sent at all.
static bool dragonfly_authenticate(indigo_device *device, const char *password) {
	static const char *levels[] = { "Unknown", "Read only", "Read / Write", "Full access" };
	if (password != NULL && *password == 0) {
		return false;
	}
	int32_t level = 0;
	bool result = password != NULL ? dragonfly_command_value(device, &level, "!aux earnaccess %s#", password) : dragonfly_command_value(device, &level, "!aux earnaccess#");
	if (!result || level < 0) {
		level = 0;
		result = false;
	}
	indigo_send_message(device, IDLE_PROPERTY, "Earned access level: %d (%s)", level, level >= 1 && level <= 3 ? levels[level] : levels[0]);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Earned access: %d", level);
	return result;
}

static bool dragonfly_read_sensors(indigo_device *device, int *sensors) {
	if (!dragonfly_command(device, "!relio snanrd 0 0 7#")) {
		return false;
	}
	if (sscanf(PRIVATE_DATA->response, "!relio snanrd 0 0 7:%d,%d,%d,%d,%d,%d,%d,%d#", sensors, sensors + 1, sensors + 2, sensors + 3, sensors + 4, sensors + 5, sensors + 6, sensors + 7) != 8) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' is not a valid sensor reply", PRIVATE_DATA->response);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "sensors = %d %d %d %d %d %d %d %d", sensors[0], sensors[1], sensors[2], sensors[3], sensors[4], sensors[5], sensors[6], sensors[7]);
	return true;
}

static bool dragonfly_read_relays(indigo_device *device, bool *relays) {
	int values[DRAGONFLY_CHANNELS];
	if (!dragonfly_command(device, "!relio rldgrd 0 0 7#")) {
		return false;
	}
	if (sscanf(PRIVATE_DATA->response, "!relio rldgrd 0 0 7:%d,%d,%d,%d,%d,%d,%d,%d#", values, values + 1, values + 2, values + 3, values + 4, values + 5, values + 6, values + 7) != 8) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' is not a valid relay reply", PRIVATE_DATA->response);
		return false;
	}
	for (int i = 0; i < DRAGONFLY_CHANNELS; i++) {
		relays[i] = values[i] != 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "relays = %d %d %d %d %d %d %d %d", relays[0], relays[1], relays[2], relays[3], relays[4], relays[5], relays[6], relays[7]);
	return true;
}

static bool dragonfly_set_relay(indigo_device *device, int relay, bool enabled) {
	if (relay < 0 || relay >= DRAGONFLY_CHANNELS) {
		return false;
	}
	return dragonfly_command_ok(device, "!relio rlset 0 %d %d#", relay, enabled ? 1 : 0);
}

static bool dragonfly_pulse_relay(indigo_device *device, int relay, int length) {
	if (relay < 0 || relay >= DRAGONFLY_CHANNELS) {
		return false;
	}
	return dragonfly_command_ok(device, "!relio rlpulse 0 %d %d#", relay, length);
}

// --------------------------------------------------------------------------------- Connection

static bool dragonfly_open(indigo_device *device) {
	char url[INDIGO_VALUE_SIZE];
	if (strstr(DEVICE_PORT_ITEM->text.value, "://")) {
		INDIGO_COPY_VALUE(url, DEVICE_PORT_ITEM->text.value);
	} else {
		snprintf(url, sizeof(url), "udp://%s", DEVICE_PORT_ITEM->text.value);
	}
	PRIVATE_DATA->handle = indigo_uni_open_url(url, DRAGONFLY_UDP_PORT, INDIGO_UDP_HANDLE, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (dragonfly_identify(device) && !strcmp(PRIVATE_DATA->board, "Dragonfly")) {
			return true;
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' is not a Dragonfly controller", url);
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void dragonfly_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

// --------------------------------------------------------------------------------- Relay device

// Publish this device's window of the controller's analog sensor inputs.
static void dragonfly_update_sensors(indigo_device *device) {
	int sensors[DRAGONFLY_CHANNELS];
	if (dragonfly_read_sensors(device, sensors)) {
		for (int i = 0; i < SENSOR_COUNT; i++) {
			(AUX_GPIO_SENSORS_PROPERTY->items + i)->number.value = sensors[i + SENSOR_FIRST];
		}
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
}

// Remaining time to the earliest unfinished relay pulse, -1 when none runs.
static double dragonfly_pulse_delay(indigo_device *device) {
	double now = indigo_monotonic_time();
	double delay = -1;
	for (int i = 0; i < RELAY_COUNT; i++) {
		if (PRIVATE_DATA->relay_pulse_until[i] > 0) {
			double remaining = PRIVATE_DATA->relay_pulse_until[i] - now;
			if (remaining < 0) {
				remaining = 0;
			}
			if (delay < 0 || remaining < delay) {
				delay = remaining;
			}
		}
	}
	return delay;
}

// One finalizer serves all relays of the device: it clears every outlet whose
// pulse has elapsed and rearms itself for the next deadline.
static void relay_pulse_finalizer(indigo_device *device) {
	double now = indigo_monotonic_time();
	bool updated = false;
	for (int i = 0; i < RELAY_COUNT; i++) {
		if (PRIVATE_DATA->relay_pulse_until[i] > 0 && now >= PRIVATE_DATA->relay_pulse_until[i]) {
			PRIVATE_DATA->relay_pulse_until[i] = 0;
			(AUX_GPIO_OUTLETS_PROPERTY->items + i)->sw.value = false;
			updated = true;
		}
	}
	if (updated) {
		indigo_update_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
	}
	double delay = dragonfly_pulse_delay(device);
	if (delay >= 0) {
		indigo_execute_handler_in(device, delay, relay_pulse_finalizer);
	}
}

// Write only the channels whose requested state differs from the controller's
// current state. A channel with a non-zero pulse length is pulsed when it is
// switched on and no pulse is running for it; otherwise it is switched.
static bool dragonfly_set_outlets(indigo_device *device) {
	bool relays[DRAGONFLY_CHANNELS];
	if (!dragonfly_read_relays(device, relays)) {
		return false;
	}
	bool result = true;
	for (int i = 0; i < RELAY_COUNT; i++) {
		indigo_item *outlet = AUX_GPIO_OUTLETS_PROPERTY->items + i;
		double length = (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value;
		bool pulse_running = PRIVATE_DATA->relay_pulse_until[i] > 0;
		if (outlet->sw.value == relays[i + RELAY_FIRST]) {
			continue;
		}
		if (length > 0 && outlet->sw.value && !pulse_running) {
			if (dragonfly_pulse_relay(device, i + RELAY_FIRST, (int)length)) {
				PRIVATE_DATA->relay_pulse_until[i] = indigo_monotonic_time() + (length + DRAGONFLY_PULSE_MARGIN_MS) / 1000;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Relay %d pulse failed, did you authorize?", i + RELAY_FIRST + 1);
				result = false;
			}
		} else if (length == 0 || (!outlet->sw.value && !pulse_running)) {
			if (!dragonfly_set_relay(device, i + RELAY_FIRST, outlet->sw.value)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Relay %d write failed, did you authorize?", i + RELAY_FIRST + 1);
				result = false;
			}
		}
	}
	return result;
}

// Outlet and sensor names are user editable, so the dependent properties carry
// them as item labels and have to be republished when a name changes.
static void dragonfly_apply_outlet_names(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
	}
	for (int i = 0; i < RELAY_COUNT; i++) {
		const char *name = (AUX_OUTLET_NAMES_PROPERTY->items + i)->text.value;
		INDIGO_COPY_VALUE((AUX_GPIO_OUTLETS_PROPERTY->items + i)->label, name);
		INDIGO_COPY_VALUE((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->label, name);
	}
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
		indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
	}
}

static void dragonfly_apply_sensor_names(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	for (int i = 0; i < SENSOR_COUNT; i++) {
		INDIGO_COPY_VALUE((AUX_GPIO_SENSORS_PROPERTY->items + i)->label, (AUX_SENSOR_NAMES_PROPERTY->items + i)->text.value);
	}
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
}

// Seed the outlet items from the controller and publish the identity the
// shared connection has already read. Used by the on_connect block of the
// relay device of both drivers.
static void dragonfly_attach_relays(indigo_device *device) {
	bool relays[DRAGONFLY_CHANNELS];
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->board);
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
	indigo_update_property(device, INFO_PROPERTY, NULL);
	for (int i = 0; i < RELAY_COUNT; i++) {
		PRIVATE_DATA->relay_pulse_until[i] = 0;
	}
	if (dragonfly_read_relays(device, relays)) {
		for (int i = 0; i < RELAY_COUNT; i++) {
			(AUX_GPIO_OUTLETS_PROPERTY->items + i)->sw.value = relays[i + RELAY_FIRST];
		}
		AUX_GPIO_OUTLETS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_GPIO_OUTLETS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	dragonfly_authenticate(device, AUTHENTICATION_PASSWORD_ITEM->text.value);
}
