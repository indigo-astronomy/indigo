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

#define DRIVER_VERSION 0x00001
#define DRIVER_NAME	"indigo_dome_nexdome"

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

#include "indigo_dome_nexdome.h"

// gp_bits is used as boolean
#define is_connected                    gp_bits

#define PRIVATE_DATA					((nexdome_private_data *)device->private_data)

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

typedef struct {
	int handle;
	float target_position, current_position;
	nexdome_dome_state_t dome_state;
	nexdome_shutter_state_t shutter_state;
	pthread_mutex_t port_mutex;
	indigo_timer *dome_timer;
} nexdome_private_data;


#define NEXDOME_CMD_LEN 100

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
		if (result == 0)
			break;
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

	// read responce
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
	if (nexdome_command(device, "v\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "V%s V %s", name, firmware);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "v -> %s, '%s' '%s'", response, name, firmware);
		if (parsed != 2) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_abort(indigo_device *device) {
	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "a\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "a -> %s", response);
		if (response[0] != 'A') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_dome_state(indigo_device *device, nexdome_dome_state_t *state) {
	if(!state) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "m\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "M %d", (int*)state);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "m -> %s, %d", response, *state);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_get_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "q\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "Q %f", azimuth);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "q -> %s, %f", response, *azimuth);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_shutter_state(indigo_device *device, nexdome_shutter_state_t *state, bool *not_raining) {
	if(!state || !not_raining) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "u\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "U %d %d", (int*)state, (int*)not_raining);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "u -> %s, %d %d", response, *state, *not_raining);
		if (parsed != 2) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_get_shutter_position(indigo_device *device, float *pos) {
	if(!pos) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "b\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "B %f", pos);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "b -> %s, %f", response, *pos);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_open_shutter(indigo_device *device) {
	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "d\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "d -> %s", response);
		if (response[0] != 'D') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_close_shutter(indigo_device *device) {
	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "e\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "e -> %s", response);
		if (response[0] != 'D') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_set_shutter_position(indigo_device *device, float position) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "f %.2f\n", position);

	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "f %.2f -> %s", position, response);
		if (response[0] != 'F') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_sync_azimuth(indigo_device *device, float azimuth) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "s %.2f\n", azimuth);

	// we ignore the returned sync value
	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "s %.2f -> %s", azimuth, response);
		if (response[0] != 'S') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_get_home_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "i\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "I %f", azimuth);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "i -> %s, %f", response, *azimuth);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_set_home_azimuth(indigo_device *device, float azimuth) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "j %.2f\n", azimuth);

	// we ignore the returned value
	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "j %.2f -> %s", azimuth, response);
		if (response[0] != 'I') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_get_park_azimuth(indigo_device *device, float *azimuth) {
	if(!azimuth) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "n\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "N %f", azimuth);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "n -> %s, %f", response, *azimuth);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_set_park_azimuth(indigo_device *device, float azimuth) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "l %.2f\n", azimuth);

	// we ignore the returned value
	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "l %.2f -> %s", azimuth, response);
		if (response[0] != 'N') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_goto_azimuth(indigo_device *device, float azimuth) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "g %.2f\n", azimuth);

	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "g %.2f -> %s", azimuth, response);
		if (response[0] != 'G') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_find_home(indigo_device *device) {
	char response[NEXDOME_CMD_LEN] = {0};

	if (nexdome_command(device, "h\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "h -> %s", response);
		if (response[0] != 'H') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_get_home_state(indigo_device *device, int *state) {
	if(!state) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "z\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "Z %d", state);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "z -> %s, %d", response, *state);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_callibrate(indigo_device *device) {
	char response[NEXDOME_CMD_LEN] = {0};

	if (nexdome_command(device, "c\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "c -> %s", response);
		if (response[0] != 'C') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_get_reversed_flag(indigo_device *device, bool *reversed) {
	if(!reversed) return false;

	char response[NEXDOME_CMD_LEN]={0};
	if (nexdome_command(device, "y\n", response, sizeof(response), NEXDOME_CMD_LEN)) {
		int parsed = sscanf(response, "Y %d", (int*)reversed);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "y -> %s, %d", response, *reversed);
		if (parsed != 1) return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool nexdome_set_reversed_flag(indigo_device *device, bool reversed) {
	char command[NEXDOME_CMD_LEN];
	char response[NEXDOME_CMD_LEN] = {0};

	snprintf(command, NEXDOME_CMD_LEN, "y %d\n", (int)reversed);

	if (nexdome_command(device, command, response, sizeof(response), NEXDOME_CMD_LEN)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "y %d -> %s, %d", reversed, response);
		if (response[0] != 'Y') return false;
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_timer_callback(indigo_device *device) {
	static bool need_update = true;
	static nexdome_shutter_state_t prev_shutter_state = SHUTTER_STATE_UNKNOWN;

	if(!nexdome_get_azimuth(device, &PRIVATE_DATA->current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(): returned error");
	}

	if(!nexdome_dome_state(device, &PRIVATE_DATA->dome_state)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_dome_state(): returned error");
	}

	if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) need_update = true;

	if (PRIVATE_DATA->dome_state != DOME_STOPED) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		need_update = true;
	} else if(need_update) {
		if (fabs((PRIVATE_DATA->target_position - PRIVATE_DATA->current_position)*100) >= 100) {
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
		need_update = false;
	}

	bool raining;
	if(!nexdome_shutter_state(device, &PRIVATE_DATA->shutter_state, &raining)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_shutter_state(): returned error");
	}
	if (PRIVATE_DATA->shutter_state != prev_shutter_state) {
		switch(PRIVATE_DATA->shutter_state) {
		case SHUTTER_STATE_NOT_CONNECTED:
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		case SHUTTER_STATE_OPEN:
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
			DOME_SHUTTER_OPENED_ITEM->sw.value = true;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			break;
		case SHUTTER_STATE_CLOSED:
			DOME_SHUTTER_CLOSED_ITEM->sw.value = true;
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			break;
		case SHUTTER_STATE_OPENING:
		case SHUTTER_STATE_CLOSING:
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
			DOME_SHUTTER_OPENED_ITEM->sw.value = true;
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			break;
		case SHUTTER_STATE_UNKNOWN:
			DOME_SHUTTER_PROPERTY->state = INDIGO_IDLE_STATE;
			break;
		}
		prev_shutter_state = PRIVATE_DATA->shutter_state;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	}

	indigo_set_timer(device, 1, dome_timer_callback);
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
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
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
				char *name = DEVICE_PORT_ITEM->text.value;
				if (strncmp(name, "nexdome://", 10)) {
					PRIVATE_DATA->handle = indigo_open_serial(name);
					/* To be on the safe side -> Wait for 2 seconds! */
					sleep(2);
				} else {
					char *host = name + 8;
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
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					return INDIGO_OK;;
				} else if (!nexdome_get_info(device, name, firmware)) {
					int res = close(PRIVATE_DATA->handle);
					if (res < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					}
					indigo_global_unlock(device);
					device->is_connected = false;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: Deep Sky Dad AF did not respond");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, "Deep Sky Dad AF did not respond");
					return INDIGO_OK;;
				} else { // Successfully connected
					uint32_t value;
					strncpy(INFO_DEVICE_MODEL_ITEM->text.value, name, INDIGO_VALUE_SIZE);
					strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					nexdome_get_azimuth(device, &PRIVATE_DATA->target_position);

					/*
					nexdome_dome_state_t state;
					nexdome_dome_state(device, &state);
					nexdome_abort(device);
					float heading;
					nexdome_get_azimuth(device, &heading);
					nexdome_shutter_state_t sstate;
					bool not_raining;
					nexdome_shutter_state(device, &sstate, &not_raining);
					float pos;
					nexdome_get_shutter_position(device, &pos);
					//nexdome_open_shutter(device);
					//nexdome_close_shutter(device);
					sleep(1);
					nexdome_set_shutter_position(device, 44.4);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "version = %s", firmware);
					nexdome_sync_azimuth(device, 24.1);
					nexdome_set_park_azimuth(device, 1.35);
					nexdome_get_park_azimuth(device, &pos);
					nexdome_set_home_azimuth(device, 54.3);
					nexdome_get_home_azimuth(device, &pos);
					nexdome_goto_azimuth(device, 31.9);
					nexdome_find_home(device);
					nexdome_callibrate(device);
					nexdome_get_home_state(device, &state);
					bool reversed;
					nexdome_get_reversed_flag(device, &reversed);
					nexdome_set_reversed_flag(device, 1);
					nexdome_get_reversed_flag(device, &reversed);
					*/

					//dsd_get_position(device, &position);
					//FOCUSER_POSITION_ITEM->number.value = (double)position;

					//if (!dsd_get_max_position(device, &PRIVATE_DATA->max_position)) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsd_get_max_position(%d) failed", PRIVATE_DATA->handle);
					//}
					//FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;

					//if (!dsd_get_speed(device, &value)) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsd_get_speed(%d) failed", PRIVATE_DATA->handle);
					//}
					//FOCUSER_SPEED_ITEM->number.value = (double)value;

					/* While we do not have max move property hardoce it to max position */
					//dsd_set_max_move(device, (uint32_t)FOCUSER_POSITION_ITEM->number.max);

					/* DSD does not have reverse motion, so we set it to be sure we know its state */
					//dsd_set_reverse(device, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);

					//update_step_mode_switches(device);
					//indigo_define_property(device, DSD_STEP_MODE_PROPERTY, NULL);

					//update_coils_mode_switches(device);
					//indigo_define_property(device, DSD_COILS_MODE_PROPERTY, NULL);

					//if (!dsd_get_move_current(device, &value)) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsd_get_move_current(%d) failed", PRIVATE_DATA->handle);
					//}
					//DSD_CURRENT_CONTROL_MOVE_ITEM->number.value = (double)value;
					//DSD_CURRENT_CONTROL_MOVE_ITEM->number.target = (double)value;
					//if (!dsd_get_hold_current(device, &value)) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsd_get_hold_current(%d) failed", PRIVATE_DATA->handle);
					//}
					//DSD_CURRENT_CONTROL_HOLD_ITEM->number.value = (double)value;
					//DSD_CURRENT_CONTROL_HOLD_ITEM->number.target = (double)value;
					//indigo_define_property(device, DSD_CURRENT_CONTROL_PROPERTY, NULL);

					//if (!dsd_get_settle_buffer(device, &value)) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsd_get_settle_buffer(%d) failed", PRIVATE_DATA->handle);
					//}
					//DSD_TIMINGS_SETTLE_ITEM->number.value = (double)value;
					//DSD_TIMINGS_SETTLE_ITEM->number.target = (double)value;
					//if (!dsd_get_coils_timeout(device, &value)) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "dsd_get_coils_timeout(%d) failed", PRIVATE_DATA->handle);
					//}
					//DSD_TIMINGS_COILS_TOUT_ITEM->number.value = (double)value;
					//DSD_TIMINGS_COILS_TOUT_ITEM->number.target = (double)value;
					//indigo_define_property(device, DSD_TIMINGS_PROPERTY, NULL);


					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected = ", PRIVATE_DATA->handle);
					PRIVATE_DATA->dome_timer = indigo_set_timer(device, 0.5, dome_timer_callback);
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->dome_timer);
				//indigo_delete_property(device, DSD_STEP_MODE_PROPERTY, NULL);
				//indigo_delete_property(device, DSD_COILS_MODE_PROPERTY, NULL);
				//indigo_delete_property(device, DSD_CURRENT_CONTROL_PROPERTY, NULL);
				//indigo_delete_property(device, DSD_TIMINGS_PROPERTY, NULL);

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
		// -------------------------------------------------------------------------------- DOME_EQUATORIAL_COORDINATES
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

		if(!nexdome_goto_azimuth(device, PRIVATE_DATA->target_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_get_azimuth(%d): returned error", PRIVATE_DATA->handle);
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
		double alt, az;
		if (indigo_fix_dome_coordinates(device, DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value, DOME_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, &alt, &az) == INDIGO_OK) {
			/*
			PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = az;
			int dif = (int)(PRIVATE_DATA->target_position - PRIVATE_DATA->current_position + 360) % 360;
			if (dif < 180) {
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = dif;
			} else if (dif > 180) {
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = 360 - dif;
			}
			*/
			DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
			//DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			//indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			//DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			//indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			//indigo_set_timer(device, 0.5, dome_timer_callback);
		} else {
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, "Invalid coordinates");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		}

		if(!nexdome_abort(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "nexdome_abort(%d): returned error", PRIVATE_DATA->handle);
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			return INDIGO_OK;
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
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->current_position > 180) {
				DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = 360 - PRIVATE_DATA->current_position;
			} else if (PRIVATE_DATA->current_position < 180) {
				DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = PRIVATE_DATA->current_position;
			}
			PRIVATE_DATA->target_position = 0;
			DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			//indigo_set_timer(device, 0.5, dome_timer_callback);
		}
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static nexdome_private_data *private_data = NULL;

static indigo_device *dome = NULL;

indigo_result indigo_dome_nexdome(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_SIMULATOR_NAME,
		dome_attach,
		indigo_dome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DOME_SIMULATOR_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

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
