// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_aux_ppb.driver

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>, PPBA support by Aaron Freimark <abf@mac.com>

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

#define DRIVER_VERSION       0x0300001A
#define DRIVER_NAME          "indigo_aux_ppb"
#define DRIVER_LABEL         "PegasusAstro Pocket Powerbox"
#define AUX_DEVICE_NAME      "Pocket Powerbox"
#define PRIVATE_DATA         ((ppb_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Powerbox"

//- define

#pragma mark - Property definitions

// AUX_OUTLET_NAMES handles definition
#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_HEATER_OUTLET_NAME_1_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_2_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 1)

// AUX_POWER_OUTLET handles definition
#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)

// AUX_DSLR_POWER handles definition
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

// AUX_POWER_OUTLET_STATE handles definition
#define AUX_POWER_OUTLET_STATE_PROPERTY (PRIVATE_DATA->aux_power_outlet_state_property)
#define AUX_POWER_OUTLET_STATE_1_ITEM   (AUX_POWER_OUTLET_STATE_PROPERTY->items + 0)

// AUX_HEATER_OUTLET handles definition
#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 1)

// AUX_DEW_CONTROL handles definition
#define AUX_DEW_CONTROL_PROPERTY       (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

// AUX_WEATHER handles definition
#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 2)

// AUX_INFO handles definition
#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_CURRENT_ITEM          (AUX_INFO_PROPERTY->items + 1)

// X_AUX_REBOOT handles definition
#define X_AUX_REBOOT_PROPERTY          (PRIVATE_DATA->x_aux_reboot_property)
#define X_AUX_REBOOT_ITEM              (X_AUX_REBOOT_PROPERTY->items + 0)

#define X_AUX_REBOOT_PROPERTY_NAME     "X_AUX_REBOOT"
#define X_AUX_REBOOT_ITEM_NAME         "REBOOT"

// AUX_SAVE_OUTLET_STATES_AS_DEFAULT handles definition
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY (PRIVATE_DATA->aux_save_outlet_states_as_default_property)
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM     (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	//+ data
	bool is_advance;
	bool is_micro;
	bool is_saddle;
	//- data
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
	indigo_timer *aux_timer;
	indigo_timer *aux_connection_handler_timer;
	indigo_timer *aux_outlet_names_handler_timer;
	indigo_timer *aux_power_outlet_handler_timer;
	indigo_timer *aux_dslr_power_handler_timer;
	indigo_timer *aux_heater_outlet_handler_timer;
	indigo_timer *aux_dew_control_handler_timer;
	indigo_timer *aux_x_aux_reboot_handler_timer;
	indigo_timer *aux_save_outlet_states_as_default_handler_timer;
} ppb_private_data;

#pragma mark - Low level code

//+ code

static bool ppb_command(indigo_device *device, char *command, char *response, int max) {
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

static bool ppb_open(indigo_device *device) {
	char response[128];
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (ppb_command(device, "P#", response, sizeof(response))) {
			if (!strcmp(response, "PPB_OK")) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro PPB");
				PRIVATE_DATA->is_advance = false;
				PRIVATE_DATA->is_micro = false;
				PRIVATE_DATA->is_saddle = false;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = true;
				AUX_DSLR_POWER_PROPERTY->hidden = true;
				AUX_POWER_OUTLET_PROPERTY->count = 2;
				ppb_command(device, "PL:1", response, sizeof(response));
				return true;
			} else if (!strcmp(response, "PPBA_OK")) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro PPBA");
				PRIVATE_DATA->is_advance = true;
				PRIVATE_DATA->is_micro = false;
				PRIVATE_DATA->is_saddle = false;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = false;
				AUX_DSLR_POWER_PROPERTY->hidden = false;
				AUX_POWER_OUTLET_PROPERTY->count = 2;
				ppb_command(device, "PL:1", response, sizeof(response));
				return true;
			} else if (!strcmp(response, "PPBM_OK")) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro PPBM");
				PRIVATE_DATA->is_advance = true;
				PRIVATE_DATA->is_micro = true;
				PRIVATE_DATA->is_saddle = false;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = false;
				AUX_DSLR_POWER_PROPERTY->hidden = false;
				AUX_POWER_OUTLET_PROPERTY->count = 2;
				ppb_command(device, "PL:1", response, sizeof(response));
				return true;
			} else if (!strcmp(response, "SPB")) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "PeagasusAstro SPB");
				PRIVATE_DATA->is_advance = false;
				PRIVATE_DATA->is_micro = false;
				PRIVATE_DATA->is_saddle = true;
				AUX_POWER_OUTLET_STATE_PROPERTY->hidden = true;
				AUX_DSLR_POWER_PROPERTY->hidden = true;
				AUX_POWER_OUTLET_PROPERTY->count = 1;
				ppb_command(device, "PL:1", response, sizeof(response));
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void ppb_close(indigo_device *device) {
	char response[128];
	if (PRIVATE_DATA->handle != NULL) {
		ppb_command(device, "PL:0", response, sizeof(response));
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

//- code

#pragma mark - High level code (aux)

// aux state checking timer callback

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ aux.on_timer
	char response[128];
	bool updatePowerOutlet = false;
	bool updatePowerOutletState = false;
	bool updateDSLRPower = false;
	bool updateHeaterOutlet = false;
	bool updateWeather = false;
	bool updateInfo = false;
	bool updateAutoHeater = false;
	if (ppb_command(device, "PA", response, sizeof(response))) {
		char *pnt, *token = strtok_r(response, ":", &pnt);
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
	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);
	//- aux.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// CONNECTION change handler

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = ppb_open(device);
		if (connection_result) {
			//+ aux.on_connect
			char response[128];
			if (ppb_command(device, "PV", response, sizeof(response))) {
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			if (ppb_command(device, "PA", response, sizeof(response))) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				if ((token = strtok_r(NULL, ":", &pnt))) { // Voltage
					AUX_INFO_VOLTAGE_ITEM->number.value = indigo_atod(token);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Current
					AUX_INFO_CURRENT_ITEM->number.value = indigo_atod(token) / 65.0;
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
				if ((token = strtok_r(NULL, ":", &pnt))) { // Power port status
					AUX_POWER_OUTLET_1_ITEM->sw.value = token[0] == '1';
				}
				if (PRIVATE_DATA->is_saddle) {
					token = strtok_r(response, ":", &pnt); // adjustment status
				} else {
					if ((token = strtok_r(NULL, ":", &pnt))) { // DSLR port status
						AUX_POWER_OUTLET_2_ITEM->sw.value = token[0] == '1';
					}
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Dew1
					AUX_HEATER_OUTLET_1_ITEM->number.value = AUX_HEATER_OUTLET_1_ITEM->number.target = round(indigo_atod(token) * 100.0 / 255.0);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Dew2
					AUX_HEATER_OUTLET_2_ITEM->number.value = AUX_HEATER_OUTLET_2_ITEM->number.target = round(indigo_atod(token) * 100.0 / 255.0);
				}
				if ((token = strtok_r(NULL, ":", &pnt))) { // Autodew
					indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, atoi(token) == 1 ? AUX_DEW_CONTROL_AUTOMATIC_ITEM : AUX_DEW_CONTROL_MANUAL_ITEM, true);
				}
				if (PRIVATE_DATA->is_advance && (token = strtok_r(NULL, ":", &pnt))) { // Power warning
					AUX_POWER_OUTLET_STATE_1_ITEM->light.value = token[0] == 1 ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
				}
				if (PRIVATE_DATA->is_advance && (token = strtok_r(NULL, ":", &pnt))) { // DSLR power
					if (!strcmp(token, "3")) {
						indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_3_ITEM, true);
					} else if (!strcmp(token, "5")) {
						indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_5_ITEM, true);
					} else if (!strcmp(token, "8")) {
						indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_8_ITEM, true);
					} else if (!strcmp(token, "9")) {
						indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_9_ITEM, true);
					} else if (!strcmp(token, "12")) {
						indigo_set_switch(AUX_DSLR_POWER_PROPERTY, AUX_DSLR_POWER_12_ITEM, true);
					}
				}
			}
			//- aux.on_connect
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			indigo_define_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_outlet_names_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_power_outlet_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_dslr_power_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_heater_outlet_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_dew_control_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_x_aux_reboot_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_save_outlet_states_as_default_handler_timer);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
		//+ aux.on_disconnect
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		//- aux.on_disconnect
		ppb_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// AUX_OUTLET_NAMES change handler

static void aux_outlet_names_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
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
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_POWER_OUTLET change handler

static void aux_power_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	char response[128];
	ppb_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? "P1:1" : "P1:0", response, sizeof(response));
	if (!PRIVATE_DATA->is_saddle) {
		ppb_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? "P2:1" : "P2:0", response, sizeof(response));
	}
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_DSLR_POWER change handler

static void aux_dslr_power_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_DSLR_POWER_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DSLR_POWER.on_change
	char response[128];
	if (AUX_DSLR_POWER_3_ITEM->sw.value) {
		ppb_command(device, "P2:3", response, sizeof(response));
	} else if (AUX_DSLR_POWER_5_ITEM->sw.value)
		ppb_command(device, "P2:5", response, sizeof(response));
	else if (AUX_DSLR_POWER_8_ITEM->sw.value)
		ppb_command(device, "P2:8", response, sizeof(response));
	else if (AUX_DSLR_POWER_9_ITEM->sw.value)
		ppb_command(device, "P2:9", response, sizeof(response));
	else if (AUX_DSLR_POWER_12_ITEM->sw.value)
		ppb_command(device, "P2:12", response, sizeof(response));
	//- aux.AUX_DSLR_POWER.on_change
	indigo_update_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_HEATER_OUTLET change handler

static void aux_heater_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	char command[16], response[128];
	sprintf(command, "P3:%d", (int)(AUX_HEATER_OUTLET_1_ITEM->number.value * 255.0 / 100.0));
	ppb_command(device, command, response, sizeof(response));
	sprintf(command, "P4:%d", (int)(AUX_HEATER_OUTLET_2_ITEM->number.value * 255.0 / 100.0));
	ppb_command(device, command, response, sizeof(response));
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_DEW_CONTROL change handler

static void aux_dew_control_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DEW_CONTROL.on_change
	char response[128];
	ppb_command(device, AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? "PD:1" : "PD:0", response, sizeof(response));
	//- aux.AUX_DEW_CONTROL.on_change
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// X_AUX_REBOOT change handler

static void aux_x_aux_reboot_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_REBOOT.on_change
	char response[128];
	if (X_AUX_REBOOT_ITEM->sw.value) {
		ppb_command(device, "PF", response, sizeof(response));
		X_AUX_REBOOT_ITEM->sw.value = false;
	}
	//- aux.X_AUX_REBOOT.on_change
	indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_SAVE_OUTLET_STATES_AS_DEFAULT change handler

static void aux_save_outlet_states_as_default_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SAVE_OUTLET_STATES_AS_DEFAULT.on_change
	char response[128];
	if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value) {
		char command[] = "PE:0000";
		char *port_mask = command + 3;
		for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
			port_mask[i] = AUX_POWER_OUTLET_PROPERTY->items[i].sw.value ? '1' : '0';
		}
		ppb_command(device, command, response, sizeof(response));
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value = false;
	}
	//- aux.AUX_SAVE_OUTLET_STATES_AS_DEFAULT.on_change
	indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
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
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater #1", "Heater #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater #2", "Heater #2");
		AUX_OUTLET_NAMES_PROPERTY->hidden = false;
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Power outlets", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "DSLR outlet", true);
		AUX_POWER_OUTLET_PROPERTY->hidden = false;
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
		AUX_HEATER_OUTLET_PROPERTY->hidden = false;
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		AUX_DEW_CONTROL_PROPERTY->hidden = false;
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [C]", -50, 100, 0, 0);
		AUX_WEATHER_PROPERTY->hidden = false;
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 20, 0, 0);
		AUX_INFO_PROPERTY->hidden = false;
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_REBOOT_PROPERTY_NAME, AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, X_AUX_REBOOT_ITEM_NAME, "Reboot", false);
		X_AUX_REBOOT_PROPERTY->hidden = false;
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY_NAME, AUX_GROUP, "Save current outlet states as default", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM_NAME, "Save", false);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->hidden = false;
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
		indigo_define_matching_property(AUX_DSLR_POWER_PROPERTY);
		indigo_define_matching_property(AUX_POWER_OUTLET_STATE_PROPERTY);
		indigo_define_matching_property(AUX_HEATER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_DEW_CONTROL_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
		indigo_define_matching_property(AUX_INFO_PROPERTY);
		indigo_define_matching_property(X_AUX_REBOOT_PROPERTY);
		indigo_define_matching_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	}
	indigo_define_matching_property(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

// aux change property API callback

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_connection_handler, &PRIVATE_DATA->aux_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_outlet_names_handler_timer == NULL) {
			indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
			AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_outlet_names_handler, &PRIVATE_DATA->aux_outlet_names_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_power_outlet_handler_timer == NULL) {
			indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_power_outlet_handler, &PRIVATE_DATA->aux_power_outlet_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DSLR_POWER_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_dslr_power_handler_timer == NULL) {
			indigo_property_copy_values(AUX_DSLR_POWER_PROPERTY, property, false);
			AUX_DSLR_POWER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_DSLR_POWER_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_dslr_power_handler, &PRIVATE_DATA->aux_dslr_power_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_heater_outlet_handler_timer == NULL) {
			indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_heater_outlet_handler, &PRIVATE_DATA->aux_heater_outlet_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_dew_control_handler_timer == NULL) {
			indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
			AUX_DEW_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_dew_control_handler, &PRIVATE_DATA->aux_dew_control_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_x_aux_reboot_handler_timer == NULL) {
			indigo_property_copy_values(X_AUX_REBOOT_PROPERTY, property, false);
			X_AUX_REBOOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_x_aux_reboot_handler, &PRIVATE_DATA->aux_x_aux_reboot_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_save_outlet_states_as_default_handler_timer == NULL) {
			indigo_property_copy_values(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, property, false);
			AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_save_outlet_states_as_default_handler, &PRIVATE_DATA->aux_save_outlet_states_as_default_handler_timer);
		}
		return INDIGO_OK;
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
	indigo_release_property(AUX_DSLR_POWER_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

// PegasusAstro Pocket Powerbox driver entry point

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
			strcpy(patterns[0].vendor_string, "Pegasus Astro");
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
