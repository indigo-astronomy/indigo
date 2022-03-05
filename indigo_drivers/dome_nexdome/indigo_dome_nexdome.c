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

/** INDIGO DOME NexDome driver
 \file indigo_dome_nexdome.c
 */

#define DRIVER_VERSION 0x00009
#define DRIVER_NAME	"indigo_dome_nexdome"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_io.h>

#include "indigo_dome_nexdome.h"

// gp_bits is used as boolean
#define is_connected                    gp_bits

#define PRIVATE_DATA                              ((nexdome_private_data *)device->private_data)

#define NEXDOME_SETTINGS_GROUP                    "Settings"

#define NEXDOME_REVERSED_PROPERTY                 (PRIVATE_DATA->reversed_property)
#define NEXDOME_REVERSED_YES_ITEM                 (NEXDOME_REVERSED_PROPERTY->items+0)
#define NEXDOME_REVERSED_NO_ITEM                  (NEXDOME_REVERSED_PROPERTY->items+1)
#define NEXDOME_REVERSED_PROPERTY_NAME            "NEXDOME_REVERSED"
#define NEXDOME_REVERSED_YES_ITEM_NAME            "YES"
#define NEXDOME_REVERSED_NO_ITEM_NAME             "NO"


#define NEXDOME_RESET_SHUTTER_COMM_PROPERTY       (PRIVATE_DATA->reset_shutter_comm_property)
#define NEXDOME_RESET_SHUTTER_COMM_ITEM           (NEXDOME_RESET_SHUTTER_COMM_PROPERTY->items+0)
#define NEXDOME_RESET_SHUTTER_COMM_PROPERTY_NAME  "NEXDOME_RESET_SHUTTER_COMM"
#define NEXDOME_RESET_SHUTTER_COMM_ITEM_NAME      "RESET"


#define NEXDOME_FIND_HOME_PROPERTY                (PRIVATE_DATA->find_home_property)
#define NEXDOME_FIND_HOME_ITEM                    (NEXDOME_FIND_HOME_PROPERTY->items+0)
#define NEXDOME_FIND_HOME_PROPERTY_NAME           "NEXDOME_FIND_HOME"
#define NEXDOME_FIND_HOME_ITEM_NAME               "FIND_HOME"


#define NEXDOME_CALLIBRATE_PROPERTY               (PRIVATE_DATA->callibrate_property)
#define NEXDOME_CALLIBRATE_ITEM                   (NEXDOME_CALLIBRATE_PROPERTY->items+0)
#define NEXDOME_CALLIBRATE_PROPERTY_NAME          "NEXDOME_CALLIBRATE"
#define NEXDOME_CALLIBRATE_ITEM_NAME              "CALLIBRATE"


#define NEXDOME_POWER_PROPERTY                    (PRIVATE_DATA->power_property)
#define NEXDOME_POWER_ROTATOR_ITEM                (NEXDOME_POWER_PROPERTY->items+0)
#define NEXDOME_POWER_SHUTTER_ITEM                (NEXDOME_POWER_PROPERTY->items+1)
#define NEXDOME_POWER_PROPERTY_NAME               "NEXDOME_POWER"
#define NEXDOME_POWER_ROTATOR_ITEM_NAME           "ROTATOR_VOLTAGE"
#define NEXDOME_POWER_SHUTTER_ITEM_NAME           "SHUTTER_VOLTAGE"


typedef enum {
	DOME_STOPED = 0,
	DOME_GOTO = 1,
	DOME_FINDIGING_HOME = 2,
	DOME_CALIBRATING = 3
} nexdome_dome_state_t;

typedef enum {
	SHUTTER_STATE_NOT_CONNECTED = 0,
	SHUTTER_STATE_OPEN = 1,
	SHUTTER_STATE_OPENING = 2,
	SHUTTER_STATE_CLOSED = 3,
	SHUTTER_STATE_CLOSING = 4,
	SHUTTER_STATE_UNKNOWN = 5
} nexdome_shutter_state_t;

typedef enum {
	DOME_HAS_NOT_BEEN_HOME = -1,
	DOME_NOT_AT_HOME =0,
	DOME_AT_HOME = 1,
} nexdome_home_state_t;

// Low Voltage threshold taken from INDI
# define VOLT_THRESHOLD (7.5)

typedef struct {
	int handle;
	float target_position, current_position;
	nexdome_dome_state_t dome_state;
	nexdome_shutter_state_t shutter_state;
	bool park_requested;
	bool callibration_requested;
	bool abort_requested;
	float park_azimuth;
	pthread_mutex_t port_mutex;
	indigo_timer *dome_timer;
	indigo_property *reversed_property;
	indigo_property *reset_shutter_comm_property;
	indigo_property *find_home_property;
	indigo_property *callibrate_property;
	indigo_property *power_property;
} nexdome_private_data;


#define NEXDOME_CMD_LEN 100
#define NEXDOME_SLEEP 100

static bool nexdome_command(indigo_device *device, const char *command, char *response, int max, int sleep) {
	char c;
	struct timeval tv;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	// flush
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
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
		while (index < max) {
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

			if ((c == '\n') || (c == '\r'))
				break;

			response[index++] = c;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static bool nexdome_get_info(indigo_device *device, char *name, char *firmware) {
	if(!name || !firmware) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "v\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int parsed = sscanf(response, "V%s V %s", name, firmware);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "v -> %s, '%s' '%s'", response, name, firmware);
		if (parsed != 2) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_abort(indigo_device *device) {
	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "a\n", response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "a -> %s", response);
		if (response[0] != 'A') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_dome_state(indigo_device *device, nexdome_dome_state_t *state) {
	if(!state) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "m\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int _state;
		int parsed = sscanf(response, "M %d", &_state);
		*state = _state;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "m -> %s, %d", response, *state);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_get_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "q\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int parsed = sscanf(response, "Q %f", azimuth);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "q -> %s, %f", response, *azimuth);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_shutter_state(indigo_device *device, nexdome_shutter_state_t *state, bool *not_raining) {
	if(!state || !not_raining) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "u\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int _state, _not_raining;
		int parsed = sscanf(response, "U %d %d", &_state, &_not_raining);
		*state = _state;
		*not_raining = _not_raining;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "u -> %s, %d %d", response, *state, *not_raining);
		if (parsed != 2) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


//static bool nexdome_get_shutter_position(indigo_device *device, float *pos) {
//	if(!pos) return false;
//
//	char response[NEXDOME_CMD_LEN]={0};
//	if (nexdome_command(device, "b\n", response, sizeof(response), NEXDOME_SLEEP)) {
//		int parsed = sscanf(response, "B %f", pos);
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "b -> %s, %f", response, *pos);
//		if (parsed != 1) return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


static bool nexdome_open_shutter(indigo_device *device) {
	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "d\n", response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "d -> %s", response);
		if (response[0] != 'D') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_close_shutter(indigo_device *device) {
	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "e\n", response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "e -> %s", response);
		if (response[0] != 'D') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


//static bool nexdome_set_shutter_position(indigo_device *device, float position) {
//	char command[NEXDOME_CMD_LEN];
//	char response[NEXDOME_CMD_LEN] = {0};
//
//	snprintf(command, NEXDOME_CMD_LEN, "f %.2f\n", position);
//
//	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_SLEEP)) {
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "f %.2f -> %s", position, response);
//		if (response[0] != 'F') return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


static bool nexdome_sync_azimuth(indigo_device *device, float azimuth) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "s %.2f\n", azimuth);

	// we ignore the returned sync value
	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "s %.2f -> %s", azimuth, response);
		if (response[0] != 'S') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


//static bool nexdome_get_home_azimuth(indigo_device *device, float *azimuth) {
//	if(!azimuth) return false;
//
//	char response[NEXDOME_CMD_LEN]={0};
//	if (nexdome_command(device, "i\n", response, sizeof(response), NEXDOME_SLEEP)) {
//		int parsed = sscanf(response, "I %f", azimuth);
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "i -> %s, %f", response, *azimuth);
//		if (parsed != 1) return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


//static bool nexdome_set_home_azimuth(indigo_device *device, float azimuth) {
//	char command[NEXDOME_CMD_LEN];
//	char response[NEXDOME_CMD_LEN] = {0};
//
//	snprintf(command, NEXDOME_CMD_LEN, "j %.2f\n", azimuth);
//
//	// we ignore the returned value
//	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_SLEEP)) {
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "j %.2f -> %s", azimuth, response);
//		if (response[0] != 'I') return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


static bool nexdome_get_park_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "n\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int parsed = sscanf(response, "N %f", azimuth);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "n -> %s, %f", response, *azimuth);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


//static bool nexdome_set_park_azimuth(indigo_device *device, float azimuth) {
//	char command[NEXDOME_CMD_LEN];
//	char response[NEXDOME_CMD_LEN] = {0};
//
//	snprintf(command, NEXDOME_CMD_LEN, "l %.2f\n", azimuth);
//
//	// we ignore the returned value
//	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_SLEEP)) {
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "l %.2f -> %s", azimuth, response);
//		if (response[0] != 'N') return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


static bool nexdome_goto_azimuth(indigo_device *device, float azimuth) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "g %.2f\n", azimuth);

	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "g %.2f -> %s", azimuth, response);
		if (response[0] != 'G') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_find_home(indigo_device *device) {
	char response[NEXDOME_CMD_LEN] = {0};

	if (nexdome_command(device, "h\n", response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "h -> %s", response);
		if (response[0] != 'H') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


//static bool nexdome_get_home_state(indigo_device *device, int *state) {
//	if(!state) return false;
//
//	char response[NEXDOME_CMD_LEN]={0};
//	if (nexdome_command(device, "z\n", response, sizeof(response), NEXDOME_SLEEP)) {
//		int _state;
//		int parsed = sscanf(response, "Z %d", &_state);
//		*state = _state;
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "z -> %s, %d", response, *state);
//		if (parsed != 1) return false;
//		return true;
//	}
//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
//	return false;
//}


static bool nexdome_callibrate(indigo_device *device) {
	char response[NEXDOME_CMD_LEN] = {0};

	if (nexdome_command(device, "c\n", response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "c -> %s", response);
		if (response[0] != 'C') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_get_reversed_flag(indigo_device *device, bool *reversed) {
	if(!reversed) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "y\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int _reversed;
		int parsed = sscanf(response, "Y %d", &_reversed);
		*reversed = _reversed;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "y -> %s, %d", response, *reversed);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_set_reversed_flag(indigo_device *device, bool reversed) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "y %d\n", (int)reversed);

	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "y %d -> %s", reversed, response);
		if (response[0] != 'Y') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}

static bool nexdome_get_voltages(indigo_device *device, float *v_rotattor, float *v_shutter) {
	if (!v_rotattor || !v_shutter) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "k\n", response, sizeof(response), NEXDOME_SLEEP)) {
		int parsed = sscanf(response, "K %f %f", v_rotattor, v_shutter);
		*v_rotattor /= 100;
		*v_shutter /= 100;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "k -> %s, %f %f", response, *v_rotattor, *v_shutter);
		if (parsed != 2) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


static bool nexdome_restart_shutter_communication(indigo_device *device) {
	char response[NEXDOME_CMD_LEN] = {0};

	if (nexdome_command(device, "w\n", response, sizeof(response), NEXDOME_SLEEP)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "W -> %s", response);
		if (response[0] != 'W') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response");
	return false;
}


// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_timer_callback(indigo_device *device) {
	static bool need_update = true;
	static bool low_voltage = false;
	static nexdome_shutter_state_t prev_shutter_state = SHUTTER_STATE_UNKNOWN;

	/* Check dome power */
	float v_rotator, v_shutter;
	if(!nexdome_get_voltages(device, &v_rotator, &v_shutter)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_voltages(): returned error");
	} else {
		/* Threshold taken from INDI driver */
		if ((v_rotator < VOLT_THRESHOLD) || (v_shutter < VOLT_THRESHOLD)) {
			if (!low_voltage) {
				indigo_send_message(device, "Dome power is low! (U_rotator = %.2fV, U_shutter = %.2fV)", v_rotator, v_shutter);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Dome power is low! (U_rotator = %.2fV, U_shutter = %.2fV)", v_rotator, v_shutter);
			}
			low_voltage = true;
		} else {
			if (low_voltage) {
				indigo_send_message(device, "Dome power is normal! (U_rotator = %.2fV, U_shutter = %.2fV)", v_rotator, v_shutter);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Dome power is normal! (U_rotator = %.2fV, U_shutter = %.2fV)", v_rotator, v_shutter);
			}
			low_voltage = false;
		}
		if ((fabs((v_rotator - NEXDOME_POWER_ROTATOR_ITEM->number.value)*100) >= 1) ||
		   (fabs((v_shutter - NEXDOME_POWER_SHUTTER_ITEM->number.value)*100) >= 1)) {
			NEXDOME_POWER_ROTATOR_ITEM->number.value = v_rotator;
			NEXDOME_POWER_SHUTTER_ITEM->number.value = v_shutter;
			indigo_update_property(device, NEXDOME_POWER_PROPERTY, NULL);
		}
	}

	/* Handle dome rotation */
	if(!nexdome_dome_state(device, &PRIVATE_DATA->dome_state)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_dome_state(): returned error");
	}

	if(!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
	}

	if ((DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) || PRIVATE_DATA->callibration_requested) need_update = true;

	if (PRIVATE_DATA->dome_state != DOME_STOPED) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		need_update = true;
	} else if(need_update) {
		if (!PRIVATE_DATA->callibration_requested && !PRIVATE_DATA->abort_requested && (indigo_azimuth_distance(PRIVATE_DATA->target_position, PRIVATE_DATA->current_position)*10) >= 1) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		} else {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		}

		if(NEXDOME_FIND_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
			NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(NEXDOME_FIND_HOME_PROPERTY, NEXDOME_FIND_HOME_ITEM, false);
			indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, "Home Found.");
		}
		if(NEXDOME_CALLIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
			NEXDOME_CALLIBRATE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(NEXDOME_CALLIBRATE_PROPERTY, NEXDOME_CALLIBRATE_ITEM, false);
			indigo_update_property(device, NEXDOME_CALLIBRATE_PROPERTY, "Callibration complete.");
		}
		PRIVATE_DATA->callibration_requested = false;
		need_update = false;
	}

	if (PRIVATE_DATA->park_requested && (indigo_azimuth_distance(PRIVATE_DATA->park_azimuth, PRIVATE_DATA->current_position)*10) <= 1) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_requested = false;
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	}
	PRIVATE_DATA->abort_requested = false;

	/* Handle dome shutter */
	bool raining;
	if(!nexdome_shutter_state(device, &PRIVATE_DATA->shutter_state, &raining)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_shutter_state(): returned error");
	}
	if (PRIVATE_DATA->shutter_state != prev_shutter_state || DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		switch(PRIVATE_DATA->shutter_state) {
		case SHUTTER_STATE_NOT_CONNECTED:
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		case SHUTTER_STATE_OPEN:
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			break;
		case SHUTTER_STATE_CLOSED:
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			break;
		case SHUTTER_STATE_OPENING:
		case SHUTTER_STATE_CLOSING:
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			break;
		case SHUTTER_STATE_UNKNOWN:
			DOME_SHUTTER_PROPERTY->state = INDIGO_IDLE_STATE;
			break;
		}
		prev_shutter_state = PRIVATE_DATA->shutter_state;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	}

	/* Keep the dome in sync if needed */
	if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
		double az;
		if (indigo_fix_dome_azimuth(device, DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value, DOME_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value, &az) &&
		   (DOME_HORIZONTAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE)) {
			PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = az;
			if(!nexdome_goto_azimuth(device, PRIVATE_DATA->target_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
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


static indigo_result nexdome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(NEXDOME_REVERSED_PROPERTY, property))
			indigo_define_property(device, NEXDOME_REVERSED_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_RESET_SHUTTER_COMM_PROPERTY, property))
			indigo_define_property(device, NEXDOME_RESET_SHUTTER_COMM_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_FIND_HOME_PROPERTY, property))
			indigo_define_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_CALLIBRATE_PROPERTY, property))
			indigo_define_property(device, NEXDOME_CALLIBRATE_PROPERTY, NULL);
		if (indigo_property_match(NEXDOME_POWER_PROPERTY, property))
			indigo_define_property(device, NEXDOME_POWER_PROPERTY, NULL);
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
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
		// -------------------------------------------------------------------------------- NEXDOME_REVERSED
		NEXDOME_REVERSED_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_REVERSED_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Reversed dome directions", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (NEXDOME_REVERSED_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_REVERSED_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_REVERSED_YES_ITEM, NEXDOME_REVERSED_YES_ITEM_NAME, "Yes", false);
		indigo_init_switch_item(NEXDOME_REVERSED_NO_ITEM, NEXDOME_REVERSED_NO_ITEM_NAME, "No", false);
		// -------------------------------------------------------------------------------- NEXDOME_RESET_SHUTTER_COMM_PROPERTY
		NEXDOME_RESET_SHUTTER_COMM_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_RESET_SHUTTER_COMM_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Reset shutter communication", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (NEXDOME_RESET_SHUTTER_COMM_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_RESET_SHUTTER_COMM_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_RESET_SHUTTER_COMM_ITEM, NEXDOME_RESET_SHUTTER_COMM_ITEM_NAME, "Reset", false);
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME_PROPERTY
		NEXDOME_FIND_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_FIND_HOME_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Find home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (NEXDOME_FIND_HOME_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_FIND_HOME_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_FIND_HOME_ITEM, NEXDOME_FIND_HOME_ITEM_NAME, "Find home", false);
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME_PROPERTY
		NEXDOME_CALLIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, NEXDOME_CALLIBRATE_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Callibrate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (NEXDOME_CALLIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_CALLIBRATE_PROPERTY->hidden = false;
		indigo_init_switch_item(NEXDOME_CALLIBRATE_ITEM, NEXDOME_CALLIBRATE_ITEM_NAME, "Callibrate", false);
		// -------------------------------------------------------------------------------- NEXDOME_POWER_PROPERTY
		NEXDOME_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, NEXDOME_POWER_PROPERTY_NAME, NEXDOME_SETTINGS_GROUP, "Power status", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (NEXDOME_POWER_PROPERTY == NULL)
			return INDIGO_FAILED;
		NEXDOME_POWER_PROPERTY->hidden = false;
		indigo_init_number_item(NEXDOME_POWER_ROTATOR_ITEM, NEXDOME_POWER_ROTATOR_ITEM_NAME, "Rotator (Volts)", 0, 500, 1, 0);
		strcpy(NEXDOME_POWER_ROTATOR_ITEM->number.format, "%.2f");
		indigo_init_number_item(NEXDOME_POWER_SHUTTER_ITEM, NEXDOME_POWER_SHUTTER_ITEM_NAME, "Shutter (Volts)", 0, 500, 1, 0);
		strcpy(NEXDOME_POWER_SHUTTER_ITEM->number.format, "%.2f");
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
			char name[NEXDOME_CMD_LEN] = "N/A";
			char firmware[NEXDOME_CMD_LEN] = "N/A";
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
				if (!indigo_is_device_url(device_name, "nexdome")) {
					PRIVATE_DATA->handle = indigo_open_serial(device_name);
					/* To be on the safe side -> Wait for 1 seconds! */
					sleep(1);
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
				} else if (!nexdome_get_info(device, name, firmware)) {
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
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, name);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with firmware V.%s connected.", name, firmware);

					bool reversed;
					if(!nexdome_get_reversed_flag(device, &reversed)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_reversed_flag(): returned error");
					}
					if (reversed) {
						indigo_set_switch(NEXDOME_REVERSED_PROPERTY, NEXDOME_REVERSED_YES_ITEM, true);
					} else {
						indigo_set_switch(NEXDOME_REVERSED_PROPERTY, NEXDOME_REVERSED_NO_ITEM, true);
					}
					indigo_define_property(device, NEXDOME_REVERSED_PROPERTY, NULL);

					indigo_define_property(device, NEXDOME_RESET_SHUTTER_COMM_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_CALLIBRATE_PROPERTY, NULL);
					indigo_define_property(device, NEXDOME_POWER_PROPERTY, NULL);

					if(!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
					}
					PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
					if(!nexdome_get_park_azimuth(device, &PRIVATE_DATA->park_azimuth)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_park_azimuth(%d): returned error", PRIVATE_DATA->handle);
					}
					if ((indigo_azimuth_distance(PRIVATE_DATA->park_azimuth, PRIVATE_DATA->current_position)*100) <= 1) {
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
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_timer);
			indigo_delete_property(device, NEXDOME_REVERSED_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_RESET_SHUTTER_COMM_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
			indigo_delete_property(device, NEXDOME_CALLIBRATE_PROPERTY, NULL);
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
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}

		if(!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(%d): returned error", PRIVATE_DATA->handle);
		}

		DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position - DOME_STEPS_ITEM->number.value + 360) % 360;
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position + DOME_STEPS_ITEM->number.value + 360) % 360;
		}

		if(!nexdome_goto_azimuth(device, PRIVATE_DATA->target_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
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
		PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			if(!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(%d): returned error", PRIVATE_DATA->handle);
			}
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}

		if (DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM->sw.value) {
			if(!nexdome_sync_azimuth(device, PRIVATE_DATA->target_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_sync_azimuth(%d): returned error", PRIVATE_DATA->handle);
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				return INDIGO_OK;
			}
		} else { /* GOTO */
			if(!nexdome_goto_azimuth(device, PRIVATE_DATA->target_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				return INDIGO_OK;
			}
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

		if(!nexdome_abort(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_abort(%d): returned error", PRIVATE_DATA->handle);
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			return INDIGO_OK;
		}

		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}

		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		PRIVATE_DATA->abort_requested = true;
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		bool success;
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			success = nexdome_open_shutter(device);
		} else {
			success = nexdome_close_shutter(device);
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
			if(!nexdome_get_park_azimuth(device, &PRIVATE_DATA->park_azimuth)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_park_azimuth(%d): returned error", PRIVATE_DATA->handle);
			}
			if(!nexdome_goto_azimuth(device, PRIVATE_DATA->park_azimuth)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_goto_azimuth(%d): returned error", PRIVATE_DATA->handle);
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
	} else if (indigo_property_match(NEXDOME_REVERSED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_REVERED
		indigo_property_copy_values(NEXDOME_REVERSED_PROPERTY, property, false);
		NEXDOME_REVERSED_PROPERTY->state = INDIGO_OK_STATE;
		if(!nexdome_set_reversed_flag(device, NEXDOME_REVERSED_YES_ITEM->sw.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_set_reversed_flag(%d, %d): returned error", PRIVATE_DATA->handle, NEXDOME_REVERSED_YES_ITEM->sw.value);
			NEXDOME_REVERSED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, NEXDOME_REVERSED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_RESET_SHUTTER_COMM_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_RESET_SHUTTER_COMM
		indigo_property_copy_values(NEXDOME_RESET_SHUTTER_COMM_PROPERTY, property, false);
		if (NEXDOME_RESET_SHUTTER_COMM_ITEM->sw.value) {
			NEXDOME_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!nexdome_restart_shutter_communication(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_restart_shutter_communication(%d): returned error", PRIVATE_DATA->handle);
				NEXDOME_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				indigo_update_property(device, NEXDOME_RESET_SHUTTER_COMM_PROPERTY, NULL);
				/* wait for XBEE to reinitialize */
				sleep(2);
				NEXDOME_RESET_SHUTTER_COMM_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(NEXDOME_RESET_SHUTTER_COMM_PROPERTY, NEXDOME_RESET_SHUTTER_COMM_ITEM, false);
			}
		}
		indigo_update_property(device, NEXDOME_RESET_SHUTTER_COMM_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_FIND_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_FIND_HOME
		indigo_property_copy_values(NEXDOME_FIND_HOME_PROPERTY, property, false);
		if (NEXDOME_FIND_HOME_ITEM->sw.value) {
			NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!nexdome_find_home(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_find_home(%d): returned error", PRIVATE_DATA->handle);
				indigo_set_switch(NEXDOME_FIND_HOME_PROPERTY, NEXDOME_FIND_HOME_ITEM, false);
				NEXDOME_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				PRIVATE_DATA->callibration_requested = true;
			}
		}
		indigo_update_property(device, NEXDOME_FIND_HOME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(NEXDOME_CALLIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NEXDOME_CALLIBRATE
		indigo_property_copy_values(NEXDOME_CALLIBRATE_PROPERTY, property, false);
		if (NEXDOME_CALLIBRATE_ITEM->sw.value) {
			NEXDOME_CALLIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
			if(!nexdome_callibrate(device)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_callibrate(%d): returned error.", PRIVATE_DATA->handle);
				indigo_set_switch(NEXDOME_CALLIBRATE_PROPERTY, NEXDOME_CALLIBRATE_ITEM, false);
				NEXDOME_CALLIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, NEXDOME_CALLIBRATE_PROPERTY, "Callibration failed. Is the dome in home position?");
				return INDIGO_OK;
			} else {
				PRIVATE_DATA->callibration_requested = true;
			}
		}
		indigo_update_property(device, NEXDOME_CALLIBRATE_PROPERTY, NULL);
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
	indigo_release_property(NEXDOME_REVERSED_PROPERTY);
	indigo_release_property(NEXDOME_RESET_SHUTTER_COMM_PROPERTY);
	indigo_release_property(NEXDOME_FIND_HOME_PROPERTY);
	indigo_release_property(NEXDOME_CALLIBRATE_PROPERTY);
	indigo_release_property(NEXDOME_POWER_PROPERTY);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static nexdome_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_nexdome(indigo_driver_action action, indigo_driver_info *info) {
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
