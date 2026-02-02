// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_aux_ppb.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_ppb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300001B
#define DRIVER_NAME          "indigo_aux_ppb"
#define DRIVER_LABEL         "PegasusAstro Pocket Powerbox"
#define AUX_DEVICE_NAME      "Pocket Powerbox"
#define PRIVATE_DATA         ((ppb_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Powerbox"

//- define

#pragma mark - Property definitions

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_HEATER_OUTLET_NAME_1_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_2_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 1)

#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)

#define AUX_DSLR_POWER_PROPERTY        (PRIVATE_DATA->aux_dslr_power_property)
#define AUX_DSLR_POWER_3_ITEM          (AUX_DSLR_POWER_PROPERTY->items + 0)
#define AUX_DSLR_POWER_5_ITEM          (AUX_DSLR_POWER_PROPERTY->items + 1)
#define AUX_DSLR_POWER_8_ITEM          (AUX_DSLR_POWER_PROPERTY->items + 2)
#define AUX_DSLR_POWER_9_ITEM          (AUX_DSLR_POWER_PROPERTY->items + 3)
#define AUX_DSLR_POWER_12_ITEM         (AUX_DSLR_POWER_PROPERTY->items + 4)

#define AUX_DSLR_POWER_PROPERTY_NAME   "X_DSLR_POWER"
#define AUX_DSLR_POWER_3_ITEM_NAME     "3"
#define AUX_DSLR_POWER_5_ITEM_NAME     "5"
#define AUX_DSLR_POWER_8_ITEM_NAME     "8"
#define AUX_DSLR_POWER_9_ITEM_NAME     "9"
#define AUX_DSLR_POWER_12_ITEM_NAME    "12"

#define AUX_POWER_OUTLET_STATE_PROPERTY (PRIVATE_DATA->aux_power_outlet_state_property)
#define AUX_POWER_OUTLET_STATE_1_ITEM   (AUX_POWER_OUTLET_STATE_PROPERTY->items + 0)

#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 1)

#define AUX_DEW_CONTROL_PROPERTY       (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 2)

#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_CURRENT_ITEM          (AUX_INFO_PROPERTY->items + 1)

#define X_AUX_REBOOT_PROPERTY          (PRIVATE_DATA->x_aux_reboot_property)
#define X_AUX_REBOOT_ITEM              (X_AUX_REBOOT_PROPERTY->items + 0)

#define X_AUX_REBOOT_PROPERTY_NAME     "X_AUX_REBOOT"
#define X_AUX_REBOOT_ITEM_NAME         "REBOOT"

#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY (PRIVATE_DATA->aux_save_outlet_states_as_default_property)
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM     (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_power_outlet_property;
	indigo_property *aux_dslr_power_property;
	indigo_property *aux_power_outlet_state_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *aux_dew_control_property;
	indigo_property *aux_weather_property;
	indigo_property *aux_info_property;
	indigo_property *x_aux_reboot_property;
	indigo_property *aux_save_outlet_states_as_default_property;
	//+ data
	bool is_advance;
	bool is_micro;
	bool is_saddle;
	char response[128];
	//- data
} ppb_private_data;

#pragma mark - Low level code

//+ code

static bool ppb_command(indigo_device *device, char *command, ...) {
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

static bool ppb_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (ppb_command(device, "P#")) {
			bool ok = false;
			if (!strcmp(PRIVATE_DATA->response, "PPB_OK")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro PPB");
				PRIVATE_DATA->is_advance = false;
				PRIVATE_DATA->is_micro = false;
				PRIVATE_DATA->is_saddle = false;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = true;
				AUX_DSLR_POWER_PROPERTY->hidden = true;
				AUX_POWER_OUTLET_PROPERTY->count = 2;
				ok = true;
			} else if (!strcmp(PRIVATE_DATA->response, "PPBA_OK")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro PPBA");
				PRIVATE_DATA->is_advance = true;
				PRIVATE_DATA->is_micro = false;
				PRIVATE_DATA->is_saddle = false;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = false;
				AUX_DSLR_POWER_PROPERTY->hidden = false;
				AUX_POWER_OUTLET_PROPERTY->count = 2;
				ok = true;
			} else if (!strcmp(PRIVATE_DATA->response, "PPBM_OK")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro PPBM");
				PRIVATE_DATA->is_advance = true;
				PRIVATE_DATA->is_micro = true;
				PRIVATE_DATA->is_saddle = false;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = false;
				AUX_DSLR_POWER_PROPERTY->hidden = false;
				AUX_POWER_OUTLET_PROPERTY->count = 2;
				ok = true;
			} else if (!strcmp(PRIVATE_DATA->response, "SPB")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro SPB");
				PRIVATE_DATA->is_advance = false;
				PRIVATE_DATA->is_micro = false;
				PRIVATE_DATA->is_saddle = true;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = true;
				AUX_DSLR_POWER_PROPERTY->hidden = true;
				AUX_POWER_OUTLET_PROPERTY->count = 1;
				ok = true;
			}
			if (ok) {
				if (ppb_command(device, "PV")) {
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response);
				}
				ppb_command(device, "PL:1");
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void ppb_close(indigo_device *device) {
	ppb_command(device, "PL:0");
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
	bool updateDSLRPower = false;
	bool updateHeaterOutlet = false;
	bool updateWeather = false;
	bool updateInfo = false;
	bool updateAutoHeater = false;
	if (ppb_command(device, "PA")) {
		char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
		if ((token = strtok_r(NULL, ":", &pnt))) { // Voltage
			double value = indigo_atod(token);
			if (AUX_INFO_VOLTAGE_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_VOLTAGE_ITEM->number.value = value;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // Current
			double value =  indigo_atod(token) / 65.0;
			if (AUX_INFO_CURRENT_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_CURRENT_ITEM->number.value = value;
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
		if ((token = strtok_r(NULL, ":", &pnt))) { // power ports status
			bool state = token[0] == '1';
			if (AUX_POWER_OUTLET_1_ITEM->sw.value != state) {
				AUX_POWER_OUTLET_1_ITEM->sw.value = state;
				updatePowerOutletState = true;
			}
		}
		if ((token = strtok_r(NULL, ":", &pnt))) { // DSLR ports status
			bool state = token[0] == '1';
			if (AUX_POWER_OUTLET_2_ITEM->sw.value != state) {
				AUX_POWER_OUTLET_2_ITEM->sw.value = state;
				updatePowerOutletState = true;
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
		if ((token = strtok_r(NULL, ":", &pnt))) { // Autodew
			bool state = token[0] == '1';
			if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value != state) {
				indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, state ? AUX_DEW_CONTROL_AUTOMATIC_ITEM : AUX_DEW_CONTROL_MANUAL_ITEM, true);
				updateAutoHeater = true;
			}
		}
		// PPBA
		if (PRIVATE_DATA->is_advance && (token = strtok_r(NULL, ":", &pnt))) { // Power Alert
			indigo_property_state state = token[0] == '1' ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
			if (AUX_POWER_OUTLET_STATE_1_ITEM->light.value != state) {
				AUX_POWER_OUTLET_STATE_1_ITEM->light.value = state;
				updatePowerOutletState = true;
			}
		}
		if (PRIVATE_DATA->is_advance && (token = strtok_r(NULL, ":", &pnt))) { // DSLR power
			if (!strcmp(token, "3")) {
				updateDSLRPower = !AUX_DSLR_POWER_3_ITEM->sw.value;
				indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_3_ITEM, true);
			} else if (!strcmp(token, "5")) {
				updateDSLRPower = !AUX_DSLR_POWER_5_ITEM->sw.value;
				indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_5_ITEM, true);
			} else if (!strcmp(token, "8")) {
				updateDSLRPower = !AUX_DSLR_POWER_8_ITEM->sw.value;
				indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_8_ITEM, true);
			} else if (!strcmp(token, "9")) {
				updateDSLRPower = !AUX_DSLR_POWER_9_ITEM->sw.value;
				indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_9_ITEM, true);
			} else if (!strcmp(token, "12")) {
				updateDSLRPower = !AUX_DSLR_POWER_12_ITEM->sw.value;
				indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_12_ITEM, true);
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
	if (updateDSLRPower) {
		AUX_DSLR_POWER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
	}
	if (updateHeaterOutlet) {
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
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
	indigo_execute_handler_in(device, 2, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = ppb_open(device);
		if (connection_result) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			indigo_define_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
		ppb_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_power_outlet_handler(indigo_device *device) {
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	ppb_command(device, "P1:%d", AUX_POWER_OUTLET_1_ITEM->sw.value ? 1 : 0);
	if (!PRIVATE_DATA->is_saddle) {
		ppb_command(device, "P2:%d", AUX_POWER_OUTLET_2_ITEM->sw.value ? 1 : 0);
	}
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
}

static void aux_dslr_power_handler(indigo_device *device) {
	AUX_DSLR_POWER_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DSLR_POWER.on_change
	if (AUX_DSLR_POWER_3_ITEM->sw.value) {
		ppb_command(device, "P2:3");
	} else if (AUX_DSLR_POWER_5_ITEM->sw.value)
		ppb_command(device, "P2:5");
	else if (AUX_DSLR_POWER_8_ITEM->sw.value)
		ppb_command(device, "P2:8");
	else if (AUX_DSLR_POWER_9_ITEM->sw.value)
		ppb_command(device, "P2:9");
	else if (AUX_DSLR_POWER_12_ITEM->sw.value)
		ppb_command(device, "P2:12");
	//- aux.AUX_DSLR_POWER.on_change
	indigo_update_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	ppb_command(device, "P3:%d", (int)(AUX_HEATER_OUTLET_1_ITEM->number.value * 255.0 / 100.0));
	ppb_command(device, "P4:%d", (int)(AUX_HEATER_OUTLET_2_ITEM->number.value * 255.0 / 100.0));
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void aux_dew_control_handler(indigo_device *device) {
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DEW_CONTROL.on_change
	ppb_command(device, "PD:%d", AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? 1 : 0);
	//- aux.AUX_DEW_CONTROL.on_change
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
}

static void aux_x_aux_reboot_handler(indigo_device *device) {
	X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_REBOOT.on_change
	if (X_AUX_REBOOT_ITEM->sw.value) {
		indigo_uni_printf(PRIVATE_DATA->handle, "PF\n");
		X_AUX_REBOOT_ITEM->sw.value = false;
	}
	//- aux.X_AUX_REBOOT.on_change
	indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
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
		ppb_command(device, command);
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
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater #1", "Heater #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater #2", "Heater #2");
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Power outlets", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "DSLR outlet", true);
		AUX_DSLR_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DSLR_POWER_PROPERTY_NAME, AUX_GROUP, "DSLR Power", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
		if (AUX_DSLR_POWER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DSLR_POWER_3_ITEM, AUX_DSLR_POWER_3_ITEM_NAME, "3V", false);
		indigo_init_switch_item(AUX_DSLR_POWER_5_ITEM, AUX_DSLR_POWER_5_ITEM_NAME, "5V", false);
		indigo_init_switch_item(AUX_DSLR_POWER_8_ITEM, AUX_DSLR_POWER_8_ITEM_NAME, "8V", false);
		indigo_init_switch_item(AUX_DSLR_POWER_9_ITEM, AUX_DSLR_POWER_9_ITEM_NAME, "9V", false);
		indigo_init_switch_item(AUX_DSLR_POWER_12_ITEM, AUX_DSLR_POWER_12_ITEM_NAME, "12V", false);
		AUX_DSLR_POWER_PROPERTY->hidden = true;
		AUX_POWER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_POWER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Power outlets state", INDIGO_OK_STATE, 1);
		if (AUX_POWER_OUTLET_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_POWER_OUTLET_STATE_1_ITEM, AUX_POWER_OUTLET_STATE_1_ITEM_NAME, "Power warning state", INDIGO_OK_STATE);
		AUX_POWER_OUTLET_STATE_PROPERTY->hidden = true;
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [C]", 0, 0, 0, 0);
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 0, 0, 0);
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_REBOOT_PROPERTY_NAME, AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, X_AUX_REBOOT_ITEM_NAME, "Reboot", false);
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
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DSLR_POWER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_CONTROL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_REBOOT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
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
	} else if (indigo_property_match_changeable(AUX_DSLR_POWER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DSLR_POWER_PROPERTY, aux_dslr_power_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_OUTLET_PROPERTY, aux_heater_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DEW_CONTROL_PROPERTY, aux_dew_control_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_REBOOT_PROPERTY, aux_x_aux_reboot_handler);
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
	indigo_release_property(AUX_DSLR_POWER_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_ppb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static ppb_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "PPB");
			patterns[0].vendor_id = 0x0403;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(ppb_private_data));
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
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
