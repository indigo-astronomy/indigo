// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

/** INDIGO AUX Driver base
 \file indigo_aux_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <indigo/indigo_aux_driver.h>

static indigo_property *aux_init_condition_property(indigo_device *device, const char *name, const char *label, const char *name1, const char *label1, const char *name2, const char *label2, const char *name3, const char *label3) {
	indigo_property *property = indigo_init_light_property(NULL, device->name, name, AUX_WEATHER_GROUP, label, INDIGO_IDLE_STATE, 3);
	if (property != NULL) {
		property->hidden = true;
		indigo_init_light_item(property->items + 0, name1, label1, INDIGO_IDLE_STATE);
		indigo_init_light_item(property->items + 1, name2, label2, INDIGO_IDLE_STATE);
		indigo_init_light_item(property->items + 2, name3, label3, INDIGO_IDLE_STATE);
	}
	return property;
}

static indigo_property *aux_init_thresholds_property(indigo_device *device, const char *name, const char *label, const char *name1, const char *label1, double value1, const char *name2, const char *label2, double value2, double min, double max) {
	indigo_property *property = indigo_init_number_property(NULL, device->name, name, AUX_THRESHOLDS_GROUP, label, INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (property != NULL) {
		property->hidden = true;
		indigo_init_number_item(property->items + 0, name1, label1, min, max, 0, value1);
		indigo_init_number_item(property->items + 1, name2, label2, min, max, 0, value2);
	}
	return property;
}

indigo_result indigo_aux_attach(indigo_device *device, const char* driver_name, unsigned version, int interface) {
	assert(device != NULL);
	if (AUX_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_aux_context));
	}
	if (AUX_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, interface) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- AUX_HUMIDITY
			AUX_HUMIDITY_PROPERTY = aux_init_condition_property(device, AUX_HUMIDITY_PROPERTY_NAME, "Humidity condition", AUX_HUMIDITY_HUMID_ITEM_NAME, "Humid", AUX_HUMIDITY_NORMAL_ITEM_NAME, "Normal", AUX_HUMIDITY_DRY_ITEM_NAME, "Dry");
			if (AUX_HUMIDITY_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_WIND
			AUX_WIND_PROPERTY = aux_init_condition_property(device, AUX_WIND_PROPERTY_NAME, "Wind condition", AUX_WIND_STRONG_ITEM_NAME, "Strong", AUX_WIND_MODERATE_ITEM_NAME, "Moderate", AUX_WIND_CALM_ITEM_NAME, "Calm");
			if (AUX_WIND_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_RAIN
			AUX_RAIN_PROPERTY = aux_init_condition_property(device, AUX_RAIN_PROPERTY_NAME, "Rain condition", AUX_RAIN_RAINING_ITEM_NAME, "Raining", AUX_RAIN_WET_ITEM_NAME, "Wet", AUX_RAIN_DRY_ITEM_NAME, "Dry");
			if (AUX_RAIN_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_CLOUD
			AUX_CLOUD_PROPERTY = aux_init_condition_property(device, AUX_CLOUD_PROPERTY_NAME, "Cloud condition", AUX_CLOUD_CLEAR_ITEM_NAME, "Clear", AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy", AUX_CLOUD_OVERCAST_ITEM_NAME, "Overcast");
			if (AUX_CLOUD_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_SKY
			AUX_SKY_PROPERTY = aux_init_condition_property(device, AUX_SKY_PROPERTY_NAME, "Sky condition", AUX_SKY_DARK_ITEM_NAME, "Dark", AUX_SKY_LIGHT_ITEM_NAME, "Light", AUX_SKY_VERY_LIGHT_ITEM_NAME, "Very light");
			if (AUX_SKY_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_HUMIDITY_THRESHOLDS
			AUX_HUMIDITY_THRESHOLDS_PROPERTY = aux_init_thresholds_property(device, AUX_HUMIDITY_THRESHOLDS_PROPERTY_NAME, "Humidity thresholds (%)", AUX_HUMIDITY_HUMID_ITEM_NAME, "Humid (at least)", 90, AUX_HUMIDITY_NORMAL_ITEM_NAME, "Normal (at least)", 70, 0, 100);
			if (AUX_HUMIDITY_THRESHOLDS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_WIND_THRESHOLDS
			AUX_WIND_THRESHOLDS_PROPERTY = aux_init_thresholds_property(device, AUX_WIND_THRESHOLDS_PROPERTY_NAME, "Wind thresholds (m/s)", AUX_WIND_STRONG_ITEM_NAME, "Strong (at least)", 10, AUX_WIND_MODERATE_ITEM_NAME, "Moderate (at least)", 3, 0, 100);
			if (AUX_WIND_THRESHOLDS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_RAIN_THRESHOLDS
			AUX_RAIN_THRESHOLDS_PROPERTY = aux_init_thresholds_property(device, AUX_RAIN_THRESHOLDS_PROPERTY_NAME, "Rain thresholds", AUX_RAIN_RAINING_ITEM_NAME, "Raining", 1, AUX_RAIN_WET_ITEM_NAME, "Wet", 0.1, 0, 1000);
			if (AUX_RAIN_THRESHOLDS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_CLOUD_THRESHOLDS
			AUX_CLOUD_THRESHOLDS_PROPERTY = aux_init_thresholds_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY_NAME, "Cloud thresholds (sky temperature °C)", AUX_CLOUD_CLEAR_ITEM_NAME, "Clear (below)", -15, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy (below)", 0, -200, 200);
			if (AUX_CLOUD_THRESHOLDS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_SKY_THRESHOLDS
			AUX_SKY_THRESHOLDS_PROPERTY = aux_init_thresholds_property(device, AUX_SKY_THRESHOLDS_PROPERTY_NAME, "Sky darkness thresholds", AUX_SKY_DARK_ITEM_NAME, "Dark", 0, AUX_SKY_LIGHT_ITEM_NAME, "Light", 0, 0, 100000);
			if (AUX_SKY_THRESHOLDS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			// -------------------------------------------------------------------------------- AUX_WEATHER_SAFETY
			AUX_WEATHER_SAFETY_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_WEATHER_SAFETY_PROPERTY_NAME, AUX_WEATHER_GROUP, "Weather safety", INDIGO_OK_STATE, 5);
			if (AUX_WEATHER_SAFETY_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			AUX_WEATHER_SAFETY_PROPERTY->hidden = true;
			indigo_init_light_item(AUX_WEATHER_SAFETY_SAFE_ITEM, AUX_WEATHER_SAFETY_SAFE_ITEM_NAME, "Safe", INDIGO_IDLE_STATE);
			indigo_init_light_item(AUX_WEATHER_SAFETY_CLOUDS_ITEM, AUX_WEATHER_SAFETY_CLOUDS_ITEM_NAME, "Clouds", INDIGO_IDLE_STATE);
			indigo_init_light_item(AUX_WEATHER_SAFETY_RAIN_ITEM, AUX_WEATHER_SAFETY_RAIN_ITEM_NAME, "Rain", INDIGO_IDLE_STATE);
			indigo_init_light_item(AUX_WEATHER_SAFETY_WIND_ITEM, AUX_WEATHER_SAFETY_WIND_ITEM_NAME, "Wind", INDIGO_IDLE_STATE);
			indigo_init_light_item(AUX_WEATHER_SAFETY_HUMIDITY_ITEM, AUX_WEATHER_SAFETY_HUMIDITY_ITEM_NAME, "Humidity", INDIGO_IDLE_STATE);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HUMIDITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WIND_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_RAIN_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_CLOUD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SKY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_SAFETY_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HUMIDITY_THRESHOLDS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WIND_THRESHOLDS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_RAIN_THRESHOLDS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_CLOUD_THRESHOLDS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SKY_THRESHOLDS_PROPERTY);
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	indigo_property *thresholds[] = { AUX_HUMIDITY_THRESHOLDS_PROPERTY, AUX_WIND_THRESHOLDS_PROPERTY, AUX_RAIN_THRESHOLDS_PROPERTY, AUX_CLOUD_THRESHOLDS_PROPERTY, AUX_SKY_THRESHOLDS_PROPERTY };
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property *connected[] = { AUX_HUMIDITY_PROPERTY, AUX_WIND_PROPERTY, AUX_RAIN_PROPERTY, AUX_CLOUD_PROPERTY, AUX_SKY_PROPERTY, AUX_WEATHER_SAFETY_PROPERTY };
		for (int i = 0; i < (int)(sizeof(connected) / sizeof(connected[0])); i++) {
			if (IS_CONNECTED) {
				indigo_define_property(device, connected[i], NULL);
			} else {
				indigo_delete_property(device, connected[i], NULL);
			}
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			for (int i = 0; i < (int)(sizeof(thresholds) / sizeof(thresholds[0])); i++) {
				indigo_save_property(device, NULL, thresholds[i]);
			}
		}
	} else {
		// -------------------------------------------------------------------------------- AUX_*_THRESHOLDS
		for (int i = 0; i < (int)(sizeof(thresholds) / sizeof(thresholds[0])); i++) {
			if (indigo_property_match_changeable(thresholds[i], property)) {
				indigo_property_copy_values(thresholds[i], property, false);
				thresholds[i]->state = INDIGO_OK_STATE;
				indigo_update_property(device, thresholds[i], NULL);
				return INDIGO_OK;
			}
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_aux_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AUX_HUMIDITY_PROPERTY);
	indigo_release_property(AUX_WIND_PROPERTY);
	indigo_release_property(AUX_RAIN_PROPERTY);
	indigo_release_property(AUX_CLOUD_PROPERTY);
	indigo_release_property(AUX_SKY_PROPERTY);
	indigo_release_property(AUX_HUMIDITY_THRESHOLDS_PROPERTY);
	indigo_release_property(AUX_WIND_THRESHOLDS_PROPERTY);
	indigo_release_property(AUX_RAIN_THRESHOLDS_PROPERTY);
	indigo_release_property(AUX_CLOUD_THRESHOLDS_PROPERTY);
	indigo_release_property(AUX_SKY_THRESHOLDS_PROPERTY);
	indigo_release_property(AUX_WEATHER_SAFETY_PROPERTY);
	return indigo_device_detach(device);
}

void indigo_aux_set_condition(indigo_property *property, indigo_property_state state, indigo_item *level) {
	assert(property != NULL);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		item->light.value = state == INDIGO_OK_STATE && item == level ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	}
	property->state = state;
}

double indigo_aux_dewpoint(double temperature, double rh) {
	double ln_rh = log(rh/100.0);
	double a = (17.625 * temperature) / (243.04 + temperature);
	return (243.04 * (ln_rh + a)) / (17.625 - ln_rh - a);
}

float indigo_aux_sky_bortle(double sky_brightness) {
	if (sky_brightness > 21.76) return 1;
	else if (sky_brightness > 21.6) return 2;
	else if (sky_brightness > 21.3) return 3;
	else if (sky_brightness > 20.8) return 4;
	else if (sky_brightness > 20.3) return 4.5;
	else if (sky_brightness > 19.25) return 5;
	else if (sky_brightness > 18.5) return 6;
	else if (sky_brightness > 18.0) return 7;
	else if (sky_brightness > 17.0) return 8;
	else return 9;
}

static const char *weather_safety_item_names[INDIGO_AUX_SAFETY_CONDITION_COUNT] = { AUX_WEATHER_SAFETY_CLOUDS_ITEM_NAME, AUX_WEATHER_SAFETY_RAIN_ITEM_NAME, AUX_WEATHER_SAFETY_WIND_ITEM_NAME, AUX_WEATHER_SAFETY_HUMIDITY_ITEM_NAME };

static const char *weather_safety_labels[INDIGO_AUX_SAFETY_CONDITION_COUNT] = { "clouds", "rain", "wind", "humidity" };

static void weather_safety_append(char *message, size_t message_size, const char *format, ...) {
	size_t length = strlen(message);
	if (length + 1 < message_size) {
		va_list args;
		va_start(args, format);
		vsnprintf(message + length, message_size - length, format, args);
		va_end(args);
	}
}

bool indigo_aux_evaluate_weather_safety(indigo_property *property, const indigo_aux_safety_input *conditions, char *message, size_t message_size) {
	assert(property != NULL);
	assert(conditions != NULL);
	indigo_item *safe_item = indigo_get_item(property, AUX_WEATHER_SAFETY_SAFE_ITEM_NAME);
	indigo_property_state previous = safe_item != NULL ? safe_item->light.value : INDIGO_IDLE_STATE;
	indigo_property_state previous_state = property->state;
	indigo_property_state verdict = INDIGO_IDLE_STATE;
	bool readings_valid = true;
	bool reasons_changed = false;
	for (int i = 0; i < INDIGO_AUX_SAFETY_CONDITION_COUNT; i++) {
		const indigo_aux_safety_input *condition = conditions + i;
		indigo_property_state state = INDIGO_IDLE_STATE;
		if (condition->fitted) {
			if (!condition->valid) {
				readings_valid = false;
				state = INDIGO_ALERT_STATE;
			} else {
				state = condition->unsafe ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
			}
		}
		if (state == INDIGO_ALERT_STATE || (state == INDIGO_OK_STATE && verdict == INDIGO_IDLE_STATE)) {
			verdict = state;
		}
		indigo_item *item = indigo_get_item(property, weather_safety_item_names[i]);
		if (item != NULL) {
			reasons_changed = reasons_changed || (item->light.value == INDIGO_ALERT_STATE) != (state == INDIGO_ALERT_STATE);
			item->light.value = state;
		}
	}
	if (safe_item != NULL) {
		safe_item->light.value = verdict;
	}
	property->state = readings_valid ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	// The previous lights and property state hold the previous reasons: the ALERT conditions and whether any reading was invalid.
	reasons_changed = reasons_changed || property->state != previous_state;
	if (verdict == previous && !(verdict == INDIGO_ALERT_STATE && reasons_changed)) {
		return false;
	}
	if (message != NULL && message_size > 0) {
		message[0] = 0;
		if (verdict == INDIGO_ALERT_STATE) {
			weather_safety_append(message, message_size, "Unsafe:");
			bool first = true;
			for (int i = 0; i < INDIGO_AUX_SAFETY_CONDITION_COUNT; i++) {
				const indigo_aux_safety_input *condition = conditions + i;
				if (!condition->fitted || (condition->valid && !condition->unsafe)) {
					continue;
				}
				const char *separator = first ? " " : ", ";
				first = false;
				if (!condition->valid) {
					weather_safety_append(message, message_size, "%s%s sensor gives no valid reading", separator, weather_safety_labels[i]);
				} else if (condition->omit_values) {
					weather_safety_append(message, message_size, "%s%s", separator, weather_safety_labels[i]);
				} else {
					const char *unit = condition->unit != NULL && *condition->unit ? condition->unit : NULL;
					const char *relation = condition->value == condition->limit ? "at" : (condition->reversed ? "below" : "above");
					if (unit != NULL) {
						weather_safety_append(message, message_size, "%s%s %.1f %s %s %.1f %s", separator, weather_safety_labels[i], condition->value, unit, relation, condition->limit, unit);
					} else {
						weather_safety_append(message, message_size, "%s%s %.1f %s %.1f", separator, weather_safety_labels[i], condition->value, relation, condition->limit);
					}
				}
			}
		} else if (verdict == INDIGO_OK_STATE) {
			weather_safety_append(message, message_size, "Safe: all conditions within limits");
		} else {
			weather_safety_append(message, message_size, "Unknown: no weather condition is monitored");
		}
	}
	return true;
}

void indigo_aux_update_weather_safety(indigo_device *device, indigo_property *property, const indigo_aux_safety_input *conditions) {
	assert(device != NULL);
	assert(property != NULL);
	char message[INDIGO_VALUE_SIZE];
	if (indigo_aux_evaluate_weather_safety(property, conditions, message, sizeof(message))) {
		indigo_update_property(device, property, "%s", message);
	} else {
		indigo_update_property(device, property, NULL);
	}
}
