// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_aux_skyalert.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_skyalert.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_aux_skyalert"
#define DRIVER_LABEL         "Interactive Astronomy SkyAlert"
#define AUX_DEVICE_NAME      "Interactive Astronomy SkyAlert"
#define PRIVATE_DATA         ((skyalert_private_data *)device->private_data)

//+ define

// Indexes of the numeric record fields, in the order the device sends them.
#define FIELD_TEMPERATURE    0
#define FIELD_SKY_TEMPERATURE 1
#define FIELD_RAIN           2
#define FIELD_SKY_BRIGHTNESS 3
#define FIELD_HUMIDITY       4
#define FIELD_WIND_SPEED     5
#define FIELD_POWER          6
#define FIELD_PRESSURE       7
#define FIELD_COUNT          8

//- define

#pragma mark - Property definitions

#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_SKY_BRIGHTNESS_ITEM   (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_POWER_ITEM            (AUX_INFO_PROPERTY->items + 1)

#define AUX_WEATHER_PROPERTY             (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM     (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM        (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_PRESSURE_ITEM        (AUX_WEATHER_PROPERTY->items + 2)
#define AUX_WEATHER_WIND_SPEED_ITEM      (AUX_WEATHER_PROPERTY->items + 3)
#define AUX_WEATHER_RAIN_ITEM            (AUX_WEATHER_PROPERTY->items + 4)
#define AUX_WEATHER_SKY_TEMPERATURE_ITEM (AUX_WEATHER_PROPERTY->items + 5)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_info_property;
	indigo_property *aux_weather_property;
	//+ data
	char response[32];
	bool weather_invalid;
	bool info_invalid;
	bool record_valid;
	bool field_valid[FIELD_COUNT];
	bool classified;
	bool cloud_seen;
	bool rain_seen;
	bool sky_seen;
	bool humidity_seen;
	bool wind_seen;
	//- data
} skyalert_private_data;

#pragma mark - Low level code

//+ code

static bool skyalert_read_value(indigo_device *device, double *value) {
	return indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r", INDIGO_DELAY(1)) > 0 && sscanf(PRIVATE_DATA->response, "%lf", value) == 1 && isfinite(*value);
}

// The record has to arrive complete, starting with "Data". A field that is not a finite number
// (an Arduino prints a failed sensor reading as "nan") keeps the item's last value and only puts
// the property it belongs to in ALERT, it does not discard the other readings.
static bool skyalert_read_record(indigo_device *device) {
	PRIVATE_DATA->record_valid = false;
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_printf(PRIVATE_DATA->handle, "send\r") <= 0 || indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r", INDIGO_DELAY(1)) <= 0 || strcmp(PRIVATE_DATA->response, "Data")) {
		return false;
	}
	double values[FIELD_COUNT];
	bool *valid = PRIVATE_DATA->field_valid;
	char firmware[sizeof(PRIVATE_DATA->response)];
	for (int i = 0; i < 9; i++) {
		char *line = i == 7 ? firmware : PRIVATE_DATA->response;
		if (indigo_uni_read_section(PRIVATE_DATA->handle, line, sizeof(PRIVATE_DATA->response) - 1, "\r", "\r", INDIGO_DELAY(1)) <= 0) {
			return false;
		}
		if (i != 7) {
			int j = i < 7 ? i : 7;
			valid[j] = sscanf(line, "%lf", values + j) == 1 && isfinite(values[j]);
		}
	}
	indigo_item *items[FIELD_COUNT] = { AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM, AUX_WEATHER_RAIN_ITEM, AUX_INFO_SKY_BRIGHTNESS_ITEM, AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_WIND_SPEED_ITEM, AUX_INFO_POWER_ITEM, AUX_WEATHER_PRESSURE_ITEM };
	PRIVATE_DATA->weather_invalid = PRIVATE_DATA->info_invalid = false;
	for (int i = 0; i < FIELD_COUNT; i++) {
		if (valid[i]) {
			// The pressure is reported in pascal.
			items[i]->number.value = i == FIELD_PRESSURE ? values[i] / 100 : values[i];
		} else if (i == FIELD_SKY_BRIGHTNESS || i == FIELD_POWER) {
			PRIVATE_DATA->info_invalid = true;
		} else {
			PRIVATE_DATA->weather_invalid = true;
		}
	}
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
	PRIVATE_DATA->record_valid = true;
	return true;
}

// No protocol document of the SkyAlert is published. The record carries the raw sensor values: the
// cloud measure is the difference between the IR sky temperature and the ambient temperature, which the
// SkyAlert software uses for its cloud condition; the rain (dampness), wind and sky brightness fields are raw 10-bit
// ADC counts (the disabled wind conversion of the 2.0 driver assumes 5 V / 1024 per count). A failed or missing sensor
// is printed as "nan", so a sensor counts as fitted only once it has given a valid reading during the current
// connection: until then its condition is IDLE and the verdict ignores it, afterwards a reading that is not a
// finite number, or a record that cannot be read, is a failure and its condition is ALERT. The cloud measure
// needs valid readings of both the sky and the ambient temperature.
static void skyalert_classify(indigo_device *device) {
	bool *valid = PRIVATE_DATA->field_valid;
	bool record = PRIVATE_DATA->record_valid;
	bool cloud_valid = record && valid[FIELD_TEMPERATURE] && valid[FIELD_SKY_TEMPERATURE];
	bool cloud_seen = PRIVATE_DATA->cloud_seen = PRIVATE_DATA->cloud_seen || cloud_valid;
	bool rain_valid = record && valid[FIELD_RAIN];
	bool rain_seen = PRIVATE_DATA->rain_seen = PRIVATE_DATA->rain_seen || rain_valid;
	bool sky_valid = record && valid[FIELD_SKY_BRIGHTNESS];
	bool sky_seen = PRIVATE_DATA->sky_seen = PRIVATE_DATA->sky_seen || sky_valid;
	bool humidity_valid = record && valid[FIELD_HUMIDITY];
	bool humidity_seen = PRIVATE_DATA->humidity_seen = PRIVATE_DATA->humidity_seen || humidity_valid;
	bool wind_valid = record && valid[FIELD_WIND_SPEED];
	bool wind_seen = PRIVATE_DATA->wind_seen = PRIVATE_DATA->wind_seen || wind_valid;
	double cloud = AUX_WEATHER_SKY_TEMPERATURE_ITEM->number.value - AUX_WEATHER_TEMPERATURE_ITEM->number.value;
	double rain = AUX_WEATHER_RAIN_ITEM->number.value;
	double sky = AUX_INFO_SKY_BRIGHTNESS_ITEM->number.value;
	double humidity = AUX_WEATHER_HUMIDITY_ITEM->number.value;
	double wind = AUX_WEATHER_WIND_SPEED_ITEM->number.value;
	if (!cloud_valid) {
		indigo_aux_set_condition(AUX_CLOUD_PROPERTY, cloud_seen ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE, NULL);
	} else if (cloud <= AUX_CLOUD_CLEAR_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_CLOUD_PROPERTY, INDIGO_OK_STATE, AUX_CLOUD_CLEAR_ITEM);
	} else if (cloud <= AUX_CLOUD_CLOUDY_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_CLOUD_PROPERTY, INDIGO_OK_STATE, AUX_CLOUD_CLOUDY_ITEM);
	} else {
		indigo_aux_set_condition(AUX_CLOUD_PROPERTY, INDIGO_OK_STATE, AUX_CLOUD_OVERCAST_ITEM);
	}
	// A wetter sensor gives a lower reading.
	if (!rain_valid) {
		indigo_aux_set_condition(AUX_RAIN_PROPERTY, rain_seen ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE, NULL);
	} else if (rain <= AUX_RAIN_RAINING_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_RAIN_PROPERTY, INDIGO_OK_STATE, AUX_RAIN_RAINING_ITEM);
	} else if (rain <= AUX_RAIN_WET_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_RAIN_PROPERTY, INDIGO_OK_STATE, AUX_RAIN_WET_ITEM);
	} else {
		indigo_aux_set_condition(AUX_RAIN_PROPERTY, INDIGO_OK_STATE, AUX_RAIN_DRY_ITEM);
	}
	// A brighter sky gives a higher reading.
	if (!sky_valid) {
		indigo_aux_set_condition(AUX_SKY_PROPERTY, sky_seen ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE, NULL);
	} else if (sky <= AUX_SKY_DARK_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_SKY_PROPERTY, INDIGO_OK_STATE, AUX_SKY_DARK_ITEM);
	} else if (sky <= AUX_SKY_LIGHT_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_SKY_PROPERTY, INDIGO_OK_STATE, AUX_SKY_LIGHT_ITEM);
	} else {
		indigo_aux_set_condition(AUX_SKY_PROPERTY, INDIGO_OK_STATE, AUX_SKY_VERY_LIGHT_ITEM);
	}
	if (!humidity_valid) {
		indigo_aux_set_condition(AUX_HUMIDITY_PROPERTY, humidity_seen ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE, NULL);
	} else if (humidity >= AUX_HUMIDITY_HUMID_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_HUMIDITY_PROPERTY, INDIGO_OK_STATE, AUX_HUMIDITY_HUMID_ITEM);
	} else if (humidity >= AUX_HUMIDITY_NORMAL_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_HUMIDITY_PROPERTY, INDIGO_OK_STATE, AUX_HUMIDITY_NORMAL_ITEM);
	} else {
		indigo_aux_set_condition(AUX_HUMIDITY_PROPERTY, INDIGO_OK_STATE, AUX_HUMIDITY_DRY_ITEM);
	}
	if (!wind_valid) {
		indigo_aux_set_condition(AUX_WIND_PROPERTY, wind_seen ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE, NULL);
	} else if (wind >= AUX_WIND_STRONG_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_WIND_PROPERTY, INDIGO_OK_STATE, AUX_WIND_STRONG_ITEM);
	} else if (wind >= AUX_WIND_MODERATE_THRESHOLD_ITEM->number.value) {
		indigo_aux_set_condition(AUX_WIND_PROPERTY, INDIGO_OK_STATE, AUX_WIND_MODERATE_ITEM);
	} else {
		indigo_aux_set_condition(AUX_WIND_PROPERTY, INDIGO_OK_STATE, AUX_WIND_CALM_ITEM);
	}
	indigo_property *conditions[] = { AUX_CLOUD_PROPERTY, AUX_RAIN_PROPERTY, AUX_SKY_PROPERTY, AUX_HUMIDITY_PROPERTY, AUX_WIND_PROPERTY };
	for (int i = 0; i < (int)(sizeof(conditions) / sizeof(conditions[0])); i++) {
		indigo_update_property(device, conditions[i], NULL);
	}
	// The raw rain and wind readings mean nothing to the user, so their reasons have no numbers.
	indigo_aux_safety_input safety[INDIGO_AUX_SAFETY_CONDITION_COUNT] = {
		{ cloud_seen, cloud_valid, AUX_CLOUD_OVERCAST_ITEM->light.value == INDIGO_OK_STATE, false, cloud, AUX_CLOUD_CLOUDY_THRESHOLD_ITEM->number.value, "C" },
		{ rain_seen, rain_valid, AUX_RAIN_RAINING_ITEM->light.value == INDIGO_OK_STATE, true, rain, AUX_RAIN_RAINING_THRESHOLD_ITEM->number.value, NULL, true },
		{ wind_seen, wind_valid, AUX_WIND_STRONG_ITEM->light.value == INDIGO_OK_STATE, false, wind, AUX_WIND_STRONG_THRESHOLD_ITEM->number.value, NULL, true },
		{ humidity_seen, humidity_valid, AUX_HUMIDITY_HUMID_ITEM->light.value == INDIGO_OK_STATE, false, humidity, AUX_HUMIDITY_HUMID_THRESHOLD_ITEM->number.value, "%" }
	};
	indigo_aux_update_weather_safety(device, AUX_WEATHER_SAFETY_PROPERTY, safety);
	PRIVATE_DATA->classified = true;
}

// A threshold changed while connected reclassifies the last reading at once.
static void skyalert_reclassify(indigo_device *device) {
	if (IS_CONNECTED && PRIVATE_DATA->classified) {
		skyalert_classify(device);
	}
}

static bool skyalert_open(indigo_device *device, const char **error_message) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed2(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG, error_message);
	if (PRIVATE_DATA->handle) {
		if (skyalert_read_record(device)) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Interactive Astronomy SkyAlert");
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void skyalert_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
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
	// The device has no unsolicited output: every reading is one "send" request. The 2.0
	// driver polled it every 10 seconds; the generated 3.0 driver read the record only in
	// skyalert_open(), so the published weather never changed after connection.
	if (skyalert_read_record(device)) {
		AUX_WEATHER_PROPERTY->state = PRIVATE_DATA->weather_invalid ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		AUX_INFO_PROPERTY->state = PRIVATE_DATA->info_invalid ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	} else {
		AUX_WEATHER_PROPERTY->state = AUX_INFO_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	skyalert_classify(device);
	indigo_execute_handler_in(device, 10, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		connection_result = skyalert_open(device, &error_message);
		if (connection_result) {
			//+ aux.on_connect
			// No condition is known before the first poll, which the connection handler queues at once. The sensors
			// seen with a valid reading in this connection start with the record read by skyalert_open().
			PRIVATE_DATA->classified = false;
			bool *valid = PRIVATE_DATA->field_valid;
			PRIVATE_DATA->cloud_seen = valid[FIELD_TEMPERATURE] && valid[FIELD_SKY_TEMPERATURE];
			PRIVATE_DATA->rain_seen = valid[FIELD_RAIN];
			PRIVATE_DATA->sky_seen = valid[FIELD_SKY_BRIGHTNESS];
			PRIVATE_DATA->humidity_seen = valid[FIELD_HUMIDITY];
			PRIVATE_DATA->wind_seen = valid[FIELD_WIND_SPEED];
			indigo_property *conditions[] = { AUX_HUMIDITY_PROPERTY, AUX_WIND_PROPERTY, AUX_RAIN_PROPERTY, AUX_CLOUD_PROPERTY, AUX_SKY_PROPERTY };
			for (int i = 0; i < (int)(sizeof(conditions) / sizeof(conditions[0])); i++) {
				indigo_aux_set_condition(conditions[i], INDIGO_BUSY_STATE, NULL);
			}
			AUX_WEATHER_SAFETY_PROPERTY->state = INDIGO_BUSY_STATE;
			for (int i = 0; i < AUX_WEATHER_SAFETY_PROPERTY->count; i++) {
				AUX_WEATHER_SAFETY_PROPERTY->items[i].light.value = INDIGO_IDLE_STATE;
			}
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
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
		// The thresholds stay defined, so a change cancelled by the disconnect must not leave them BUSY for clients.
		indigo_property *thresholds[] = { AUX_HUMIDITY_THRESHOLDS_PROPERTY, AUX_WIND_THRESHOLDS_PROPERTY, AUX_RAIN_THRESHOLDS_PROPERTY, AUX_CLOUD_THRESHOLDS_PROPERTY, AUX_SKY_THRESHOLDS_PROPERTY };
		for (int i = 0; i < (int)(sizeof(thresholds) / sizeof(thresholds[0])); i++) {
			if (thresholds[i]->state == INDIGO_BUSY_STATE) {
				INDIGO_UPDATE_PROPERTY_STATE(thresholds[i], INDIGO_OK_STATE, NULL);
			}
		}
		//- aux.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			AUX_INFO_PROPERTY,
			AUX_WEATHER_PROPERTY,
			AUX_HUMIDITY_PROPERTY,
			AUX_WIND_PROPERTY,
			AUX_RAIN_PROPERTY,
			AUX_CLOUD_PROPERTY,
			AUX_SKY_PROPERTY,
			AUX_WEATHER_SAFETY_PROPERTY,
			AUX_HUMIDITY_THRESHOLDS_PROPERTY,
			AUX_WIND_THRESHOLDS_PROPERTY,
			AUX_RAIN_THRESHOLDS_PROPERTY,
			AUX_CLOUD_THRESHOLDS_PROPERTY,
			AUX_SKY_THRESHOLDS_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		skyalert_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_timer_callback);
	}
}

static void aux_humidity_thresholds_handler(indigo_device *device) {
	AUX_HUMIDITY_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HUMIDITY_THRESHOLDS.on_change
	skyalert_reclassify(device);
	//- aux.AUX_HUMIDITY_THRESHOLDS.on_change
	indigo_update_property(device, AUX_HUMIDITY_THRESHOLDS_PROPERTY, NULL);
}

static void aux_wind_thresholds_handler(indigo_device *device) {
	AUX_WIND_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_WIND_THRESHOLDS.on_change
	skyalert_reclassify(device);
	//- aux.AUX_WIND_THRESHOLDS.on_change
	indigo_update_property(device, AUX_WIND_THRESHOLDS_PROPERTY, NULL);
}

static void aux_rain_thresholds_handler(indigo_device *device) {
	AUX_RAIN_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_RAIN_THRESHOLDS.on_change
	skyalert_reclassify(device);
	//- aux.AUX_RAIN_THRESHOLDS.on_change
	indigo_update_property(device, AUX_RAIN_THRESHOLDS_PROPERTY, NULL);
}

static void aux_cloud_thresholds_handler(indigo_device *device) {
	AUX_CLOUD_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_CLOUD_THRESHOLDS.on_change
	skyalert_reclassify(device);
	//- aux.AUX_CLOUD_THRESHOLDS.on_change
	indigo_update_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY, NULL);
}

static void aux_sky_thresholds_handler(indigo_device *device) {
	AUX_SKY_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SKY_THRESHOLDS.on_change
	skyalert_reclassify(device);
	//- aux.AUX_SKY_THRESHOLDS.on_change
	indigo_update_property(device, AUX_SKY_THRESHOLDS_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Info", "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_SKY_BRIGHTNESS_ITEM, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness [raw]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_POWER_ITEM, AUX_INFO_POWER_ITEM_NAME, "Power [1 = ok, 0 = failure]", 0, 0, 0, 0);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Info", "Weather conditions", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [hPa]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_WIND_SPEED_ITEM, AUX_WEATHER_WIND_SPEED_ITEM_NAME, "Wind speed [raw]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_RAIN_ITEM, AUX_WEATHER_RAIN_ITEM_NAME, "Dampness [raw]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sky temperature [\u00B0C]", 0, 0, 0, 0);
		AUX_HUMIDITY_PROPERTY->hidden = false;
		AUX_WIND_PROPERTY->hidden = false;
		AUX_RAIN_PROPERTY->hidden = false;
		AUX_CLOUD_PROPERTY->hidden = false;
		AUX_SKY_PROPERTY->hidden = false;
		AUX_WEATHER_SAFETY_PROPERTY->hidden = false;
		AUX_HUMIDITY_THRESHOLDS_PROPERTY->hidden = false;
		//+ aux.AUX_HUMIDITY_THRESHOLDS.on_attach
		INDIGO_COPY_VALUE(AUX_HUMIDITY_THRESHOLDS_PROPERTY->label, "Humidity thresholds [%]");
		indigo_init_number_item(AUX_HUMIDITY_HUMID_THRESHOLD_ITEM, AUX_HUMIDITY_HUMID_ITEM_NAME, "Humid (at least)", 0, 100, 0, 90);
		indigo_init_number_item(AUX_HUMIDITY_NORMAL_THRESHOLD_ITEM, AUX_HUMIDITY_NORMAL_ITEM_NAME, "Normal (at least)", 0, 100, 0, 70);
		//- aux.AUX_HUMIDITY_THRESHOLDS.on_attach
		AUX_WIND_THRESHOLDS_PROPERTY->hidden = false;
		//+ aux.AUX_WIND_THRESHOLDS.on_attach
		// The wind reading is a raw ADC count and its conversion depends on the sensor; the defaults are
		// beyond the 10-bit range, so the wind is CALM and safe until the user sets thresholds for the unit.
		INDIGO_COPY_VALUE(AUX_WIND_THRESHOLDS_PROPERTY->label, "Wind thresholds [raw]");
		indigo_init_number_item(AUX_WIND_STRONG_THRESHOLD_ITEM, AUX_WIND_STRONG_ITEM_NAME, "Strong (at least)", 0, 1024, 0, 1024);
		indigo_init_number_item(AUX_WIND_MODERATE_THRESHOLD_ITEM, AUX_WIND_MODERATE_ITEM_NAME, "Moderate (at least)", 0, 1024, 0, 1024);
		//- aux.AUX_WIND_THRESHOLDS.on_attach
		AUX_RAIN_THRESHOLDS_PROPERTY->hidden = false;
		//+ aux.AUX_RAIN_THRESHOLDS.on_attach
		INDIGO_COPY_VALUE(AUX_RAIN_THRESHOLDS_PROPERTY->label, "Dampness thresholds [raw]");
		indigo_init_number_item(AUX_RAIN_RAINING_THRESHOLD_ITEM, AUX_RAIN_RAINING_ITEM_NAME, "Raining (at most)", 0, 1024, 0, 800);
		indigo_init_number_item(AUX_RAIN_WET_THRESHOLD_ITEM, AUX_RAIN_WET_ITEM_NAME, "Wet (at most)", 0, 1024, 0, 950);
		//- aux.AUX_RAIN_THRESHOLDS.on_attach
		AUX_CLOUD_THRESHOLDS_PROPERTY->hidden = false;
		//+ aux.AUX_CLOUD_THRESHOLDS.on_attach
		INDIGO_COPY_VALUE(AUX_CLOUD_THRESHOLDS_PROPERTY->label, "Cloud thresholds (sky - ambient temperature) [C]");
		indigo_init_number_item(AUX_CLOUD_CLEAR_THRESHOLD_ITEM, AUX_CLOUD_CLEAR_ITEM_NAME, "Clear (at most)", -200, 200, 0, -20);
		indigo_init_number_item(AUX_CLOUD_CLOUDY_THRESHOLD_ITEM, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy (at most)", -200, 200, 0, -10);
		//- aux.AUX_CLOUD_THRESHOLDS.on_attach
		AUX_SKY_THRESHOLDS_PROPERTY->hidden = false;
		//+ aux.AUX_SKY_THRESHOLDS.on_attach
		INDIGO_COPY_VALUE(AUX_SKY_THRESHOLDS_PROPERTY->label, "Sky brightness thresholds [raw]");
		indigo_init_number_item(AUX_SKY_DARK_THRESHOLD_ITEM, AUX_SKY_DARK_ITEM_NAME, "Dark (at most)", 0, 1024, 0, 250);
		indigo_init_number_item(AUX_SKY_LIGHT_THRESHOLD_ITEM, AUX_SKY_LIGHT_ITEM_NAME, "Light (at most)", 0, 1024, 0, 750);
		//- aux.AUX_SKY_THRESHOLDS.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HUMIDITY_THRESHOLDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HUMIDITY_THRESHOLDS_PROPERTY, aux_humidity_thresholds_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_WIND_THRESHOLDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_WIND_THRESHOLDS_PROPERTY, aux_wind_thresholds_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_RAIN_THRESHOLDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_RAIN_THRESHOLDS_PROPERTY, aux_rain_thresholds_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_CLOUD_THRESHOLDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_CLOUD_THRESHOLDS_PROPERTY, aux_cloud_thresholds_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SKY_THRESHOLDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SKY_THRESHOLDS_PROPERTY, aux_sky_thresholds_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_skyalert(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static skyalert_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (skyalert_private_data *)indigo_safe_malloc(sizeof(skyalert_private_data));
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
