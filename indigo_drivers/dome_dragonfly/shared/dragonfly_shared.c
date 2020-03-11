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

#define DOME_DRAGONFLY_NAME    "Dome Dragonfly"
#define AUX_DRAGONFLY_NAME     "Powerbox Dragonfly"

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
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_aux_driver.h>

#define DEFAULT_BAUDRATE            "115200"

#define ROTATOR_SPEED 1

#define MAX_LOGICAL_DEVICES  2
#define MAX_PHYSICAL_DEVICES 4

#define DEVICE_CONNECTED_MASK            0x80
#define L_DEVICE_INDEX_MASK              0x0F

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)

#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define get_locical_device_index(dev)              ((dev)->gp_bits & L_DEVICE_INDEX_MASK)
#define set_logical_device_index(dev, index)       ((dev)->gp_bits = ((dev)->gp_bits & ~L_DEVICE_INDEX_MASK) | (L_DEVICE_INDEX_MASK & index))

#define device_exists(p_device_index, l_device_index) (device_data[p_device_index].device[l_device_index] != NULL)

#define PRIVATE_DATA                      ((lunatico_private_data *)device->private_data)
#define DEVICE_DATA                       (PRIVATE_DATA->device_data[get_locical_device_index(device)])

#define AUX_POWERBOX_GROUP	"Powerbox"

#define AUX_OUTLET_NAMES_PROPERTY      (DEVICE_DATA.outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_2_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_3_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_PROPERTY      (DEVICE_DATA.power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 3)

#define AUX_SENSORS_GROUP	"Sensors"

#define AUX_SENSOR_NAMES_PROPERTY      (DEVICE_DATA.sensor_names_property)
#define AUX_SENSOR_NAME_1_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_2_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)

#define AUX_GPIO_SENSORS_PROPERTY     (DEVICE_DATA.sensors_property)
#define AUX_GPIO_SENSOR_1_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_2_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_3_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_4_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 3)


typedef enum {
	TYPE_DOME    = 0,
	TYPE_AUX     = 1
} device_type_t;

typedef struct {
	device_type_t device_type;
	double prev_temp;

	indigo_timer *temperature_timer;
	indigo_timer *sensors_timer;
	indigo_property *outlet_names_property,
	                *power_outlet_property,
	                *sensor_names_property,
	                *sensors_property;
} logical_device_data;

typedef struct {
	int handle;
	int count_open;
	int port_count;
	bool udp;
	pthread_mutex_t port_mutex;
	logical_device_data device_data[MAX_LOGICAL_DEVICES];
} lunatico_private_data;

typedef struct {
	indigo_device *device[MAX_LOGICAL_DEVICES];
	lunatico_private_data *private_data;
} lunatico_device_data;

static void compensate_focus(indigo_device *device, double new_temp);

static lunatico_device_data device_data[MAX_PHYSICAL_DEVICES] = {0};

static void create_port_device(int p_device_index, int l_device_index, device_type_t type);
static void delete_port_device(int p_device_index, int l_device_index);

/* Linatico Astronomia device Commands ======================================================================== */

#define LUNATICO_CMD_LEN 100

typedef enum {
	MODEL_SELETEK = 1,
	MODEL_ARMADILLO = 2,
	MODEL_PLATYPUS = 3,
	MODEL_DRAGONFLY = 4,
	MODEL_LIMPET = 5
} lunatico_model_t;

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

	const char *operative[3] = { "", "Bootloader", "Error" };
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

static bool lunatico_enable_power_outlet(indigo_device *device, int pin, bool enable) {
	char command[LUNATICO_CMD_LEN];
	int res;
	if (pin < 1 || pin > 4) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!write dig %d %d %d#", get_locical_device_index(device), pin, enable ? 1 : 0);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_read_sensor(indigo_device *device, int pin, int *sensor_value) {
	if (!sensor_value) return false;

	char command[LUNATICO_CMD_LEN];
	int value;

	if (pin < 5 || pin > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!read an %d %d#", get_locical_device_index(device), pin);
	if (!lunatico_command_get_result(device, command, &value)) return false;
	if (value >= 0) {
		*sensor_value = value;
		return true;
	}
	return false;
}

static bool lunatico_analog_read_sensor(indigo_device *device, int sensor, int *sensor_value) {
	if (!sensor_value) return false;

	char command[LUNATICO_CMD_LEN];
	int value;

	if (sensor < 0 || sensor > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio snanrd 1 %d#", sensor);
	if (!lunatico_command_get_result(device, command, &value)) return false;
	if (value >= 0) {
		*sensor_value = value;
		return true;
	}
	return false;
}

static bool lunatico_digital_read_sensor(indigo_device *device, int sensor, bool *sensor_value) {
	if (!sensor_value) return false;

	char command[LUNATICO_CMD_LEN];
	int value;

	if (sensor < 0 || sensor > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio sndgrd 1 %d#", sensor);
	if (!lunatico_command_get_result(device, command, &value)) return false;
	if (value >= 0) {
		*sensor_value = (bool)value;
		return true;
	}
	return false;
}


static bool lunatico_read_relay(indigo_device *device, int relay, bool *enabled) {
	if (!enabled) return false;

	char command[LUNATICO_CMD_LEN];
	int value;

	if (relay < 0 || relay > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio sndgrd 1 %d#", relay);
	if (!lunatico_command_get_result(device, command, &value)) return false;
	if (value >= 0) {
		*enabled = (bool)value;
		return true;
	}
	return false;
}


static bool lunatico_set_relay(indigo_device *device, int relay, bool enable) {
	char command[LUNATICO_CMD_LEN];
	int res;
	if (relay < 0 || relay > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio rlset 0 %d#", relay, enable ? 1 : 0);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
	return true;
}


static bool lunatico_pulse_relay(indigo_device *device, int relay, uint32_t lenms) {
	char command[LUNATICO_CMD_LEN];
	int res;
	if (relay < 0 || relay > 8) return false;

	snprintf(command, LUNATICO_CMD_LEN, "!relio rlpulse 0 %d#", relay, lenms);
	if (!lunatico_command_get_result(device, command, &res)) return false;
	if (res != 0) return false;
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
	strncpy(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE, INDIGO_VALUE_SIZE);
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 5;
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "DB9 Pin 1", "Power #1");
	indigo_init_text_item(AUX_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "DB9 Pin 2", "Power #2");
	indigo_init_text_item(AUX_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "DB9 Pin 3", "Power #3");
	indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "DB9 Pin 4", "Power #4");
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_OUTLET_NAMES_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- POWER OUTLETS
	AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
	if (AUX_POWER_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Power #1", false);
	indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power #2", false);
	indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Power #3", false);
	indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Power #4", false);
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_POWER_OUTLET_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- SENSOR_NAMES
	AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (AUX_SENSOR_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_SENSOR_NAME_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "DB9 Pin 6", "Sensor #1");
	indigo_init_text_item(AUX_SENSOR_NAME_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "DB9 Pin 7", "Sensor #2");
	indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "DB9 Pin 8", "Sensor #3");
	indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "DB9 Pin 9", "Sensor #4");
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_SENSOR_NAMES_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- GPIO_SENSORS
	AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "GPIO sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
	if (AUX_GPIO_SENSORS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_SENSOR_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor #1", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor #2", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_GPIO_SENSORS_PROPERTY->hidden = true;
	//---------------------------------------------------------------------------
	indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}


static indigo_result lunatico_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_GPIO_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}


static void lunatico_init_device(indigo_device *device) {
	char board[LUNATICO_CMD_LEN] = "N/A";
	char firmware[LUNATICO_CMD_LEN] = "N/A";
	uint32_t value;
	if (lunatico_get_info(device, board, firmware)) {
		strncpy(INFO_DEVICE_MODEL_ITEM->text.value, board, INDIGO_VALUE_SIZE);
		strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
		indigo_update_property(device, INFO_PROPERTY, NULL);
	}
}


static indigo_result lunatico_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	lunatico_close(device);
	indigo_device_disconnect(NULL, device->name);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	indigo_delete_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);
	return INDIGO_OK;
}


static void lunatico_save_properties(indigo_device *device) {
	indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
	indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
}


static void lunatico_delete_properties(indigo_device *device) {
	indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
}


static indigo_result lunatico_common_update_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(CONFIG_PROPERTY, property)) {
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
	indigo_reschedule_timer(device, 3, &DEVICE_DATA.sensors_timer);
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
	if (indigo_aux_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO | INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		if (lunatico_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		int position;
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!DEVICE_CONNECTED) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				if (lunatico_open(device)) {
					char board[LUNATICO_CMD_LEN] = "N/A";
					char firmware[LUNATICO_CMD_LEN] = "N/A";
					if (lunatico_get_info(device, board, firmware)) {
						strncpy(INFO_DEVICE_MODEL_ITEM->text.value, board, INDIGO_VALUE_SIZE);
						strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
						indigo_update_property(device, INFO_PROPERTY, NULL);
					}
					indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
					indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
					set_power_outlets(device);
					DEVICE_DATA.sensors_timer = indigo_set_timer(device, 0, sensors_timer_callback);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
		} else {
			if (DEVICE_CONNECTED) {
				indigo_cancel_timer(device, &DEVICE_DATA.sensors_timer);
				lunatico_delete_properties(device);
				lunatico_close(device);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

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
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_2_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_3_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_4_ITEM->text.value);
		AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
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
	lunatico_detach(device);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void dome_timer_callback(indigo_device *device) {

}

static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// This is a sliding roof -> we need only open and close
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_EQUATORIAL_COORDINATES_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;

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
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
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

static int device_number = 0;

static void create_port_device(int p_device_index, int l_device_index, device_type_t device_type) {

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_DRAGONFLY_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(
		DOME_DRAGONFLY_NAME,
		dome_attach,
		indigo_dome_enumerate_properties,
		dome_change_property,
		NULL,
		dome_detach
	);

	if (l_device_index >= MAX_LOGICAL_DEVICES) return;
	if (p_device_index >= MAX_PHYSICAL_DEVICES) return;
	if (device_data[p_device_index].device[l_device_index] != NULL) {
		if ((device_data[p_device_index].private_data) && (device_data[p_device_index].private_data->device_data[l_device_index].device_type == device_type)) {
				return;
		} else {
				delete_port_device(p_device_index, l_device_index);
		}
	}

	if (device_data[p_device_index].private_data == NULL) {
		device_data[p_device_index].private_data = malloc(sizeof(lunatico_private_data));
		assert(device_data[p_device_index].private_data != NULL);
		memset(device_data[p_device_index].private_data, 0, sizeof(lunatico_private_data));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	device_data[p_device_index].device[l_device_index] = malloc(sizeof(indigo_device));
	assert(device_data[p_device_index].device[l_device_index] != NULL);
	if (device_type == TYPE_DOME) {
		memcpy(device_data[p_device_index].device[l_device_index], &dome_template, sizeof(indigo_device));
		sprintf(device_data[p_device_index].device[l_device_index]->name, "%s", DOME_DRAGONFLY_NAME);
		device_data[p_device_index].private_data->device_data[l_device_index].device_type = TYPE_DOME;
	} else {
		memcpy(device_data[p_device_index].device[l_device_index], &aux_template, sizeof(indigo_device));
		sprintf(device_data[p_device_index].device[l_device_index]->name, "%s", AUX_DRAGONFLY_NAME);
		device_data[p_device_index].private_data->device_data[l_device_index].device_type = TYPE_AUX;
	}
	set_logical_device_index(device_data[p_device_index].device[l_device_index], l_device_index);
	device_data[p_device_index].device[l_device_index]->private_data = device_data[p_device_index].private_data;
	indigo_attach_device(device_data[p_device_index].device[l_device_index]);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: Device with port index = %d", get_locical_device_index(device_data[p_device_index].device[l_device_index]));
}


static void delete_port_device(int p_device_index, int l_device_index) {
	if (l_device_index >= MAX_LOGICAL_DEVICES) return;
	if (p_device_index >= MAX_PHYSICAL_DEVICES) return;

	if (device_data[p_device_index].device[l_device_index] != NULL) {
		indigo_detach_device(device_data[p_device_index].device[l_device_index]);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: Locical Device with index = %d", get_locical_device_index(device_data[p_device_index].device[l_device_index]));
		free(device_data[p_device_index].device[l_device_index]);
		device_data[p_device_index].device[l_device_index] = NULL;
	}

	for (int i = 0; i < MAX_LOGICAL_DEVICES; i++) {
		if (device_data[p_device_index].device[i] != NULL) return;
	}

	if (device_data[p_device_index].private_data != NULL) {
		free(device_data[p_device_index].private_data);
		device_data[p_device_index].private_data = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: PRIVATE_DATA");
	}
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
		create_port_device(0, 1, TYPE_AUX);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		for (int index = 0; index < MAX_LOGICAL_DEVICES; index++) {
			delete_port_device(0, index);
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
