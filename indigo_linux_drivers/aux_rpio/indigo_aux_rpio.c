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

/** INDIGO Praspberry Pi GPIO AUX driver
 \file indigo_aux_rpio.c
 */

#include "indigo_aux_rpio.h"

#define DRIVER_VERSION         0x0001
#define AUX_DRAGONFLY_NAME     "Raspberry Pi GPIO"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_aux_driver.h>

#define PRIVATE_DATA                      ((rpio_private_data *)device->private_data)

#define AUX_RELAYS_GROUP	"Pin Control"

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

#define AUX_SENSORS_GROUP	"Inputs"

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
	int count_open;
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
} rpio_private_data;

typedef struct {
	indigo_device *device;
	rpio_private_data *private_data;
} rpio_device_data;

static rpio_device_data device_data = {0};

static void create_device();
static void delete_device();


static int input_pins[]  = { 4,17,27,22,23,24,25, 3};
static int output_pins[] = {18,12,13,26,16, 5, 6, 2};


static bool rpio_export(int pin) {
	char buffer[10];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open export for writing!");
		return false;
	}

	bytes_written = snprintf(buffer, 10, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return true;
}


static bool rpio_unexport(int pin) {
	char buffer[10];
	ssize_t bytes;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open unexport for writing!");
		return false;
	}

	bytes = snprintf(buffer, 10, "%d", pin);
	write(fd, buffer, bytes);
	close(fd);
	return true;
}


static bool rpio_set_input(int pin) {
	char path[256];
	int fd;

	sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio direction for writing!");
		return false;
	}

	if (write(fd, "in", 2) < 0) {
		fprintf(stderr, "Failed to set direction!\n");
		return false;
	}

	close(fd);
	return true;
}


static bool rpio_set_output(int pin) {
	char path[256];
	int fd;

	sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio direction for writing!");
		return false;
	}

	if (write(fd, "out", 3) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set direction!");
		return false;
	}

	close(fd);
	return true;
}


static int rpio_read(int pin, int *value) {
	char path[255];
	char value_str[3];
	int fd;

	if (value == NULL) return false;

	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio value for reading!");
		return false;
	}

	if (read(fd, value_str, 3) < 0) {
		fprintf(stderr, "Failed to read value!\n");
		return false;
	}
	close(fd);
	*value = atoi(value_str);
	return true;
}


static bool rpio_write(int pin, int value) {
	char path[255];
	int fd;

	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio value for writing!");
		return false;
	}

	char val = value ? '0' : '1';

	if (write(fd, &val, 1) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write value!");
		return false;
	}

	close(fd);
	return true;
}

static bool rpio_set_output_line(uint16_t line, int value) {
	if (line >= 8) return false;
	return rpio_write(output_pins[line], value);
}

static bool rpio_read_input_line(uint16_t line, int *value) {
	if (line >= 8) return false;
	return rpio_read(input_pins[line], value);
}

static bool rpio_read_input_lines(int *values) {
	for (int i = 0; i < 8; i++) {
		if (!rpio_read(input_pins[i], &values[i])) return false;
	}
	return true;
}


bool rpio_export_all() {
	for (int i = 0; i < 8; i++) {
		if (!rpio_export(output_pins[i])) return false;
		if (!rpio_export(input_pins[i])) return false;
	}
	return true;
}

bool rpio_unexport_all() {
	for (int i = 0; i < 8; i++) {
		if (!rpio_unexport(output_pins[i])) return false;
		if (!rpio_unexport(input_pins[i])) return false;
	}
	return true;
}

// --------------------------------------------------------------------------------- INDIGO AUX RELAYS device implementation

static int rpio_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = true;
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 5;
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
	return INDIGO_OK;
}


static void sensors_timer_callback(indigo_device *device) {
	//int sensor_value;
	//bool success;
	int sensors[8];

	if (!rpio_read_input_lines(sensors)) {
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		for (int i = 0; i < 8; i++) {
			(AUX_GPIO_SENSORS_PROPERTY->items + i)->number.value = (double)sensors[i];
		}
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->sensors_timer);
}


static void relay_1_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[0] = false;
	rpio_set_output_line(0, 0);
	AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_2_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[1] = false;
	rpio_set_output_line(1, 0);
	AUX_GPIO_OUTLET_2_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_3_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[2] = false;
	rpio_set_output_line(2, 0);
	AUX_GPIO_OUTLET_3_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_4_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[3] = false;
	rpio_set_output_line(3, 0);
	AUX_GPIO_OUTLET_4_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_5_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[4] = false;
	rpio_set_output_line(4, 0);
	AUX_GPIO_OUTLET_5_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_6_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[5] = false;
	rpio_set_output_line(5, 0);
	AUX_GPIO_OUTLET_6_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_7_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[6] = false;
	rpio_set_output_line(6, 0);
	AUX_GPIO_OUTLET_7_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_8_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[7] = false;
	rpio_set_output_line(7, 0);
	AUX_GPIO_OUTLET_8_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}


static void (*relay_timer_callbacks[])(indigo_device*) = {
	relay_1_timer_callback,
	relay_2_timer_callback,
	relay_3_timer_callback,
	relay_4_timer_callback,
	relay_5_timer_callback,
	relay_6_timer_callback,
	relay_7_timer_callback,
	relay_8_timer_callback
};


static bool set_gpio_outlets(indigo_device *device) {
	bool success = true;
	int relay_value[8];

	if (!rpio_read_input_lines(relay_value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "rpio_read(%d) failed", PRIVATE_DATA->handle);
		return false;
	}

	for (int i = 0; i < 8; i++) {
		if ((AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value != relay_value[i]) {
			if (((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value > 0) && (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !PRIVATE_DATA->relay_active[i]) {
				if (!rpio_set_output_line(i, (int)(AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "rpio_write(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				} else {
					PRIVATE_DATA->relay_active[i] = true;
					indigo_set_timer(device, ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value+20)/1000.0, relay_timer_callbacks[i], &PRIVATE_DATA->relay_timers[i]);
				}
			} else if ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value == 0 || (!(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !PRIVATE_DATA->relay_active[i])) {
				if (!rpio_set_output_line(i, (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "rpio_write(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				}
			}
		}
	}
	return success;
}


static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
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
	if (indigo_aux_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->relay_mutex, NULL);
		// --------------------------------------------------------------------------------
		if (rpio_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_aux_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!IS_CONNECTED) {
			if (rpio_export_all()) {
				char board[INDIGO_VALUE_SIZE] = "N/A";
				char firmware[INDIGO_VALUE_SIZE] = "N/A";
				strncpy(INFO_DEVICE_MODEL_ITEM->text.value, board, INDIGO_VALUE_SIZE);
				strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				int relay_value[8];
				if (!rpio_read(1, &relay_value[1])) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "rpio_read(%d) failed", PRIVATE_DATA->handle);
					AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					for (int i = 0; i < 8; i++) {
						(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value = relay_value[i];
						PRIVATE_DATA->relay_active[i] = false;
					}
				}
				indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
				indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
				indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
				indigo_set_timer(device, 0, sensors_timer_callback, &PRIVATE_DATA->sensors_timer);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
			}
		}
	} else {
		if (IS_CONNECTED) {
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			for (int i = 0; i < 8; i++) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->relay_timers[i]);
			}
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->sensors_timer);
			rpio_unexport_all();
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
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
		if (IS_CONNECTED) {
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
		if (IS_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(AUX_GPIO_OUTLET_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;

		if (set_gpio_outlets(device) == true) {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		} else {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, "Relay operation failed, did you authorize?");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
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
		snprintf(AUX_GPIO_SENSOR_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_5_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_6_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_7_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_8_ITEM->text.value);
		AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
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
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_aux_connect_property(device);
	}
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

//static int device_number = 0;

static void create_device() {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_DRAGONFLY_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	if (device_data.device != NULL) return;

	if (device_data.private_data == NULL) {
		device_data.private_data = malloc(sizeof(rpio_private_data));
		assert(device_data.private_data != NULL);
		memset(device_data.private_data, 0, sizeof(rpio_private_data));
		pthread_mutex_init(&device_data.private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	device_data.device = malloc(sizeof(indigo_device));
	assert(device_data.device != NULL);

	memcpy(device_data.device, &aux_template, sizeof(indigo_device));
	sprintf(device_data.device->name, "%s", AUX_DRAGONFLY_NAME);

	device_data.device->private_data = device_data.private_data;
	indigo_attach_device(device_data.device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: Device.");
}

static void delete_device() {
	if (device_data.device != NULL) {
		indigo_detach_device(device_data.device);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: Device.");
		free(device_data.device);
		device_data.device = NULL;
	}

	if (device_data.private_data != NULL) {
		free(device_data.private_data);
		device_data.private_data = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: PRIVATE_DATA");
	}
}


indigo_result indigo_aux_rpio(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		create_device();
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(device_data.device);
		last_action = action;
		delete_device();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
