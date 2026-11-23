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

indigo_result indigo_aux_attach(indigo_device *device, const char* driver_name, unsigned version, int interface) {
	assert(device != NULL);
	if (indigo_device_attach(device, driver_name, version, interface) == INDIGO_OK) {
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_aux_detach(indigo_device *device) {
	assert(device != NULL);
	return indigo_device_detach(device);
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
