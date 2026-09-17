// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_dome_talon6ror.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_talon6ror.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000003
#define DRIVER_NAME          "indigo_dome_talon6ror"
#define DRIVER_LABEL         "Talon6 ROR"
#define DOME_DEVICE_NAME     "Talon6 ROR"
#define PRIVATE_DATA         ((talon6ror_private_data *)device->private_data)

//+ define

#define TALON6ROR_READ_LENGTH 160
#define TALON6ROR_PAYLOAD_LENGTH 64
// configuration frame is 'a' + 54 bytes + checksum, status payload is 'G' + 20 bytes + checksum
#define TALON6ROR_CONFIGURATION_LENGTH 56
#define TALON6ROR_STATUS_LENGTH 22
#define TALON6ROR_BYTE_TIMEOUT 5
#define TALON6ROR_FLUSH_TIMEOUT 5
#define TALON6ROR_FLUSH_GAP  0.01
#define TALON6ROR_POLL_DELAY 0.5
// the controller may need a moment before it starts moving the roof
#define TALON6ROR_START_TIMEOUT 3
#define TALON6ROR_STATE_OPEN 0
#define TALON6ROR_STATE_CLOSED 1
#define TALON6ROR_STATE_OPENING 2
#define TALON6ROR_STATE_CLOSING 3
#define TALON6ROR_ACTION_PARK 15
#define TALON6ROR_STATE_UNKNOWN (-1)

//- define

#pragma mark - Property definitions

#define X_SENSORS_PROPERTY                    (PRIVATE_DATA->x_sensors_property)
#define X_SENSORS_POWER_CONDITION_ITEM        (X_SENSORS_PROPERTY->items + 0)
#define X_SENSORS_WEATHER_CONDITION_ITEM      (X_SENSORS_PROPERTY->items + 1)
#define X_SENSORS_PARKED_SENSOR_ITEM          (X_SENSORS_PROPERTY->items + 2)
#define X_SENSORS_OPEN_SENSOR_ITEM            (X_SENSORS_PROPERTY->items + 3)
#define X_SENSORS_CLOSE_SENSOR_ITEM           (X_SENSORS_PROPERTY->items + 4)
#define X_SENSORS_OPEN_BUTTON_ITEM            (X_SENSORS_PROPERTY->items + 5)
#define X_SENSORS_CLOSE_BUTTON_ITEM           (X_SENSORS_PROPERTY->items + 6)
#define X_SENSORS_STOP_BUTTON_ITEM            (X_SENSORS_PROPERTY->items + 7)

#define X_SENSORS_PROPERTY_NAME               "X_SENSORS"
#define X_SENSORS_POWER_CONDITION_ITEM_NAME   "POWER_CONDITION"
#define X_SENSORS_WEATHER_CONDITION_ITEM_NAME "WEATHER_CONDITION"
#define X_SENSORS_PARKED_SENSOR_ITEM_NAME     "PARKED_SENSOR"
#define X_SENSORS_OPEN_SENSOR_ITEM_NAME       "OPEN_SENSOR"
#define X_SENSORS_CLOSE_SENSOR_ITEM_NAME      "CLOSE_SENSOR"
#define X_SENSORS_OPEN_BUTTON_ITEM_NAME       "OPEN_BUTTON"
#define X_SENSORS_CLOSE_BUTTON_ITEM_NAME      "CLOSE_BUTTON"
#define X_SENSORS_STOP_BUTTON_ITEM_NAME       "STOP_BUTTON"

#define X_MOTOR_CONF_PROPERTY               (PRIVATE_DATA->x_motor_conf_property)
#define X_MOTOR_CONF_KP_ITEM                (X_MOTOR_CONF_PROPERTY->items + 0)
#define X_MOTOR_CONF_KI_ITEM                (X_MOTOR_CONF_PROPERTY->items + 1)
#define X_MOTOR_CONF_KD_ITEM                (X_MOTOR_CONF_PROPERTY->items + 2)
#define X_MOTOR_CONF_MAX_SPEED_ITEM         (X_MOTOR_CONF_PROPERTY->items + 3)
#define X_MOTOR_CONF_MIN_SPEED_ITEM         (X_MOTOR_CONF_PROPERTY->items + 4)
#define X_MOTOR_CONF_ACCELERATION_ITEM      (X_MOTOR_CONF_PROPERTY->items + 5)
#define X_MOTOR_CONF_RAMP_ITEM              (X_MOTOR_CONF_PROPERTY->items + 6)
#define X_MOTOR_CONF_FULL_ENC_ITEM          (X_MOTOR_CONF_PROPERTY->items + 7)
#define X_MOTOR_CONF_ENC_FACTOR_ITEM        (X_MOTOR_CONF_PROPERTY->items + 8)
#define X_MOTOR_CONF_REVERSE_ITEM           (X_MOTOR_CONF_PROPERTY->items + 9)

#define X_MOTOR_CONF_PROPERTY_NAME          "X_MOTOR_CONF"
#define X_MOTOR_CONF_KP_ITEM_NAME           "KP"
#define X_MOTOR_CONF_KI_ITEM_NAME           "KI"
#define X_MOTOR_CONF_KD_ITEM_NAME           "KD"
#define X_MOTOR_CONF_MAX_SPEED_ITEM_NAME    "MAX_SPEED"
#define X_MOTOR_CONF_MIN_SPEED_ITEM_NAME    "MIN_SPEED"
#define X_MOTOR_CONF_ACCELERATION_ITEM_NAME "ACCELERATION"
#define X_MOTOR_CONF_RAMP_ITEM_NAME         "RAMP"
#define X_MOTOR_CONF_FULL_ENC_ITEM_NAME     "FULL_ENC"
#define X_MOTOR_CONF_ENC_FACTOR_ITEM_NAME   "ENC_FACTOR"
#define X_MOTOR_CONF_REVERSE_ITEM_NAME      "REVERSE"

#define X_DELAY_CONF_PROPERTY          (PRIVATE_DATA->x_delay_conf_property)
#define X_DELAY_CONF_PARK_ITEM         (X_DELAY_CONF_PROPERTY->items + 0)
#define X_DELAY_CONF_WEATHER_ITEM      (X_DELAY_CONF_PROPERTY->items + 1)
#define X_DELAY_CONF_POWER_ITEM        (X_DELAY_CONF_PROPERTY->items + 2)
#define X_DELAY_CONF_TIMEOUT_ITEM      (X_DELAY_CONF_PROPERTY->items + 3)

#define X_DELAY_CONF_PROPERTY_NAME     "X_DELAY_CONF"
#define X_DELAY_CONF_PARK_ITEM_NAME    "PARK"
#define X_DELAY_CONF_WEATHER_ITEM_NAME "WEATHER"
#define X_DELAY_CONF_POWER_ITEM_NAME   "POWER"
#define X_DELAY_CONF_TIMEOUT_ITEM_NAME "TIMEOUT"

#define X_CLOSE_COND_PROPERTY          (PRIVATE_DATA->x_close_cond_property)
#define X_CLOSE_COND_WEATHER_ITEM      (X_CLOSE_COND_PROPERTY->items + 0)
#define X_CLOSE_COND_POWER_ITEM        (X_CLOSE_COND_PROPERTY->items + 1)
#define X_CLOSE_COND_TIMEOUT_ITEM      (X_CLOSE_COND_PROPERTY->items + 2)

#define X_CLOSE_COND_PROPERTY_NAME     "X_CLOSE_COND"
#define X_CLOSE_COND_WEATHER_ITEM_NAME "WEATHER"
#define X_CLOSE_COND_POWER_ITEM_NAME   "POWER"
#define X_CLOSE_COND_TIMEOUT_ITEM_NAME "TIMEOUT"

#define X_CLOSE_TIMER_PROPERTY          (PRIVATE_DATA->x_close_timer_property)
#define X_CLOSE_TIMER_WEATHER_ITEM      (X_CLOSE_TIMER_PROPERTY->items + 0)
#define X_CLOSE_TIMER_POWER_ITEM        (X_CLOSE_TIMER_PROPERTY->items + 1)
#define X_CLOSE_TIMER_TIMEOUT_ITEM      (X_CLOSE_TIMER_PROPERTY->items + 2)

#define X_CLOSE_TIMER_PROPERTY_NAME     "X_TIMER_COND"
#define X_CLOSE_TIMER_WEATHER_ITEM_NAME "WEATHER"
#define X_CLOSE_TIMER_POWER_ITEM_NAME   "POWER"
#define X_CLOSE_TIMER_TIMEOUT_ITEM_NAME "TIMEOUT"

#define X_POSITION_PROPERTY            (PRIVATE_DATA->x_position_property)
#define X_POSITION_ITEM                (X_POSITION_PROPERTY->items + 0)

#define X_POSITION_PROPERTY_NAME       "X_POSITION_PROPERTY"
#define X_POSITION_ITEM_NAME           "POSITION"

#define X_STATUS_PROPERTY              (PRIVATE_DATA->x_status_property)
#define X_STATUS_VOLTAGE_ITEM          (X_STATUS_PROPERTY->items + 0)

#define X_STATUS_PROPERTY_NAME         "X_STATUS_PROPERTY"
#define X_STATUS_VOLTAGE_ITEM_NAME     "VOLTAGE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_sensors_property;
	indigo_property *x_motor_conf_property;
	indigo_property *x_delay_conf_property;
	indigo_property *x_close_cond_property;
	indigo_property *x_close_timer_property;
	indigo_property *x_position_property;
	indigo_property *x_status_property;
	//+ data
	char reply[TALON6ROR_READ_LENGTH];
	uint8_t response[TALON6ROR_PAYLOAD_LENGTH + 1];
	long response_length;
	uint8_t configuration[TALON6ROR_CONFIGURATION_LENGTH + 1];
	uint8_t confirmed_configuration[TALON6ROR_CONFIGURATION_LENGTH + 1];
	const char *last_action;
	uint16_t sensors;
	int last_state, failed_state;
	char request;
	double request_time;
	bool request_motion, status_busy;
	//- data
} talon6ror_private_data;

#pragma mark - Low level code

//+ code

static const char *talon6ror_last_action[] = {
	NULL,
	"Open requested by user",
	"Close requested by user",
	NULL,
	"Go to requested by user",
	"Calibrate requested by user",
	"Close due to weather",
	"Close due to power failure",
	"Close due to lost communication",
	"Close due to internet failure",
	"Close due to timeout expired",
	"Close requested by management",
	"Close requested by automation",
	"Motor stalled",
	"Emergency stop",
	"Mount park requested"
};

static void talon6ror_pack(uint8_t *data, int value) {
	data[0] = 0x80 | ((value >> 14) & 0x7F);
	data[1] = 0x80 | ((value >> 7) & 0x7F);
	data[2] = 0x80 | ((value >> 0) & 0x7F);
}

static int talon6ror_unpack(const uint8_t *data) {
	return ((data[0] & 0x7F) << 14) | ((data[1] & 0x7F) << 7) | (data[2] & 0x7F);
}

static int talon6ror_unpack2(const uint8_t *data) {
	return ((data[0] & 0x7F) << 7) | (data[1] & 0x7F);
}

static uint8_t talon6ror_checksum(const uint8_t *data, int length) {
	int sum = 0;
	for (int i = 0; i < length; i++) {
		sum += data[i];
	}
	return (uint8_t)(0x80 | (-(sum % 128) & 0x7F));
}

// replies are "&<payload>#"; bytes before the ampersand and further ampersands are ignored like in the original reader
static bool talon6ror_read_reply(indigo_device *device) {
	PRIVATE_DATA->response[0] = 0;
	PRIVATE_DATA->response_length = 0;
	for (int attempt = 0; attempt < 4; attempt++) {
		long length = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->reply, TALON6ROR_READ_LENGTH - 1, "#", "", INDIGO_DELAY(TALON6ROR_BYTE_TIMEOUT), INDIGO_DELAY(TALON6ROR_BYTE_TIMEOUT));
		if (length <= 0 || PRIVATE_DATA->reply[length - 1] != '#') {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unterminated reply of %ld bytes", length);
			indigo_uni_discard(PRIVATE_DATA->handle);
			return false;
		}
		char *start = memchr(PRIVATE_DATA->reply, '&', length);
		if (start == NULL) {
			continue;
		}
		long payload = 0;
		for (char *pointer = start + 1; pointer < PRIVATE_DATA->reply + length - 1; pointer++) {
			if (*pointer == '&') {
				continue;
			}
			if (payload == TALON6ROR_PAYLOAD_LENGTH) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Overlong reply");
				indigo_uni_discard(PRIVATE_DATA->handle);
				return false;
			}
			PRIVATE_DATA->response[payload++] = (uint8_t)*pointer;
		}
		PRIVATE_DATA->response[payload] = 0;
		PRIVATE_DATA->response_length = payload;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No reply found");
	return false;
}

static bool talon6ror_vcommand(indigo_device *device, const char *format, va_list args) {
	if (indigo_uni_vtprintf(PRIVATE_DATA->handle, format, args, "%#") <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write command %s", format);
		return false;
	}
	return talon6ror_read_reply(device);
}

static bool talon6ror_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = talon6ror_vcommand(device, format, args);
	va_end(args);
	return result;
}

// acknowledgements are empty replies; the controller answers a rejected command with "ERROR"
static bool talon6ror_ack(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = talon6ror_vcommand(device, format, args);
	va_end(args);
	if (result && !strncmp((char *)PRIVATE_DATA->response, "ERROR", 5)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command %s rejected", format);
		return false;
	}
	return result;
}

// the controller sends a banner after the reset; drain it before the handshake
static bool talon6ror_flush(indigo_device *device) {
	long timeout = INDIGO_DELAY(TALON6ROR_FLUSH_TIMEOUT);
	while (true) {
		long length = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->reply, TALON6ROR_READ_LENGTH - 1, "", "", timeout, INDIGO_DELAY(TALON6ROR_FLUSH_GAP));
		if (length < 0) {
			return false;
		}
		if (length == 0) {
			return true;
		}
		timeout = INDIGO_DELAY(TALON6ROR_FLUSH_GAP);
	}
}

static bool talon6ror_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
		return false;
	}
	if (talon6ror_flush(device) && talon6ror_command(device, "&V") && PRIVATE_DATA->response[0] == 'V') {
		char revision[8] = { 0 };
		memcpy(revision, PRIVATE_DATA->response + 1, 7);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, revision);
		indigo_update_property(device, INFO_PROPERTY, NULL);
		if (talon6ror_command(device, "&p") && PRIVATE_DATA->response[0] == 'p' && PRIVATE_DATA->response_length == TALON6ROR_CONFIGURATION_LENGTH) {
			if (PRIVATE_DATA->response[55] == talon6ror_checksum(PRIVATE_DATA->response + 1, 54)) {
				memcpy(PRIVATE_DATA->configuration, PRIVATE_DATA->response, TALON6ROR_CONFIGURATION_LENGTH);
				PRIVATE_DATA->configuration[0] = 'a';
				PRIVATE_DATA->configuration[TALON6ROR_CONFIGURATION_LENGTH] = 0;
				memcpy(PRIVATE_DATA->confirmed_configuration, PRIVATE_DATA->configuration, sizeof(PRIVATE_DATA->confirmed_configuration));
				return true;
			}
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Checksum error, handshake failed");
			indigo_send_message(device, ALERT_PROPERTY, "Checksum error, handshake failed");
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			indigo_send_message(device, ALERT_PROPERTY, "Handshake failed");
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
		indigo_send_message(device, ALERT_PROPERTY, "Handshake failed");
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void talon6ror_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

//- code

//+ dome.code

static void dome_status_poll(indigo_device *device);
static void dome_shutter_handler(indigo_device *device);

static void talon6ror_unpack_configuration(indigo_device *device) {
	uint8_t *configuration = PRIVATE_DATA->configuration;
	X_MOTOR_CONF_KP_ITEM->number.value = X_MOTOR_CONF_KP_ITEM->number.target = talon6ror_unpack(configuration + 1);
	X_MOTOR_CONF_KI_ITEM->number.value = X_MOTOR_CONF_KI_ITEM->number.target = talon6ror_unpack(configuration + 4);
	X_MOTOR_CONF_KD_ITEM->number.value = X_MOTOR_CONF_KD_ITEM->number.target = talon6ror_unpack(configuration + 7);
	X_MOTOR_CONF_MAX_SPEED_ITEM->number.value = X_MOTOR_CONF_MAX_SPEED_ITEM->number.target = talon6ror_unpack(configuration + 10);
	X_MOTOR_CONF_MIN_SPEED_ITEM->number.value = X_MOTOR_CONF_MIN_SPEED_ITEM->number.target = talon6ror_unpack(configuration + 13);
	X_MOTOR_CONF_ACCELERATION_ITEM->number.value = X_MOTOR_CONF_ACCELERATION_ITEM->number.target = talon6ror_unpack(configuration + 16);
	X_DELAY_CONF_PARK_ITEM->number.value = X_DELAY_CONF_PARK_ITEM->number.target = talon6ror_unpack(configuration + 19);
	X_DELAY_CONF_WEATHER_ITEM->number.value = X_DELAY_CONF_WEATHER_ITEM->number.target = talon6ror_unpack(configuration + 22);
	X_DELAY_CONF_POWER_ITEM->number.value = X_DELAY_CONF_POWER_ITEM->number.target = talon6ror_unpack(configuration + 25);
	// 28 communication delay, 31 maximum aperture, 34 IP switch are not exposed
	X_MOTOR_CONF_FULL_ENC_ITEM->number.value = X_MOTOR_CONF_FULL_ENC_ITEM->number.target = talon6ror_unpack(configuration + 37);
	X_MOTOR_CONF_RAMP_ITEM->number.value = X_MOTOR_CONF_RAMP_ITEM->number.target = talon6ror_unpack(configuration + 40);
	X_DELAY_CONF_TIMEOUT_ITEM->number.value = X_DELAY_CONF_TIMEOUT_ITEM->number.target = talon6ror_unpack(configuration + 43);
	X_MOTOR_CONF_ENC_FACTOR_ITEM->number.value = X_MOTOR_CONF_ENC_FACTOR_ITEM->number.target = configuration[46] & 0x7F;
	X_MOTOR_CONF_REVERSE_ITEM->number.value = X_MOTOR_CONF_REVERSE_ITEM->number.target = configuration[47] & 0x0F;
	X_CLOSE_COND_POWER_ITEM->sw.value = (configuration[48] & (1 << 0)) != 0;
	X_CLOSE_COND_WEATHER_ITEM->sw.value = (configuration[48] & (1 << 1)) != 0;
	X_CLOSE_COND_TIMEOUT_ITEM->sw.value = (configuration[48] & (1 << 2)) != 0;
	// 49 dummy, 52 TW time are not exposed
}

static bool talon6ror_write_configuration(indigo_device *device) {
	PRIVATE_DATA->configuration[55] = talon6ror_checksum(PRIVATE_DATA->configuration + 1, 54);
	if (!talon6ror_ack(device, "&%s", (char *)PRIVATE_DATA->configuration)) {
		// the rejected values must not be sent again by the next write
		memcpy(PRIVATE_DATA->configuration, PRIVATE_DATA->confirmed_configuration, sizeof(PRIVATE_DATA->configuration));
		talon6ror_unpack_configuration(device);
		return false;
	}
	memcpy(PRIVATE_DATA->confirmed_configuration, PRIVATE_DATA->configuration, sizeof(PRIVATE_DATA->confirmed_configuration));
	return true;
}

static void talon6ror_update_switches(indigo_device *device) {
	if (PRIVATE_DATA->last_state == TALON6ROR_STATE_OPEN) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
	} else if (PRIVATE_DATA->last_state == TALON6ROR_STATE_CLOSED) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
	} else {
		DOME_SHUTTER_OPENED_ITEM->sw.value = DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
	}
}

// publishes the roof state reported by the controller when no request of this driver is active
static void talon6ror_mirror_state(indigo_device *device, int state) {
	if (state == PRIVATE_DATA->failed_state) {
		// a failed request keeps its ALERT until the controller reports another state
		return;
	}
	switch (state) {
		case TALON6ROR_STATE_OPEN:
			PRIVATE_DATA->status_busy = false;
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE || !DOME_SHUTTER_OPENED_ITEM->sw.value) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof opened");
			}
			break;
		case TALON6ROR_STATE_CLOSED:
			PRIVATE_DATA->status_busy = false;
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE || !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof closed");
			}
			break;
		case TALON6ROR_STATE_OPENING:
			PRIVATE_DATA->status_busy = true;
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE || !DOME_SHUTTER_OPENED_ITEM->sw.value) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof opening");
			}
			break;
		case TALON6ROR_STATE_CLOSING:
			PRIVATE_DATA->status_busy = true;
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE || !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof closing");
			}
			break;
		default:
			PRIVATE_DATA->status_busy = false;
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_ALERT_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Error reported");
			}
			break;
	}
}

// completes an open or close request of this driver from the reported roof state
static void talon6ror_finish_request(indigo_device *device, int state, int action) {
	int requested = PRIVATE_DATA->request == 'O' ? TALON6ROR_STATE_OPEN : TALON6ROR_STATE_CLOSED;
	if (state == TALON6ROR_STATE_OPENING || state == TALON6ROR_STATE_CLOSING) {
		PRIVATE_DATA->request_motion = true;
		PRIVATE_DATA->status_busy = true;
		bool opening = state == TALON6ROR_STATE_OPENING;
		if (!(opening ? DOME_SHUTTER_OPENED_ITEM : DOME_SHUTTER_CLOSED_ITEM)->sw.value) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, opening ? DOME_SHUTTER_OPENED_ITEM : DOME_SHUTTER_CLOSED_ITEM, true);
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, opening ? "Roof opening" : "Roof closing");
		}
		return;
	}
	if (state == requested) {
		PRIVATE_DATA->request = 0;
		PRIVATE_DATA->status_busy = false;
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, requested == TALON6ROR_STATE_OPEN ? DOME_SHUTTER_OPENED_ITEM : DOME_SHUTTER_CLOSED_ITEM, true);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, requested == TALON6ROR_STATE_OPEN ? "Roof opened" : "Roof closed");
		return;
	}
	if (!PRIVATE_DATA->request_motion && (indigo_monotonic_time() - PRIVATE_DATA->request_time < TALON6ROR_START_TIMEOUT || action == TALON6ROR_ACTION_PARK)) {
		// the controller has not started the motion yet; a close waits until the mount is parked
		return;
	}
	bool opening = PRIVATE_DATA->request == 'O';
	PRIVATE_DATA->request = 0;
	PRIVATE_DATA->status_busy = false;
	PRIVATE_DATA->failed_state = state;
	PRIVATE_DATA->last_state = state;
	talon6ror_update_switches(device);
	DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	if (state == TALON6ROR_STATE_OPEN || state == TALON6ROR_STATE_CLOSED) {
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, opening ? "Roof did not open" : "Roof did not close");
	} else {
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Error reported");
	}
}

static void dome_status_poll(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	if (talon6ror_command(device, "&G") && PRIVATE_DATA->response[0] == 'G') {
		uint8_t *response = PRIVATE_DATA->response;
		if (PRIVATE_DATA->response_length == TALON6ROR_STATUS_LENGTH && response[21] == talon6ror_checksum(response + 1, 20)) {
			int state = (response[1] & 0x70) >> 4;
			int action = response[1] & 0x0F;
			if (state != PRIVATE_DATA->failed_state) {
				PRIVATE_DATA->failed_state = TALON6ROR_STATE_UNKNOWN;
			}
			PRIVATE_DATA->last_state = state;
			if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE && PRIVATE_DATA->request == 0 && !PRIVATE_DATA->status_busy) {
				// a request is queued behind this poll; its handler owns the property
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Shutter request queued");
			} else if (PRIVATE_DATA->request) {
				talon6ror_finish_request(device, state, action);
			} else {
				talon6ror_mirror_state(device, state);
			}
			const char *last_action = talon6ror_last_action[action];
			if (PRIVATE_DATA->last_action != last_action) {
				indigo_send_message(device, IDLE_PROPERTY, last_action);
				PRIVATE_DATA->last_action = last_action;
			}
			int position = talon6ror_unpack(response + 2);
			if (X_POSITION_ITEM->number.value != position) {
				X_POSITION_ITEM->number.value = position;
				X_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_POSITION_PROPERTY, NULL);
			}
			double voltage = round(talon6ror_unpack2(response + 5) * 150.0 / 1024) / 10.0;
			if (X_STATUS_VOLTAGE_ITEM->number.value != voltage) {
				X_STATUS_VOLTAGE_ITEM->number.value = voltage;
				X_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_STATUS_PROPERTY, NULL);
			}
			int timeout_timer = talon6ror_unpack(response + 7);
			int power_timer = talon6ror_unpack2(response + 10);
			int weather_timer = talon6ror_unpack2(response + 12);
			if (X_CLOSE_TIMER_TIMEOUT_ITEM->number.value != timeout_timer || X_CLOSE_TIMER_POWER_ITEM->number.value != power_timer || X_CLOSE_TIMER_WEATHER_ITEM->number.value != weather_timer) {
				X_CLOSE_TIMER_TIMEOUT_ITEM->number.value = timeout_timer;
				X_CLOSE_TIMER_POWER_ITEM->number.value = power_timer;
				X_CLOSE_TIMER_WEATHER_ITEM->number.value = weather_timer;
				X_CLOSE_TIMER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_CLOSE_TIMER_PROPERTY, NULL);
			}
			uint16_t sensors = ((response[14] & 0x7F) << 8) | (response[15] & 0x7F);
			X_SENSORS_POWER_CONDITION_ITEM->light.value = sensors & (1 << 0) ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_WEATHER_CONDITION_ITEM->light.value = sensors & (1 << 1) ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_PARKED_SENSOR_ITEM->light.value = sensors & (1 << 2) ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_OPEN_SENSOR_ITEM->light.value = sensors & (1 << 3) ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_CLOSE_SENSOR_ITEM->light.value = sensors & (1 << 4) ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_OPEN_BUTTON_ITEM->light.value = sensors & (1 << 5) ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_STOP_BUTTON_ITEM->light.value = sensors & (1 << 6) ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
			X_SENSORS_CLOSE_BUTTON_ITEM->light.value = sensors & (1 << 8) ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
			if (PRIVATE_DATA->sensors != sensors) {
				PRIVATE_DATA->sensors = sensors;
				indigo_update_property(device, X_SENSORS_PROPERTY, NULL);
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Checksum error");
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_ALERT_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Checksum error");
			}
		}
	}
	indigo_execute_handler_in(device, TALON6ROR_POLL_DELAY, dome_status_poll);
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = talon6ror_open(device);
		if (connection_result) {
			//+ dome.on_connect
			PRIVATE_DATA->last_action = NULL;
			PRIVATE_DATA->sensors = 0;
			PRIVATE_DATA->last_state = TALON6ROR_STATE_UNKNOWN;
			PRIVATE_DATA->failed_state = TALON6ROR_STATE_UNKNOWN;
			PRIVATE_DATA->request = 0;
			PRIVATE_DATA->request_motion = PRIVATE_DATA->status_busy = false;
			talon6ror_unpack_configuration(device);
			indigo_execute_handler(device, dome_status_poll);
			//- dome.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_SENSORS_PROPERTY, NULL);
			indigo_define_property(device, X_MOTOR_CONF_PROPERTY, NULL);
			indigo_define_property(device, X_DELAY_CONF_PROPERTY, NULL);
			indigo_define_property(device, X_CLOSE_COND_PROPERTY, NULL);
			indigo_define_property(device, X_CLOSE_TIMER_PROPERTY, NULL);
			indigo_define_property(device, X_POSITION_PROPERTY, NULL);
			indigo_define_property(device, X_STATUS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, X_MOTOR_CONF_PROPERTY, NULL);
		indigo_delete_property(device, X_DELAY_CONF_PROPERTY, NULL);
		indigo_delete_property(device, X_CLOSE_COND_PROPERTY, NULL);
		indigo_delete_property(device, X_CLOSE_TIMER_PROPERTY, NULL);
		indigo_delete_property(device, X_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, X_STATUS_PROPERTY, NULL);
		talon6ror_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	PRIVATE_DATA->failed_state = TALON6ROR_STATE_UNKNOWN;
	bool open = DOME_SHUTTER_OPENED_ITEM->sw.value;
	if (!open && !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
		talon6ror_update_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
	} else if (!talon6ror_ack(device, open ? "&O" : "&P")) {
		talon6ror_update_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, DOME_SHUTTER_PROPERTY, open ? "Roof open failed" : "Roof close failed");
	} else {
		PRIVATE_DATA->request = open ? 'O' : 'P';
		PRIVATE_DATA->request_time = indigo_monotonic_time();
		PRIVATE_DATA->request_motion = false;
	}
	//- dome.DOME_SHUTTER.on_change
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_abort_motion_handler(indigo_device *device) {
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	if (!DOME_ABORT_MOTION_ITEM->sw.value || DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		// the urgent abort can overtake a queued open or close request
		indigo_cancel_pending_handler(device, dome_shutter_handler);
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		if (!talon6ror_ack(device, "&S")) {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->request = 0;
			PRIVATE_DATA->status_busy = false;
			PRIVATE_DATA->failed_state = TALON6ROR_STATE_UNKNOWN;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		}
	}
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void dome_x_motor_conf_handler(indigo_device *device) {
	//+ dome.X_MOTOR_CONF.on_change
	X_MOTOR_CONF_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	talon6ror_pack(PRIVATE_DATA->configuration + 1, X_MOTOR_CONF_KP_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 4, X_MOTOR_CONF_KI_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 7, X_MOTOR_CONF_KD_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 10, X_MOTOR_CONF_MAX_SPEED_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 13, X_MOTOR_CONF_MIN_SPEED_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 16, X_MOTOR_CONF_ACCELERATION_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 37, X_MOTOR_CONF_FULL_ENC_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 40, X_MOTOR_CONF_RAMP_ITEM->number.value);
	PRIVATE_DATA->configuration[46] = 0x80 | (int)X_MOTOR_CONF_ENC_FACTOR_ITEM->number.value;
	PRIVATE_DATA->configuration[47] = (PRIVATE_DATA->configuration[47] & 0xF0) | (((int)X_MOTOR_CONF_REVERSE_ITEM->number.value) & 0x0F);
	if (talon6ror_write_configuration(device)) {
		X_MOTOR_CONF_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_MOTOR_CONF_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, X_MOTOR_CONF_PROPERTY, "Configuration write failed");
	}
	//- dome.X_MOTOR_CONF.on_change
	indigo_update_property(device, X_MOTOR_CONF_PROPERTY, NULL);
}

static void dome_x_delay_conf_handler(indigo_device *device) {
	//+ dome.X_DELAY_CONF.on_change
	X_DELAY_CONF_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	talon6ror_pack(PRIVATE_DATA->configuration + 19, X_DELAY_CONF_PARK_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 22, X_DELAY_CONF_WEATHER_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 25, X_DELAY_CONF_POWER_ITEM->number.value);
	talon6ror_pack(PRIVATE_DATA->configuration + 43, X_DELAY_CONF_TIMEOUT_ITEM->number.value);
	if (talon6ror_write_configuration(device)) {
		X_DELAY_CONF_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_DELAY_CONF_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, X_DELAY_CONF_PROPERTY, "Configuration write failed");
	}
	//- dome.X_DELAY_CONF.on_change
	indigo_update_property(device, X_DELAY_CONF_PROPERTY, NULL);
}

static void dome_x_close_cond_handler(indigo_device *device) {
	//+ dome.X_CLOSE_COND.on_change
	X_CLOSE_COND_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] & 0xF8;
	PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] | (X_CLOSE_COND_POWER_ITEM->sw.value ? (1 << 0) : 0);
	PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] | (X_CLOSE_COND_WEATHER_ITEM->sw.value ? (1 << 1) : 0);
	PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] | (X_CLOSE_COND_TIMEOUT_ITEM->sw.value ? (1 << 2) : 0);
	if (talon6ror_write_configuration(device)) {
		X_CLOSE_COND_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_CLOSE_COND_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, X_CLOSE_COND_PROPERTY, "Configuration write failed");
	}
	//- dome.X_CLOSE_COND.on_change
	indigo_update_property(device, X_CLOSE_COND_PROPERTY, NULL);
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
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Talon6 ROR");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- dome.on_attach
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
		DOME_SHUTTER_PROPERTY->hidden = false;
		//+ dome.DOME_SHUTTER.on_attach
		DOME_SHUTTER_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
		INDIGO_COPY_VALUE(DOME_SHUTTER_PROPERTY->label, "Roof state");
		INDIGO_COPY_VALUE(DOME_SHUTTER_OPENED_ITEM->label, "Roof opened");
		INDIGO_COPY_VALUE(DOME_SHUTTER_CLOSED_ITEM->label, "Roof closed");
		//- dome.DOME_SHUTTER.on_attach
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		X_SENSORS_PROPERTY = indigo_init_light_property(NULL, device->name, X_SENSORS_PROPERTY_NAME, DOME_MAIN_GROUP, "Sensors", INDIGO_OK_STATE, 8);
		if (X_SENSORS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_SENSORS_POWER_CONDITION_ITEM, X_SENSORS_POWER_CONDITION_ITEM_NAME, "Power condition", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_WEATHER_CONDITION_ITEM, X_SENSORS_WEATHER_CONDITION_ITEM_NAME, "Weather condition", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_PARKED_SENSOR_ITEM, X_SENSORS_PARKED_SENSOR_ITEM_NAME, "Mount at park sensor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_OPEN_SENSOR_ITEM, X_SENSORS_OPEN_SENSOR_ITEM_NAME, "Roof open sensor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_CLOSE_SENSOR_ITEM, X_SENSORS_CLOSE_SENSOR_ITEM_NAME, "Roof closed sensor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_OPEN_BUTTON_ITEM, X_SENSORS_OPEN_BUTTON_ITEM_NAME, "Open button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_CLOSE_BUTTON_ITEM, X_SENSORS_CLOSE_BUTTON_ITEM_NAME, "Close button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_STOP_BUTTON_ITEM, X_SENSORS_STOP_BUTTON_ITEM_NAME, "Stop button pushed", INDIGO_IDLE_STATE);
		X_MOTOR_CONF_PROPERTY = indigo_init_number_property(NULL, device->name, X_MOTOR_CONF_PROPERTY_NAME, "Configuration", "Motor configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 10);
		if (X_MOTOR_CONF_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_MOTOR_CONF_KP_ITEM, X_MOTOR_CONF_KP_ITEM_NAME, "Proportional constant", 1, 1000, 1, 180);
		indigo_init_number_item(X_MOTOR_CONF_KI_ITEM, X_MOTOR_CONF_KI_ITEM_NAME, "Integral constant", 1, 1000, 1, 140);
		indigo_init_number_item(X_MOTOR_CONF_KD_ITEM, X_MOTOR_CONF_KD_ITEM_NAME, "Differential constant", 1, 1000, 1, 2);
		indigo_init_number_item(X_MOTOR_CONF_MAX_SPEED_ITEM, X_MOTOR_CONF_MAX_SPEED_ITEM_NAME, "Maximum speed (%)", 0, 100, 1, 70);
		indigo_init_number_item(X_MOTOR_CONF_MIN_SPEED_ITEM, X_MOTOR_CONF_MIN_SPEED_ITEM_NAME, "Minimum speed (%)", 0, 100, 1, 10);
		indigo_init_number_item(X_MOTOR_CONF_ACCELERATION_ITEM, X_MOTOR_CONF_ACCELERATION_ITEM_NAME, "Acceleration (%)", 0, 100, 1, 10);
		indigo_init_number_item(X_MOTOR_CONF_RAMP_ITEM, X_MOTOR_CONF_RAMP_ITEM_NAME, "Decceleration ramp (%)", 0, 100, 1, 20);
		indigo_init_number_item(X_MOTOR_CONF_FULL_ENC_ITEM, X_MOTOR_CONF_FULL_ENC_ITEM_NAME, "Full travel encoder ticks", 0, 2097151, 1, 10000);
		indigo_init_number_item(X_MOTOR_CONF_ENC_FACTOR_ITEM, X_MOTOR_CONF_ENC_FACTOR_ITEM_NAME, "Encoder factor (2 or 4)", 2, 4, 2, 4);
		indigo_init_number_item(X_MOTOR_CONF_REVERSE_ITEM, X_MOTOR_CONF_REVERSE_ITEM_NAME, "Reverse (0 or 1)", 0, 1, 1, 0);
		X_DELAY_CONF_PROPERTY = indigo_init_number_property(NULL, device->name, X_DELAY_CONF_PROPERTY_NAME, "Configuration", "Delay configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (X_DELAY_CONF_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_DELAY_CONF_PARK_ITEM, X_DELAY_CONF_PARK_ITEM_NAME, "Wait for mount park", 0, 2097151, 1, 1);
		indigo_init_number_item(X_DELAY_CONF_WEATHER_ITEM, X_DELAY_CONF_WEATHER_ITEM_NAME, "Weather condition delay", 0, 2097151, 1, 120);
		indigo_init_number_item(X_DELAY_CONF_POWER_ITEM, X_DELAY_CONF_POWER_ITEM_NAME, "Power condition delay", 0, 2097151, 1, 60);
		indigo_init_number_item(X_DELAY_CONF_TIMEOUT_ITEM, X_DELAY_CONF_TIMEOUT_ITEM_NAME, "Temporal opening delay", 0, 2097151, 1, 10);
		X_CLOSE_COND_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CLOSE_COND_PROPERTY_NAME, "Configuration", "Close conditions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (X_CLOSE_COND_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CLOSE_COND_WEATHER_ITEM, X_CLOSE_COND_WEATHER_ITEM_NAME, "Weather condition delay enabled", false);
		indigo_init_switch_item(X_CLOSE_COND_POWER_ITEM, X_CLOSE_COND_POWER_ITEM_NAME, "Power condition delay enabled", false);
		indigo_init_switch_item(X_CLOSE_COND_TIMEOUT_ITEM, X_CLOSE_COND_TIMEOUT_ITEM_NAME, "Temporal opening delay enabled", false);
		X_CLOSE_TIMER_PROPERTY = indigo_init_number_property(NULL, device->name, X_CLOSE_TIMER_PROPERTY_NAME, DOME_MAIN_GROUP, "Close timers", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_CLOSE_TIMER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_CLOSE_TIMER_WEATHER_ITEM, X_CLOSE_TIMER_WEATHER_ITEM_NAME, "Weather condition timer", 0, 16383, 1, 0);
		indigo_init_number_item(X_CLOSE_TIMER_POWER_ITEM, X_CLOSE_TIMER_POWER_ITEM_NAME, "Power condition timer", 0, 16383, 1, 0);
		indigo_init_number_item(X_CLOSE_TIMER_TIMEOUT_ITEM, X_CLOSE_TIMER_TIMEOUT_ITEM_NAME, "Temporal opening timer", 0, 2097151, 1, 0);
		X_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, X_POSITION_PROPERTY_NAME, DOME_MAIN_GROUP, "Roof position", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_POSITION_ITEM, X_POSITION_ITEM_NAME, "Roof position", 0, 2097151, 1, 0);
		X_STATUS_PROPERTY = indigo_init_number_property(NULL, device->name, X_STATUS_PROPERTY_NAME, DOME_MAIN_GROUP, "System status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_STATUS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_STATUS_VOLTAGE_ITEM, X_STATUS_VOLTAGE_ITEM_NAME, "Voltage", 0, 10000, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SENSORS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MOTOR_CONF_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_DELAY_CONF_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CLOSE_COND_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CLOSE_TIMER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_POSITION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STATUS_PROPERTY);
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
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MOTOR_CONF_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MOTOR_CONF_PROPERTY, dome_x_motor_conf_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DELAY_CONF_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_DELAY_CONF_PROPERTY, dome_x_delay_conf_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CLOSE_COND_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CLOSE_COND_PROPERTY, dome_x_close_cond_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(X_SENSORS_PROPERTY);
	indigo_release_property(X_MOTOR_CONF_PROPERTY);
	indigo_release_property(X_DELAY_CONF_PROPERTY);
	indigo_release_property(X_CLOSE_COND_PROPERTY);
	indigo_release_property(X_CLOSE_TIMER_PROPERTY);
	indigo_release_property(X_POSITION_PROPERTY);
	indigo_release_property(X_STATUS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_talon6ror(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static talon6ror_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (talon6ror_private_data *)indigo_safe_malloc(sizeof(talon6ror_private_data));
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
