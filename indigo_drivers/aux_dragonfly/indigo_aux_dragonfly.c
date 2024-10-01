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

/** INDIGO Lunatico Dragonfly AUX driver
 \file indigo_aux_dragonfly.c
 */

#include "indigo_aux_dragonfly.h"

#define DRIVER_VERSION         0x0004
#define AUX_DRAGONFLY_NAME     "Dragonfly Controller"

#define DRIVER_NAME              "indigo_aux_dragonfly"
#define CONFLICTING_DRIVER       "indigo_dome_dragonfly"
#define DRIVER_INFO              "Lunatico Dragonfly Relay Controller"


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
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_aux_driver.h>

#define DEFAULT_BAUDRATE            "115200"

#define ROTATOR_SPEED 1

#define MAX_LOGICAL_DEVICES  2
#define MAX_PHYSICAL_DEVICES 4

#define PRIVATE_DATA                      ((lunatico_private_data *)device->private_data)
#define DEVICE_DATA                       (PRIVATE_DATA->device_data[get_locical_device_index(device)])

#define AUX_RELAYS_GROUP	"Relay control"

#define AUX_OUTLET_NAMES_PROPERTY      (DEVICE_DATA.outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_2_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_3_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_OUTLET_NAME_5_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_OUTLET_NAME_6_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_OUTLET_NAME_7_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_OUTLET_NAME_8_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_OUTLET_PROPERTY      (DEVICE_DATA.gpio_outlet_property)
#define AUX_GPIO_OUTLET_1_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_2_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 1)
#define AUX_GPIO_OUTLET_3_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 2)
#define AUX_GPIO_OUTLET_4_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 3)
#define AUX_GPIO_OUTLET_5_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 4)
#define AUX_GPIO_OUTLET_6_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 5)
#define AUX_GPIO_OUTLET_7_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 6)
#define AUX_GPIO_OUTLET_8_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 7)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY      (DEVICE_DATA.gpio_outlet_pulse_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_2_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_3_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)
#define AUX_OUTLET_PULSE_LENGTHS_5_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 4)
#define AUX_OUTLET_PULSE_LENGTHS_6_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 5)
#define AUX_OUTLET_PULSE_LENGTHS_7_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 6)
#define AUX_OUTLET_PULSE_LENGTHS_8_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 7)

#define AUX_SENSORS_GROUP	"Sensors"

#define AUX_SENSOR_NAMES_PROPERTY      (DEVICE_DATA.sensor_names_property)
#define AUX_SENSOR_NAME_1_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_2_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)
#define AUX_SENSOR_NAME_5_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 4)
#define AUX_SENSOR_NAME_6_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 5)
#define AUX_SENSOR_NAME_7_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 6)
#define AUX_SENSOR_NAME_8_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_SENSORS_PROPERTY     (DEVICE_DATA.sensors_property)
#define AUX_GPIO_SENSOR_1_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_2_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_3_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_4_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 3)
#define AUX_GPIO_SENSOR_5_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 4)
#define AUX_GPIO_SENSOR_6_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 5)
#define AUX_GPIO_SENSOR_7_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 6)
#define AUX_GPIO_SENSOR_8_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 7)

typedef struct {
	bool relay_active[8];
	indigo_timer *relay_timers[8];
	pthread_mutex_t relay_mutex;
	indigo_timer *sensors_timer;

	indigo_property *outlet_names_property,
	                *gpio_outlet_property,
	                *gpio_outlet_pulse_property,
	                *sensor_names_property,
	                *sensors_property;
} logical_device_data;

typedef struct {
	int handle;
	int count_open;
	bool udp;
	pthread_mutex_t port_mutex;
	logical_device_data device_data[MAX_LOGICAL_DEVICES];
} lunatico_private_data;

typedef struct {
	indigo_device *device[MAX_LOGICAL_DEVICES];
	lunatico_private_data *private_data;
} lunatico_device_data;

static lunatico_device_data device_data[MAX_PHYSICAL_DEVICES] = {0};

static void create_port_device(int p_device_index, int l_device_index);
static void delete_port_device(int p_device_index, int l_device_index);


#include "shared/dragonfly_shared.c"

// --------------------------------------------------------------------------------- INDIGO AUX RELAYS device implementation

static int lunatico_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- AUTHENTICATION
	AUTHENTICATION_PROPERTY->hidden = false;
	AUTHENTICATION_PROPERTY->count = 1;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_copy_value(DEVICE_PORT_ITEM->text.value, "udp://dragonfly");
	indigo_copy_value(DEVICE_PORT_ITEM->label, "Devce URL");
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 6;
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

	if (!lunatico_analog_read_sensors(device, sensors)) {
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		for (int i = 0; i < 8; i++) {
			(AUX_GPIO_SENSORS_PROPERTY->items + i)->number.value = (double)sensors[i];
		}
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	indigo_reschedule_timer(device, 1, &DEVICE_DATA.sensors_timer);
}


static void relay_1_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[0] = false;
	AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_2_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[1] = false;
	AUX_GPIO_OUTLET_2_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_3_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[2] = false;
	AUX_GPIO_OUTLET_3_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_4_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[3] = false;
	AUX_GPIO_OUTLET_4_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_5_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[4] = false;
	AUX_GPIO_OUTLET_5_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_6_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[5] = false;
	AUX_GPIO_OUTLET_6_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_7_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[6] = false;
	AUX_GPIO_OUTLET_7_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_8_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[7] = false;
	AUX_GPIO_OUTLET_8_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
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
	bool relay_value[8];

	if (!lunatico_read_relays(device, relay_value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_relays(%d) failed", PRIVATE_DATA->handle);
		return false;
	}

	for (int i = 0; i < 8; i++) {
		if ((AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value != relay_value[i]) {
			if (((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value > 0) && (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !DEVICE_DATA.relay_active[i]) {
				if (!lunatico_pulse_relay(device, i, (int)(AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_pulse_relay(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				} else {
					DEVICE_DATA.relay_active[i] = true;
					indigo_set_timer(device, ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value+20)/1000.0, relay_timer_callbacks[i], &DEVICE_DATA.relay_timers[i]);
				}
			} else if ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value == 0 || (!(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !DEVICE_DATA.relay_active[i])) {
				if (!lunatico_set_relay(device, i, (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_relay(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				}
			}
		}
	}

	return success;
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
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		pthread_mutex_init(&DEVICE_DATA.relay_mutex, NULL);
		// --------------------------------------------------------------------------------
		if (lunatico_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void handle_aux_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (lunatico_open(device)) {
				char board[LUNATICO_CMD_LEN] = "N/A";
				char firmware[LUNATICO_CMD_LEN] = "N/A";
				if (lunatico_get_info(device, board, firmware) && !strncmp(board, "Dragonfly", INDIGO_VALUE_SIZE)) {
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					bool relay_value[8];
					if (!lunatico_read_relays(device, relay_value)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_relays(%d) failed", PRIVATE_DATA->handle);
						AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						for (int i = 0; i < 8; i++) {
							(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value = relay_value[i];
							DEVICE_DATA.relay_active[i] = false;
						}
					}
					indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
					indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
					indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
					lunatico_authenticate2(device, AUTHENTICATION_PASSWORD_ITEM->text.value);
					indigo_set_timer(device, 0, sensors_timer_callback, &DEVICE_DATA.sensors_timer);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
					lunatico_close(device);
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
			}
		}
	} else {
		if (DEVICE_CONNECTED) {
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			for (int i = 0; i < 8; i++) {
				indigo_cancel_timer_sync(device, &DEVICE_DATA.relay_timers[i]);
			}
			indigo_cancel_timer_sync(device, &DEVICE_DATA.sensors_timer);
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
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
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_aux_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
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
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(AUX_GPIO_OUTLET_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;

		if (set_gpio_outlets(device) == true) {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		} else {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, "Relay operation failed, did you authorize?");
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SENSOR_NAMES_PROPERTY, property)) {
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
	} else if (indigo_property_match_changeable(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION_PROPERTY
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		if (AUTHENTICATION_PASSWORD_ITEM->text.value[0] == 0) {
			lunatico_authenticate2(device, NULL);
		} else {
			lunatico_authenticate2(device, AUTHENTICATION_PASSWORD_ITEM->text.value);
		}
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
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
	if (DEVICE_CONNECTED) {
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

static void create_port_device(int p_device_index, int l_device_index) {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_DRAGONFLY_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	if (l_device_index >= MAX_LOGICAL_DEVICES) return;
	if (p_device_index >= MAX_PHYSICAL_DEVICES) return;
	if (device_data[p_device_index].device[l_device_index] != NULL) return;

	if (device_data[p_device_index].private_data == NULL) {
		device_data[p_device_index].private_data = indigo_safe_malloc(sizeof(lunatico_private_data));
		pthread_mutex_init(&device_data[p_device_index].private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	device_data[p_device_index].device[l_device_index] = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
	sprintf(device_data[p_device_index].device[l_device_index]->name, "%s", AUX_DRAGONFLY_NAME);

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


static bool at_least_one_device_connected() {
	for (int p_index = 0; p_index < MAX_PHYSICAL_DEVICES; p_index++) {
		for (int l_index = 0; l_index < MAX_LOGICAL_DEVICES; l_index++) {
			if (is_connected(device_data[p_index].device[l_index])) return true;
		}
	}
	return false;
}


indigo_result indigo_aux_dragonfly(indigo_driver_action action, indigo_driver_info *info) {

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
		create_port_device(0, 0);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		if (at_least_one_device_connected() == true) return INDIGO_BUSY;
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
