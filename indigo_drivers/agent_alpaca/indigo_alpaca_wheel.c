// Copyright (c) 2021-2025 CloudMakers, s. r. o.
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
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_wheel.c
 */

#include <indigo/indigo_wheel_driver.h>

#include "indigo_alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 2;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_position(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->wheel.position;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_position(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < 0 || value >= device->wheel.count) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (device->wheel.position != value) {
		device->wheel.position = -1;
		indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, value + 1);
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}


static indigo_alpaca_error alpaca_get_names(indigo_alpaca_device *device, int version, char ***value, int *count) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		*count = 0;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->wheel.names;
	*count = device->wheel.count;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_focusoffsets(indigo_alpaca_device *device, int version, int **value, int *count) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		*count = 0;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->wheel.focusoffsets;
	*count = device->wheel.count;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

void indigo_alpaca_wheel_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, WHEEL_SLOT_ITEM_NAME)) {
					alpaca_device->wheel.count = (int)item->number.max;
					alpaca_device->wheel.position = (int)item->number.value - 1;
				}
			}
		} else {
			alpaca_device->wheel.position = - 1;
		}
	} else if (!strcmp(property->name, WHEEL_SLOT_OFFSET_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			alpaca_device->wheel.count = property->count;
			for (int i = 0; i < property->count; i++) {
				indigo_item* item = property->items + i;
				int index = 0;
				if (sscanf(item->name, WHEEL_SLOT_OFFSET_ITEM_NAME, &index) == 1 && index > 0 && index <= ALPACA_MAX_FILTERS) {
					alpaca_device->wheel.focusoffsets[index - 1] = (int)item->number.value;
				}
			}
		}
	} else if (!strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			alpaca_device->wheel.count = property->count;
			for (int i = 0; i < property->count; i++) {
				indigo_item* item = property->items + i;
				int index = 0;
				if (sscanf(item->name, WHEEL_SLOT_NAME_ITEM_NAME, &index) == 1 && index > 0 && index <= ALPACA_MAX_FILTERS) {
					alpaca_device->wheel.names[index - 1] = item->text.value;
				}
			}
		}
	}
}

long indigo_alpaca_wheel_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "position")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_position(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "names")) {
		char **value;
		int count = 0;
		indigo_alpaca_error result = alpaca_get_names(alpaca_device, version, &value, &count);
		if (result == indigo_alpaca_error_OK) {
			long index = snprintf(buffer, buffer_length, "\"Value\": [ ");
			for (int i = 0; i < count; i++) {
				index += snprintf(buffer + index, buffer_length - index, "%s\"%s\"", i == 0 ? "" : ", ", value[i]);
			}
			index += snprintf(buffer + index, buffer_length - index, " ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
			return index;
		} else {
			return indigo_alpaca_append_error(buffer, buffer_length, result);
		}
	}
	if (!strcmp(command, "focusoffsets")) {
		int *value;
		int count = 0;
		indigo_alpaca_error result = alpaca_get_focusoffsets(alpaca_device, version, &value, &count);
		if (result == indigo_alpaca_error_OK) {
			long index = snprintf(buffer, buffer_length, "\"Value\": [ ");
			for (int i = 0; i < count; i++) {
				index += snprintf(buffer + index, buffer_length - index, "%s%u", i == 0 ? "" : ", ", value[i]);
			}
			index += snprintf(buffer + index, buffer_length - index, " ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
			return index;
		} else {
			return indigo_alpaca_append_error(buffer, buffer_length, result);
		}
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_wheel_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "position")) {
		int value = 1;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Position=%d", &value) == 1) {
			result = alpaca_set_position(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
