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

/** INDIGO Lunatico Armadillo, Platypus etc. focuser driver
 \file lunatico_shared.c
 */

#define FOCUSER_LUNATICO_NAME    "Focuser Lunatico"
#define ROTATOR_LUNATICO_NAME    "Rotator Lunatico"
#define AUX_LUNATICO_NAME        "Powerbox Lunatico"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <indigo/indigo_driver_xml.h>

#include <indigo/indigo_io.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_aux_driver.h>

#define DEFAULT_BAUDRATE            "115200"

#define MAX_PORTS  3
#define MAX_DEVICES 4

#define DEVICE_CONNECTED_MASK            0x80
#define PORT_INDEX_MASK                  0x0F

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)

#define is_connected(dev)                ((dev) && (dev)->gp_bits & DEVICE_CONNECTED_MASK)
#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define get_port_index(dev)              ((dev)->gp_bits & PORT_INDEX_MASK)
#define set_port_index(dev, index)       ((dev)->gp_bits = ((dev)->gp_bits & ~PORT_INDEX_MASK) | (PORT_INDEX_MASK & index))

#define device_exists(device_index, port_index) (device_data[device_index].port[port_index] != NULL)

#define PRIVATE_DATA                    ((lunatico_private_data *)device->private_data)
#define PORT_DATA                       (PRIVATE_DATA->port_data[get_port_index(device)])

#define LA_MODEL_PROPERTY               (PORT_DATA.model_property)
#define LA_MODEL_LIMPET_ITEM            (LA_MODEL_PROPERTY->items+0)
#define LA_MODEL_ARMADILLO_ITEM         (LA_MODEL_PROPERTY->items+1)
#define LA_MODEL_PLATYPUS_ITEM          (LA_MODEL_PROPERTY->items+2)

#define LA_MODEL_PROPERTY_NAME          "LUNATICO_MODEL"
#define LA_MODEL_LIMPET_ITEM_NAME       "LIMPET"
#define LA_MODEL_ARMADILLO_ITEM_NAME    "ARMADILLO"
#define LA_MODEL_PLATYPUS_ITEM_NAME     "PLATYPUS"

#define LA_PORT_EXP_CONFIG_PROPERTY    (PORT_DATA.port_exp_config)
#define LA_PORT_EXP_FOCUSER_ITEM       (LA_PORT_EXP_CONFIG_PROPERTY->items+0)
#define LA_PORT_EXP_ROTATOR_ITEM       (LA_PORT_EXP_CONFIG_PROPERTY->items+1)
#define LA_PORT_EXP_AUX_POWERBOX_ITEM  (LA_PORT_EXP_CONFIG_PROPERTY->items+2)

#define LA_PORT_THIRD_CONFIG_PROPERTY    (PORT_DATA.port_third_config)
#define LA_PORT_THIRD_FOCUSER_ITEM       (LA_PORT_THIRD_CONFIG_PROPERTY->items+0)
#define LA_PORT_THIRD_ROTATOR_ITEM       (LA_PORT_THIRD_CONFIG_PROPERTY->items+1)
#define LA_PORT_THIRD_AUX_POWERBOX_ITEM  (LA_PORT_THIRD_CONFIG_PROPERTY->items+2)

#define LA_PORT_EXP_CONFIG_PROPERTY_NAME        "LUNATICO_PORT_EXP_CONFIG"
#define LA_PORT_THIRD_CONFIG_PROPERTY_NAME      "LUNATICO_PORT_THIRD_CONFIG"
#define LA_PORT_CONFIG_FOCUSER_ITEM_NAME        "FOCUSER"
#define LA_PORT_CONFIG_ROTATOR_ITEM_NAME        "ROTATOR"
#define LA_PORT_CONFIG_AUX_POWERBOX_ITEM_NAME   "AUX_POWERBOX"


#define LA_STEP_MODE_PROPERTY          (PORT_DATA.step_mode_property)
#define LA_STEP_MODE_FULL_ITEM         (LA_STEP_MODE_PROPERTY->items+0)
#define LA_STEP_MODE_HALF_ITEM         (LA_STEP_MODE_PROPERTY->items+1)

#define LA_STEP_MODE_PROPERTY_NAME     "LA_STEP_MODE"
#define LA_STEP_MODE_FULL_ITEM_NAME    "FULL"
#define LA_STEP_MODE_HALF_ITEM_NAME    "HALF"

#define LA_POWER_CONTROL_PROPERTY         (PORT_DATA.current_control_property)
#define LA_POWER_CONTROL_MOVE_ITEM        (LA_POWER_CONTROL_PROPERTY->items+0)
#define LA_POWER_CONTROL_STOP_ITEM        (LA_POWER_CONTROL_PROPERTY->items+1)

#define LA_POWER_CONTROL_PROPERTY_NAME    "LA_POWER_CONTROL"
#define LA_POWER_CONTROL_MOVE_ITEM_NAME   "MOVE_POWER"
#define LA_POWER_CONTROL_STOP_ITEM_NAME   "STOP_POWER"

#define LA_TEMPERATURE_SENSOR_PROPERTY      (PORT_DATA.temperature_sensor_property)
#define LA_TEMPERATURE_SENSOR_INTERNAL_ITEM (LA_TEMPERATURE_SENSOR_PROPERTY->items+0)
#define LA_TEMPERATURE_SENSOR_EXTERNAL_ITEM (LA_TEMPERATURE_SENSOR_PROPERTY->items+1)

#define LA_TEMPERATURE_SENSOR_PROPERTY_NAME        "LA_TEMPERATURE_SENSOR"
#define LA_TEMPERATURE_SENSOR_INTERNAL_ITEM_NAME   "INTERNAL"
#define LA_TEMPERATURE_SENSOR_EXTERNAL_ITEM_NAME   "EXTERNAL"

#define LA_WIRING_PROPERTY          (PORT_DATA.wiring_property)
#define LA_WIRING_LUNATICO_ITEM     (LA_WIRING_PROPERTY->items+0)
#define LA_WIRING_MOONLITE_ITEM     (LA_WIRING_PROPERTY->items+1)

#define LA_WIRING_PROPERTY_NAME        "LA_MOTOR_WIRING"
#define LA_WIRING_LUNATICO_ITEM_NAME   "LUNATICO"
#define LA_WIRING_MOONLITE_ITEM_NAME   "MOONLITE"

#define LA_MOTOR_TYPE_PROPERTY         (PORT_DATA.motor_type_property)
#define LA_MOTOR_TYPE_UNIPOLAR_ITEM    (LA_MOTOR_TYPE_PROPERTY->items+0)
#define LA_MOTOR_TYPE_BIPOLAR_ITEM     (LA_MOTOR_TYPE_PROPERTY->items+1)
#define LA_MOTOR_TYPE_DC_ITEM          (LA_MOTOR_TYPE_PROPERTY->items+2)
#define LA_MOTOR_TYPE_STEP_DIR_ITEM    (LA_MOTOR_TYPE_PROPERTY->items+3)

#define LA_MOTOR_TYPE_PROPERTY_NAME        "LA_MOTOR_TYPE"
#define LA_MOTOR_TYPE_UNIPOLAR_ITEM_NAME   "UNIPOLAR"
#define LA_MOTOR_TYPE_BIPOLAR_ITEM_NAME    "BIPOLAR"
#define LA_MOTOR_TYPE_DC_ITEM_NAME         "DC"
#define LA_MOTOR_TYPE_STEP_DIR_ITEM_NAME   "STEP_DIR"

#define AUX_POWERBOX_GROUP	"Powerbox"

#define AUX_OUTLET_NAMES_PROPERTY      (PORT_DATA.outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_2_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_3_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_PROPERTY      (PORT_DATA.power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 3)

#define AUX_SENSORS_GROUP	"Sensors"

#define AUX_SENSOR_NAMES_PROPERTY      (PORT_DATA.sensor_names_property)
#define AUX_SENSOR_NAME_1_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_2_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)

#define AUX_GPIO_SENSORS_PROPERTY     (PORT_DATA.sensors_property)
#define AUX_GPIO_SENSOR_1_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_2_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_3_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_4_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 3)


typedef enum {
	TYPE_FOCUSER = 0,
	TYPE_ROTATOR = 1,
	TYPE_AUX     = 2
} device_type_t;


static const char *port_name[3] = { "Main", "Exp", "Third" };

typedef struct {
	int f_current_position,
	    f_target_position,
	    backlash,
	    temperature_sensor_index;
	device_type_t device_type;
	double r_target_position, r_current_position, prev_temp;
	indigo_timer *focuser_timer;
	indigo_timer *rotator_timer;
	indigo_timer *temperature_timer;
	indigo_timer *sensors_timer;
	indigo_property *step_mode_property,
	                *current_control_property,
	                *model_property,
	                *port_exp_config,
	                *port_third_config,
	                *temperature_sensor_property,
	                *wiring_property,
	                *motor_type_property,
	                *outlet_names_property,
	                *power_outlet_property,
	                *sensor_names_property,
	                *sensors_property;
} lunatico_port_data;

typedef struct {
	int handle;
	int count_open;
	bool udp;
	pthread_mutex_t port_mutex;
	lunatico_port_data port_data[MAX_PORTS];
} lunatico_private_data;

typedef struct {
	indigo_device *port[MAX_PORTS];
	lunatico_private_data *private_data;
} lunatico_device_data;

static void compensate_focus(indigo_device *device, double new_temp);

static lunatico_device_data device_data[MAX_DEVICES] = {0};

static void create_port_device(int device_index, int port_index, device_type_t type);
static void delete_port_device(int device_index, int port_index);

/* Linatico Astronomia device Commands ======================================================================== */

#define LUNATICO_CMD_LEN 100

typedef enum {
	MODEL_SELETEK = 1,
	MODEL_ARMADILLO = 2,
	MODEL_PLATYPUS = 3,
	MODEL_DRAGONFLY = 4,
	MODEL_LIMPET = 5
} lunatico_model_t;

typedef enum {
	STEP_MODE_FULL = 0,
	STEP_MODE_HALF = 1,
} step_mode_t;

typedef enum {
	MW_LUNATICO_NORMAL = 0,
	MW_LUNATICO_REVERSED = 1,
	MW_MOONLITE_NORMAL = 2,
	MW_MOONLITE_REVERSED = 3
} wiring_t;

typedef enum {
	MT_UNIPOLAR = 0,
	MT_BIPOLAR = 1,
	MT_DC = 2,
	MT_STEP_DIR = 3
} motor_types_t;

#define NO_TEMP_READING                (-25)


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


static bool lunatico_get_info(indigo_device *device, char *board, char *firmware) {
	if(!board || !firmware) return false;

	const char *models[6] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet" };
	int fwmaj, fwmin, model, oper, data;
	char response[LUNATICO_CMD_LEN]={0};
	if (lunatico_command(device, "!seletek version#", response, sizeof(response), 100)) {
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


static bool lunatico_check_port_existance(indigo_device *device, bool *exists) {
	if(!exists) return false;

	int model, oper, data;
	char response[LUNATICO_CMD_LEN]={0};
	if (lunatico_command(device, "!seletek version#", response, sizeof(response), 100)) {
		int parsed = sscanf(response, "!seletek version:%d#", &data);
		if (parsed != 1) return false;

		oper = data / 10000;
		if (oper == 2) return false;

		model = (data / 1000) % 10;	// 1 seletek, etc.
		if (model > 5) model = 0;

		/* if devce is "Seletek", "Armadillo" etc... */
		if (model == MODEL_SELETEK && get_port_index(device) < 2) *exists = true;
		else if (model == MODEL_ARMADILLO && get_port_index(device) < 2) *exists = true;
		else if (model == MODEL_PLATYPUS && get_port_index(device) < 3) *exists = true;
		else if (model == MODEL_LIMPET && get_port_index(device) < 1) *exists = true;
		else *exists = false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "!seletek version# -> %s, port exists = %d", response, *exists);
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

	if (lunatico_command(device, command, response, sizeof(response), 100)) {
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


static bool lunatico_stop(indigo_device *device) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step stop %d#", get_port_index(device));
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_sync_position(indigo_device *device, uint32_t position) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step setpos %d %d#", get_port_index(device), position);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_get_position(indigo_device *device, int32_t *pos) {
	char command[LUNATICO_CMD_LEN]={0};
	bool res;

	sprintf(command, "!step getpos %d#", get_port_index(device));
	res = lunatico_command_get_result(device, command, pos);
	if ((res == false) || (*pos < 0)) return false;
	else return true;
}


static bool lunatico_goto_position(indigo_device *device, uint32_t position, uint32_t backlash) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step goto %d %d %d#", get_port_index(device), position, backlash);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}

/* not used now... maybe use it for relative goto */
/*
static bool lunatico_goto_position_relative(indigo_device *device, uint32_t position) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step gopr %d %d#", get_port_index(device), position);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}
*/

static bool lunatico_is_moving(indigo_device *device, bool *is_moving) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step ismoving %d#", get_port_index(device));
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res < 0) return false;

	if (res == 0) *is_moving = false;
	else *is_moving = true;

	return true;
}

static bool lunatico_get_temperature(indigo_device *device, int sensor_index, double *temperature) {
	if (!temperature) return false;

	char command[LUNATICO_CMD_LEN];
	int value;
	double idC1 = 261;
	double idC2 = 250;
	double idF = 1.8;

	snprintf(command, LUNATICO_CMD_LEN, "!read temps %d#", sensor_index);
	if (!lunatico_command_get_result(device, command, &value)) return false;

	if (sensor_index != 0) { // not insternal
		idC1 = 192;
		idC2 = 0;
		idF = 1.7;
	}

	*temperature = (((value - idC1) * idF) - idC2) / 10;
	return true;
}


static bool lunatico_set_step(indigo_device *device, step_mode_t mode) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step halfstep %d %d#", get_port_index(device), mode ? 1 : 0);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_set_wiring(indigo_device *device, wiring_t wiring) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step wiremode %d %d#", get_port_index(device), wiring);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_set_motor_type(indigo_device *device, motor_types_t type) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step model %d %d#", get_port_index(device), type);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_set_move_power(indigo_device *device, double power_percent) {
	char command[LUNATICO_CMD_LEN];
	int res;

	int power = (int)(power_percent * 10.23);

	snprintf(command, LUNATICO_CMD_LEN, "!step movepow %d %d#", get_port_index(device), power);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_set_stop_power(indigo_device *device, double power_percent) {
	char command[LUNATICO_CMD_LEN];
	int res;

	int power = (int)(power_percent * 10.23);

	snprintf(command, LUNATICO_CMD_LEN, "!step stoppow %d %d#", get_port_index(device), power);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_set_limits(indigo_device *device, uint32_t min, uint32_t max) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step setswlimits %d %d %d#", get_port_index(device), min, max);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_delete_limits(indigo_device *device) {
	char command[LUNATICO_CMD_LEN];
	int res;

	snprintf(command, LUNATICO_CMD_LEN, "!step delswlimits %d#", get_port_index(device));
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_set_speed(indigo_device *device, double speed_khz) {
	char command[LUNATICO_CMD_LEN];
	int res;

	if (speed_khz <= 0.00001) return false;
	int speed_us = (int)(1000 / speed_khz);
	if ((speed_us < 50) || (speed_us > 500000 )) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Speed out of range %.3f", speed_khz);
		return false;
	}

	snprintf(command, LUNATICO_CMD_LEN, "!step speedrangeus %d %d %d#", get_port_index(device), speed_us, speed_us);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_enable_power_outlet(indigo_device *device, int pin, bool enable) {
	char command[LUNATICO_CMD_LEN];
	int res;
	if (pin < 1 || pin > 4) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!write dig %d %d %d#", get_port_index(device), pin, enable ? 1 : 0);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_read_sensor(indigo_device *device, int pin, int *sensor_value) {
	if (!sensor_value) return false;

	char command[LUNATICO_CMD_LEN];
	int value;

	if (pin < 5 || pin > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!read an %d %d#", get_port_index(device), pin);
	if (!lunatico_command_get_result(device, command, &value)) return false;
	if (value >= 0) {
		*sensor_value = value;
		return true;
	}
	return false;
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
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			return false;
		}
		char *name = DEVICE_PORT_ITEM->text.value;
		if (!indigo_is_device_url(name, "lunatico")) {
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

	bool exists = false;
	/* check if the current port exists */
	lunatico_check_port_existance(device, &exists);
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (!exists) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No responce or port does not exist on this hardware");
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_update_property(device, CONNECTION_PROPERTY, "No response or port does not exist on this hardware");
		if (--PRIVATE_DATA->count_open == 0) {
			close(PRIVATE_DATA->handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d)", PRIVATE_DATA->handle);
			indigo_global_unlock(device);
			PRIVATE_DATA->handle = 0;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}
	set_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
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


static void configure_ports(indigo_device *device) {
	device_type_t exp_device_type, third_device_type;

	if (LA_PORT_EXP_FOCUSER_ITEM->sw.value) {
		exp_device_type = TYPE_FOCUSER;
	} else if (LA_PORT_EXP_ROTATOR_ITEM->sw.value){
		exp_device_type = TYPE_ROTATOR;
	} else {
		exp_device_type = TYPE_AUX;
	}

	if (LA_PORT_THIRD_FOCUSER_ITEM->sw.value) {
		third_device_type = TYPE_FOCUSER;
	} else if (LA_PORT_THIRD_ROTATOR_ITEM->sw.value) {
		third_device_type = TYPE_ROTATOR;
	} else {
		third_device_type = TYPE_AUX;
	}

	if (LA_MODEL_PLATYPUS_ITEM->sw.value) {
		create_port_device(0, 1, exp_device_type);
		create_port_device(0, 2, third_device_type);
	} else if (LA_MODEL_ARMADILLO_ITEM->sw.value) {
		create_port_device(0, 1, exp_device_type);
		delete_port_device(0, 2);
	} else if (LA_MODEL_LIMPET_ITEM->sw.value) {
		delete_port_device(0, 1);
		delete_port_device(0, 2);
	}
}


static int lunatico_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------- LA_MODEL_PROPERTY
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_PORTS
	DEVICE_PORTS_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
	DEVICE_BAUDRATE_PROPERTY->hidden = false;
	indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE);
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 6;
	// --------------------------------------------------------------------------------
	LA_MODEL_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_MODEL_PROPERTY_NAME, "Configuration", "Device model", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	if (LA_MODEL_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_MODEL_LIMPET_ITEM, LA_MODEL_LIMPET_ITEM_NAME, "Limpet (1 port)", true);
	indigo_init_switch_item(LA_MODEL_ARMADILLO_ITEM, LA_MODEL_ARMADILLO_ITEM_NAME, "Seletek/Armadillo (2 ports)", false);
	indigo_init_switch_item(LA_MODEL_PLATYPUS_ITEM, LA_MODEL_PLATYPUS_ITEM_NAME, "Platypus (3 ports)", false);
	if (get_port_index(device) != 0) LA_MODEL_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------- LA_PORT_EXP_CONFIG_PROPERTY
	LA_PORT_EXP_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_PORT_EXP_CONFIG_PROPERTY_NAME, "Configuration", "Exp port", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	if (LA_PORT_EXP_CONFIG_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_PORT_EXP_FOCUSER_ITEM, LA_PORT_CONFIG_FOCUSER_ITEM_NAME, "Focuser", true);
	indigo_init_switch_item(LA_PORT_EXP_ROTATOR_ITEM, LA_PORT_CONFIG_ROTATOR_ITEM_NAME, "Rotator", false);
	indigo_init_switch_item(LA_PORT_EXP_AUX_POWERBOX_ITEM, LA_PORT_CONFIG_AUX_POWERBOX_ITEM_NAME, "Powerbox/GPIO", false);
	if (get_port_index(device) != 0) LA_PORT_EXP_CONFIG_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------- LA_PORT_THIRD_CONFIG_PROPERTY
	LA_PORT_THIRD_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_PORT_THIRD_CONFIG_PROPERTY_NAME, "Configuration", "Third port", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	if (LA_PORT_THIRD_CONFIG_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_PORT_THIRD_FOCUSER_ITEM, LA_PORT_CONFIG_FOCUSER_ITEM_NAME, "Focuser", true);
	indigo_init_switch_item(LA_PORT_THIRD_ROTATOR_ITEM, LA_PORT_CONFIG_ROTATOR_ITEM_NAME, "Rotator", false);
	indigo_init_switch_item(LA_PORT_THIRD_AUX_POWERBOX_ITEM, LA_PORT_CONFIG_AUX_POWERBOX_ITEM_NAME, "Powerbox/GPIO", false);
	if (get_port_index(device) != 0) LA_PORT_THIRD_CONFIG_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------- STEP_MODE_PROPERTY
	LA_STEP_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_STEP_MODE_PROPERTY_NAME, "Advanced", "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (LA_STEP_MODE_PROPERTY == NULL)
		return INDIGO_FAILED;
	LA_STEP_MODE_PROPERTY->hidden = false;
	indigo_init_switch_item(LA_STEP_MODE_FULL_ITEM, LA_STEP_MODE_FULL_ITEM_NAME, "Full step", true);
	indigo_init_switch_item(LA_STEP_MODE_HALF_ITEM, LA_STEP_MODE_HALF_ITEM_NAME, "1/2 step", false);
	if (PORT_DATA.device_type == TYPE_AUX) LA_STEP_MODE_PROPERTY->hidden = true;
	//--------------------------------------------------------------------------- CURRENT_CONTROL_PROPERTY
	LA_POWER_CONTROL_PROPERTY = indigo_init_number_property(NULL, device->name, LA_POWER_CONTROL_PROPERTY_NAME, "Advanced", "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (LA_POWER_CONTROL_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(LA_POWER_CONTROL_MOVE_ITEM, LA_POWER_CONTROL_MOVE_ITEM_NAME, "Move power (%)", 0, 100, 1, 100);
	indigo_init_number_item(LA_POWER_CONTROL_STOP_ITEM, LA_POWER_CONTROL_STOP_ITEM_NAME, "Stop power (%)", 0, 100, 1, 0);
	if (PORT_DATA.device_type == TYPE_AUX) LA_POWER_CONTROL_PROPERTY->hidden = true;
	//--------------------------------------------------------------------------- TEMPERATURE_SENSOR_PROPERTY
	LA_TEMPERATURE_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_TEMPERATURE_SENSOR_PROPERTY_NAME, "Advanced", "Temperature Sensor in use", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (LA_TEMPERATURE_SENSOR_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_TEMPERATURE_SENSOR_INTERNAL_ITEM, LA_TEMPERATURE_SENSOR_INTERNAL_ITEM_NAME, "Internal sensor", true);
	indigo_init_switch_item(LA_TEMPERATURE_SENSOR_EXTERNAL_ITEM, LA_TEMPERATURE_SENSOR_EXTERNAL_ITEM_NAME, "External Sensor", false);
	if (PORT_DATA.device_type != TYPE_FOCUSER) LA_TEMPERATURE_SENSOR_PROPERTY->hidden = true;
	//--------------------------------------------------------------------------- WIRING_PROPERTY
	LA_WIRING_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_WIRING_PROPERTY_NAME, "Advanced", "Motor wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (LA_WIRING_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_WIRING_LUNATICO_ITEM, LA_WIRING_LUNATICO_ITEM_NAME, "Lunatico", true);
	indigo_init_switch_item(LA_WIRING_MOONLITE_ITEM, LA_WIRING_MOONLITE_ITEM_NAME, "RF/Moonlite", false);
	if (PORT_DATA.device_type == TYPE_AUX) LA_WIRING_PROPERTY->hidden = true;
	//--------------------------------------------------------------------------- LA_MOTOR_TYPE_PROPERTY
	LA_MOTOR_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_MOTOR_TYPE_PROPERTY_NAME, "Advanced", "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
	if (LA_MOTOR_TYPE_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_MOTOR_TYPE_UNIPOLAR_ITEM, LA_MOTOR_TYPE_UNIPOLAR_ITEM_NAME, "Unipolar", true);
	indigo_init_switch_item(LA_MOTOR_TYPE_BIPOLAR_ITEM, LA_MOTOR_TYPE_BIPOLAR_ITEM_NAME, "Bipolar", false);
	indigo_init_switch_item(LA_MOTOR_TYPE_DC_ITEM, LA_MOTOR_TYPE_DC_ITEM_NAME, "DC", false);
	indigo_init_switch_item(LA_MOTOR_TYPE_STEP_DIR_ITEM, LA_MOTOR_TYPE_STEP_DIR_ITEM_NAME, "Step-dir", false);
	if (PORT_DATA.device_type == TYPE_AUX) LA_MOTOR_TYPE_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "DB9 Pin 1", "Power #1");
	indigo_init_text_item(AUX_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "DB9 Pin 2", "Power #2");
	indigo_init_text_item(AUX_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "DB9 Pin 3", "Power #3");
	indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "DB9 Pin 4", "Power #4");
	if (PORT_DATA.device_type != TYPE_AUX) AUX_OUTLET_NAMES_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- POWER OUTLETS
	AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
	if (AUX_POWER_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Power #1", false);
	indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power #2", false);
	indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Power #3", false);
	indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Power #4", false);
	if (PORT_DATA.device_type != TYPE_AUX) AUX_POWER_OUTLET_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- SENSOR_NAMES
	AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (AUX_SENSOR_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_SENSOR_NAME_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "DB9 Pin 6", "Sensor #1");
	indigo_init_text_item(AUX_SENSOR_NAME_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "DB9 Pin 7", "Sensor #2");
	indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "DB9 Pin 8", "Sensor #3");
	indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "DB9 Pin 9", "Sensor #4");
	if (PORT_DATA.device_type != TYPE_AUX) AUX_SENSOR_NAMES_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- GPIO_SENSORS
	AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "GPIO sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
	if (AUX_GPIO_SENSORS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_SENSOR_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor #1", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor #2", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
	if (PORT_DATA.device_type != TYPE_AUX) AUX_GPIO_SENSORS_PROPERTY->hidden = true;
	//---------------------------------------------------------------------------
	indigo_define_property(device, LA_MODEL_PROPERTY, NULL);
	indigo_define_property(device, LA_PORT_EXP_CONFIG_PROPERTY, NULL);
	indigo_define_property(device, LA_PORT_THIRD_CONFIG_PROPERTY, NULL);
	indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}


static indigo_result lunatico_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (DEVICE_CONNECTED) {
		if (indigo_property_match(LA_STEP_MODE_PROPERTY, property))
			indigo_define_property(device, LA_STEP_MODE_PROPERTY, NULL);
		if (indigo_property_match(LA_POWER_CONTROL_PROPERTY, property))
			indigo_define_property(device, LA_POWER_CONTROL_PROPERTY, NULL);
		if (indigo_property_match(LA_TEMPERATURE_SENSOR_PROPERTY, property))
			indigo_define_property(device, LA_TEMPERATURE_SENSOR_PROPERTY, NULL);
		if (indigo_property_match(LA_WIRING_PROPERTY, property))
			indigo_define_property(device, LA_WIRING_PROPERTY, NULL);
		if (indigo_property_match(LA_MOTOR_TYPE_PROPERTY, property))
			indigo_define_property(device, LA_MOTOR_TYPE_PROPERTY, NULL);
		if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_GPIO_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	if (indigo_property_match(LA_MODEL_PROPERTY, property))
		indigo_define_property(device, LA_MODEL_PROPERTY, NULL);
	if (indigo_property_match(LA_PORT_EXP_CONFIG_PROPERTY, property))
		indigo_define_property(device, LA_PORT_EXP_CONFIG_PROPERTY, NULL);
	if (indigo_property_match(LA_PORT_THIRD_CONFIG_PROPERTY, property))
		indigo_define_property(device, LA_PORT_THIRD_CONFIG_PROPERTY, NULL);
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}


static void lunatico_init_device(indigo_device *device) {
	char board[LUNATICO_CMD_LEN] = "N/A";
	char firmware[LUNATICO_CMD_LEN] = "N/A";
	if (lunatico_get_info(device, board, firmware)) {
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
		indigo_update_property(device, INFO_PROPERTY, NULL);
	}

	if (!lunatico_delete_limits(device)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_delete_limits(%d) failed", PRIVATE_DATA->handle);
	}

	if (!lunatico_set_move_power(device, LA_POWER_CONTROL_MOVE_ITEM->number.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_move_power(%d) failed", PRIVATE_DATA->handle);
	}
	if (!lunatico_set_stop_power(device, LA_POWER_CONTROL_STOP_ITEM->number.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_stop_power(%d) failed", PRIVATE_DATA->handle);
	}
	indigo_define_property(device, LA_POWER_CONTROL_PROPERTY, NULL);


	if (LA_TEMPERATURE_SENSOR_INTERNAL_ITEM->sw.value) {
		PORT_DATA.temperature_sensor_index = 0;
	} else {
		PORT_DATA.temperature_sensor_index = 1;
	}
	indigo_define_property(device, LA_TEMPERATURE_SENSOR_PROPERTY, NULL);

	indigo_define_property(device, LA_WIRING_PROPERTY, NULL);

	bool success = false;
	if (LA_MOTOR_TYPE_UNIPOLAR_ITEM->sw.value) {
			success = lunatico_set_motor_type(device, MT_UNIPOLAR);
	} else if (LA_MOTOR_TYPE_BIPOLAR_ITEM->sw.value) {
			success = lunatico_set_motor_type(device, MT_BIPOLAR);
	} else if (LA_MOTOR_TYPE_DC_ITEM->sw.value) {
			success = lunatico_set_motor_type(device, MT_DC);
	} else if (LA_MOTOR_TYPE_STEP_DIR_ITEM->sw.value) {
			success = lunatico_set_motor_type(device, MT_STEP_DIR);
	}
	if (!success) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_motor_type(%d) failed", PRIVATE_DATA->handle);
	}
	indigo_define_property(device, LA_MOTOR_TYPE_PROPERTY, NULL);

	step_mode_t mode = STEP_MODE_FULL;
	if(LA_STEP_MODE_HALF_ITEM->sw.value) {
		mode = STEP_MODE_HALF;
	}
	if (!lunatico_set_step(device, mode)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_step(%d, %d) failed", PRIVATE_DATA->handle, mode);
	}
	indigo_define_property(device, LA_STEP_MODE_PROPERTY, NULL);
}


static indigo_result lunatico_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(LA_STEP_MODE_PROPERTY);
	indigo_release_property(LA_POWER_CONTROL_PROPERTY);
	indigo_release_property(LA_TEMPERATURE_SENSOR_PROPERTY);
	indigo_release_property(LA_WIRING_PROPERTY);
	indigo_release_property(LA_MOTOR_TYPE_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, LA_MODEL_PROPERTY, NULL);
	indigo_release_property(LA_MODEL_PROPERTY);

	indigo_delete_property(device, LA_PORT_EXP_CONFIG_PROPERTY, NULL);
	indigo_release_property(LA_PORT_EXP_CONFIG_PROPERTY);

	indigo_delete_property(device, LA_PORT_THIRD_CONFIG_PROPERTY, NULL);
	indigo_release_property(LA_PORT_THIRD_CONFIG_PROPERTY);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	indigo_delete_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);
	return INDIGO_OK;
}


static void lunatico_save_properties(indigo_device *device) {
	indigo_save_property(device, NULL, LA_MODEL_PROPERTY);
	indigo_save_property(device, NULL, LA_PORT_EXP_CONFIG_PROPERTY);
	indigo_save_property(device, NULL, LA_PORT_THIRD_CONFIG_PROPERTY);
	indigo_save_property(device, NULL, LA_STEP_MODE_PROPERTY);
	indigo_save_property(device, NULL, LA_POWER_CONTROL_PROPERTY);
	indigo_save_property(device, NULL, LA_TEMPERATURE_SENSOR_PROPERTY);
	indigo_save_property(device, NULL, LA_WIRING_PROPERTY);
	indigo_save_property(device, NULL, LA_MOTOR_TYPE_PROPERTY);
	indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
	indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
}


static void lunatico_delete_properties(indigo_device *device) {
	indigo_delete_property(device, LA_STEP_MODE_PROPERTY, NULL);
	indigo_delete_property(device, LA_POWER_CONTROL_PROPERTY, NULL);
	indigo_delete_property(device, LA_TEMPERATURE_SENSOR_PROPERTY, NULL);
	indigo_delete_property(device, LA_WIRING_PROPERTY, NULL);
	indigo_delete_property(device, LA_MOTOR_TYPE_PROPERTY, NULL);
	indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
}


static indigo_result lunatico_common_update_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(LA_MODEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_MODEL
		indigo_property_copy_values(LA_MODEL_PROPERTY, property, false);
		LA_MODEL_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_ASYNC(configure_ports, device);
		indigo_update_property(device, LA_MODEL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_PORT_EXP_CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_PORT_EXP_CONFIG_PROPERTY
		indigo_property_copy_values(LA_PORT_EXP_CONFIG_PROPERTY, property, false);
		LA_PORT_EXP_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_ASYNC(configure_ports, device);
		indigo_update_property(device, LA_PORT_EXP_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_PORT_THIRD_CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_PORT_THIRD_CONFIG_PROPERTY
		indigo_property_copy_values(LA_PORT_THIRD_CONFIG_PROPERTY, property, false);
		LA_PORT_THIRD_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_ASYNC(configure_ports, device);
		indigo_update_property(device, LA_PORT_THIRD_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_STEP_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_STEP_MODE_PROPERTY
		indigo_property_copy_values(LA_STEP_MODE_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;

		LA_STEP_MODE_PROPERTY->state = INDIGO_OK_STATE;
		step_mode_t mode = STEP_MODE_FULL;
		if(LA_STEP_MODE_FULL_ITEM->sw.value) {
			mode = STEP_MODE_FULL;
		} else if(LA_STEP_MODE_HALF_ITEM->sw.value) {
			mode = STEP_MODE_HALF;
		}
		if (!lunatico_set_step(device, mode)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_step(%d, %d) failed", PRIVATE_DATA->handle, mode);
			LA_STEP_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, LA_STEP_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_POWER_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_POWER_CONTROL_PROPERTY
		indigo_property_copy_values(LA_POWER_CONTROL_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		LA_POWER_CONTROL_PROPERTY->state = INDIGO_OK_STATE;

		if (!lunatico_set_move_power(device, LA_POWER_CONTROL_MOVE_ITEM->number.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_move_power(%d) failed", PRIVATE_DATA->handle);
			LA_POWER_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, LA_POWER_CONTROL_PROPERTY, NULL);
			return INDIGO_OK;
		}
		if (!lunatico_set_stop_power(device, LA_POWER_CONTROL_STOP_ITEM->number.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_stop_power(%d) failed", PRIVATE_DATA->handle);
			LA_POWER_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, LA_POWER_CONTROL_PROPERTY, NULL);
			return INDIGO_OK;
		}

		indigo_update_property(device, LA_POWER_CONTROL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_TEMPERATURE_SENSOR_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_TEMPERATURE_SENSOR
		indigo_property_copy_values(LA_TEMPERATURE_SENSOR_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		LA_TEMPERATURE_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
		if (LA_TEMPERATURE_SENSOR_INTERNAL_ITEM->sw.value) {
			PORT_DATA.temperature_sensor_index = 0;
		} else {
			PORT_DATA.temperature_sensor_index = 1;
		}
		indigo_update_property(device, LA_TEMPERATURE_SENSOR_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_MOTOR_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_MOTOR_TYPE_PROPERTY
		bool success = true;
		indigo_property_copy_values(LA_MOTOR_TYPE_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		LA_MOTOR_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (LA_MOTOR_TYPE_UNIPOLAR_ITEM->sw.value) {
				success = lunatico_set_motor_type(device, MT_UNIPOLAR);
		} else if (LA_MOTOR_TYPE_BIPOLAR_ITEM->sw.value) {
				success = lunatico_set_motor_type(device, MT_BIPOLAR);
		} else if (LA_MOTOR_TYPE_DC_ITEM->sw.value) {
				success = lunatico_set_motor_type(device, MT_DC);
		} else if (LA_MOTOR_TYPE_STEP_DIR_ITEM->sw.value) {
				success = lunatico_set_motor_type(device, MT_STEP_DIR);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported Motor type");
			LA_MOTOR_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!success) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_motor_type() failed");
			LA_MOTOR_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, LA_MOTOR_TYPE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			lunatico_save_properties(device);
		}
	}
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------- INDIGO AUX Powerbox device implementation

static void sensors_timer_callback(indigo_device *device) {
	int sensor_value;
	bool success;

	AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_OK_STATE;

	/* NOTE: Pins are hrizontally flipped on the newer devices with female DB9 connectors.
	   We reverse them here, so that there is no issue for the users with these devices.
	*/
	if (!(success = lunatico_read_sensor(device, 8, &sensor_value))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_sensor(%d) failed", PRIVATE_DATA->handle);
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		AUX_GPIO_SENSOR_1_ITEM->number.value = (double)sensor_value;
	}

	if ((success) && (!(success = lunatico_read_sensor(device, 7, &sensor_value)))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_sensor(%d) failed", PRIVATE_DATA->handle);
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		AUX_GPIO_SENSOR_2_ITEM->number.value = (double)sensor_value;
	}

	if ((success) && (!(success = lunatico_read_sensor(device, 6, &sensor_value)))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_sensor(%d) failed", PRIVATE_DATA->handle);
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		AUX_GPIO_SENSOR_3_ITEM->number.value = (double)sensor_value;
	}

	if ((success) && (!(success = lunatico_read_sensor(device, 5, &sensor_value)))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_sensor(%d) failed", PRIVATE_DATA->handle);
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		AUX_GPIO_SENSOR_4_ITEM->number.value = (double)sensor_value;
	}
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	indigo_reschedule_timer(device, 3, &PORT_DATA.sensors_timer);
}


static bool set_power_outlets(indigo_device *device) {
	bool success = true;
	/* NOTE: Pins are hrizontally flipped on the newer devices with female DB9 connectors.
	   We reverse them here, so that there is no issue for the users with these devices.
	*/
	if (!lunatico_enable_power_outlet(device, 4, AUX_POWER_OUTLET_1_ITEM->sw.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_enable_power_outlet(%d) failed", PRIVATE_DATA->handle);
		success = false;
	}
	if (!lunatico_enable_power_outlet(device, 3, AUX_POWER_OUTLET_2_ITEM->sw.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_enable_power_outlet(%d) failed", PRIVATE_DATA->handle);
		success = false;
	}
	if (!lunatico_enable_power_outlet(device, 2, AUX_POWER_OUTLET_3_ITEM->sw.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_enable_power_outlet(%d) failed", PRIVATE_DATA->handle);
		success = false;
	}
	if (!lunatico_enable_power_outlet(device, 1, AUX_POWER_OUTLET_4_ITEM->sw.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_enable_power_outlet(%d) failed", PRIVATE_DATA->handle);
		success = false;
	}
	return success;
}


static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	lunatico_enumerate_properties(device, client, property);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO | INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		if (lunatico_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void handle_aux_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (lunatico_open(device)) {
				char board[LUNATICO_CMD_LEN] = "N/A";
				char firmware[LUNATICO_CMD_LEN] = "N/A";
				if (lunatico_get_info(device, board, firmware)) {
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
				}
				indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
				indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
				set_power_outlets(device);
				indigo_set_timer(device, 0, sensors_timer_callback, &PORT_DATA.sensors_timer);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else {
		if (DEVICE_CONNECTED) {
			indigo_cancel_timer_sync(device, &PORT_DATA.sensors_timer);
			lunatico_delete_properties(device);
			lunatico_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, handle_aux_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (DEVICE_CONNECTED) {
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;

		if (set_power_outlets(device) == true) {
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_SENSOR_NAMES
		indigo_property_copy_values(AUX_SENSOR_NAMES_PROPERTY, property, false);
		if (DEVICE_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_2_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_3_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_4_ITEM->text.value);
		AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	lunatico_common_update_property(device, client, property);
	return indigo_aux_change_property(device, client, property);
}


static indigo_result aux_detach(indigo_device *device) {
	if (DEVICE_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_aux_connect_property(device);
	}
	lunatico_detach(device);
	return indigo_aux_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO rotator device implementation
static int degrees_to_steps(double degrees, int steps_rev, double min) {
	double deg = degrees;
	while (deg >= (360 - min)) deg -= 360;
	deg -= min;
	int steps = (int)(deg * steps_rev / 360.0);
	while (steps < 0) steps += steps_rev;
	while (steps >= steps_rev) steps -= steps_rev;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s(): %.3f deg => %d steps (deg = %.3f, steps_rev = %d, min = %.3f)", __FUNCTION__, degrees, steps, deg, steps_rev, min);
	return steps;
}


static double steps_to_degrees(int steps, int steps_rev, double min) {
	if (steps_rev == 0) return 0;
	int st = steps;
	while (st >= steps_rev) st -= steps_rev;
	st += (int)(steps_rev * min / 360);
	double degrees = st * 360.0 / steps_rev;
	while (degrees < 0) degrees += 360;
	while (degrees >= 360) degrees -= 360;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s(): %d steps => %.3f deg (st = %d, steps_rev = %d, min = %.3f)", __FUNCTION__, steps, degrees, st, steps_rev, min);
	return degrees;
}


static void lunatico_sync_to_current(indigo_device *device) {
	double steps = degrees_to_steps(
		ROTATOR_POSITION_ITEM->number.value,
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
	);
	if (!lunatico_sync_position(device, steps)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_sync_position(%d) failed", PRIVATE_DATA->handle);
	}
}


static void rotator_timer_callback(indigo_device *device) {
	bool moving;
	int32_t position = 0;
	bool success = false;

	if (!(success = lunatico_is_moving(device, &moving))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_is_moving(%d) failed", PRIVATE_DATA->handle);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	if ((success) && (!(success = lunatico_get_position(device, &position)))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PORT_DATA.r_current_position = steps_to_degrees(
			position,
			ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
		);
	}

	if (success) {
		ROTATOR_POSITION_ITEM->number.value = PORT_DATA.r_current_position;
		if ((!moving) || (PORT_DATA.r_current_position == PORT_DATA.r_target_position)) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			PORT_DATA.focuser_timer = NULL;
		} else {
			indigo_reschedule_timer(device, 0.5, &(PORT_DATA.focuser_timer));
		}
	} else {
		PORT_DATA.focuser_timer = NULL;
	}
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
}


static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	lunatico_enumerate_properties(device, client, property);
	return indigo_rotator_enumerate_properties(device, NULL, NULL);
}


static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.min = 100;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.max = 100000;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value = ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.target = 3600;
		ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = -180;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = 180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		if (lunatico_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_rotator_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (lunatico_open(device)) {
				lunatico_init_device(device);

				int32_t position = 0;
				if (!lunatico_get_position(device, &position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
				}
				ROTATOR_POSITION_ITEM->number.value = steps_to_degrees(
					position,
					ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
					ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
				);

				lunatico_sync_to_current(device);

				if (!lunatico_set_speed(device, 0.1)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_speed(%d) failed", PRIVATE_DATA->handle);
				}

				bool success = false;
				if (LA_WIRING_LUNATICO_ITEM->sw.value) {
					if(ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
						success = lunatico_set_wiring(device, MW_LUNATICO_NORMAL);
					} else {
						success = lunatico_set_wiring(device, MW_LUNATICO_REVERSED);
					}
				} else if (LA_WIRING_MOONLITE_ITEM->sw.value) {
					if (ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
						success = lunatico_set_wiring(device, MW_MOONLITE_NORMAL);
					} else {
						success = lunatico_set_wiring(device, MW_MOONLITE_REVERSED);
					}
				}
				if (!success) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_wiring(%d) failed", PRIVATE_DATA->handle);
				}

				int min_steps = degrees_to_steps(
					ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value,
					ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
					ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
				);
				int max_steps = degrees_to_steps(
					ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value,
					ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
					ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
				);
				if (max_steps == min_steps) {
					success = lunatico_delete_limits(device);
				} else {
					success = lunatico_set_limits(device, min_steps, max_steps);
				}
				if (!success) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits(%d) failed", PRIVATE_DATA->handle);
				}

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

				indigo_set_timer(device, 0.1, rotator_timer_callback, &PORT_DATA.rotator_timer);
			}
		}
	} else {
		if (DEVICE_CONNECTED) {
			indigo_cancel_timer_sync(device, &PORT_DATA.rotator_timer);
			lunatico_delete_properties(device);
			lunatico_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, handle_rotator_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		double current_position = ROTATOR_POSITION_ITEM->number.value;
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);

		int min_steps = degrees_to_steps(
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value,
			ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
		);
		int max_steps = degrees_to_steps(
			ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value,
			ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
		);
		int steps_position = degrees_to_steps(
			ROTATOR_POSITION_ITEM->number.target,
			ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
		);

		/* Make sure we do not go in the forbidden area */
		if ((max_steps != min_steps) && (steps_position > max_steps || steps_position < min_steps)) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			ROTATOR_POSITION_ITEM->number.value = current_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Requested position is not in the limits.");
			return INDIGO_OK;
		}

		/* We do not want to go in the forbidden area */
		if (ROTATOR_POSITION_ITEM->number.target == PORT_DATA.r_current_position) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		} else { /* GOTO position */
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			PORT_DATA.r_target_position = ROTATOR_POSITION_ITEM->number.target;
			ROTATOR_POSITION_ITEM->number.value = PORT_DATA.r_current_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				if (!lunatico_goto_position(device, steps_position, (uint32_t)ROTATOR_BACKLASH_ITEM->number.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_goto_position(%d, %d, %d) failed", PRIVATE_DATA->handle, PORT_DATA.r_target_position, (uint32_t)ROTATOR_BACKLASH_ITEM->number.value);
					ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				indigo_set_timer(device, 0.5, rotator_timer_callback, &PORT_DATA.focuser_timer);
			} else { /* RESET CURRENT POSITION */
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				if(!lunatico_sync_position(device, steps_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_sync_position(%d, %d) failed", PRIVATE_DATA->handle, PORT_DATA.r_target_position);
					ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				if (!lunatico_get_position(device, &steps_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
					ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					ROTATOR_POSITION_ITEM->number.value =
					PORT_DATA.r_current_position = steps_to_degrees(
						steps_position,
						ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
						ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
					);
				}
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PORT_DATA.rotator_timer);

		if (!lunatico_stop(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_stop(%d) failed", PRIVATE_DATA->handle);
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		int32_t position = 0;
		if (!lunatico_get_position(device, &position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PORT_DATA.r_current_position =
				steps_to_degrees(
					position,
					ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
					ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
				);
		}
		ROTATOR_POSITION_ITEM->number.value = PORT_DATA.r_current_position;
		ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(ROTATOR_DIRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_DIRECTION_PROPERTY
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_DIRECTION_PROPERTY, property, false);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		bool success = true;
		if (LA_WIRING_LUNATICO_ITEM->sw.value) {
			if(ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_LUNATICO_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_LUNATICO_REVERSED);
			}
		} else if (LA_WIRING_MOONLITE_ITEM->sw.value) {
			if (ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_MOONLITE_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_MOONLITE_REVERSED);
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported Motor wiring");
			ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!success) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_wiring() failed");
			ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_WIRING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_WIRING_PROPERTY
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(LA_WIRING_PROPERTY, property, false);
		LA_WIRING_PROPERTY->state = INDIGO_OK_STATE;
		bool success = true;
		if (LA_WIRING_LUNATICO_ITEM->sw.value) {
			if(ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_LUNATICO_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_LUNATICO_REVERSED);
			}
		} else if (LA_WIRING_MOONLITE_ITEM->sw.value) {
			if (ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_MOONLITE_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_MOONLITE_REVERSED);
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported Motor wiring");
			LA_WIRING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!success) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_wiring() failed");
			LA_WIRING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, LA_WIRING_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- ROTATOR_LIMITS_PROPERTY
	} else if (indigo_property_match(ROTATOR_LIMITS_PROPERTY, property)) {
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_LIMITS_PROPERTY, property, false);
		ROTATOR_LIMITS_PROPERTY->state = INDIGO_OK_STATE;

		bool success;
		int min_steps = degrees_to_steps(
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value,
			ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
		);
		int max_steps = degrees_to_steps(
			ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value,
			ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value,
			ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value
		);
		if (max_steps == min_steps) {
			success = lunatico_delete_limits(device);
		} else {
			success = lunatico_set_limits(device, min_steps, max_steps);
		}
		if (!success) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits(%d) failed", PRIVATE_DATA->handle);
			ROTATOR_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		lunatico_sync_to_current(device);

		indigo_update_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ROTATOR_STEPS_PER_REVOLUTION
	} else if (indigo_property_match(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->state = INDIGO_OK_STATE;

		lunatico_sync_to_current(device);

		indigo_update_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	lunatico_common_update_property(device, client, property);
	return indigo_rotator_change_property(device, client, property);
}


static indigo_result rotator_detach(indigo_device *device) {
	if (DEVICE_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_rotator_connect_property(device);
	}
	lunatico_detach(device);
	return indigo_rotator_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	bool moving;
	int32_t position = 0;
	bool success = false;

	if (!(success = lunatico_is_moving(device, &moving))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_is_moving(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	if ((success) && (!(success = lunatico_get_position(device, &position)))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PORT_DATA.f_current_position = (double)position;
	}

	if (success) {
		FOCUSER_POSITION_ITEM->number.value = PORT_DATA.f_current_position;
		if ((!moving) || (PORT_DATA.f_current_position == PORT_DATA.f_target_position)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			PORT_DATA.focuser_timer = NULL;
		} else {
			indigo_reschedule_timer(device, 0.5, &(PORT_DATA.focuser_timer));
		}
	} else {
		PORT_DATA.focuser_timer = NULL;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static void temperature_timer_callback(indigo_device *device) {
	double temp;
	static bool has_sensor = true;

	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	if (!lunatico_get_temperature(device, PORT_DATA.temperature_sensor_index, &temp)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_temperature(%d) -> %f failed", PRIVATE_DATA->handle, temp);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_TEMPERATURE_ITEM->number.value = temp;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lunatico_get_temperature(%d) -> %f succeeded", PRIVATE_DATA->handle, FOCUSER_TEMPERATURE_ITEM->number.value);
	}

	if (FOCUSER_TEMPERATURE_ITEM->number.value <= NO_TEMP_READING) { /* -127 is returned when the sensor is not connected */
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (has_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected.");
			has_sensor = false;
		}
	} else {
		has_sensor = true;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		compensate_focus(device, temp);
	} else {
		/* reset temp so that the compensation starts when auto mode is selected */
		PORT_DATA.prev_temp = NO_TEMP_READING;
	}

	indigo_reschedule_timer(device, 3, &(PORT_DATA.temperature_timer));
}


static void compensate_focus(indigo_device *device, double new_temp) {
	int compensation;
	double temp_difference = new_temp - PORT_DATA.prev_temp;

	/* we do not have previous temperature reading */
	if (PORT_DATA.prev_temp <= NO_TEMP_READING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PORT_DATA.prev_temp = %f", PORT_DATA.prev_temp);
		PORT_DATA.prev_temp = new_temp;
		return;
	}

	/* we do not have current temperature reading or focuser is moving */
	if ((new_temp <= NO_TEMP_READING) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}

	/* temperature difference if more than 1 degree so compensation needed */
	if ((fabs(temp_difference) >= 1.0) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.1f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %f", temp_difference);
		return;
	}

	PORT_DATA.f_target_position = PORT_DATA.f_current_position - compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PORT_DATA.f_current_position = %d, PORT_DATA.f_target_position = %d", PORT_DATA.f_current_position, PORT_DATA.f_target_position);

	int32_t current_position = 0;
	if (!lunatico_get_position(device, &current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
	}
	PORT_DATA.f_current_position = (double)current_position;

	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PORT_DATA.f_target_position) {
		PORT_DATA.f_target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PORT_DATA.f_target_position) {
		PORT_DATA.f_target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PORT_DATA.f_target_position = %d", PORT_DATA.f_target_position);

	if (!lunatico_goto_position(device, (uint32_t)PORT_DATA.f_target_position, (uint32_t)FOCUSER_BACKLASH_ITEM->number.value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_goto_position(%d, %d, %d) failed", PRIVATE_DATA->handle, PORT_DATA.f_target_position, (uint32_t)FOCUSER_BACKLASH_ITEM->number.value);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	PORT_DATA.prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PORT_DATA.f_current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PORT_DATA.focuser_timer);
}


static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	lunatico_enumerate_properties(device, client, property);
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 100000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min;

		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.step = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min;

		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.max = 200;
		FOCUSER_BACKLASH_ITEM->number.step = 5;
		FOCUSER_BACKLASH_ITEM->number.min = FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = 0;

		FOCUSER_SPEED_PROPERTY->hidden = false;
		FOCUSER_SPEED_ITEM->number.min = .002;
		FOCUSER_SPEED_ITEM->number.max = 20;
		FOCUSER_SPEED_ITEM->number.step = 0.1;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 0.1;
		strcpy(FOCUSER_SPEED_ITEM->label, "Speed (kHz)");

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 100;
		FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;

		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;

		if (lunatico_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_focuser_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (lunatico_open(device)) {
				lunatico_init_device(device);

				int32_t position = 0;
				if (!lunatico_get_position(device, &position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
				}
				FOCUSER_POSITION_ITEM->number.value = (double)position;

				if (!lunatico_set_speed(device, FOCUSER_SPEED_ITEM->number.target)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_speed(%d) failed", PRIVATE_DATA->handle);
				}

				bool success = false;
				if (LA_WIRING_LUNATICO_ITEM->sw.value) {
					if(FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value) {
						success = lunatico_set_wiring(device, MW_LUNATICO_NORMAL);
					} else {
						success= lunatico_set_wiring(device, MW_LUNATICO_REVERSED);
					}
				} else if (LA_WIRING_MOONLITE_ITEM->sw.value) {
					if (FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value) {
						success = lunatico_set_wiring(device, MW_MOONLITE_NORMAL);
					} else {
						success = lunatico_set_wiring(device, MW_MOONLITE_REVERSED);
					}
				}
				if (!success) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_wiring(%d) failed", PRIVATE_DATA->handle);
				}

				if (FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value == FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max &&
					FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value == FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min) {
					success = lunatico_delete_limits(device);
				} else {
					success = lunatico_set_limits(device, FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value, FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value);
				}
				if (!success) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_limits(%d) failed", PRIVATE_DATA->handle);
				}

				lunatico_get_temperature(device, 0, &FOCUSER_TEMPERATURE_ITEM->number.value);
				PORT_DATA.prev_temp = FOCUSER_TEMPERATURE_ITEM->number.value;
				indigo_set_timer(device, 1, temperature_timer_callback, &PORT_DATA.temperature_timer);

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PORT_DATA.focuser_timer);
			}
		}
	} else {
		if (DEVICE_CONNECTED) {
			indigo_cancel_timer_sync(device, &PORT_DATA.focuser_timer);
			indigo_cancel_timer_sync(device, &PORT_DATA.temperature_timer);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PORT_DATA.temperature_timer == %p", PORT_DATA.temperature_timer);
			lunatico_delete_properties(device);
			lunatico_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}


static void handle_focuser_disconnect(indigo_device *device, indigo_client *client, indigo_property *property) {


	if (client && property) indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, handle_focuser_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		bool success = true;
		if (LA_WIRING_LUNATICO_ITEM->sw.value) {
			if(FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_LUNATICO_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_LUNATICO_REVERSED);
			}
		} else if (LA_WIRING_MOONLITE_ITEM->sw.value) {
			if (FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_MOONLITE_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_MOONLITE_REVERSED);
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported Motor wiring");
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!success) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_wiring() failed");
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_w(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_ITEM->number.target < 0 || FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (FOCUSER_POSITION_ITEM->number.target == PORT_DATA.f_current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else { /* GOTO position */
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PORT_DATA.f_target_position = FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = PORT_DATA.f_current_position;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				if (!lunatico_goto_position(device, (uint32_t)PORT_DATA.f_target_position, (uint32_t)FOCUSER_BACKLASH_ITEM->number.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_goto_position(%d, %d, %d) failed", PRIVATE_DATA->handle, PORT_DATA.f_target_position, (uint32_t)FOCUSER_BACKLASH_ITEM->number.value);
				}
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PORT_DATA.focuser_timer);
			} else { /* RESET CURRENT POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				if(!lunatico_sync_position(device, PORT_DATA.f_target_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_sync_position(%d, %d) failed", PRIVATE_DATA->handle, PORT_DATA.f_target_position);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				int32_t position = 0;
				if (!lunatico_get_position(device, &position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					FOCUSER_POSITION_ITEM->number.value = PORT_DATA.f_current_position = (double)position;
				}
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		int success = true;
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		int max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		int min_position = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target;

		if (max_position < min_position) {
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, "Minimum value can not be bigger then maximum");
			return INDIGO_OK;
		}
		if (FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target == FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max &&
			FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target == FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min) {
			success = lunatico_delete_limits(device);
		} else {
			success = lunatico_set_limits(device, min_position, max_position);
		}
		if (!success) {
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;

		if (!lunatico_set_speed(device, FOCUSER_SPEED_ITEM->number.target)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_speed(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			int32_t position = 0;
			if (!lunatico_get_position(device, &position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
			} else {
				PORT_DATA.f_current_position = (double)position;
			}

			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PORT_DATA.f_target_position = PORT_DATA.f_current_position - FOCUSER_STEPS_ITEM->number.value;
			} else {
				PORT_DATA.f_target_position = PORT_DATA.f_current_position + FOCUSER_STEPS_ITEM->number.value;
			}

			// Make sure we do not attempt to go beyond the limits
			if (FOCUSER_POSITION_ITEM->number.max < PORT_DATA.f_target_position) {
				PORT_DATA.f_target_position = FOCUSER_POSITION_ITEM->number.max;
			} else if (FOCUSER_POSITION_ITEM->number.min > PORT_DATA.f_target_position) {
				PORT_DATA.f_target_position = FOCUSER_POSITION_ITEM->number.min;
			}

			FOCUSER_POSITION_ITEM->number.value = PORT_DATA.f_current_position;
			if (!lunatico_goto_position(device, (uint32_t)PORT_DATA.f_target_position, 0)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_goto_position(%d, %d, 0) failed", PRIVATE_DATA->handle, PORT_DATA.f_target_position);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PORT_DATA.focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PORT_DATA.focuser_timer);

		if (!lunatico_stop(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_stop(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		int32_t position = 0;
		if (!lunatico_get_position(device, &position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_get_position(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PORT_DATA.f_current_position = (double)position;
		}
		FOCUSER_POSITION_ITEM->number.value = PORT_DATA.f_current_position;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION_PROPERTY
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH_PROPERTY
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_WIRING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_WIRING_PROPERTY
		bool success = true;
		indigo_property_copy_values(LA_WIRING_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		LA_WIRING_PROPERTY->state = INDIGO_OK_STATE;
		if (LA_WIRING_LUNATICO_ITEM->sw.value) {
			if(FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_LUNATICO_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_LUNATICO_REVERSED);
			}
		} else if (LA_WIRING_MOONLITE_ITEM->sw.value) {
			if (FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value) {
				success = lunatico_set_wiring(device, MW_MOONLITE_NORMAL);
			} else {
				success = lunatico_set_wiring(device, MW_MOONLITE_REVERSED);
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported Motor wiring");
			LA_WIRING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!success) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_wiring() failed");
			LA_WIRING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, LA_WIRING_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	lunatico_common_update_property(device, client, property);
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	if (DEVICE_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_focuser_connect_property(device);
	}
	lunatico_detach(device);
	return indigo_focuser_detach(device);
}


// --------------------------------------------------------------------------------

static void create_port_device(int device_index, int port_index, device_type_t device_type) {
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_LUNATICO_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(
		ROTATOR_LUNATICO_NAME,
		rotator_attach,
		rotator_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
	);

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_LUNATICO_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	if (port_index >= MAX_PORTS) return;
	if (device_index >= MAX_DEVICES) return;
	if (device_data[device_index].port[port_index] != NULL) {
		if ((device_data[device_index].private_data) && (device_data[device_index].private_data->port_data[port_index].device_type == device_type)) {
				return;
		} else {
				delete_port_device(device_index, port_index);
		}
	}

	if (device_data[device_index].private_data == NULL) {
		device_data[device_index].private_data = indigo_safe_malloc(sizeof(lunatico_private_data));
		pthread_mutex_init(&device_data[device_index].private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	if (device_type == TYPE_FOCUSER) {
		device_data[device_index].port[port_index] = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
		sprintf(device_data[device_index].port[port_index]->name, "%s (%s)", FOCUSER_LUNATICO_NAME, port_name[port_index]);
		device_data[device_index].private_data->port_data[port_index].device_type = TYPE_FOCUSER;
	} else if (device_type == TYPE_ROTATOR) {
		device_data[device_index].port[port_index] = indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
		sprintf(device_data[device_index].port[port_index]->name, "%s (%s)", ROTATOR_LUNATICO_NAME, port_name[port_index]);
		device_data[device_index].private_data->port_data[port_index].device_type = TYPE_ROTATOR;
	} else {
		device_data[device_index].port[port_index] = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
		sprintf(device_data[device_index].port[port_index]->name, "%s (%s)", AUX_LUNATICO_NAME, port_name[port_index]);
		device_data[device_index].private_data->port_data[port_index].device_type = TYPE_AUX;
	}
	set_port_index(device_data[device_index].port[port_index], port_index);
	device_data[device_index].port[port_index]->private_data = device_data[device_index].private_data;
	indigo_attach_device(device_data[device_index].port[port_index]);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: Device with port index = %d", get_port_index(device_data[device_index].port[port_index]));
}


static void delete_port_device(int device_index, int port_index) {
	if (port_index >= MAX_PORTS) return;
	if (device_index >= MAX_DEVICES) return;

	if (device_data[device_index].port[port_index] != NULL) {
		indigo_detach_device(device_data[device_index].port[port_index]);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: Device with port index = %d", get_port_index(device_data[device_index].port[port_index]));
		free(device_data[device_index].port[port_index]);
		device_data[device_index].port[port_index] = NULL;
	}

	for (int i = 0; i < MAX_PORTS; i++) {
		if (device_data[device_index].port[i] != NULL) return;
	}

	if (device_data[device_index].private_data != NULL) {
		free(device_data[device_index].private_data);
		device_data[device_index].private_data = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: PRIVATE_DATA");
	}
}


static bool at_least_one_device_connected() {
	for (int p_index = 0; p_index < MAX_PORTS; p_index++) {
		for (int d_index = 0; d_index < MAX_DEVICES; d_index++) {
			if (is_connected(device_data[d_index].port[p_index])) return true;
		}
	}
	return false;
}


indigo_result DRIVER_ENTRY_POINT(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		if (indigo_driver_initialized(CONFLICTING_DRIVER)) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Conflicting driver %s is already loaded", CONFLICTING_DRIVER);
			last_action = INDIGO_DRIVER_SHUTDOWN;
			return INDIGO_FAILED;
		}
		create_port_device(0, 0, DEFAULT_DEVICE);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		if (at_least_one_device_connected() == true) return INDIGO_BUSY;
		last_action = action;
		for (int index = 0; index < MAX_PORTS; index++) {
			delete_port_device(0, index);
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
