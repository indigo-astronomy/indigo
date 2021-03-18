// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_wheel.c
 */

#include <indigo/indigo_wheel_driver.h>

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_position(indigo_alpaca_device *device, int version, uint32_t *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->filterwheel.position;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_connected(indigo_alpaca_device *device, int version, uint32_t value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value > device->filterwheel.count) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, value + 1);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}


static indigo_alpaca_error alpaca_get_names(indigo_alpaca_device *device, int version, char **value, uint32_t *count) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		*count = 0;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	for (uint32_t i = 0; i < device->filterwheel.count; i++)
		*value++ = device->filterwheel.names[i];
	*count = device->filterwheel.count;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_focusoffsets(indigo_alpaca_device *device, int version, uint32_t *value, uint32_t *count) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		*count = 0;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	for (uint32_t i = 0; i < device->filterwheel.count; i++)
		*value++ = device->filterwheel.focusoffsets[i];
	*count = device->filterwheel.count;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

void indigo_alpaca_wheel_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, WHEEL_SLOT_ITEM_NAME)) {
				alpaca_device->filterwheel.count = item->number.max;
				alpaca_device->filterwheel.position = item->number.value - 1;
			}
		}
	} else if (!strcmp(property->name, WHEEL_SLOT_OFFSET_PROPERTY_NAME)) {
		alpaca_device->filterwheel.count = property->count;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			int index = 0;
			sscanf(item->name, WHEEL_SLOT_OFFSET_ITEM_NAME, &index);
			if (index <= ALPACA_MAX_FILTERS)
				alpaca_device->filterwheel.focusoffsets[index - 1] = item->number.value;
		}
	} else if (!strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		alpaca_device->filterwheel.count = property->count;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			int index = 0;
			sscanf(item->name, WHEEL_SLOT_NAME_ITEM_NAME, &index);
			if (index <= ALPACA_MAX_FILTERS)
				alpaca_device->filterwheel.names[index - 1] = item->text.value;
		}
	}
}

long indigo_alpaca_wheel_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "position")) {
		uint32_t value;
		indigo_alpaca_error result = alpaca_get_position(alpaca_device, version, &value);
		return snprintf(buffer, buffer_length, "\"Value:\": %u, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "names")) {
		char *value[ALPACA_MAX_FILTERS];
		uint32_t count;
		indigo_alpaca_error result = alpaca_get_names(alpaca_device, version, value, &count);
		if (result == indigo_alpaca_error_OK) {
			long index = snprintf(buffer, buffer_length, "\"Value:\": [ ");
			for (int i = 0; i < count; i++) {
				index += snprintf(buffer + index, buffer_length - index, "%s\"%s\"", i == 0 ? "" : ", ", value[i]);
			}
			index += snprintf(buffer + index, buffer_length - index, " ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
			return index;
		} else {
			return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", result, indigo_alpaca_error_string(result));
		}
	}
	if (!strcmp(command, "focusoffsets")) {
		uint32_t value[ALPACA_MAX_FILTERS];
		uint32_t count;
		indigo_alpaca_error result = alpaca_get_focusoffsets(alpaca_device, version, value, &count);
		if (result == indigo_alpaca_error_OK) {
			long index = snprintf(buffer, buffer_length, "\"Value:\": [ ");
			for (int i = 0; i < count; i++) {
				index += snprintf(buffer + index, buffer_length - index, "%s%u", i == 0 ? "" : ", ", value[i]);
			}
			index += snprintf(buffer + index, buffer_length - index, " ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
			return index;
		} else {
			return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", result, indigo_alpaca_error_string(result));
		}
	}
	return 0;
}

long indigo_alpaca_wheel_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "position")) {
		uint32_t value = 1;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Position=%d", &value) == 1)
			result = alpaca_set_connected(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", result, indigo_alpaca_error_string(result));
	}
	return 0;
}
