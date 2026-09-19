// Copyright (c) 2020-2026 Rumen G. Bogdanovski
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_dome_dragonfly.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <indigo/indigo_client.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_dragonfly.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_dome_dragonfly"
#define DRIVER_LABEL         "Lunatico Dragonfly Dome"
#define DOME_DEVICE_NAME     "Dome Dragonfly"
#define AUX_DEVICE_NAME      "Dragonfly Controller"
#define PRIVATE_DATA         ((dragonfly_private_data *)device->private_data)

//+ define

#include "../aux_dragonfly/shared/dragonfly_shared.h"

// Relays 1...3 and sensors 1, 2 and 8 drive the roof, so the relay
// device exposes relays 4...8 and sensors 3...7 only.
#define RELAY_FIRST          3
#define RELAY_COUNT          5
#define SENSOR_FIRST         2
#define SENSOR_COUNT         5

#define CONFLICTING_DRIVER   "indigo_aux_dragonfly"

// Roof wiring, see README.md. In the one button wiring relay 1 toggles
// the roof, in the three button wiring it stops it.
#define OPEN_CLOSE_RELAY     0
#define STOP_RELAY           0
#define OPEN_RELAY           1
#define CLOSE_RELAY          2
#define OPENED_SENSOR        0
#define CLOSED_SENSOR        1
#define PARK_SENSOR          7

// Authentication expires about 30 s after the last command.
#define KEEP_ALIVE_INTERVAL  10
// Sensor poll interval while the roof is moving.
#define ROOF_POLL_INTERVAL   1

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

//- define

#pragma mark - Property definitions

#define X_DOME_SETTINGS_PROPERTY                     (PRIVATE_DATA->x_dome_settings_property)
#define X_DOME_SETTINGS_BUTTON_PULSE_ITEM            (X_DOME_SETTINGS_PROPERTY->items + 0)
#define X_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM      (X_DOME_SETTINGS_PROPERTY->items + 1)
#define X_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM      (X_DOME_SETTINGS_PROPERTY->items + 2)
#define X_DOME_SETTINGS_PARK_THRESHOLD_ITEM          (X_DOME_SETTINGS_PROPERTY->items + 3)

#define X_DOME_SETTINGS_PROPERTY_NAME                "X_DOME_SETTINGS"
#define X_DOME_SETTINGS_BUTTON_PULSE_ITEM_NAME       "BUTTON_PULSE_LENGTH"
#define X_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM_NAME "READ_SENSORS_DELAY"
#define X_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM_NAME "OPEN_CLOSE_TIMEOUT"
#define X_DOME_SETTINGS_PARK_THRESHOLD_ITEM_NAME     "PARK_SENSOR_THRESHOLD"

#define X_DOME_BUTTON_FUNCTION_PROPERTY            (PRIVATE_DATA->x_dome_button_function_property)
#define X_DOME_BUTTON_FUNCTION_1_BUTTON_ITEM       (X_DOME_BUTTON_FUNCTION_PROPERTY->items + 0)
#define X_DOME_BUTTON_FUNCTION_2_BUTTONS_ITEM      (X_DOME_BUTTON_FUNCTION_PROPERTY->items + 1)
#define X_DOME_BUTTON_FUNCTION_3_BUTTONS_ITEM      (X_DOME_BUTTON_FUNCTION_PROPERTY->items + 2)

#define X_DOME_BUTTON_FUNCTION_PROPERTY_NAME       "X_DOME_BUTTON_FUNCTION"
#define X_DOME_BUTTON_FUNCTION_1_BUTTON_ITEM_NAME  "1_BUTTON_PUSH"
#define X_DOME_BUTTON_FUNCTION_2_BUTTONS_ITEM_NAME "2_BUTTONS_PUSH_HOLD"
#define X_DOME_BUTTON_FUNCTION_3_BUTTONS_ITEM_NAME "3_BUTTONS_PUSH"

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_5_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_6_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_7_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_OUTLET_NAME_8_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 4)

#define AUX_GPIO_OUTLETS_PROPERTY      (PRIVATE_DATA->aux_gpio_outlets_property)
#define AUX_GPIO_OUTLET_4_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_5_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 1)
#define AUX_GPIO_OUTLET_6_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 2)
#define AUX_GPIO_OUTLET_7_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 3)
#define AUX_GPIO_OUTLET_8_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 4)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY (PRIVATE_DATA->aux_outlet_pulse_lengths_property)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_5_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_6_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_7_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)
#define AUX_OUTLET_PULSE_LENGTHS_8_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 4)

#define AUX_SENSOR_NAMES_PROPERTY      (PRIVATE_DATA->aux_sensor_names_property)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_5_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_6_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)
#define AUX_SENSOR_NAME_7_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 4)

#define AUX_GPIO_SENSORS_PROPERTY      (PRIVATE_DATA->aux_gpio_sensors_property)
#define AUX_GPIO_SENSOR_3_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_4_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_5_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_6_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 3)
#define AUX_GPIO_SENSOR_7_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 4)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_dome_settings_property;
	indigo_property *x_dome_button_function_property;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_gpio_outlets_property;
	indigo_property *aux_outlet_pulse_lengths_property;
	indigo_property *aux_sensor_names_property;
	indigo_property *aux_gpio_sensors_property;
	//+ data
	char command[DRAGONFLY_CMD_LEN];
	char response[DRAGONFLY_CMD_LEN];
	char board[INDIGO_VALUE_SIZE];
	char firmware[INDIGO_VALUE_SIZE];
	double relay_pulse_until[DRAGONFLY_CHANNELS];
	roof_state_t roof_state;
	double roof_deadline;
	//- data
} dragonfly_private_data;

#pragma mark - Low level code

//+ code

#include "../aux_dragonfly/shared/dragonfly_shared.c"

//- code

//+ dome.code

static void dome_shutter_handler(indigo_device *device);
static void dome_shutter_finalizer(indigo_device *device);

// Seed the shutter from the end sensors when the session opens. A
// roof that is neither open nor closed is reported as unknown.
static void dome_seed_roof_state(indigo_device *device) {
	int sensors[DRAGONFLY_CHANNELS];
	DOME_SHUTTER_OPENED_ITEM->sw.value = false;
	DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
	PRIVATE_DATA->roof_state = ROOF_UNKNOWN;
	DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	if (dragonfly_read_sensors(device, sensors)) {
		bool opened = sensors[OPENED_SENSOR] > DRAGONFLY_SENSOR_THRESHOLD;
		bool closed = sensors[CLOSED_SENSOR] > DRAGONFLY_SENSOR_THRESHOLD;
		if (opened && !closed) {
			DOME_SHUTTER_OPENED_ITEM->sw.value = true;
			PRIVATE_DATA->roof_state = ROOF_OPENED;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		} else if (!opened && closed) {
			DOME_SHUTTER_CLOSED_ITEM->sw.value = true;
			PRIVATE_DATA->roof_state = ROOF_CLOSED;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
}

// Drive the configured wiring and publish the start of the motion.
// Returns true when the roof was actually started.
static bool dome_start_motion(indigo_device *device) {
	int relay = OPEN_CLOSE_RELAY;
	const char *message = NULL;
	roof_state_t state = ROOF_UNKNOWN;
	bool hold = false;
	if (X_DOME_BUTTON_FUNCTION_1_BUTTON_ITEM->sw.value) {
		if (PRIVATE_DATA->roof_state == ROOF_OPENED && DOME_SHUTTER_CLOSED_ITEM->sw.value) {
			state = ROOF_CLOSING;
			message = "Roof is closing...";
		} else if (PRIVATE_DATA->roof_state == ROOF_CLOSED && DOME_SHUTTER_OPENED_ITEM->sw.value) {
			state = ROOF_OPENING;
			message = "Roof is opening...";
		} else {
			// The roof position is unknown, so push the button and
			// find out where it ends up.
			state = ROOF_OPENING_OR_CLOSING;
			message = "Roof is either opening or closing...";
		}
	} else {
		// The push and hold wiring keeps the contact closed for the
		// whole travel, the three button wiring releases it.
		hold = X_DOME_BUTTON_FUNCTION_2_BUTTONS_ITEM->sw.value;
		if (PRIVATE_DATA->roof_state != ROOF_CLOSED && DOME_SHUTTER_CLOSED_ITEM->sw.value) {
			relay = CLOSE_RELAY;
			state = ROOF_CLOSING;
			message = "Roof is closing...";
		} else if (PRIVATE_DATA->roof_state != ROOF_OPENED && DOME_SHUTTER_OPENED_ITEM->sw.value) {
			relay = OPEN_RELAY;
			state = ROOF_OPENING;
			message = "Roof is opening...";
		} else {
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
			return false;
		}
	}
	if (!dragonfly_set_relay(device, relay, true)) {
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "Can not move the roof, did you authorize?");
		return false;
	}
	PRIVATE_DATA->roof_state = state;
	PRIVATE_DATA->roof_deadline = indigo_monotonic_time() + X_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM->number.value;
	INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_BUSY_STATE, message);
	// Momentary button emulation. The contact must be released as one
	// transaction, otherwise a cancelled or delayed release would
	// leave the button latched, so this bounded wait is deliberate.
	indigo_usleep(INDIGO_DELAY(X_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value));
	if (!hold) {
		dragonfly_set_relay(device, relay, false);
	}
	return true;
}

// Turning both direction relays off is safe in every wiring.
static void dome_release_direction_relays(indigo_device *device) {
	dragonfly_set_relay(device, OPEN_RELAY, false);
	dragonfly_set_relay(device, CLOSE_RELAY, false);
}

static void dome_shutter_finalizer(indigo_device *device) {
	int sensors[DRAGONFLY_CHANNELS];
	if (DOME_SHUTTER_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if (indigo_monotonic_time() > PRIVATE_DATA->roof_deadline) {
		dome_release_direction_relays(device);
		PRIVATE_DATA->roof_state = ROOF_UNKNOWN;
		DOME_SHUTTER_OPENED_ITEM->sw.value = false;
		DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "Open / Close timed out.");
		return;
	}
	if (dragonfly_read_sensors(device, sensors)) {
		bool opened = sensors[OPENED_SENSOR] > DRAGONFLY_SENSOR_THRESHOLD;
		bool closed = sensors[CLOSED_SENSOR] > DRAGONFLY_SENSOR_THRESHOLD;
		if (opened || closed) {
			dome_release_direction_relays(device);
		}
		if (opened && !closed) {
			PRIVATE_DATA->roof_state = ROOF_OPENED;
			DOME_SHUTTER_OPENED_ITEM->sw.value = true;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, "Roof is open.");
			return;
		}
		if (!opened && closed) {
			PRIVATE_DATA->roof_state = ROOF_CLOSED;
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = true;
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, "Roof is closed.");
			return;
		}
		if (opened && closed) {
			PRIVATE_DATA->roof_state = ROOF_UNKNOWN;
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "Roof shows quantum properties, it is both opened and closed.");
			return;
		}
	}
	indigo_execute_handler_in(device, ROOF_POLL_INTERVAL, dome_shutter_finalizer);
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ dome.on_timer
	dragonfly_keep_alive(device);
	indigo_execute_handler_in(device, KEEP_ALIVE_INTERVAL, dome_timer_callback);
	//- dome.on_timer
}

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = dragonfly_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ dome.on_connect
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->board);
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			dragonfly_authenticate(device, AUTHENTICATION_PASSWORD_ITEM->text.value);
			dome_seed_roof_state(device);
			//- dome.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, dome_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				dragonfly_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			dragonfly_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_authentication_handler(indigo_device *device) {
	AUTHENTICATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.AUTHENTICATION.on_change
	dragonfly_authenticate(device, *AUTHENTICATION_PASSWORD_ITEM->text.value == 0 ? NULL : AUTHENTICATION_PASSWORD_ITEM->text.value);
	//- dome.AUTHENTICATION.on_change
	indigo_update_property(device, AUTHENTICATION_PROPERTY, NULL);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	int sensors[DRAGONFLY_CHANNELS];
	bool parked = false;
	if (dragonfly_read_sensors(device, sensors) && sensors[PARK_SENSOR] > (int)X_DOME_SETTINGS_PARK_THRESHOLD_ITEM->number.value) {
		parked = true;
		bool opened = sensors[OPENED_SENSOR] > DRAGONFLY_SENSOR_THRESHOLD;
		bool closed = sensors[CLOSED_SENSOR] > DRAGONFLY_SENSOR_THRESHOLD;
		// A roof that reached an end position by hand gives control
		// back, one that left an end position by hand takes it away.
		if (opened && PRIVATE_DATA->roof_state != ROOF_OPENED) {
			PRIVATE_DATA->roof_state = ROOF_OPENED;
		} else if (closed && PRIVATE_DATA->roof_state != ROOF_CLOSED) {
			PRIVATE_DATA->roof_state = ROOF_CLOSED;
		} else if ((!closed && PRIVATE_DATA->roof_state == ROOF_CLOSED) || (!opened && PRIVATE_DATA->roof_state == ROOF_OPENED)) {
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "The roof seems to be moved by hand, the driver will regain control when opened or closed position is reached.");
			return;
		}
	}
	if ((PRIVATE_DATA->roof_state == ROOF_OPENED && DOME_SHUTTER_OPENED_ITEM->sw.value) || (PRIVATE_DATA->roof_state == ROOF_CLOSED && DOME_SHUTTER_CLOSED_ITEM->sw.value) || (!DOME_SHUTTER_OPENED_ITEM->sw.value && !DOME_SHUTTER_CLOSED_ITEM->sw.value)) {
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
		return;
	}
	if (!parked) {
		if (PRIVATE_DATA->roof_state == ROOF_OPENED) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		} else if (PRIVATE_DATA->roof_state == ROOF_CLOSED) {
			indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
		} else {
			DOME_SHUTTER_OPENED_ITEM->sw.value = false;
			DOME_SHUTTER_CLOSED_ITEM->sw.value = false;
		}
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "Can not move the roof, mount is not parked!");
		return;
	}
	if (dome_start_motion(device)) {
		indigo_execute_handler_in(device, X_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM->number.value, dome_shutter_finalizer);
	}
	//- dome.DOME_SHUTTER.on_change
}

static void dome_abort_motion_handler(indigo_device *device) {
	//+ dome.DOME_ABORT_MOTION.on_change
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, dome_shutter_handler);
		indigo_cancel_pending_handler(device, dome_shutter_finalizer);
		if (dragonfly_set_relay(device, OPEN_RELAY, false)) {
			dragonfly_set_relay(device, CLOSE_RELAY, false);
			PRIVATE_DATA->roof_state = PRIVATE_DATA->roof_state == ROOF_CLOSING ? ROOF_STOPPED_WHILE_CLOSING : ROOF_STOPPED_WHILE_OPENING;
			if (!X_DOME_BUTTON_FUNCTION_2_BUTTONS_ITEM->sw.value) {
				// Push and release wirings need the stop button.
				dragonfly_set_relay(device, STOP_RELAY, true);
				indigo_usleep(INDIGO_DELAY(X_DOME_SETTINGS_BUTTON_PULSE_ITEM->number.value));
				dragonfly_set_relay(device, STOP_RELAY, false);
			}
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "Roof Stopped.");
		} else {
			INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_ALERT_STATE, "Can not stop the roof, did you authorize?");
		}
	}
	INDIGO_UPDATE_PROPERTY_STATE(DOME_ABORT_MOTION_PROPERTY, INDIGO_OK_STATE, NULL);
	//- dome.DOME_ABORT_MOTION.on_change
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		//+ dome.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, "udp://dragonfly");
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->label, "Device URL");
		INDIGO_COPY_VALUE(DOME_SHUTTER_PROPERTY->label, "Shutter / Roof");
		INDIGO_COPY_VALUE(DOME_SHUTTER_OPENED_ITEM->label, "Shutter / Roof opened");
		INDIGO_COPY_VALUE(DOME_SHUTTER_CLOSED_ITEM->label, "Shutter / Roof closed");
		//- dome.on_attach
		AUTHENTICATION_PROPERTY->hidden = false;
		//+ dome.AUTHENTICATION.on_attach
		AUTHENTICATION_PROPERTY->count = 1;
		//- dome.AUTHENTICATION.on_attach
		DOME_SHUTTER_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_DIRECTION_PROPERTY->hidden = true;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
		DOME_STEPS_PROPERTY->hidden = true;
		DOME_PARK_PROPERTY->hidden = true;
		DOME_DIMENSION_PROPERTY->hidden = true;
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
		X_DOME_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, X_DOME_SETTINGS_PROPERTY_NAME, "Settings", "Dome Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (X_DOME_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_DOME_SETTINGS_BUTTON_PULSE_ITEM, X_DOME_SETTINGS_BUTTON_PULSE_ITEM_NAME, "Open/Close push duration (sec)", 0, 3, 0.5, 0.5);
		indigo_init_number_item(X_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM, X_DOME_SETTINGS_READ_SENSORS_DELAY_ITEM_NAME, "Read sensors delay after push (sec)", 0, 6, 0.5, 2.5);
		indigo_init_number_item(X_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM, X_DOME_SETTINGS_OPEN_CLOSE_TIMEOUT_ITEM_NAME, "Open/Close timeout (sec)", 0, 300, 1, 60);
		indigo_init_number_item(X_DOME_SETTINGS_PARK_THRESHOLD_ITEM, X_DOME_SETTINGS_PARK_THRESHOLD_ITEM_NAME, "Mount park sensor threshold", 0, 1024, 1, 512);
		X_DOME_BUTTON_FUNCTION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_DOME_BUTTON_FUNCTION_PROPERTY_NAME, "Settings", "Buttons Function Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_DOME_BUTTON_FUNCTION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_DOME_BUTTON_FUNCTION_1_BUTTON_ITEM, X_DOME_BUTTON_FUNCTION_1_BUTTON_ITEM_NAME, "1 Button, push (relay #1-open/close/stop)", true);
		indigo_init_switch_item(X_DOME_BUTTON_FUNCTION_2_BUTTONS_ITEM, X_DOME_BUTTON_FUNCTION_2_BUTTONS_ITEM_NAME, "2 Buttons, push and hold (relays: #2-open, #3-close)", false);
		indigo_init_switch_item(X_DOME_BUTTON_FUNCTION_3_BUTTONS_ITEM, X_DOME_BUTTON_FUNCTION_3_BUTTONS_ITEM_NAME, "3 Buttons, push (relays: #1-stop, #2-open, #3-close)", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(X_DOME_SETTINGS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(X_DOME_BUTTON_FUNCTION_PROPERTY);
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, dome_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUTHENTICATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUTHENTICATION_PROPERTY, dome_authentication_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DOME_SETTINGS_PROPERTY, property)) {
		indigo_property_copy_values(X_DOME_SETTINGS_PROPERTY, property, false);
		X_DOME_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_DOME_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_DOME_BUTTON_FUNCTION_PROPERTY, property)) {
		indigo_property_copy_values(X_DOME_BUTTON_FUNCTION_PROPERTY, property, false);
		X_DOME_BUTTON_FUNCTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_DOME_BUTTON_FUNCTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_DOME_SETTINGS_PROPERTY);
			indigo_save_property(device, NULL, X_DOME_BUTTON_FUNCTION_PROPERTY);
		}
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(X_DOME_SETTINGS_PROPERTY);
	indigo_release_property(X_DOME_BUTTON_FUNCTION_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	dragonfly_update_sensors(device);
	indigo_execute_handler_in(device, 1, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = dragonfly_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux.on_connect
			dragonfly_attach_relays(device);
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				dragonfly_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		for (int i = 0; i < RELAY_COUNT; i++) {
			PRIVATE_DATA->relay_pulse_until[i] = 0;
		}
		//- aux.on_disconnect
		indigo_delete_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			dragonfly_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_authentication_handler(indigo_device *device) {
	AUTHENTICATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUTHENTICATION.on_change
	dragonfly_authenticate(device, *AUTHENTICATION_PASSWORD_ITEM->text.value == 0 ? NULL : AUTHENTICATION_PASSWORD_ITEM->text.value);
	//- aux.AUTHENTICATION.on_change
	indigo_update_property(device, AUTHENTICATION_PROPERTY, NULL);
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	dragonfly_apply_outlet_names(device);
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_gpio_outlets_handler(indigo_device *device) {
	//+ aux.AUX_GPIO_OUTLETS.on_change
	if (dragonfly_set_outlets(device)) {
		INDIGO_UPDATE_PROPERTY_STATE(AUX_GPIO_OUTLETS_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		INDIGO_UPDATE_PROPERTY_STATE(AUX_GPIO_OUTLETS_PROPERTY, INDIGO_ALERT_STATE, "Relay operation failed, did you authorize?");
	}
	double delay = dragonfly_pulse_delay(device);
	if (delay >= 0) {
		// One finalizer serves every relay of this device, so a pulse
		// started while another is running replaces its wake-up.
		indigo_cancel_pending_handler(device, relay_pulse_finalizer);
		indigo_execute_handler_in(device, delay, relay_pulse_finalizer);
	}
	//- aux.AUX_GPIO_OUTLETS.on_change
}

static void aux_sensor_names_handler(indigo_device *device) {
	AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SENSOR_NAMES.on_change
	dragonfly_apply_sensor_names(device);
	//- aux.AUX_SENSOR_NAMES.on_change
	indigo_update_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		//- aux.on_attach
		AUTHENTICATION_PROPERTY->hidden = false;
		//+ aux.AUTHENTICATION.on_attach
		AUTHENTICATION_PROPERTY->count = 1;
		//- aux.AUTHENTICATION.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay names", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_GPIO_OUTLET_NAME_4_ITEM_NAME, "Relay 4", "Relay #4");
		indigo_init_text_item(AUX_OUTLET_NAME_5_ITEM, AUX_GPIO_OUTLET_NAME_5_ITEM_NAME, "Relay 5", "Relay #5");
		indigo_init_text_item(AUX_OUTLET_NAME_6_ITEM, AUX_GPIO_OUTLET_NAME_6_ITEM_NAME, "Relay 6", "Relay #6");
		indigo_init_text_item(AUX_OUTLET_NAME_7_ITEM, AUX_GPIO_OUTLET_NAME_7_ITEM_NAME, "Relay 7", "Relay #7");
		indigo_init_text_item(AUX_OUTLET_NAME_8_ITEM, AUX_GPIO_OUTLET_NAME_8_ITEM_NAME, "Relay 8", "Relay #8");
		AUX_GPIO_OUTLETS_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 5);
		if (AUX_GPIO_OUTLETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_GPIO_OUTLET_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", false);
		AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_OUTLET_PULSE_LENGTHS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Relay pulse lengths (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", 0, 100000, 100, 0);
		AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (AUX_SENSOR_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor 3", "Sensor #3");
		indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor 4", "Sensor #4");
		indigo_init_text_item(AUX_SENSOR_NAME_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor 5", "Sensor #5");
		indigo_init_text_item(AUX_SENSOR_NAME_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor 6", "Sensor #6");
		indigo_init_text_item(AUX_SENSOR_NAME_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor 7", "Sensor #7");
		AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 5);
		if (AUX_GPIO_SENSORS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor #5", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor #6", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor #7", 0, 1024, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_OUTLETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_SENSORS_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SENSOR_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUTHENTICATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUTHENTICATION_PROPERTY, aux_authentication_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLETS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_GPIO_OUTLETS_PROPERTY, aux_gpio_outlets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		AUX_OUTLET_PULSE_LENGTHS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SENSOR_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SENSOR_NAMES_PROPERTY, aux_sensor_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_GPIO_OUTLETS_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_dome_dragonfly(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static dragonfly_private_data *private_data = NULL;
	static indigo_device *dome = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			// The relay driver exposes all eight channels of the same controller,
			// so the two drivers must not be loaded at the same time.
			if (indigo_driver_initialized(CONFLICTING_DRIVER)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Conflicting driver %s is already loaded", CONFLICTING_DRIVER);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			//- on_init
			private_data = (dragonfly_private_data *)indigo_safe_malloc(sizeof(dragonfly_private_data));
			dome = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = dome;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(dome);
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
			}
			if (dome != NULL) {
				indigo_detach_device(dome);
				indigo_safe_free(dome);
				dome = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
