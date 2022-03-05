// Copyright (c) 2021 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Talon6 driver
 \file indigo_dome_talon6ror.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_dome_talon6ror"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include <indigo/indigo_io.h>
#include <indigo/indigo_dome_driver.h>

#include "indigo_dome_talon6ror.h"

#define PRIVATE_DATA												((talon6ror_private_data *)device->private_data)

#define X_SENSORS_PROPERTY_NAME								"X_SENSORS"
#define X_SENSORS_POWER_CONDITION_ITEM_NAME		"POWER_CONDITION"
#define X_SENSORS_WEATHER_CONDITION_ITEM_NAME	"WEATHER_CONDITION"
#define X_SENSORS_PARKED_SENSOR_ITEM_NAME			"PARKED_SENSOR"
#define X_SENSORS_OPEN_SENSOR_ITEM_NAME				"OPEN_SENSOR"
#define X_SENSORS_CLOSE_SENSOR_ITEM_NAME			"CLOSE_SENSOR"
#define X_SENSORS_OPEN_BUTTON_ITEM_NAME				"OPEN_BUTTON"
#define X_SENSORS_CLOSE_BUTTON_ITEM_NAME			"CLOSE_BUTTON"
#define X_SENSORS_STOP_BUTTON_ITEM_NAME				"STOP_BUTTON"

#define X_SENSORS_PROPERTY    								(PRIVATE_DATA->status_property)
#define X_SENSORS_POWER_CONDITION_ITEM				(X_SENSORS_PROPERTY->items + 0)
#define X_SENSORS_WEATHER_CONDITION_ITEM			(X_SENSORS_PROPERTY->items + 1)
#define X_SENSORS_PARKED_SENSOR_ITEM					(X_SENSORS_PROPERTY->items + 2)
#define X_SENSORS_OPEN_SENSOR_ITEM						(X_SENSORS_PROPERTY->items + 3)
#define X_SENSORS_CLOSE_SENSOR_ITEM						(X_SENSORS_PROPERTY->items + 4)
#define X_SENSORS_OPEN_BUTTON_ITEM						(X_SENSORS_PROPERTY->items + 5)
#define X_SENSORS_CLOSE_BUTTON_ITEM						(X_SENSORS_PROPERTY->items + 6)
#define X_SENSORS_STOP_BUTTON_ITEM						(X_SENSORS_PROPERTY->items + 7)

#define X_MOTOR_CONF_PROPERTY_NAME						"X_MOTOR_CONF"
#define X_MOTOR_CONF_KP_ITEM_NAME							"KP"
#define X_MOTOR_CONF_KI_ITEM_NAME							"KI"
#define X_MOTOR_CONF_KD_ITEM_NAME							"KD"
#define X_MOTOR_CONF_MAX_SPEED_ITEM_NAME			"MAX_SPEED"
#define X_MOTOR_CONF_MIN_SPEED_ITEM_NAME			"MIN_SPEED"
#define X_MOTOR_CONF_ACCELERATION_ITEM_NAME		"ACCELERATION"
#define X_MOTOR_CONF_RAMP_ITEM_NAME						"RAMP"
#define X_MOTOR_CONF_FULL_ENC_ITEM_NAME				"FULL_ENC"
#define X_MOTOR_CONF_ENC_FACTOR_ITEM_NAME			"ENC_FACTOR"
#define X_MOTOR_CONF_REVERSE_ITEM_NAME				"REVERSE"

#define X_MOTOR_CONF_PROPERTY    							(PRIVATE_DATA->motor_conf_property)
#define X_MOTOR_CONF_KP_ITEM									(X_MOTOR_CONF_PROPERTY->items + 0)
#define X_MOTOR_CONF_KI_ITEM									(X_MOTOR_CONF_PROPERTY->items + 1)
#define X_MOTOR_CONF_KD_ITEM									(X_MOTOR_CONF_PROPERTY->items + 2)
#define X_MOTOR_CONF_MAX_SPEED_ITEM						(X_MOTOR_CONF_PROPERTY->items + 3)
#define X_MOTOR_CONF_MIN_SPEED_ITEM						(X_MOTOR_CONF_PROPERTY->items + 4)
#define X_MOTOR_CONF_ACCELERATION_ITEM				(X_MOTOR_CONF_PROPERTY->items + 5)
#define X_MOTOR_CONF_RAMP_ITEM								(X_MOTOR_CONF_PROPERTY->items + 6)
#define X_MOTOR_CONF_FULL_ENC_ITEM						(X_MOTOR_CONF_PROPERTY->items + 7)
#define X_MOTOR_CONF_ENC_FACTOR_ITEM					(X_MOTOR_CONF_PROPERTY->items + 8)
#define X_MOTOR_CONF_REVERSE_ITEM							(X_MOTOR_CONF_PROPERTY->items + 9)

#define X_DELAY_CONF_PROPERTY_NAME						"X_DELAY_CONF"
#define X_DELAY_CONF_PARK_ITEM_NAME						"PARK"
#define X_DELAY_CONF_WEATHER_ITEM_NAME				"WEATHER"
#define X_DELAY_CONF_POWER_ITEM_NAME					"POWER"
#define X_DELAY_CONF_TIMEOUT_ITEM_NAME				"TIMEOUT"

#define X_DELAY_CONF_PROPERTY									(PRIVATE_DATA->delay_conf_property)
#define X_DELAY_CONF_PARK_ITEM								(X_DELAY_CONF_PROPERTY->items + 0)
#define X_DELAY_CONF_WEATHER_ITEM							(X_DELAY_CONF_PROPERTY->items + 1)
#define X_DELAY_CONF_POWER_ITEM								(X_DELAY_CONF_PROPERTY->items + 2)
#define X_DELAY_CONF_TIMEOUT_ITEM							(X_DELAY_CONF_PROPERTY->items + 3)

#define X_CLOSE_COND_PROPERTY_NAME						"X_CLOSE_COND"
#define X_CLOSE_COND_WEATHER_ITEM_NAME				"WEATHER"
#define X_CLOSE_COND_POWER_ITEM_NAME					"POWER"
#define X_CLOSE_COND_TIMEOUT_ITEM_NAME				"TIMEOUT"

#define X_CLOSE_COND_PROPERTY									(PRIVATE_DATA->close_cond_property)
#define X_CLOSE_COND_WEATHER_ITEM							(X_CLOSE_COND_PROPERTY->items + 0)
#define X_CLOSE_COND_POWER_ITEM								(X_CLOSE_COND_PROPERTY->items + 1)
#define X_CLOSE_COND_TIMEOUT_ITEM							(X_CLOSE_COND_PROPERTY->items + 2)

#define X_CLOSE_TIMER_PROPERTY_NAME						"X_TIMER_COND"
#define X_CLOSE_TIMER_WEATHER_ITEM_NAME				"WEATHER"
#define X_CLOSE_TIMER_POWER_ITEM_NAME					"POWER"
#define X_CLOSE_TIMER_TIMEOUT_ITEM_NAME				"TIMEOUT"

#define X_CLOSE_TIMER_PROPERTY								(PRIVATE_DATA->close_timer_property)
#define X_CLOSE_TIMER_WEATHER_ITEM						(X_CLOSE_TIMER_PROPERTY->items + 0)
#define X_CLOSE_TIMER_POWER_ITEM							(X_CLOSE_TIMER_PROPERTY->items + 1)
#define X_CLOSE_TIMER_TIMEOUT_ITEM						(X_CLOSE_TIMER_PROPERTY->items + 2)

#define X_POSITION_PROPERTY_NAME							"X_POSITION_PROPERTY"
#define X_POSITION_ITEM_NAME									"POSITION"

#define X_POSITION_PROPERTY										(PRIVATE_DATA->position_property)
#define X_POSITION_ITEM												(X_POSITION_PROPERTY->items + 0)

#define X_STATUS_PROPERTY_NAME								"X_STATUS_PROPERTY"
#define X_STATUS_VOLTAGE_ITEM_NAME						"VOLTAGE"

#define X_STATUS_PROPERTY											(PRIVATE_DATA->voltage_property)
#define X_STATUS_VOLTAGE_ITEM									(X_STATUS_PROPERTY->items + 0)

#define RESPONSE_LENGTH		64

typedef struct {
	int handle;
	pthread_mutex_t mutex;
	indigo_property *status_property;
	indigo_property *motor_conf_property;
	indigo_property *delay_conf_property;
	indigo_property *close_cond_property;
	indigo_property *close_timer_property;
	indigo_property *position_property;
	indigo_property *voltage_property;
	indigo_timer *state_timer;
	char *last_action;
	uint16_t sensors;
	int position;
	int power_timer;
	int weather_timer;
	int timeout_timer;
	double voltage;
	uint8_t configuration[RESPONSE_LENGTH];
} talon6ror_private_data;

static char *last_action_string[] = {
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
	"Stop requested",
	"Emergency stop",
	"Mount park requested"
};

// -------------------------------------------------------------------------------- serial interface

static void talon6ror_pack(uint8_t *string, int value) {
	string[0] = 0x80 | ((value >> 14) & 0x7F);
	string[1] = 0x80 | ((value >> 7) & 0x7F);
	string[2] = 0x80 | ((value >> 0) & 0x7F);
}

static int talon6ror_unpack(uint8_t *string) {
	return ((string[0] & 0x7F) << 14) | ((string[1] & 0x7F) << 7) | (string[2] & 0x7F);
}

static bool talon6ror_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
	if (PRIVATE_DATA->handle < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
	char c;
	struct timeval tv;
		// flush
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0)
			break;
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			return false;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			return false;
		}
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> flushed", PRIVATE_DATA->handle);
	return true;
}

static char *dump_hex(const uint8_t *data) {
	static char buffer[RESPONSE_LENGTH * 3];
	buffer[0] = (char)data[0];
	buffer[1] = 0;
	for (int i = 1; data[i]; i++)
		sprintf(buffer + i * 3 - 2, " %02X", data[i]);
	return buffer;
}

static bool talon6ror_command(indigo_device *device, const char *command, uint8_t *response) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int result = indigo_printf(PRIVATE_DATA->handle, "&%s%%#", command);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d <- \"%s\" (%s)", PRIVATE_DATA->handle, dump_hex((uint8_t *)command), result ? "OK" : strerror(errno));
	if (result) {
		uint8_t c, *pnt = response;
		*pnt = 0;
		result = false;
		bool start = false;
		while (pnt - response < RESPONSE_LENGTH) {
			if (indigo_read(PRIVATE_DATA->handle,(char *) &c, 1) < 1) {
				if (pnt)
					*pnt = 0;
				break;
			}
			if (c == '&') {
				start = true;
				continue;
			}
			if (!start) {
				continue;
			}
			if (c == '#') {
				if (pnt)
					*pnt = 0;
				result = true;
				break;
			}
			if (pnt)
				*pnt++ = c;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> \"%s\" (%s)", PRIVATE_DATA->handle, dump_hex(response), result ? "OK" : strerror(errno));
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return result;
}

static void talon6ror_close(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
	}
}

static void talon6ror_get_status(indigo_device *device) {
	uint8_t response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "G", response) && (response[0] == 'G')) {
		int sum = 0;
		for (int i = 1; i < 21; i++)
			sum += response[i];
		if (response[21] == (uint8_t)(0x80 | -(sum % 128))) {
			switch (response[1] & 0x70) {
				case 0x00:
					if (DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE || !DOME_SHUTTER_OPENED_ITEM->sw.value) {
						DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
						indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
						indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof opened");
					}
					break;
				case 0x10:
					if (DOME_SHUTTER_PROPERTY->state != INDIGO_OK_STATE || !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
						DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
						indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
						indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof closed");
					}
					break;
				case 0x20:
					if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE || !DOME_SHUTTER_OPENED_ITEM->sw.value) {
						DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
						indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof opening");
					}
					break;
				case 0x30:
					if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE || !DOME_SHUTTER_CLOSED_ITEM->sw.value) {
						DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
						indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof closing");
					}
					break;
				default:
					if (DOME_SHUTTER_PROPERTY->state != INDIGO_ALERT_STATE) {
						DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Error reported");
					}
					break;
			}
			char *last_action = last_action_string[response[1] & 0x0F];
			if (PRIVATE_DATA->last_action != last_action) {
				indigo_send_message(device, last_action);
				PRIVATE_DATA->last_action = last_action;
			}
			PRIVATE_DATA->position = talon6ror_unpack(response + 2);
			if (X_POSITION_ITEM->number.value != PRIVATE_DATA->position) {
				X_POSITION_ITEM->number.value = PRIVATE_DATA->position;
				X_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_POSITION_PROPERTY, NULL);
			}
			PRIVATE_DATA->voltage = round((((response[5] & 0x7F) << 7) | (response[6] & 0x7F)) * 150.0 / 1024) / 10.0;
			if (X_STATUS_VOLTAGE_ITEM->number.value != PRIVATE_DATA->voltage) {
				X_STATUS_VOLTAGE_ITEM->number.value = PRIVATE_DATA->voltage;
				X_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_STATUS_PROPERTY, NULL);
			}
			PRIVATE_DATA->timeout_timer = talon6ror_unpack(response + 7);
			PRIVATE_DATA->power_timer = ((response[10] & 0x7F) << 7) | (response[11] & 0x7F);
			PRIVATE_DATA->weather_timer = ((response[12] & 0x7F) << 7) | (response[13] & 0x7F);
			if (X_CLOSE_TIMER_TIMEOUT_ITEM->number.value != PRIVATE_DATA->timeout_timer || X_CLOSE_TIMER_POWER_ITEM->number.value != PRIVATE_DATA->power_timer || X_CLOSE_TIMER_WEATHER_ITEM->number.value != PRIVATE_DATA->weather_timer) {
				X_CLOSE_TIMER_TIMEOUT_ITEM->number.value = PRIVATE_DATA->timeout_timer;
				X_CLOSE_TIMER_POWER_ITEM->number.value = PRIVATE_DATA->power_timer;
				X_CLOSE_TIMER_WEATHER_ITEM->number.value = PRIVATE_DATA->weather_timer;
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
				indigo_update_property(device, X_SENSORS_PROPERTY, NULL);
				PRIVATE_DATA->sensors = sensors;
			}
		} else {
			if (DOME_SHUTTER_PROPERTY->state != INDIGO_ALERT_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Checksum error");
			}
		}
	}
	indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->state_timer);
}

// -------------------------------------------------------------------------------- async handlers

static void dome_connect_handler(indigo_device *device) {
	uint8_t response[RESPONSE_LENGTH];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!talon6ror_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (talon6ror_command(device, "V", response) && response[0] == 'V') {
				strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, (char *)response + 1, 7);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			}
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (talon6ror_command(device, "p", response) && response[0] == 'p') {
				int sum = 0;
				for (int i = 1; i < 55; i++)
					sum += response[i];
				if (response[55] == (uint8_t)(0x80 | -(sum % 128))) {
					memcpy(PRIVATE_DATA->configuration, response, sizeof(PRIVATE_DATA->configuration));
					PRIVATE_DATA->configuration[0] = 'a';
					X_MOTOR_CONF_KP_ITEM->number.value = talon6ror_unpack(response + 1);
					X_MOTOR_CONF_KI_ITEM->number.value = talon6ror_unpack(response + 4);
					X_MOTOR_CONF_KD_ITEM->number.value = talon6ror_unpack(response + 7);
					X_MOTOR_CONF_MAX_SPEED_ITEM->number.value = talon6ror_unpack(response + 10);
					X_MOTOR_CONF_MIN_SPEED_ITEM->number.value = talon6ror_unpack(response + 13);
					X_MOTOR_CONF_ACCELERATION_ITEM->number.value = talon6ror_unpack(response + 16);
					X_DELAY_CONF_PARK_ITEM->number.value = talon6ror_unpack(response + 19);
					X_DELAY_CONF_WEATHER_ITEM->number.value = talon6ror_unpack(response + 22);
					X_DELAY_CONF_POWER_ITEM->number.value = talon6ror_unpack(response + 25);
					// DEL_COM 28
					// MAX_OPEN 31
					// IPSWITCH 34
					X_MOTOR_CONF_FULL_ENC_ITEM->number.value = talon6ror_unpack(response + 37);
					X_MOTOR_CONF_RAMP_ITEM->number.value = talon6ror_unpack(response + 40);
					X_DELAY_CONF_TIMEOUT_ITEM->number.value = talon6ror_unpack(response + 43);
					X_MOTOR_CONF_ENC_FACTOR_ITEM->number.value = response[46] & 0x7F;
					X_MOTOR_CONF_REVERSE_ITEM->number.value = response[47] & 0x0F;
					X_CLOSE_COND_POWER_ITEM->sw.value = (response[48] & (1 << 0)) != 0;
					X_CLOSE_COND_WEATHER_ITEM->sw.value = (response[48] & (1 << 1)) != 0;
					X_CLOSE_COND_TIMEOUT_ITEM->sw.value = (response[48] & (1 << 2)) != 0;
					// DUMMY 49
					// TWTIME 52
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Checksum error, handshake failed");
					indigo_send_message(device, "Checksum error, handshake failed");
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
			}
		}
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, X_SENSORS_PROPERTY, NULL);
			indigo_define_property(device, X_MOTOR_CONF_PROPERTY, NULL);
			indigo_define_property(device, X_DELAY_CONF_PROPERTY, NULL);
			indigo_define_property(device, X_CLOSE_COND_PROPERTY, NULL);
			indigo_define_property(device, X_CLOSE_TIMER_PROPERTY, NULL);
			indigo_define_property(device, X_POSITION_PROPERTY, NULL);
			indigo_define_property(device, X_STATUS_PROPERTY, NULL);
			indigo_set_timer(device, 0, talon6ror_get_status, &PRIVATE_DATA->state_timer);
		} else {
			talon6ror_close(device);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device,  &PRIVATE_DATA->state_timer);
		if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		talon6ror_close(device);
		indigo_delete_property(device, X_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, X_MOTOR_CONF_PROPERTY, NULL);
		indigo_delete_property(device, X_DELAY_CONF_PROPERTY, NULL);
		indigo_delete_property(device, X_CLOSE_COND_PROPERTY, NULL);
		indigo_delete_property(device, X_CLOSE_TIMER_PROPERTY, NULL);
		indigo_delete_property(device, X_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, X_STATUS_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_open_handler(indigo_device *device) {
	uint8_t response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "O", response)) {
		while (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_usleep(500000);
		}
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_close_handler(indigo_device *device) {
	uint8_t response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "P", response)) {
		while (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_usleep(500000);
		}
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_abort_handler(indigo_device *device) {
	uint8_t response[RESPONSE_LENGTH];
	if (talon6ror_command(device, "S", response))
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	else
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void write_configuration_handler(indigo_device *device) {
	uint8_t response[RESPONSE_LENGTH];
	int checksum = 0;
	for (int i = 1; i< 55; i++)
		checksum += PRIVATE_DATA->configuration[i];
	PRIVATE_DATA->configuration[55] = -(checksum % 128);
	if (talon6ror_command(device, (char *)PRIVATE_DATA->configuration, response)) {
		if (X_MOTOR_CONF_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_MOTOR_CONF_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_MOTOR_CONF_PROPERTY, NULL);
		}
		if (X_DELAY_CONF_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_DELAY_CONF_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_DELAY_CONF_PROPERTY, NULL);
		}
		if (X_CLOSE_COND_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_CLOSE_COND_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_CLOSE_COND_PROPERTY, NULL);
		}
	} else {
		if (X_MOTOR_CONF_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_MOTOR_CONF_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_MOTOR_CONF_PROPERTY, NULL);
		}
		if (X_DELAY_CONF_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_DELAY_CONF_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_DELAY_CONF_PROPERTY, NULL);
		}
		if (X_CLOSE_COND_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_CLOSE_COND_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CLOSE_COND_PROPERTY, NULL);
		}
	}
}

// -------------------------------------------------------------------------------- INDIGO dome device implementation

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Talon6");
		// -------------------------------------------------------------------------------- standard properties
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Talon6 ROR");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_EQUATORIAL_COORDINATES_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
		DOME_SHUTTER_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
		indigo_copy_value(DOME_SHUTTER_PROPERTY->label, "Roof state");
		indigo_copy_value(DOME_SHUTTER_OPENED_ITEM->label, "Roof opened");
		indigo_copy_value(DOME_SHUTTER_CLOSED_ITEM->label, "Roof closed");
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_SENSORS
		X_SENSORS_PROPERTY = indigo_init_light_property(NULL, device->name, X_SENSORS_PROPERTY_NAME, DOME_MAIN_GROUP, "Sensors", INDIGO_OK_STATE, 8);
		if (X_SENSORS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(X_SENSORS_POWER_CONDITION_ITEM, X_SENSORS_POWER_CONDITION_ITEM_NAME, "Power condition", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_WEATHER_CONDITION_ITEM, X_SENSORS_WEATHER_CONDITION_ITEM_NAME, "Weather condition", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_PARKED_SENSOR_ITEM, X_SENSORS_PARKED_SENSOR_ITEM_NAME, "Mount at park sensor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_OPEN_SENSOR_ITEM, X_SENSORS_OPEN_SENSOR_ITEM_NAME, "Roof open sensor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_CLOSE_SENSOR_ITEM, X_SENSORS_CLOSE_SENSOR_ITEM_NAME, "Roof closed sensor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_OPEN_BUTTON_ITEM, X_SENSORS_OPEN_BUTTON_ITEM_NAME, "Open button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_CLOSE_BUTTON_ITEM, X_SENSORS_CLOSE_BUTTON_ITEM_NAME, "Close button pushed", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SENSORS_STOP_BUTTON_ITEM, X_SENSORS_STOP_BUTTON_ITEM_NAME, "Stop button pushed", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------------- X_MOTOR_CONF
		X_MOTOR_CONF_PROPERTY = indigo_init_number_property(NULL, device->name, X_MOTOR_CONF_PROPERTY_NAME, "Configuration", "Motor configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 10);
		if (X_MOTOR_CONF_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_MOTOR_CONF_KP_ITEM, X_MOTOR_CONF_KP_ITEM_NAME, "Proportional constant", 1, 1000, 1, 180);
		indigo_init_number_item(X_MOTOR_CONF_KI_ITEM, X_MOTOR_CONF_KI_ITEM_NAME, "Integral constant", 1, 1000, 1, 140);
		indigo_init_number_item(X_MOTOR_CONF_KD_ITEM, X_MOTOR_CONF_KD_ITEM_NAME, "Differential constant", 1, 1000, 1, 2);
		indigo_init_number_item(X_MOTOR_CONF_MAX_SPEED_ITEM, X_MOTOR_CONF_MAX_SPEED_ITEM_NAME, "Maximum speed (%)", 0, 100, 1, 70);
		indigo_init_number_item(X_MOTOR_CONF_MIN_SPEED_ITEM, X_MOTOR_CONF_MIN_SPEED_ITEM_NAME, "Minimum speed (%)", 0, 100, 1, 10);
		indigo_init_number_item(X_MOTOR_CONF_ACCELERATION_ITEM, X_MOTOR_CONF_ACCELERATION_ITEM_NAME, "Acceleration (%)", 0, 100, 1, 10);
		indigo_init_number_item(X_MOTOR_CONF_RAMP_ITEM, X_MOTOR_CONF_RAMP_ITEM_NAME, "Decceleration ramp (%)", 0, 100, 1, 20);
		indigo_init_number_item(X_MOTOR_CONF_FULL_ENC_ITEM, X_MOTOR_CONF_FULL_ENC_ITEM_NAME, "Full travel encoder ticks", 0, INT32_MAX, 1, 10000);
		indigo_init_number_item(X_MOTOR_CONF_ENC_FACTOR_ITEM, X_MOTOR_CONF_ENC_FACTOR_ITEM_NAME, "Encoder factor (2 or 4)", 2, 4, 2, 4);
		indigo_init_number_item(X_MOTOR_CONF_REVERSE_ITEM, X_MOTOR_CONF_REVERSE_ITEM_NAME, "Reverse (0 or 1)", 0, 1, 0, 0);
		// -------------------------------------------------------------------------------- X_DELAY_CONF
		X_DELAY_CONF_PROPERTY = indigo_init_number_property(NULL, device->name, X_DELAY_CONF_PROPERTY_NAME, "Configuration", "Delay configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (X_MOTOR_CONF_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_DELAY_CONF_PARK_ITEM, X_DELAY_CONF_PARK_ITEM_NAME, "Wait for mount park", 1, 0x377, 1, 1);
		indigo_init_number_item(X_DELAY_CONF_WEATHER_ITEM, X_DELAY_CONF_WEATHER_ITEM_NAME, "Weather condition delay", 1, 0x377, 1, 120);
		indigo_init_number_item(X_DELAY_CONF_POWER_ITEM, X_DELAY_CONF_POWER_ITEM_NAME, "Power condition delay", 1, 0x377, 1, 60);
		indigo_init_number_item(X_DELAY_CONF_TIMEOUT_ITEM, X_DELAY_CONF_TIMEOUT_ITEM_NAME, "Temporal opening delay", 1, 0x377, 1, 10);
		// -------------------------------------------------------------------------------- X_CLOSE_COND
		X_CLOSE_COND_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CLOSE_COND_PROPERTY_NAME, "Configuration", "Close conditions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (X_MOTOR_CONF_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CLOSE_COND_WEATHER_ITEM, X_CLOSE_COND_WEATHER_ITEM_NAME, "Weather condition delay enabled", false);
		indigo_init_switch_item(X_CLOSE_COND_POWER_ITEM, X_CLOSE_COND_POWER_ITEM_NAME, "Power condition delay enabled", false);
		indigo_init_switch_item(X_CLOSE_COND_TIMEOUT_ITEM, X_CLOSE_COND_TIMEOUT_ITEM_NAME, "Temporal opening delay enabled", false);
		// -------------------------------------------------------------------------------- X_DELAY_CONF
		X_CLOSE_TIMER_PROPERTY = indigo_init_number_property(NULL, device->name, X_CLOSE_TIMER_PROPERTY_NAME, DOME_MAIN_GROUP, "Close timers", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_CLOSE_TIMER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_CLOSE_TIMER_WEATHER_ITEM, X_CLOSE_TIMER_WEATHER_ITEM_NAME, "Weather condition timer", 0, 0x377, 1, 0);
		indigo_init_number_item(X_CLOSE_TIMER_POWER_ITEM, X_CLOSE_TIMER_POWER_ITEM_NAME, "Power condition timer", 0, 0x377, 1, 0);
		indigo_init_number_item(X_CLOSE_TIMER_TIMEOUT_ITEM, X_CLOSE_TIMER_TIMEOUT_ITEM_NAME, "Temporal opening timer", 0, 0x377, 1, 0);
		// -------------------------------------------------------------------------------- X_POSITION
		X_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, X_POSITION_PROPERTY_NAME, DOME_MAIN_GROUP, "Roof position", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_POSITION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_POSITION_ITEM, X_POSITION_ITEM_NAME, "Roof position", 0, 10000, 1, 0);
		// -------------------------------------------------------------------------------- X_VOLTAGE
		X_STATUS_PROPERTY = indigo_init_number_property(NULL, device->name, X_STATUS_PROPERTY_NAME, DOME_MAIN_GROUP, "System status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_STATUS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_STATUS_VOLTAGE_ITEM, X_STATUS_VOLTAGE_ITEM_NAME, "Voltage", 0, 10000, 1, 0);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_SENSORS_PROPERTY, property))
			indigo_define_property(device, X_SENSORS_PROPERTY, NULL);
		if (indigo_property_match(X_MOTOR_CONF_PROPERTY, property))
			indigo_define_property(device, X_MOTOR_CONF_PROPERTY, NULL);
		if (indigo_property_match(X_DELAY_CONF_PROPERTY, property))
			indigo_define_property(device, X_DELAY_CONF_PROPERTY, NULL);
		if (indigo_property_match(X_CLOSE_COND_PROPERTY, property))
			indigo_define_property(device, X_CLOSE_COND_PROPERTY, NULL);
		if (indigo_property_match(X_CLOSE_TIMER_PROPERTY, property))
			indigo_define_property(device, X_CLOSE_TIMER_PROPERTY, NULL);
		if (indigo_property_match(X_POSITION_PROPERTY, property))
			indigo_define_property(device, X_POSITION_PROPERTY, NULL);
		if (indigo_property_match(X_STATUS_PROPERTY, property))
			indigo_define_property(device, X_STATUS_PROPERTY, NULL);

	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, dome_connect_handler, NULL);
		return INDIGO_OK;

	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, dome_abort_handler, NULL);
		} else {
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
			if (DOME_SHUTTER_OPENED_ITEM->sw.value && DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
				indigo_set_timer(device, 0, dome_open_handler, NULL);
			} else if (DOME_SHUTTER_CLOSED_ITEM->sw.value && DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
				indigo_set_timer(device, 0, dome_close_handler, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_MOTOR_CONF_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_MOTOR_CONF
		if (X_MOTOR_CONF_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(X_MOTOR_CONF_PROPERTY, property, false);
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
			X_MOTOR_CONF_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_MOTOR_CONF_PROPERTY, NULL);
			indigo_set_timer(device, 0, write_configuration_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_DELAY_CONF_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_DELAY_CONF
		if (X_DELAY_CONF_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(X_DELAY_CONF_PROPERTY, property, false);
			talon6ror_pack(PRIVATE_DATA->configuration + 19, X_DELAY_CONF_PARK_ITEM->number.value);
			talon6ror_pack(PRIVATE_DATA->configuration + 22, X_DELAY_CONF_WEATHER_ITEM->number.value);
			talon6ror_pack(PRIVATE_DATA->configuration + 25, X_DELAY_CONF_POWER_ITEM->number.value);
			talon6ror_pack(PRIVATE_DATA->configuration + 43, X_DELAY_CONF_TIMEOUT_ITEM->number.value);
			X_DELAY_CONF_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_DELAY_CONF_PROPERTY, NULL);
			indigo_set_timer(device, 0, write_configuration_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_CLOSE_COND_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CLOSE_COND
		if (X_CLOSE_COND_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(X_CLOSE_COND_PROPERTY, property, false);
			PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] & 0xF8;
			PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] | (X_CLOSE_COND_POWER_ITEM->sw.value ? (1 << 0) : 0);
			PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] | (X_CLOSE_COND_WEATHER_ITEM->sw.value ? (1 << 1) : 0);
			PRIVATE_DATA->configuration[48] = PRIVATE_DATA->configuration[48] | (X_CLOSE_COND_TIMEOUT_ITEM->sw.value ? (1 << 2) : 0);
			X_CLOSE_COND_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_CLOSE_COND_PROPERTY, NULL);
			indigo_set_timer(device, 0, write_configuration_handler, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connect_handler(device);
	}
	indigo_release_property(X_SENSORS_PROPERTY);
	indigo_release_property(X_MOTOR_CONF_PROPERTY);
	indigo_release_property(X_DELAY_CONF_PROPERTY);
	indigo_release_property(X_CLOSE_COND_PROPERTY);
	indigo_release_property(X_CLOSE_TIMER_PROPERTY);
	indigo_release_property(X_POSITION_PROPERTY);
	indigo_release_property(X_STATUS_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static talon6ror_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_talon6ror(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		"Talon6 ROR",
		dome_attach,
		dome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Talon6 ROR", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(talon6ror_private_data));
			dome = indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				free(dome);
				dome = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
