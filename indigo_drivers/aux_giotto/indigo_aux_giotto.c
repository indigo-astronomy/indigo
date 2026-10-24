// Copyright (c) 2026 Rumen G. Bogdanovski
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

// This file generated from indigo_aux_giotto.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include "../shared/primalucelib/primalucelib.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_giotto.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_aux_giotto"
#define DRIVER_LABEL         "PrimaluceLab GIOTTO Flat Panel"
#define AUX_DEVICE_NAME      "PrimaluceLab GIOTTO"
#define PRIVATE_DATA         ((giotto_private_data *)device->private_data)

//+ define

// The controller connection, see indigo_drivers/shared/primalucelib (linked as a static library).
#define LINK                 (&PRIVATE_DATA->link)
// The light and the brightness are polled this often (seconds); the temperature and the supplies every
// ENVIRONMENT_TICKS polls.
#define POLL_PERIOD          2
#define ENVIRONMENT_TICKS    5

//- define

#pragma mark - Property definitions

#define AUX_LIGHT_SWITCH_PROPERTY      (PRIVATE_DATA->aux_light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM       (AUX_LIGHT_SWITCH_PROPERTY->items + 0)
#define AUX_LIGHT_SWITCH_OFF_ITEM      (AUX_LIGHT_SWITCH_PROPERTY->items + 1)

#define AUX_LIGHT_INTENSITY_PROPERTY   (PRIVATE_DATA->aux_light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM       (AUX_LIGHT_INTENSITY_PROPERTY->items + 0)

#define X_STATE_PROPERTY               (PRIVATE_DATA->x_state_property)
#define X_STATE_TEMPERATURE_ITEM       (X_STATE_PROPERTY->items + 0)
#define X_STATE_VIN_12V_ITEM           (X_STATE_PROPERTY->items + 1)
#define X_STATE_VIN_USB_ITEM           (X_STATE_PROPERTY->items + 2)

#define X_STATE_PROPERTY_NAME          "X_STATE"
#define X_STATE_TEMPERATURE_ITEM_NAME  "TEMPERATURE"
#define X_STATE_VIN_12V_ITEM_NAME      "VIN_12V"
#define X_STATE_VIN_USB_ITEM_NAME      "VIN_USB"

#define X_PRESETS_PROPERTY             (PRIVATE_DATA->x_presets_property)
#define X_PRESETS_1_ITEM               (X_PRESETS_PROPERTY->items + 0)
#define X_PRESETS_2_ITEM               (X_PRESETS_PROPERTY->items + 1)
#define X_PRESETS_3_ITEM               (X_PRESETS_PROPERTY->items + 2)
#define X_PRESETS_4_ITEM               (X_PRESETS_PROPERTY->items + 3)
#define X_PRESETS_5_ITEM               (X_PRESETS_PROPERTY->items + 4)
#define X_PRESETS_6_ITEM               (X_PRESETS_PROPERTY->items + 5)
#define X_PRESETS_7_ITEM               (X_PRESETS_PROPERTY->items + 6)
#define X_PRESETS_8_ITEM               (X_PRESETS_PROPERTY->items + 7)
#define X_PRESETS_9_ITEM               (X_PRESETS_PROPERTY->items + 8)

#define X_PRESETS_PROPERTY_NAME        "X_PRESETS"
#define X_PRESETS_1_ITEM_NAME          "PRESET_1"
#define X_PRESETS_2_ITEM_NAME          "PRESET_2"
#define X_PRESETS_3_ITEM_NAME          "PRESET_3"
#define X_PRESETS_4_ITEM_NAME          "PRESET_4"
#define X_PRESETS_5_ITEM_NAME          "PRESET_5"
#define X_PRESETS_6_ITEM_NAME          "PRESET_6"
#define X_PRESETS_7_ITEM_NAME          "PRESET_7"
#define X_PRESETS_8_ITEM_NAME          "PRESET_8"
#define X_PRESETS_9_ITEM_NAME          "PRESET_9"

#define X_PRESET_NAMES_PROPERTY        (PRIVATE_DATA->x_preset_names_property)
#define X_PRESET_NAMES_1_ITEM          (X_PRESET_NAMES_PROPERTY->items + 0)
#define X_PRESET_NAMES_2_ITEM          (X_PRESET_NAMES_PROPERTY->items + 1)
#define X_PRESET_NAMES_3_ITEM          (X_PRESET_NAMES_PROPERTY->items + 2)
#define X_PRESET_NAMES_4_ITEM          (X_PRESET_NAMES_PROPERTY->items + 3)
#define X_PRESET_NAMES_5_ITEM          (X_PRESET_NAMES_PROPERTY->items + 4)
#define X_PRESET_NAMES_6_ITEM          (X_PRESET_NAMES_PROPERTY->items + 5)
#define X_PRESET_NAMES_7_ITEM          (X_PRESET_NAMES_PROPERTY->items + 6)
#define X_PRESET_NAMES_8_ITEM          (X_PRESET_NAMES_PROPERTY->items + 7)
#define X_PRESET_NAMES_9_ITEM          (X_PRESET_NAMES_PROPERTY->items + 8)

#define X_PRESET_NAMES_PROPERTY_NAME   "X_PRESET_NAMES"
#define X_PRESET_NAMES_1_ITEM_NAME     "PRESET_1"
#define X_PRESET_NAMES_2_ITEM_NAME     "PRESET_2"
#define X_PRESET_NAMES_3_ITEM_NAME     "PRESET_3"
#define X_PRESET_NAMES_4_ITEM_NAME     "PRESET_4"
#define X_PRESET_NAMES_5_ITEM_NAME     "PRESET_5"
#define X_PRESET_NAMES_6_ITEM_NAME     "PRESET_6"
#define X_PRESET_NAMES_7_ITEM_NAME     "PRESET_7"
#define X_PRESET_NAMES_8_ITEM_NAME     "PRESET_8"
#define X_PRESET_NAMES_9_ITEM_NAME     "PRESET_9"

#define X_PRESET_RECALL_PROPERTY       (PRIVATE_DATA->x_preset_recall_property)
#define X_PRESET_RECALL_1_ITEM         (X_PRESET_RECALL_PROPERTY->items + 0)
#define X_PRESET_RECALL_2_ITEM         (X_PRESET_RECALL_PROPERTY->items + 1)
#define X_PRESET_RECALL_3_ITEM         (X_PRESET_RECALL_PROPERTY->items + 2)
#define X_PRESET_RECALL_4_ITEM         (X_PRESET_RECALL_PROPERTY->items + 3)
#define X_PRESET_RECALL_5_ITEM         (X_PRESET_RECALL_PROPERTY->items + 4)
#define X_PRESET_RECALL_6_ITEM         (X_PRESET_RECALL_PROPERTY->items + 5)
#define X_PRESET_RECALL_7_ITEM         (X_PRESET_RECALL_PROPERTY->items + 6)
#define X_PRESET_RECALL_8_ITEM         (X_PRESET_RECALL_PROPERTY->items + 7)
#define X_PRESET_RECALL_9_ITEM         (X_PRESET_RECALL_PROPERTY->items + 8)

#define X_PRESET_RECALL_PROPERTY_NAME  "X_PRESET_RECALL"
#define X_PRESET_RECALL_1_ITEM_NAME    "PRESET_1"
#define X_PRESET_RECALL_2_ITEM_NAME    "PRESET_2"
#define X_PRESET_RECALL_3_ITEM_NAME    "PRESET_3"
#define X_PRESET_RECALL_4_ITEM_NAME    "PRESET_4"
#define X_PRESET_RECALL_5_ITEM_NAME    "PRESET_5"
#define X_PRESET_RECALL_6_ITEM_NAME    "PRESET_6"
#define X_PRESET_RECALL_7_ITEM_NAME    "PRESET_7"
#define X_PRESET_RECALL_8_ITEM_NAME    "PRESET_8"
#define X_PRESET_RECALL_9_ITEM_NAME    "PRESET_9"

#define X_FADING_PROPERTY              (PRIVATE_DATA->x_fading_property)
#define X_FADING_ON_ITEM               (X_FADING_PROPERTY->items + 0)
#define X_FADING_OFF_ITEM              (X_FADING_PROPERTY->items + 1)

#define X_FADING_PROPERTY_NAME         "X_FADING"
#define X_FADING_ON_ITEM_NAME          "ON"
#define X_FADING_OFF_ITEM_NAME         "OFF"

#define X_POWER_ON_LIGHT_PROPERTY      (PRIVATE_DATA->x_power_on_light_property)
#define X_POWER_ON_LIGHT_ON_ITEM       (X_POWER_ON_LIGHT_PROPERTY->items + 0)
#define X_POWER_ON_LIGHT_OFF_ITEM      (X_POWER_ON_LIGHT_PROPERTY->items + 1)

#define X_POWER_ON_LIGHT_PROPERTY_NAME "X_POWER_ON_LIGHT"
#define X_POWER_ON_LIGHT_ON_ITEM_NAME  "ON"
#define X_POWER_ON_LIGHT_OFF_ITEM_NAME "OFF"

#define X_WIFI_PROPERTY                (PRIVATE_DATA->x_wifi_property)
#define X_WIFI_OFF_ITEM                (X_WIFI_PROPERTY->items + 0)
#define X_WIFI_AP_ITEM                 (X_WIFI_PROPERTY->items + 1)
#define X_WIFI_STA_ITEM                (X_WIFI_PROPERTY->items + 2)

#define X_WIFI_PROPERTY_NAME           "X_WIFI"
#define X_WIFI_OFF_ITEM_NAME           "OFF"
#define X_WIFI_AP_ITEM_NAME            "AP"
#define X_WIFI_STA_ITEM_NAME           "STA"

#define X_WIFI_AP_PROPERTY             (PRIVATE_DATA->x_wifi_ap_property)
#define X_WIFI_AP_SSID_ITEM            (X_WIFI_AP_PROPERTY->items + 0)
#define X_WIFI_AP_PASSWORD_ITEM        (X_WIFI_AP_PROPERTY->items + 1)

#define X_WIFI_AP_PROPERTY_NAME        "X_WIFI_AP"
#define X_WIFI_AP_SSID_ITEM_NAME       "AP_SSID"
#define X_WIFI_AP_PASSWORD_ITEM_NAME   "AP_PASSWORD"

#define X_WIFI_STA_PROPERTY            (PRIVATE_DATA->x_wifi_sta_property)
#define X_WIFI_STA_SSID_ITEM           (X_WIFI_STA_PROPERTY->items + 0)
#define X_WIFI_STA_PASSWORD_ITEM       (X_WIFI_STA_PROPERTY->items + 1)

#define X_WIFI_STA_PROPERTY_NAME       "X_WIFI_STA"
#define X_WIFI_STA_SSID_ITEM_NAME      "STA_SSID"
#define X_WIFI_STA_PASSWORD_ITEM_NAME  "STA_PASSWORD"

#define X_LEDS_PROPERTY                (PRIVATE_DATA->x_leds_property)
#define X_LEDS_OFF_ITEM                (X_LEDS_PROPERTY->items + 0)
#define X_LEDS_DIM_ITEM                (X_LEDS_PROPERTY->items + 1)
#define X_LEDS_MIDDLE_ITEM             (X_LEDS_PROPERTY->items + 2)
#define X_LEDS_ON_ITEM                 (X_LEDS_PROPERTY->items + 3)

#define X_LEDS_PROPERTY_NAME           "X_LEDS"
#define X_LEDS_OFF_ITEM_NAME           "OFF"
#define X_LEDS_DIM_ITEM_NAME           "DIM"
#define X_LEDS_MIDDLE_ITEM_NAME        "MIDDLE"
#define X_LEDS_ON_ITEM_NAME            "ON"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_light_switch_property;
	indigo_property *aux_light_intensity_property;
	indigo_property *x_state_property;
	indigo_property *x_presets_property;
	indigo_property *x_preset_names_property;
	indigo_property *x_preset_recall_property;
	indigo_property *x_fading_property;
	indigo_property *x_power_on_light_property;
	indigo_property *x_wifi_property;
	indigo_property *x_wifi_ap_property;
	indigo_property *x_wifi_sta_property;
	indigo_property *x_leds_property;
	//+ data
	primalucelib_link link;
	// The light and brightness the panel reported or accepted last.
	bool light;
	int brightness;
	int preset_brightness[PRIMALUCE_PANEL_PRESET_COUNT];
	char preset_names[PRIMALUCE_PANEL_PRESET_COUNT][INDIGO_VALUE_SIZE];
	indigo_item *leds_item, *fading_item, *power_on_item, *wifi_item;
	int environment_ticks;
	bool poll_alert;
	//- data
} giotto_private_data;

#pragma mark - Low level code

//+ code

static indigo_item *selected_item(indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value) {
			return property->items + i;
		}
	}
	return NULL;
}

// A refused switch request keeps the item the device has.
static void restore_switch(indigo_property *property, indigo_item *item) {
	if (item != NULL) {
		indigo_set_switch(property, item, true);
	}
	property->state = INDIGO_ALERT_STATE;
}

// Publishes property with the reason the controller gave for the last refusal, if it gave one.
static void update_with_refusal(indigo_device *device, indigo_property *property) {
	const char *reason = primalucelib_last_error(LINK);
	if (*reason) {
		indigo_update_property(device, property, "%s", reason);
	} else {
		indigo_update_property(device, property, NULL);
	}
}

static void giotto_publish_light(indigo_device *device) {
	indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, PRIVATE_DATA->light ? AUX_LIGHT_SWITCH_ON_ITEM : AUX_LIGHT_SWITCH_OFF_ITEM, true);
	AUX_LIGHT_INTENSITY_ITEM->number.value = AUX_LIGHT_INTENSITY_ITEM->number.target = PRIVATE_DATA->brightness;
}

static bool giotto_open(indigo_device *device, const char **error_message) {
	if (primalucelib_open(LINK, DRIVER_NAME, DEVICE_PORT_ITEM->text.value, error_message)) {
		char *text = primalucelib_get_model_name(LINK);
		if (text != NULL && primalucelib_model_from_name(text) == PRIMALUCELIB_MODEL_GIOTTO) {
			// Diagnostic output would interleave with the replies on the serial line.
			primalucelib_disable_log_output(LINK);
			// The full state stays in the link for on_connect. The brightness range comes from the factory
			// calibration; without it nothing can be set.
			if (primalucelib_get_state(LINK) && primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_MAX_BRIGHTNESS) > 0) {
				return true;
			}
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No brightness range in the panel state");
			*error_message = "The panel reports no brightness range";
		} else if (text != NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' is not a GIOTTO", text);
			*error_message = "The device is not a GIOTTO";
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No reply to the model request");
		}
		primalucelib_close(LINK);
	}
	return false;
}

static void giotto_close(indigo_device *device) {
	if (primalucelib_is_open(LINK)) {
		primalucelib_close(LINK);
	}
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	bool on;
	int brightness;
	bool polled = primalucelib_panel_get_state(LINK, &on, &brightness);
	// A request accepted meanwhile (the property is BUSY until its handler runs) owns its property: the poll
	// neither publishes over it nor changes the value the handler compares with; the next poll looks again.
	bool switch_free = AUX_LIGHT_SWITCH_PROPERTY->state != INDIGO_BUSY_STATE;
	bool intensity_free = AUX_LIGHT_INTENSITY_PROPERTY->state != INDIGO_BUSY_STATE;
	if (polled) {
		// Changes made on the web page or by a WiFi client are published as they are found.
		if (switch_free && (on != PRIVATE_DATA->light || PRIVATE_DATA->poll_alert)) {
			PRIVATE_DATA->light = on;
			indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, on ? AUX_LIGHT_SWITCH_ON_ITEM : AUX_LIGHT_SWITCH_OFF_ITEM, true);
			AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		}
		if (intensity_free && (brightness != PRIVATE_DATA->brightness || PRIVATE_DATA->poll_alert)) {
			PRIVATE_DATA->brightness = brightness;
			AUX_LIGHT_INTENSITY_ITEM->number.value = AUX_LIGHT_INTENSITY_ITEM->number.target = brightness;
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		}
		PRIVATE_DATA->poll_alert = false;
	} else if (!PRIVATE_DATA->poll_alert) {
		// A failed poll keeps the last values; the next good poll restores OK.
		PRIVATE_DATA->poll_alert = true;
		if (switch_free) {
			AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		}
		if (intensity_free) {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		}
	}
	if (++PRIVATE_DATA->environment_ticks % ENVIRONMENT_TICKS == 0) {
		double temperature, supply, usb;
		if (primalucelib_panel_get_environment(LINK, &temperature, &supply, &usb)) {
			X_STATE_TEMPERATURE_ITEM->number.value = temperature;
			X_STATE_VIN_12V_ITEM->number.value = supply;
			X_STATE_VIN_USB_ITEM->number.value = usb;
			X_STATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			X_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_STATE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, POLL_PERIOD, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		connection_result = giotto_open(device, &error_message);
		if (connection_result) {
			//+ aux.on_connect
			// giotto_open() left the full state in the link.
			char *text;
			int max_brightness = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_MAX_BRIGHTNESS);
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_MODEL_NAME))) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, text);
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_APPLICATION_FIRMWARE))) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_WEB_FIRMWARE))) {
					// The web firmware is appended within the item; a long version is cut off instead of overflowing it.
					size_t used = strlen(INFO_DEVICE_FW_REVISION_ITEM->text.value);
					snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value + used, sizeof(INFO_DEVICE_FW_REVISION_ITEM->text.value) - used, " / %s", text);
				}
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_SERIAL_NUMBER))) {
				INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, text);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			if (primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_CALIBRATION_RESTART_CH1) || primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_CALIBRATION_RESTART_CH2)) {
				indigo_send_message(device, BUSY_PROPERTY, "%s needs calibration", INFO_DEVICE_MODEL_ITEM->text.value);
			}
			PRIVATE_DATA->light = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_LIGHT) == 1;
			PRIVATE_DATA->brightness = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_BRIGHTNESS);
			AUX_LIGHT_INTENSITY_ITEM->number.max = max_brightness;
			giotto_publish_light(device);
			AUX_LIGHT_SWITCH_PROPERTY->state = AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			for (int i = 0; i < PRIMALUCE_PANEL_PRESET_COUNT; i++) {
				PRIVATE_DATA->preset_brightness[i] = (int)primalucelib_reply_panel_preset_brightness(LINK, i + 1);
				X_PRESETS_PROPERTY->items[i].number.max = max_brightness;
				X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = PRIVATE_DATA->preset_brightness[i];
				INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], (text = primalucelib_reply_stored_position_name(LINK, i + 1)) ? text : "");
				INDIGO_COPY_VALUE(X_PRESET_NAMES_PROPERTY->items[i].text.value, PRIVATE_DATA->preset_names[i]);
			}
			indigo_set_switch(X_FADING_PROPERTY, primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_FADING) ? X_FADING_ON_ITEM : X_FADING_OFF_ITEM, true);
			PRIVATE_DATA->fading_item = selected_item(X_FADING_PROPERTY);
			indigo_set_switch(X_POWER_ON_LIGHT_PROPERTY, primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_LIGHT_AT_POWER_ON) ? X_POWER_ON_LIGHT_ON_ITEM : X_POWER_ON_LIGHT_OFF_ITEM, true);
			PRIVATE_DATA->power_on_item = selected_item(X_POWER_ON_LIGHT_PROPERTY);
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_LEDS))) {
				if (!strcmp(text, PRIMALUCE_LEDS_ON)) {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_ON_ITEM, true);
				} else if (!strcmp(text, PRIMALUCE_LEDS_LOW)) {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_DIM_ITEM, true);
				} else if (!strcmp(text, PRIMALUCE_LEDS_MIDDLE)) {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_MIDDLE_ITEM, true);
				} else {
					indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_OFF_ITEM, true);
				}
			}
			PRIVATE_DATA->leds_item = selected_item(X_LEDS_PROPERTY);
			// LANCFG selects between the access point and a connection to an existing network.
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_LAN_CONFIG)) && !strcmp(text, PRIMALUCE_LAN_STATION)) {
				indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_STA_ITEM, true);
			} else if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ACCESS_POINT_STATUS))) {
				indigo_set_switch(X_WIFI_PROPERTY, strcmp(text, PRIMALUCE_ACCESS_POINT_ON) ? X_WIFI_OFF_ITEM : X_WIFI_AP_ITEM, true);
			}
			PRIVATE_DATA->wifi_item = selected_item(X_WIFI_PROPERTY);
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ACCESS_POINT_SSID))) {
				INDIGO_COPY_VALUE(X_WIFI_AP_SSID_ITEM->text.value, text);
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ACCESS_POINT_PASSWORD))) {
				INDIGO_COPY_VALUE(X_WIFI_AP_PASSWORD_ITEM->text.value, text);
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_STATION_SSID))) {
				INDIGO_COPY_VALUE(X_WIFI_STA_SSID_ITEM->text.value, text);
			}
			if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_STATION_PASSWORD))) {
				INDIGO_COPY_VALUE(X_WIFI_STA_PASSWORD_ITEM->text.value, text);
			}
			X_STATE_TEMPERATURE_ITEM->number.value = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_PANEL_TEMPERATURE);
			X_STATE_VIN_12V_ITEM->number.value = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_SUPPLY_VOLTAGE);
			X_STATE_VIN_USB_ITEM->number.value = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_USB_VOLTAGE);
			PRIVATE_DATA->environment_ticks = 0;
			PRIVATE_DATA->poll_alert = false;
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, X_STATE_PROPERTY, NULL);
			indigo_define_property(device, X_PRESETS_PROPERTY, NULL);
			indigo_define_property(device, X_PRESET_NAMES_PROPERTY, NULL);
			indigo_define_property(device, X_PRESET_RECALL_PROPERTY, NULL);
			indigo_define_property(device, X_FADING_PROPERTY, NULL);
			indigo_define_property(device, X_POWER_ON_LIGHT_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_AP_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_STA_PROPERTY, NULL);
			indigo_define_property(device, X_LEDS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		// The light is switched off, as other flat panel drivers do; the panel keeps the brightness for the next use.
		if (!primalucelib_panel_set_light(LINK, false)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to switch the light off");
		}
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		//- aux.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			AUX_LIGHT_SWITCH_PROPERTY,
			AUX_LIGHT_INTENSITY_PROPERTY,
			X_STATE_PROPERTY,
			X_PRESETS_PROPERTY,
			X_PRESET_NAMES_PROPERTY,
			X_PRESET_RECALL_PROPERTY,
			X_FADING_PROPERTY,
			X_POWER_ON_LIGHT_PROPERTY,
			X_WIFI_PROPERTY,
			X_WIFI_AP_PROPERTY,
			X_WIFI_STA_PROPERTY,
			X_LEDS_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, X_STATE_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESETS_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESET_NAMES_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESET_RECALL_PROPERTY, NULL);
		indigo_delete_property(device, X_FADING_PROPERTY, NULL);
		indigo_delete_property(device, X_POWER_ON_LIGHT_PROPERTY, NULL);
		indigo_delete_property(device, X_WIFI_PROPERTY, NULL);
		indigo_delete_property(device, X_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, X_WIFI_STA_PROPERTY, NULL);
		indigo_delete_property(device, X_LEDS_PROPERTY, NULL);
		giotto_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_timer_callback);
	}
}

static void aux_light_switch_handler(indigo_device *device) {
	AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_SWITCH.on_change
	bool on = AUX_LIGHT_SWITCH_ON_ITEM->sw.value;
	if (primalucelib_panel_set_light(LINK, on)) {
		PRIVATE_DATA->light = on;
	} else {
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, PRIVATE_DATA->light ? AUX_LIGHT_SWITCH_ON_ITEM : AUX_LIGHT_SWITCH_OFF_ITEM, true);
		update_with_refusal(device, AUX_LIGHT_SWITCH_PROPERTY);
		return;
	}
	//- aux.AUX_LIGHT_SWITCH.on_change
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
}

static void aux_light_intensity_handler(indigo_device *device) {
	AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_INTENSITY.on_change
	// The brightness is a device level (0 to MAX_BRIGHTNESS); the panel keeps it while the light is off.
	int brightness = (int)AUX_LIGHT_INTENSITY_ITEM->number.target;
	if (primalucelib_panel_set_brightness(LINK, brightness)) {
		PRIVATE_DATA->brightness = brightness;
	} else {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
		AUX_LIGHT_INTENSITY_ITEM->number.value = AUX_LIGHT_INTENSITY_ITEM->number.target = PRIVATE_DATA->brightness;
		update_with_refusal(device, AUX_LIGHT_INTENSITY_PROPERTY);
		return;
	}
	//- aux.AUX_LIGHT_INTENSITY.on_change
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
}

static void aux_x_presets_handler(indigo_device *device) {
	X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_PRESETS.on_change
	// Only changed presets are written; the panel refuses the brightness of a preset without a name. The first
	// refusal is reported.
	char reason_text[INDIGO_VALUE_SIZE];
	const char *reason = NULL;
	for (int i = 0; i < PRIMALUCE_PANEL_PRESET_COUNT; i++) {
		int brightness = (int)X_PRESETS_PROPERTY->items[i].number.target;
		if (brightness == PRIVATE_DATA->preset_brightness[i]) {
			continue;
		}
		if (primalucelib_panel_set_preset_brightness(LINK, i + 1, brightness)) {
			PRIVATE_DATA->preset_brightness[i] = brightness;
		} else {
			X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
			if (reason == NULL && *primalucelib_last_error(LINK) && PRIVATE_DATA->preset_names[i][0] == 0) {
				snprintf(reason_text, sizeof(reason_text), "Preset #%d needs a name before its brightness can be set", i + 1);
				reason = reason_text;
			} else if (reason == NULL && *primalucelib_last_error(LINK)) {
				INDIGO_COPY_VALUE(reason_text, primalucelib_last_error(LINK));
				reason = reason_text;
			}
		}
		X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = PRIVATE_DATA->preset_brightness[i];
	}
	if (reason != NULL) {
		indigo_update_property(device, X_PRESETS_PROPERTY, "%s", reason);
		return;
	}
	//- aux.X_PRESETS.on_change
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
}

static void aux_x_preset_names_handler(indigo_device *device) {
	X_PRESET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_PRESET_NAMES.on_change
	// An empty name deletes the preset. The first refusal is reported.
	char reason_text[INDIGO_VALUE_SIZE];
	const char *reason = NULL;
	for (int i = 0; i < PRIMALUCE_PANEL_PRESET_COUNT; i++) {
		char *name = X_PRESET_NAMES_PROPERTY->items[i].text.value;
		if (!strcmp(name, PRIVATE_DATA->preset_names[i])) {
			continue;
		}
		primalucelib_clear_last_error(LINK);
		if (primalucelib_panel_set_preset_name(LINK, i + 1, name)) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], name);
		} else {
			X_PRESET_NAMES_PROPERTY->state = INDIGO_ALERT_STATE;
			if (reason == NULL && *primalucelib_last_error(LINK)) {
				INDIGO_COPY_VALUE(reason_text, primalucelib_last_error(LINK));
				reason = reason_text;
			} else if (reason == NULL && (strchr(name, '"') != NULL || strchr(name, '\\') != NULL)) {
				// The protocol has no escaping, so such a name is not sent.
				reason = "A preset name cannot contain \" or \\";
			}
			INDIGO_COPY_VALUE(name, PRIVATE_DATA->preset_names[i]);
		}
	}
	if (reason != NULL) {
		indigo_update_property(device, X_PRESET_NAMES_PROPERTY, "%s", reason);
		return;
	}
	//- aux.X_PRESET_NAMES.on_change
	indigo_update_property(device, X_PRESET_NAMES_PROPERTY, NULL);
}

static void aux_x_preset_recall_handler(indigo_device *device) {
	X_PRESET_RECALL_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_PRESET_RECALL.on_change
	int index = -1;
	for (int i = 0; i < PRIMALUCE_PANEL_PRESET_COUNT; i++) {
		if (X_PRESET_RECALL_PROPERTY->items[i].sw.value) {
			index = i;
		}
		X_PRESET_RECALL_PROPERTY->items[i].sw.value = false;
	}
	if (index >= 0) {
		int brightness;
		// The panel sets the brightness of the preset and leaves the light as it is.
		if (!primalucelib_panel_recall_preset(LINK, index + 1)) {
			X_PRESET_RECALL_PROPERTY->state = INDIGO_ALERT_STATE;
			update_with_refusal(device, X_PRESET_RECALL_PROPERTY);
			return;
		}
		if (primalucelib_panel_get_brightness(LINK, &brightness)) {
			PRIVATE_DATA->brightness = brightness;
		}
		AUX_LIGHT_INTENSITY_ITEM->number.value = AUX_LIGHT_INTENSITY_ITEM->number.target = PRIVATE_DATA->brightness;
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	}
	//- aux.X_PRESET_RECALL.on_change
	indigo_update_property(device, X_PRESET_RECALL_PROPERTY, NULL);
}

static void aux_x_fading_handler(indigo_device *device) {
	X_FADING_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_FADING.on_change
	if (primalucelib_panel_set_fading(LINK, X_FADING_ON_ITEM->sw.value)) {
		PRIVATE_DATA->fading_item = selected_item(X_FADING_PROPERTY);
	} else {
		restore_switch(X_FADING_PROPERTY, PRIVATE_DATA->fading_item);
		update_with_refusal(device, X_FADING_PROPERTY);
		return;
	}
	//- aux.X_FADING.on_change
	indigo_update_property(device, X_FADING_PROPERTY, NULL);
}

static void aux_x_power_on_light_handler(indigo_device *device) {
	X_POWER_ON_LIGHT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_POWER_ON_LIGHT.on_change
	if (primalucelib_panel_set_light_at_power_on(LINK, X_POWER_ON_LIGHT_ON_ITEM->sw.value)) {
		PRIVATE_DATA->power_on_item = selected_item(X_POWER_ON_LIGHT_PROPERTY);
	} else {
		restore_switch(X_POWER_ON_LIGHT_PROPERTY, PRIVATE_DATA->power_on_item);
		update_with_refusal(device, X_POWER_ON_LIGHT_PROPERTY);
		return;
	}
	//- aux.X_POWER_ON_LIGHT.on_change
	indigo_update_property(device, X_POWER_ON_LIGHT_PROPERTY, NULL);
}

static void aux_x_wifi_handler(indigo_device *device) {
	X_WIFI_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_WIFI.on_change
	// Off is the access point mode with the access point switched off. Switching between station mode and the
	// access point restarts the panel; over USB the handler waits until it answers again (about 2 s).
	bool restarted = false;
	if (primalucelib_select_wifi(LINK, X_WIFI_STA_ITEM->sw.value ? PRIMALUCE_WIFI_STATION : X_WIFI_AP_ITEM->sw.value ? PRIMALUCE_WIFI_ACCESS_POINT : PRIMALUCE_WIFI_OFF, &restarted)) {
		PRIVATE_DATA->wifi_item = selected_item(X_WIFI_PROPERTY);
		if (restarted && primalucelib_is_http(LINK)) {
			indigo_send_message(device, X_WIFI_PROPERTY, "The panel restarts to apply the WiFi mode and leaves its current network");
		}
	} else {
		restore_switch(X_WIFI_PROPERTY, PRIVATE_DATA->wifi_item);
		update_with_refusal(device, X_WIFI_PROPERTY);
		return;
	}
	//- aux.X_WIFI.on_change
	indigo_update_property(device, X_WIFI_PROPERTY, NULL);
}

static void aux_x_wifi_ap_handler(indigo_device *device) {
	X_WIFI_AP_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_WIFI_AP.on_change
	// The library refuses what WiFi cannot use (an SSID of 1 to 32 characters and a WPA2 password of 8 to 63);
	// the panel would store it.
	if (!primalucelib_set_access_point_credentials(LINK, X_WIFI_AP_SSID_ITEM->text.value, X_WIFI_AP_PASSWORD_ITEM->text.value)) {
		X_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
		update_with_refusal(device, X_WIFI_AP_PROPERTY);
		return;
	}
	//- aux.X_WIFI_AP.on_change
	indigo_update_property(device, X_WIFI_AP_PROPERTY, NULL);
}

static void aux_x_wifi_sta_handler(indigo_device *device) {
	X_WIFI_STA_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_WIFI_STA.on_change
	if (!primalucelib_set_station_credentials(LINK, X_WIFI_STA_SSID_ITEM->text.value, X_WIFI_STA_PASSWORD_ITEM->text.value)) {
		X_WIFI_STA_PROPERTY->state = INDIGO_ALERT_STATE;
		update_with_refusal(device, X_WIFI_STA_PROPERTY);
		return;
	}
	//- aux.X_WIFI_STA.on_change
	indigo_update_property(device, X_WIFI_STA_PROPERTY, NULL);
}

static void aux_x_leds_handler(indigo_device *device) {
	X_LEDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_LEDS.on_change
	bool result = false;
	if (X_LEDS_OFF_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_OFF);
	} else if (X_LEDS_DIM_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_LOW);
	} else if (X_LEDS_MIDDLE_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_MIDDLE);
	} else if (X_LEDS_ON_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_ON);
	}
	if (result) {
		PRIVATE_DATA->leds_item = selected_item(X_LEDS_PROPERTY);
	} else {
		restore_switch(X_LEDS_PROPERTY, PRIVATE_DATA->leds_item);
	}
	//- aux.X_LEDS.on_change
	indigo_update_property(device, X_LEDS_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity", 0, 100, 1, 0);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
		X_STATE_PROPERTY = indigo_init_number_property(NULL, device->name, X_STATE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "State", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_STATE_TEMPERATURE_ITEM, X_STATE_TEMPERATURE_ITEM_NAME, "Temperature (°C)", -50, 100, 0, 0);
		indigo_init_number_item(X_STATE_VIN_12V_ITEM, X_STATE_VIN_12V_ITEM_NAME, "12V power (V)", 0, 50, 0, 0);
		indigo_init_number_item(X_STATE_VIN_USB_ITEM, X_STATE_VIN_USB_ITEM_NAME, "USB power (V)", 0, 10, 0, 0);
		X_PRESETS_PROPERTY = indigo_init_number_property(NULL, device->name, X_PRESETS_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Preset brightness", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (X_PRESETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_PRESETS_1_ITEM, X_PRESETS_1_ITEM_NAME, "Preset #1", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_2_ITEM, X_PRESETS_2_ITEM_NAME, "Preset #2", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_3_ITEM, X_PRESETS_3_ITEM_NAME, "Preset #3", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_4_ITEM, X_PRESETS_4_ITEM_NAME, "Preset #4", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_5_ITEM, X_PRESETS_5_ITEM_NAME, "Preset #5", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_6_ITEM, X_PRESETS_6_ITEM_NAME, "Preset #6", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_7_ITEM, X_PRESETS_7_ITEM_NAME, "Preset #7", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_8_ITEM, X_PRESETS_8_ITEM_NAME, "Preset #8", 0, 100, 1, 0);
		indigo_init_number_item(X_PRESETS_9_ITEM, X_PRESETS_9_ITEM_NAME, "Preset #9", 0, 100, 1, 0);
		X_PRESET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, X_PRESET_NAMES_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Preset names", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (X_PRESET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_PRESET_NAMES_1_ITEM, X_PRESET_NAMES_1_ITEM_NAME, "Preset #1", "");
		indigo_init_text_item(X_PRESET_NAMES_2_ITEM, X_PRESET_NAMES_2_ITEM_NAME, "Preset #2", "");
		indigo_init_text_item(X_PRESET_NAMES_3_ITEM, X_PRESET_NAMES_3_ITEM_NAME, "Preset #3", "");
		indigo_init_text_item(X_PRESET_NAMES_4_ITEM, X_PRESET_NAMES_4_ITEM_NAME, "Preset #4", "");
		indigo_init_text_item(X_PRESET_NAMES_5_ITEM, X_PRESET_NAMES_5_ITEM_NAME, "Preset #5", "");
		indigo_init_text_item(X_PRESET_NAMES_6_ITEM, X_PRESET_NAMES_6_ITEM_NAME, "Preset #6", "");
		indigo_init_text_item(X_PRESET_NAMES_7_ITEM, X_PRESET_NAMES_7_ITEM_NAME, "Preset #7", "");
		indigo_init_text_item(X_PRESET_NAMES_8_ITEM, X_PRESET_NAMES_8_ITEM_NAME, "Preset #8", "");
		indigo_init_text_item(X_PRESET_NAMES_9_ITEM, X_PRESET_NAMES_9_ITEM_NAME, "Preset #9", "");
		X_PRESET_RECALL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PRESET_RECALL_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Recall preset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 9);
		if (X_PRESET_RECALL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_PRESET_RECALL_1_ITEM, X_PRESET_RECALL_1_ITEM_NAME, "Preset #1", false);
		indigo_init_switch_item(X_PRESET_RECALL_2_ITEM, X_PRESET_RECALL_2_ITEM_NAME, "Preset #2", false);
		indigo_init_switch_item(X_PRESET_RECALL_3_ITEM, X_PRESET_RECALL_3_ITEM_NAME, "Preset #3", false);
		indigo_init_switch_item(X_PRESET_RECALL_4_ITEM, X_PRESET_RECALL_4_ITEM_NAME, "Preset #4", false);
		indigo_init_switch_item(X_PRESET_RECALL_5_ITEM, X_PRESET_RECALL_5_ITEM_NAME, "Preset #5", false);
		indigo_init_switch_item(X_PRESET_RECALL_6_ITEM, X_PRESET_RECALL_6_ITEM_NAME, "Preset #6", false);
		indigo_init_switch_item(X_PRESET_RECALL_7_ITEM, X_PRESET_RECALL_7_ITEM_NAME, "Preset #7", false);
		indigo_init_switch_item(X_PRESET_RECALL_8_ITEM, X_PRESET_RECALL_8_ITEM_NAME, "Preset #8", false);
		indigo_init_switch_item(X_PRESET_RECALL_9_ITEM, X_PRESET_RECALL_9_ITEM_NAME, "Preset #9", false);
		X_FADING_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FADING_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Brightness fading", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FADING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FADING_ON_ITEM, X_FADING_ON_ITEM_NAME, "Enabled", true);
		indigo_init_switch_item(X_FADING_OFF_ITEM, X_FADING_OFF_ITEM_NAME, "Disabled", false);
		X_POWER_ON_LIGHT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_POWER_ON_LIGHT_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Light at power on", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_POWER_ON_LIGHT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_POWER_ON_LIGHT_ON_ITEM, X_POWER_ON_LIGHT_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(X_POWER_ON_LIGHT_OFF_ITEM, X_POWER_ON_LIGHT_OFF_ITEM_NAME, "Off", true);
		X_WIFI_PROPERTY = indigo_init_switch_property(NULL, device->name, X_WIFI_PROPERTY_NAME, AUX_ADVANCED_GROUP, "WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_WIFI_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_WIFI_OFF_ITEM, X_WIFI_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_WIFI_AP_ITEM, X_WIFI_AP_ITEM_NAME, "Access Point mode", false);
		indigo_init_switch_item(X_WIFI_STA_ITEM, X_WIFI_STA_ITEM_NAME, "Station mode", false);
		X_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, device->name, X_WIFI_AP_PROPERTY_NAME, AUX_ADVANCED_GROUP, "AP WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_WIFI_AP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_WIFI_AP_SSID_ITEM, X_WIFI_AP_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(X_WIFI_AP_PASSWORD_ITEM, X_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		X_WIFI_STA_PROPERTY = indigo_init_text_property(NULL, device->name, X_WIFI_STA_PROPERTY_NAME, AUX_ADVANCED_GROUP, "STA WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_WIFI_STA_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_WIFI_STA_SSID_ITEM, X_WIFI_STA_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(X_WIFI_STA_PASSWORD_ITEM, X_WIFI_STA_PASSWORD_ITEM_NAME, "Password", "");
		X_LEDS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_LEDS_PROPERTY_NAME, AUX_ADVANCED_GROUP, "LEDs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_LEDS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_LEDS_OFF_ITEM, X_LEDS_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(X_LEDS_DIM_ITEM, X_LEDS_DIM_ITEM_NAME, "Dim", false);
		indigo_init_switch_item(X_LEDS_MIDDLE_ITEM, X_LEDS_MIDDLE_ITEM_NAME, "Middle", false);
		indigo_init_switch_item(X_LEDS_ON_ITEM, X_LEDS_ON_ITEM_NAME, "On", true);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_SWITCH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_INTENSITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESET_NAMES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESET_RECALL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FADING_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_POWER_ON_LIGHT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_AP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_STA_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_LEDS_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_SWITCH_PROPERTY, aux_light_switch_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_INTENSITY_PROPERTY, aux_light_intensity_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESETS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESETS_PROPERTY, aux_x_presets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESET_NAMES_PROPERTY, aux_x_preset_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESET_RECALL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESET_RECALL_PROPERTY, aux_x_preset_recall_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FADING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FADING_PROPERTY, aux_x_fading_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_POWER_ON_LIGHT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_POWER_ON_LIGHT_PROPERTY, aux_x_power_on_light_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WIFI_PROPERTY, aux_x_wifi_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_AP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WIFI_AP_PROPERTY, aux_x_wifi_ap_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_STA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WIFI_STA_PROPERTY, aux_x_wifi_sta_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_LEDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_LEDS_PROPERTY, aux_x_leds_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	indigo_release_property(X_STATE_PROPERTY);
	indigo_release_property(X_PRESETS_PROPERTY);
	indigo_release_property(X_PRESET_NAMES_PROPERTY);
	indigo_release_property(X_PRESET_RECALL_PROPERTY);
	indigo_release_property(X_FADING_PROPERTY);
	indigo_release_property(X_POWER_ON_LIGHT_PROPERTY);
	indigo_release_property(X_WIFI_PROPERTY);
	indigo_release_property(X_WIFI_AP_PROPERTY);
	indigo_release_property(X_WIFI_STA_PROPERTY);
	indigo_release_property(X_LEDS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_giotto(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static giotto_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "CP2102N");
			patterns[0].vendor_id = 0x10c4;
			patterns[0].product_id = 0xea60;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = (giotto_private_data *)indigo_safe_malloc(sizeof(giotto_private_data));
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
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
