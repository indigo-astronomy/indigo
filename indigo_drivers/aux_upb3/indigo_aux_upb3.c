// Copyright (c) 2025 CloudMakers, s. r. o.
// All rights reserved.

// You can use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_aux_upb3.driver (2025-04-23 10:31).

// version history
// 3.0 Peter Polakovic <peter.polakovic@cloudmakers.eu>

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_upb3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_aux_upb3"
#define DRIVER_LABEL "PegasusAstro Ultimate Powerbox v3"
#define AUX_DEVICE_NAME "Ultimate Powerbox 3"
#define FOCUSER_DEVICE_NAME "Ultimate Powerbox 3 (focuser)"

// Custom code below

#define POWERBOX_GROUP "Powerbox"

// Custom code above

#pragma mark - Property definitions

// AUX_OUTLET_NAMES handles definition

#define AUX_OUTLET_NAMES_PROPERTY (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_NAME_2_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_NAME_3_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_NAME_4_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_NAME_5_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_NAME_6_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_POWER_OUTLET_NAME_7_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_POWER_OUTLET_NAME_8_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 7)
#define AUX_POWER_OUTLET_NAME_9_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 8)
#define AUX_HEATER_OUTLET_NAME_1_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 9)
#define AUX_HEATER_OUTLET_NAME_2_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 10)
#define AUX_HEATER_OUTLET_NAME_3_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 11)
#define AUX_USB_PORT_NAME_1_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 12)
#define AUX_USB_PORT_NAME_2_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 13)
#define AUX_USB_PORT_NAME_3_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 14)
#define AUX_USB_PORT_NAME_4_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 15)
#define AUX_USB_PORT_NAME_5_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 16)
#define AUX_USB_PORT_NAME_6_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 17)
#define AUX_USB_PORT_NAME_7_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 18)
#define AUX_USB_PORT_NAME_8_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 19)

// AUX_POWER_OUTLET handles definition

#define AUX_POWER_OUTLET_PROPERTY (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_5_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_6_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 5)
#define AUX_POWER_OUTLET_7_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 6)
#define AUX_POWER_OUTLET_8_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 7)
#define AUX_POWER_OUTLET_9_ITEM (AUX_POWER_OUTLET_PROPERTY->items + 8)

// AUX_POWER_OUTLET_STATE handles definition

#define AUX_POWER_OUTLET_STATE_PROPERTY (PRIVATE_DATA->aux_power_outlet_state_property)
#define AUX_POWER_OUTLET_STATE_1_ITEM (AUX_POWER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_STATE_2_ITEM (AUX_POWER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_STATE_3_ITEM (AUX_POWER_OUTLET_STATE_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_STATE_4_ITEM (AUX_POWER_OUTLET_STATE_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_STATE_5_ITEM (AUX_POWER_OUTLET_STATE_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_STATE_6_ITEM (AUX_POWER_OUTLET_STATE_PROPERTY->items + 5)

// AUX_HEATER_OUTLET handles definition

#define AUX_HEATER_OUTLET_PROPERTY (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM (AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM (AUX_HEATER_OUTLET_PROPERTY->items + 2)

// AUX_DEW_CONTROL handles definition

#define AUX_DEW_CONTROL_PROPERTY (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

// AUX_USB_PORT handles definition

#define AUX_USB_PORT_PROPERTY (PRIVATE_DATA->aux_usb_port_property)
#define AUX_USB_PORT_1_ITEM (AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM (AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM (AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM (AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM (AUX_USB_PORT_PROPERTY->items + 4)
#define AUX_USB_PORT_6_ITEM (AUX_USB_PORT_PROPERTY->items + 5)
#define AUX_USB_PORT_7_ITEM (AUX_USB_PORT_PROPERTY->items + 6)
#define AUX_USB_PORT_8_ITEM (AUX_USB_PORT_PROPERTY->items + 7)

// AUX_WEATHER handles definition

#define AUX_WEATHER_PROPERTY (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM (AUX_WEATHER_PROPERTY->items + 2)

// AUX_INFO handles definition

#define AUX_INFO_PROPERTY (PRIVATE_DATA->aux_info_property)
#define AUX_AVERAGE_ITEM (AUX_INFO_PROPERTY->items + 0)
#define AUX_AMP_HOUR_ITEM (AUX_INFO_PROPERTY->items + 1)
#define AUX_WATT_HOUR_ITEM (AUX_INFO_PROPERTY->items + 2)
#define AUX_INFO_VOLTAGE_ITEM (AUX_INFO_PROPERTY->items + 3)
#define AUX_INFO_CURRENT_ITEM (AUX_INFO_PROPERTY->items + 4)

// AUX_SAVE_OUTLET_STATES_AS_DEFAULT handles definition

#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY (PRIVATE_DATA->aux_save_outlet_states_as_default_property)
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->items + 0)

// AUX_REBOOT handles definition

#define AUX_REBOOT_PROPERTY (PRIVATE_DATA->aux_reboot_property)
#define AUX_REBOOT_ITEM (AUX_REBOOT_PROPERTY->items + 0)

// AUX_VARIABLE_POWER_OUTLET handles definition

#define AUX_VARIABLE_POWER_OUTLET_PROPERTY (PRIVATE_DATA->aux_variable_power_outlet_property)
#define AUX_VARIABLE_POWER_OUTLET_7_ITEM (AUX_VARIABLE_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_VARIABLE_POWER_OUTLET_8_ITEM (AUX_VARIABLE_POWER_OUTLET_PROPERTY->items + 1)

#pragma mark - Private data definition

#define PRIVATE_DATA ((upb3_private_data *)device->private_data)

typedef struct {
	pthread_mutex_t mutex;
	int count;
	indigo_uni_handle *handle;

	// Custom code below

	int version;

	// Custom code above

	indigo_timer *aux_timer;
	indigo_timer *focuser_timer;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_power_outlet_property;
	indigo_property *aux_power_outlet_state_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *aux_dew_control_property;
	indigo_property *aux_usb_port_property;
	indigo_property *aux_weather_property;
	indigo_property *aux_info_property;
	indigo_property *aux_save_outlet_states_as_default_property;
	indigo_property *aux_reboot_property;
	indigo_property *aux_variable_power_outlet_property;
} upb3_private_data;

#pragma mark - Low level code

// Custom code below

static bool upb3_command(indigo_device *device, char *command, char *response, int max) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) > 0) {
			if (response) {
				if (indigo_uni_read_section(PRIVATE_DATA->handle, response, max, "\n", "\r\n", INDIGO_DELAY(1)) > 0) {
					return true;
				}
			} else {
				return true;
			}
		}
	}
	return false;
}

static bool upb3_open(indigo_device *device) {
	char response[128];
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		int attempt = 0;
		while (true) {
			if (upb3_command(device, "P#", response, sizeof(response)) && !strncmp(response, "UPBv3_", 6)) {
				PRIVATE_DATA->version = 3;
				if (upb3_command(device, "PV", response, sizeof(response))) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro UPBv3");
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 3);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					return true;
				}
			}
			if (attempt++ == 3) {
				indigo_uni_close(&PRIVATE_DATA->handle);
				break;
			}
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Device not detected - retrying in 1 second...");
			indigo_sleep(1);
		}
	}
	return false;
}

static void upb3_close(indigo_device *device) {
	char response[128];
	strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	if (PRIVATE_DATA->handle != NULL) {
		upb3_command(device, "PL:0", response, sizeof(response));
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

// Custom code above

#pragma mark - High level code (aux)

// aux state checking timer callback

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);

	// Custom code below

	char response[128];
	bool updatePowerOutletState = false;
	bool updateWeather = false;
	bool updateInfo = false;
	if (upb3_command(device, "IS", response, sizeof(response))) {
		char *pnt, *token = strtok_r(response, ":", &pnt);
		for (int i = 0; i < 6; i++) { // power 1-6
			indigo_item *item = AUX_POWER_OUTLET_STATE_PROPERTY->items + i;
			if ((token = strtok_r(NULL, ":", &pnt))) {
				bool value = *token == '1';
				if (value && item->light.value != INDIGO_ALERT_STATE) {
					updatePowerOutletState = true;
					item->light.value = INDIGO_ALERT_STATE;
				} else if (!value && item->light.value == INDIGO_ALERT_STATE) {
					updatePowerOutletState = true;
					item->light.value = AUX_POWER_OUTLET_PROPERTY->items[i].sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
				} else if (!value) {
					if (AUX_POWER_OUTLET_PROPERTY->items[i].sw.value) {
						if (item->light.value != INDIGO_OK_STATE) {
							item->light.value = INDIGO_OK_STATE;
							updatePowerOutletState = true;
						}
					} else {
						if (item->light.value != INDIGO_IDLE_STATE) {
							item->light.value = INDIGO_IDLE_STATE;
							updatePowerOutletState = true;
						}
					}
				}
			}
		}
	}
	if (upb3_command(device, "ES", response, sizeof(response))) {
		char *pnt, *token = strtok_r(response, ":", &pnt);
		if ((token = strtok_r(NULL, ":", &pnt))) { // Temp
			double value = indigo_atod(token);
			if (AUX_WEATHER_TEMPERATURE_ITEM->number.value != value) {
				updateWeather = true;
				AUX_WEATHER_TEMPERATURE_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Humidity
			double value = indigo_atod(token);
			if (AUX_WEATHER_HUMIDITY_ITEM->number.value != value) {
				updateWeather = true;
				AUX_WEATHER_HUMIDITY_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Dewpoint
			double value = indigo_atod(token);
			if (AUX_WEATHER_DEWPOINT_ITEM->number.value != value) {
				updateWeather = true;
				AUX_WEATHER_DEWPOINT_ITEM->number.value = value;
			}
		}
	}
	if (upb3_command(device, "VR", response, sizeof(response))) {
		char *pnt, *token = strtok_r(response, ":", &pnt);
		if ((token = strtok_r(NULL, ":", &pnt))) { // Voltage
			double value = indigo_atod(token);
			if (AUX_INFO_VOLTAGE_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_VOLTAGE_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current
			double value = indigo_atod(token);
			if (AUX_INFO_CURRENT_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_CURRENT_ITEM->number.value = value;
			}
		}
	}
	if (upb3_command(device, "PC", response, sizeof(response))) {
		char *pnt, *token = strtok_r(response, ":", &pnt);
		if ((token = strtok_r(NULL, ":", &pnt))) {
			double value = indigo_atod(token);
			if (AUX_AVERAGE_ITEM->number.value != value) {
				updateInfo = true;
				AUX_AVERAGE_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) {
			double value = indigo_atod(token);
			if (AUX_AMP_HOUR_ITEM->number.value != value) {
				updateInfo = true;
				AUX_AMP_HOUR_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) {
			double value = indigo_atod(token);
			if (AUX_WATT_HOUR_ITEM->number.value != value) {
				updateInfo = true;
				AUX_WATT_HOUR_ITEM->number.value = value;
			}
		}
	}
	if (updatePowerOutletState) {
		AUX_POWER_OUTLET_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
	}
	if (updateWeather) {
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	}
	if (updateInfo) {
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);

	// Custom code above

	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// CONNECTION change handler

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = upb3_open(device);
		}
		if (connection_result) {

			// Custom code below

			char response[128];
			if (upb3_command(device, "PA", response, sizeof(response))) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				for (int i = 0; i < 6; i++) { // output 1-6
					indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
					if ((token = strtok_r(NULL, ":", &pnt))) {
						bool value = atoi(token) > 0;
						if (item->sw.value != value) {
							item->sw.value = value;
						}
					}
				}
				for (int i = 0; i < 3; i++) { // dew 1-3
					indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
					if ((token = strtok_r(NULL, ":", &pnt))) {
						int value = atoi(token);
						if (item->number.value != value) {
							item->number.value = value;
						}
					}
				}
				for (int i = 6; i < 9; i++) { // buck, boost, relay
					indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
					if ((token = strtok_r(NULL, ":", &pnt))) {
						bool value = *token == '1';
						if (item->sw.value != value) {
							item->sw.value = value;
						}
					}
				}
			}
			if (upb3_command(device, "AJ", response, sizeof(response))) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				if ((token = strtok_r(NULL, ":", &pnt))) { // Buck voltage
					int value = atoi(token);
					if (AUX_VARIABLE_POWER_OUTLET_7_ITEM->number.value != value) {
						AUX_VARIABLE_POWER_OUTLET_7_ITEM->number.target = AUX_VARIABLE_POWER_OUTLET_7_ITEM->number.value = value;
					}
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Buck status
					AUX_POWER_OUTLET_7_ITEM->sw.value = *token == '1';
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Boost voltage
					AUX_VARIABLE_POWER_OUTLET_8_ITEM->number.target = AUX_VARIABLE_POWER_OUTLET_8_ITEM->number.value = atoi(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Boost status
					AUX_POWER_OUTLET_8_ITEM->sw.value = *token == '1';
				}
			}
			if (upb3_command(device, "UA", response, sizeof(response))) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				for (int i = 0; i < 8; i++) { // usb 1-8
					indigo_item *item = AUX_USB_PORT_PROPERTY->items + i;
					if ((token = strtok_r(NULL, ":", &pnt))) {
						bool value = *token == '1';
						if (item->sw.value != value) {
							item->sw.value = value;
						}
					}
				}
			}
			if (upb3_command(device, "PD", response, sizeof(response))) {
				if (!strcmp(response, "PD:000")) {
					if (AUX_DEW_CONTROL_MANUAL_ITEM->sw.value) {
						indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_MANUAL_ITEM, true);
					}
				} else {
					if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value) {
						indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_AUTOMATIC_ITEM, true);
					}
				}
			}
			upb3_command(device, "PL:1", response, sizeof(response));

			// Custom code above

			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
			indigo_define_property(device, AUX_REBOOT_PROPERTY, NULL);
			indigo_define_property(device, AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			upb3_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_unlock_master_device(device);
}

// AUX_OUTLET_NAMES change handler

static void aux_outlet_names_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_5_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_6_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_7_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_8_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_9_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_9_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_4_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_5_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_5_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_6_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_6_ITEM->text.value);
	snprintf(AUX_USB_PORT_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
	snprintf(AUX_USB_PORT_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
	snprintf(AUX_USB_PORT_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_3_ITEM->text.value);
	snprintf(AUX_USB_PORT_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_4_ITEM->text.value);
	snprintf(AUX_USB_PORT_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_5_ITEM->text.value);
	snprintf(AUX_USB_PORT_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
	snprintf(AUX_USB_PORT_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
	snprintf(AUX_USB_PORT_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}

	// Custom code above

	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_POWER_OUTLET change handler

static void aux_power_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char response[128];
	upb3_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? "P1:100" : "P1:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? "P2:100" : "P2:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_3_ITEM->sw.value ? "P3:100" : "P3:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_4_ITEM->sw.value ? "P4:100" : "P4:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_5_ITEM->sw.value ? "P5:100" : "P5:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_6_ITEM->sw.value ? "P6:100" : "P6:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_7_ITEM->sw.value ? "PJ:1" : "PJ:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_8_ITEM->sw.value ? "PB:1" : "PB:0", response, sizeof(response));
	upb3_command(device, AUX_POWER_OUTLET_9_ITEM->sw.value ? "RL:1" : "RL:0", response, sizeof(response));

	// Custom code above

	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_HEATER_OUTLET change handler

static void aux_heater_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16], response[128];
	sprintf(command, "D1:%d", (int)AUX_HEATER_OUTLET_1_ITEM->number.value);
	upb3_command(device, command, response, sizeof(response));
	sprintf(command, "D2:%d", (int)AUX_HEATER_OUTLET_2_ITEM->number.value);
	upb3_command(device, command, response, sizeof(response));
	sprintf(command, "D3:%d", (int)AUX_HEATER_OUTLET_3_ITEM->number.value);
	upb3_command(device, command, response, sizeof(response));

	// Custom code above

	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_DEW_CONTROL change handler

static void aux_dew_control_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char response[128];
	if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value) {
		upb3_command(device, "ADW1:1", response, sizeof(response));
		upb3_command(device, "ADW2:1", response, sizeof(response));
		upb3_command(device, "ADW3:1", response, sizeof(response));
		upb3_command(device, "DA:5", response, sizeof(response));
	} else {
		upb3_command(device, "ADW1:0", response, sizeof(response));
		upb3_command(device, "ADW2:0", response, sizeof(response));
		upb3_command(device, "ADW3:0", response, sizeof(response));
	}

	// Custom code above

	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_USB_PORT change handler

static void aux_usb_port_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16], response[128];
	for (int i = 0; i < AUX_USB_PORT_PROPERTY->count; i++) {
		sprintf(command, "U%d:%d", i + 1, AUX_USB_PORT_PROPERTY->items[i].sw.value ? 1 : 0);
		upb3_command(device, command, response, sizeof(response));
	}

	// Custom code above

	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_SAVE_OUTLET_STATES_AS_DEFAULT change handler

static void aux_save_outlet_states_as_default_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char response[128];
	if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value) {
		upb3_command(device, "PS", response, sizeof(response));
		upb3_command(device, "US", response, sizeof(response));
		upb3_command(device, "DSTR", response, sizeof(response));
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value = false;
	}

	// Custom code above

	indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_REBOOT change handler

static void aux_reboot_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char response[128];
	if (AUX_REBOOT_ITEM->sw.value) {
		upb3_command(device, "PF", response, sizeof(response));
		AUX_REBOOT_ITEM->sw.value = false;
	}

	// Custom code above

	indigo_update_property(device, AUX_REBOOT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_VARIABLE_POWER_OUTLET change handler

static void aux_variable_power_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_VARIABLE_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char response[128], command[32];
	sprintf(command, "PJ:%d", (int)AUX_VARIABLE_POWER_OUTLET_7_ITEM->number.value);
	upb3_command(device, command, response, sizeof(response));
	sprintf(command, "PB:%d", (int)AUX_VARIABLE_POWER_OUTLET_8_ITEM->number.value);
	upb3_command(device, command, response, sizeof(response));

	// Custom code above

	indigo_update_property(device, AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// aux attach API callback

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;


		// Custom code below

		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");

		// Custom code above

		// AUX_OUTLET_NAMES initialisation

		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, POWERBOX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 20);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Outlet #1", "Outlet #1");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Outlet #2", "Outlet #2");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "Outlet #3", "Outlet #3");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "Outlet #4", "Outlet #4");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_5_ITEM, AUX_POWER_OUTLET_NAME_5_ITEM_NAME, "Outlet #5", "Outlet #5");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_6_ITEM, AUX_POWER_OUTLET_NAME_6_ITEM_NAME, "Outlet #6", "Outlet #6");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_7_ITEM, AUX_POWER_OUTLET_NAME_7_ITEM_NAME, "Outlet #7", "Outlet #7");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_8_ITEM, AUX_POWER_OUTLET_NAME_8_ITEM_NAME, "Outlet #8", "Outlet #8");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_9_ITEM, AUX_POWER_OUTLET_NAME_9_ITEM_NAME, "Outlet #9", "Outlet #9");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater #1", "Heater #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater #2", "Heater #2");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater #3", "Heater #3");
		indigo_init_text_item(AUX_USB_PORT_NAME_1_ITEM, AUX_USB_PORT_NAME_1_ITEM_NAME, "Port #1", "Port #1");
		indigo_init_text_item(AUX_USB_PORT_NAME_2_ITEM, AUX_USB_PORT_NAME_2_ITEM_NAME, "Port #2", "Port #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_3_ITEM, AUX_USB_PORT_NAME_3_ITEM_NAME, "Port #3", "Port #3");
		indigo_init_text_item(AUX_USB_PORT_NAME_4_ITEM, AUX_USB_PORT_NAME_4_ITEM_NAME, "Port #4", "Port #4");
		indigo_init_text_item(AUX_USB_PORT_NAME_5_ITEM, AUX_USB_PORT_NAME_5_ITEM_NAME, "Port #5", "Port #5");
		indigo_init_text_item(AUX_USB_PORT_NAME_6_ITEM, AUX_USB_PORT_NAME_6_ITEM_NAME, "Port #6", "Port #6");
		indigo_init_text_item(AUX_USB_PORT_NAME_7_ITEM, AUX_USB_PORT_NAME_7_ITEM_NAME, "Port #7", "Port #7");
		indigo_init_text_item(AUX_USB_PORT_NAME_8_ITEM, AUX_USB_PORT_NAME_8_ITEM_NAME, "Port #8", "Port #8");

		// AUX_POWER_OUTLET initialisation

		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, POWERBOX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 9);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Outlet #3", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Outlet #4", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_5_ITEM, AUX_POWER_OUTLET_5_ITEM_NAME, "Outlet #5", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_6_ITEM, AUX_POWER_OUTLET_6_ITEM_NAME, "Outlet #6", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_7_ITEM, AUX_POWER_OUTLET_7_ITEM_NAME, "Buck output", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_8_ITEM, AUX_POWER_OUTLET_8_ITEM_NAME, "Boost output", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_9_ITEM, AUX_POWER_OUTLET_9_ITEM_NAME, "Relay switch", true);

		// AUX_POWER_OUTLET_STATE initialisation

		AUX_POWER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_POWER_OUTLET_STATE_PROPERTY_NAME, POWERBOX_GROUP, "Power outlets state", INDIGO_OK_STATE, 6);
		if (AUX_POWER_OUTLET_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_1_ITEM, AUX_POWER_OUTLET_STATE_1_ITEM_NAME, "Outlet #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_2_ITEM, AUX_POWER_OUTLET_STATE_2_ITEM_NAME, "Outlet #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_3_ITEM, AUX_POWER_OUTLET_STATE_3_ITEM_NAME, "Outlet #3 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_4_ITEM, AUX_POWER_OUTLET_STATE_4_ITEM_NAME, "Outlet #4 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_5_ITEM, AUX_POWER_OUTLET_STATE_5_ITEM_NAME, "Outlet #5 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_6_ITEM, AUX_POWER_OUTLET_STATE_6_ITEM_NAME, "Outlet #6 state", INDIGO_OK_STATE);

		// AUX_HEATER_OUTLET initialisation

		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, POWERBOX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater #3 [%]", 0, 100, 5, 0);

		// AUX_DEW_CONTROL initialisation

		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, POWERBOX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", true);

		// AUX_USB_PORT initialisation

		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, POWERBOX_GROUP, "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 8);
		if (AUX_USB_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "Port #1", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "Port #2", true);
		indigo_init_switch_item(AUX_USB_PORT_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "Port #3", true);
		indigo_init_switch_item(AUX_USB_PORT_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "Port #4", true);
		indigo_init_switch_item(AUX_USB_PORT_5_ITEM, AUX_USB_PORT_5_ITEM_NAME, "Port #5", true);
		indigo_init_switch_item(AUX_USB_PORT_6_ITEM, AUX_USB_PORT_6_ITEM_NAME, "Port #6", true);
		indigo_init_switch_item(AUX_USB_PORT_7_ITEM, AUX_USB_PORT_7_ITEM_NAME, "Port #7", true);
		indigo_init_switch_item(AUX_USB_PORT_8_ITEM, AUX_USB_PORT_8_ITEM_NAME, "Port #8", true);

		// AUX_WEATHER initialisation

		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, POWERBOX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [C]", -50, 100, 0, 0);

		// AUX_INFO initialisation

		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, POWERBOX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 5);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_AVERAGE_ITEM, "AUX_AVERAGE", "Avereage current [A]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_AMP_HOUR_ITEM, "AUX_AMP_HOUR", "Amp-hour [Ah]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WATT_HOUR_ITEM, "AUX_WATT_HOUR", "Watt-hour [Wh]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 20, 0, 0);

		// AUX_SAVE_OUTLET_STATES_AS_DEFAULT initialisation

		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY = indigo_init_switch_property(NULL, device->name, "AUX_SAVE_OUTLET_STATES_AS_DEFAULT", POWERBOX_GROUP, "Save current outlet states as default", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM_NAME, "Save", false);

		// AUX_REBOOT initialisation

		AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, "AUX_REBOOT", POWERBOX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_REBOOT_ITEM, "REBOOT", "Manual", false);

		// AUX_VARIABLE_POWER_OUTLET initialisation

		AUX_VARIABLE_POWER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, "AUX_VARIABLE_POWER_OUTLET", POWERBOX_GROUP, "Variable voltage power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_VARIABLE_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_VARIABLE_POWER_OUTLET_7_ITEM, "OUTLET_7", "Voltage of adjustable buck output", 3, 12, 1, 3);
		indigo_init_number_item(AUX_VARIABLE_POWER_OUTLET_8_ITEM, "OUTLET_8", "Voltage of adjustable boost output", 12, 24, 1, 12);

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// aux enumerate API callback

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_POWER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_POWER_OUTLET_STATE_PROPERTY);
		indigo_define_matching_property(AUX_HEATER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_DEW_CONTROL_PROPERTY);
		indigo_define_matching_property(AUX_USB_PORT_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
		indigo_define_matching_property(AUX_INFO_PROPERTY);
		indigo_define_matching_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
		indigo_define_matching_property(AUX_REBOOT_PROPERTY);
		indigo_define_matching_property(AUX_VARIABLE_POWER_OUTLET_PROPERTY);
	}
	indigo_define_matching_property(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

// aux change property API callback

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {

  // CONNECTION change handling

	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;

  // AUX_OUTLET_NAMES change handling

	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_outlet_names_handler, NULL);
		return INDIGO_OK;

  // AUX_POWER_OUTLET change handling

	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_power_outlet_handler, NULL);
		return INDIGO_OK;

  // AUX_HEATER_OUTLET change handling

	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_heater_outlet_handler, NULL);
		return INDIGO_OK;

  // AUX_DEW_CONTROL change handling

	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_dew_control_handler, NULL);
		return INDIGO_OK;

  // AUX_USB_PORT change handling

	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		indigo_property_copy_values(AUX_USB_PORT_PROPERTY, property, false);
		AUX_USB_PORT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_usb_port_handler, NULL);
		return INDIGO_OK;

  // AUX_SAVE_OUTLET_STATES_AS_DEFAULT change handling

	} else if (indigo_property_match_changeable(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, property)) {
		indigo_property_copy_values(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, property, false);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_save_outlet_states_as_default_handler, NULL);
		return INDIGO_OK;

  // AUX_REBOOT change handling

	} else if (indigo_property_match_changeable(AUX_REBOOT_PROPERTY, property)) {
		indigo_property_copy_values(AUX_REBOOT_PROPERTY, property, false);
		AUX_REBOOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_REBOOT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_reboot_handler, NULL);
		return INDIGO_OK;

  // AUX_VARIABLE_POWER_OUTLET change handling

	} else if (indigo_property_match_changeable(AUX_VARIABLE_POWER_OUTLET_PROPERTY, property)) {
		indigo_property_copy_values(AUX_VARIABLE_POWER_OUTLET_PROPERTY, property, false);
		AUX_VARIABLE_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_variable_power_outlet_handler, NULL);
		return INDIGO_OK;

  // CONFIG change handling

	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

// aux detach API callback

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	indigo_release_property(AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_VARIABLE_POWER_OUTLET_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (focuser)

// focuser state checking timer callback

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);

	// Custom code below

	char response[128];
	if (upb3_command(device, "ES", response, sizeof(response))) {
		double temp = indigo_atod(response + 3);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	bool update = false;
	if (upb3_command(device, "SP", response, sizeof(response))) {
		int pos = atoi(response + 3);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			update = true;
		}
	}
	if (upb3_command(device, "SI", response, sizeof(response))) {
		if (response[3] == '0') {
			if (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				update = true;
			}
		} else {
			if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				update = true;
			}
		}
	}
	if (update) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->focuser_timer);

	// Custom code above

	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// CONNECTION change handler

static void focuser_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = upb3_open(device->master_device);
		}
		if (connection_result) {

			// Custom code below

			char response[128];
			if (upb3_command(device, "SA", response, sizeof(response))) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				token = strtok_r(NULL, ":", &pnt); // position
				if (token) {
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atoi(token);
				}
				token = strtok_r(NULL, ":", &pnt); // is_running
				if (token) {
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				}
				token = strtok_r(NULL, ":", &pnt); // direction
				if (token) {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, *token == '1' ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // speed
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = atoi(token);
				}
			}
			if (upb3_command(device, "PV", response, sizeof(response)) && !strncmp(response, "PV:", 3)) {
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 3);
			}
			upb3_command(device, "PL:1", response, sizeof(response));

			// Custom code above

			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
		if (--PRIVATE_DATA->count == 0) {
			upb3_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_unlock_master_device(device);
}

// FOCUSER_SPEED change handler

static void focuser_speed_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16], response[128];
	snprintf(command, sizeof(command), "SS:%d", (int)FOCUSER_SPEED_ITEM->number.value);
	if (!upb3_command(device, command, response, sizeof(response))) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	// Custom code above

	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// FOCUSER_STEPS change handler

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16];
	snprintf(command, sizeof(command), "SG:%d", (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1));
	if (upb3_command(device, command, NULL, 0)) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);

	// Custom code above

	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// FOCUSER_POSITION change handler

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16], response[64];
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		snprintf(command, sizeof(command), "SM:%d", (int)FOCUSER_POSITION_ITEM->number.target);
		if (upb3_command(device, command, response, sizeof(response))) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		snprintf(command, sizeof(command), "SC:%d", (int)FOCUSER_POSITION_ITEM->number.target);
		if (!upb3_command(device, command, response, sizeof(response))) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}

	// Custom code above

	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// FOCUSER_ABORT_MOTION change handler

static void focuser_abort_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char response[128];
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (upb3_command(device, "SH", response, sizeof(response))) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}

	// Custom code above

	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// FOCUSER_REVERSE_MOTION change handler

static void focuser_reverse_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16], response[128];
	snprintf(command, sizeof(command), "SR:%d", (int)FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value? 0 : 1);
	if (!upb3_command(device, command, response, sizeof(response))) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	// Custom code above

	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// FOCUSER_BACKLASH change handler

static void focuser_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;

	// Custom code below

	char command[16], response[128];
	snprintf(command, sizeof(command), "SB:%d", (int)FOCUSER_BACKLASH_ITEM->number.value);
	if (!upb3_command(device, command, response, sizeof(response))) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	// Custom code above

	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// focuser attach API callback

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;

		// Custom code below

		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 9999999;
		FOCUSER_POSITION_ITEM->number.step = 1;

		// Custom code above

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// focuser enumerate API callback

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

// focuser change property API callback

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {

  // CONNECTION change handling

	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;

  // FOCUSER_SPEED change handling

	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_speed_handler, NULL);
		return INDIGO_OK;

  // FOCUSER_STEPS change handling

	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;

  // FOCUSER_POSITION change handling

	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;

  // FOCUSER_ABORT_MOTION change handling

	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_motion_handler, NULL);
		return INDIGO_OK;

  // FOCUSER_REVERSE_MOTION change handling

	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_reverse_motion_handler, NULL);
		return INDIGO_OK;

  // FOCUSER_BACKLASH change handling

	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_backlash_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

// focuser detach API callback

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Main code

// PegasusAstro Ultimate Powerbox v3 driver entry point

indigo_result indigo_aux_upb3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static upb3_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_DEVICE_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	static indigo_device_match_pattern aux_patterns[1] = { 0 };
	strcpy(aux_patterns[0].product_string, "UPBv3");
	INDIGO_REGISER_MATCH_PATTERNS(aux_template, aux_patterns, 1);

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_DEVICE_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(upb3_private_data));
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			focuser->master_device = aux;
			indigo_attach_device(focuser);
			break;
		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
			}
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				free(focuser);
				focuser = NULL;
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
