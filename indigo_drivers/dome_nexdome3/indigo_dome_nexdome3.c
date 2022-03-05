// Copyright (c) 2019 Rumen G. Bgdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumen@skyarchive.org>

/** INDIGO DOME NexDome (with Firmware v3) driver
 \file indigo_dome_nexdome3.c
 */

#define DRIVER_VERSION 0x0000B
#define DRIVER_NAME    "indigo_dome_nexdome3"

#define FIRMWARE_VERSION_3_2 0x0302

// Uncomment to be able to send custom commands -> displays NEXDOME_COMMAND property
// #define CMD_AID

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_io.h>

#include "indigo_dome_nexdome3.h"

// gp_bits is used as boolean
#define is_connected                              gp_bits

#define PRIVATE_DATA                              ((nexdome_private_data *)device->private_data)

#define NEXDOME_SETTINGS_GROUP                    "Settings"

#define NEXDOME_FIND_HOME_PROPERTY                (PRIVATE_DATA->find_home_property)
#define NEXDOME_FIND_HOME_ITEM                    (NEXDOME_FIND_HOME_PROPERTY->items+0)
#define NEXDOME_FIND_HOME_PROPERTY_NAME           "NEXDOME_FIND_HOME"
#define NEXDOME_FIND_HOME_ITEM_NAME               "FIND_HOME"

#define NEXDOME_HOME_POSITION_PROPERTY               (PRIVATE_DATA->home_position_property)
#define NEXDOME_HOME_POSITION_ITEM                   (NEXDOME_HOME_POSITION_PROPERTY->items+0)
#define NEXDOME_HOME_POSITION_PROPERTY_NAME          "NEXDOME_HOME_POSITION"
#define NEXDOME_HOME_POSITION_ITEM_NAME              "POSITION"

#define NEXDOME_ACCELERATION_PROPERTY                (PRIVATE_DATA->acceleration_property)
#define NEXDOME_ACCELERATION_ROTATOR_ITEM            (NEXDOME_ACCELERATION_PROPERTY->items+0)
#define NEXDOME_ACCELERATION_SHUTTER_ITEM            (NEXDOME_ACCELERATION_PROPERTY->items+1)
#define NEXDOME_ACCELERATION_PROPERTY_NAME           "NEXDOME_ACCELERATION_TIME"
#define NEXDOME_ACCELERATION_ROTATOR_ITEM_NAME       "ROTATOR"
#define NEXDOME_ACCELERATION_SHUTTER_ITEM_NAME       "SHUTTER"

#define NEXDOME_VELOCITY_PROPERTY                    (PRIVATE_DATA->velocity_property)
#define NEXDOME_VELOCITY_ROTATOR_ITEM                (NEXDOME_VELOCITY_PROPERTY->items+0)
#define NEXDOME_VELOCITY_SHUTTER_ITEM                (NEXDOME_VELOCITY_PROPERTY->items+1)
#define NEXDOME_VELOCITY_PROPERTY_NAME               "NEXDOME_VELOCITY"
#define NEXDOME_VELOCITY_ROTATOR_ITEM_NAME           "ROTATOR"
#define NEXDOME_VELOCITY_SHUTTER_ITEM_NAME           "SHUTTER"

#define NEXDOME_RANGE_PROPERTY                       (PRIVATE_DATA->range_property)
#define NEXDOME_RANGE_ROTATOR_ITEM                   (NEXDOME_RANGE_PROPERTY->items+0)
#define NEXDOME_RANGE_SHUTTER_ITEM                   (NEXDOME_RANGE_PROPERTY->items+1)
#define NEXDOME_RANGE_PROPERTY_NAME                  "NEXDOME_RANGE"
#define NEXDOME_RANGE_ROTATOR_ITEM_NAME              "ROTATOR"
#define NEXDOME_RANGE_SHUTTER_ITEM_NAME              "SHUTTER"

#define NEXDOME_MOVE_THRESHOLD_PROPERTY              (PRIVATE_DATA->move_threshold_property)
#define NEXDOME_MOVE_THRESHOLD_ITEM                  (NEXDOME_MOVE_THRESHOLD_PROPERTY->items+0)
#define NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME         "NEXDOME_MOVE_THRESHOLD"
#define NEXDOME_MOVE_THRESHOLD_ITEM_NAME             "THRESHOLD"

#define NEXDOME_POWER_PROPERTY                    (PRIVATE_DATA->power_property)
#define NEXDOME_POWER_VOLTAGE_ITEM                (NEXDOME_POWER_PROPERTY->items+0)
#define NEXDOME_POWER_PROPERTY_NAME               "NEXDOME_BATTERY_POWER"
#define NEXDOME_POWER_VOLTAGE_ITEM_NAME           "VOLTAGE"

#define NEXDOME_SETTINGS_PROPERTY                 (PRIVATE_DATA->settings_property)
#define NEXDOME_SETTINGS_LOAD_ITEM                (NEXDOME_SETTINGS_PROPERTY->items+0)
#define NEXDOME_SETTINGS_SAVE_ITEM                (NEXDOME_SETTINGS_PROPERTY->items+1)
#define NEXDOME_SETTINGS_DEFAULT_ITEM             (NEXDOME_SETTINGS_PROPERTY->items+2)
#define NEXDOME_SETTINGS_PROPERTY_NAME            "NEXDOME_SETTINGS"
#define NEXDOME_SETTINGS_LOAD_ITEM_NAME           "LOAD_EEPROM"
#define NEXDOME_SETTINGS_SAVE_ITEM_NAME           "SAVE_EEPROM"
#define NEXDOME_SETTINGS_DEFAULT_ITEM_NAME        "LOAD_DEFAULT"

#define NEXDOME_RAIN_PROPERTY                     (PRIVATE_DATA->rain_property)
#define NEXDOME_RAIN_ALERT_ITEM                   (NEXDOME_RAIN_PROPERTY->items+0)
#define NEXDOME_RAIN_PROPERTY_NAME                "NEXDOME_RAIN_SENSOR"
#define NEXDOME_RAIN_ALERT_ITEM_NAME              "RAIN_ALERT"

#define NEXDOME_XB_STATE_PROPERTY                 (PRIVATE_DATA->xb_state_property)
#define NEXDOME_XB_STATE_ITEM                     (NEXDOME_XB_STATE_PROPERTY->items+0)
#define NEXDOME_XB_STATE_PROPERTY_NAME            "NEXDOME_XB_STATE"
#define NEXDOME_XB_STATE_ITEM_NAME                "XB_STATE"

#ifdef CMD_AID
#define NEXDOME_COMMAND_PROPERTY                  (PRIVATE_DATA->command_property)
#define NEXDOME_COMMAND_ITEM                      (NEXDOME_COMMAND_PROPERTY->items+0)
#define NEXDOME_COMMAND_PROPERTY_NAME             "NEXDOME_COMMAND"
#define NEXDOME_COMMAND_ITEM_NAME                 "COMMAND"
#endif

// Low Voltage threshold taken from INDI
# define VOLT_THRESHOLD (7.5)

typedef struct {
	int handle;
	int version;
	float steps_per_degree;
	bool park_requested;
	bool callibration_requested;
	bool shutter_stop_requested;
	bool rotator_stop_requested;
	float park_azimuth;
	pthread_mutex_t port_r_mutex;
	pthread_mutex_t port_w_mutex;
	pthread_mutex_t property_mutex;
	indigo_timer *dome_event;
	indigo_property *find_home_property;
	indigo_property *home_position_property;
	indigo_property *move_threshold_property;
	indigo_property *power_property;
	indigo_property *acceleration_property;
	indigo_property *velocity_property;
	indigo_property *range_property;
	indigo_property *settings_property;
	indigo_property *rain_property;
	indigo_property *xb_state_property;
#ifdef CMD_AID
	indigo_property *command_property;
#endif
} nexdome_private_data;

#define NEXDOME_CMD_LEN 100


#define IN_PARK_POSITION (indigo_azimuth_distance(PRIVATE_DATA->park_azimuth, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value) <= 1)

#define PROPERTY_LOCK()    pthread_mutex_lock(&PRIVATE_DATA->property_mutex)
#define PROPERTY_UNLOCK()  pthread_mutex_unlock(&PRIVATE_DATA->property_mutex)

static bool nexdome_command(indigo_device *device, const char *command) {
	if (!command) return false;
	char wrapped_command[NEXDOME_CMD_LEN];
	snprintf(wrapped_command, NEXDOME_CMD_LEN, "@%s\n", command);
	pthread_mutex_lock(&PRIVATE_DATA->port_w_mutex);
	indigo_write(PRIVATE_DATA->handle, wrapped_command, strlen(wrapped_command));
	pthread_mutex_unlock(&PRIVATE_DATA->port_w_mutex);
	return true;
}


static bool nexdome_get_message(indigo_device *device, char *response, int max) {
	char c;
	/*
	fd_set readout;
	struct timeval tv;
	FD_ZERO(&readout);
	FD_SET(PRIVATE_DATA->handle, &readout);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
	if (result <= 0) return false;
	*/
	pthread_mutex_lock(&PRIVATE_DATA->port_r_mutex);
	if (response != NULL) {
		int index = 0;
		while (index < max) {
			if (read(PRIVATE_DATA->handle, &c, 1) < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_r_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				return false;
			}

			/* Skip empty lines as messages end with "#\n" but we should not relay
			   on "\n" as it may be removed in future !
			*/
			if ((index == 0) && ((c == '\n') || (c == '\r'))) continue;

			response[index++] = c;
			if ((c == '\n') || (c == '\r') || (c == '#'))
				break;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_r_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Response -> %s", response != NULL ? response : "NULL");
	return true;
}


static bool nexdome_handshake(indigo_device *device, char *firmware) {
	if(!firmware) return false;
	char response[255];
	nexdome_command(device, "FRR");
	/* I hope in 30 messages response will be sent */
	for(int i = 0; i < 30; i++) {
		if (!nexdome_get_message(device, response, sizeof(response))) return false;
		if (!strncmp(":FR", response, 3)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", response);
			char *end = strchr(response, '#');
			if (end) {
				*end = '\0';
				strcpy(firmware, response+3);
				return true;
			}
			return false;
		}
	}
	return false;
}


static void request_settings(indigo_device *device) {
	nexdome_command(device, "ARR");
	nexdome_command(device, "ARS");
	nexdome_command(device, "DRR");
	nexdome_command(device, "HRR");
	nexdome_command(device, "PRR");
	nexdome_command(device, "PRS");
	nexdome_command(device, "VRR");
	nexdome_command(device, "VRS");
	nexdome_command(device, "RRR");
	nexdome_command(device, "RRS");
}

// =============================================================================

static void dome_rotator_status_request(indigo_device *device) {
	if (IS_CONNECTED) {
		nexdome_command(device, "SRR");
	}
}


static void dome_shutter_status_request(indigo_device *device) {
	if (IS_CONNECTED) {
		nexdome_command(device, "SRS");
	}
}


static void handle_rotator_position(indigo_device *device, char *message) {
	int position;
	if (sscanf(message, ":PRR%d#", &position) != 1) {
		if (sscanf(message, "P%d\n", &position) != 1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
			return;
		}
	}
	PROPERTY_LOCK();
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = position / PRIVATE_DATA->steps_per_degree;
	PROPERTY_UNLOCK();
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %d", message, position);
}

static void handle_shutter_position(indigo_device *device, char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
}


static void handle_rotator_move(indigo_device *device, char *message) {
	PROPERTY_LOCK();
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	PROPERTY_UNLOCK();
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	if (PRIVATE_DATA->callibration_requested) {
		PROPERTY_LOCK();
		NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, "Going home...");
	}
	if (PRIVATE_DATA->park_requested) {
		PROPERTY_LOCK();
		DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		PROPERTY_UNLOCK();
		indigo_update_property(device, DOME_PARK_PROPERTY, "Going to park position...");
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
}


static void handle_shutter_move(indigo_device *device, char *message) {
	PROPERTY_LOCK();
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	PROPERTY_UNLOCK();
	if (message[1] == 'c') {
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is closing...");
	} else {
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is opening...");
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s %s", __FUNCTION__, message);
}


static void handle_rotator_status(indigo_device *device, char *message) {
	int position, at_home, max_position, home_position, dead_zone;
	if (sscanf(message, ":SER,%d,%d,%d,%d,%d#", &position, &at_home, &max_position, &home_position, &dead_zone) != 5) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %d %d %d %d %d", message, position, at_home, max_position, home_position, dead_zone);
	PROPERTY_LOCK();
	PRIVATE_DATA->steps_per_degree = max_position / 360.0;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = position / PRIVATE_DATA->steps_per_degree;
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	PROPERTY_UNLOCK();
	if (PRIVATE_DATA->callibration_requested && at_home) {
		PROPERTY_LOCK();
		NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(NEXDOME_FIND_HOME_PROPERTY, NEXDOME_FIND_HOME_ITEM, false);
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, "Dome is at home.");
		PRIVATE_DATA->callibration_requested = false;
	}

	if (PRIVATE_DATA->park_requested && IN_PARK_POSITION) {
		PROPERTY_LOCK();
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
		PROPERTY_UNLOCK();
		indigo_update_property(device, DOME_PARK_PROPERTY, "Dome is parked.");
		PRIVATE_DATA->park_requested = false;
	}

	if (PRIVATE_DATA->rotator_stop_requested) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->rotator_stop_requested = false;
		if (PRIVATE_DATA->callibration_requested) {
			NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
			PRIVATE_DATA->callibration_requested = false;
		}
		if (PRIVATE_DATA->park_requested) {
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
			PRIVATE_DATA->park_requested = false;
		}
	}

	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}


static void handle_shutter_status(indigo_device *device, char *message) {
	int position, max_position;
	int open_switch, close_switch;
	if (sscanf(message, ":SES,%d,%d,%d,%d#", &position, &max_position, &open_switch, &close_switch) != 4) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %d %d %d %d", message, position, max_position, open_switch, close_switch);
	if (close_switch || (position <= 0)) {
		PROPERTY_LOCK();
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
		PROPERTY_UNLOCK();
		if (close_switch) {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is closed.");
		} else {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is closed, but end swich is not activated.");
		}
	} else if (open_switch || (position >= max_position)) {
		PROPERTY_LOCK();
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		PROPERTY_UNLOCK();
		if (open_switch) {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is open.");
		} else {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is open, but end swich is not activated.");
		}
	} else if (PRIVATE_DATA->shutter_stop_requested) {
		PROPERTY_LOCK();
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		PROPERTY_UNLOCK();
		PRIVATE_DATA->shutter_stop_requested = false;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter stopped.");
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	}
}


static void handle_battery_status(indigo_device *device, char *message) {
	static bool low_voltage = false;
	int adc_value;
	if (sscanf(message, ":BV%d#", &adc_value) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	/* 15 / 1024 =  0.01465 V/ADU */
	double volts = 0.01465 * adc_value;
	if (volts < VOLT_THRESHOLD) {
		if (!low_voltage) {
			indigo_send_message(device, "Dome power is low! (U = %.2fV)", volts);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Dome power is low! (U = %.2fV", volts);
		}
		low_voltage = true;
	} else {
		if (low_voltage) {
			indigo_send_message(device, "Dome power is normal! (U = %.2fV)", volts);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Dome power is normal! (U = %.2fV)", volts);
		}
		low_voltage = false;
	}
	if (fabs((volts - NEXDOME_POWER_VOLTAGE_ITEM->number.value)*100) >= 1)  {
		NEXDOME_POWER_VOLTAGE_ITEM->number.value = volts;
		indigo_update_property(device, NEXDOME_POWER_PROPERTY, NULL);
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %d %.2f", message, adc_value, volts);
}


static void handle_home_poition(indigo_device *device, char *message) {
	int home_position;
	if (sscanf(message, ":HRR%d#", &home_position) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %d", message, home_position);
	PROPERTY_LOCK();
	if (indigo_azimuth_distance(home_position, NEXDOME_HOME_POSITION_ITEM->number.value) >= 1)  {
		NEXDOME_HOME_POSITION_ITEM->number.value = home_position;
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
	} else {
		PROPERTY_UNLOCK();
	}
}


static void handle_move_threshold(indigo_device *device, char *message) {
	int dead_zone;
	if (sscanf(message, ":DRR%d#", &dead_zone) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %d", message, dead_zone);
	PROPERTY_LOCK();
	if (fabs(dead_zone - NEXDOME_MOVE_THRESHOLD_ITEM->number.value) >= 1)  {
		NEXDOME_MOVE_THRESHOLD_ITEM->number.value = dead_zone;
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
	} else {
		PROPERTY_UNLOCK();
	}

	PROPERTY_LOCK();
	if (dead_zone > (DOME_SLAVING_THRESHOLD_ITEM->number.value * PRIVATE_DATA->steps_per_degree)) {
		DOME_SLAVING_THRESHOLD_ITEM->number.value = dead_zone / PRIVATE_DATA->steps_per_degree;
		DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
		PROPERTY_UNLOCK();
		indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, "Dome synchrinization threshold updated as it can not be less than the dome minimal move.");
	} else {
		PROPERTY_UNLOCK();
	}
}


static void handle_acceleration(indigo_device *device, char *message) {
	int acceleration;
	char target;
	if (sscanf(message, ":AR%c%d#", &target, &acceleration) != 2) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %c %d", message, target, acceleration);
	switch (target) {
		case 'R':
			PROPERTY_LOCK();
			NEXDOME_ACCELERATION_ROTATOR_ITEM->number.value = acceleration;
			PROPERTY_UNLOCK();
			indigo_update_property(device, NEXDOME_ACCELERATION_PROPERTY, NULL);
			break;
		case 'S':
			PROPERTY_LOCK();
			NEXDOME_ACCELERATION_SHUTTER_ITEM->number.value = acceleration;
			PROPERTY_UNLOCK();
			indigo_update_property(device, NEXDOME_ACCELERATION_PROPERTY, NULL);
			break;
		default:
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Message '%s' contains wrong target '%c'.", message, target);
	}
}


static void handle_velocity(indigo_device *device, char *message) {
	int velocity;
	char target;
	if (sscanf(message, ":VR%c%d#", &target, &velocity) != 2) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %c %d", message, target, velocity);
	switch (target) {
		case 'R':
			PROPERTY_LOCK();
			NEXDOME_VELOCITY_ROTATOR_ITEM->number.value = velocity;
			PROPERTY_UNLOCK();
			indigo_update_property(device, NEXDOME_VELOCITY_PROPERTY, NULL);
			break;
		case 'S':
			PROPERTY_LOCK();
			NEXDOME_VELOCITY_SHUTTER_ITEM->number.value = velocity;
			PROPERTY_UNLOCK();
			indigo_update_property(device, NEXDOME_VELOCITY_PROPERTY, NULL);
			break;
		default:
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Message '%s' contains wrong target '%c'.", message, target);
	}
}


static void handle_range(indigo_device *device, char *message) {
	int range;
	char target;
	if (sscanf(message, ":RR%c%d#", &target, &range) != 2) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %c %d", message, target, range);
	switch (target) {
		case 'R':
			PROPERTY_LOCK();
			NEXDOME_RANGE_ROTATOR_ITEM->number.value = range;
			PROPERTY_UNLOCK();
			indigo_update_property(device, NEXDOME_RANGE_PROPERTY, NULL);
			break;
		case 'S':
			PROPERTY_LOCK();
			NEXDOME_RANGE_SHUTTER_ITEM->number.value = range;
			PROPERTY_UNLOCK();
			indigo_update_property(device, NEXDOME_RANGE_PROPERTY, NULL);
			break;
		default:
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Message '%s' contains wrong target '%c'.", message, target);
	}
}


static void handle_xb(indigo_device *device, char *message) {
	char state[20];

	if (sscanf(message, "XB->%s", state) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> '%s'", message, state);
	if (strcmp(state, NEXDOME_XB_STATE_ITEM->text.value)) {
		strcpy(NEXDOME_XB_STATE_ITEM->text.value, state);
		if (!strcmp(state, "Online")) {
			NEXDOME_XB_STATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			NEXDOME_XB_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_update_property(device, NEXDOME_XB_STATE_PROPERTY, NULL);
	}
}


static void handle_rain(indigo_device *device, char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
	if (!strncmp(message, ":Rain#", 6)) {
		NEXDOME_RAIN_PROPERTY->state = INDIGO_ALERT_STATE;
		NEXDOME_RAIN_ALERT_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (!strncmp(message, ":RainStopped#", 13)) {
		NEXDOME_RAIN_PROPERTY->state = INDIGO_OK_STATE;
		NEXDOME_RAIN_ALERT_ITEM->light.value = INDIGO_OK_STATE;
	}
	indigo_update_property(device, NEXDOME_RAIN_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_event_handler(indigo_device *device) {
	char message[NEXDOME_CMD_LEN];
	while (IS_CONNECTED) {
		if (nexdome_get_message(device, message, sizeof(message))) {
			if (!strncmp(message, "P", 1) || !strncmp(message, ":PRR", 4)) {
				handle_rotator_position(device, message);
			} else if (!strncmp(message, "S", 1) || !strncmp(message, ":PRS", 4)) {
				handle_shutter_position(device, message);
			} else if (!strcmp(message, ":left#") || !strcmp(message, ":right#")) {
				handle_rotator_move(device, message);
			}else if (!strcmp(message, ":open#") || !strcmp(message, ":close#")) {
				handle_shutter_move(device, message);
			} else if (!strncmp(message, ":SER", 4)) {
				handle_rotator_status(device, message);
			} else if (!strncmp(message, ":SES", 4)) {
				handle_shutter_status(device, message);
			} else if (!strncmp(message, ":BV", 3)) {
				handle_battery_status(device, message);
			} else if (!strncmp(message, ":HRR", 4)) {
				handle_home_poition(device, message);
			} else if (!strncmp(message, ":DRR", 4)) {
				handle_move_threshold(device, message);
			} else if (!strncmp(message, ":AR", 3)) {
				handle_acceleration(device, message);
			} else if (!strncmp(message, ":VR", 3)) {
				handle_velocity(device, message);
			} else if (!strncmp(message, ":RR", 3)) {
				handle_range(device, message);
			} else if (!strncmp(message, "XB->", 4)) {
				handle_xb(device, message);
			} else if (!strncmp(message, ":Rain", 5)) {
				handle_rain(device, message);
			}
		}
	}
}


static indigo_result nexdome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(NEXDOME_FIND_HOME_PROPERTY, property))
			indigo_define_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_HOME_POSITION_PROPERTY, property))
			indigo_define_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_MOVE_THRESHOLD_PROPERTY, property))
			indigo_define_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_POWER_PROPERTY, property))
			indigo_define_property(device, NEXDOME_POWER_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_ACCELERATION_PROPERTY, property))
			indigo_define_property(device, NEXDOME_ACCELERATION_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_VELOCITY_PROPERTY, property))
			indigo_define_property(device, NEXDOME_VELOCITY_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_RANGE_PROPERTY, property))
			indigo_define_property(device, NEXDOME_RANGE_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_SETTINGS_PROPERTY, property))
			indigo_define_property(device, NEXDOME_SETTINGS_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_RAIN_PROPERTY, property))
			indigo_define_property(device, NEXDOME_RAIN_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_XB_STATE_PROPERTY, property))
			indigo_define_property(device, NEXDOME_XB_STATE_PROPERTY, NULL);

#ifdef CMD_AID
		if (indigo_property_match(NEXDOME_COMMAND_PROPERTY, property))
			indigo_define_property(device, NEXDOME_COMMAND_PROPERTY, NULL);
#endif
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_r_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->port_w_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->property_mutex, NULL);
		// -------------------------------------------------------------------------------- DOME_SPEED
		DOME_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DOME_STEPS_PROPERTY
		indigo_copy_value(DOME_STEPS_ITEM->label, "Relative move (°)");
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
		DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME
		NEXDOME_FIND_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_FIND_HOME_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Find home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (NEXDOME_FIND_HOME_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_FIND_HOME_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_FIND_HOME_ITEM, NEXDOME_FIND_HOME_ITEM_NAME, "Find home sensor", false);
		// -------------------------------------------------------------------------------- NEXDOME_MOVE_THRESHOLD
		NEXDOME_MOVE_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Move threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (NEXDOME_MOVE_THRESHOLD_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_MOVE_THRESHOLD_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_MOVE_THRESHOLD_ITEM, NEXDOME_MOVE_THRESHOLD_ITEM_NAME, "Minimal move (steps, ~153 steps/°)", 0, 10000, 1, 300);
		strcpy(NEXDOME_MOVE_THRESHOLD_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- NEXDOME_HOME_POSITION
		NEXDOME_HOME_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_HOME_POSITION_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Home position", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (NEXDOME_HOME_POSITION_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_HOME_POSITION_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_HOME_POSITION_ITEM, NEXDOME_HOME_POSITION_ITEM_NAME, "Position (steps, ~153 steps/°)", 0, 100000, 1, 0);
		strcpy(NEXDOME_HOME_POSITION_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- NEXDOME_POWER
		NEXDOME_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_POWER_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Power status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (NEXDOME_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_POWER_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_POWER_VOLTAGE_ITEM, NEXDOME_POWER_VOLTAGE_ITEM_NAME, "Battery charge (Volts)", 0, 500, 1, 0);
		strcpy(NEXDOME_POWER_VOLTAGE_ITEM->number.format, "%.2f");
		// -------------------------------------------------------------------------------- NEXDOME_ACCELERATION
		NEXDOME_ACCELERATION_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_ACCELERATION_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Acceleration time", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NEXDOME_ACCELERATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_ACCELERATION_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_ACCELERATION_ROTATOR_ITEM, NEXDOME_ACCELERATION_ROTATOR_ITEM_NAME, "Rotator (ms)", 100, 10000, 1, 1500);
		strcpy(NEXDOME_ACCELERATION_ROTATOR_ITEM->number.format, "%.0f");
		indigo_init_number_item(NEXDOME_ACCELERATION_SHUTTER_ITEM, NEXDOME_ACCELERATION_SHUTTER_ITEM_NAME, "Shutter (ms)", 100, 10000, 1, 1500);
		strcpy(NEXDOME_ACCELERATION_SHUTTER_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- NEXDOME_VELOCITY
		NEXDOME_VELOCITY_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_VELOCITY_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Movement velocity", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NEXDOME_VELOCITY_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_VELOCITY_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_VELOCITY_ROTATOR_ITEM, NEXDOME_VELOCITY_ROTATOR_ITEM_NAME, "Rotator (steps/s)", 32, 5000, 1, 600);
		strcpy(NEXDOME_VELOCITY_ROTATOR_ITEM->number.format, "%.0f");
		indigo_init_number_item(NEXDOME_VELOCITY_SHUTTER_ITEM, NEXDOME_VELOCITY_SHUTTER_ITEM_NAME, "Shutter (steps/s)", 32, 5000, 1, 800);
		strcpy(NEXDOME_VELOCITY_SHUTTER_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- NEXDOME_RANGE
		NEXDOME_RANGE_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_RANGE_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Movement range", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NEXDOME_RANGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_RANGE_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_RANGE_ROTATOR_ITEM, NEXDOME_RANGE_ROTATOR_ITEM_NAME, "Dome circumference (steps)", 30000, 100000, 1, 55080);
		strcpy(NEXDOME_RANGE_ROTATOR_ITEM->number.format, "%.0f");
		indigo_init_number_item(NEXDOME_RANGE_SHUTTER_ITEM, NEXDOME_RANGE_SHUTTER_ITEM_NAME, "Shutter travel (steps)", 20000, 90000, 1, 46000);
		strcpy(NEXDOME_RANGE_SHUTTER_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME
		NEXDOME_SETTINGS_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_SETTINGS_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Settings management", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (NEXDOME_SETTINGS_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_SETTINGS_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_SETTINGS_LOAD_ITEM, NEXDOME_SETTINGS_LOAD_ITEM_NAME, "Load from EEPROM", false);
		indigo_init_switch_item(NEXDOME_SETTINGS_SAVE_ITEM, NEXDOME_SETTINGS_SAVE_ITEM_NAME, "Save to EEPROM", false);
		indigo_init_switch_item(NEXDOME_SETTINGS_DEFAULT_ITEM, NEXDOME_SETTINGS_DEFAULT_ITEM_NAME, "Load factory defaults", false);
		// -------------------------------------------------------------------------------- NEXDOME_RAIN
		NEXDOME_RAIN_PROPERTY = indigo_init_light_property(NULL, device->name, NEXDOME_RAIN_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Rain sensor", INDIGO_OK_STATE, 1);
		if (NEXDOME_RAIN_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_RAIN_PROPERTY->hidden = false;
		indigo_init_light_item(NEXDOME_RAIN_ALERT_ITEM, NEXDOME_RAIN_ALERT_ITEM_NAME, "Rain alert", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------------- NEXDOME_XB_STATE
		NEXDOME_XB_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, NEXDOME_XB_STATE_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Shutter state", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
		if (NEXDOME_XB_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_XB_STATE_PROPERTY->hidden = false;
		indigo_init_text_item(NEXDOME_XB_STATE_ITEM, NEXDOME_XB_STATE_ITEM_NAME, "Shutter state", "");
#ifdef CMD_AID
		// -------------------------------------------------------------------------------- NEXDOME_COMMAND
		NEXDOME_COMMAND_PROPERTY = indigo_init_text_property(NULL, device->name, NEXDOME_COMMAND_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Custom command", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
		if (NEXDOME_COMMAND_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_COMMAND_PROPERTY->hidden = false;
		indigo_init_text_item(NEXDOME_COMMAND_ITEM, NEXDOME_COMMAND_ITEM_NAME, "Command", INDIGO_IDLE_STATE);
#endif
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void dome_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			//char name[NEXDOME_CMD_LEN] = "N/A";
			char firmware[NEXDOME_CMD_LEN] = "N/A";
			PROPERTY_LOCK();
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				PROPERTY_UNLOCK();
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				PROPERTY_UNLOCK();
				char *device_name = DEVICE_PORT_ITEM->text.value;
				if (!indigo_is_device_url(device_name, "nexdome")) {
					PRIVATE_DATA->handle = indigo_open_serial(device_name);
					/* To be on the safe side -> Wait for 1 seconds! */
					sleep(1);
				} else {
					indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
					PRIVATE_DATA->handle = indigo_open_network_device(device_name, 8080, &proto);
				}
				if ( PRIVATE_DATA->handle < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
					device->is_connected = false;
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					return;
				} else if (!nexdome_handshake(device, firmware)) {
					int res = close(PRIVATE_DATA->handle);
					if (res < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					}
					indigo_global_unlock(device);
					device->is_connected = false;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: NexDome did not respond. Are you using the correct firmware?");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, "NexDome did not respond. Are you using the correct firmware?");
					return;
				} else { // Successfully connected
					//uint32_t value;
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, "NexDome");
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					int version, revision;
					char leftover[255];
					sscanf(firmware, "%d.%d.%s", &version, &revision, leftover);
					PRIVATE_DATA->version = (version<<8) + revision;
					indigo_update_property(device, INFO_PROPERTY, NULL);
					INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with firmware V.%s (%04x) connected.", "NexDome", firmware, PRIVATE_DATA->version);

					indigo_define_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_POWER_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_ACCELERATION_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_VELOCITY_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_RANGE_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_SETTINGS_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_RAIN_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_XB_STATE_PROPERTY, NULL);
#ifdef CMD_AID
					indigo_define_property(device, NEXDOME_COMMAND_PROPERTY, NULL);
#endif

					PRIVATE_DATA->steps_per_degree = 153;

					indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
					DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_requested = true;

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected = %d", PRIVATE_DATA->handle);
					indigo_set_timer(device, 0, dome_event_handler, &PRIVATE_DATA->dome_event);

					/* Request Rotator and Shutter report to set the current values */
					nexdome_command(device, "SRR");
					nexdome_command(device, "SRS");

					request_settings(device);
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer(device, &PRIVATE_DATA->dome_event);
			indigo_delete_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_POWER_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_ACCELERATION_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_VELOCITY_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_RANGE_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_SETTINGS_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_RAIN_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_XB_STATE_PROPERTY, NULL);
#ifdef CMD_AID
			indigo_delete_property(device, NEXDOME_COMMAND_PROPERTY, NULL);
#endif
			pthread_mutex_lock(&PRIVATE_DATA->port_r_mutex);
			pthread_mutex_lock(&PRIVATE_DATA->port_w_mutex);
			int res = close(PRIVATE_DATA->handle);
			if (res < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
			}
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->port_w_mutex);
			pthread_mutex_unlock(&PRIVATE_DATA->port_r_mutex);
			device->is_connected = false;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected = %d", PRIVATE_DATA->handle);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
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
		indigo_set_timer(device, 0, dome_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_STEPS
		indigo_property_copy_values(DOME_STEPS_PROPERTY, property, false);
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked.");
			return INDIGO_OK;
		}
		PROPERTY_LOCK();
		double current_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value;
		double target_position = 0;

		DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			target_position = (int)(current_position - DOME_STEPS_ITEM->number.value + 360) % 360;
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			target_position = (int)(current_position + DOME_STEPS_ITEM->number.value + 360) % 360;
		}
		PRIVATE_DATA->park_requested = false;
		PRIVATE_DATA->callibration_requested = false;
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "GAR,%.0f", target_position);
		nexdome_command(device, command);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		PROPERTY_UNLOCK();
		indigo_set_timer(device, 3, dome_rotator_status_request, NULL);
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_HRIZONTAL_COORDINATES
		indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
		double target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked.");
			nexdome_command(device, "PRR");
			return INDIGO_OK;
		}
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		if (DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM->sw.value) {
			sprintf(command, "PWR,%.0f", target_position * PRIVATE_DATA->steps_per_degree);
			nexdome_command(device, command);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else { /* GOTO */
			PRIVATE_DATA->park_requested = false;
			PRIVATE_DATA->callibration_requested = false;
			if (PRIVATE_DATA->version < FIRMWARE_VERSION_3_2) {
				sprintf(command, "GAR,%.0f", target_position);
			} else {
				sprintf(command, "GSR,%.0f", target_position * PRIVATE_DATA->steps_per_degree);
			}
			nexdome_command(device, command);
		}
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		PROPERTY_UNLOCK();
		indigo_set_timer(device, 3, dome_rotator_status_request, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_EQUATORIAL_COORDINATES
		indigo_property_copy_values(DOME_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		/* Keep the dome in sync if needed */
		if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
			PROPERTY_LOCK();
			double az;
			char command[NEXDOME_CMD_LEN];
			if (indigo_fix_dome_azimuth(device, DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value, DOME_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value, &az) &&
			   (DOME_PARK_PROPERTY->state != INDIGO_BUSY_STATE) && (PRIVATE_DATA->callibration_requested == false) && (DOME_HORIZONTAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE)) {
				if (DOME_PARK_PARKED_ITEM->sw.value) {
					PROPERTY_UNLOCK();
					if (DOME_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_ALERT_STATE) {
						DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, "Can not Synchronize. Dome is parked.");
					} else {
						indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
					}
					return INDIGO_OK;
				}
				PRIVATE_DATA->park_requested = false;
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = az;
				if (PRIVATE_DATA->version < FIRMWARE_VERSION_3_2) {
					sprintf(command, "GAR,%.0f", az);
				} else {
					sprintf(command, "GSR,%.0f", az * PRIVATE_DATA->steps_per_degree);
				}
				nexdome_command(device, command);
			}
			nexdome_command(device, "PRR");
			PROPERTY_UNLOCK();
		}
		DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
		PROPERTY_LOCK();
		if (DOME_ABORT_MOTION_ITEM->sw.value) {
			nexdome_command(device, "SWR");
			if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE)
				PRIVATE_DATA->rotator_stop_requested = true;
			nexdome_command(device, "SWS");
			if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE)
				PRIVATE_DATA->shutter_stop_requested = true;
		}
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		PROPERTY_UNLOCK();
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		PROPERTY_LOCK();
		//bool success;
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			nexdome_command(device, "OPS");
		} else {
			nexdome_command(device, "CLS");
		}
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		PROPERTY_UNLOCK();
		indigo_set_timer(device, 3, dome_shutter_status_request, NULL);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_PARK
		indigo_property_copy_values(DOME_PARK_PROPERTY, property, false);
		PROPERTY_LOCK();
		if (DOME_PARK_UNPARKED_ITEM->sw.value) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			PROPERTY_UNLOCK();
			indigo_update_property(device, DOME_PARK_PROPERTY, "Dome is unparked.");
			return INDIGO_OK;
		} else if (DOME_PARK_PARKED_ITEM->sw.value) {
			if (IN_PARK_POSITION) {
				DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PROPERTY_UNLOCK();
				indigo_update_property(device, DOME_PARK_PROPERTY, "Dome is parked.");
				return INDIGO_OK;
			} else {
				indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
				char command[NEXDOME_CMD_LEN];
				sprintf(command, "GAR,%.0f", PRIVATE_DATA->park_azimuth);
				nexdome_command(device, command);
				PRIVATE_DATA->park_requested = true;
				nexdome_command(device, "PRR");
				DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		}
		PROPERTY_UNLOCK();
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_FIND_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME
		indigo_property_copy_values(NEXDOME_FIND_HOME_PROPERTY, property, false);
		PROPERTY_LOCK();
		if (NEXDOME_FIND_HOME_ITEM->sw.value) {
			nexdome_command(device, "GHR");
			PRIVATE_DATA->callibration_requested = true;
		}
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_HOME_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_HOME_POSITION
		indigo_property_copy_values(NEXDOME_HOME_POSITION_PROPERTY, property, false);
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "HWR,%.0f", NEXDOME_HOME_POSITION_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "HRR");
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_ACCELERATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_ACCELERATION
		indigo_property_copy_values(NEXDOME_ACCELERATION_PROPERTY, property, false);
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "AWR,%.0f", NEXDOME_ACCELERATION_ROTATOR_ITEM->number.value);
		nexdome_command(device, command);
		sprintf(command, "AWS,%.0f", NEXDOME_ACCELERATION_SHUTTER_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "ARR");
		nexdome_command(device, "ARS");
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_ACCELERATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_VELOCITY_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_VELOCITY
		indigo_property_copy_values(NEXDOME_VELOCITY_PROPERTY, property, false);
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "VWR,%.0f", NEXDOME_VELOCITY_ROTATOR_ITEM->number.value);
		nexdome_command(device, command);
		sprintf(command, "VWS,%.0f", NEXDOME_VELOCITY_SHUTTER_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "VRR");
		nexdome_command(device, "VRS");
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_VELOCITY_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_RANGE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_RANGE
		indigo_property_copy_values(NEXDOME_RANGE_PROPERTY, property, false);
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "RWR,%.0f", NEXDOME_RANGE_ROTATOR_ITEM->number.value);
		nexdome_command(device, command);
		sprintf(command, "RWS,%.0f", NEXDOME_RANGE_SHUTTER_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "RRR");
		nexdome_command(device, "RRS");
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_RANGE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_MOVE_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_MOVE_THRESHOLD
		indigo_property_copy_values(NEXDOME_MOVE_THRESHOLD_PROPERTY, property, false);
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "DWR,%.0f", NEXDOME_MOVE_THRESHOLD_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "DRR");
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SLAVING_PARAMETERS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
		indigo_property_copy_values(DOME_SLAVING_PARAMETERS_PROPERTY, property, false);
		PROPERTY_LOCK();
		if (NEXDOME_MOVE_THRESHOLD_ITEM->number.value > (DOME_SLAVING_THRESHOLD_ITEM->number.value * PRIVATE_DATA->steps_per_degree)) {
			DOME_SLAVING_THRESHOLD_ITEM->number.value = NEXDOME_MOVE_THRESHOLD_ITEM->number.value / PRIVATE_DATA->steps_per_degree;
			DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
			PROPERTY_UNLOCK();
			indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, "Requested synchronization threshold is less than the dome minimal move. Minimal move is used.");
		} else {
			DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
			PROPERTY_UNLOCK();
			indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_SETTINGS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_SETTINGS
		indigo_property_copy_values(NEXDOME_SETTINGS_PROPERTY, property, false);
		PROPERTY_LOCK();
		if (NEXDOME_SETTINGS_LOAD_ITEM->sw.value) {
			indigo_set_switch(NEXDOME_SETTINGS_PROPERTY, NEXDOME_SETTINGS_LOAD_ITEM, false);
			// Load settings
			nexdome_command(device, "ZRR");
			nexdome_command(device, "ZRS");
			// Request settings
			request_settings(device);
		} else if (NEXDOME_SETTINGS_SAVE_ITEM->sw.value) {
			indigo_set_switch(NEXDOME_SETTINGS_PROPERTY, NEXDOME_SETTINGS_SAVE_ITEM, false);
			// Save current settings
			nexdome_command(device, "ZWR");
			nexdome_command(device, "ZWS");
		} else if (NEXDOME_SETTINGS_DEFAULT_ITEM->sw.value) {
			indigo_set_switch(NEXDOME_SETTINGS_PROPERTY, NEXDOME_SETTINGS_DEFAULT_ITEM, false);
			// Load default settings
			nexdome_command(device, "ZDR");
			nexdome_command(device, "ZDS");
			// Request settings
			request_settings(device);
		}
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
#ifdef CMD_AID
	} else if (indigo_property_match(NEXDOME_COMMAND_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_COMMAND
		indigo_property_copy_values(NEXDOME_COMMAND_PROPERTY, property, false);
		PROPERTY_LOCK();
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "%s\n", NEXDOME_COMMAND_ITEM->text.value);
		nexdome_command(device, command);
		PROPERTY_UNLOCK();
		indigo_update_property(device, NEXDOME_COMMAND_PROPERTY, NULL);
		return INDIGO_OK;
#endif
		// --------------------------------------------------------------------------------
	}
	return indigo_dome_change_property(device, client, property);
}


static indigo_result dome_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connect_callback(device);
	}
	indigo_release_property(NEXDOME_FIND_HOME_PROPERTY);
	indigo_release_property(NEXDOME_HOME_POSITION_PROPERTY);
	indigo_release_property(NEXDOME_MOVE_THRESHOLD_PROPERTY);
	indigo_release_property(NEXDOME_POWER_PROPERTY);
	indigo_release_property(NEXDOME_ACCELERATION_PROPERTY);
	indigo_release_property(NEXDOME_VELOCITY_PROPERTY);
	indigo_release_property(NEXDOME_RANGE_PROPERTY);
	indigo_release_property(NEXDOME_SETTINGS_PROPERTY);
	indigo_release_property(NEXDOME_RAIN_PROPERTY);
	indigo_release_property(NEXDOME_XB_STATE_PROPERTY);
#ifdef CMD_AID
	indigo_release_property(NEXDOME_COMMAND_PROPERTY);
#endif
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static nexdome_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_nexdome3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_NEXDOME3_NAME,
		dome_attach,
		nexdome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DOME_NEXDOME3_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(nexdome_private_data));
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
