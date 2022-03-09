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

#define DRIVER_VERSION 0x00002
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


#define X_MISC_GROUP                              "Misc"

#define X_SHUTTER_CALIBRATE_PROPERTY              (PRIVATE_DATA->shutter_cal_property)
#define X_SHUTTER_CALIBRATE_ITEM                  (X_SHUTTER_CALIBRATE_PROPERTY->items+0)
#define X_SHUTTER_CALIBRATE_PROPERTY_NAME         "X_SHUTTER_CALIBRATE"
#define X_SHUTTER_CALIBRATE_ITEM_NAME             "CALIBRATE"

#define X_ROTATOR_CALIBRATE_PROPERTY              (PRIVATE_DATA->rotator_cal_property)
#define X_ROTATOR_CALIBRATE_ITEM                  (X_ROTATOR_CALIBRATE_PROPERTY->items+0)
#define X_ROTATOR_CALIBRATE_PROPERTY_NAME         "X_ROTATOR_CALIBRATE"
#define X_ROTATOR_CALIBRATE_ITEM_NAME             "CALIBRATE"

#define X_FAILURE_MESSAGE_PROPERTY                (PRIVATE_DATA->failure_msg_property)
#define X_FAILURE_MESSAGE_ROTATOR_ITEM            (X_FAILURE_MESSAGE_PROPERTY->items+0)
#define X_FAILURE_MESSAGE_SHUTTER_ITEM            (X_FAILURE_MESSAGE_PROPERTY->items+1)
#define X_FAILURE_MESSAGE_PROPERTY_NAME           "X_FAILURE_MESSAGES"
#define X_FAILURE_MESSAGE_ROTATOR_ITEM_NAME       "ROTATOR"
#define X_FAILURE_MESSAGE_SHUTTER_ITEM_NAME       "SHUTTER"

#define X_CLEAR_FAILURE_PROPERTY                  (PRIVATE_DATA->clear_failure_property)
#define X_CLEAR_FAILURE_ITEM                      (X_CLEAR_FAILURE_PROPERTY->items+0)
#define X_CLEAR_FAILURE_PROPERTY_NAME             "X_CLEAR_FAILURES"
#define X_CLEAR_FAILURE_ITEM_NAME                 "CLEAR"

#define X_CONDITIONS_SAFETY_PROPERTY             (PRIVATE_DATA->conditions_safety_property)
#define X_SAFE_CW_ITEM                           (X_CONDITIONS_SAFETY_PROPERTY->items+0)
#define X_SAFE_HYDREON_ITEM                      (X_CONDITIONS_SAFETY_PROPERTY->items+1)

#define X_CONDITIONS_SAFETY_PROPERTY_NAME        "X_CONDITIONS_SAFETY"
#define X_SAFE_CW_ITEM_NAME                      "CLOUD_WATCHER"
#define X_SAFE_HYDREON_ITEM_NAME                 "HYDREON"

#define LUNATICO_CMD_LEN 100

typedef enum {
	BDB_ROTATOR_MOVING     = 0,
	BDB_SHUTTER_MOVING     = 1,
	BDB_ROTATOR_ERROR      = 2,
	BDB_SHUTTER_ERROR      = 3,
	BDB_COMM_ERROR         = 4,
	BDB_UNSAFE_CW          = 5,
	BDB_UNSAFE_HYDREON     = 6
} beaver_status_bits_t;

typedef enum {
	BDS_NO_CALIBRATION     = 0,
	BDS_CLIBRATING         = 1,
	BDS_CALIBRATED         = 2,
	BDS_CALIBRATION_ERROR  = 3
} beaver_shutter_callibration_status_t;


#define CHECK_BIT(bitmap, bit)  (((bitmap) >> (bit)) & 1UL)

typedef enum {
	MODEL_SELETEK = 1,
	MODEL_ARMADILLO = 2,
	MODEL_PLATYPUS = 3,
	MODEL_DRAGONFLY = 4,
	MODEL_LIMPET = 5,
	MODEL_LYNX = 6,
	MODEL_BEEVER_ROTATOR = 7,
	MODEL_BEEVER_SHUTTER = 8
} lunatico_model_t;

typedef enum {
	BD_SHUTTER_OPEN = 0,
	BD_SHUTTER_CLOSED = 1,
	BD_SHUTTER_OPENING = 2,
	BD_SHUTTER_CLOSING = 3,
	BD_SHUTTER_ERROR = 4
} beaver_shutter_status_t;

typedef enum {
	BD_SUCCESS = 0,
	BD_PARAM_ERROR = 1,
	BD_COMMAND_ERROR = 2,
	BD_NO_RESPONSE = 3
} beaver_rc_t;

typedef struct {
	int handle;
	bool udp;
	int count_open;
	float target_position, current_position;
	int shutter_status;
	int prev_shutter_status;
	int dome_status;
	int prev_dome_status;
	int rotator_failure_code, shutter_failure_code;
	bool shutter_is_up;
	//bool rain, wind, timeout, powercut;
	bool park_requested;
	bool aborted;
	pthread_mutex_t port_mutex;
	pthread_mutex_t move_mutex;
	indigo_timer *dome_timer;
	indigo_property *shutter_cal_property;
	indigo_property *rotator_cal_property;
	indigo_property *failure_msg_property;
	indigo_property *clear_failure_property;
	indigo_property *conditions_safety_property;
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

	const char *models[9] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet", "Lynx", "Beaver (rotator)", "Beaver (shutter)" };
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

static bool beaver_command_get_result_i(indigo_device *device, const char *command, int32_t *result) {
	if (!result) return false;

	char response[LUNATICO_CMD_LEN]={0};
	char response_prefix[LUNATICO_CMD_LEN];
	char format[LUNATICO_CMD_LEN];

	if (beaver_command(device, command, response, sizeof(response), 100)) {
		strncpy(response_prefix, command, LUNATICO_CMD_LEN);
		char *p = strrchr(response_prefix, '#');
		if (p) *p = ':';
		sprintf(format, "%s%%d#", response_prefix);
		int parsed = sscanf(response, format, result);
		if (parsed != 1) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s = %d", command, response, *result);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}

static bool beaver_command_get_result_f(indigo_device *device, const char *command, float *result) {
	if (!result) return false;

	char response[LUNATICO_CMD_LEN]={0};
	char response_prefix[LUNATICO_CMD_LEN];
	char format[LUNATICO_CMD_LEN];

	if (beaver_command(device, command, response, sizeof(response), 100)) {
		strncpy(response_prefix, command, LUNATICO_CMD_LEN);
		char *p = strrchr(response_prefix, '#');
		if (p) *p = ':';
		sprintf(format, "%s%%f#", response_prefix);
		int parsed = sscanf(response, format, result);
		if (parsed != 1) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s = %f", command, response, *result);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool beaver_command_get_result_s(indigo_device *device, const char *command, char *string) {
	if (!string) return false;

	char response[INDIGO_VALUE_SIZE]={0};
	char response_prefix[INDIGO_VALUE_SIZE];
	char format[LUNATICO_CMD_LEN];

	if (beaver_command(device, command, response, sizeof(response), 100)) {
		//strncpy(response, "!seletek getfailuremsg:krpok frepofkkorep#", INDIGO_VALUE_SIZE);
		strncpy(response_prefix, command, INDIGO_VALUE_SIZE);
		char *p = strrchr(response_prefix, '#');
		if (p) *p = ':';
		sprintf(format, "%s%%[^#]", response_prefix);
		int parsed = sscanf(response, format, string);
		if (parsed != 1) string[0] = '\0';
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s = '%s'", command, response, string);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
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
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome abort 1#", &res)) return BD_NO_RESPONSE;
	if (res != 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_save(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!seletek savefs#", &res)) return BD_NO_RESPONSE;
	if (res != 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_azimuth(indigo_device *device, float *azimuth) {
	bool res = false;
	res = beaver_command_get_result_f(device, "!dome getaz#", azimuth);
	if ((res == false) || (*azimuth < 0)) return BD_NO_RESPONSE;
	else return BD_SUCCESS;
}


static beaver_rc_t beaver_goto_azimuth(indigo_device *device, float azimuth) {
	char command[LUNATICO_CMD_LEN];
	int res = 0;
	snprintf(command, LUNATICO_CMD_LEN, "!dome gotoaz %f#", azimuth);
	if (!beaver_command_get_result_i(device, command, &res)) return BD_NO_RESPONSE;
	if (res != 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_set_azimuth(indigo_device *device, float azimuth) {
	char command[LUNATICO_CMD_LEN];
	int res = 0;
	snprintf(command, LUNATICO_CMD_LEN, "!dome setaz %f#", azimuth);
	if (!beaver_command_get_result_i(device, command, &res)) return BD_NO_RESPONSE;
	if (res != 0) return BD_COMMAND_ERROR;
	beaver_save(device);
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_shutter_status(indigo_device *device, int *status) {
	if (!beaver_command_get_result_i(device, "!dome shutterstatus#", status)) return BD_NO_RESPONSE;
	if (*status < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_failure_messages(indigo_device *device, char *rotator_message, char *shutter_message) {
	if (!beaver_command_get_result_s(device, "!seletek getfailuremsg#", rotator_message)) return BD_NO_RESPONSE;
	if (PRIVATE_DATA->shutter_is_up) {
		if (!beaver_command_get_result_s(device, "!dome sendtoshutter \"seletek getfailuremsg\"#", shutter_message)) return BD_NO_RESPONSE;
	}
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_failure_codes(indigo_device *device, int *rotator_code, int *shutter_code) {
	if (!beaver_command_get_result_i(device, "!seletek getfailurecode#", rotator_code)) return BD_NO_RESPONSE;
	if (PRIVATE_DATA->shutter_is_up) {
		if (!beaver_command_get_result_i(device, "!dome sendtoshutter \"seletek getfailurecode\"#", shutter_code)) return BD_NO_RESPONSE;
	}
	return BD_SUCCESS;
}


static beaver_rc_t beaver_clear_failure(indigo_device *device) {
	int res1 = -1, res2 = -1;
	if (!beaver_command_get_result_i(device, "!seletek clearfailure#", &res1)) return BD_NO_RESPONSE;
	if (PRIVATE_DATA->shutter_is_up) {
		if (!beaver_command_get_result_i(device, "!dome sendtoshutter \"seletek clearfailure\"#", &res2)) return BD_NO_RESPONSE;
	}
	if (res1 < 0 || res2 < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_is_athome(indigo_device *device, int *athome) {
	if (!beaver_command_get_result_i(device, "!dome athome#", athome)) return BD_NO_RESPONSE;
	if (*athome < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_is_parked(indigo_device *device, int *parked) {
	if (!beaver_command_get_result_i(device, "!dome atpark#", parked)) return BD_NO_RESPONSE;
	if (*parked < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_goto_park(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome gopark#", &res)) return BD_NO_RESPONSE;
	if (res < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_set_park(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome setpark#", &res)) return BD_NO_RESPONSE;
	if (res < 0) return BD_COMMAND_ERROR;
	beaver_save(device);
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_park(indigo_device *device, float *azimuth) {
	if (!beaver_command_get_result_f(device, "!domerot getpark#", azimuth)) return BD_NO_RESPONSE;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_calibrate_rotator(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome autocalrot 2#", &res)) return BD_NO_RESPONSE;
	if (res < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_calibrate_shutter(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome autocalshutter#", &res)) return BD_NO_RESPONSE;
	if (res < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_shutter_calibration_status(indigo_device *device, int *status) {
	if (!beaver_command_get_result_i(device, "!dome sendtoshutter \"shutter getcalibrationstatus\"#", status)) return BD_NO_RESPONSE;
	if (*status < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_goto_home(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome gohome#", &res)) return BD_NO_RESPONSE;
	if (res < 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_dome_status(indigo_device *device, int *status) {
	if (!beaver_command_get_result_i(device, "!dome status#", status)) return BD_NO_RESPONSE;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_shutterisup(indigo_device *device, int *status) {
	if (!beaver_command_get_result_i(device, "!dome shutterisup#", status)) return BD_NO_RESPONSE;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_shutter_enable(indigo_device *device, bool enable) {
	int res = -1;
	if (enable) {
		if (!beaver_command_get_result_i(device, "!dome setshutterenable 1#", &res)) return BD_NO_RESPONSE;
	} else {
		if (!beaver_command_get_result_i(device, "!dome setshutterenable 0#", &res)) return BD_NO_RESPONSE;
	}
	if (res != 0) return BD_COMMAND_ERROR;
	beaver_save(device);
	return BD_SUCCESS;
}


static beaver_rc_t beaver_open_shutter(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome openshutter#", &res)) return BD_NO_RESPONSE;
	if (res != 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_close_shutter(indigo_device *device) {
	int res = -1;
	if (!beaver_command_get_result_i(device, "!dome closeshutter#", &res)) return BD_NO_RESPONSE;
	if (res != 0) return BD_COMMAND_ERROR;
	return BD_SUCCESS;
}


static beaver_rc_t beaver_get_emergency_status(indigo_device *device, bool *rain, bool *wind, bool *timeout, bool *powercut) {
	return BD_SUCCESS;
}

// -------------------------------------------------------------------------------- INDIGO dome device implementation
static void dome_timer_callback(indigo_device *device) {
	beaver_rc_t rc;

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);

	if ((rc = beaver_get_dome_status(device, &PRIVATE_DATA->dome_status)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_dome_status(): returned error %d", rc);
	}

	if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(): returned error %d", rc);
	}

	/* Handle observing conditions */
	if (CHECK_BIT(PRIVATE_DATA->dome_status, BDB_UNSAFE_CW) != CHECK_BIT(PRIVATE_DATA->prev_dome_status, BDB_UNSAFE_CW) ||
	    CHECK_BIT(PRIVATE_DATA->dome_status, BDB_UNSAFE_HYDREON) != CHECK_BIT(PRIVATE_DATA->prev_dome_status, BDB_UNSAFE_HYDREON)
	) {
		X_CONDITIONS_SAFETY_PROPERTY->state = INDIGO_OK_STATE;
		X_SAFE_CW_ITEM->light.value = (CHECK_BIT(PRIVATE_DATA->dome_status, BDB_UNSAFE_CW)) ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		X_SAFE_HYDREON_ITEM->light.value = (CHECK_BIT(PRIVATE_DATA->dome_status, BDB_UNSAFE_HYDREON)) ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		if (CHECK_BIT(PRIVATE_DATA->dome_status, BDB_UNSAFE_CW) || CHECK_BIT(PRIVATE_DATA->dome_status, BDB_UNSAFE_HYDREON)) {
			indigo_update_property(device, X_CONDITIONS_SAFETY_PROPERTY, "Unsafe weather conditions reported, check X_CONDITIONS_SAFETY prperty");
		} else {
			indigo_update_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);
		}
	}

	/* Handle dome rotation */
	if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ||
	    DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE ||
		PRIVATE_DATA->dome_status != PRIVATE_DATA->prev_dome_status
	) {
		if (CHECK_BIT(PRIVATE_DATA->dome_status, BDB_ROTATOR_MOVING)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Rotator stationary = %d", PRIVATE_DATA->dome_status);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Rotator moving = %d", PRIVATE_DATA->dome_status);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		}

		int atpark = 0;
		if ((rc = beaver_is_parked(device, &atpark)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_is_atpark(): returned error %d", rc);
		}
		if (atpark && PRIVATE_DATA->park_requested) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
			indigo_update_property(device, DOME_PARK_PROPERTY, "Dome parked");

			float park_pos;
			if ((rc = beaver_get_park(device, &park_pos)) != BD_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_park(%d): returned error %d", PRIVATE_DATA->handle, rc);
			} else {
				DOME_PARK_POSITION_AZ_ITEM->number.target = DOME_PARK_POSITION_AZ_ITEM->number.value = park_pos;
				DOME_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
			}
		}

		int athome = 0;
		if ((rc = beaver_is_athome(device, &athome)) != BD_SUCCESS)  {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_is_athome(): returned error %d", rc);
		}
		if (DOME_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (athome) {
				DOME_HOME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_HOME_PROPERTY, DOME_HOME_ITEM, true);
				indigo_update_property(device, DOME_HOME_PROPERTY, "Dome is at home");
			} else if (!CHECK_BIT(PRIVATE_DATA->dome_status, BDB_ROTATOR_MOVING)) {
				DOME_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(DOME_HOME_PROPERTY, DOME_HOME_ITEM, false);
				indigo_update_property(device, DOME_HOME_PROPERTY, "Failed to find home.");
			}
		} else {
			if ((bool)athome != DOME_HOME_ITEM->sw.value) {
				DOME_HOME_ITEM->sw.value = (bool)athome;
				indigo_update_property(device, DOME_HOME_PROPERTY, NULL);
			}
		}

		PRIVATE_DATA->prev_dome_status = PRIVATE_DATA->dome_status;
	}

	/* Handle dome rotator calibration */
	if (!CHECK_BIT(PRIVATE_DATA->dome_status, BDB_ROTATOR_MOVING)) {
		if (X_ROTATOR_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
			X_ROTATOR_CALIBRATE_ITEM->sw.value = false;
			if (CHECK_BIT(PRIVATE_DATA->dome_status, BDB_ROTATOR_ERROR)) {
				X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration failed");
			} else {
				X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration complete");
			}
		}
	}

	/* Handle dome shutter */
	if ((rc = beaver_get_shutter_status(device, &PRIVATE_DATA->shutter_status)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_shutter_status(): returned error %d", rc);
	}
	if (PRIVATE_DATA->shutter_status != PRIVATE_DATA->prev_shutter_status) {
		if (PRIVATE_DATA->shutter_status == BD_SHUTTER_OPEN) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter open");
		} else if (PRIVATE_DATA->shutter_status == BD_SHUTTER_CLOSED) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter closed");
		} else if (PRIVATE_DATA->shutter_status == BD_SHUTTER_ERROR) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			if (PRIVATE_DATA->aborted) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter aborted");
			} else {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter error");
			}
		} else if (PRIVATE_DATA->shutter_status == BD_SHUTTER_OPENING){
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Opening shutter...");
		} else if (PRIVATE_DATA->shutter_status == BD_SHUTTER_CLOSING){
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Closing shutter...");
		}
		PRIVATE_DATA->prev_shutter_status = PRIVATE_DATA->shutter_status;
	}

	/* Handle dome shutter calibration */
	if (X_SHUTTER_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_SHUTTER_CALIBRATE_ITEM->sw.value = false;
		int cal_status;
		if ((rc = beaver_get_shutter_calibration_status(device, &cal_status)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_shutter_calibration_status(): returned error %d", rc);
		}
		if (cal_status == BDS_CALIBRATION_ERROR) {
			X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration failed");
		} else if (cal_status == BDS_CALIBRATED || cal_status == BDS_NO_CALIBRATION) {
			X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration complete");
		}
	}

	/* Abort */
	if (PRIVATE_DATA->aborted) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->prev_dome_status = PRIVATE_DATA->dome_status;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);

		DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		PRIVATE_DATA->aborted = false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);

	/* Handle failures */
	if (X_CLEAR_FAILURE_ITEM->sw.value) {
		X_CLEAR_FAILURE_ITEM->sw.value = false;
		if ((rc = beaver_clear_failure(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_clear_failure(): returned error %d", rc);
			X_CLEAR_FAILURE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			X_CLEAR_FAILURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
	}
	int rotator_code = 0, shutter_code = 0;
	if ((rc = beaver_get_failure_codes(device, &rotator_code, &shutter_code)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_failure_codes(): returned error %d", rc);
	} else {
		if (rotator_code != PRIVATE_DATA->rotator_failure_code || shutter_code != PRIVATE_DATA->shutter_failure_code) {
			beaver_get_failure_messages(device, X_FAILURE_MESSAGE_ROTATOR_ITEM->text.value, X_FAILURE_MESSAGE_SHUTTER_ITEM->text.value);
			if (rotator_code != 0 || shutter_code != 0) {
				X_FAILURE_MESSAGE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_FAILURE_MESSAGE_PROPERTY, "Rotator or Shutter failure detected, check X_FAILURE_MESSAGES property");
			} else {
				X_FAILURE_MESSAGE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
			}
			PRIVATE_DATA->rotator_failure_code = rotator_code;
			PRIVATE_DATA->shutter_failure_code = shutter_code;
		}
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
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			} else {
				DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			}
		}
	}

	indigo_reschedule_timer(device, 1, &(PRIVATE_DATA->dome_timer));
}


static indigo_result beaver_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_SHUTTER_CALIBRATE_PROPERTY, property))
			indigo_define_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
		if (indigo_property_match(X_ROTATOR_CALIBRATE_PROPERTY, property))
			indigo_define_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
		if (indigo_property_match(X_FAILURE_MESSAGE_PROPERTY, property))
			indigo_define_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
		if (indigo_property_match(X_CLEAR_FAILURE_PROPERTY, property))
			indigo_define_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
		if (indigo_property_match(X_CONDITIONS_SAFETY_PROPERTY, property))
			indigo_define_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);
	}
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->move_mutex, NULL);
		// -------------------------------------------------------------------------------- DOME_SPEED
		DOME_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DOME_STEPS_PROPERTY
		indigo_copy_value(DOME_STEPS_ITEM->label, "Relative move (°)");
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY->hidden = true;
		indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE);
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
		DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DOME_HOME
		DOME_HOME_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DOME_PARK_POSITION
		DOME_PARK_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_SHUTTER_CALIBRATE_PROPERTY
		X_SHUTTER_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SHUTTER_CALIBRATE_PROPERTY_NAME, X_MISC_GROUP, "Calibrate shutter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_SHUTTER_CALIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_SHUTTER_CALIBRATE_ITEM, X_SHUTTER_CALIBRATE_ITEM_NAME, "Calibrate", false);
		// -------------------------------------------------------------------------------- X_ROTATOR_CALIBRATE_PROPERTY
		X_ROTATOR_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_CALIBRATE_PROPERTY_NAME, X_MISC_GROUP, "Calibrate rotator", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_ROTATOR_CALIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_ROTATOR_CALIBRATE_ITEM, X_ROTATOR_CALIBRATE_ITEM_NAME, "Calibrate", false);
		// -------------------------------------------------------------------------------- X_FAILURE_MESSAGE_PROPERTY
		X_FAILURE_MESSAGE_PROPERTY = indigo_init_text_property(NULL, device->name, X_FAILURE_MESSAGE_PROPERTY_NAME, X_MISC_GROUP, "Last failures", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (X_FAILURE_MESSAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(X_FAILURE_MESSAGE_ROTATOR_ITEM, X_FAILURE_MESSAGE_ROTATOR_ITEM_NAME, "Rotator message", "");
		indigo_init_text_item(X_FAILURE_MESSAGE_SHUTTER_ITEM, X_FAILURE_MESSAGE_SHUTTER_ITEM_NAME, "Shutter message", "");
		// -------------------------------------------------------------------------------- X_CLEAR_FAILURE_PROPERTY
		X_CLEAR_FAILURE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CLEAR_FAILURE_PROPERTY_NAME, X_MISC_GROUP, "Clear last failures", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CLEAR_FAILURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CLEAR_FAILURE_ITEM, X_CLEAR_FAILURE_ITEM_NAME, "Clear", false);
		// -------------------------------------------------------------------------------- X_CONDITIONS_SAFETY_PROPERTY
		X_CONDITIONS_SAFETY_PROPERTY = indigo_init_light_property(NULL, device->name, X_CONDITIONS_SAFETY_PROPERTY_NAME, X_MISC_GROUP, "Observing conditions safety", INDIGO_IDLE_STATE, 2);
		if (X_CONDITIONS_SAFETY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(X_SAFE_CW_ITEM, X_SAFE_CW_ITEM_NAME, "Safe by Cloud Wacher", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SAFE_HYDREON_ITEM, X_SAFE_HYDREON_ITEM_NAME, "Safe by Hydreon RG-x", INDIGO_IDLE_STATE);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
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
				int shutter_is_up = 0;
				if ((rc = beaver_get_shutterisup(device, &shutter_is_up)) != BD_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_shutterisup(): returned error %d", rc);
				}
				PRIVATE_DATA->shutter_is_up = (bool)shutter_is_up;

				if (!shutter_is_up) {
					indigo_send_message(device, "Shutter not detected");
					DOME_SHUTTER_PROPERTY->hidden = true;
					X_SHUTTER_CALIBRATE_PROPERTY->hidden = true;
				} else {
					DOME_SHUTTER_PROPERTY->hidden = false;
					X_SHUTTER_CALIBRATE_PROPERTY->hidden = false;
				}

				indigo_define_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
				indigo_define_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
				indigo_define_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
				indigo_define_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
				indigo_define_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);

				PRIVATE_DATA->prev_shutter_status = -1;
				PRIVATE_DATA->prev_dome_status = -1;

				if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(): returned error %d", rc);
				}
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
				PRIVATE_DATA->aborted = false;

				int atpark = false;
				if ((rc = beaver_is_parked(device, &atpark)) != BD_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_is_parked(): returned error %d", rc);
				}
				if (atpark) {
					indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
				} else {
					indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
				}
				DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_requested = false;
				indigo_update_property(device, DOME_PARK_PROPERTY, NULL);

				float park_pos;
				if ((rc = beaver_get_park(device, &park_pos)) != BD_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_park(%d): returned error %d", PRIVATE_DATA->handle, rc);
				} else {
					DOME_PARK_POSITION_AZ_ITEM->number.target = DOME_PARK_POSITION_AZ_ITEM->number.value = park_pos;
					DOME_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
				}

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				device->is_connected = true;

				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected = %d", PRIVATE_DATA->handle);
				indigo_set_timer(device, 0.5, dome_timer_callback, &PRIVATE_DATA->dome_timer);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_timer);

			indigo_delete_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
			indigo_delete_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
			indigo_delete_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
			indigo_delete_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
			indigo_delete_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);

			beaver_close(device);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected = %d", PRIVATE_DATA->handle);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}


static void dome_steps_callback(indigo_device *device) {
	beaver_rc_t rc;

	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked, please unpark");
		return;
	}

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
	}

	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);

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
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed");
		pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
		return;
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
}


static void dome_horizontal_coordinates_callback(indigo_device *device) {
	beaver_rc_t rc;

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		if ((rc = beaver_get_azimuth(device, &PRIVATE_DATA->current_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
		}
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked, please unpark");
		pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
		return;
	}

	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);

	PRIVATE_DATA->target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
	if (DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if ((rc = beaver_set_azimuth(device, PRIVATE_DATA->target_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_set_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Set azimuth failed");
			pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
			return;
		}
	} else {
		if ((rc = beaver_goto_azimuth(device, PRIVATE_DATA->target_position)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_azimuth(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Goto azimuth failed");
			pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
			return;
		}
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
}


static void dome_shutter_callback(indigo_device *device) {
	beaver_rc_t rc;

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	if (DOME_SHUTTER_OPENED_ITEM->sw.value) {
		rc = beaver_open_shutter(device);
	} else {
		rc = beaver_close_shutter(device);
	}
	if (rc != BD_SUCCESS) {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Shutter open/close failed");
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
		return;
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
}


static void dome_park_callback(indigo_device *device) {
	beaver_rc_t rc;

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	if (DOME_PARK_UNPARKED_ITEM->sw.value) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_requested = false;
		indigo_update_property(device, DOME_PARK_PROPERTY, "Dome unparked");
	} else if (DOME_PARK_PARKED_ITEM->sw.value) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, DOME_PARK_PROPERTY, "Dome parking...");
		if ((rc = beaver_goto_park(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_park(%d): returned error %d", PRIVATE_DATA->handle, rc);
		}
		PRIVATE_DATA->park_requested = true;
	} else {
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
}


static void dome_gohome_callback(indigo_device *device) {
	beaver_rc_t rc;

	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_HOME_ITEM->sw.value = false;
		DOME_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HOME_PROPERTY, "Dome is parked, please unpark");
		return;
	}

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	if (DOME_HOME_ITEM->sw.value) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_HOME_ITEM, false);

		DOME_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HOME_PROPERTY, "Dome going home...");
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);

		if ((rc = beaver_goto_home(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_goto_home(%d): returned error %d", PRIVATE_DATA->handle, rc);
		}

	} else {
		indigo_update_property(device, DOME_HOME_PROPERTY, NULL);
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
}


static void dome_calibrate_rotator_callback(indigo_device *device) {
	beaver_rc_t rc;

	if (DOME_PARK_PARKED_ITEM->sw.value) {
		X_ROTATOR_CALIBRATE_ITEM->sw.value = false;
		X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Dome is parked, please unpark");
		return;
	}

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	if (X_ROTATOR_CALIBRATE_ITEM->sw.value) {
		X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
		if ((rc = beaver_calibrate_rotator(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_calibrate_rotator(%d): returned error %d", PRIVATE_DATA->handle, rc);
			X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration falied");
		} else {
			indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Calibrating rotator...");
		}
	} else {
		indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
}


static void dome_calibrate_shutter_callback(indigo_device *device) {
	beaver_rc_t rc;

	pthread_mutex_lock(&PRIVATE_DATA->move_mutex);
	if (X_SHUTTER_CALIBRATE_ITEM->sw.value) {
		X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
		if ((rc = beaver_calibrate_shutter(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_calibrate_shutter(%d): returned error %d", PRIVATE_DATA->handle, rc);
			X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration falied");
		} else {
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Calibrating shutter...");
		}
	} else {
		indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
	}

	indigo_usleep(0.5*ONE_SECOND_DELAY);
	pthread_mutex_unlock(&PRIVATE_DATA->move_mutex);
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

		indigo_set_timer(device, 0, dome_steps_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
		if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK;
		}
		indigo_property_copy_values(DOME_HORIZONTAL_COORDINATES_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		indigo_set_timer(device, 0, dome_horizontal_coordinates_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_EQUATORIAL_COORDINATES
		indigo_property_copy_values(DOME_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (DOME_PARK_PARKED_ITEM->sw.value) {
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, "Dome is parked, please unpark");
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
			indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, "Abort failed");
			return INDIGO_OK;
		} else {
			PRIVATE_DATA->aborted = true;
		}

		if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}

		PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
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

		indigo_set_timer(device, 0, dome_shutter_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_PARK
		indigo_property_copy_values(DOME_PARK_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		indigo_set_timer(device, 0, dome_park_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_PARK_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_PARK_POSITION
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		beaver_rc_t rc;
		if ((rc = beaver_set_park(device)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_set_park(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_PARK_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, "Failed to set current position to park position");
			return INDIGO_OK;
		}

		float park_pos;
		if ((rc = beaver_get_park(device, &park_pos)) != BD_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "beaver_get_park(%d): returned error %d", PRIVATE_DATA->handle, rc);
			DOME_PARK_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, "Failed to set current position to park position");
			return INDIGO_OK;
		}
		DOME_PARK_POSITION_AZ_ITEM->number.target = DOME_PARK_POSITION_AZ_ITEM->number.value = park_pos;
		DOME_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_HOME
		indigo_property_copy_values(DOME_HOME_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		indigo_set_timer(device, 0, dome_gohome_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_CLEAR_FAILURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CLEAR_FAILURE
		indigo_property_copy_values(X_CLEAR_FAILURE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (X_CLEAR_FAILURE_ITEM->sw.value) {
			X_CLEAR_FAILURE_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			X_CLEAR_FAILURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);

		return INDIGO_OK;
	} else if (indigo_property_match(X_ROTATOR_CALIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_ROTATOR_CALIBRATE
		indigo_property_copy_values(X_ROTATOR_CALIBRATE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		indigo_set_timer(device, 0, dome_calibrate_rotator_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_SHUTTER_CALIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SHUTTER_CALIBRATE
		indigo_property_copy_values(X_SHUTTER_CALIBRATE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		indigo_set_timer(device, 0, dome_calibrate_shutter_callback, NULL);
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

	indigo_release_property(X_SHUTTER_CALIBRATE_PROPERTY);
	indigo_release_property(X_ROTATOR_CALIBRATE_PROPERTY);
	indigo_release_property(X_FAILURE_MESSAGE_PROPERTY);
	indigo_release_property(X_CLEAR_FAILURE_PROPERTY);
	indigo_release_property(X_CONDITIONS_SAFETY_PROPERTY);

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
