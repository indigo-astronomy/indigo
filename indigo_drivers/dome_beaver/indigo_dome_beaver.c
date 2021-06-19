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

/** INDIGO Nexdome Beaver Dome driver
 \file indigo_dome_beaver.c
 */

#define DRIVER_VERSION 0x00001
#define DRIVER_NAME	"indigo_dome_beaver"

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

#include "indigo_dome_beaver.h"

// gp_bits is used as boolean
#define is_connected                        gp_bits
#define DEVICE_CONNECTED                    (device->is_connected)

#define PRIVATE_DATA                        ((beaver_private_data *)device->private_data)
#define DEFAULT_BAUDRATE                    "115200"

#define X_SETTINGS_GROUP                    "Settings"

#define X_EMERGENCY_CLOSE_PROPERTY               (PRIVATE_DATA->emergency_property)
#define X_EMERGENCY_RAIN_ITEM                    (X_EMERGENCY_CLOSE_PROPERTY->items+0)
#define X_EMERGENCY_WIND_ITEM                    (X_EMERGENCY_CLOSE_PROPERTY->items+1)
#define X_EMERGENCY_OPERATION_TIMEOUT_ITEM       (X_EMERGENCY_CLOSE_PROPERTY->items+2)
#define X_EMERGENCY_POWERCUT_ITEM                (X_EMERGENCY_CLOSE_PROPERTY->items+3)
#define X_EMERGENCY_CLOSE_PROPERTY_NAME          "X_EMERGENCY_CLOSE"
#define X_EMERGENCY_RAIN_ITEM_NAME               "RAIN"
#define X_EMERGENCY_WIND_ITEM_NAME               "WIND"
#define X_EMERGENCY_OPERATION_TIMEOUT_ITEM_NAME  "OPERATION_TIMEOUT"
#define X_EMERGENCY_POWERCUT_ITEM_NAME           "POWER_CUT"

#define LUNATICO_CMD_LEN 100

typedef enum {
	MODEL_SELETEK = 1,
	MODEL_ARMADILLO = 2,
	MODEL_PLATYPUS = 3,
	MODEL_DRAGONFLY = 4,
	MODEL_LIMPET = 5,
	MODEL_XXX = 6,
	MODEL_BEEVER_ROTATOR = 7,
	MODEL_BEEVER_SHUTTER = 8
} lunatico_model_t;

typedef enum {
	BD_SUCCESS = 0,
	BD_PARAM_ERROR = 1,
	BD_COMMAND_ERROR = 2,
	BD_NO_RESPONSE = 3,
	BD_DOME_ERROR = 4
} beaver_rc_t;

typedef struct {
	int handle;
	bool udp;
	int count_open;
	float target_position, current_position;
	int shutter_position;
	bool rain, wind, timeout, powercut;
	bool park_requested;
	bool aborted;
	float park_azimuth;
	pthread_mutex_t port_mutex;
	indigo_timer *dome_timer;
	indigo_property *emergency_property;
} beaver_private_data;

#define BEAVER_CMD_LEN 10


static bool beaver_command(indigo_device *device, const char *command, char *response, int max, int sleep) {
	char c;
	char buff[LUNATICO_CMD_LEN];
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
		if (PRIVATE_DATA->udp) {
			result = read(PRIVATE_DATA->handle, buff, LUNATICO_CMD_LEN);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			break;
		} else {
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
		}
	}

	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (sleep > 0)
		usleep(sleep);

	// read responce
	if (response != NULL) {
		long index = 0;
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
			if (PRIVATE_DATA->udp) {
				result = read(PRIVATE_DATA->handle, response, LUNATICO_CMD_LEN);
				if (result < 1) {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				index = result;
				break;
			} else {
				result = read(PRIVATE_DATA->handle, &c, 1);
				if (result < 1) {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				response[index++] = c;
				if (c == '#') break;
			}
		}
		response[index] = '\0';
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static bool beaver_get_info(indigo_device *device, char *board, char *firmware) {
	if(!board || !firmware) return false;

	const char *models[9] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet", "No Idea", "Beaver (rotator)", "Beaver (shutter)" };
	int fwmaj, fwmin, model, oper, data;
	char response[LUNATICO_CMD_LEN]={0};
	if (beaver_command(device, "!seletek version#", response, sizeof(response), 100)) {
		// !seletek version:2510#
		int parsed = sscanf(response, "!seletek version:%d#", &data);
		if (parsed != 1) return false;
		oper = data / 10000;		// 0 normal, 1 bootloader
		model = (data / 1000) % 10;	// 1 seletek, etc.
		fwmaj = (data / 100) % 10;
		fwmin = (data % 100);
		if (oper >= 2) oper = 2;
		if (model > 8) model = 0;
		sprintf(board, "%s", models[model]);
		sprintf(firmware, "%d.%d", fwmaj, fwmin);

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "!seletek version# -> %s = %s %s", response, board, firmware);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}

static beaver_rc_t beaver_get_serial_number(indigo_device *device, char *serial_num) {
	return BD_NO_RESPONSE;
}

static bool beaver_open(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OPEN REQUESTED: %d -> %d, count_open = %d", PRIVATE_DATA->handle, DEVICE_CONNECTED, PRIVATE_DATA->count_open);
	if (DEVICE_CONNECTED) return false;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			return false;
		}
		char *name = DEVICE_PORT_ITEM->text.value;
		if (!indigo_is_device_url(name, "nexdome")) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening local device on port: '%s', baudrate = %d", DEVICE_PORT_ITEM->text.value, atoi(DEVICE_BAUDRATE_ITEM->text.value));
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value));
			PRIVATE_DATA->udp = false;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening network device on host: %s", DEVICE_PORT_ITEM->text.value);
			indigo_network_protocol proto = INDIGO_PROTOCOL_UDP;
			PRIVATE_DATA->handle = indigo_open_network_device(name, 10000, &proto);
			PRIVATE_DATA->udp = true;
		}
		if (PRIVATE_DATA->handle < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_global_unlock(device);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);

	bool beaver = false;
	char message[INDIGO_VALUE_SIZE] = "";
	char board[LUNATICO_CMD_LEN] = "N/A";
	char firmware[LUNATICO_CMD_LEN] = "N/A";
	bool success = beaver_get_info(device, board, firmware);
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (success) {
		if (!strncmp(board, "Beaver (rotator)", 16)) {
			beaver = true;
			indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
			indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
			indigo_update_property(device, INFO_PROPERTY, NULL);
		} else if (!strncmp(board, "Beaver (shutter)", 16)) {
			beaver = false;
			indigo_copy_value(message, "Beaver shutter controler found, this driver works with Beaver rotator");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
		} else {
			beaver = false;
			indigo_copy_value(message, "Connected device is not a Beaver dome controler");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
		}
	} else {
		beaver = false;
		indigo_copy_value(message, "No response from the device");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
	}
	if (!beaver) {
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_update_property(device, CONNECTION_PROPERTY, message);
		if (--PRIVATE_DATA->count_open == 0) {
			close(PRIVATE_DATA->handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d)", PRIVATE_DATA->handle);
			indigo_global_unlock(device);
			PRIVATE_DATA->handle = 0;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}
	device->is_connected = true;
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}


static void beaver_close(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSE REQUESTED: %d -> %d, count_open = %d", PRIVATE_DATA->handle, DEVICE_CONNECTED, PRIVATE_DATA->count_open);
	if (!DEVICE_CONNECTED) return;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		close(PRIVATE_DATA->handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d)", PRIVATE_DATA->handle);
		indigo_global_unlock(device);
		PRIVATE_DATA->handle = 0;
	}
	indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, DOME_BEAVER_NAME);
	indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	device->is_connected = false;
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
}


static beaver_rc_t beaver_abort(indigo_device *device) {
	return BD_NO_RESPONSE;
}


static beaver_rc_t beaver_get_azimuth(indigo_device *device, float *azimuth) {
	return BD_NO_RESPONSE;
}


static beaver_rc_t beaver_get_shutter_position(indigo_device *device, int *pos) {
	return BD_NO_RESPONSE;
}


static beaver_rc_t beaver_open_shutter(indigo_device *device) {
	return BD_NO_RESPONSE;
}


static beaver_rc_t beaver_close_shutter(indigo_device *device) {
	return BD_NO_RESPONSE;
}


static beaver_rc_t beaver_get_emergency_status(indigo_device *device, bool *rain, bool *wind, bool *timeout, bool *powercut) {
	return BD_NO_RESPONSE;
}


static beaver_rc_t beaver_goto_azimuth(indigo_device *device, float azimuth) {
	return BD_NO_RESPONSE;
}

//static bool beaver_callibrate(indigo_device *device) {
//	return false;
//}


// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_timer_callback(indigo_device *device) {
	static int prev_shutter_position = -1;
	beaver_rc_t rc;

	/* Handle dome rotation */
	if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(): returned error %d", rc);
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
	if ((rc = beaver_get_shutter_position(device, &PRIVATE_DATA->shutter_position)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_shutter_position(): returned error %d", rc);
	}
	if (PRIVATE_DATA->shutter_position != prev_shutter_position || DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
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

	/* Keep the dome in sync if needed */
	if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
		double az;
		if (indigo_fix_dome_azimuth(device, DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value, DOME_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value, &az) &&
		   (DOME_HORIZONTAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE)) {
			PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = az;
			if ((rc = beaver_goto_azimuth(device, PRIVATE_DATA->target_position)) != BD_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				if (rc == BD_DOME_ERROR) {
					indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome sync failed with DOME_ERROR. Please inspect the dome!");
				} else {
					indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				}
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

		PRIVATE_DATA->aborted = false;
	}

	/* Emergency flags state */
	bool rain, wind, timeout, powercut;
	if ((rc = beaver_get_emergency_status(device, &rain, &wind, &timeout, &powercut)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_emergency_status(): returned error %d", rc);
	} else {
		if (X_EMERGENCY_CLOSE_PROPERTY->state == INDIGO_IDLE_STATE ||
		    PRIVATE_DATA->rain != rain ||
		    PRIVATE_DATA->wind != wind ||
		    PRIVATE_DATA->timeout != timeout ||
		    PRIVATE_DATA->powercut != powercut)
		{
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
	}

	indigo_reschedule_timer(device, 1, &(PRIVATE_DATA->dome_timer));
}


static indigo_result beaver_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_EMERGENCY_CLOSE_PROPERTY, property))
			indigo_define_property(device, X_EMERGENCY_CLOSE_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE);
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
		DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_EMERGENCY_CLOSE_PROPERTY
		X_EMERGENCY_CLOSE_PROPERTY = indigo_init_light_property(NULL, device->name, X_EMERGENCY_CLOSE_PROPERTY_NAME, DOME_MAIN_GROUP, "Energency close flags", INDIGO_IDLE_STATE, 4);
		if (X_EMERGENCY_CLOSE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(X_EMERGENCY_RAIN_ITEM, X_EMERGENCY_RAIN_ITEM_NAME, "Rain alert", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_EMERGENCY_WIND_ITEM, X_EMERGENCY_WIND_ITEM_NAME, "Wind alert", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_EMERGENCY_OPERATION_TIMEOUT_ITEM, X_EMERGENCY_OPERATION_TIMEOUT_ITEM_NAME, "Operation timeout alert", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_EMERGENCY_POWERCUT_ITEM, X_EMERGENCY_POWERCUT_ITEM_NAME, "Power coutage alert", INDIGO_IDLE_STATE);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void dome_connect_callback(indigo_device *device) {
	beaver_rc_t rc;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (beaver_open(device)) { // Successfully connected
				X_EMERGENCY_CLOSE_PROPERTY->state =
				X_EMERGENCY_RAIN_ITEM->light.value =
				X_EMERGENCY_WIND_ITEM->light.value =
				X_EMERGENCY_OPERATION_TIMEOUT_ITEM->light.value =
				X_EMERGENCY_POWERCUT_ITEM->light.value = INDIGO_IDLE_STATE;
				indigo_define_property(device, X_EMERGENCY_CLOSE_PROPERTY, NULL);

				if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(): returned error %d", rc);
				}
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
				PRIVATE_DATA->aborted = false;
				PRIVATE_DATA->park_azimuth = 0;
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
			indigo_delete_property(device, X_EMERGENCY_CLOSE_PROPERTY, NULL);
			beaver_close(device);
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
	beaver_rc_t rc;
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
		if (DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK;
		}
		indigo_property_copy_values(DOME_STEPS_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}

		if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
		}

		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = ((int)(10 * (PRIVATE_DATA->current_position - DOME_STEPS_ITEM->number.value) + 3600) % 3600) / 10.0;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PRIVATE_DATA->target_position = %f\n", PRIVATE_DATA->target_position);
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = ((int)(10 * (PRIVATE_DATA->current_position + DOME_STEPS_ITEM->number.value) + 3600) % 3600) / 10.0;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PRIVATE_DATA->target_position = %f\n", PRIVATE_DATA->target_position);
		}

		if ((rc = beaver_goto_azimuth(device, PRIVATE_DATA->target_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			if (rc == BD_DOME_ERROR) {
				indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed with DOME_ERROR. Please inspect the dome!");
			} else {
				indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed");
			}
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
			return INDIGO_OK;
		}
		indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (DOME_PARK_PARKED_ITEM->sw.value) {
			if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
			}
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;

			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked");
			return INDIGO_OK;
		}
		PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
		if ((rc = beaver_goto_azimuth(device, PRIVATE_DATA->target_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			if (rc == BD_DOME_ERROR) {
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Goto azimuth failed with DOME_ERROR. Please inspect the dome!");
			} else {
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Goto azimuth failed");
			}
			return INDIGO_OK;
		}
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
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

		if ((rc = beaver_abort(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_abort(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_ABORT_MOTION_ITEM->sw.value = false;
			if (rc == BD_DOME_ERROR) {
				indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, "Abort failed with DOME_ERROR. Please inspect the dome!");
			} else {
				indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, "Abort failed");
			}
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
			return INDIGO_OK;
		 }
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		char message[INDIGO_VALUE_SIZE] = {0};
		beaver_rc_t rc;
		if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
			rc = beaver_open_shutter(device);
			strncpy(message,"Opening shutter...", INDIGO_VALUE_SIZE);
		} else {
			rc = beaver_close_shutter(device);
			strncpy(message,"Closing shutter...", INDIGO_VALUE_SIZE);
		}
		if (rc != BD_SUCCESS) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			if (rc == BD_DOME_ERROR) {
				indigo_update_property(device, DOME_STEPS_PROPERTY, "Shutter open/close failed with DOME_ERROR. Please inspect the dome!");
			} else {
				indigo_update_property(device, DOME_STEPS_PROPERTY, "Shutter open/close failed");
			}
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
			return INDIGO_OK;
		}
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, message);
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

			if ((rc = beaver_goto_azimuth(device, PRIVATE_DATA->park_azimuth)) != BD_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
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
	indigo_release_property(X_EMERGENCY_CLOSE_PROPERTY);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

static beaver_private_data *private_data = NULL;
static indigo_device *dome = NULL;

indigo_result indigo_dome_beaver(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_BEAVER_NAME,
		dome_attach,
		beaver_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DOME_BEAVER_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(beaver_private_data));
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
