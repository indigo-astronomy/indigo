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
#define is_connected                        gp_bits

#define PRIVATE_DATA                        ((baader_private_data *)device->private_data)

#define X_SETTINGS_GROUP                    "Settings"

#define X_CALLIBRATE_PROPERTY               (PRIVATE_DATA->callibrate_property)
#define X_CALLIBRATE_ITEM                   (X_CALLIBRATE_PROPERTY->items+0)
#define X_CALLIBRATE_PROPERTY_NAME          "X_CALLIBRATE"
#define X_CALLIBRATE_ITEM_NAME              "CALLIBRATE"

typedef enum {
	FLAP_CLOSED = 0,
	FLAP_OPEN = 1,
	FLAP_STOPPED = 2,
	FLAP_CLOSING = 3,
	FLAP_OPENING = 4,
	FLAP_MOVING = 5 // Old firmware uses it instead of FLAP_STOPPED, FLAP_CLOSING and FLAP_OPENING
} baader_flap_state_t;

typedef struct {
	int handle;
	float target_position, current_position;
	int shutter_position;
	baader_flap_state_t flap_state;
	bool park_requested;
	bool aborted;
	float park_azimuth;
	pthread_mutex_t port_mutex;
	indigo_timer *dome_timer;
	indigo_property *callibrate_property;
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


static bool baader_get_serial_number(indigo_device *device, char *serial_num) {
	if(!serial_num) return false;

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


static bool baader_get_flap_state(indigo_device *device, int *flap_state) {
	if(!flap_state) return false;
	bool success = false;
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#getflap", response, 100)) {
		if (!strcmp(response, "d#flapope")) {
			success = true;
			*flap_state = FLAP_OPEN;
		} else if (!strcmp(response, "d#flapclo")) {
			success = true;
			*flap_state = FLAP_CLOSED;
		} else if (!strcmp(response, "d#flaprim")){
			success = true;
			*flap_state = FLAP_STOPPED;
		} else if (!strcmp(response, "d#flaprop")){
			success = true;
			*flap_state = FLAP_OPENING;
		} else if (!strcmp(response, "d#flaprcl")) {
			success = true;
			*flap_state = FLAP_CLOSING;
		} else if (!strcmp(response, "d#flaprun")) {
			success = true;
			*flap_state = FLAP_MOVING;
		}
		if(success) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#getshut -> %s, %d", response, *flap_state);
			return true;
		}
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_open_flap(indigo_device *device) {
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#opeflap", response, 100)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#opeflap -> %s", response);
		if (strcmp(response, "d#gotmess")) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool baader_close_flap(indigo_device *device) {
	char response[BAADER_CMD_LEN]={0};
	if (baader_command(device, "d#cloflap", response, 100)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d#cloflap -> %s", response);
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

static bool baader_callibrate(indigo_device *device) {
	return false;
}


// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_timer_callback(indigo_device *device) {
	static int prev_shutter_position = -1;
	static baader_flap_state_t prev_flap_state = -1;

	/* Handle dome rotation */
	if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error");
	}

	if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ||
	    DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE ||
	    fabs((PRIVATE_DATA->target_position - PRIVATE_DATA->current_position)*10) >= 1
	) {
		if (fabs((PRIVATE_DATA->target_position - PRIVATE_DATA->current_position)*10) >= 1) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			//DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
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
			indigo_update_property(device, DOME_PARK_PROPERTY, "Dome parked");
		}
	}

	/* Handle dome shutter */
	if(!baader_get_shutter_position(device, &PRIVATE_DATA->shutter_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_shutter_position(): returned error");
	}
	if (PRIVATE_DATA->shutter_position != prev_shutter_position) {
		if (PRIVATE_DATA->shutter_position == 100) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter open");
		} else if (PRIVATE_DATA->shutter_position == 0) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter closed");
		} else {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			//DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		}
		prev_shutter_position = PRIVATE_DATA->shutter_position;

	}

	/* Handle dome flap */
	if(!baader_get_flap_state(device, (int *)&PRIVATE_DATA->flap_state)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_flap_state(): returned error");
	}
	if (PRIVATE_DATA->flap_state != prev_flap_state) {
		if (PRIVATE_DATA->flap_state == FLAP_OPEN) {
			indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_OPENED_ITEM, true);
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_FLAP_PROPERTY, "Flap open");
		} else if (PRIVATE_DATA->flap_state == FLAP_CLOSED) {
			indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_CLOSED_ITEM, true);
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_FLAP_PROPERTY, "Flap closed");
		} else if (PRIVATE_DATA->flap_state == FLAP_STOPPED) {
			indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_OPENED_ITEM, false);
			indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_CLOSED_ITEM, false);
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		} else {
			indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_OPENED_ITEM, false);
			indigo_set_switch(DOME_FLAP_PROPERTY, DOME_FLAP_CLOSED_ITEM, false);
			//DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		}
		prev_flap_state = PRIVATE_DATA->flap_state;
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

	if (PRIVATE_DATA->aborted) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);

		DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);

		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		prev_shutter_position = PRIVATE_DATA->shutter_position;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);

		DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
		prev_flap_state = PRIVATE_DATA->flap_state;
		indigo_update_property(device, DOME_FLAP_PROPERTY, NULL);
		PRIVATE_DATA->aborted = false;
	}

	indigo_reschedule_timer(device, 1, &(PRIVATE_DATA->dome_timer));
}


static indigo_result baader_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_CALLIBRATE_PROPERTY, property))
			indigo_define_property(device, X_CALLIBRATE_PROPERTY, NULL);
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
		INFO_PROPERTY->count = 7;
		// -------------------------------------------------------------------------------- DOME_FLAP
		DOME_FLAP_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
		DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_CALLIBRATE_PROPERTY
		X_CALLIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALLIBRATE_PROPERTY_NAME, X_SETTINGS_GROUP, "Callibrate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CALLIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_CALLIBRATE_PROPERTY->hidden = false;
		indigo_init_switch_item(X_CALLIBRATE_ITEM, X_CALLIBRATE_ITEM_NAME, "Callibrate", false);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void dome_connect_callback(indigo_device *device) {
	if (!device->is_connected) {
		char serial_number[INDIGO_VALUE_SIZE] = "N/A";
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock");
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
			} else if (!baader_get_serial_number(device, serial_number)) {
				int res = close(PRIVATE_DATA->handle);
				if (res < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
				}
				indigo_global_unlock(device);
				device->is_connected = false;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Connect failed: Baader dome did not respond");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, "Baader dome did not respond");
				return;
			} else { // Successfully connected
				strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, serial_number, INDIGO_VALUE_SIZE);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with serial No.%s connected", INFO_DEVICE_MODEL_ITEM->text.value, serial_number);

				indigo_define_property(device, X_CALLIBRATE_PROPERTY, NULL);

				if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(): returned error");
				}
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
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

				PRIVATE_DATA->flap_state = FLAP_CLOSED;

				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected = %d", PRIVATE_DATA->handle);
				indigo_set_timer(device, 0.5, dome_timer_callback, &PRIVATE_DATA->dome_timer);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_timer);
			indigo_delete_property(device, X_CALLIBRATE_PROPERTY, NULL);
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
		if (DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK_STATE;
		}
		indigo_property_copy_values(DOME_STEPS_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}

		if(!baader_get_azimuth(device, &PRIVATE_DATA->current_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_get_azimuth(%d): returned error", PRIVATE_DATA->handle);
		}

		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = ((int)(10 * (PRIVATE_DATA->current_position - DOME_STEPS_ITEM->number.value) + 3600) % 3600) / 10.0;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "PRIVATE_DATA->target_position = %f\n", PRIVATE_DATA->target_position);
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = ((int)(10 * (PRIVATE_DATA->current_position + DOME_STEPS_ITEM->number.value) + 3600) % 3600) / 10.0;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "PRIVATE_DATA->target_position = %f\n", PRIVATE_DATA->target_position);
		}

		if(!baader_goto_azimuth(device, PRIVATE_DATA->target_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed");
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
		if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK_STATE;
		}
		indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

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
		if (!IS_CONNECTED) return INDIGO_OK;

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
		if (!IS_CONNECTED) return INDIGO_OK;

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
		if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter is moving: request can not be completed");
			return INDIGO_OK_STATE;
		 }
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		char message[INDIGO_VALUE_SIZE] = {0};
		bool success;
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			success = baader_open_shutter(device);
			strncpy(message,"Opening shutter...", INDIGO_VALUE_SIZE);
		} else {
			success = baader_close_shutter(device);
			strncpy(message,"Closing shutter...", INDIGO_VALUE_SIZE);
		}
		if (!success) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
			return INDIGO_OK;
		}
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, message);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_FLAP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_FLAP
		if (DOME_FLAP_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_FLAP_PROPERTY, "Flap is moving: request can not be completed");
			return INDIGO_OK_STATE;
		}
		bool is_opened = DOME_FLAP_OPENED_ITEM->sw.value;
		bool is_closed = DOME_FLAP_CLOSED_ITEM->sw.value;
		indigo_property_copy_values(DOME_FLAP_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		char message[INDIGO_VALUE_SIZE] = {0};
		bool success;
		if (DOME_FLAP_OPENED_ITEM->sw.value) {
			success = baader_open_flap(device);
			strncpy(message,"Opening flap...", INDIGO_VALUE_SIZE);
		} else {
			success = baader_close_flap(device);
			strncpy(message,"Closing flap...", INDIGO_VALUE_SIZE);
		}
		if (!success) {
			DOME_FLAP_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_FLAP_CLOSED_ITEM->sw.value = is_closed;
			DOME_FLAP_OPENED_ITEM->sw.value = is_opened;
			indigo_update_property(device, DOME_FLAP_PROPERTY, "Flap operation failed. Is the shutter open enough?");
			return INDIGO_OK;
		}
		DOME_FLAP_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_FLAP_PROPERTY, message);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_PARK
		indigo_property_copy_values(DOME_PARK_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (DOME_PARK_UNPARKED_ITEM->sw.value) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_update_property(device, DOME_PARK_PROPERTY, "Dome unparked");
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
			indigo_update_property(device, DOME_PARK_PROPERTY, "Dome parking...");
		} else {
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_CALLIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALLIBRATE
		indigo_property_copy_values(X_CALLIBRATE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (X_CALLIBRATE_ITEM->sw.value) {
			X_CALLIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!baader_callibrate(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "baader_callibrate(%d): returned error", PRIVATE_DATA->handle);
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
	indigo_release_property(X_CALLIBRATE_PROPERTY);
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
