// Copyright (c) 2018 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO PegasusAstro UPB aux driver
 \file indigo_aux_upb.c
 */

#define DRIVER_VERSION 0x0009
#define DRIVER_NAME "indigo_aux_upb"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>
#include <sys/termios.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_aux_upb.h"

#define PRIVATE_DATA												((upb_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY						(PRIVATE_DATA->outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_NAME_2_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_NAME_3_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_NAME_4_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_HEATER_OUTLET_NAME_1_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_HEATER_OUTLET_NAME_2_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_USB_PORT_NAME_1_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_USB_PORT_NAME_2_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 7)
#define AUX_USB_PORT_NAME_3_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 8)
#define AUX_USB_PORT_NAME_4_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 9)
#define AUX_USB_PORT_NAME_5_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 10)
#define AUX_USB_PORT_NAME_6_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 11)

#define AUX_POWER_OUTLET_PROPERTY						(PRIVATE_DATA->power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM							(AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM							(AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM							(AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM							(AUX_POWER_OUTLET_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_STATE_PROPERTY			(PRIVATE_DATA->power_outlet_state_property)
#define AUX_POWER_OUTLET_STATE_1_ITEM				(AUX_POWER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_STATE_2_ITEM				(AUX_POWER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_STATE_3_ITEM				(AUX_POWER_OUTLET_STATE_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_STATE_4_ITEM				(AUX_POWER_OUTLET_STATE_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_CURRENT_PROPERTY		(PRIVATE_DATA->power_outlet_current_property)
#define AUX_POWER_OUTLET_CURRENT_1_ITEM			(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_CURRENT_2_ITEM			(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_CURRENT_3_ITEM			(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_CURRENT_4_ITEM			(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 3)

#define AUX_HEATER_OUTLET_PROPERTY					(PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM						(AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM						(AUX_HEATER_OUTLET_PROPERTY->items + 1)

#define AUX_HEATER_OUTLET_STATE_PROPERTY		(PRIVATE_DATA->heater_outlet_state_property)
#define AUX_HEATER_OUTLET_STATE_1_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_STATE_2_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 1)

#define AUX_HEATER_OUTLET_CURRENT_PROPERTY	(PRIVATE_DATA->heater_outlet_current_property)
#define AUX_HEATER_OUTLET_CURRENT_1_ITEM		(AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_CURRENT_2_ITEM		(AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 1)

#define AUX_USB_PORT_PROPERTY								(PRIVATE_DATA->usb_port_property)
#define AUX_USB_PORT_1_ITEM									(AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM									(AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM									(AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM									(AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM									(AUX_USB_PORT_PROPERTY->items + 4)
#define AUX_USB_PORT_6_ITEM									(AUX_USB_PORT_PROPERTY->items + 5)

#define AUX_USB_PORT_STATE_PROPERTY					(PRIVATE_DATA->usb_port_state_property)
#define AUX_USB_PORT_STATE_1_ITEM						(AUX_USB_PORT_STATE_PROPERTY->items + 0)
#define AUX_USB_PORT_STATE_2_ITEM						(AUX_USB_PORT_STATE_PROPERTY->items + 1)
#define AUX_USB_PORT_STATE_3_ITEM						(AUX_USB_PORT_STATE_PROPERTY->items + 2)
#define AUX_USB_PORT_STATE_4_ITEM						(AUX_USB_PORT_STATE_PROPERTY->items + 3)
#define AUX_USB_PORT_STATE_5_ITEM						(AUX_USB_PORT_STATE_PROPERTY->items + 4)
#define AUX_USB_PORT_STATE_6_ITEM						(AUX_USB_PORT_STATE_PROPERTY->items + 5)

#define AUX_WEATHER_PROPERTY								(PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM				(AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM						(AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEVPOINT_ITEM						(AUX_WEATHER_PROPERTY->items + 2)

#define AUX_DEW_CONTROL_PROPERTY						(PRIVATE_DATA->heating_mode_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM					(AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM			(AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_INFO_PROPERTY										(PRIVATE_DATA->info_property)
#define X_AUX_AVERAGE_ITEM									(AUX_INFO_PROPERTY->items + 0)
#define X_AUX_AMP_HOUR_ITEM									(AUX_INFO_PROPERTY->items + 1)
#define X_AUX_WATT_HOUR_ITEM								(AUX_INFO_PROPERTY->items + 2)
#define X_AUX_VOLTAGE_ITEM									(AUX_INFO_PROPERTY->items + 3)
#define X_AUX_CURRENT_ITEM									(AUX_INFO_PROPERTY->items + 4)
#define X_AUX_POWER_ITEM										(AUX_INFO_PROPERTY->items + 5)

#define X_AUX_HUB_PROPERTY									(PRIVATE_DATA->hub_property)
#define X_AUX_HUB_ENABLED_ITEM							(X_AUX_HUB_PROPERTY->items + 0)
#define X_AUX_HUB_DISABLED_ITEM							(X_AUX_HUB_PROPERTY->items + 1)

#define X_AUX_REBOOT_PROPERTY								(PRIVATE_DATA->reboot_property)
#define X_AUX_REBOOT_ITEM										(X_AUX_REBOOT_PROPERTY->items + 0)

#define AUX_GROUP															"Powerbox"

typedef struct {
	int handle;
	pthread_mutex_t port_mutex;
	indigo_timer *aux_timer;
	indigo_timer *focuser_timer;
	indigo_property *outlet_names_property;
	indigo_property *power_outlet_property;
	indigo_property *power_outlet_state_property;
	indigo_property *power_outlet_current_property;
	indigo_property *heater_outlet_property;
	indigo_property *heater_outlet_state_property;
	indigo_property *heater_outlet_current_property;
	indigo_property *usb_port_property;
	indigo_property *usb_port_state_property;
	indigo_property *heating_mode_property;
	indigo_property *weather_property;
	indigo_property *info_property;
	indigo_property *hub_property;
	indigo_property *reboot_property;
	int count;
	libusb_device_handle *smart_hub;
} upb_private_data;

static bool upb_command(indigo_device *device, char *command, char *response, int max) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	indigo_write(PRIVATE_DATA->handle, "\n", 1);
	if (response != NULL) {
		if (indigo_read_line(PRIVATE_DATA->handle, response, max) == -1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> no response", command);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static void aux_timer_callback(indigo_device *device) {
	char response[128];
	bool updatePowerOutlet = false;
	bool updatePowerOutletState = false;
	bool updatePowerOutletCurrent = false;
	bool updateHeaterOutlet = false;
	bool updateHeaterOutletState = false;
	bool updateHeaterOutletCurrent = false;
	bool updateWeather = false;
	bool updateInfo = false;
	bool updateAutoHeater = false;
	bool updateHub = false;
	if (upb_command(device, "PA", response, sizeof(response))) {
		char *token = strtok(response, ":");
		if ((token = strtok(NULL, ":"))) { // Voltage
			double value = atof(token);
			if (X_AUX_VOLTAGE_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_VOLTAGE_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current
			double value = atof(token);
			if (X_AUX_CURRENT_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_CURRENT_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Power
			double value = atof(token);
			if (X_AUX_POWER_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_POWER_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Temp
			double value = atof(token);
			if (AUX_WEATHER_TEMPERATURE_ITEM->number.value != value) {
				updateWeather = true;
				AUX_WEATHER_TEMPERATURE_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Humidity
			double value = atof(token);
			if (AUX_WEATHER_HUMIDITY_ITEM->number.value != value) {
				updateWeather = true;
				AUX_WEATHER_HUMIDITY_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Dewpoint
			double value = atof(token);
			if (AUX_WEATHER_DEVPOINT_ITEM->number.value != value) {
				updateWeather = true;
				AUX_WEATHER_DEVPOINT_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // portstatus
			bool state = token[0] == '1';
			if (AUX_POWER_OUTLET_1_ITEM->sw.value != state) {
				AUX_POWER_OUTLET_1_ITEM->sw.value = state;
				updatePowerOutletState = true;
			}
			state = token[1] == '1';
			if (AUX_POWER_OUTLET_2_ITEM->sw.value != state) {
				AUX_POWER_OUTLET_2_ITEM->sw.value = state;
				updatePowerOutletState = true;
			}
			state = token[2] == '1';
			if (AUX_POWER_OUTLET_3_ITEM->sw.value != state) {
				AUX_POWER_OUTLET_3_ITEM->sw.value = state;
				updatePowerOutletState = true;
			}
			state = token[3] == '1';
			if (AUX_POWER_OUTLET_4_ITEM->sw.value != state) {
				AUX_POWER_OUTLET_4_ITEM->sw.value = state;
				updatePowerOutletState = true;
			}
		}
		if ((token = strtok(NULL, ":"))) { // USB Hub
			bool state = token[0] == '0';
			if (X_AUX_HUB_ENABLED_ITEM->sw.value != state) {
				indigo_set_switch(X_AUX_HUB_PROPERTY, state ? X_AUX_HUB_ENABLED_ITEM : X_AUX_HUB_DISABLED_ITEM, true);
				updateAutoHeater = true;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Dew1
			double value = round(atof(token) * 100.0 / 255.0);
			if (AUX_HEATER_OUTLET_1_ITEM->number.value != value) {
				updateHeaterOutlet = true;
				AUX_HEATER_OUTLET_1_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Dew2
			double value = round(atof(token) * 100.0 / 255.0);
			if (AUX_HEATER_OUTLET_2_ITEM->number.value != value) {
				updateHeaterOutlet = true;
				AUX_HEATER_OUTLET_2_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current_port1
			double value = atof(token) / 400.0;
			if (AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current_port2
			double value = atof(token) / 400.0;
			if (AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current_port2
			double value = atof(token) / 400.0;
			if (AUX_POWER_OUTLET_CURRENT_3_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_3_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current_port2
			double value = atof(token) / 400.0;
			if (AUX_POWER_OUTLET_CURRENT_4_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_4_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current_dew1
			double value = atof(token) / 400.0;
			if (AUX_HEATER_OUTLET_CURRENT_1_ITEM->number.value != value) {
				updateHeaterOutletCurrent = true;
				AUX_HEATER_OUTLET_CURRENT_1_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Current_dew2
			double value = atof(token) / 400.0;
			if (AUX_HEATER_OUTLET_CURRENT_2_ITEM->number.value != value) {
				updateHeaterOutletCurrent = true;
				AUX_HEATER_OUTLET_CURRENT_2_ITEM->number.value = value;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Overcurrent
			indigo_property_state state = token[0] == '1' ? INDIGO_ALERT_STATE : AUX_POWER_OUTLET_1_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			if (AUX_POWER_OUTLET_STATE_1_ITEM->light.value != state) {
				AUX_POWER_OUTLET_STATE_1_ITEM->light.value = state;
				updatePowerOutletState = true;
			}
			state = token[1] == '1' ? INDIGO_ALERT_STATE : AUX_POWER_OUTLET_2_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			if (AUX_POWER_OUTLET_STATE_2_ITEM->light.value != state) {
				AUX_POWER_OUTLET_STATE_2_ITEM->light.value = state;
				updatePowerOutletState = true;
			}
			state = token[2] == '1' ? INDIGO_ALERT_STATE : AUX_POWER_OUTLET_3_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			if (AUX_POWER_OUTLET_STATE_3_ITEM->light.value != state) {
				AUX_POWER_OUTLET_STATE_3_ITEM->light.value = state;
				updatePowerOutletState = true;
			}
			state = token[3] == '1' ? INDIGO_ALERT_STATE : AUX_POWER_OUTLET_4_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			if (AUX_POWER_OUTLET_STATE_4_ITEM->light.value != state) {
				AUX_POWER_OUTLET_STATE_4_ITEM->light.value = state;
				updatePowerOutletState = true;
			}
			state = token[4] == '1' ? INDIGO_ALERT_STATE : AUX_HEATER_OUTLET_1_ITEM->number.value > 0 ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			if (AUX_HEATER_OUTLET_STATE_1_ITEM->light.value != state) {
				AUX_HEATER_OUTLET_STATE_1_ITEM->light.value = state;
				updateHeaterOutletState = true;
			}
			state = token[5] == '1' ? INDIGO_ALERT_STATE : AUX_HEATER_OUTLET_2_ITEM->number.value > 0 ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
			if (AUX_HEATER_OUTLET_STATE_2_ITEM->light.value != state) {
				AUX_HEATER_OUTLET_STATE_2_ITEM->light.value = state;
				updateHeaterOutletState = true;
			}
		}
		if ((token = strtok(NULL, ":"))) { // Autodew
			bool state = token[0] == '1';
			if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value != state) {
				indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, state ? AUX_DEW_CONTROL_AUTOMATIC_ITEM : AUX_DEW_CONTROL_MANUAL_ITEM, true);
				updateAutoHeater = true;
			}
		}
	}
	if (upb_command(device, "PC", response, sizeof(response))) {
		char *token = strtok(response, ":");
		if (token) {
			double value = atof(token);
			if (X_AUX_AVERAGE_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_AVERAGE_ITEM->number.value = value;
			}
		}
		token = strtok(NULL, ":");
		if (token) {
			double value = atof(token);
			if (X_AUX_AMP_HOUR_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_AMP_HOUR_ITEM->number.value = value;
			}
		}
		token = strtok(NULL, ":");
		if (token) {
			double value = atof(token);
			if (X_AUX_WATT_HOUR_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_WATT_HOUR_ITEM->number.value = value;
			}
		}
	}
	if (updatePowerOutlet) {
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	}
	if (updatePowerOutletState) {
		AUX_POWER_OUTLET_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
	}
	if (updatePowerOutletCurrent) {
		AUX_POWER_OUTLET_CURRENT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
	}
	if (updateHeaterOutlet) {
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}
	if (updateHeaterOutletState) {
		AUX_HEATER_OUTLET_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
	}
	if (updateHeaterOutletCurrent) {
		AUX_HEATER_OUTLET_CURRENT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
	}
	if (updateAutoHeater) {
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	}
	if (updateWeather) {
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	}
	if (updateInfo) {
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	}
	if (updateHub) {
		X_AUX_HUB_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_AUX_HUB_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- OUTLET_NAMES
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, "X_AUX_OUTLET_NAMES", AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 12);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Outlet #1", "Outlet #1");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Outlet #2", "Outlet #2");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "Outlet #3", "Outlet #3");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "Outlet #4", "Outlet #4");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater #1", "Heater #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater #2", "Heater #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_1_ITEM, AUX_USB_PORT_NAME_1_ITEM_NAME, "Port #1", "Port #1");
		indigo_init_text_item(AUX_USB_PORT_NAME_2_ITEM, AUX_USB_PORT_NAME_2_ITEM_NAME, "Port #2", "Port #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_3_ITEM, AUX_USB_PORT_NAME_3_ITEM_NAME, "Port #3", "Port #3");
		indigo_init_text_item(AUX_USB_PORT_NAME_4_ITEM, AUX_USB_PORT_NAME_4_ITEM_NAME, "Port #4", "Port #4");
		indigo_init_text_item(AUX_USB_PORT_NAME_5_ITEM, AUX_USB_PORT_NAME_5_ITEM_NAME, "Port #5", "Port #5");
		indigo_init_text_item(AUX_USB_PORT_NAME_6_ITEM, AUX_USB_PORT_NAME_6_ITEM_NAME, "Port #6", "Port #6");
		// -------------------------------------------------------------------------------- POWER OUTLETS
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AUX_POWER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Outlet #3", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Outlet #4", true);
		AUX_POWER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_POWER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Power outlets state", INDIGO_OK_STATE, 4);
		if (AUX_POWER_OUTLET_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_1_ITEM, AUX_POWER_OUTLET_STATE_1_ITEM_NAME, "Outlet #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_2_ITEM, AUX_POWER_OUTLET_STATE_2_ITEM_NAME, "Outlet #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_3_ITEM, AUX_POWER_OUTLET_STATE_3_ITEM_NAME, "Outlet #3 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_4_ITEM, AUX_POWER_OUTLET_STATE_4_ITEM_NAME, "Outlet #4 state", INDIGO_OK_STATE);
		AUX_POWER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Power outlets current", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_POWER_OUTLET_CURRENT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_1_ITEM, AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME, "Outlet #1 current [A]", 0, 3, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_2_ITEM, AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME, "Outlet #2 current [A]", 0, 3, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_3_ITEM, AUX_POWER_OUTLET_CURRENT_3_ITEM_NAME, "Outlet #3 current [A]", 0, 3, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_4_ITEM, AUX_POWER_OUTLET_CURRENT_4_ITEM_NAME, "Outlet #4 current [A]", 0, 3, 0, 0);
		// -------------------------------------------------------------------------------- HEATER OUTLETS
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		AUX_HEATER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_HEATER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Heater outlets state", INDIGO_OK_STATE, 2);
		if (AUX_HEATER_OUTLET_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_1_ITEM, AUX_HEATER_OUTLET_STATE_1_ITEM_NAME, "Heater #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_2_ITEM, AUX_HEATER_OUTLET_STATE_2_ITEM_NAME, "Heater #2 state", INDIGO_OK_STATE);
		AUX_HEATER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Heater outlets current", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_HEATER_OUTLET_CURRENT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_1_ITEM, AUX_HEATER_OUTLET_CURRENT_1_ITEM_NAME, "Heater #1 current [A]", 0, 3, 0, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_2_ITEM, AUX_HEATER_OUTLET_CURRENT_2_ITEM_NAME, "Heater #2 current [A]", 0, 3, 0, 0);
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", true);
		// -------------------------------------------------------------------------------- USB PORTS
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 6);
		if (AUX_USB_PORT_PROPERTY == NULL)
		return INDIGO_FAILED;
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "Port #1", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "Port #2", true);
		indigo_init_switch_item(AUX_USB_PORT_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "Port #3", true);
		indigo_init_switch_item(AUX_USB_PORT_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "Port #4", true);
		indigo_init_switch_item(AUX_USB_PORT_5_ITEM, AUX_USB_PORT_5_ITEM_NAME, "Port #5", true);
		indigo_init_switch_item(AUX_USB_PORT_6_ITEM, AUX_USB_PORT_6_ITEM_NAME, "Port #6", true);
		AUX_USB_PORT_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_USB_PORT_STATE_PROPERTY_NAME, AUX_GROUP, "USB ports state", INDIGO_OK_STATE, 6);
		if (AUX_USB_PORT_STATE_PROPERTY == NULL)
		return INDIGO_FAILED;
		indigo_init_light_item(AUX_USB_PORT_STATE_1_ITEM, AUX_USB_PORT_STATE_1_ITEM_NAME, "Port #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_2_ITEM, AUX_USB_PORT_STATE_2_ITEM_NAME, "Port #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_3_ITEM, AUX_USB_PORT_STATE_3_ITEM_NAME, "Port #3 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_4_ITEM, AUX_USB_PORT_STATE_4_ITEM_NAME, "Port #4 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_5_ITEM, AUX_USB_PORT_STATE_5_ITEM_NAME, "Port #5 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_6_ITEM, AUX_USB_PORT_STATE_6_ITEM_NAME, "Port #6 state", INDIGO_OK_STATE);
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEVPOINT_ITEM, AUX_WEATHER_DEVPOINT_ITEM_NAME, "Devpoint [C]", -50, 100, 0, 0);
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_AUX_AVERAGE_ITEM, "X_AUX_AVERAGE", "Avereage current [A]", 0, 100, 0, 0);
		indigo_init_number_item(X_AUX_AMP_HOUR_ITEM, "X_AUX_AMP_HOUR", "Amp-hour [Ah]", 0, 100, 0, 0);
		indigo_init_number_item(X_AUX_WATT_HOUR_ITEM, "X_AUX_WATT_HOUR", "Watt-hour [Wh]", 0, 100, 0, 0);
		indigo_init_number_item(X_AUX_VOLTAGE_ITEM, "X_AUX_VOLTAGE", "Voltage [V]", 0, 15, 0, 0);
		indigo_init_number_item(X_AUX_CURRENT_ITEM, "X_AUX_CURRENT", "Current [A]", 0, 20, 0, 0);
		indigo_init_number_item(X_AUX_POWER_ITEM, "X_AUX_POWER_OUTLET", "Power [W]", 0, 200, 0, 0);
		// -------------------------------------------------------------------------------- UPB SPECIFIC
		X_AUX_HUB_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_AUX_HUB", AUX_GROUP, "USB hub", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_AUX_HUB_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_AUX_HUB_ENABLED_ITEM, "ENABLED", "Enabled", true);
		indigo_init_switch_item(X_AUX_HUB_DISABLED_ITEM, "DISABLED", "Disabled", false);
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_AUX_REBOOT", AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, "REBOOT", "Reboot", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUPB");
#endif
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_POWER_OUTLET_STATE_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		if (indigo_property_match(AUX_POWER_OUTLET_CURRENT_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_STATE_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_CURRENT_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		if (indigo_property_match(AUX_USB_PORT_PROPERTY, property))
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
		if (indigo_property_match(AUX_USB_PORT_STATE_PROPERTY, property))
			indigo_define_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		if (indigo_property_match(AUX_WEATHER_PROPERTY, property))
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
		if (indigo_property_match(AUX_INFO_PROPERTY, property))
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
		if (indigo_property_match(X_AUX_HUB_PROPERTY, property))
			indigo_define_property(device, X_AUX_HUB_PROPERTY, NULL);
		if (indigo_property_match(X_AUX_REBOOT_PROPERTY, property))
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	char command[16], response[128];
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			if (PRIVATE_DATA->count++ == 0) {
				PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
				if (PRIVATE_DATA->handle > 0) {
					if (upb_command(device, "P#", response, sizeof(response)) && !strcmp(response, "UPB_OK")) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "UPB not detected");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				if (upb_command(device, "PA", response, sizeof(response)) && !strncmp(response, "UPB", 3)) {
					char *token = strtok(response, ":");
					if ((token = strtok(NULL, ":"))) { // Voltage
						X_AUX_VOLTAGE_ITEM->number.value = atof(token);
					}
					if ((token = strtok(NULL, ":"))) { // Current
						X_AUX_CURRENT_ITEM->number.value = atof(token);
					}
					if ((token = strtok(NULL, ":"))) { // Power
						X_AUX_POWER_ITEM->number.value = atoi(token);
					}
					if ((token = strtok(NULL, ":"))) { // Temp
						AUX_WEATHER_TEMPERATURE_ITEM->number.value = atof(token);
					}
					if ((token = strtok(NULL, ":"))) { // Humidity
						AUX_WEATHER_HUMIDITY_ITEM->number.value = atof(token);
					}
					if ((token = strtok(NULL, ":"))) { // Dewpoint
						AUX_WEATHER_DEVPOINT_ITEM->number.value = atof(token);
					}
					if ((token = strtok(NULL, ":"))) { // portstatus
						AUX_POWER_OUTLET_1_ITEM->sw.value = token[0] == '1';
						AUX_POWER_OUTLET_2_ITEM->sw.value = token[1] == '1';
						AUX_POWER_OUTLET_3_ITEM->sw.value = token[2] == '1';
						AUX_POWER_OUTLET_4_ITEM->sw.value = token[3] == '1';
					}
					if ((token = strtok(NULL, ":"))) { // USB Hub
						indigo_set_switch(X_AUX_HUB_PROPERTY, atoi(token) == 0 ? X_AUX_HUB_ENABLED_ITEM : X_AUX_HUB_DISABLED_ITEM, true);
					}
					if ((token = strtok(NULL, ":"))) { // Dew1
						AUX_HEATER_OUTLET_1_ITEM->number.value = round(atof(token) * 100.0 / 255.0);
					}
					if ((token = strtok(NULL, ":"))) { // Dew2
						AUX_HEATER_OUTLET_2_ITEM->number.value = round(atof(token) * 100.0 / 255.0);
					}
					if ((token = strtok(NULL, ":"))) { // Current_port1
						AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = atoi(token) / 400.0;
					}
					if ((token = strtok(NULL, ":"))) { // Current_port2
						AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = atoi(token) / 400.0;
					}
					if ((token = strtok(NULL, ":"))) { // Current_port2
						AUX_POWER_OUTLET_CURRENT_3_ITEM->number.value = atoi(token) / 400.0;
					}
					if ((token = strtok(NULL, ":"))) { // Current_port2
						AUX_POWER_OUTLET_CURRENT_4_ITEM->number.value = atoi(token) / 400.0;
					}
					if ((token = strtok(NULL, ":"))) { // Current_dew1
						AUX_HEATER_OUTLET_CURRENT_1_ITEM->number.value = atoi(token) / 400.0;
					}
					if ((token = strtok(NULL, ":"))) { // Current_dew2
						AUX_HEATER_OUTLET_CURRENT_2_ITEM->number.value = atoi(token) / 400.0;
					}
					if ((token = strtok(NULL, ":"))) { // Overcurrent
						AUX_POWER_OUTLET_STATE_1_ITEM->light.value = token[0] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
						AUX_POWER_OUTLET_STATE_2_ITEM->light.value = token[1] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
						AUX_POWER_OUTLET_STATE_3_ITEM->light.value = token[2] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
						AUX_POWER_OUTLET_STATE_4_ITEM->light.value = token[3] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
						AUX_HEATER_OUTLET_STATE_1_ITEM->light.value = token[4] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
						AUX_HEATER_OUTLET_STATE_2_ITEM->light.value = token[5] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					}
					if ((token = strtok(NULL, ":"))) { // Autodew
						indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, atoi(token) == 1 ? AUX_DEW_CONTROL_AUTOMATIC_ITEM : AUX_DEW_CONTROL_MANUAL_ITEM, true);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'SA' response");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'SA' response");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				if (upb_command(device, "PV", response, sizeof(response)) ) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "UPB");
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response);
					indigo_update_property(device, INFO_PROPERTY, NULL);
				}
				indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
				indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
				indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
				indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
				indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
				indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
				indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
				indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
				indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
				upb_command(device, "PU:1", response, sizeof(response));
				indigo_set_switch(X_AUX_HUB_PROPERTY, X_AUX_HUB_ENABLED_ITEM, true);
				indigo_define_property(device, X_AUX_HUB_PROPERTY, NULL);
				indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
				
				libusb_context *ctx = NULL;
				libusb_device **usb_devices;
				struct libusb_device_descriptor descriptor;
				ssize_t total = libusb_get_device_list(ctx, &usb_devices);
				AUX_USB_PORT_PROPERTY->hidden = true;
				AUX_USB_PORT_STATE_PROPERTY->hidden = true;
				for (int i = 0; i < total; i++) {
					libusb_device *dev = usb_devices[i];
					if (libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == 0x0424 && descriptor.idProduct == 0x2517) {
						if (libusb_open(dev, &PRIVATE_DATA->smart_hub) == LIBUSB_SUCCESS) {
							AUX_USB_PORT_PROPERTY->hidden = false;
							AUX_USB_PORT_STATE_PROPERTY->hidden = false;
							indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
							indigo_define_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
						}
					}
				}
				PRIVATE_DATA->aux_timer = indigo_set_timer(device, 0, aux_timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
				PRIVATE_DATA->count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->aux_timer);
			char command[] = "PE:0000";
			if (AUX_POWER_OUTLET_1_ITEM->sw.value)
				command[3] = '1';
			if (AUX_POWER_OUTLET_2_ITEM->sw.value)
				command[4] = '1';
			if (AUX_POWER_OUTLET_3_ITEM->sw.value)
				command[5] = '1';
			if (AUX_POWER_OUTLET_4_ITEM->sw.value)
				command[6] = '1';
			upb_command(device, command, response, sizeof(response));
			indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_delete_property(device, X_AUX_HUB_PROPERTY, NULL);
			indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
			strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
			indigo_update_property(device, INFO_PROPERTY, NULL);
			if (--PRIVATE_DATA->count == 0) {
				if (PRIVATE_DATA->handle > 0) {
					upb_command(device, "PL:0", response, sizeof(response));
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_delete_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
		}
		snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_STATE_4_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_CURRENT_1_ITEM->label, INDIGO_NAME_SIZE, "%s current [A] ", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_CURRENT_2_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_CURRENT_3_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_CURRENT_4_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_CURRENT_1_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_CURRENT_2_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_USB_PORT_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
		snprintf(AUX_USB_PORT_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
		snprintf(AUX_USB_PORT_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_3_ITEM->text.value);
		snprintf(AUX_USB_PORT_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_4_ITEM->text.value);
		snprintf(AUX_USB_PORT_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_5_ITEM->text.value);
		snprintf(AUX_USB_PORT_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
		if (IS_CONNECTED) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		}
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		if (IS_CONNECTED) {
			indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
			upb_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? "P1:1" : "P1:0", response, sizeof(response));
			upb_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? "P2:1" : "P2:0", response, sizeof(response));
			upb_command(device, AUX_POWER_OUTLET_3_ITEM->sw.value ? "P3:1" : "P3:0", response, sizeof(response));
			upb_command(device, AUX_POWER_OUTLET_4_ITEM->sw.value ? "P4:1" : "P4:0", response, sizeof(response));
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
		if (IS_CONNECTED) {
			indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
			sprintf(command, "P5:%d", (int)(AUX_HEATER_OUTLET_1_ITEM->number.value * 255.0 / 100.0));
			upb_command(device, command, response, sizeof(response));
			sprintf(command, "P6:%d", (int)(AUX_HEATER_OUTLET_2_ITEM->number.value * 255.0 / 100.0));
			upb_command(device, command, response, sizeof(response));
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		if (IS_CONNECTED) {
			indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
			upb_command(device, AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? "PD:1" : "PD:0", response, sizeof(response));
			AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_AUX_HUB_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_HUB
		if (IS_CONNECTED) {
			indigo_property_copy_values(X_AUX_HUB_PROPERTY, property, false);
			upb_command(device, X_AUX_HUB_ENABLED_ITEM->sw.value ? "PU:1" : "PU:0", response, sizeof(response));
			X_AUX_HUB_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_AUX_HUB_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_AUX_REBOOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_REBOOT
		if (IS_CONNECTED) {
			indigo_property_copy_values(X_AUX_REBOOT_PROPERTY, property, false);
			if (X_AUX_REBOOT_ITEM->sw.value) {
				upb_command(device, "PF", response, sizeof(response));
				X_AUX_REBOOT_ITEM->sw.value = false;
				X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_USB_PORT_STATE_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_HUB_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	char response[128];
	if (upb_command(device, "ST", response, sizeof(response))) {
		double temp = atof(response);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	bool update = false;
	if (upb_command(device, "SP", response, sizeof(response))) {
		int pos = atoi(response);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			update = true;
		}
	}
	if (upb_command(device, "SI", response, sizeof(response))) {
		if (*response == '0') {
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
	indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->focuser_timer);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUPB");
#endif
		// -------------------------------------------------------------------------------- FOCUSER_ROTATION
		FOCUSER_ROTATION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 500;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = -9999999;
		FOCUSER_POSITION_ITEM->number.max = 9999999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	char command[16], response[128];
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			if (PRIVATE_DATA->count++ == 0) {
				PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
				if (PRIVATE_DATA->handle > 0) {
					if (upb_command(device, "P#", response, sizeof(response)) && !strcmp(response, "UPB_OK")) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "UPB not detected");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				if (upb_command(device, "SA", response, sizeof(response))) {
					char *token = strtok(response, ":");
					if (token) { // Stepper position
						FOCUSER_POSITION_ITEM->number.value = atol(token);
					}
					if ((token = strtok(NULL, ":"))) { // Motor is running
						FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
					}
					if ((token = strtok(NULL, ":"))) { // Motor Invert
						indigo_set_switch(FOCUSER_ROTATION_PROPERTY, *token == '1' ? FOCUSER_ROTATION_COUNTERCLOCKWISE_ITEM : FOCUSER_ROTATION_CLOCKWISE_ITEM, true);
					}
					if ((token = strtok(NULL, ":"))) { // Backlash Steps
						FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = atoi(token);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'SA' response");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'SA' response");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				if (upb_command(device, "PV", response, sizeof(response)) ) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "UPB");
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response);
					indigo_update_property(device, INFO_PROPERTY, NULL);
				}
				if (upb_command(device, "SS", response, sizeof(response)) ) {
					FOCUSER_SPEED_ITEM->number.value = atol(response);
				}
				upb_command(device, "PL:1", response, sizeof(response));
				PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0, focuser_timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
				PRIVATE_DATA->count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
			if (--PRIVATE_DATA->count == 0) {
				if (PRIVATE_DATA->handle > 0) {
					upb_command(device, "PL:0", response, sizeof(response));
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
			snprintf(command, sizeof(command), "SS:%d", (int)FOCUSER_SPEED_ITEM->number.value);
			if (upb_command(device, command, response, sizeof(response))) {
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		snprintf(command, sizeof(command), "SG:%d", (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1));
		if (upb_command(device, command, response, sizeof(response))) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
			snprintf(command, sizeof(command), "SM:%d", (int)FOCUSER_POSITION_ITEM->number.value);
			if (upb_command(device, command, response, sizeof(response))) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
			snprintf(command, sizeof(command), "SC:%d", (int)FOCUSER_POSITION_ITEM->number.value);
			if (upb_command(device, command,  response, sizeof(response))) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			if (upb_command(device, "SH", response, sizeof(response))) {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_ROTATION
	} else if (indigo_property_match(FOCUSER_ROTATION_PROPERTY, property)) {
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_ROTATION_PROPERTY, property, false);
			snprintf(command, sizeof(command), "SR:%d", (int)FOCUSER_ROTATION_CLOCKWISE_ITEM->sw.value? 0 : 1);
			if (upb_command(device, command, response, sizeof(response))) {
				FOCUSER_ROTATION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_ROTATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, FOCUSER_ROTATION_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		snprintf(command, sizeof(command), "SB:%d", (int)FOCUSER_BACKLASH_ITEM->number.value);
		if (upb_command(device, command, response, sizeof(response))) {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

indigo_result indigo_aux_upb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static upb_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Ultimate Powerbox",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"Ultimate Powerbox (focuser)",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "PegasusAstro Ultimate Powerbox", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(upb_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(upb_private_data));
			aux = malloc(sizeof(indigo_device));
			assert(aux != NULL);
			memcpy(aux, &aux_template, sizeof(indigo_device));
			aux->private_data = private_data;
			indigo_attach_device(aux);
			focuser = malloc(sizeof(indigo_device));
			assert(focuser != NULL);
			memcpy(focuser, &focuser_template, sizeof(indigo_device));
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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
