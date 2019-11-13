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

#define DRIVER_VERSION 0x00001
#define DRIVER_NAME    "indigo_dome_nexdome3"

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
#define NEXDOME_HOME_POSITION_ITEM_ITEM_NAME         "POSITION"

#define NEXDOME_MOVE_THRESHOLD_PROPERTY              (PRIVATE_DATA->move_threshold_property)
#define NEXDOME_MOVE_THRESHOLD_ITEM                  (NEXDOME_MOVE_THRESHOLD_PROPERTY->items+0)
#define NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME         "NEXDOME_MOVE_THRESHOLD"
#define NEXDOME_MOVE_THRESHOLD_ITEM_NAME             "THRESHOLD"

#define NEXDOME_POWER_PROPERTY                    (PRIVATE_DATA->power_property)
#define NEXDOME_POWER_VOLTAGE_ITEM                (NEXDOME_POWER_PROPERTY->items+0)
#define NEXDOME_POWER_PROPERTY_NAME               "NEXDOME_BATTERY_POWER"
#define NEXDOME_POWER_VOLTAGE_ITEM_NAME           "VOLTAGE"

// Low Voltage threshold taken from INDI
# define VOLT_THRESHOLD (7.5)

typedef struct {
	int handle;
	float steps_per_degree;
	bool park_requested;
	bool callibration_requested;
	bool shutter_stop_requested;
	bool rotator_stop_requested;
	float park_azimuth;
	pthread_mutex_t port_mutex;
	indigo_timer *dome_event;
	indigo_property *find_home_property;
	indigo_property *home_position_property;
	indigo_property *move_threshold_property;
	indigo_property *power_property;
} nexdome_private_data;

#define NEXDOME_CMD_LEN 100


static bool nexdome_command(indigo_device *device, const char *command) {
	if (!command) return false;
	char wrapped_command[NEXDOME_CMD_LEN];
	snprintf(wrapped_command, NEXDOME_CMD_LEN, "@%s\n", command);
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	indigo_write(PRIVATE_DATA->handle, wrapped_command, strlen(wrapped_command));
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}


static bool nexdome_get_message(indigo_device *device, char *response, int max) {
	char c;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (response != NULL) {
		int index = 0;
		while (index < max) {
			if (read(PRIVATE_DATA->handle, &c, 1) < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
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
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Response -> %s", response != NULL ? response : "NULL");
	return true;
}


static bool nexdome_handshake(indigo_device *device, char *firmware) {
	if(!firmware) return false;
	char response[255];
	nexdome_command(device, "FRR");
	/* I hope in 30 messages responce will be sent */
	for(int i = 0; i < 30; i++) {
		nexdome_get_message(device, response, sizeof(response));
		if (!strncmp(":FR", response, 3)) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "%s", response);
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

// =============================================================================

static void handle_rotator_position(indigo_device *device, char *message) {
	int position;
	if (sscanf(message, ":PRR%d#", &position) != 1) {
		if (sscanf(message, "P%d\n", &position) != 1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
			return;
		}
	}
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = position / PRIVATE_DATA->steps_per_degree;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s %d", __FUNCTION__, message, position);
}

static void handle_shutter_position(indigo_device *device, char *message) {
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s", __FUNCTION__, message);
}

static void handle_rotator_move(indigo_device *device, char *message) {
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	if (PRIVATE_DATA->callibration_requested) {
		NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, "Going home...");
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s", __FUNCTION__, message);
}

static void handle_shutter_move(indigo_device *device, char *message) {
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s", __FUNCTION__, message);
}

static void handle_rotator_status(indigo_device *device, char *message) {
	int position, at_home, max_position, home_position, dead_zone;
	if (sscanf(message, ":SER,%d,%d,%d,%d,%d#", &position, &at_home, &max_position, &home_position, &dead_zone) != 5) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s, %d %d %d %d %d", __FUNCTION__, message, position, at_home, max_position, home_position, dead_zone);
	PRIVATE_DATA->steps_per_degree = max_position / 360.0;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = position / PRIVATE_DATA->steps_per_degree;
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;

	if (PRIVATE_DATA->rotator_stop_requested) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		PRIVATE_DATA->rotator_stop_requested = false;
	}

	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);

	if (PRIVATE_DATA->callibration_requested && at_home) {
		NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(NEXDOME_FIND_HOME_PROPERTY, NEXDOME_FIND_HOME_ITEM, false);
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, "At home.");
		PRIVATE_DATA->callibration_requested = false;
	}
}

static void handle_shutter_status(indigo_device *device, char *message) {
	int position, max_position;
	int open_switch, close_switch;
	if (sscanf(message, ":SES,%d,%d,%d,%d#", &position, &max_position, &open_switch, &close_switch) != 4) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}

	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s %d %d %d %d", __FUNCTION__, message, position, max_position, open_switch, close_switch);

	if (close_switch) {
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
	} else if (open_switch) {
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
	} else if (PRIVATE_DATA->shutter_stop_requested){
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		PRIVATE_DATA->shutter_stop_requested = false;
	} else {
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
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
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s %d %.2f", __FUNCTION__, message, adc_value, volts);
}

static void handle_home_poition(indigo_device *device, char *message) {
	int home_position;
	if (sscanf(message, ":HRR%d#", &home_position) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}

	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s %d", __FUNCTION__, message, home_position);
	if (fabs(home_position - NEXDOME_HOME_POSITION_ITEM->number.value) >= 1)  {
		NEXDOME_HOME_POSITION_ITEM->number.value = home_position;
		indigo_update_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
	}
}


static void handle_move_threshold(indigo_device *device, char *message) {
	int dead_zone;
	if (sscanf(message, ":DRR%d#", &dead_zone) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}

	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s %d", __FUNCTION__, message, dead_zone);
	if (fabs(dead_zone - NEXDOME_MOVE_THRESHOLD_ITEM->number.value) >= 1)  {
		NEXDOME_MOVE_THRESHOLD_ITEM->number.value = dead_zone;
		indigo_update_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
	}
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
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DOME_SPEED
		DOME_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DOME_STEPS_PROPERTY
		strncpy(DOME_STEPS_ITEM->label, "Relative move (°)", INDIGO_VALUE_SIZE);
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 5;
		// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
		DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- DOME_SYNC_PARAMETERS
		DOME_SYNC_PARAMETERS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME
		NEXDOME_FIND_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_FIND_HOME_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Find home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (NEXDOME_FIND_HOME_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_FIND_HOME_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_FIND_HOME_ITEM, NEXDOME_FIND_HOME_ITEM_NAME, "Find home", false);
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
		indigo_init_number_item(NEXDOME_HOME_POSITION_ITEM, NEXDOME_HOME_POSITION_PROPERTY_NAME, "Position (steps, ~153 steps/°)", 0, 100000, 1, 0);
		strcpy(NEXDOME_HOME_POSITION_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- NEXDOME_POWER_PROPERTY
		NEXDOME_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_POWER_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Power status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (NEXDOME_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_POWER_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_POWER_VOLTAGE_ITEM, NEXDOME_POWER_VOLTAGE_ITEM_NAME, "Battery charge (Volts)", 0, 500, 1, 0);
		strcpy(NEXDOME_POWER_VOLTAGE_ITEM->number.format, "%.2f");
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (!device->is_connected) {
			char name[NEXDOME_CMD_LEN] = "N/A";
			char firmware[NEXDOME_CMD_LEN] = "N/A";
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				char *device_name = DEVICE_PORT_ITEM->text.value;
				if (strncmp(device_name, "nexdome://", 10)) {
					PRIVATE_DATA->handle = indigo_open_serial(device_name);
					/* To be on the safe side -> Wait for 1 seconds! */
					sleep(1);
				} else {
					char *host = device_name + 8;
					char *colon = strchr(host, ':');
					if (colon == NULL) {
						PRIVATE_DATA->handle = indigo_open_tcp(host, 8080);
					} else {
						char host_name[INDIGO_NAME_SIZE];
						strncpy(host_name, host, colon - host);
						host_name[colon - host] = 0;
						int port = atoi(colon + 1);
						PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
					}
				}
				if ( PRIVATE_DATA->handle < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, " indigo_open_serial(%s): failed", DEVICE_PORT_ITEM->text.value);
					device->is_connected = false;
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					return INDIGO_OK;
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
					return INDIGO_OK;;
				} else { // Successfully connected
					uint32_t value;
					strncpy(INFO_DEVICE_MODEL_ITEM->text.value, "NexDome", INDIGO_VALUE_SIZE);
					strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with firmware V.%s connected.", "NexDome", firmware);

					indigo_define_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_POWER_PROPERTY, NULL);

					/* request Rotator and Shutter report to set the current values */
					nexdome_command(device, "SRR");
					nexdome_command(device, "SRS");
					nexdome_command(device, "ARR");
					nexdome_command(device, "ARS");
					nexdome_command(device, "DRR");
					nexdome_command(device, "HRR");
					nexdome_command(device, "PRR");
					nexdome_command(device, "PRS");
					nexdome_command(device, "VRR");
					nexdome_command(device, "VRS");
					PRIVATE_DATA->steps_per_degree = 153;
					/*
					if(!nexdome_get_park_azimuth(device, &PRIVATE_DATA->park_azimuth)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_park_azimuth(%d): returned error", PRIVATE_DATA->handle);
					}
					if (fabs((PRIVATE_DATA->park_azimuth - PRIVATE_DATA->current_position)*100) <= 1) {
						indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
					} else {
						indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
					}
					*/
					DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_requested = false;
					indigo_update_property(device, DOME_PARK_PROPERTY, NULL);

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected = %d", PRIVATE_DATA->handle);
					PRIVATE_DATA->dome_event = indigo_set_timer(device, 0, dome_event_handler);
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->dome_event);
				indigo_delete_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
				indigo_delete_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
				indigo_delete_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
				indigo_delete_property(device, NEXDOME_POWER_PROPERTY, NULL);
				pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
				int res = close(PRIVATE_DATA->handle);
				if (res < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
				}
				indigo_global_unlock(device);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				device->is_connected = false;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected = %d", PRIVATE_DATA->handle);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(DOME_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_STEPS
		indigo_property_copy_values(DOME_STEPS_PROPERTY, property, false);
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}
		double current_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value;
		double target_position;

		DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			target_position = (int)(current_position - DOME_STEPS_ITEM->number.value + 360) % 360;
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			target_position = (int)(current_position + DOME_STEPS_ITEM->number.value + 360) % 360;
		}
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "GAR,%.0f", target_position);
		nexdome_command(device, command);
		nexdome_command(device, "PRR");
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_HRIZONTAL_COORDINATES
		indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
		double target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked");
			nexdome_command(device, "PRR");
			return INDIGO_OK;
		}
		char command[NEXDOME_CMD_LEN];
		if (DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM->sw.value) {
			sprintf(command, "PWR,%.0f", target_position * PRIVATE_DATA->steps_per_degree);
			nexdome_command(device, command);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else { /* GOTO */
			sprintf(command, "GAR,%.0f", target_position);
			nexdome_command(device, command);
		}
		nexdome_command(device, "PRR");

		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_EQUATORIAL_COORDINATES
		indigo_property_copy_values(DOME_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}
		DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
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
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		bool success;
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			nexdome_command(device, "OPS");
		} else {
			nexdome_command(device, "CLS");
		}
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_PARK
		indigo_property_copy_values(DOME_PARK_PROPERTY, property, false);
		if (DOME_PARK_UNPARKED_ITEM->sw.value) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
		} else if (DOME_PARK_PARKED_ITEM->sw.value) {
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			// handle park
			PRIVATE_DATA->park_requested = true;

			DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		}
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_FIND_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME
		indigo_property_copy_values(NEXDOME_FIND_HOME_PROPERTY, property, false);
		if (NEXDOME_FIND_HOME_ITEM->sw.value) {
			nexdome_command(device, "GHR");
			PRIVATE_DATA->callibration_requested = true;
		}
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_HOME_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_HOME_POSITION
		indigo_property_copy_values(NEXDOME_HOME_POSITION_PROPERTY, property, false);
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "HWR,%.0f", NEXDOME_HOME_POSITION_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "HRR");
		indigo_update_property(device, NEXDOME_HOME_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_MOVE_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_MOVE_THRESHOLD
		indigo_property_copy_values(NEXDOME_MOVE_THRESHOLD_PROPERTY, property, false);
		char command[NEXDOME_CMD_LEN];
		sprintf(command, "DWR,%.0f", NEXDOME_MOVE_THRESHOLD_ITEM->number.value);
		nexdome_command(device, command);
		nexdome_command(device, "DRR");
		indigo_update_property(device, NEXDOME_MOVE_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
}
	return indigo_dome_change_property(device, client, property);
}


static indigo_result dome_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(NEXDOME_FIND_HOME_PROPERTY);
	indigo_release_property(NEXDOME_HOME_POSITION_PROPERTY);
	indigo_release_property(NEXDOME_MOVE_THRESHOLD_PROPERTY);
	indigo_release_property(NEXDOME_POWER_PROPERTY);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static nexdome_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_nexdome3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_NEXDOME_NAME,
		dome_attach,
		nexdome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DOME_NEXDOME_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(nexdome_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(nexdome_private_data));
			dome = malloc(sizeof(indigo_device));
			assert(dome != NULL);
			memcpy(dome, &dome_template, sizeof(indigo_device));
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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
