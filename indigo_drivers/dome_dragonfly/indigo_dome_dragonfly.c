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

/** INDIGO Lunatico Dragonfly dome driver
 \file indigo_dome_dragonfly.c
 */

#include "indigo_dome_dragonfly.h"

#define DRIVER_VERSION         0x0005

#define DOME_DRAGONFLY_NAME    "Dome Dragonfly"
#define AUX_DRAGONFLY_NAME     "Dragonfly Controller"

#define DRIVER_NAME              "indigo_dome_dragonfly"
#define CONFLICTING_DRIVER       "indigo_aux_dragonfly"
#define DRIVER_INFO              "Lunatico Dragonfly Dome"


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

#define KEEP_ALIVE_INTERVAL   10
#define DOME_SENSORS_INTERVAL 1
#define AUX_SENSORS_INTERVAL  1

#define MAX_LOGICAL_DEVICES  2
#define MAX_PHYSICAL_DEVICES 4

#define PRIVATE_DATA                      ((lunatico_private_data *)device->private_data)
#define DEVICE_DATA                       (PRIVATE_DATA->device_data[get_locical_device_index(device)])

#define AUX_RELAYS_GROUP	"Relay control"

#define AUX_OUTLET_NAMES_PROPERTY      (DEVICE_DATA.outlet_names_property)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_5_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_6_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_7_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_OUTLET_NAME_8_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 4)

#define AUX_GPIO_OUTLET_PROPERTY      (DEVICE_DATA.gpio_outlet_property)
#define AUX_GPIO_OUTLET_4_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_5_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 1)
#define AUX_GPIO_OUTLET_6_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 2)
#define AUX_GPIO_OUTLET_7_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 3)
#define AUX_GPIO_OUTLET_8_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 4)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY      (DEVICE_DATA.gpio_outlet_pulse_property)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_5_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_6_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_7_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)
#define AUX_OUTLET_PULSE_LENGTHS_8_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 4)

#define AUX_SENSORS_GROUP	"Sensors"

#define AUX_SENSOR_NAMES_PROPERTY      (DEVICE_DATA.sensor_names_property)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_5_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_6_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)
#define AUX_SENSOR_NAME_7_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 4)

#define AUX_GPIO_SENSORS_PROPERTY     (DEVICE_DATA.sensors_property)
#define AUX_GPIO_SENSOR_3_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_4_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_5_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_6_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 3)
#define AUX_GPIO_SENSOR_7_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 4)

#define LA_DOME_SETTINGS_PROPERTY                     (DEVICE_DATA.dome_settings_property)
#define LA_DOME_SETTINGS_BUTTON_PULSE_ITEM            (LA_DOME_SETTINGS_PROPERTY->items + 0)
#define LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM      (LA_DOME_SETTINGS_PROPERTY->items + 1)
#define LA_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM      (LA_DOME_SETTINGS_PROPERTY->items + 2)
#define LA_DOME_SETTINGS_PARK_THRESHOLD_ITEM          (LA_DOME_SETTINGS_PROPERTY->items + 3)

#define LA_DOME_SETTINGS_PROPERTY_NAME                "LA_DOME_SETTINGS"
#define LA_DOME_SETTINGS_BUTTON_PULSE_ITEM_NAME       "BUTTON_PULSE_LENGTH"
#define LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM_NAME "READ_SENSORS_DELAY"
#define LA_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM_NAME "OPEN_CLOSE_TIMEOUT"
#define LA_DOME_SETTINGS_PARK_THRESHOLD_ITEM_NAME     "PARK_SENSOR_THRESHOLD"

#define LA_DOME_FUNCTION_PROPERTY                     (DEVICE_DATA.dome_function_property)
#define LA_DOME_FUNCTION_1_BUTTON_ITEM                (LA_DOME_FUNCTION_PROPERTY->items + 0)
#define LA_DOME_FUNCTION_2_BUTTONS_ITEM               (LA_DOME_FUNCTION_PROPERTY->items + 1)
#define LA_DOME_FUNCTION_3_BUTTONS_ITEM               (LA_DOME_FUNCTION_PROPERTY->items + 2)

#define LA_DOME_FUNCTION_PROPERTY_NAME                "LA_DOME_BUTTON_FUNCTION"
#define LA_DOME_FUNCTION_1_BUTTON_ITEM_NAME           "1_BUTTON_PUSH"
#define LA_DOME_FUNCTION_2_BUTTONS_ITEM_NAME          "2_BUTTONS_PUSH_HOLD"
#define LA_DOME_FUNCTION_3_BUTTONS_ITEM_NAME          "3_BUTTONS_PUSH"

typedef enum {
	TYPE_DOME    = 0,
	TYPE_AUX     = 1
} device_type_t;

typedef enum {
	OPENED_SENSOR = 0,
	CLOSED_SENSOR = 1,
	PARK_SENSOR = 7
} sensors_assignment_t;

typedef enum {
	STOP_RELAY = 0,
	OPEN_CLOSE_RELAY = 0,
	OPEN_RELAY = 1,
	CLOSE_RELAY = 2
} relay_assignment_t;

typedef enum {
	ROOF_OPENED = 1,
	ROOF_CLOSED,
	ROOF_OPENING,
	ROOF_STOPPED_WHILE_OPENING,
	ROOF_CLOSING,
	ROOF_STOPPED_WHILE_CLOSING,
	ROOF_OPENING_OR_CLOSING,
	ROOF_UNKNOWN
} roof_state_t;

typedef struct {
	device_type_t device_type;

	bool relay_active[5];
	indigo_timer *relay_timers[5];
	pthread_mutex_t relay_mutex;
	indigo_timer *sensors_timer;

	roof_state_t roof_state;
	indigo_timer *roof_timer;
	indigo_timer *keep_alive_timer;
	uint32_t roof_timer_hits;

	indigo_property *outlet_names_property,
	                *gpio_outlet_property,
	                *gpio_outlet_pulse_property,
	                *sensor_names_property,
	                *sensors_property,
	                *dome_settings_property,
	                *dome_function_property;
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

static void create_port_device(int p_device_index, int l_device_index, device_type_t type);
static void delete_port_device(int p_device_index, int l_device_index);

#include "../aux_dragonfly/shared/dragonfly_shared.c"

// --------------------------------------------------------------------------------- Common

static int lunatico_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- AUTHENTICATION
	AUTHENTICATION_PROPERTY->hidden = false;
	AUTHENTICATION_PROPERTY->count = 1;
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_copy_value(DEVICE_PORT_ITEM->text.value, "udp://dragonfly");
	indigo_copy_value(DEVICE_PORT_ITEM->label, "Devce URL");
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 6;
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay names", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_GPIO_OUTLET_NAME_4_ITEM_NAME, "Relay 4", "Relay #4");
	indigo_init_text_item(AUX_OUTLET_NAME_5_ITEM, AUX_GPIO_OUTLET_NAME_5_ITEM_NAME, "Relay 5", "Relay #5");
	indigo_init_text_item(AUX_OUTLET_NAME_6_ITEM, AUX_GPIO_OUTLET_NAME_6_ITEM_NAME, "Relay 6", "Relay #6");
	indigo_init_text_item(AUX_OUTLET_NAME_7_ITEM, AUX_GPIO_OUTLET_NAME_7_ITEM_NAME, "Relay 7", "Relay #7");
	indigo_init_text_item(AUX_OUTLET_NAME_8_ITEM, AUX_GPIO_OUTLET_NAME_8_ITEM_NAME, "Relay 8", "Relay #8");
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_OUTLET_NAMES_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- GPIO OUTLETS
	AUX_GPIO_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 5);
	if (AUX_GPIO_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_GPIO_OUTLET_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", false);
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_GPIO_OUTLET_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- GPIO PULSE OUTLETS
	AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, "AUX_OUTLET_PULSE_LENGTHS", AUX_RELAYS_GROUP, "Relay pulse lengths (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
	if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", 0, 100000, 100, 0);
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_OUTLET_PULSE_LENGTHS_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- SENSOR_NAMES
	AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
	if (AUX_SENSOR_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor 3", "Sensor #3");
	indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor 4", "Sensor #4");
	indigo_init_text_item(AUX_SENSOR_NAME_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor 5", "Sensor #5");
	indigo_init_text_item(AUX_SENSOR_NAME_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor 6", "Sensor #6");
	indigo_init_text_item(AUX_SENSOR_NAME_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor 7", "Sensor #7");
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_SENSOR_NAMES_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- GPIO_SENSORS
	AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 5);
	if (AUX_GPIO_SENSORS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor #5", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor #6", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor #7", 0, 1024, 1, 0);
	if (DEVICE_DATA.device_type != TYPE_AUX) AUX_GPIO_SENSORS_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- LA_DOME_SETTINGS
	LA_DOME_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, LA_DOME_SETTINGS_PROPERTY_NAME, "Settings", "Dome Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (LA_DOME_SETTINGS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(LA_DOME_SETTINGS_BUTTON_PULSE_ITEM, LA_DOME_SETTINGS_BUTTON_PULSE_ITEM_NAME, "Open/Close push duration (sec)", 0, 3, 0.5, 0.5);
	indigo_init_number_item(LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM, LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM_NAME, "Read sensors delay after push (sec)", 0, 6, 0.5, 2.5);
	indigo_init_number_item(LA_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM, LA_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM_NAME, "Open/Close tumeout (sec)", 0, 300, 1, 60);
	indigo_init_number_item(LA_DOME_SETTINGS_PARK_THRESHOLD_ITEM, LA_DOME_SETTINGS_PARK_THRESHOLD_ITEM_NAME, "Mount park sensor threshold", 0, 1024, 1, 512);
	if (DEVICE_DATA.device_type != TYPE_DOME) LA_DOME_SETTINGS_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- LA_DOME_FUNCTION
	LA_DOME_FUNCTION_PROPERTY = indigo_init_switch_property(NULL, device->name, LA_DOME_FUNCTION_PROPERTY_NAME, "Settings", "Buttons Function Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	if (LA_DOME_FUNCTION_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(LA_DOME_FUNCTION_1_BUTTON_ITEM, LA_DOME_FUNCTION_1_BUTTON_ITEM_NAME, "1 Button, push (relay #1-open/close/stop)", true);
	indigo_init_switch_item(LA_DOME_FUNCTION_2_BUTTONS_ITEM, LA_DOME_FUNCTION_2_BUTTONS_ITEM_NAME, "2 Buttons, push and hold (relays: #2-open, #3-close)", false);
	indigo_init_switch_item(LA_DOME_FUNCTION_3_BUTTONS_ITEM, LA_DOME_FUNCTION_3_BUTTONS_ITEM_NAME, "3 Buttons, push (relays: #1-stop, #2-open, #3-close)", false);
	if (DEVICE_DATA.device_type != TYPE_DOME) LA_DOME_FUNCTION_PROPERTY->hidden = true;
	//---------------------------------------------------------------------------
	return INDIGO_OK;
}


static indigo_result lunatico_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (DEVICE_CONNECTED) {
		if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property))
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		if (indigo_property_match(AUX_GPIO_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	if (indigo_property_match(LA_DOME_SETTINGS_PROPERTY, property))
		indigo_define_property(device, LA_DOME_SETTINGS_PROPERTY, NULL);
	if (indigo_property_match(LA_DOME_FUNCTION_PROPERTY, property))
		indigo_define_property(device, LA_DOME_FUNCTION_PROPERTY, NULL);
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}


static indigo_result lunatico_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AUX_GPIO_OUTLET_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);

	indigo_delete_property(device, LA_DOME_SETTINGS_PROPERTY, NULL);
	indigo_release_property(LA_DOME_SETTINGS_PROPERTY);

	indigo_delete_property(device, LA_DOME_FUNCTION_PROPERTY, NULL);
	indigo_release_property(LA_DOME_FUNCTION_PROPERTY);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	indigo_delete_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return INDIGO_OK;
}


static void lunatico_save_properties(indigo_device *device) {
	indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
	indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
	indigo_save_property(device, NULL, LA_DOME_SETTINGS_PROPERTY);
	indigo_save_property(device, NULL, LA_DOME_FUNCTION_PROPERTY);
}


static void lunatico_delete_properties(indigo_device *device) {
	indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
	indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
}


static indigo_result lunatico_common_update_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION_PROPERTY
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		if (AUTHENTICATION_PASSWORD_ITEM->text.value[0] == 0) {
			lunatico_authenticate2(device, NULL);
		} else {
			lunatico_authenticate2(device, AUTHENTICATION_PASSWORD_ITEM->text.value);
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			lunatico_save_properties(device);
		}
	}
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------- INDIGO AUX RELAYS device implementation

static void sensors_timer_callback(indigo_device *device) {
	int sensors[8];

	if (!lunatico_analog_read_sensors(device, sensors)) {
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		for (int i = 0; i < 5; i++) {
			(AUX_GPIO_SENSORS_PROPERTY->items + i)->number.value = (double)sensors[i + 2];
		}
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	indigo_reschedule_timer(device, AUX_SENSORS_INTERVAL, &DEVICE_DATA.sensors_timer);
}

static void relay_4_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[0] = false;
	AUX_GPIO_OUTLET_4_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_5_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[1] = false;
	AUX_GPIO_OUTLET_5_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_6_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[2] = false;
	AUX_GPIO_OUTLET_6_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_7_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[3] = false;
	AUX_GPIO_OUTLET_7_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}

static void relay_8_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	DEVICE_DATA.relay_active[4] = false;
	AUX_GPIO_OUTLET_8_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}


static void (*relay_timer_callbacks[])(indigo_device*) = {
	relay_4_timer_callback,
	relay_5_timer_callback,
	relay_6_timer_callback,
	relay_7_timer_callback,
	relay_8_timer_callback,
};


static bool set_gpio_outlets(indigo_device *device) {
	bool success = true;
	bool relay_value[8];
	if (!lunatico_read_relays(device, relay_value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_read_relays(%d) failed", PRIVATE_DATA->handle);
		return false;
	}

	for (int i = 0; i < 5; i++) {
		if ((AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value != relay_value[i + 3]) {
			if (((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value > 0) && (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !DEVICE_DATA.relay_active[i]) {
				if (!lunatico_pulse_relay(device, i + 3, (int)(AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_pulse_relay(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				} else {
					DEVICE_DATA.relay_active[i] = true;
					indigo_set_timer(device, ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value+20)/1000.0, relay_timer_callbacks[i], &DEVICE_DATA.relay_timers[i]);
				}
			} else if ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value == 0 || (!(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !DEVICE_DATA.relay_active[i])) {
				if (!lunatico_set_relay(device, i + 3, (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "lunatico_set_relay(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				}
			}
		}
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
						for (int i = 0; i < 5; i++) {
							(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value = relay_value[i + 3];
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
			for (int i = 0; i < 5; i++) {
				indigo_cancel_timer_sync(device, &DEVICE_DATA.relay_timers[i]);
			}
			indigo_cancel_timer_sync(device, &DEVICE_DATA.sensors_timer);
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
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_5_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_6_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_7_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_8_ITEM->text.value);

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
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_SENSOR_NAMES
		indigo_property_copy_values(AUX_SENSOR_NAMES_PROPERTY, property, false);
		if (DEVICE_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_SENSOR_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_3_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_4_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_5_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_6_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_7_ITEM->text.value);
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

// -------------------------------------------------------------------------------- INDIGO dome device implementation

static void keep_alive_timer_callback(indigo_device *device) {
	// Authentication is kept for 30 sec after last command -> send "echo" to preserve it.
	if (lunatico_keep_alive(device)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Keep Alive!");
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Keep Alive falied!");
	}
	indigo_reschedule_timer(device, KEEP_ALIVE_INTERVAL, &DEVICE_DATA.keep_alive_timer);
}


static void dome_timer_callback(indigo_device *device) {
	int sensors[8];

	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);

	// time tick 1sec
	DEVICE_DATA.roof_timer_hits ++;

	// Timed out
	if (DEVICE_DATA.roof_timer_hits > LA_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM->number.value) {
		// Nomatter which setup is used it is safe to turn off the relays
		lunatico_set_relay(device, OPEN_RELAY, false);
		lunatico_set_relay(device, CLOSE_RELAY, false);
		DEVICE_DATA.roof_timer_hits = 0;
		DEVICE_DATA.roof_state = ROOF_UNKNOWN;
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_SHUTTER_OPENED_ITEM->sw.value = false;
		DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
		pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Open / Close timed out.");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Open / Close timed out.");
		return;
	}

	if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
		pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
		return;
	}

	// Check if end sensor reached
	if (lunatico_analog_read_sensors(device, sensors)) {
		bool opened = (sensors[OPENED_SENSOR] > 512) ? true : false;
		bool closed = (sensors[CLOSED_SENSOR] > 512) ? true : false;
		if (opened || closed) {
			// Nomatter which setup is used it is safe to turn off the relays
			lunatico_set_relay(device, OPEN_RELAY, false);
			lunatico_set_relay(device, CLOSE_RELAY, false);
		}
		if (opened && !closed) {
			DEVICE_DATA.roof_timer_hits = 0;
			DEVICE_DATA.roof_state = ROOF_OPENED;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_OPENED_ITEM->sw.value = true;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is open.");
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is open.");
			return;
		} else if(!opened && closed) {
			DEVICE_DATA.roof_timer_hits = 0;
			DEVICE_DATA.roof_state = ROOF_CLOSED;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = true;
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is closed.");
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is closed.");
			return;
		} else if (opened && closed) {
			DEVICE_DATA.roof_timer_hits = 0;
			DEVICE_DATA.roof_state = ROOF_UNKNOWN;
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof shows qantum properties, it is both opened and closed.");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Roof shows qantum properties, it is both opened and closed.");
			return;
		}
	}
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
	indigo_reschedule_timer(device, DOME_SENSORS_INTERVAL, &DEVICE_DATA.roof_timer);
}


static void dome_handle_abort(indigo_device *device) {
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);

	pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
	if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
		pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Attempting Abort...");

	if (DEVICE_DATA.roof_state == ROOF_OPENING || DEVICE_DATA.roof_state == ROOF_CLOSING || DEVICE_DATA.roof_state == ROOF_OPENING_OR_CLOSING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Aborting...");
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		// Does not hurt to turn off OPEN and CLOSE relays in all situations
		if (lunatico_set_relay(device, OPEN_RELAY, false)) {
			lunatico_set_relay(device, CLOSE_RELAY, false);
			// In push and release setups push stop button
			if (!LA_DOME_FUNCTION_2_BUTTONS_ITEM->sw.value) {
				lunatico_set_relay(device, STOP_RELAY, true);
			}
			if (DEVICE_DATA.roof_state == ROOF_CLOSING) {
				DEVICE_DATA.roof_state = ROOF_STOPPED_WHILE_CLOSING;
			} else {
				DEVICE_DATA.roof_state = ROOF_STOPPED_WHILE_OPENING;
			}
			DEVICE_DATA.roof_timer_hits = 0;
			// In push and release setups wait for the button to be released
			if (!LA_DOME_FUNCTION_2_BUTTONS_ITEM->sw.value) {
				indigo_usleep((int)LA_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value * 1000000);
				lunatico_set_relay(device, STOP_RELAY, false);
			}
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof Stopped.");
		} else {
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not stop the roof, did you authorize?");
		}
		pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
		return;
	}
	pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
}


static void dome_handle_shutter(indigo_device *device) {

	if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		return;
	}

	int sensors[8] = {0};
	bool parked = false;
	if (lunatico_analog_read_sensors(device, sensors) && (sensors[PARK_SENSOR] > (int)LA_DOME_SETTINGS_PARK_THRESHOLD_ITEM->number.value)) {
		parked = true;
		bool opened = (sensors[OPENED_SENSOR] > 512) ? true : false;
		bool closed = (sensors[CLOSED_SENSOR] > 512) ? true : false;
		/* The roof seems to be moved with the buttons,
		   if not fully open or closed the driver will not control it.
		*/
		if (opened && DEVICE_DATA.roof_state != ROOF_OPENED) {
			DEVICE_DATA.roof_state = ROOF_OPENED;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof open, control regained.");
		} else if (closed && DEVICE_DATA.roof_state != ROOF_CLOSED) {
			DEVICE_DATA.roof_state = ROOF_CLOSED;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof closed, control regained.");
		} else if ((!closed && DEVICE_DATA.roof_state == ROOF_CLOSED) || (!opened && DEVICE_DATA.roof_state == ROOF_OPENED)) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(
				device,
				DOME_SHUTTER_PROPERTY,
				"The roof seems to be moved by hand, the driver will regain control when opened or closed position is reached."
			);
			INDIGO_DRIVER_DEBUG(
				DRIVER_NAME,
				"Contol lost. The roof seems to be moved by hand, the driver will regain control when opened or closed position is reached."
			);
			return;
		}
	}

	// Do not move if the state is the requested one.
	if ((DEVICE_DATA.roof_state == ROOF_OPENED && DOME_SHUTTER_OPENED_ITEM->sw.value == true) ||
		(DEVICE_DATA.roof_state == ROOF_CLOSED && DOME_SHUTTER_CLOSED_ITEM->sw.value == true) ||
		(DOME_SHUTTER_CLOSED_ITEM->sw.value == false && DOME_SHUTTER_OPENED_ITEM->sw.value == false)) {
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Will not process change. Reason OPENED_ITEM = %d, CLOSED_ITEM = %d, roof_state = %d",
			DOME_SHUTTER_OPENED_ITEM->sw.value,
			DOME_SHUTTER_CLOSED_ITEM->sw.value,
			DEVICE_DATA.roof_state
		);
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
		return;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PARKED = %d (%d > %d)?", parked, sensors[PARK_SENSOR], (int)LA_DOME_SETTINGS_PARK_THRESHOLD_ITEM->number.value);

	// Do not move when not parked!
	if (!parked) {
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		if (DEVICE_DATA.roof_state == ROOF_OPENED)
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		else if (DEVICE_DATA.roof_state == ROOF_CLOSED)
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
		else {
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
		}
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not move the roof, mount is not parked!");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not move the roof, mount is not parked!");
		return;
	}

	if (LA_DOME_FUNCTION_1_BUTTON_ITEM->sw.value) {
		// Close roof
		if (DEVICE_DATA.roof_state == ROOF_OPENED && DOME_SHUTTER_CLOSED_ITEM->sw.value == true) {
			pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
			if (lunatico_set_relay(device, OPEN_CLOSE_RELAY, true)) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is closing...");
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is closing...");
				DEVICE_DATA.roof_state = ROOF_CLOSING;
				DEVICE_DATA.roof_timer_hits = 0;
				indigo_usleep((int)LA_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value * 1000000);
				lunatico_set_relay(device, OPEN_CLOSE_RELAY, false);
				indigo_set_timer(device, LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM->number.value, dome_timer_callback, &DEVICE_DATA.roof_timer);
			} else {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not close the roof, did you authorize?");
			}
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			return;
		}

		// Open roof
		if (DEVICE_DATA.roof_state == ROOF_CLOSED && DOME_SHUTTER_OPENED_ITEM->sw.value == true) {
			pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
			if (lunatico_set_relay(device, OPEN_CLOSE_RELAY, true)) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is opening...");
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is opening...");
				DEVICE_DATA.roof_state = ROOF_OPENING;
				DEVICE_DATA.roof_timer_hits = 0;
				indigo_usleep((int)LA_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value * 1000000);
				lunatico_set_relay(device, OPEN_CLOSE_RELAY, false);
				indigo_set_timer(device, LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM->number.value, dome_timer_callback, &DEVICE_DATA.roof_timer);
			} else {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not close the roof, did you authorize?");
			}
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			return;
		}

		// State unknown! Go somewhere and see where it will end up...
		pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
		if (lunatico_set_relay(device, OPEN_CLOSE_RELAY, true)) {
			DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is either opening or closing...");
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is either opening or closing...");
			DEVICE_DATA.roof_state = ROOF_OPENING_OR_CLOSING;
			DEVICE_DATA.roof_timer_hits = 0;
			indigo_usleep((int)LA_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value * 1000000);
			lunatico_set_relay(device, OPEN_CLOSE_RELAY, false);
			indigo_set_timer(device, LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM->number.value, dome_timer_callback, &DEVICE_DATA.roof_timer);
		} else {
			DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not move the roof, did you authorize?");
		}
		pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);

	} else if (LA_DOME_FUNCTION_2_BUTTONS_ITEM->sw.value || LA_DOME_FUNCTION_3_BUTTONS_ITEM->sw.value) {
		// Close roof
		if (DEVICE_DATA.roof_state != ROOF_CLOSED && DOME_SHUTTER_CLOSED_ITEM->sw.value == true) {
			pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
			if (lunatico_set_relay(device, CLOSE_RELAY, true)) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is closing...");
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is closing...");
				DEVICE_DATA.roof_state = ROOF_CLOSING;
				DEVICE_DATA.roof_timer_hits = 0;
				indigo_usleep((int)LA_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value * 1000000);
				if (LA_DOME_FUNCTION_3_BUTTONS_ITEM->sw.value) {
					lunatico_set_relay(device, CLOSE_RELAY, false);
				}
				indigo_set_timer(device, LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM->number.value, dome_timer_callback, &DEVICE_DATA.roof_timer);
			} else {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not close the roof, did you authorize?");
			}
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			return;
		}
		// Open roof
		if (DEVICE_DATA.roof_state != ROOF_OPENED && DOME_SHUTTER_OPENED_ITEM->sw.value == true) {
			pthread_mutex_lock(&DEVICE_DATA.relay_mutex);
			if (lunatico_set_relay(device, OPEN_RELAY, true)) {
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Roof is opening...");
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Roof is opening...");
				DEVICE_DATA.roof_state = ROOF_OPENING;
				DEVICE_DATA.roof_timer_hits = 0;
				indigo_usleep((int)LA_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value * 1000000);
				if (LA_DOME_FUNCTION_3_BUTTONS_ITEM->sw.value) {
					lunatico_set_relay(device, OPEN_RELAY, false);
				}
				indigo_set_timer(device, LA_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM->number.value, dome_timer_callback, &DEVICE_DATA.roof_timer);
			} else {
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Can not open the roof, did you authorize?");
			}
			pthread_mutex_unlock(&DEVICE_DATA.relay_mutex);
			return;
		}
	}
}


static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	lunatico_enumerate_properties(device, client, property);
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}


static indigo_result dome_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&DEVICE_DATA.relay_mutex, NULL);

		DEVICE_PORT_PROPERTY->hidden = false;
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
		// Relabel Open / Close
		indigo_copy_value(DOME_SHUTTER_PROPERTY->label, "Shutter / Roof");
		indigo_copy_value(DOME_SHUTTER_OPENED_ITEM->label, "Shutter / Roof opened");
		indigo_copy_value(DOME_SHUTTER_CLOSED_ITEM->label, "Shutter / Roof closed");
		// --------------------------------------------------------------------------------
		if (lunatico_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_dome_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (lunatico_open(device)) {
				char board[LUNATICO_CMD_LEN] = "N/A";
				char firmware[LUNATICO_CMD_LEN] = "N/A";
				if (lunatico_get_info(device, board, firmware) && !strncmp(board, "Dragonfly", INDIGO_VALUE_SIZE)) {
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					lunatico_authenticate2(device, AUTHENTICATION_PASSWORD_ITEM->text.value);
					int sensors[8];
					DOME_SHUTTER_OPENED_ITEM->sw.value = false;
					DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
					DEVICE_DATA.roof_state = ROOF_UNKNOWN;
					DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
					if (lunatico_analog_read_sensors(device, sensors)) {
						bool opened = (sensors[OPENED_SENSOR] > 512) ? true : false;
						bool closed = (sensors[CLOSED_SENSOR] > 512) ? true : false;
						if (opened && !closed) {
							DOME_SHUTTER_OPENED_ITEM->sw.value = true;
							DEVICE_DATA.roof_state = ROOF_OPENED;
							DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
						} else if(!opened && closed) {
							DOME_SHUTTER_CLOSED_ITEM->sw.value = true;
							DEVICE_DATA.roof_state = ROOF_CLOSED;
							DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
						}
					}
					indigo_set_timer(device, KEEP_ALIVE_INTERVAL, keep_alive_timer_callback, &DEVICE_DATA.keep_alive_timer);
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
			indigo_cancel_timer_sync(device, &DEVICE_DATA.keep_alive_timer);
			indigo_cancel_timer_sync(device, &DEVICE_DATA.roof_timer);
			lunatico_delete_properties(device);
			lunatico_close(device);
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
		indigo_set_timer(device, 0, handle_dome_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
		indigo_property_copy_values(DOME_ABORT_MOTION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_set_timer(device, 0, dome_handle_abort, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DOME_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DOME_SHUTTER
		indigo_property_copy_values(DOME_SHUTTER_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_set_timer(device, 0, dome_handle_shutter, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_DOME_SETTINGS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_DOME_SETTINGS
		indigo_property_copy_values(LA_DOME_SETTINGS_PROPERTY, property, false);
		LA_DOME_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LA_DOME_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(LA_DOME_FUNCTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LA_DOME_FUNCTION
		indigo_property_copy_values(LA_DOME_FUNCTION_PROPERTY, property, false);
		LA_DOME_FUNCTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LA_DOME_FUNCTION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	lunatico_common_update_property(device, client, property);
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (DEVICE_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_dome_connect_property(device);
	}
	lunatico_detach(device);
	return indigo_dome_detach(device);
}

// --------------------------------------------------------------------------------

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
		dome_enumerate_properties,
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
		device_data[p_device_index].private_data = indigo_safe_malloc(sizeof(lunatico_private_data));
		pthread_mutex_init(&device_data[p_device_index].private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	if (device_type == TYPE_DOME) {
		device_data[p_device_index].device[l_device_index] = indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
		sprintf(device_data[p_device_index].device[l_device_index]->name, "%s", DOME_DRAGONFLY_NAME);
		device_data[p_device_index].private_data->device_data[l_device_index].device_type = TYPE_DOME;
	} else {
		device_data[p_device_index].device[l_device_index] = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
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


static bool at_least_one_device_connected() {
	for (int p_index = 0; p_index < MAX_PHYSICAL_DEVICES; p_index++) {
		for (int l_index = 0; l_index < MAX_LOGICAL_DEVICES; l_index++) {
			if (is_connected(device_data[p_index].device[l_index])) return true;
		}
	}
	return false;
}


indigo_result indigo_dome_dragonfly(indigo_driver_action action, indigo_driver_info *info) {

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
		create_port_device(0, 0, TYPE_DOME);
		create_port_device(0, 1, TYPE_AUX);
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
