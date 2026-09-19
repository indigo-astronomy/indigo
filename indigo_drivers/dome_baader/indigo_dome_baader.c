// Copyright (c) 2020-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_dome_baader.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_baader.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_dome_baader"
#define DRIVER_LABEL         "Baader Classic Dome"
#define DOME_DEVICE_NAME     "Baader Classic Dome"
#define PRIVATE_DATA         ((baader_private_data *)device->private_data)

//+ define

#define BAADER_FRAME_LEN     9
#define BAADER_FIRST_BYTE_TIMEOUT 3.1
#define BAADER_NEXT_BYTE_TIMEOUT 0.1
#define BAADER_NETWORK_PORT  8080
#define BAADER_FIRST_POLL_DELAY 0.5
#define BAADER_POLL_DELAY    1
#define BAADER_PARK_AZIMUTH  0
#define BAADER_POSITION_UNKNOWN (-1)

//- define

#pragma mark - Property definitions

#define X_EMERGENCY_CLOSE_PROPERTY              (PRIVATE_DATA->x_emergency_close_property)
#define X_EMERGENCY_RAIN_ITEM                   (X_EMERGENCY_CLOSE_PROPERTY->items + 0)
#define X_EMERGENCY_WIND_ITEM                   (X_EMERGENCY_CLOSE_PROPERTY->items + 1)
#define X_EMERGENCY_OPERATION_TIMEOUT_ITEM      (X_EMERGENCY_CLOSE_PROPERTY->items + 2)
#define X_EMERGENCY_POWERCUT_ITEM               (X_EMERGENCY_CLOSE_PROPERTY->items + 3)

#define X_EMERGENCY_CLOSE_PROPERTY_NAME         "X_EMERGENCY_CLOSE"
#define X_EMERGENCY_RAIN_ITEM_NAME              "RAIN"
#define X_EMERGENCY_WIND_ITEM_NAME              "WIND"
#define X_EMERGENCY_OPERATION_TIMEOUT_ITEM_NAME "OPERATION_TIMEOUT"
#define X_EMERGENCY_POWERCUT_ITEM_NAME          "POWER_CUT"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_emergency_close_property;
	//+ data
	char response[16];
	double target_position, current_position;
	int shutter_position, flap_state;
	bool rain, wind, timeout, powercut;
	bool park_requested, aborted;
	bool rotation_active, shutter_active, flap_active;
	int rotation_emergency, shutter_emergency, flap_emergency;
	bool shutter_alert, flap_alert;
	//- data
} baader_private_data;

#pragma mark - Low level code

//+ code

typedef enum {
	FLAP_CLOSED = 0,
	FLAP_OPEN = 1,
	FLAP_STOPPED = 2,
	FLAP_CLOSING = 3,
	FLAP_OPENING = 4,
	FLAP_MOVING = 5 // Old firmware uses it instead of FLAP_STOPPED, FLAP_CLOSING and FLAP_OPENING
} baader_flap_state_t;

typedef enum {
	BD_SUCCESS = 0,
	BD_PARAM_ERROR = 1,
	BD_COMMAND_ERROR = 2,
	BD_NO_RESPONSE = 3,
	BD_DOME_ERROR = 4
} baader_rc_t;

static void dome_horizontal_coordinates_handler(indigo_device *device);
static void dome_steps_handler(indigo_device *device);
static void dome_park_handler(indigo_device *device);
static void dome_shutter_handler(indigo_device *device);
static void dome_flap_handler(indigo_device *device);

// every command and every reply is a 9-byte frame without terminator
static bool baader_vcommand(indigo_device *device, const char *format, va_list args) {
	PRIVATE_DATA->response[0] = 0;
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_vprintf(PRIVATE_DATA->handle, format, args) != BAADER_FRAME_LEN) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write command %s", format);
		return false;
	}
	indigo_usleep(100);
	long length = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, BAADER_FRAME_LEN, "", "", INDIGO_DELAY(BAADER_FIRST_BYTE_TIMEOUT), INDIGO_DELAY(BAADER_NEXT_BYTE_TIMEOUT));
	if (length != BAADER_FRAME_LEN || strncmp(PRIVATE_DATA->response, "d#", 2)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command %s: invalid reply '%s'", format, length > 0 ? PRIVATE_DATA->response : "");
		PRIVATE_DATA->response[0] = 0;
		return false;
	}
	return true;
}

static bool baader_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = baader_vcommand(device, format, args);
	va_end(args);
	return result;
}

static bool baader_digits(const char *text, int count, int *value) {
	int result = 0;
	for (int i = 0; i < count; i++) {
		if (text[i] < '0' || text[i] > '9') {
			return false;
		}
		result = result * 10 + text[i] - '0';
	}
	*value = result;
	return true;
}

static baader_rc_t baader_command_ok(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = baader_vcommand(device, format, args);
	va_end(args);
	if (!result) {
		return BD_NO_RESPONSE;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#gotmess")) {
		return BD_SUCCESS;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#domerro")) {
		return BD_DOME_ERROR;
	}
	return BD_COMMAND_ERROR;
}

static baader_rc_t baader_get_serial_number(indigo_device *device, char *serial_num) {
	if (!baader_command(device, "d#ser_num")) {
		return BD_NO_RESPONSE;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#domerro")) {
		return BD_DOME_ERROR;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#comerro") || !strcmp(PRIVATE_DATA->response, "d#gotmess") || !strcmp(PRIVATE_DATA->response, "d#err_sht")) {
		return BD_COMMAND_ERROR;
	}
	strcpy(serial_num, PRIVATE_DATA->response + 2);
	return BD_SUCCESS;
}

static baader_rc_t baader_get_azimuth(indigo_device *device, double *azimuth) {
	int tenths;
	if (!baader_command(device, "d#getazim")) {
		return BD_NO_RESPONSE;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#domerro")) {
		return BD_DOME_ERROR;
	}
	// "d#azi" while rotating, old firmware reports "d#azr" when stopped
	if (strncmp(PRIVATE_DATA->response, "d#az", 4) || !baader_digits(PRIVATE_DATA->response + 5, 4, &tenths) || tenths > 3600) {
		return BD_COMMAND_ERROR;
	}
	*azimuth = (tenths % 3600) / 10.0;
	return BD_SUCCESS;
}

// azimuths are compared in whole tenths of degree, the resolution of the protocol
static int baader_tenths(double azimuth) {
	return (int)((lround(azimuth * 10) % 3600 + 3600) % 3600);
}

static baader_rc_t baader_goto_azimuth(indigo_device *device, double azimuth) {
	return baader_command_ok(device, "d#azi%04d", baader_tenths(azimuth));
}

static baader_rc_t baader_get_shutter_position(indigo_device *device, int *position) {
	if (!baader_command(device, "d#getshut")) {
		return BD_NO_RESPONSE;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#domerro")) {
		return BD_DOME_ERROR;
	} else if (!strcmp(PRIVATE_DATA->response, "d#shutope")) {
		*position = 100;
	} else if (!strcmp(PRIVATE_DATA->response, "d#shutclo")) {
		*position = 0;
	} else if (!strcmp(PRIVATE_DATA->response, "d#shutrun")) {
		*position = 50;
	} else if (strncmp(PRIVATE_DATA->response, "d#shut_", 7) || !baader_digits(PRIVATE_DATA->response + 7, 2, position)) {
		return BD_COMMAND_ERROR;
	}
	return BD_SUCCESS;
}

static baader_rc_t baader_get_flap_state(indigo_device *device, int *flap_state) {
	static const struct {
		const char *reply;
		baader_flap_state_t state;
	} states[] = {
		{ "d#flapope", FLAP_OPEN },
		{ "d#flapclo", FLAP_CLOSED },
		{ "d#flaprim", FLAP_STOPPED },
		{ "d#flaprop", FLAP_OPENING },
		{ "d#flaprcl", FLAP_CLOSING },
		{ "d#flaprun", FLAP_MOVING }
	};
	if (!baader_command(device, "d#getflap")) {
		return BD_NO_RESPONSE;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#domerro")) {
		return BD_DOME_ERROR;
	}
	for (int i = 0; i < (int)(sizeof(states) / sizeof(states[0])); i++) {
		if (!strcmp(PRIVATE_DATA->response, states[i].reply)) {
			*flap_state = states[i].state;
			return BD_SUCCESS;
		}
	}
	return BD_COMMAND_ERROR;
}

static baader_rc_t baader_get_emergency_status(indigo_device *device, bool *rain, bool *wind, bool *timeout, bool *powercut) {
	if (!baader_command(device, "d#get_eme")) {
		return BD_NO_RESPONSE;
	}
	if (!strcmp(PRIVATE_DATA->response, "d#domerro")) {
		return BD_DOME_ERROR;
	}
	const char *flags = PRIVATE_DATA->response + 5;
	if (strncmp(PRIVATE_DATA->response, "d#eme", 5) || strspn(flags, "01") != 4) {
		return BD_COMMAND_ERROR;
	}
	*rain = flags[0] == '1';
	*wind = flags[1] == '1';
	*timeout = flags[2] == '1';
	*powercut = flags[3] == '1';
	return BD_SUCCESS;
}

// emergency flags as a bit mask: rain, wind, operation timeout, power cut
static int baader_emergency_flags(bool rain, bool wind, bool timeout, bool powercut) {
	return (rain ? 1 : 0) | (wind ? 2 : 0) | (timeout ? 4 : 0) | (powercut ? 8 : 0);
}

static int baader_known_emergency(indigo_device *device) {
	return baader_emergency_flags(PRIVATE_DATA->rain, PRIVATE_DATA->wind, PRIVATE_DATA->timeout, PRIVATE_DATA->powercut);
}

static void baader_emergency_message(int flags, char *message, int size) {
	static const char *names[] = { "rain", "wind", "operation timeout", "power cut" };
	int length = snprintf(message, size, "Emergency close:");
	for (int i = 0; i < 4 && length < size; i++) {
		if (flags & (1 << i)) {
			length += snprintf(message + length, size - length, "%s %s", length > 16 ? "," : "", names[i]);
		}
	}
}

static bool baader_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (indigo_uni_is_url(name, "baader")) {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, BAADER_NETWORK_PORT, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle != NULL) {
		char serial_number[INDIGO_VALUE_SIZE] = "N/A";
		baader_rc_t rc = baader_get_serial_number(device, serial_number);
		if (rc == BD_SUCCESS) {
			INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, serial_number);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with serial No.%s connected", INFO_DEVICE_MODEL_ITEM->text.value, serial_number);
			return true;
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Connect failed: Baader dome did not respond (%d)", rc);
		indigo_uni_close(&PRIVATE_DATA->handle);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", name);
	}
	return false;
}

static void baader_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

//- code

//+ dome.code

static void baader_update_shutter_switches(indigo_device *device) {
	if (PRIVATE_DATA->shutter_position == 0) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
	} else if (PRIVATE_DATA->shutter_position != BAADER_POSITION_UNKNOWN) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
	}
}

static void baader_update_flap_switches(indigo_device *device) {
	if (PRIVATE_DATA->flap_state == FLAP_OPEN) {
		indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_OPENED_ITEM, true);
	} else if (PRIVATE_DATA->flap_state == FLAP_CLOSED) {
		indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_CLOSED_ITEM, true);
	} else if (PRIVATE_DATA->flap_state != BAADER_POSITION_UNKNOWN) {
		DOME_FLAP_OPENED_ITEM->sw.value = DOME_FLAP_CLOSED_ITEM->sw.value = false;
	}
}

static void dome_status_poll(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	baader_rc_t rc;
	double azimuth = PRIVATE_DATA->current_position;
	int shutter_position = PRIVATE_DATA->shutter_position, flap_state = PRIVATE_DATA->flap_state;
	bool rain, wind, timeout, powercut;
	// all four queries are sent in every poll; publication of a property with a queued request is deferred to its handler
	bool azimuth_read = (rc = baader_get_azimuth(device, &azimuth)) == BD_SUCCESS;
	if (!azimuth_read) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error %d", rc);
	}
	bool shutter_read = (rc = baader_get_shutter_position(device, &shutter_position)) == BD_SUCCESS;
	if (!shutter_read) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_shutter_position(): returned error %d", rc);
	}
	bool flap_read = (rc = baader_get_flap_state(device, &flap_state)) == BD_SUCCESS;
	if (!flap_read) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_flap_state(): returned error %d", rc);
	}
	bool emergency_read = (rc = baader_get_emergency_status(device, &rain, &wind, &timeout, &powercut)) == BD_SUCCESS;
	if (!emergency_read) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_emergency_status(): returned error %d", rc);
	}
	// an emergency flag raised during an operation means the controller stopped it
	int emergency = emergency_read ? baader_emergency_flags(rain, wind, timeout, powercut) : 0;
	char emergency_message[INDIGO_VALUE_SIZE];
	/* Handle dome rotation */
	PRIVATE_DATA->current_position = azimuth;
	bool rotation_queued = !PRIVATE_DATA->rotation_active && (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE);
	if (rotation_queued) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Rotation request queued");
	} else if (PRIVATE_DATA->rotation_active && !PRIVATE_DATA->aborted && (emergency & ~PRIVATE_DATA->rotation_emergency)) {
		baader_emergency_message(emergency & ~PRIVATE_DATA->rotation_emergency, emergency_message, sizeof(emergency_message));
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotation stopped: %s", emergency_message);
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, emergency_message);
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		if (PRIVATE_DATA->park_requested) {
			PRIVATE_DATA->park_requested = false;
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, emergency_message);
		}
		PRIVATE_DATA->rotation_active = false;
	} else if (PRIVATE_DATA->rotation_active) {
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		if (baader_tenths(PRIVATE_DATA->target_position) != baader_tenths(PRIVATE_DATA->current_position)) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		} else {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->aborted;
		}
		if (baader_tenths(BAADER_PARK_AZIMUTH) == baader_tenths(PRIVATE_DATA->current_position) && PRIVATE_DATA->park_requested && !PRIVATE_DATA->rotation_active) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}
	} else if (azimuth_read && baader_tenths(PRIVATE_DATA->target_position) != baader_tenths(PRIVATE_DATA->current_position)) {
		// moved by the hand controller or another client
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	}
	/* Handle dome shutter */
	if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->shutter_active) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Shutter request queued");
	} else if (PRIVATE_DATA->shutter_active && !PRIVATE_DATA->aborted && (emergency & ~PRIVATE_DATA->shutter_emergency)) {
		baader_emergency_message(emergency & ~PRIVATE_DATA->shutter_emergency, emergency_message, sizeof(emergency_message));
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Shutter stopped: %s", emergency_message);
		PRIVATE_DATA->shutter_position = shutter_position;
		baader_update_shutter_switches(device);
		PRIVATE_DATA->shutter_active = false;
		PRIVATE_DATA->shutter_alert = true;
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, emergency_message);
	} else if (shutter_position != PRIVATE_DATA->shutter_position || DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->shutter_position = shutter_position;
		baader_update_shutter_switches(device);
		if (PRIVATE_DATA->shutter_alert) {
			// the request stopped by an emergency stays ALERT until the next shutter request
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		} else if (shutter_position == 100) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->shutter_active = false;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter open");
		} else if (shutter_position == 0) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->shutter_active = false;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter closed");
		} else {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		}
	}
	/* Handle dome flap */
	if (DOME_FLAP_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->flap_active) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Flap request queued");
	} else if (PRIVATE_DATA->flap_active && !PRIVATE_DATA->aborted && (emergency & ~PRIVATE_DATA->flap_emergency)) {
		baader_emergency_message(emergency & ~PRIVATE_DATA->flap_emergency, emergency_message, sizeof(emergency_message));
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Flap stopped: %s", emergency_message);
		PRIVATE_DATA->flap_state = flap_state;
		baader_update_flap_switches(device);
		PRIVATE_DATA->flap_active = false;
		PRIVATE_DATA->flap_alert = true;
		DOME_FLAP_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_FLAP_PROPERTY, emergency_message);
	} else if (flap_state != PRIVATE_DATA->flap_state || DOME_FLAP_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->flap_state = flap_state;
		baader_update_flap_switches(device);
		if (PRIVATE_DATA->flap_alert) {
			// the request stopped by an emergency stays ALERT until the next flap request
			indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		} else if (flap_state == FLAP_OPEN) {
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->flap_active = false;
			indigo_update_property(device, DOME_FLAP_PROPERTY, "Flap open");
		} else if (flap_state == FLAP_CLOSED) {
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->flap_active = false;
			indigo_update_property(device, DOME_FLAP_PROPERTY, "Flap closed");
		} else if (flap_state == FLAP_STOPPED) {
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->flap_active = false;
			indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		} else {
			indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		}
	}
	if (PRIVATE_DATA->aborted) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		PRIVATE_DATA->rotation_active = PRIVATE_DATA->shutter_active = PRIVATE_DATA->flap_active = PRIVATE_DATA->aborted = false;
	}
	/* Emergency flags state */
	if (emergency_read && (X_EMERGENCY_CLOSE_PROPERTY->state == INDIGO_IDLE_STATE || PRIVATE_DATA->rain != rain || PRIVATE_DATA->wind != wind || PRIVATE_DATA->timeout != timeout || PRIVATE_DATA->powercut != powercut)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Updating X_EMERGENCY_CLOSE");
		X_EMERGENCY_CLOSE_PROPERTY->state = INDIGO_OK_STATE;
		X_EMERGENCY_RAIN_ITEM->light.value = rain ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		PRIVATE_DATA->rain = rain;
		X_EMERGENCY_WIND_ITEM->light.value = wind ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		PRIVATE_DATA->wind = wind;
		X_EMERGENCY_OPERATION_TIMEOUT_ITEM->light.value = timeout ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		PRIVATE_DATA->timeout = timeout;
		X_EMERGENCY_POWERCUT_ITEM->light.value = powercut ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		PRIVATE_DATA->powercut = powercut;
		indigo_update_property(device, X_EMERGENCY_CLOSE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, BAADER_POLL_DELAY, dome_status_poll);
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = baader_open(device);
		if (connection_result) {
			//+ dome.on_connect
			baader_rc_t rc;
			PRIVATE_DATA->shutter_position = PRIVATE_DATA->flap_state = BAADER_POSITION_UNKNOWN;
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->shutter_active = PRIVATE_DATA->flap_active = false;
			PRIVATE_DATA->shutter_alert = PRIVATE_DATA->flap_alert = false;
			PRIVATE_DATA->rain = PRIVATE_DATA->wind = PRIVATE_DATA->timeout = PRIVATE_DATA->powercut = false;
			X_EMERGENCY_CLOSE_PROPERTY->state = X_EMERGENCY_RAIN_ITEM->light.value = X_EMERGENCY_WIND_ITEM->light.value = X_EMERGENCY_OPERATION_TIMEOUT_ITEM->light.value = X_EMERGENCY_POWERCUT_ITEM->light.value = INDIGO_IDLE_STATE;
			if ((rc = baader_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error %d", rc);
			}
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			PRIVATE_DATA->aborted = false;
			if ((indigo_azimuth_distance(BAADER_PARK_AZIMUTH, PRIVATE_DATA->current_position) * 100) <= 1) {
				indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
			} else {
				indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			}
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
			indigo_execute_handler_in(device, BAADER_FIRST_POLL_DELAY, dome_status_poll);
			//- dome.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_EMERGENCY_CLOSE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_EMERGENCY_CLOSE_PROPERTY, NULL);
		baader_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_horizontal_coordinates_handler(indigo_device *device) {
	//+ dome.DOME_HORIZONTAL_COORDINATES.on_change
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	baader_rc_t rc;
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		if ((rc = baader_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error %d", rc);
		}
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked");
		return;
	}
	double target = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
	if ((rc = baader_goto_azimuth(device, target)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(): returned error %d", rc);
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, rc == BD_DOME_ERROR ? "Goto azimuth failed with DOME_ERROR. Please inspect the dome!" : "Goto azimuth failed");
		return;
	}
	PRIVATE_DATA->target_position = target;
	PRIVATE_DATA->rotation_active = true;
	PRIVATE_DATA->rotation_emergency = baader_known_emergency(device);
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	//- dome.DOME_HORIZONTAL_COORDINATES.on_change
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
}

static void dome_steps_handler(indigo_device *device) {
	//+ dome.DOME_STEPS.on_change
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device commands run
	baader_rc_t rc;
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
		return;
	}
	if ((rc = baader_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error %d", rc);
	}
	// integer tenths of degree avoid truncating the sum of a float azimuth and the step
	long current = lround(PRIVATE_DATA->current_position * 10), steps = lround(DOME_STEPS_ITEM->number.value * 10);
	double target = PRIVATE_DATA->target_position;
	if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
		target = ((current - steps) % 3600 + 3600) % 3600 / 10.0;
	} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
		target = (current + steps) % 3600 / 10.0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "target_position = %.1f", target);
	if ((rc = baader_goto_azimuth(device, target)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(): returned error %d", rc);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, rc == BD_DOME_ERROR ? "Goto azimuth failed with DOME_ERROR. Please inspect the dome!" : "Goto azimuth failed");
		return;
	}
	PRIVATE_DATA->target_position = target;
	PRIVATE_DATA->rotation_active = true;
	PRIVATE_DATA->rotation_emergency = baader_known_emergency(device);
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	//- dome.DOME_STEPS.on_change
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}

static void dome_park_handler(indigo_device *device) {
	//+ dome.DOME_PARK.on_change
	DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	baader_rc_t rc;
	if (DOME_PARK_UNPARKED_ITEM->sw.value) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_requested = false;
	} else if (DOME_PARK_PARKED_ITEM->sw.value) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		if ((rc = baader_goto_azimuth(device, BAADER_PARK_AZIMUTH)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(): returned error %d", rc);
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, rc == BD_DOME_ERROR ? "Goto azimuth failed with DOME_ERROR. Please inspect the dome!" : "Goto azimuth failed");
			return;
		}
		PRIVATE_DATA->target_position = BAADER_PARK_AZIMUTH;
		PRIVATE_DATA->park_requested = true;
		PRIVATE_DATA->rotation_active = true;
		PRIVATE_DATA->rotation_emergency = baader_known_emergency(device);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	}
	//- dome.DOME_PARK.on_change
	indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
}

static void dome_abort_motion_handler(indigo_device *device) {
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	baader_rc_t rc;
	// urgent abort can overtake queued requests: settle the properties they left BUSY
	indigo_cancel_pending_handler(device, dome_horizontal_coordinates_handler);
	indigo_cancel_pending_handler(device, dome_steps_handler);
	indigo_cancel_pending_handler(device, dome_park_handler);
	indigo_cancel_pending_handler(device, dome_shutter_handler);
	indigo_cancel_pending_handler(device, dome_flap_handler);
	if (!PRIVATE_DATA->rotation_active && (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->current_position;
		INDIGO_UPDATE_PROPERTY_STATE(DOME_HORIZONTAL_COORDINATES_PROPERTY, INDIGO_OK_STATE, NULL);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_STEPS_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->shutter_active && DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		baader_update_shutter_switches(device);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->flap_active && DOME_FLAP_PROPERTY->state == INDIGO_BUSY_STATE) {
		baader_update_flap_switches(device);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_FLAP_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	PRIVATE_DATA->park_requested = false;
	PRIVATE_DATA->shutter_alert = PRIVATE_DATA->flap_alert = false;
	if ((rc = baader_command_ok(device, "d#stopdom")) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_abort(): returned error %d", rc);
		if (DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->rotation_active) {
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			INDIGO_UPDATE_PROPERTY_STATE(DOME_PARK_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, rc == BD_DOME_ERROR ? "Abort failed with DOME_ERROR. Please inspect the dome!" : "Abort failed");
		return;
	}
	if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	}
	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
	PRIVATE_DATA->aborted = true;
	DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
	PRIVATE_DATA->shutter_active = false;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	PRIVATE_DATA->shutter_alert = false;
	bool open = DOME_SHUTTER_OPENED_ITEM->sw.value;
	baader_rc_t rc = open ? baader_command_ok(device, "d#opeshut") : baader_command_ok(device, "d#closhut");
	if (rc != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Shutter %s returned error %d", open ? "open" : "close", rc);
		baader_update_shutter_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, rc == BD_DOME_ERROR ? "Shutter open/close failed with DOME_ERROR. Please inspect the dome!" : "Shutter open/close failed");
		return;
	}
	PRIVATE_DATA->shutter_active = true;
	PRIVATE_DATA->shutter_emergency = baader_known_emergency(device);
	indigo_send_message(device, DOME_SHUTTER_PROPERTY, open ? "Opening shutter..." : "Closing shutter...");
	//- dome.DOME_SHUTTER.on_change
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_flap_handler(indigo_device *device) {
	//+ dome.DOME_FLAP.on_change
	DOME_FLAP_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	PRIVATE_DATA->flap_alert = false;
	bool open = DOME_FLAP_OPENED_ITEM->sw.value;
	baader_rc_t rc = open ? baader_command_ok(device, "d#opeflap") : baader_command_ok(device, "d#cloflap");
	if (rc != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Flap %s returned error %d", open ? "open" : "close", rc);
		baader_update_flap_switches(device);
		DOME_FLAP_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_FLAP_PROPERTY, rc == BD_DOME_ERROR ? "Flap open/close failed with DOME_ERROR. Please inspect the dome!" : "Flap open/close failed. Is the shutter open enough?");
		return;
	}
	PRIVATE_DATA->flap_active = true;
	PRIVATE_DATA->flap_emergency = baader_known_emergency(device);
	indigo_send_message(device, DOME_FLAP_PROPERTY, open ? "Opening flap..." : "Closing flap...");
	//- dome.DOME_FLAP.on_change
	indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ dome.on_attach
		INFO_PROPERTY->count = 8;
		//- dome.on_attach
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_ON_COORDINATES_SET_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = false;
		//+ dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		//- dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_STEPS_PROPERTY->hidden = false;
		DOME_PARK_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		DOME_SHUTTER_PROPERTY->hidden = false;
		DOME_FLAP_PROPERTY->hidden = false;
		X_EMERGENCY_CLOSE_PROPERTY = indigo_init_light_property(NULL, device->name, X_EMERGENCY_CLOSE_PROPERTY_NAME, DOME_MAIN_GROUP, "Emergency close flags", INDIGO_OK_STATE, 4);
		if (X_EMERGENCY_CLOSE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_EMERGENCY_RAIN_ITEM, X_EMERGENCY_RAIN_ITEM_NAME, "Rain alert", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_EMERGENCY_WIND_ITEM, X_EMERGENCY_WIND_ITEM_NAME, "Wind alert", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_EMERGENCY_OPERATION_TIMEOUT_ITEM, X_EMERGENCY_OPERATION_TIMEOUT_ITEM_NAME, "Operation timeout alert", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_EMERGENCY_POWERCUT_ITEM, X_EMERGENCY_POWERCUT_ITEM_NAME, "Power outage alert", INDIGO_IDLE_STATE);
		//+ dome.X_EMERGENCY_CLOSE.on_attach
		X_EMERGENCY_CLOSE_PROPERTY->state = INDIGO_IDLE_STATE;
		//- dome.X_EMERGENCY_CLOSE.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_EMERGENCY_CLOSE_PROPERTY);
	}
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, dome_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_HORIZONTAL_COORDINATES_PROPERTY, dome_horizontal_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_STEPS_PROPERTY, property)) {
		if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < DOME_STEPS_PROPERTY->count; i++) {
				DOME_STEPS_PROPERTY->items[i].do_update = true;
			}
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_STEPS_PROPERTY, dome_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_PARK_PROPERTY, dome_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_FLAP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_FLAP_PROPERTY, dome_flap_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(X_EMERGENCY_CLOSE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_baader(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static baader_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (baader_private_data *)indigo_safe_malloc(sizeof(baader_private_data));
			dome = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				indigo_safe_free(dome);
				dome = NULL;
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
