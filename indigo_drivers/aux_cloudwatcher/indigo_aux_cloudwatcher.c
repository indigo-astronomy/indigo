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

/** INDIGO Lunatico AAG CloudWatcher AUX driver
 \file indigo_aux_cloudwatcher.c
 */

#include "indigo_aux_cloudwatcher.h"

#define DRIVER_VERSION         0x0001
#define AUX_CLOUDWATCHER_NAME  "AAG CloudWatcher"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_aux_driver.h>

#define DEFAULT_BAUDRATE            "9600"

#define ROTATOR_SPEED 1

#define MAX_DEVICES 1

#define PRIVATE_DATA                   ((lunatico_private_data *)device->private_data)

#define AUX_RELAYS_GROUP	"Relay control"

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_2_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_3_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_OUTLET_NAME_5_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_OUTLET_NAME_6_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_OUTLET_NAME_7_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_OUTLET_NAME_8_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_OUTLET_PROPERTY      (PRIVATE_DATA->gpio_outlet_property)
#define AUX_GPIO_OUTLET_1_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_2_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 1)
#define AUX_GPIO_OUTLET_3_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 2)
#define AUX_GPIO_OUTLET_4_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 3)
#define AUX_GPIO_OUTLET_5_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 4)
#define AUX_GPIO_OUTLET_6_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 5)
#define AUX_GPIO_OUTLET_7_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 6)
#define AUX_GPIO_OUTLET_8_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 7)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY      (PRIVATE_DATA->gpio_outlet_pulse_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_2_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_3_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)
#define AUX_OUTLET_PULSE_LENGTHS_5_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 4)
#define AUX_OUTLET_PULSE_LENGTHS_6_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 5)
#define AUX_OUTLET_PULSE_LENGTHS_7_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 6)
#define AUX_OUTLET_PULSE_LENGTHS_8_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 7)

#define AUX_SENSORS_GROUP	"Sensors"

#define AUX_SENSOR_NAMES_PROPERTY      (PRIVATE_DATA->sensor_names_property)
#define AUX_SENSOR_NAME_1_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_2_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)
#define AUX_SENSOR_NAME_5_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 4)
#define AUX_SENSOR_NAME_6_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 5)
#define AUX_SENSOR_NAME_7_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 6)
#define AUX_SENSOR_NAME_8_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_SENSORS_PROPERTY     (PRIVATE_DATA->sensors_property)
#define AUX_GPIO_SENSOR_1_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_2_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_3_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_4_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 3)
#define AUX_GPIO_SENSOR_5_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 4)
#define AUX_GPIO_SENSOR_6_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 5)
#define AUX_GPIO_SENSOR_7_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 6)
#define AUX_GPIO_SENSOR_8_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 7)

typedef struct {

} logical_device_data;

typedef struct {
	int handle;
	float firmware;
	bool udp;
	pthread_mutex_t port_mutex;
	bool relay_active[8];
	indigo_timer *relay_timers[8];
	pthread_mutex_t relay_mutex;
	indigo_timer *sensors_timer;

	indigo_property *outlet_names_property,
	                *gpio_outlet_property,
	                *gpio_outlet_pulse_property,
	                *sensor_names_property,
	                *sensors_property;
} lunatico_private_data;

typedef struct {
	indigo_device *device;
	lunatico_private_data *private_data;
} lunatico_device_data;

static lunatico_device_data device_data[MAX_DEVICES] = {0};

static void create_port_device(int device_index);
static void delete_port_device(int device_index);


#define DEVICE_CONNECTED_MASK            0x80

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)

#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define LUNATICO_CMD_LEN 100
#define BLOCK_SIZE 15

/* Linatico AAG CloudWatcher device Commands ======================================================================== */
static bool aag_command(indigo_device *device, const char *command, char *response, int block_count, int sleep) {
	int max = block_count * BLOCK_SIZE;
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
			}
		}
		if ((index > BLOCK_SIZE) && (response[index - BLOCK_SIZE] == '!')) {
			response[index - BLOCK_SIZE] = '\0';
		} else {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			response[index-1] = '\0';
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wrong response %s -> %s", command, response);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}


bool aag_is_cloudwatcher(indigo_device *device, char *name) {
	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "A!", buffer, 2, 0);

	if (!r) return false;

	if (name) sscanf(buffer, "%*s %15s", name);
	const char *internal_name_block = "!N CloudWatcher";
	for (int i = 0; i < 15; i++) {
		if (buffer[i] != internal_name_block[i]) {
			return false;
		}
	}
	return true;
}

bool aag_reset_buffers(indigo_device *device) {
	bool r = aag_command(device, "z!", NULL, 0, 0);
	if (!r) return false;
	return true;
}

bool aag_get_firmware_version(indigo_device *device, char *version) {
	if (version == NULL) return false;

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "B!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!V %4s", version);
	if (res != 1) return false;

	PRIVATE_DATA->firmware = atof(version);
	return true;
}

bool aag_get_serial_number(indigo_device *device, char *serial_number) {
	if (serial_number == NULL) return false;

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "K!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!K %4s", serial_number);
	if (res != 1) return false;

	return true;
}


bool aag_get_values(indigo_device *device, int *power_voltage, int *ambient_temperature, int *ldr_value, int *rain_sensor_temperature) {
	int zenerV;
	int ambTemp = -10000;
	int ldrRes;
	int rainSensTemp;

	if (PRIVATE_DATA->firmware >= 3.0) {
		char buffer[BLOCK_SIZE * 4];
		bool r = aag_command(device, "C!", buffer, 4, 0);
		if (!r) return false;

		int res = sscanf(buffer, "!6 %d!4 %d!5 %d", &zenerV, &ldrRes, &rainSensTemp);
		if (res != 3) return false;
	} else {
		char buffer[BLOCK_SIZE * 5];
		bool r = aag_command(device, "C!", buffer, 5, 0);
		if (!r) return false;

		int res = sscanf(buffer, "!6 %d!3 %d!4 %d!5 %d", &zenerV, &ambTemp, &ldrRes, &rainSensTemp);

		if (res != 4) return false;
	}

	*power_voltage           = zenerV;
	*ambient_temperature     = ambTemp;
	*ldr_value               = ldrRes;
	*rain_sensor_temperature = rainSensTemp;

	INDIGO_DRIVER_DEBUG(
		DRIVER_NAME,
		"Values: version = %f, power_voltage = %d, ambient_temperature = %d, ldr_value = %d, rain_sensor_temperature = %d",
		PRIVATE_DATA->firmware,
		*power_voltage,
		*ambient_temperature,
		*ldr_value,
		*rain_sensor_temperature
	);

	return true;
}


bool aag_get_ir_sky_temperature(indigo_device *device, int *temp) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "S!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!1 %d", temp);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "temp = %d", *temp);
	return true;
}

bool aag_get_sensor_temperature(indigo_device *device, int *temp) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "T!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!2 %d", temp);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "temp = %d", *temp);
	return true;
}

bool aag_get_rain_frequency(indigo_device *device, int *rain_freqency) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "E!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!R %d", rain_freqency);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "rain_freqency = %d", *rain_freqency);
	return true;
}

bool aag_get_electrical_constants(
		indigo_device *device,
		float *zenerConstant,
		float *LDRMaxResistance,
		float *LDRPullUpResistance,
		float *rainBeta,
		float *rainResAt25,
		float *rainPullUpResistance
	) {

	if (PRIVATE_DATA->firmware < 3.0) return false;

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "M!", buffer, 2, 0);
	if (!r) return false;

	if (buffer[1] != 'M') return false;

	*zenerConstant        = (256 * buffer[2] + buffer[3]) / 100.0;
	*LDRMaxResistance     = (256 * buffer[4] + buffer[5]) / 1.0;
	*LDRPullUpResistance  = (256 * buffer[6] + buffer[7]) / 10.0;
	*rainBeta             = (256 * buffer[8] + buffer[9]) / 1.0;
	*rainResAt25          = (256 * buffer[10] + buffer[11]) / 10.0;
	*rainPullUpResistance = (256 * buffer[12] + buffer[13]) / 10.0;

	return true;
}

// --------------------------------------------------------------------------------- Common stuff
static bool lunatico_open(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OPEN REQUESTED: %d -> %d", PRIVATE_DATA->handle, DEVICE_CONNECTED);
	if (DEVICE_CONNECTED) return false;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}

	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "aag")) {
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
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}
	set_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}

static void lunatico_close(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSE REQUESTED: %d -> %d", PRIVATE_DATA->handle, DEVICE_CONNECTED);
	if (!DEVICE_CONNECTED) return;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d)", PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	PRIVATE_DATA->handle = 0;
	clear_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
}


// --------------------------------------------------------------------------------- INDIGO AUX RELAYS device implementation

static int lunatico_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- AUTHENTICATION
	//AUTHENTICATION_PROPERTY->hidden = false;
	//AUTHENTICATION_PROPERTY->count = 1;
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_PORTS
	DEVICE_PORTS_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
	DEVICE_BAUDRATE_PROPERTY->hidden = true;
	strncpy(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE, INDIGO_VALUE_SIZE);
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 7;
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay names", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_GPIO_OUTLET_NAME_1_ITEM_NAME, "Relay 1", "Relay #1");
	indigo_init_text_item(AUX_OUTLET_NAME_2_ITEM, AUX_GPIO_OUTLET_NAME_2_ITEM_NAME, "Relay 2", "Relay #2");
	indigo_init_text_item(AUX_OUTLET_NAME_3_ITEM, AUX_GPIO_OUTLET_NAME_3_ITEM_NAME, "Relay 3", "Relay #3");
	indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_GPIO_OUTLET_NAME_4_ITEM_NAME, "Relay 4", "Relay #4");
	indigo_init_text_item(AUX_OUTLET_NAME_5_ITEM, AUX_GPIO_OUTLET_NAME_5_ITEM_NAME, "Relay 5", "Relay #5");
	indigo_init_text_item(AUX_OUTLET_NAME_6_ITEM, AUX_GPIO_OUTLET_NAME_6_ITEM_NAME, "Relay 6", "Relay #6");
	indigo_init_text_item(AUX_OUTLET_NAME_7_ITEM, AUX_GPIO_OUTLET_NAME_7_ITEM_NAME, "Relay 7", "Relay #7");
	indigo_init_text_item(AUX_OUTLET_NAME_8_ITEM, AUX_GPIO_OUTLET_NAME_8_ITEM_NAME, "Relay 8", "Relay #8");
	// -------------------------------------------------------------------------------- GPIO OUTLETS
	AUX_GPIO_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 8);
	if (AUX_GPIO_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_GPIO_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Relay #1", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Relay #2", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Relay #3", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", false);
	// -------------------------------------------------------------------------------- GPIO PULSE OUTLETS
	AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, "AUX_OUTLET_PULSE_LENGTHS", AUX_RELAYS_GROUP, "Relay pulse lengths (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
	if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Relay #1", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Relay #2", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Relay #3", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", 0, 100000, 100, 0);
	// -------------------------------------------------------------------------------- SENSOR_NAMES
	AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
	if (AUX_SENSOR_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_SENSOR_NAME_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor 1", "Sensor #1");
	indigo_init_text_item(AUX_SENSOR_NAME_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor 2", "Sensor #2");
	indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor 3", "Sensor #3");
	indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor 4", "Sensor #4");
	indigo_init_text_item(AUX_SENSOR_NAME_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor 5", "Sensor #5");
	indigo_init_text_item(AUX_SENSOR_NAME_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor 6", "Sensor #6");
	indigo_init_text_item(AUX_SENSOR_NAME_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor 7", "Sensor #7");
	indigo_init_text_item(AUX_SENSOR_NAME_8_ITEM, AUX_GPIO_SENSOR_NAME_8_ITEM_NAME, "Sensor 8", "Sensor #8");
	// -------------------------------------------------------------------------------- GPIO_SENSORS
	AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
	if (AUX_GPIO_SENSORS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_SENSOR_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor #1", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor #2", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor #5", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor #6", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor #7", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_8_ITEM, AUX_GPIO_SENSOR_NAME_8_ITEM_NAME, "Sensor #8", 0, 1024, 1, 0);
	//---------------------------------------------------------------------------
	indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}


static void sensors_timer_callback(indigo_device *device) {
	int sensor_value;
	int power_voltage, ambient_temperature, ldr_value, rain_sensor_temperature;
	bool success = aag_get_values(device, &power_voltage, &ambient_temperature, &ldr_value, &rain_sensor_temperature);
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->sensors_timer);
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (DEVICE_CONNECTED) {
		if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property))
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		if (indigo_property_match(AUX_GPIO_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);

	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->relay_mutex, NULL);
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
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!DEVICE_CONNECTED) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				aag_reset_buffers(device);
				if (lunatico_open(device)) {
					char board[LUNATICO_CMD_LEN] = "N/A";
					char firmware[LUNATICO_CMD_LEN] = "N/A";
					char serial_number[LUNATICO_CMD_LEN] = "N/A";
					if (aag_is_cloudwatcher(device, board)) {
						strncpy(INFO_DEVICE_MODEL_ITEM->text.value, board, INDIGO_VALUE_SIZE);
						aag_get_firmware_version(device, firmware);
						strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
						aag_get_serial_number(device, serial_number);
						strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, serial_number, INDIGO_VALUE_SIZE);
						indigo_update_property(device, INFO_PROPERTY, NULL);

						indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
						indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
						indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);

						PRIVATE_DATA->sensors_timer = indigo_set_timer(device, 0, sensors_timer_callback);
						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					} else {
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
						lunatico_close(device);
					}
				}
			}
		} else {
			if (DEVICE_CONNECTED) {
				indigo_cancel_timer(device, &PRIVATE_DATA->sensors_timer);

				indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
				indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
				indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);

				lunatico_close(device);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (DEVICE_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_5_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_6_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_7_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_8_ITEM->text.value);

		snprintf(AUX_OUTLET_PULSE_LENGTHS_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_5_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_6_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_7_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_8_ITEM->text.value);

		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(AUX_GPIO_OUTLET_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;

		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
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
		snprintf(AUX_GPIO_SENSOR_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_5_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_6_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_7_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_8_ITEM->text.value);
		AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION_PROPERTY
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		if (AUTHENTICATION_PASSWORD_ITEM->text.value[0] == 0) {
		} else {
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_aux_change_property(device, client, property);
}


static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	lunatico_close(device);
	indigo_device_disconnect(NULL, device->name);
	indigo_release_property(AUX_GPIO_OUTLET_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	indigo_delete_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);
	return indigo_aux_detach(device);
}

// --------------------------------------------------------------------------------

static int device_number = 0;

static void create_port_device(int device_index) {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_CLOUDWATCHER_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	if (device_index >= MAX_DEVICES) return;
	if (device_data[device_index].device != NULL) return;

	if (device_data[device_index].private_data == NULL) {
		device_data[device_index].private_data = malloc(sizeof(lunatico_private_data));
		assert(device_data[device_index].private_data != NULL);
		memset(device_data[device_index].private_data, 0, sizeof(lunatico_private_data));
		pthread_mutex_init(&device_data[device_index].private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	device_data[device_index].device = malloc(sizeof(indigo_device));
	assert(device_data[device_index].device != NULL);

	memcpy(device_data[device_index].device, &aux_template, sizeof(indigo_device));
	sprintf(device_data[device_index].device->name, "%s", AUX_CLOUDWATCHER_NAME);

	device_data[device_index].device->private_data = device_data[device_index].private_data;
	indigo_attach_device(device_data[device_index].device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: Device with port index = %d", device_index);
}


static void delete_port_device(int device_index) {
	if (device_index >= MAX_DEVICES) return;

	if (device_data[device_index].device != NULL) {
		indigo_detach_device(device_data[device_index].device);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: Locical Device with index = %d", device_index);
		free(device_data[device_index].device);
		device_data[device_index].device = NULL;
	}

	if (device_data[device_index].private_data != NULL) {
		free(device_data[device_index].private_data);
		device_data[device_index].private_data = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: PRIVATE_DATA");
	}
}


indigo_result indigo_aux_cloudwatcher(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int index = 0; index < MAX_DEVICES; index++) {
			create_port_device(index);
		}
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		for (int index = 0; index < MAX_DEVICES; index++) {
			delete_port_device(index);
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
