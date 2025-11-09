// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_aux_upb.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <indigo/indigo_usb_utils.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_upb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000018
#define DRIVER_NAME          "indigo_aux_upb"
#define DRIVER_LABEL         "PegasusAstro Ultimate Powerbox"
#define AUX_DEVICE_NAME      "Ultimate Powerbox"
#define FOCUSER_DEVICE_NAME  "Ultimate Powerbox (focuser)"
#define PRIVATE_DATA         ((upb_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Powerbox"

//- define

#pragma mark - Property definitions

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_NAME_2_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_NAME_3_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_NAME_4_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_HEATER_OUTLET_NAME_1_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_HEATER_OUTLET_NAME_2_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_HEATER_OUTLET_NAME_3_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_USB_PORT_NAME_1_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 7)
#define AUX_USB_PORT_NAME_2_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 8)
#define AUX_USB_PORT_NAME_3_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 9)
#define AUX_USB_PORT_NAME_4_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 10)
#define AUX_USB_PORT_NAME_5_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 11)
#define AUX_USB_PORT_NAME_6_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 12)

#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_STATE_PROPERTY (PRIVATE_DATA->aux_power_outlet_state_property)
#define AUX_POWER_OUTLET_STATE_1_ITEM   (AUX_POWER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_STATE_2_ITEM   (AUX_POWER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_STATE_3_ITEM   (AUX_POWER_OUTLET_STATE_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_STATE_4_ITEM   (AUX_POWER_OUTLET_STATE_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_CURRENT_PROPERTY (PRIVATE_DATA->aux_power_outlet_current_property)
#define AUX_POWER_OUTLET_CURRENT_1_ITEM   (AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_CURRENT_2_ITEM   (AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_CURRENT_3_ITEM   (AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_CURRENT_4_ITEM   (AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 3)

#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_STATE_PROPERTY (PRIVATE_DATA->aux_heater_outlet_state_property)
#define AUX_HEATER_OUTLET_STATE_1_ITEM   (AUX_HEATER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_STATE_2_ITEM   (AUX_HEATER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_STATE_3_ITEM   (AUX_HEATER_OUTLET_STATE_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_CURRENT_PROPERTY (PRIVATE_DATA->aux_heater_outlet_current_property)
#define AUX_HEATER_OUTLET_CURRENT_1_ITEM   (AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_CURRENT_2_ITEM   (AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_CURRENT_3_ITEM   (AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 2)

#define AUX_DEW_CONTROL_PROPERTY       (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_USB_PORT_PROPERTY          (PRIVATE_DATA->aux_usb_port_property)
#define AUX_USB_PORT_1_ITEM            (AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM            (AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM            (AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM            (AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM            (AUX_USB_PORT_PROPERTY->items + 4)
#define AUX_USB_PORT_6_ITEM            (AUX_USB_PORT_PROPERTY->items + 5)

#define AUX_USB_PORT_STATE_PROPERTY    (PRIVATE_DATA->aux_usb_port_state_property)
#define AUX_USB_PORT_STATE_1_ITEM      (AUX_USB_PORT_STATE_PROPERTY->items + 0)
#define AUX_USB_PORT_STATE_2_ITEM      (AUX_USB_PORT_STATE_PROPERTY->items + 1)
#define AUX_USB_PORT_STATE_3_ITEM      (AUX_USB_PORT_STATE_PROPERTY->items + 2)
#define AUX_USB_PORT_STATE_4_ITEM      (AUX_USB_PORT_STATE_PROPERTY->items + 3)
#define AUX_USB_PORT_STATE_5_ITEM      (AUX_USB_PORT_STATE_PROPERTY->items + 4)
#define AUX_USB_PORT_STATE_6_ITEM      (AUX_USB_PORT_STATE_PROPERTY->items + 5)

#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 2)

#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define X_AUX_AVERAGE_ITEM             (AUX_INFO_PROPERTY->items + 0)
#define X_AUX_AMP_HOUR_ITEM            (AUX_INFO_PROPERTY->items + 1)
#define X_AUX_WATT_HOUR_ITEM           (AUX_INFO_PROPERTY->items + 2)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 3)
#define AUX_INFO_CURRENT_ITEM          (AUX_INFO_PROPERTY->items + 4)
#define AUX_INFO_POWER_ITEM            (AUX_INFO_PROPERTY->items + 5)

#define X_AUX_AVERAGE_ITEM_NAME        "X_AUX_AVERAGE"
#define X_AUX_AMP_HOUR_ITEM_NAME       "X_AUX_AMP_HOUR"
#define X_AUX_WATT_HOUR_ITEM_NAME      "X_AUX_WATT_HOUR"

#define X_AUX_HUB_PROPERTY             (PRIVATE_DATA->x_aux_hub_property)
#define X_AUX_HUB_ENABLED_ITEM         (X_AUX_HUB_PROPERTY->items + 0)
#define X_AUX_HUB_DISABLED_ITEM        (X_AUX_HUB_PROPERTY->items + 1)

#define X_AUX_HUB_PROPERTY_NAME        "X_AUX_HUB"
#define X_AUX_HUB_ENABLED_ITEM_NAME    "ENABLED"
#define X_AUX_HUB_DISABLED_ITEM_NAME   "DISABLED"

#define X_AUX_REBOOT_PROPERTY          (PRIVATE_DATA->x_aux_reboot_property)
#define X_AUX_REBOOT_ITEM              (X_AUX_REBOOT_PROPERTY->items + 0)

#define X_AUX_REBOOT_PROPERTY_NAME     "X_AUX_REBOOT"
#define X_AUX_REBOOT_ITEM_NAME         "REBOOT"

#define X_AUX_VARIABLE_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->x_aux_variable_power_outlet_property)
#define X_AUX_VARIABLE_POWER_OUTLET_1_ITEM        (X_AUX_VARIABLE_POWER_OUTLET_PROPERTY->items + 0)

#define X_AUX_VARIABLE_POWER_OUTLET_PROPERTY_NAME "X_AUX_VARIABLE_POWER_OUTLET"
#define X_AUX_VARIABLE_POWER_OUTLET_1_ITEM_NAME   "OUTLET_1"

#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY (PRIVATE_DATA->aux_save_outlet_states_as_default_property)
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM     (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_power_outlet_property;
	indigo_property *aux_power_outlet_state_property;
	indigo_property *aux_power_outlet_current_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *aux_heater_outlet_state_property;
	indigo_property *aux_heater_outlet_current_property;
	indigo_property *aux_dew_control_property;
	indigo_property *aux_usb_port_property;
	indigo_property *aux_usb_port_state_property;
	indigo_property *aux_weather_property;
	indigo_property *aux_info_property;
	indigo_property *x_aux_hub_property;
	indigo_property *x_aux_reboot_property;
	indigo_property *x_aux_variable_power_outlet_property;
	indigo_property *aux_save_outlet_states_as_default_property;
	//+ data
	int version;
	libusb_device_handle *smart_hub;
	char response[128];
	//- data
} upb_private_data;

#pragma mark - Low level code

//+ code

static bool upb_command(indigo_device *device, char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
		if (result > 0) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool upb_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		bool ok = false;
		if (upb_command(device, "P#")) {
			if (!strcmp(PRIVATE_DATA->response, "UPB_OK")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro UPB");
				PRIVATE_DATA->version = 1;
				ok = true;
			} else if (!strcmp(PRIVATE_DATA->response, "UPB2_OK")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro UPBv2");
				PRIVATE_DATA->version = 2;
				ok = true;
			}
		}
		if (upb_command(device, "PV")) {
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			upb_command(device, "PL:1");
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void upb_close(indigo_device *device) {
	upb_command(device, "PL:0");
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
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
	bool updateUSBPorts = false;
	if (upb_command(device, "PA")) {
		char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
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
		if ((token = strtok_r(NULL, ":", &pnt))) { // Power
			double value = indigo_atod(token);
			if (AUX_INFO_POWER_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_POWER_ITEM->number.value = value;
			}
		}
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
		if ((token = strtok_r(NULL, ":", &pnt))) { // portstatus
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
		if (PRIVATE_DATA->version == 1) {
			if ((token = strtok_r(NULL, ":", &pnt))) { // USB Hub
				bool state = token[0] == '0';
				if (X_AUX_HUB_ENABLED_ITEM->sw.value != state) {
					indigo_set_switch(X_AUX_HUB_PROPERTY, state ? X_AUX_HUB_ENABLED_ITEM : X_AUX_HUB_DISABLED_ITEM, true);
					updateAutoHeater = true;
				}
			}
		}
		if (PRIVATE_DATA->version == 2) {
			if ((token = strtok_r(NULL, ":", &pnt))) { // USB status
				bool state = token[0] == '1';
				if (AUX_USB_PORT_1_ITEM->sw.value != state) {
					AUX_USB_PORT_1_ITEM->sw.value = state;
					updateUSBPorts = true;
				}
				state = token[1] == '1';
				if (AUX_USB_PORT_2_ITEM->sw.value != state) {
					AUX_USB_PORT_2_ITEM->sw.value = state;
					updateUSBPorts = true;
				}
				state = token[2] == '1';
				if (AUX_USB_PORT_3_ITEM->sw.value != state) {
					AUX_USB_PORT_3_ITEM->sw.value = state;
					updateUSBPorts = true;
				}
				state = token[3] == '1';
				if (AUX_USB_PORT_4_ITEM->sw.value != state) {
					AUX_USB_PORT_4_ITEM->sw.value = state;
					updateUSBPorts = true;
				}
				state = token[4] == '1';
				if (AUX_USB_PORT_5_ITEM->sw.value != state) {
					AUX_USB_PORT_5_ITEM->sw.value = state;
					updateUSBPorts = true;
				}
				state = token[5] == '1';
				if (AUX_USB_PORT_6_ITEM->sw.value != state) {
					AUX_USB_PORT_6_ITEM->sw.value = state;
					updateUSBPorts = true;
				}
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Dew1
			double value = round(indigo_atod(token) * 100.0 / 255.0);
			if (AUX_HEATER_OUTLET_1_ITEM->number.value != value) {
				updateHeaterOutlet = true;
				AUX_HEATER_OUTLET_1_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Dew2
			double value = round(indigo_atod(token) * 100.0 / 255.0);
			if (AUX_HEATER_OUTLET_2_ITEM->number.value != value) {
				updateHeaterOutlet = true;
				AUX_HEATER_OUTLET_2_ITEM->number.value = value;
			}
		}
		if (PRIVATE_DATA->version == 2) {
			if ((token = strtok_r(NULL, ":", &pnt))) { // Dew3
				double value = round(indigo_atod(token) * 100.0 / 255.0);
				if (AUX_HEATER_OUTLET_3_ITEM->number.value != value) {
					updateHeaterOutlet = true;
					AUX_HEATER_OUTLET_3_ITEM->number.value = value;
				}
			}
		}
		double div_by = 400.0;
		if (PRIVATE_DATA->version == 2) {
			div_by = 300.0;
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port1
			double value = indigo_atod(token) / div_by;
			if (AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port2
			double value = indigo_atod(token) / div_by;
			if (AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port2
			double value = indigo_atod(token) / div_by;
			if (AUX_POWER_OUTLET_CURRENT_3_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_3_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port2
			double value = indigo_atod(token) / div_by;
			if (AUX_POWER_OUTLET_CURRENT_4_ITEM->number.value != value) {
				updatePowerOutletCurrent = true;
				AUX_POWER_OUTLET_CURRENT_4_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current_dew1
			double value = indigo_atod(token) / div_by;
			if (AUX_HEATER_OUTLET_CURRENT_1_ITEM->number.value != value) {
				updateHeaterOutletCurrent = true;
				AUX_HEATER_OUTLET_CURRENT_1_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current_dew2
			double value = indigo_atod(token) / div_by;
			if (AUX_HEATER_OUTLET_CURRENT_2_ITEM->number.value != value) {
				updateHeaterOutletCurrent = true;
				AUX_HEATER_OUTLET_CURRENT_2_ITEM->number.value = value;
			}
		}
		if (PRIVATE_DATA->version == 2) {
			if ((token = strtok_r(NULL, ":", &pnt))) { // Current_dew3
				double value = indigo_atod(token) / 600.0;
				if (AUX_HEATER_OUTLET_CURRENT_3_ITEM->number.value != value) {
					updateHeaterOutletCurrent = true;
					AUX_HEATER_OUTLET_CURRENT_3_ITEM->number.value = value;
				}
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Overcurrent
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
			if (PRIVATE_DATA->version == 2) {
				state = token[6] == '1' ? INDIGO_ALERT_STATE : AUX_HEATER_OUTLET_3_ITEM->number.value > 0 ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
				if (AUX_HEATER_OUTLET_STATE_3_ITEM->light.value != state) {
					AUX_HEATER_OUTLET_STATE_3_ITEM->light.value = state;
					updateHeaterOutletState = true;
				}
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Autodew
			bool state = token[0] == '1';
			if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value != state) {
				indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, state ? AUX_DEW_CONTROL_AUTOMATIC_ITEM : AUX_DEW_CONTROL_MANUAL_ITEM, true);
				updateAutoHeater = true;
			}
		}
	}
	if (upb_command(device, "PC")) {
		char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
		if (token) {
			double value = indigo_atod(token);
			if (X_AUX_AVERAGE_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_AVERAGE_ITEM->number.value = value;
			}
		}
		token = strtok_r(NULL, ":", &pnt);
		if (token) {
			double value = indigo_atod(token);
			if (X_AUX_AMP_HOUR_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_AMP_HOUR_ITEM->number.value = value;
			}
		}
		token = strtok_r(NULL, ":", &pnt);
		if (token) {
			double value = indigo_atod(token);
			if (X_AUX_WATT_HOUR_ITEM->number.value != value) {
				updateInfo = true;
				X_AUX_WATT_HOUR_ITEM->number.value = value;
			}
		}
	}
	if (PRIVATE_DATA->version == 1) {
		if (PRIVATE_DATA->smart_hub) {
			for (int i = 1; i < 7; i++) {
				uint32_t port_state;
				int rc;
				if ((rc = libusb_control_transfer(PRIVATE_DATA->smart_hub, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER, LIBUSB_REQUEST_GET_STATUS, 0, i, (unsigned char*)&port_state, sizeof(port_state), 3000)) == sizeof(port_state)) {
					bool is_enabled = port_state & 0x00000100;
					indigo_property_state state = INDIGO_IDLE_STATE;
					if (port_state & 0x00000015) {
						state = INDIGO_BUSY_STATE;
					}
					if (port_state & 0x00000002) {
						state = INDIGO_OK_STATE;
					}
					if (port_state & 0x00000008) {
						state = INDIGO_ALERT_STATE;
					}
					if (AUX_USB_PORT_PROPERTY->items[i - 1].sw.value != is_enabled || AUX_USB_PORT_STATE_PROPERTY->items[i - 1].light.value != state) {
						AUX_USB_PORT_PROPERTY->items[i - 1].sw.value = is_enabled;
						AUX_USB_PORT_STATE_PROPERTY->items[i - 1].light.value = state;
						updateUSBPorts = true;
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get USB port status (%s)", libusb_strerror(rc));
					break;
				}
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
	if (updateUSBPorts) {
		AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		AUX_USB_PORT_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 2, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = upb_open(device);
		}
		if (connection_result) {
			//+ aux.on_connect
			if (PRIVATE_DATA->version == 1) {
				AUX_HEATER_OUTLET_PROPERTY->count = 2;
				AUX_HEATER_OUTLET_STATE_PROPERTY->count = 2;
				AUX_HEATER_OUTLET_CURRENT_PROPERTY->count = 2;
				X_AUX_VARIABLE_POWER_OUTLET_PROPERTY->hidden = true;
			} else {
				AUX_HEATER_OUTLET_PROPERTY->count = 3;
				AUX_HEATER_OUTLET_STATE_PROPERTY->count = 3;
				AUX_HEATER_OUTLET_CURRENT_PROPERTY->count = 3;
				X_AUX_VARIABLE_POWER_OUTLET_PROPERTY->hidden = false;
				AUX_USB_PORT_PROPERTY->hidden = false;
				AUX_USB_PORT_STATE_PROPERTY->hidden = true;
			}
			if (upb_command(device, "PA") && !strncmp(PRIVATE_DATA->response, "UPB", 3)) {
				char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
				if ((token = strtok_r(NULL, ":", &pnt))) { // Voltage
					AUX_INFO_VOLTAGE_ITEM->number.value = indigo_atod(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current
					AUX_INFO_CURRENT_ITEM->number.value = indigo_atod(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Power
					AUX_INFO_POWER_ITEM->number.value = atoi(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Temp
					AUX_WEATHER_TEMPERATURE_ITEM->number.value = indigo_atod(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Humidity
					AUX_WEATHER_HUMIDITY_ITEM->number.value = indigo_atod(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Dewpoint
					AUX_WEATHER_DEWPOINT_ITEM->number.value = indigo_atod(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // portstatus
					AUX_POWER_OUTLET_1_ITEM->sw.value = token[0] == '1';
					AUX_POWER_OUTLET_2_ITEM->sw.value = token[1] == '1';
					AUX_POWER_OUTLET_3_ITEM->sw.value = token[2] == '1';
					AUX_POWER_OUTLET_4_ITEM->sw.value = token[3] == '1';
				}
				if (PRIVATE_DATA->version == 1) {
					if ((token = strtok_r(NULL, ":", &pnt))) { // USB Hub
						indigo_set_switch(X_AUX_HUB_PROPERTY, atoi(token) == 0 ? X_AUX_HUB_ENABLED_ITEM : X_AUX_HUB_DISABLED_ITEM, true);
					}
				}
				if (PRIVATE_DATA->version == 2) {
					if ((token = strtok_r(NULL, ":", &pnt))) { // USB status
						AUX_USB_PORT_1_ITEM->sw.value =  token[0] == '1';
						AUX_USB_PORT_2_ITEM->sw.value =  token[1] == '1';
						AUX_USB_PORT_3_ITEM->sw.value =  token[2] == '1';
						AUX_USB_PORT_4_ITEM->sw.value =  token[3] == '1';
						AUX_USB_PORT_5_ITEM->sw.value =  token[4] == '1';
						AUX_USB_PORT_6_ITEM->sw.value =  token[5] == '1';
					}
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Dew1
					AUX_HEATER_OUTLET_1_ITEM->number.value = AUX_HEATER_OUTLET_1_ITEM->number.target = round(indigo_atod(token) * 100.0 / 255.0);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Dew2
					AUX_HEATER_OUTLET_2_ITEM->number.value = AUX_HEATER_OUTLET_2_ITEM->number.target = round(indigo_atod(token) * 100.0 / 255.0);
				}
				if (PRIVATE_DATA->version == 2) {
					if ((token = strtok_r(NULL, ":", &pnt))) { // Dew3
						AUX_HEATER_OUTLET_3_ITEM->number.value = AUX_HEATER_OUTLET_3_ITEM->number.target = round(indigo_atod(token) * 100.0 / 255.0);
					}
				}
				double div_by = 400.0;
				if (PRIVATE_DATA->version == 2) {
					div_by = 300.0;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port1
					AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = atoi(token) / div_by;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port2
					AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = atoi(token) / div_by;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port2
					AUX_POWER_OUTLET_CURRENT_3_ITEM->number.value = atoi(token) / div_by;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current_port2
					AUX_POWER_OUTLET_CURRENT_4_ITEM->number.value = atoi(token) / div_by;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current_dew1
					AUX_HEATER_OUTLET_CURRENT_1_ITEM->number.value = atoi(token) / div_by;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current_dew2
					AUX_HEATER_OUTLET_CURRENT_2_ITEM->number.value = atoi(token) / div_by;
				}
				if (PRIVATE_DATA->version == 2) {
					if ((token = strtok_r(NULL, ":", &pnt))) { // Current_dew3
						AUX_HEATER_OUTLET_CURRENT_3_ITEM->number.value = atoi(token) / 600.0;
					}
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Overcurrent
					AUX_POWER_OUTLET_STATE_1_ITEM->light.value = token[0] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					AUX_POWER_OUTLET_STATE_2_ITEM->light.value = token[1] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					AUX_POWER_OUTLET_STATE_3_ITEM->light.value = token[2] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					AUX_POWER_OUTLET_STATE_4_ITEM->light.value = token[3] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					AUX_HEATER_OUTLET_STATE_1_ITEM->light.value = token[4] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					AUX_HEATER_OUTLET_STATE_2_ITEM->light.value = token[5] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					if (PRIVATE_DATA->version == 2) {
						AUX_HEATER_OUTLET_STATE_3_ITEM->light.value = token[6] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
					}
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Autodew
					indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, atoi(token) == 0 ? AUX_DEW_CONTROL_MANUAL_ITEM : AUX_DEW_CONTROL_AUTOMATIC_ITEM, true);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'PA' response");
					indigo_uni_close(&PRIVATE_DATA->handle);
				}
			}
			upb_command(device, "PU:1");
			indigo_set_switch(X_AUX_HUB_PROPERTY, X_AUX_HUB_ENABLED_ITEM, true);
			if (PRIVATE_DATA->version == 1) {
				libusb_context *ctx = NULL;
				libusb_device **usb_devices;
				struct libusb_device_descriptor descriptor;
				ssize_t total = libusb_get_device_list(ctx, &usb_devices);
				AUX_USB_PORT_PROPERTY->hidden = true;
				AUX_USB_PORT_STATE_PROPERTY->hidden = true;
				for (int i = 0; i < total; i++) {
					libusb_device *dev = usb_devices[i];
					if (libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == 0x0424 && descriptor.idProduct == 0x2517) {
						int rc;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "USB hub found");
						if ((rc = libusb_open(dev, &PRIVATE_DATA->smart_hub)) == LIBUSB_SUCCESS) {
							for (int i = 1; i < 7; i++) {
								uint32_t port_state;
								if ((rc = libusb_control_transfer(PRIVATE_DATA->smart_hub, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER, LIBUSB_REQUEST_GET_STATUS, 0, i, (unsigned char*)&port_state, sizeof(port_state), 3000)) == sizeof(port_state)) {
									indigo_property_state state = INDIGO_IDLE_STATE;
									if (port_state & 0x00000015) {
										state = INDIGO_BUSY_STATE;
									}
									if (port_state & 0x00000002) {
										state = INDIGO_OK_STATE;
									}
									if (port_state & 0x00000008) {
										state = INDIGO_ALERT_STATE;
									}
									AUX_USB_PORT_PROPERTY->items[i - 1].sw.value = port_state & 0x00000100;
									AUX_USB_PORT_STATE_PROPERTY->items[i - 1].light.value = state;
								} else {
									INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get USB port status (%s)", libusb_strerror(rc));
									break;
								}
							}
							AUX_USB_PORT_PROPERTY->hidden = false;
							AUX_USB_PORT_STATE_PROPERTY->hidden = false;
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to USB hub (%s)", libusb_strerror(rc));
						}
						break;
					}
				}
				libusb_free_device_list(usb_devices, 1);
			}
			if (PRIVATE_DATA->version == 2) {
				if (upb_command(device, "PS") && !strncmp(PRIVATE_DATA->response, "PS", 2)) {
					char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
					if ((token = strtok_r(NULL, ":", &pnt))) { // Power-up state
					}
					if ((token = strtok_r(NULL, ":", &pnt))) { // Variable voltage
						X_AUX_VARIABLE_POWER_OUTLET_1_ITEM->number.target = X_AUX_VARIABLE_POWER_OUTLET_1_ITEM->number.value = atoi(token);
					}
				}
			}
			//- aux.on_connect
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_HUB_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		if (PRIVATE_DATA->smart_hub) {
			libusb_close(PRIVATE_DATA->smart_hub);
			PRIVATE_DATA->smart_hub = 0;
		}
		//- aux.on_disconnect
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_HUB_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			upb_close(device);
		}
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_STATE_4_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_CURRENT_1_ITEM->label, INDIGO_NAME_SIZE, "%s current [A] ", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_CURRENT_2_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_CURRENT_3_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_CURRENT_4_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_CURRENT_1_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_CURRENT_2_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_CURRENT_3_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_USB_PORT_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
	snprintf(AUX_USB_PORT_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
	snprintf(AUX_USB_PORT_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_3_ITEM->text.value);
	snprintf(AUX_USB_PORT_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_4_ITEM->text.value);
	snprintf(AUX_USB_PORT_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_5_ITEM->text.value);
	snprintf(AUX_USB_PORT_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
	snprintf(AUX_USB_PORT_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
	snprintf(AUX_USB_PORT_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
	snprintf(AUX_USB_PORT_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_3_ITEM->text.value);
	snprintf(AUX_USB_PORT_STATE_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_4_ITEM->text.value);
	snprintf(AUX_USB_PORT_STATE_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_5_ITEM->text.value);
	snprintf(AUX_USB_PORT_STATE_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_define_property(device, AUX_USB_PORT_STATE_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_power_outlet_handler(indigo_device *device) {
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	upb_command(device, "P1:%d", AUX_POWER_OUTLET_1_ITEM->sw.value ? 1 : 0);
	upb_command(device, "P2:%d", AUX_POWER_OUTLET_2_ITEM->sw.value ? 1 : 0);
	upb_command(device, "P3:%d", AUX_POWER_OUTLET_3_ITEM->sw.value ? 1 : 0);
	upb_command(device, "P4:%d", AUX_POWER_OUTLET_4_ITEM->sw.value ? 1 : 0);
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	upb_command(device, "P5:%d", (int)(AUX_HEATER_OUTLET_1_ITEM->number.value * 255.0 / 100.0));
	upb_command(device, "P6:%d", (int)(AUX_HEATER_OUTLET_2_ITEM->number.value * 255.0 / 100.0));
	if (PRIVATE_DATA->version == 2) {
		upb_command(device, "P7:%d", (int)(AUX_HEATER_OUTLET_3_ITEM->number.value * 255.0 / 100.0));
	}
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void aux_dew_control_handler(indigo_device *device) {
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DEW_CONTROL.on_change
	upb_command(device, "PD:%d", AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? 1 : 0);
	//- aux.AUX_DEW_CONTROL.on_change
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
}

static void aux_usb_port_handler(indigo_device *device) {
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_USB_PORT.on_change
	if (PRIVATE_DATA->version == 1) {
		if (PRIVATE_DATA->smart_hub) {
			for (int i = 1; i < 7; i++) {
				uint32_t port_state;
				int rc;
				if ((rc = libusb_control_transfer(PRIVATE_DATA->smart_hub, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER, LIBUSB_REQUEST_GET_STATUS, 0, i, (unsigned char*)&port_state, sizeof(port_state), 3000)) == sizeof(port_state)) {
					bool is_enabled = port_state & 0x00000100;
					if (AUX_USB_PORT_PROPERTY->items[i - 1].sw.value != is_enabled) {
						if (AUX_USB_PORT_PROPERTY->items[i - 1].sw.value) {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Turning port #%d on", i);
							rc = libusb_control_transfer(PRIVATE_DATA->smart_hub, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER, LIBUSB_REQUEST_SET_FEATURE, 8, i, NULL, 0, 3000);
						} else {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Turning port #%d off", i);
							rc = libusb_control_transfer(PRIVATE_DATA->smart_hub, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER, LIBUSB_REQUEST_CLEAR_FEATURE, 8, i, NULL, 0, 3000);
						}
						if (rc < 0) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set USB port status (%s)", libusb_strerror(rc));
							AUX_USB_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
							break;
						}
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get USB port status (%s)", libusb_strerror(rc));
					AUX_USB_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
			}
		} else {
			AUX_USB_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	if (PRIVATE_DATA->version == 2) {
		for (int i = 0; i < AUX_USB_PORT_PROPERTY->count; i++) {
			upb_command(device, "U%d:%d", i + 1, AUX_USB_PORT_PROPERTY->items[i].sw.value ? 1 : 0);
		}
	}
	//- aux.AUX_USB_PORT.on_change
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
}

static void aux_x_aux_hub_handler(indigo_device *device) {
	X_AUX_HUB_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_HUB.on_change
	upb_command(device, "PU:%d", X_AUX_HUB_ENABLED_ITEM->sw.value ? 1 : 0);
	//- aux.X_AUX_HUB.on_change
	indigo_update_property(device, X_AUX_HUB_PROPERTY, NULL);
}

static void aux_x_aux_reboot_handler(indigo_device *device) {
	X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_REBOOT.on_change
	if (X_AUX_REBOOT_ITEM->sw.value) {
		upb_command(device, "PF");
		X_AUX_REBOOT_ITEM->sw.value = false;
	}
	//- aux.X_AUX_REBOOT.on_change
	indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
}

static void aux_x_aux_variable_power_outlet_handler(indigo_device *device) {
	X_AUX_VARIABLE_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_VARIABLE_POWER_OUTLET.on_change
	upb_command(device, "P8:%d",(int)X_AUX_VARIABLE_POWER_OUTLET_1_ITEM->number.target);
	//- aux.X_AUX_VARIABLE_POWER_OUTLET.on_change
	indigo_update_property(device, X_AUX_VARIABLE_POWER_OUTLET_PROPERTY, NULL);
}

static void aux_save_outlet_states_as_default_handler(indigo_device *device) {
	AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SAVE_OUTLET_STATES_AS_DEFAULT.on_change
	if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value) {
		char command[] = "PE:0000";
		char *port_mask = command + 3;
		for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
			port_mask[i] = AUX_POWER_OUTLET_PROPERTY->items[i].sw.value ? '1' : '0';
		}
		upb_command(device, command);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value = false;
	}
	//- aux.AUX_SAVE_OUTLET_STATES_AS_DEFAULT.on_change
	indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 13);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Outlet #1", "Outlet #1");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Outlet #2", "Outlet #2");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "Outlet #3", "Outlet #3");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "Outlet #4", "Outlet #4");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater #1", "Heater #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater #2", "Heater #2");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater #3", "Heater #3");
		indigo_init_text_item(AUX_USB_PORT_NAME_1_ITEM, AUX_USB_PORT_NAME_1_ITEM_NAME, "Port #1", "Port #1");
		indigo_init_text_item(AUX_USB_PORT_NAME_2_ITEM, AUX_USB_PORT_NAME_2_ITEM_NAME, "Port #2", "Port #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_3_ITEM, AUX_USB_PORT_NAME_3_ITEM_NAME, "Port #3", "Port #3");
		indigo_init_text_item(AUX_USB_PORT_NAME_4_ITEM, AUX_USB_PORT_NAME_4_ITEM_NAME, "Port #4", "Port #4");
		indigo_init_text_item(AUX_USB_PORT_NAME_5_ITEM, AUX_USB_PORT_NAME_5_ITEM_NAME, "Port #5", "Port #5");
		indigo_init_text_item(AUX_USB_PORT_NAME_6_ITEM, AUX_USB_PORT_NAME_6_ITEM_NAME, "Port #6", "Port #6");
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Outlet #3", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Outlet #4", true);
		AUX_POWER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_POWER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Power outlets state", INDIGO_OK_STATE, 4);
		if (AUX_POWER_OUTLET_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_1_ITEM, AUX_POWER_OUTLET_STATE_1_ITEM_NAME, "Outlet #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_2_ITEM, AUX_POWER_OUTLET_STATE_2_ITEM_NAME, "Outlet #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_3_ITEM, AUX_POWER_OUTLET_STATE_3_ITEM_NAME, "Outlet #3 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_4_ITEM, AUX_POWER_OUTLET_STATE_4_ITEM_NAME, "Outlet #4 state", INDIGO_OK_STATE);
		AUX_POWER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Power outlets current", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_POWER_OUTLET_CURRENT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_1_ITEM, AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME, "Outlet #1 current [A]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_2_ITEM, AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME, "Outlet #2 current [A]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_3_ITEM, AUX_POWER_OUTLET_CURRENT_3_ITEM_NAME, "Outlet #3 current [A]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_4_ITEM, AUX_POWER_OUTLET_CURRENT_4_ITEM_NAME, "Outlet #4 current [A]", 0, 0, 0, 0);
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater #3 [%]", 0, 100, 5, 0);
		AUX_HEATER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_HEATER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Heater outlets state", INDIGO_OK_STATE, 3);
		if (AUX_HEATER_OUTLET_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_1_ITEM, AUX_HEATER_OUTLET_STATE_1_ITEM_NAME, "Heater #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_2_ITEM, AUX_HEATER_OUTLET_STATE_2_ITEM_NAME, "Heater #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_3_ITEM, AUX_HEATER_OUTLET_STATE_3_ITEM_NAME, "Heater #3 state", INDIGO_OK_STATE);
		AUX_HEATER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Heater outlets current", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_HEATER_OUTLET_CURRENT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_1_ITEM, AUX_HEATER_OUTLET_CURRENT_1_ITEM_NAME, "Heater #1 current [A]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_2_ITEM, AUX_HEATER_OUTLET_CURRENT_2_ITEM_NAME, "Heater #2 current [A]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_3_ITEM, AUX_HEATER_OUTLET_CURRENT_3_ITEM_NAME, "Heater #3 current [A]", 0, 0, 0, 0);
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", true);
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 6);
		if (AUX_USB_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "Port #1", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "Port #2", true);
		indigo_init_switch_item(AUX_USB_PORT_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "Port #3", true);
		indigo_init_switch_item(AUX_USB_PORT_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "Port #4", true);
		indigo_init_switch_item(AUX_USB_PORT_5_ITEM, AUX_USB_PORT_5_ITEM_NAME, "Port #5", true);
		indigo_init_switch_item(AUX_USB_PORT_6_ITEM, AUX_USB_PORT_6_ITEM_NAME, "Port #6", true);
		AUX_USB_PORT_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_USB_PORT_STATE_PROPERTY_NAME, AUX_GROUP, "USB ports state", INDIGO_OK_STATE, 6);
		if (AUX_USB_PORT_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_USB_PORT_STATE_1_ITEM, AUX_USB_PORT_STATE_1_ITEM_NAME, "Port #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_2_ITEM, AUX_USB_PORT_STATE_2_ITEM_NAME, "Port #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_3_ITEM, AUX_USB_PORT_STATE_3_ITEM_NAME, "Port #3 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_4_ITEM, AUX_USB_PORT_STATE_4_ITEM_NAME, "Port #4 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_5_ITEM, AUX_USB_PORT_STATE_5_ITEM_NAME, "Port #5 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_USB_PORT_STATE_6_ITEM, AUX_USB_PORT_STATE_6_ITEM_NAME, "Port #6 state", INDIGO_OK_STATE);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [C]", 0, 0, 0, 0);
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_AUX_AVERAGE_ITEM, X_AUX_AVERAGE_ITEM_NAME, "Avereage current [A]", 0, 0, 0, 0);
		indigo_init_number_item(X_AUX_AMP_HOUR_ITEM, X_AUX_AMP_HOUR_ITEM_NAME, "Amp-hour [Ah]", 0, 0, 0, 0);
		indigo_init_number_item(X_AUX_WATT_HOUR_ITEM, X_AUX_WATT_HOUR_ITEM_NAME, "Watt-hour [Wh]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_POWER_ITEM, AUX_INFO_POWER_ITEM_NAME, "Power [W]", 0, 0, 0, 0);
		X_AUX_HUB_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_HUB_PROPERTY_NAME, AUX_GROUP, "USB hub", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_AUX_HUB_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_HUB_ENABLED_ITEM, X_AUX_HUB_ENABLED_ITEM_NAME, "Enabled", true);
		indigo_init_switch_item(X_AUX_HUB_DISABLED_ITEM, X_AUX_HUB_DISABLED_ITEM_NAME, "Disabled", false);
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_REBOOT_PROPERTY_NAME, AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, X_AUX_REBOOT_ITEM_NAME, "Reboot", false);
		X_AUX_VARIABLE_POWER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, X_AUX_VARIABLE_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Variable voltage power outlet", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_AUX_VARIABLE_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_AUX_VARIABLE_POWER_OUTLET_1_ITEM, X_AUX_VARIABLE_POWER_OUTLET_1_ITEM_NAME, "Variable voltage power outlet ", 3, 12, 1, 12);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY_NAME, AUX_GROUP, "Save current outlet states as default", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM_NAME, "Save", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_CURRENT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_CURRENT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_CONTROL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_USB_PORT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_USB_PORT_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_HUB_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_REBOOT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_VARIABLE_POWER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_PROPERTY, aux_power_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_OUTLET_PROPERTY, aux_heater_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DEW_CONTROL_PROPERTY, aux_dew_control_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_USB_PORT_PROPERTY, aux_usb_port_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_HUB_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_HUB_PROPERTY, aux_x_aux_hub_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_REBOOT_PROPERTY, aux_x_aux_reboot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_VARIABLE_POWER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_VARIABLE_POWER_OUTLET_PROPERTY, aux_x_aux_variable_power_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, aux_save_outlet_states_as_default_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
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
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_USB_PORT_STATE_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_HUB_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(X_AUX_VARIABLE_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (upb_command(device, "ST")) {
		double temp = indigo_atod(PRIVATE_DATA->response);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	bool update = false;
	if (upb_command(device, "SP")) {
		int pos = atoi(PRIVATE_DATA->response);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			update = true;
		}
	}
	if (upb_command(device, "SI")) {
		if (PRIVATE_DATA->response[0] == '0') {
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
	indigo_execute_handler_in(device, 0.5, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = upb_open(device->master_device);
		}
		if (connection_result) {
			//+ focuser.on_connect
			if (upb_command(device, "SA")) {
				char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
				if (token) { // Stepper position
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atol(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Motor is running
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Motor Invert
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, *token == '1' ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Backlash Steps
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = atoi(token);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'SA' response");
					indigo_uni_close(&PRIVATE_DATA->handle);
				}
			}
			if (upb_command(device, "SS")) {
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = atol(PRIVATE_DATA->response);
			}
			//- focuser.on_connect
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			upb_close(device);
		}
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!upb_command(device, "SB:%d", (int)FOCUSER_BACKLASH_ITEM->number.value)) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	if (!upb_command(device, "SR:%d", (int)FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value? 0 : 1)) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_temperature_handler(indigo_device *device) {
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!upb_command(device, "SS:%d", (int)FOCUSER_SPEED_ITEM->number.value)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
		if (position + FOCUSER_STEPS_ITEM->number.target > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value) {
			FOCUSER_STEPS_ITEM->number.value = FOCUSER_STEPS_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value - position;
		}
	} else {
		if (position - FOCUSER_STEPS_ITEM->number.target < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value) {
			FOCUSER_STEPS_ITEM->number.value = FOCUSER_STEPS_ITEM->number.target = position - FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
		}
	}
	if (upb_command(device, "SG:%d", (int)FOCUSER_STEPS_ITEM->number.target * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1))) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_on_position_set_handler(indigo_device *device) {
	FOCUSER_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		int position = (int)FOCUSER_POSITION_ITEM->number.target;
		if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value) {
			position = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
		}
		if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value) {
			position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
		}
		FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
		if (upb_command(device, "SM:%d", position)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		if (!upb_command(device, "SC:%d", (int)FOCUSER_POSITION_ITEM->number.target)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (upb_command(device, "SH")) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = -9999999;
		strcpy(FOCUSER_LIMITS_MIN_POSITION_ITEM->number.format, "%.0f");
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 9999999;
		strcpy(FOCUSER_LIMITS_MAX_POSITION_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = -9999999;
		FOCUSER_POSITION_ITEM->number.max = 9999999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		strcpy(FOCUSER_POSITION_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_TEMPERATURE_PROPERTY, focuser_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ON_POSITION_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ON_POSITION_SET_PROPERTY, focuser_on_position_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_aux_upb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static upb_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "UPB");
			patterns[0].vendor_id = 0x0403;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(upb_private_data));
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
