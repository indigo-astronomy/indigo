// Copyright (c) 2020 Rumen G. Bgdanovski
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

/** INDIGO Baader Classic Dome driver
 \file indigo_dome_baader.c
 */

#define DRIVER_VERSION 0x00005
#define DRIVER_NAME	"indigo_dome_baader"

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

#include "indigo_dome_baader.h"

// gp_bits is used as boolean
#define is_connected                    gp_bits

#define PRIVATE_DATA                              ((baader_private_data *)device->private_data)

#define X_SETTINGS_GROUP                    "Settings"

#define X_REVERSED_PROPERTY                 (PRIVATE_DATA->reversed_property)
#define X_REVERSED_YES_ITEM                 (X_REVERSED_PROPERTY->items+0)
#define X_REVERSED_NO_ITEM                  (X_REVERSED_PROPERTY->items+1)
#define X_REVERSED_PROPERTY_NAME            "X_REVERSED"
#define X_REVERSED_YES_ITEM_NAME            "YES"
#define X_REVERSED_NO_ITEM_NAME             "NO"


#define X_RESET_SHUTTER_COMM_PROPERTY       (PRIVATE_DATA->reset_shutter_comm_property)
#define X_RESET_SHUTTER_COMM_ITEM           (X_RESET_SHUTTER_COMM_PROPERTY->items+0)
#define X_RESET_SHUTTER_COMM_PROPERTY_NAME  "X_RESET_SHUTTER_COMM"
#define X_RESET_SHUTTER_COMM_ITEM_NAME      "RESET"


#define X_FIND_HOME_PROPERTY                (PRIVATE_DATA->find_home_property)
#define X_FIND_HOME_ITEM                    (X_FIND_HOME_PROPERTY->items+0)
#define X_FIND_HOME_PROPERTY_NAME           "X_FIND_HOME"
#define X_FIND_HOME_ITEM_NAME               "FIND_HOME"


#define X_CALLIBRATE_PROPERTY               (PRIVATE_DATA->callibrate_property)
#define X_CALLIBRATE_ITEM                   (X_CALLIBRATE_PROPERTY->items+0)
#define X_CALLIBRATE_PROPERTY_NAME          "X_CALLIBRATE"
#define X_CALLIBRATE_ITEM_NAME              "CALLIBRATE"


#define X_POWER_PROPERTY                    (PRIVATE_DATA->power_property)
#define X_POWER_ROTATOR_ITEM                (X_POWER_PROPERTY->items+0)
#define X_POWER_SHUTTER_ITEM                (X_POWER_PROPERTY->items+1)
#define X_POWER_PROPERTY_NAME               "X_POWER"
#define X_POWER_ROTATOR_ITEM_NAME           "ROTATOR_VOLTAGE"
#define X_POWER_SHUTTER_ITEM_NAME           "SHUTTER_VOLTAGE"


typedef enum {
	DOME_STOPED = 0,
	DOME_GOTO = 1,
	DOME_FINDIGING_HOME = 2,
	DOME_CALIBRATING = 3
} baader_dome_state_t;


typedef enum {
	DOME_HAS_NOT_BEEN_HOME = -1,
	DOME_NOT_AT_HOME =0,
	DOME_AT_HOME = 1,
} baader_home_state_t;

// Low Voltage threshold taken from INDI
# define VOLT_THRESHOLD (7.5)

typedef struct {
	int handle;
	float target_position, current_position;
	baader_dome_state_t dome_state;
	int shutter_position;
	bool park_requested;
	bool aborted;
	float park_azimuth;
	pthread_mutex_t port_mutex;
	indigo_timer *dome_timer;
	indigo_property *reversed_property;
	indigo_property *reset_shutter_comm_property;
	indigo_property *find_home_property;
	indigo_property *callibrate_property;
	indigo_property *power_property;
} baader_private_data;

#define BAADER_CMD_LEN 10

static bool baader_command(indigo_device *device, const char *command, char *response, int sleep) {
	char c;
	struct timeval tv;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	// flush
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0) {
			break;
		}
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (sleep > 0)
		usleep(sleep);

	// read response
	if (response != NULL) {
		int index = 0;
		int timeout = 3;
		while (index < BAADER_CMD_LEN-1) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = timeout;
			tv.tv_usec = 100000;
			timeout = 0;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				return false;
			}

			response[index++] = c;
		}
		response[index] = '\0';
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}


static bool baader_get_info(indigo_device *device, char *name, char *serial_num) {
	if(!name || !serial_num) return false;

	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#ser_num", response, 100)) {
		int parsed = sscanf(response, "d#%s", serial_num);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#ser_num -> %s, '%s'", response, serial_num);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_abort(indigo_device *device) {
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#stopdom", response, 100)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#stopdom -> %s", response);
		if (strcmp(response, "d#gotmess")) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_get_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;
	int azim;
	char ch;
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#getazim", response, 100)) {
		int parsed = sscanf(response, "d#az%c%d", &ch, &azim);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#getazim -> %s, %d, ch = %c", response, azim, ch);
		if (parsed != 2) return false;
		*azimuth = azim / 10.0;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_get_shutter_position(indigo_device *device, int *pos) {
	if(!pos) return false;
	bool success = false;
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#getshut", response, 100)) {
		if (!strcmp(response, "d#shutope")) {
			success = true;
			*pos = 100;
		} else if (!strcmp(response, "d#shutclo")) {
			success = true;
			*pos = 0;
		} else if (!strcmp(response, "d#shutrun")){
			success = true;
			*pos = 50;
		} else {
			int parsed = sscanf(response, "d#shut_%d", pos);
			if (parsed == 1) success = true;
		}
		if(success) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#getshut -> %s, %d", response, *pos);
			return true;
		}
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_open_shutter(indigo_device *device) {
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#opeshut", response, 100)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#openshut -> %s", response);
		if (strcmp(response, "d#gotmess")) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_close_shutter(indigo_device *device) {
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#closhut", response, 100)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#closhut -> %s", response);
		if (strcmp(response, "d#gotmess")) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


//static bool baader_set_shutter_position(indigo_device *device, float position) {
//	char command[BAADER_CMD_LEN];
//	char response[BAADER_CMD_LEN] = {0};
//
//	snprintf(command, BAADER_CMD_LEN, "f %.2f\n", position);
//
//	if (baader_command(device, command, response, 100)) {
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "f %.2f -> %s", position, response);
//		if (response[0] != 'F') return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


static bool baader_sync_azimuth(indigo_device *device, float azimuth) {
	return false;
}


static bool baader_get_park_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;
	return false;
}


static bool baader_goto_azimuth(indigo_device *device, float azimuth) {
	char command[BAADER_CMD_LEN];
	char response[BAADER_CMD_LEN] = {0};

	snprintf(command, BAADER_CMD_LEN, "d#azi%04d", (int)(azimuth *10));

	if (baader_command(device, command, response, 100)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s", command, response);
		if (strcmp(response, "d#gotmess")) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_find_home(indigo_device *device) {
	char response[BAADER_CMD_LEN] = {0};
	return false;
}


static bool baader_callibrate(indigo_device *device) {
	return false;
}


static bool baader_get_reversed_flag(indigo_device *device, bool *reversed) {
	if(!reversed) return false;
	return false;
}


static bool baader_set_reversed_flag(indigo_device *device, bool reversed) {
	return false;
}

static bool baader_get_voltages(indigo_device *device, float *v_rotattor, float *v_shutter) {
	if (!v_rotattor || !v_shutter) return false;
	return false;
}


static bool baader_restart_shutter_communication(indigo_device *device) {
	return false;
}


// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_timer_callback(indigo_device *device) {
	static bool need_update = true;
	static bool low_voltage = false;
	int prev_shutter_position = -1;

	/* Handle dome rotation */
	if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error");
	}

	if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE ||
	    fabs((PRIVATE_DATA->target_position - PRIVATE_DATA->current_position)*10) >= 1) need_update = true;

	if (need_update) {
		if (PRIVATE_DATA->aborted) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->aborted = false;
		} else if (fabs((PRIVATE_DATA->target_position - PRIVATE_DATA->current_position)*10) >= 1) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		} else {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		}

		if ((fabs((PRIVATE_DATA->park_azimuth - PRIVATE_DATA->current_position)*10) < 1) && PRIVATE_DATA->park_requested) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}
		need_update = false;
	}

	/* Handle dome shutter */
	if(!baader_get_shutter_position(device, &PRIVATE_DATA->shutter_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_shutter_position(): returned error");
	}
	if (PRIVATE_DATA->shutter_position != prev_shutter_position) {
		if (PRIVATE_DATA->shutter_position == 100) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		} else if (PRIVATE_DATA->shutter_position == 0) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		prev_shutter_position = PRIVATE_DATA->shutter_position;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	}

	/* Keep the dome in sync if needed */
	if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
		double az;
		if (indigo_fix_dome_azimuth(device, DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value, DOME_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value, &az) &&
		   (DOME_HORIZONTAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE)) {
			PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = az;
			if(!baader_goto_azimuth(device, PRIVATE_DATA->target_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				return;
			}
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		}
	}

	indigo_reschedule_timer(device, 1, &(PRIVATE_DATA->dome_timer));
}


static indigo_result baader_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_REVERSED_PROPERTY, property))
			indigo_define_property(device, X_REVERSED_PROPERTY, NULL);
		if (indigo_property_match(X_RESET_SHUTTER_COMM_PROPERTY, property))
			indigo_define_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
		if (indigo_property_match(X_FIND_HOME_PROPERTY, property))
			indigo_define_property(device, X_FIND_HOME_PROPERTY, NULL);
		if (indigo_property_match(X_CALLIBRATE_PROPERTY, property))
			indigo_define_property(device, X_CALLIBRATE_PROPERTY, NULL);
		if (indigo_property_match(X_POWER_PROPERTY, property))
			indigo_define_property(device, X_POWER_PROPERTY, NULL);
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
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
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_REVERSED
		X_REVERSED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_REVERSED_PROPERTY_NAME, X_SETTINGS_GROUP, "Reversed dome directions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_REVERSED_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_REVERSED_PROPERTY->hidden = false;
		indigo_init_switch_item(X_REVERSED_YES_ITEM, X_REVERSED_YES_ITEM_NAME, "Yes", false);
		indigo_init_switch_item(X_REVERSED_NO_ITEM, X_REVERSED_NO_ITEM_NAME, "No", false);
		// -------------------------------------------------------------------------------- X_RESET_SHUTTER_COMM_PROPERTY
		X_RESET_SHUTTER_COMM_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RESET_SHUTTER_COMM_PROPERTY_NAME, X_SETTINGS_GROUP, "Reset shutter communication", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_RESET_SHUTTER_COMM_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_RESET_SHUTTER_COMM_PROPERTY->hidden = false;
		indigo_init_switch_item(X_RESET_SHUTTER_COMM_ITEM, X_RESET_SHUTTER_COMM_ITEM_NAME, "Reset", false);
		// -------------------------------------------------------------------------------- X_FIND_HOME_PROPERTY
		X_FIND_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FIND_HOME_PROPERTY_NAME, X_SETTINGS_GROUP, "Find home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_FIND_HOME_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_FIND_HOME_PROPERTY->hidden = false;
		indigo_init_switch_item(X_FIND_HOME_ITEM, X_FIND_HOME_ITEM_NAME, "Find home", false);
		// -------------------------------------------------------------------------------- X_FIND_HOME_PROPERTY
		X_CALLIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALLIBRATE_PROPERTY_NAME, X_SETTINGS_GROUP, "Callibrate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CALLIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_CALLIBRATE_PROPERTY->hidden = false;
		indigo_init_switch_item(X_CALLIBRATE_ITEM, X_CALLIBRATE_ITEM_NAME, "Callibrate", false);
		// -------------------------------------------------------------------------------- X_POWER_PROPERTY
		X_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, X_POWER_PROPERTY_NAME, X_SETTINGS_GROUP, "Power status", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (X_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_POWER_PROPERTY->hidden = false;
		indigo_init_number_item(X_POWER_ROTATOR_ITEM, X_POWER_ROTATOR_ITEM_NAME, "Rotator (Volts)", 0, 500, 1, 0);
		strcpy(X_POWER_ROTATOR_ITEM->number.format, "%.2f");
		indigo_init_number_item(X_POWER_SHUTTER_ITEM, X_POWER_SHUTTER_ITEM_NAME, "Shutter (Volts)", 0, 500, 1, 0);
		strcpy(X_POWER_SHUTTER_ITEM->number.format, "%.2f");
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void dome_connect_callback(indigo_device *device) {
	if (!device->is_connected) {
		char name[BAADER_CMD_LEN] = "N/A";
		char firmware[BAADER_CMD_LEN] = "N/A";
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
			if (!indigo_is_device_url(device_name, "baader")) {
				PRIVATE_DATA->handle = indigo_open_serial(device_name);
			} else {
				indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
				PRIVATE_DATA->handle = indigo_open_network_device(device_name, 8080, &proto);
			}
			if (PRIVATE_DATA->handle < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				indigo_global_unlock(device);
				return;
			} else if (!baader_get_info(device, name, firmware)) {
				int res = close(PRIVATE_DATA->handle);
				if (res < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
				}
				indigo_global_unlock(device);
				device->is_connected = false;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: Baader dome did not respond. Are you using the correct firmware?");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, "Baader dome did not respond. Are you using the correct firmware?");
				return;
			} else { // Successfully connected
				//uint32_t value;
				strncpy(INFO_DEVICE_MODEL_ITEM->text.value, name, INDIGO_VALUE_SIZE);
				strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with firmware V.%s connected.", name, firmware);
				/*
				bool reversed;
				if(!baader_get_reversed_flag(device, &reversed)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_reversed_flag(): returned error");
				}
				if (reversed) {
					indigo_set_switch(X_REVERSED_PROPERTY, X_REVERSED_YES_ITEM, true);
				} else {
					indigo_set_switch(X_REVERSED_PROPERTY, X_REVERSED_NO_ITEM, true);
				}
				indigo_define_property(device, X_REVERSED_PROPERTY, NULL);

				indigo_define_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
				indigo_define_property(device, X_FIND_HOME_PROPERTY, NULL);
				indigo_define_property(device, X_CALLIBRATE_PROPERTY, NULL);
				indigo_define_property(device, X_POWER_PROPERTY, NULL);
				*/

				if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error");
				}
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
				PRIVATE_DATA->aborted = false;
				PRIVATE_DATA->park_azimuth = 180;
				if (fabs((PRIVATE_DATA->park_azimuth - PRIVATE_DATA->current_position)*100) <= 1) {
					indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
				} else {
					indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
				}
				DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_requested = false;
				indigo_update_property(device, DOME_PARK_PROPERTY, NULL);

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				device->is_connected = true;

				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected = %d", PRIVATE_DATA->handle);
				indigo_set_timer(device, 0.5, dome_timer_callback, &PRIVATE_DATA->dome_timer);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_timer);
			/*
			indigo_delete_property(device, X_REVERSED_PROPERTY, NULL);
			indigo_delete_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
			indigo_delete_property(device, X_FIND_HOME_PROPERTY, NULL);
			indigo_delete_property(device, X_CALLIBRATE_PROPERTY, NULL);
			indigo_delete_property(device, X_POWER_PROPERTY, NULL);
			*/
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
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
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
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}

		if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(%d): returned error", PRIVATE_DATA->handle);
		}

		DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position - DOME_STEPS_ITEM->number.value + 360) % 360;
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position + DOME_STEPS_ITEM->number.value + 360) % 360;
		}

		if(!baader_goto_azimuth(device, PRIVATE_DATA->target_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed.");
			return INDIGO_OK;
		}

		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(%d): returned error", PRIVATE_DATA->handle);
			}
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}
		PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
		if(!baader_goto_azimuth(device, PRIVATE_DATA->target_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			return INDIGO_OK;
		}

		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
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

		if(!baader_abort(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_abort(%d): returned error", PRIVATE_DATA->handle);
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			return INDIGO_OK;
		}

		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}

		PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		PRIVATE_DATA->aborted = true;
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		bool success;
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			success = baader_open_shutter(device);
		} else {
			success = baader_close_shutter(device);
		}
		if (!success) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
			return INDIGO_OK;
		}
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
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

			if(!baader_goto_azimuth(device, PRIVATE_DATA->park_azimuth)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
			}
			PRIVATE_DATA->target_position = PRIVATE_DATA->park_azimuth;
			PRIVATE_DATA->park_requested = true;

			DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		}
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_REVERSED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_REVERED
		indigo_property_copy_values(X_REVERSED_PROPERTY, property, false);
		X_REVERSED_PROPERTY->state = INDIGO_OK_STATE;
		if(!baader_set_reversed_flag(device, X_REVERSED_YES_ITEM->sw.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_set_reversed_flag(%d, %d): returned error", PRIVATE_DATA->handle, X_REVERSED_YES_ITEM->sw.value);
			X_REVERSED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_REVERSED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RESET_SHUTTER_COMM_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RESET_SHUTTER_COMM
		indigo_property_copy_values(X_RESET_SHUTTER_COMM_PROPERTY, property, false);
		if (X_RESET_SHUTTER_COMM_ITEM->sw.value) {
			X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!baader_restart_shutter_communication(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_restart_shutter_communication(%d): returned error", PRIVATE_DATA->handle);
				X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				indigo_update_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
				/* wait for XBEE to reinitialize */
				sleep(2);
				X_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(X_RESET_SHUTTER_COMM_PROPERTY, X_RESET_SHUTTER_COMM_ITEM, false);
			}
		}
		indigo_update_property(device, X_RESET_SHUTTER_COMM_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_FIND_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FIND_HOME
		indigo_property_copy_values(X_FIND_HOME_PROPERTY, property, false);
		if (X_FIND_HOME_ITEM->sw.value) {
			X_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!baader_find_home(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_find_home(%d): returned error", PRIVATE_DATA->handle);
				indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
				X_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				//PRIVATE_DATA->callibration_requested = true;
			}
		}
		indigo_update_property(device, X_FIND_HOME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_CALLIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALLIBRATE
		indigo_property_copy_values(X_CALLIBRATE_PROPERTY, property, false);
		if (X_CALLIBRATE_ITEM->sw.value) {
			X_CALLIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!baader_callibrate(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_callibrate(%d): returned error.", PRIVATE_DATA->handle);
				indigo_set_switch(X_CALLIBRATE_PROPERTY, X_CALLIBRATE_ITEM, false);
				X_CALLIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_CALLIBRATE_PROPERTY, "Callibration failed. Is the dome in home position?");
				return INDIGO_OK;
			} else {
				//PRIVATE_DATA->callibration_requested = true;
			}
		}
		indigo_update_property(device, X_CALLIBRATE_PROPERTY, NULL);
		return INDIGO_OK;
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
	indigo_release_property(X_REVERSED_PROPERTY);
	indigo_release_property(X_RESET_SHUTTER_COMM_PROPERTY);
	indigo_release_property(X_FIND_HOME_PROPERTY);
	indigo_release_property(X_CALLIBRATE_PROPERTY);
	indigo_release_property(X_POWER_PROPERTY);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static baader_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_baader(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_BAADER_NAME,
		dome_attach,
		baader_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DOME_BAADER_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(baader_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(baader_private_data));
			dome = malloc(sizeof(indigo_device));
			assert(dome != NULL);
			memcpy(dome, &dome_template, sizeof(indigo_device));
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
