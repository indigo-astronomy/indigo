// Copyright (C) 2020 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO Lunatico Dragonfly driver
 \file dragonfly_shared.c
 */

#define DEVICE_CONNECTED_MASK            0x80
#define L_DEVICE_INDEX_MASK              0x0F

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)

#define is_connected(dev)                ((dev) && (dev)->gp_bits & DEVICE_CONNECTED_MASK)
#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define get_locical_device_index(dev)              ((dev)->gp_bits & L_DEVICE_INDEX_MASK)
#define set_logical_device_index(dev, index)       ((dev)->gp_bits = ((dev)->gp_bits & ~L_DEVICE_INDEX_MASK) | (L_DEVICE_INDEX_MASK & index))

#define LUNATICO_CMD_LEN 100

/* Linatico Astronomia device Commands ======================================================================== */

static bool lunatico_command(indigo_device *device, const char *command, char *response, int max, int sleep) {
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
			if (PRIVATE_DATA->udp) {
				result = read(PRIVATE_DATA->handle, response, LUNATICO_CMD_LEN);
				if (result < 1) {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				index = (int)result;
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


static bool lunatico_get_info(indigo_device *device, char *board, char *firmware) {
	if (!board || !firmware) return false;

	//const char *operative[3] = { "", "Bootloader", "Error" };
	const char *models[6] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet" };
	int fwmaj, fwmin, model, oper, data;
	char response[LUNATICO_CMD_LEN]={0};
	if (lunatico_command(device, "!seletek version#", response, sizeof(response), 0)) {
		// !seletek version:2510#
		int parsed = sscanf(response, "!seletek version:%d#", &data);
		if (parsed != 1) return false;
		oper = data / 10000;		// 0 normal, 1 bootloader
		model = (data / 1000) % 10;	// 1 seletek, etc.
		fwmaj = (data / 100) % 10;
		fwmin = (data % 100);
		if (oper >= 2) oper = 2;
		if (model > 5) model = 0;
		sprintf(board, "%s", models[model]);
		sprintf(firmware, "%d.%d", fwmaj, fwmin);

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "!seletek version# -> %s = %s %s", response, board, firmware);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool lunatico_command_get_result(indigo_device *device, const char *command, int32_t *result) {
	if (!result) return false;

	char response[LUNATICO_CMD_LEN]={0};
	char response_prefix[LUNATICO_CMD_LEN];
	char format[LUNATICO_CMD_LEN];

	if (lunatico_command(device, command, response, sizeof(response), 0)) {
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static bool lunatico_keep_alive(indigo_device *device) {
	int res;
	if (!lunatico_command_get_result(device, "!seletek echo#", &res)) return false;
	if (res < 0) return false;
	return true;
}

#pragma GCC diagnostic pop

static bool lunatico_authenticate(indigo_device *device, char* password, int *access) {
	if (!access) return false;

	char command[LUNATICO_CMD_LEN];
	int value;

	if (password) {
		snprintf(command, LUNATICO_CMD_LEN, "!aux earnaccess %s#", password);
	} else {
		snprintf(command, LUNATICO_CMD_LEN, "!aux earnaccess#");
	}
	if (!lunatico_command_get_result(device, command, &value)) return false;
	if (value >= 0) {
		*access = value;
		return true;
	}
	return false;
}


//static bool lunatico_analog_read_sensor(indigo_device *device, int sensor, int *sensor_value) {
//	if (!sensor_value) return false;
//
//	char command[LUNATICO_CMD_LEN];
//	int value;
//
//	if (sensor < 0 || sensor > 8) return false;
//
//	snprintf(command, LUNATICO_CMD_LEN, "!relio snanrd 0 %d#", sensor);
//	if (!lunatico_command_get_result(device, command, &value)) return false;
//	if (value >= 0) {
//		*sensor_value = value;
//		return true;
//	}
//	return false;
//}


static bool lunatico_analog_read_sensors(indigo_device *device, int *sensors) {
	 char response[LUNATICO_CMD_LEN]={0};
	 char format[LUNATICO_CMD_LEN];
	 int isensors[8];

	 if (lunatico_command(device, "!relio snanrd 0 0 7#", response, sizeof(response), 0)) {
		sprintf(format, "!relio snanrd 0 0 7:%%d,%%d,%%d,%%d,%%d,%%d,%%d,%%d#");
		int parsed = sscanf(response, format, isensors, isensors+1, isensors+2, isensors+3, isensors+4, isensors+5, isensors+6, isensors+7);
		if (parsed != 8) return false;
		sensors[0] = isensors[0];
		sensors[1] = isensors[1];
		sensors[2] = isensors[2];
		sensors[3] = isensors[3];
		sensors[4] = isensors[4];
		sensors[5] = isensors[5];
		sensors[6] = isensors[6];
		sensors[7] = isensors[7];
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "-> %s = %d %d %d %d %d %d %d %d", response, sensors[0], sensors[1], sensors[2], sensors[3], sensors[4], sensors[5], sensors[6], sensors[7]);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


//static bool lunatico_digital_read_sensor(indigo_device *device, int sensor, bool *sensor_value) {
//	if (!sensor_value) return false;
//
//	char command[LUNATICO_CMD_LEN];
//	int value;
//
//	if (sensor < 0 || sensor > 8) return false;
//
//	snprintf(command, LUNATICO_CMD_LEN, "!relio sndgrd 0 %d#", sensor);
//	if (!lunatico_command_get_result(device, command, &value)) return false;
//	if (value >= 0) {
//		*sensor_value = (bool)value;
//		return true;
//	}
//	return false;
//}


//static bool lunatico_read_relay(indigo_device *device, int relay, bool *enabled) {
//	if (!enabled) return false;
//
//	char command[LUNATICO_CMD_LEN];
//	int value;
//
//	if (relay < 0 || relay > 8) return false;
//
//	snprintf(command, LUNATICO_CMD_LEN, "!relio rldgrd 0 %d#", relay);
//	if (!lunatico_command_get_result(device, command, &value)) return false;
//	if (value >= 0) {
//		*enabled = (bool)value;
//		return true;
//	}
//	return false;
//}


static bool lunatico_read_relays(indigo_device *device, bool *relays) {
	char response[LUNATICO_CMD_LEN]={0};
	char format[LUNATICO_CMD_LEN];
	int irelays[8];

	if (lunatico_command(device, "!relio rldgrd 0 0 7#", response, sizeof(response), 0)) {
		sprintf(format, "!relio rldgrd 0 0 7:%%d,%%d,%%d,%%d,%%d,%%d,%%d,%%d#");
		int parsed = sscanf(response, format, irelays, irelays+1, irelays+2, irelays+3, irelays+4, irelays+5, irelays+6, irelays+7);
		if (parsed != 8) return false;
		relays[0] = (bool)irelays[0];
		relays[1] = (bool)irelays[1];
		relays[2] = (bool)irelays[2];
		relays[3] = (bool)irelays[3];
		relays[4] = (bool)irelays[4];
		relays[5] = (bool)irelays[5];
		relays[6] = (bool)irelays[6];
		relays[7] = (bool)irelays[7];
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "-> %s = %d %d %d %d %d %d %d %d", response, relays[0], relays[1], relays[2], relays[3], relays[4], relays[5], relays[6], relays[7]);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool lunatico_set_relay(indigo_device *device, int relay, bool enable) {
	char command[LUNATICO_CMD_LEN];
	int res;
	if (relay < 0 || relay > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio rlset 0 %d %d#", relay, enable ? 1 : 0);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res < 0) return false;
	return true;
}


static bool lunatico_pulse_relay(indigo_device *device, int relay, uint32_t lenms) {
	char command[LUNATICO_CMD_LEN];
	int res;
	if (relay < 0 || relay > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio rlpulse 0 %d %d#", relay, lenms);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res < 0) return false;
	return true;
}


// --------------------------------------------------------------------------------- Common stuff
static bool lunatico_open(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OPEN REQUESTED: %d -> %d, count_open = %d", PRIVATE_DATA->handle, DEVICE_CONNECTED, PRIVATE_DATA->count_open);
	if (DEVICE_CONNECTED) return false;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			return false;
		}

		char url[INDIGO_VALUE_SIZE];
		if (strstr(DEVICE_PORT_ITEM->text.value, "://")) {
			indigo_copy_value(url, DEVICE_PORT_ITEM->text.value);
		} else {
			snprintf(url, INDIGO_VALUE_SIZE, "udp://%s", DEVICE_PORT_ITEM->text.value);
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening network device on host: %s", DEVICE_PORT_ITEM->text.value);
		indigo_network_protocol proto = INDIGO_PROTOCOL_UDP;
		PRIVATE_DATA->handle = indigo_open_network_device(url, 10000, &proto);
		PRIVATE_DATA->udp = true;

		if (PRIVATE_DATA->handle < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
			indigo_global_unlock(device);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	set_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}


static bool lunatico_authenticate2(indigo_device *device, char *password) {
	bool result = false;
	if (password && password[0] == 0) return false;

	int access = 0;
	result = lunatico_authenticate(device, password, &access);
	if (access == 1) {
		indigo_send_message(device, "Earned access level: %d (Read only)", access);
	} else if (access == 2) {
		indigo_send_message(device, "Earned access level: %d (Read / Write)", access);
	} else if (access == 3) {
		indigo_send_message(device, "Earned access level: %d (Full access)", access);
	} else {
		indigo_send_message(device, "Earned access level: %d (Unknown)", access);
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Earned access: %d", access);
	return result;
 }


static void lunatico_close(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSE REQUESTED: %d -> %d, count_open = %d", PRIVATE_DATA->handle, DEVICE_CONNECTED, PRIVATE_DATA->count_open);
	if (!DEVICE_CONNECTED) return;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		close(PRIVATE_DATA->handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d)", PRIVATE_DATA->handle);
		indigo_global_unlock(device);
		PRIVATE_DATA->handle = 0;
	}
	clear_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
}
