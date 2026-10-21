// Copyright (c) 2026 CloudMakers, s. r. o.
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

// This file generated from indigo_aux_simulator.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000001
#define DRIVER_NAME          "indigo_aux_simulator"
#define DRIVER_LABEL         "Auxiliary Simulators"
#define POWERBOX_DEVICE_NAME "PowerBox Simulator"
#define WEATHER_DEVICE_NAME  "Weather Station Simulator"
#define SQM_DEVICE_NAME      "SQM Simulator"
#define FLAT_DEVICE_NAME     "Flat Panel Simulator"
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)

#pragma mark - Property definitions

#define PB_POWER_PROPERTY              (PRIVATE_DATA->pb_power_property)
#define PB_POWER_1_ITEM                (PB_POWER_PROPERTY->items + 0)
#define PB_POWER_2_ITEM                (PB_POWER_PROPERTY->items + 1)
#define PB_POWER_3_ITEM                (PB_POWER_PROPERTY->items + 2)
#define PB_POWER_4_ITEM                (PB_POWER_PROPERTY->items + 3)

#define PB_POWER_STATE_PROPERTY        (PRIVATE_DATA->pb_power_state_property)
#define PB_POWER_STATE_1_ITEM          (PB_POWER_STATE_PROPERTY->items + 0)
#define PB_POWER_STATE_2_ITEM          (PB_POWER_STATE_PROPERTY->items + 1)
#define PB_POWER_STATE_3_ITEM          (PB_POWER_STATE_PROPERTY->items + 2)
#define PB_POWER_STATE_4_ITEM          (PB_POWER_STATE_PROPERTY->items + 3)

#define PB_POWER_CURRENT_PROPERTY      (PRIVATE_DATA->pb_power_current_property)
#define PB_POWER_CURRENT_1_ITEM        (PB_POWER_CURRENT_PROPERTY->items + 0)
#define PB_POWER_CURRENT_2_ITEM        (PB_POWER_CURRENT_PROPERTY->items + 1)
#define PB_POWER_CURRENT_3_ITEM        (PB_POWER_CURRENT_PROPERTY->items + 2)
#define PB_POWER_CURRENT_4_ITEM        (PB_POWER_CURRENT_PROPERTY->items + 3)

#define PB_USB_PROPERTY                (PRIVATE_DATA->pb_usb_property)
#define PB_USB_1_ITEM                  (PB_USB_PROPERTY->items + 0)
#define PB_USB_2_ITEM                  (PB_USB_PROPERTY->items + 1)
#define PB_USB_3_ITEM                  (PB_USB_PROPERTY->items + 2)
#define PB_USB_4_ITEM                  (PB_USB_PROPERTY->items + 3)

#define PB_USB_STATE_PROPERTY          (PRIVATE_DATA->pb_usb_state_property)
#define PB_USB_STATE_1_ITEM            (PB_USB_STATE_PROPERTY->items + 0)
#define PB_USB_STATE_2_ITEM            (PB_USB_STATE_PROPERTY->items + 1)
#define PB_USB_STATE_3_ITEM            (PB_USB_STATE_PROPERTY->items + 2)
#define PB_USB_STATE_4_ITEM            (PB_USB_STATE_PROPERTY->items + 3)

#define PB_HEATER_PROPERTY             (PRIVATE_DATA->pb_heater_property)
#define PB_HEATER_1_ITEM               (PB_HEATER_PROPERTY->items + 0)
#define PB_HEATER_2_ITEM               (PB_HEATER_PROPERTY->items + 1)

#define PB_HEATER_STATE_PROPERTY       (PRIVATE_DATA->pb_heater_state_property)
#define PB_HEATER_STATE_1_ITEM         (PB_HEATER_STATE_PROPERTY->items + 0)
#define PB_HEATER_STATE_2_ITEM         (PB_HEATER_STATE_PROPERTY->items + 1)

#define PB_HEATER_CURRENT_PROPERTY     (PRIVATE_DATA->pb_heater_current_property)
#define PB_HEATER_CURRENT_1_ITEM       (PB_HEATER_CURRENT_PROPERTY->items + 0)
#define PB_HEATER_CURRENT_2_ITEM       (PB_HEATER_CURRENT_PROPERTY->items + 1)

#define PB_DEW_PROPERTY                (PRIVATE_DATA->pb_dew_property)
#define PB_DEW_MANUAL_ITEM             (PB_DEW_PROPERTY->items + 0)
#define PB_DEW_AUTO_ITEM               (PB_DEW_PROPERTY->items + 1)

#define PB_INFO_PROPERTY               (PRIVATE_DATA->pb_info_property)
#define PB_INFO_VOLTAGE_ITEM           (PB_INFO_PROPERTY->items + 0)
#define PB_INFO_CURRENT_ITEM           (PB_INFO_PROPERTY->items + 1)
#define PB_INFO_POWER_ITEM             (PB_INFO_PROPERTY->items + 2)

#define PB_WEATHER_PROPERTY            (PRIVATE_DATA->pb_weather_property)
#define PB_TEMPERATURE_ITEM            (PB_WEATHER_PROPERTY->items + 0)
#define PB_HUMIDITY_ITEM               (PB_WEATHER_PROPERTY->items + 1)
#define PB_DEWPOINT_ITEM               (PB_WEATHER_PROPERTY->items + 2)

#define WX_INPUT_PROPERTY              (PRIVATE_DATA->wx_input_property)
#define WX_INPUT_TEMPERATURE_ITEM      (WX_INPUT_PROPERTY->items + 0)
#define WX_INPUT_HUMIDITY_ITEM         (WX_INPUT_PROPERTY->items + 1)
#define WX_INPUT_WIND_SPEED_ITEM       (WX_INPUT_PROPERTY->items + 2)
#define WX_INPUT_WIND_DIRECTION_ITEM   (WX_INPUT_PROPERTY->items + 3)
#define WX_INPUT_PRESSURE_ITEM         (WX_INPUT_PROPERTY->items + 4)
#define WX_INPUT_RAIN_ITEM             (WX_INPUT_PROPERTY->items + 5)
#define WX_INPUT_SKY_TEMPERATURE_ITEM  (WX_INPUT_PROPERTY->items + 6)

#define WX_INPUT_PROPERTY_NAME         "X_SIMULATION_PARAMETERS"

#define WX_WEATHER_PROPERTY            (PRIVATE_DATA->wx_weather_property)
#define WX_TEMPERATURE_ITEM            (WX_WEATHER_PROPERTY->items + 0)
#define WX_HUMIDITY_ITEM               (WX_WEATHER_PROPERTY->items + 1)
#define WX_WIND_SPEED_ITEM             (WX_WEATHER_PROPERTY->items + 2)
#define WX_WIND_DIRECTION_ITEM         (WX_WEATHER_PROPERTY->items + 3)
#define WX_PRESSURE_ITEM               (WX_WEATHER_PROPERTY->items + 4)
#define WX_RAIN_ITEM                   (WX_WEATHER_PROPERTY->items + 5)
#define WX_SKY_TEMPERATURE_ITEM        (WX_WEATHER_PROPERTY->items + 6)
#define WX_DEWPOINT_ITEM               (WX_WEATHER_PROPERTY->items + 7)

#define WX_WIND_PROPERTY               (PRIVATE_DATA->wx_wind_property)
#define WX_WIND_STRONG_ITEM            (WX_WIND_PROPERTY->items + 0)
#define WX_WIND_MODERATE_ITEM          (WX_WIND_PROPERTY->items + 1)
#define WX_WIND_CALM_ITEM              (WX_WIND_PROPERTY->items + 2)

#define WX_RAIN_STATE_PROPERTY         (PRIVATE_DATA->wx_rain_state_property)
#define WX_RAIN_STATE_RAINING_ITEM     (WX_RAIN_STATE_PROPERTY->items + 0)
#define WX_RAIN_STATE_WET_ITEM         (WX_RAIN_STATE_PROPERTY->items + 1)
#define WX_RAIN_STATE_DRY_ITEM         (WX_RAIN_STATE_PROPERTY->items + 2)

#define WX_CLOUD_PROPERTY              (PRIVATE_DATA->wx_cloud_property)
#define WX_CLOUD_CLEAR_ITEM            (WX_CLOUD_PROPERTY->items + 0)
#define WX_CLOUD_CLOUDY_ITEM           (WX_CLOUD_PROPERTY->items + 1)
#define WX_CLOUD_OVERCAST_ITEM         (WX_CLOUD_PROPERTY->items + 2)

#define SQM_INPUT_PROPERTY             (PRIVATE_DATA->sqm_input_property)
#define SQM_INPUT_BRIGHTNESS_ITEM      (SQM_INPUT_PROPERTY->items + 0)
#define SQM_INPUT_TEMPERATURE_ITEM     (SQM_INPUT_PROPERTY->items + 1)

#define SQM_INPUT_PROPERTY_NAME        "X_SIMULATION_PARAMETERS"

#define SQM_WEATHER_PROPERTY           (PRIVATE_DATA->sqm_weather_property)
#define SQM_BRIGHTNESS_ITEM            (SQM_WEATHER_PROPERTY->items + 0)
#define SQM_TEMPERATURE_ITEM           (SQM_WEATHER_PROPERTY->items + 1)
#define SQM_BORTLE_ITEM                (SQM_WEATHER_PROPERTY->items + 2)

#define FLAT_LIGHT_PROPERTY            (PRIVATE_DATA->flat_light_property)
#define FLAT_LIGHT_ON_ITEM             (FLAT_LIGHT_PROPERTY->items + 0)
#define FLAT_LIGHT_OFF_ITEM            (FLAT_LIGHT_PROPERTY->items + 1)

#define FLAT_INTENSITY_PROPERTY        (PRIVATE_DATA->flat_intensity_property)
#define FLAT_INTENSITY_ITEM            (FLAT_INTENSITY_PROPERTY->items + 0)

#define FLAT_COVER_PROPERTY            (PRIVATE_DATA->flat_cover_property)
#define FLAT_OPEN_ITEM                 (FLAT_COVER_PROPERTY->items + 0)
#define FLAT_CLOSE_ITEM                (FLAT_COVER_PROPERTY->items + 1)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_property *pb_power_property;
	indigo_property *pb_power_state_property;
	indigo_property *pb_power_current_property;
	indigo_property *pb_usb_property;
	indigo_property *pb_usb_state_property;
	indigo_property *pb_heater_property;
	indigo_property *pb_heater_state_property;
	indigo_property *pb_heater_current_property;
	indigo_property *pb_dew_property;
	indigo_property *pb_info_property;
	indigo_property *pb_weather_property;
	indigo_property *wx_input_property;
	indigo_property *wx_weather_property;
	indigo_property *wx_wind_property;
	indigo_property *wx_rain_state_property;
	indigo_property *wx_cloud_property;
	indigo_property *sqm_input_property;
	indigo_property *sqm_weather_property;
	indigo_property *flat_light_property;
	indigo_property *flat_intensity_property;
	indigo_property *flat_cover_property;
	//+ data
	bool cover_open;
	//- data
} simulator_private_data;

#pragma mark - Low level code

//+ code

static void powerbox_refresh(indigo_device *device, bool publish) {
	double current = 0.2;
	for (int i = 0; i < 4; i++) {
		bool on = PB_POWER_PROPERTY->items[i].sw.value;
		PB_POWER_STATE_PROPERTY->items[i].light.value = on ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
		PB_POWER_CURRENT_PROPERTY->items[i].number.value = on ? 0.5 * (i + 1) : 0;
		current += PB_POWER_CURRENT_PROPERTY->items[i].number.value;
		PB_USB_STATE_PROPERTY->items[i].light.value = PB_USB_PROPERTY->items[i].sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	}
	for (int i = 0; i < 2; i++) {
		PB_HEATER_STATE_PROPERTY->items[i].light.value = PB_HEATER_PROPERTY->items[i].number.value > 0 ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
		PB_HEATER_CURRENT_PROPERTY->items[i].number.value = PB_HEATER_PROPERTY->items[i].number.value / 100;
		current += PB_HEATER_CURRENT_PROPERTY->items[i].number.value;
	}
	PB_INFO_CURRENT_ITEM->number.value = current;
	PB_INFO_POWER_ITEM->number.value = current * PB_INFO_VOLTAGE_ITEM->number.value;
	indigo_property *properties[] = { PB_POWER_STATE_PROPERTY, PB_POWER_CURRENT_PROPERTY, PB_USB_STATE_PROPERTY, PB_HEATER_STATE_PROPERTY, PB_HEATER_CURRENT_PROPERTY, PB_INFO_PROPERTY, PB_WEATHER_PROPERTY };
	for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
		properties[i]->state = INDIGO_OK_STATE;
		if (publish) {
			indigo_update_property(device, properties[i], NULL);
		}
	}
}
static void weather_refresh(indigo_device *device, bool publish) {
	for (int i = 0; i < WX_INPUT_PROPERTY->count; i++) {
		WX_WEATHER_PROPERTY->items[i].number.value = WX_INPUT_PROPERTY->items[i].number.value;
	}
	double temperature = WX_TEMPERATURE_ITEM->number.value;
	double humidity = WX_HUMIDITY_ITEM->number.value;
	// Magnus approximation; clamp zero humidity for a finite dew point.
	double gamma = log(fmax(humidity, 0.1) / 100) + 17.62 * temperature / (243.12 + temperature);
	WX_DEWPOINT_ITEM->number.value = 243.12 * gamma / (17.62 - gamma);
	int wind = WX_WIND_SPEED_ITEM->number.value >= 10 ? 0 : (WX_WIND_SPEED_ITEM->number.value >= 3 ? 1 : 2);
	int rain = WX_RAIN_ITEM->number.value >= 1 ? 0 : (WX_RAIN_ITEM->number.value > 0 ? 1 : 2);
	double delta = temperature - WX_SKY_TEMPERATURE_ITEM->number.value;
	int cloud = delta >= 20 ? 0 : (delta >= 10 ? 1 : 2);
	indigo_property *conditions[] = { WX_WIND_PROPERTY, WX_RAIN_STATE_PROPERTY, WX_CLOUD_PROPERTY };
	int selected[] = { wind, rain, cloud };
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			conditions[i]->items[j].light.value = j == selected[i] ? (j == (i == 2 ? 0 : 2) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE) : INDIGO_IDLE_STATE;
		}
	}
	indigo_property *properties[] = { WX_WEATHER_PROPERTY, WX_WIND_PROPERTY, WX_RAIN_STATE_PROPERTY, WX_CLOUD_PROPERTY };
	for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
		properties[i]->state = INDIGO_OK_STATE;
		if (publish) {
			indigo_update_property(device, properties[i], NULL);
		}
	}
}
static void sqm_refresh(indigo_device *device, bool publish) {
	SQM_BRIGHTNESS_ITEM->number.value = SQM_INPUT_BRIGHTNESS_ITEM->number.value;
	SQM_TEMPERATURE_ITEM->number.value = SQM_INPUT_TEMPERATURE_ITEM->number.value;
	SQM_BORTLE_ITEM->number.value = indigo_aux_sky_bortle(SQM_BRIGHTNESS_ITEM->number.value);
	SQM_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
	if (publish) {
		indigo_update_property(device, SQM_WEATHER_PROPERTY, NULL);
	}
}
static void flat_cover_finalizer(indigo_device *device) {
	PRIVATE_DATA->cover_open = indigo_get_switch_target(FLAT_COVER_PROPERTY, AUX_COVER_OPEN_ITEM_NAME);
	indigo_set_switch(FLAT_COVER_PROPERTY, PRIVATE_DATA->cover_open ? FLAT_OPEN_ITEM : FLAT_CLOSE_ITEM, true);
	INDIGO_UPDATE_PROPERTY_STATE(FLAT_COVER_PROPERTY, INDIGO_OK_STATE, NULL);
}

//- code

#pragma mark - High level code (powerbox)
// device_id: powerbox type: aux

static void powerbox_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//+ powerbox.on_connect
		powerbox_refresh(device, false);
		//- powerbox.on_connect
			indigo_define_property(device, PB_POWER_PROPERTY, NULL);
			indigo_define_property(device, PB_POWER_STATE_PROPERTY, NULL);
			indigo_define_property(device, PB_POWER_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, PB_USB_PROPERTY, NULL);
			indigo_define_property(device, PB_USB_STATE_PROPERTY, NULL);
			indigo_define_property(device, PB_HEATER_PROPERTY, NULL);
			indigo_define_property(device, PB_HEATER_STATE_PROPERTY, NULL);
			indigo_define_property(device, PB_HEATER_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, PB_DEW_PROPERTY, NULL);
			indigo_define_property(device, PB_INFO_PROPERTY, NULL);
			indigo_define_property(device, PB_WEATHER_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			PB_POWER_PROPERTY,
			PB_POWER_STATE_PROPERTY,
			PB_POWER_CURRENT_PROPERTY,
			PB_USB_PROPERTY,
			PB_USB_STATE_PROPERTY,
			PB_HEATER_PROPERTY,
			PB_HEATER_STATE_PROPERTY,
			PB_HEATER_CURRENT_PROPERTY,
			PB_DEW_PROPERTY,
			PB_INFO_PROPERTY,
			PB_WEATHER_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, PB_POWER_PROPERTY, NULL);
		indigo_delete_property(device, PB_POWER_STATE_PROPERTY, NULL);
		indigo_delete_property(device, PB_POWER_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, PB_USB_PROPERTY, NULL);
		indigo_delete_property(device, PB_USB_STATE_PROPERTY, NULL);
		indigo_delete_property(device, PB_HEATER_PROPERTY, NULL);
		indigo_delete_property(device, PB_HEATER_STATE_PROPERTY, NULL);
		indigo_delete_property(device, PB_HEATER_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, PB_DEW_PROPERTY, NULL);
		indigo_delete_property(device, PB_INFO_PROPERTY, NULL);
		indigo_delete_property(device, PB_WEATHER_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void powerbox_pb_power_handler(indigo_device *device) {
	PB_POWER_PROPERTY->state = INDIGO_OK_STATE;
	//+ powerbox.PB_POWER.on_change
	powerbox_refresh(device, true);
	//- powerbox.PB_POWER.on_change
	indigo_update_property(device, PB_POWER_PROPERTY, NULL);
}

static void powerbox_pb_usb_handler(indigo_device *device) {
	PB_USB_PROPERTY->state = INDIGO_OK_STATE;
	//+ powerbox.PB_USB.on_change
	powerbox_refresh(device, true);
	//- powerbox.PB_USB.on_change
	indigo_update_property(device, PB_USB_PROPERTY, NULL);
}

static void powerbox_pb_heater_handler(indigo_device *device) {
	PB_HEATER_PROPERTY->state = INDIGO_OK_STATE;
	//+ powerbox.PB_HEATER.on_change
	indigo_set_switch(PB_DEW_PROPERTY, PB_DEW_MANUAL_ITEM, true);
	indigo_update_property(device, PB_DEW_PROPERTY, NULL);
	powerbox_refresh(device, true);
	//- powerbox.PB_HEATER.on_change
	indigo_update_property(device, PB_HEATER_PROPERTY, NULL);
}

static void powerbox_pb_dew_handler(indigo_device *device) {
	PB_DEW_PROPERTY->state = INDIGO_OK_STATE;
	//+ powerbox.PB_DEW.on_change
	if (PB_DEW_AUTO_ITEM->sw.value) {
		for (int i = 0; i < 2; i++) {
			PB_HEATER_PROPERTY->items[i].number.value = PB_HEATER_PROPERTY->items[i].number.target = 35;
		}
		PB_HEATER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, PB_HEATER_PROPERTY, NULL);
	}
	powerbox_refresh(device, true);
	//- powerbox.PB_DEW.on_change
	indigo_update_property(device, PB_DEW_PROPERTY, NULL);
}

#pragma mark - Device API (powerbox)

static indigo_result powerbox_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result powerbox_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		//+ powerbox.on_attach
		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		//- powerbox.on_attach
		PB_POWER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, "Main", "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (PB_POWER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(PB_POWER_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", true);
		indigo_init_switch_item(PB_POWER_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", true);
		indigo_init_switch_item(PB_POWER_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Outlet #3", true);
		indigo_init_switch_item(PB_POWER_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Outlet #4", true);
		PB_POWER_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_POWER_OUTLET_STATE_PROPERTY_NAME, "Main", "Power outlet state", INDIGO_OK_STATE, 4);
		if (PB_POWER_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(PB_POWER_STATE_1_ITEM, AUX_POWER_OUTLET_STATE_1_ITEM_NAME, "Outlet #1", INDIGO_OK_STATE);
		indigo_init_light_item(PB_POWER_STATE_2_ITEM, AUX_POWER_OUTLET_STATE_2_ITEM_NAME, "Outlet #2", INDIGO_OK_STATE);
		indigo_init_light_item(PB_POWER_STATE_3_ITEM, AUX_POWER_OUTLET_STATE_3_ITEM_NAME, "Outlet #3", INDIGO_OK_STATE);
		indigo_init_light_item(PB_POWER_STATE_4_ITEM, AUX_POWER_OUTLET_STATE_4_ITEM_NAME, "Outlet #4", INDIGO_OK_STATE);
		PB_POWER_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME, "Main", "Power outlet current [A]", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (PB_POWER_CURRENT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(PB_POWER_CURRENT_1_ITEM, AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME, "Outlet #1", 0, 10, 0, 0);
		indigo_init_number_item(PB_POWER_CURRENT_2_ITEM, AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME, "Outlet #2", 0, 10, 0, 0);
		indigo_init_number_item(PB_POWER_CURRENT_3_ITEM, AUX_POWER_OUTLET_CURRENT_3_ITEM_NAME, "Outlet #3", 0, 10, 0, 0);
		indigo_init_number_item(PB_POWER_CURRENT_4_ITEM, AUX_POWER_OUTLET_CURRENT_4_ITEM_NAME, "Outlet #4", 0, 10, 0, 0);
		PB_USB_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, "Main", "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (PB_USB_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(PB_USB_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "USB port #1", true);
		indigo_init_switch_item(PB_USB_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "USB port #2", true);
		indigo_init_switch_item(PB_USB_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "USB port #3", true);
		indigo_init_switch_item(PB_USB_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "USB port #4", true);
		PB_USB_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_USB_PORT_STATE_PROPERTY_NAME, "Main", "USB port state", INDIGO_OK_STATE, 4);
		if (PB_USB_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(PB_USB_STATE_1_ITEM, AUX_USB_PORT_STATE_1_ITEM_NAME, "USB port #1", INDIGO_OK_STATE);
		indigo_init_light_item(PB_USB_STATE_2_ITEM, AUX_USB_PORT_STATE_2_ITEM_NAME, "USB port #2", INDIGO_OK_STATE);
		indigo_init_light_item(PB_USB_STATE_3_ITEM, AUX_USB_PORT_STATE_3_ITEM_NAME, "USB port #3", INDIGO_OK_STATE);
		indigo_init_light_item(PB_USB_STATE_4_ITEM, AUX_USB_PORT_STATE_4_ITEM_NAME, "USB port #4", INDIGO_OK_STATE);
		PB_HEATER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, "Main", "Heater outputs [%]", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (PB_HEATER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(PB_HEATER_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1", 0, 100, 1, 0);
		indigo_init_number_item(PB_HEATER_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2", 0, 100, 1, 0);
		PB_HEATER_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_HEATER_OUTLET_STATE_PROPERTY_NAME, "Main", "Heater output state", INDIGO_OK_STATE, 2);
		if (PB_HEATER_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(PB_HEATER_STATE_1_ITEM, AUX_HEATER_OUTLET_STATE_1_ITEM_NAME, "Heater #1", INDIGO_IDLE_STATE);
		indigo_init_light_item(PB_HEATER_STATE_2_ITEM, AUX_HEATER_OUTLET_STATE_2_ITEM_NAME, "Heater #2", INDIGO_IDLE_STATE);
		PB_HEATER_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_CURRENT_PROPERTY_NAME, "Main", "Heater current [A]", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (PB_HEATER_CURRENT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(PB_HEATER_CURRENT_1_ITEM, AUX_HEATER_OUTLET_CURRENT_1_ITEM_NAME, "Heater #1", 0, 1, 0, 0);
		indigo_init_number_item(PB_HEATER_CURRENT_2_ITEM, AUX_HEATER_OUTLET_CURRENT_2_ITEM_NAME, "Heater #2", 0, 1, 0, 0);
		PB_DEW_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, "Main", "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (PB_DEW_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(PB_DEW_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(PB_DEW_AUTO_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		PB_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Main", "Power sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (PB_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(PB_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 20, 0, 12.5);
		indigo_init_number_item(PB_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 30, 0, 0);
		indigo_init_number_item(PB_INFO_POWER_ITEM, AUX_INFO_POWER_ITEM_NAME, "Power [W]", 0, 600, 0, 0);
		PB_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Main", "Environment", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (PB_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(PB_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 10);
		indigo_init_number_item(PB_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 60);
		indigo_init_number_item(PB_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dew point [C]", -100, 100, 0, 2.6);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return powerbox_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result powerbox_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_POWER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_POWER_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_POWER_CURRENT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_USB_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_USB_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_HEATER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_HEATER_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_HEATER_CURRENT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_DEW_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(PB_WEATHER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result powerbox_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(powerbox_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(PB_POWER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(PB_POWER_PROPERTY, powerbox_pb_power_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(PB_USB_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(PB_USB_PROPERTY, powerbox_pb_usb_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(PB_HEATER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(PB_HEATER_PROPERTY, powerbox_pb_heater_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(PB_DEW_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(PB_DEW_PROPERTY, powerbox_pb_dew_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result powerbox_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		powerbox_connection_handler(device);
	}
	indigo_release_property(PB_POWER_PROPERTY);
	indigo_release_property(PB_POWER_STATE_PROPERTY);
	indigo_release_property(PB_POWER_CURRENT_PROPERTY);
	indigo_release_property(PB_USB_PROPERTY);
	indigo_release_property(PB_USB_STATE_PROPERTY);
	indigo_release_property(PB_HEATER_PROPERTY);
	indigo_release_property(PB_HEATER_STATE_PROPERTY);
	indigo_release_property(PB_HEATER_CURRENT_PROPERTY);
	indigo_release_property(PB_DEW_PROPERTY);
	indigo_release_property(PB_INFO_PROPERTY);
	indigo_release_property(PB_WEATHER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (weather)
// device_id: weather type: aux

static void weather_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//+ weather.on_connect
		weather_refresh(device, false);
		//- weather.on_connect
			indigo_define_property(device, WX_INPUT_PROPERTY, NULL);
			indigo_define_property(device, WX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, WX_WIND_PROPERTY, NULL);
			indigo_define_property(device, WX_RAIN_STATE_PROPERTY, NULL);
			indigo_define_property(device, WX_CLOUD_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			WX_INPUT_PROPERTY,
			WX_WEATHER_PROPERTY,
			WX_WIND_PROPERTY,
			WX_RAIN_STATE_PROPERTY,
			WX_CLOUD_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, WX_INPUT_PROPERTY, NULL);
		indigo_delete_property(device, WX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, WX_WIND_PROPERTY, NULL);
		indigo_delete_property(device, WX_RAIN_STATE_PROPERTY, NULL);
		indigo_delete_property(device, WX_CLOUD_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void weather_wx_input_handler(indigo_device *device) {
	WX_INPUT_PROPERTY->state = INDIGO_OK_STATE;
	//+ weather.WX_INPUT.on_change
	weather_refresh(device, true);
	//- weather.WX_INPUT.on_change
	indigo_update_property(device, WX_INPUT_PROPERTY, NULL);
}

#pragma mark - Device API (weather)

static indigo_result weather_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result weather_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		//+ weather.on_attach
		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		//- weather.on_attach
		WX_INPUT_PROPERTY = indigo_init_number_property(NULL, device->name, WX_INPUT_PROPERTY_NAME, "Simulation", "Simulated weather", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (WX_INPUT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(WX_INPUT_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 60, 0, 10);
		indigo_init_number_item(WX_INPUT_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 60);
		indigo_init_number_item(WX_INPUT_WIND_SPEED_ITEM, AUX_WEATHER_WIND_SPEED_ITEM_NAME, "Wind speed [m/s]", 0, 100, 0, 1);
		indigo_init_number_item(WX_INPUT_WIND_DIRECTION_ITEM, AUX_WEATHER_WIND_DIRECTION_ITEM_NAME, "Wind direction [deg]", 0, 360, 0, 180);
		indigo_init_number_item(WX_INPUT_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [hPa]", 300, 1200, 0, 1013);
		indigo_init_number_item(WX_INPUT_RAIN_ITEM, AUX_WEATHER_RAIN_ITEM_NAME, "Rain [mm/h]", 0, 100, 0, 0);
		indigo_init_number_item(WX_INPUT_SKY_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sky temperature [C]", -100, 60, 0, -15);
		WX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Main", "Weather", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (WX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(WX_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 60, 0, 10);
		indigo_init_number_item(WX_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 60);
		indigo_init_number_item(WX_WIND_SPEED_ITEM, AUX_WEATHER_WIND_SPEED_ITEM_NAME, "Wind speed [m/s]", 0, 100, 0, 1);
		indigo_init_number_item(WX_WIND_DIRECTION_ITEM, AUX_WEATHER_WIND_DIRECTION_ITEM_NAME, "Wind direction [deg]", 0, 360, 0, 180);
		indigo_init_number_item(WX_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [hPa]", 300, 1200, 0, 1013);
		indigo_init_number_item(WX_RAIN_ITEM, AUX_WEATHER_RAIN_ITEM_NAME, "Rain [mm/h]", 0, 100, 0, 0);
		indigo_init_number_item(WX_SKY_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sky temperature [C]", -100, 60, 0, -15);
		indigo_init_number_item(WX_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dew point [C]", -100, 60, 0, 0);
		WX_WIND_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_WIND_PROPERTY_NAME, "Main", "Wind", INDIGO_OK_STATE, 3);
		if (WX_WIND_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(WX_WIND_STRONG_ITEM, AUX_WIND_STRONG_ITEM_NAME, "Strong", INDIGO_IDLE_STATE);
		indigo_init_light_item(WX_WIND_MODERATE_ITEM, AUX_WIND_MODERATE_ITEM_NAME, "Moderate", INDIGO_IDLE_STATE);
		indigo_init_light_item(WX_WIND_CALM_ITEM, AUX_WIND_CALM_ITEM_NAME, "Calm", INDIGO_IDLE_STATE);
		WX_RAIN_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_RAIN_PROPERTY_NAME, "Main", "Rain", INDIGO_OK_STATE, 3);
		if (WX_RAIN_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(WX_RAIN_STATE_RAINING_ITEM, AUX_RAIN_RAINING_ITEM_NAME, "Raining", INDIGO_IDLE_STATE);
		indigo_init_light_item(WX_RAIN_STATE_WET_ITEM, AUX_RAIN_WET_ITEM_NAME, "Wet", INDIGO_IDLE_STATE);
		indigo_init_light_item(WX_RAIN_STATE_DRY_ITEM, AUX_RAIN_DRY_ITEM_NAME, "Dry", INDIGO_IDLE_STATE);
		WX_CLOUD_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_CLOUD_PROPERTY_NAME, "Main", "Cloud cover", INDIGO_OK_STATE, 3);
		if (WX_CLOUD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(WX_CLOUD_CLEAR_ITEM, AUX_CLOUD_CLEAR_ITEM_NAME, "Clear", INDIGO_IDLE_STATE);
		indigo_init_light_item(WX_CLOUD_CLOUDY_ITEM, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy", INDIGO_IDLE_STATE);
		indigo_init_light_item(WX_CLOUD_OVERCAST_ITEM, AUX_CLOUD_OVERCAST_ITEM_NAME, "Overcast", INDIGO_IDLE_STATE);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return weather_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result weather_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(WX_INPUT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(WX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(WX_WIND_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(WX_RAIN_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(WX_CLOUD_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result weather_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(weather_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WX_INPUT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WX_INPUT_PROPERTY, weather_wx_input_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result weather_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		weather_connection_handler(device);
	}
	indigo_release_property(WX_INPUT_PROPERTY);
	indigo_release_property(WX_WEATHER_PROPERTY);
	indigo_release_property(WX_WIND_PROPERTY);
	indigo_release_property(WX_RAIN_STATE_PROPERTY);
	indigo_release_property(WX_CLOUD_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (sqm)
// device_id: sqm type: aux

static void sqm_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//+ sqm.on_connect
		sqm_refresh(device, false);
		//- sqm.on_connect
			indigo_define_property(device, SQM_INPUT_PROPERTY, NULL);
			indigo_define_property(device, SQM_WEATHER_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			SQM_INPUT_PROPERTY,
			SQM_WEATHER_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, SQM_INPUT_PROPERTY, NULL);
		indigo_delete_property(device, SQM_WEATHER_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void sqm_sqm_input_handler(indigo_device *device) {
	SQM_INPUT_PROPERTY->state = INDIGO_OK_STATE;
	//+ sqm.SQM_INPUT.on_change
	sqm_refresh(device, true);
	//- sqm.SQM_INPUT.on_change
	indigo_update_property(device, SQM_INPUT_PROPERTY, NULL);
}

#pragma mark - Device API (sqm)

static indigo_result sqm_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result sqm_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM) == INDIGO_OK) {
		//+ sqm.on_attach
		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		//- sqm.on_attach
		SQM_INPUT_PROPERTY = indigo_init_number_property(NULL, device->name, SQM_INPUT_PROPERTY_NAME, "Simulation", "Simulated sky quality", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (SQM_INPUT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(SQM_INPUT_BRIGHTNESS_ITEM, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness [mag/arcsec^2]", 0, 25, 0, 21.5);
		indigo_init_number_item(SQM_INPUT_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sensor temperature [C]", -50, 100, 0, 10);
		SQM_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Main", "Sky quality", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (SQM_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(SQM_BRIGHTNESS_ITEM, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness [mag/arcsec^2]", 0, 25, 0, 21.5);
		indigo_init_number_item(SQM_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sensor temperature [C]", -50, 100, 0, 10);
		indigo_init_number_item(SQM_BORTLE_ITEM, AUX_WEATHER_SKY_BORTLE_CLASS_ITEM_NAME, "Bortle class", 1, 9, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return sqm_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result sqm_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(SQM_INPUT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(SQM_WEATHER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result sqm_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(sqm_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(SQM_INPUT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(SQM_INPUT_PROPERTY, sqm_sqm_input_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result sqm_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		sqm_connection_handler(device);
	}
	indigo_release_property(SQM_INPUT_PROPERTY);
	indigo_release_property(SQM_WEATHER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (flat)
// device_id: flat type: aux

static void flat_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//+ flat.on_connect
		indigo_set_switch(FLAT_COVER_PROPERTY, PRIVATE_DATA->cover_open ? FLAT_OPEN_ITEM : FLAT_CLOSE_ITEM, true);
		FLAT_COVER_PROPERTY->state = INDIGO_OK_STATE;
		//- flat.on_connect
			indigo_define_property(device, FLAT_LIGHT_PROPERTY, NULL);
			indigo_define_property(device, FLAT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, FLAT_COVER_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ flat.on_disconnect
		// The generated connection handler cancels delayed work before deleting properties.
		indigo_set_switch(FLAT_COVER_PROPERTY, PRIVATE_DATA->cover_open ? FLAT_OPEN_ITEM : FLAT_CLOSE_ITEM, true);
		FLAT_COVER_PROPERTY->state = INDIGO_OK_STATE;
		//- flat.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			FLAT_LIGHT_PROPERTY,
			FLAT_INTENSITY_PROPERTY,
			FLAT_COVER_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, FLAT_LIGHT_PROPERTY, NULL);
		indigo_delete_property(device, FLAT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, FLAT_COVER_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void flat_flat_cover_handler(indigo_device *device) {
	//+ flat.FLAT_COVER.on_change
	if (indigo_get_switch_target(FLAT_COVER_PROPERTY, AUX_COVER_OPEN_ITEM_NAME) == PRIVATE_DATA->cover_open) {
		INDIGO_UPDATE_PROPERTY_STATE(FLAT_COVER_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		INDIGO_UPDATE_PROPERTY_STATE(FLAT_COVER_PROPERTY, INDIGO_BUSY_STATE, NULL);
		indigo_execute_handler_in(device, 1, flat_cover_finalizer);
	}
	//- flat.FLAT_COVER.on_change
}

#pragma mark - Device API (flat)

static indigo_result flat_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result flat_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_DUSTCAP | INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		//+ flat.on_attach
		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		//- flat.on_attach
		FLAT_LIGHT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, "Main", "Light on/off", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (FLAT_LIGHT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(FLAT_LIGHT_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(FLAT_LIGHT_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		FLAT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, "Main", "Light intensity [%]", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (FLAT_INTENSITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(FLAT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity [%]", 0, 100, 1, 50);
		FLAT_COVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_COVER_PROPERTY_NAME, "Main", "Cover open/close", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (FLAT_COVER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(FLAT_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(FLAT_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Close", true);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return flat_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result flat_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(FLAT_LIGHT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(FLAT_INTENSITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(FLAT_COVER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result flat_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(flat_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FLAT_LIGHT_PROPERTY, property)) {
		indigo_property_copy_values(FLAT_LIGHT_PROPERTY, property, false);
		FLAT_LIGHT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FLAT_LIGHT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FLAT_INTENSITY_PROPERTY, property)) {
		indigo_property_copy_values(FLAT_INTENSITY_PROPERTY, property, false);
		FLAT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FLAT_INTENSITY_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FLAT_COVER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FLAT_COVER_PROPERTY, flat_flat_cover_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result flat_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		flat_connection_handler(device);
	}
	indigo_release_property(FLAT_LIGHT_PROPERTY);
	indigo_release_property(FLAT_INTENSITY_PROPERTY);
	indigo_release_property(FLAT_COVER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device powerbox_template = INDIGO_DEVICE_INITIALIZER(POWERBOX_DEVICE_NAME, powerbox_attach, powerbox_enumerate_properties, powerbox_change_property, NULL, powerbox_detach);

static indigo_device weather_template = INDIGO_DEVICE_INITIALIZER(WEATHER_DEVICE_NAME, weather_attach, weather_enumerate_properties, weather_change_property, NULL, weather_detach);

static indigo_device sqm_template = INDIGO_DEVICE_INITIALIZER(SQM_DEVICE_NAME, sqm_attach, sqm_enumerate_properties, sqm_change_property, NULL, sqm_detach);

static indigo_device flat_template = INDIGO_DEVICE_INITIALIZER(FLAT_DEVICE_NAME, flat_attach, flat_enumerate_properties, flat_change_property, NULL, flat_detach);

#pragma mark - Main code

indigo_result indigo_aux_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *powerbox = NULL;
	static indigo_device *weather = NULL;
	static indigo_device *sqm = NULL;
	static indigo_device *flat = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (simulator_private_data *)indigo_safe_malloc(sizeof(simulator_private_data));
			powerbox = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &powerbox_template);
			powerbox->private_data = private_data;
			powerbox->master_device = powerbox;
			indigo_attach_device(powerbox);
			weather = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &weather_template);
			weather->private_data = private_data;
			weather->master_device = powerbox;
			indigo_attach_device(weather);
			sqm = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &sqm_template);
			sqm->private_data = private_data;
			sqm->master_device = powerbox;
			indigo_attach_device(sqm);
			flat = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &flat_template);
			flat->private_data = private_data;
			flat->master_device = powerbox;
			indigo_attach_device(flat);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(powerbox);
			VERIFY_NOT_CONNECTED(weather);
			VERIFY_NOT_CONNECTED(sqm);
			VERIFY_NOT_CONNECTED(flat);
			last_action = action;
			if (flat != NULL) {
				indigo_detach_device(flat);
				indigo_safe_free(flat);
				flat = NULL;
			}
			if (sqm != NULL) {
				indigo_detach_device(sqm);
				indigo_safe_free(sqm);
				sqm = NULL;
			}
			if (weather != NULL) {
				indigo_detach_device(weather);
				indigo_safe_free(weather);
				weather = NULL;
			}
			if (powerbox != NULL) {
				indigo_detach_device(powerbox);
				indigo_safe_free(powerbox);
				powerbox = NULL;
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
