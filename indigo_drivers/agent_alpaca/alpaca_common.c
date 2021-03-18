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
 \file alpaca_common.c
 */

#include <indigo/indigo_driver.h>

#include "alpaca_common.h"

char *indigo_alpaca_error_string(int code) {
	switch (code) {
		case indigo_alpaca_error_OK:
			return "";
		case indigo_alpaca_error_NotImplemented:
			return "Property or method not implemented";
		case indigo_alpaca_error_InvalidValue:
			return "Invalid value";
		case indigo_alpaca_error_ValueNotSet:
			return "Value not set";
		case indigo_alpaca_error_NotConnected:
			return "Not connected";
		case indigo_alpaca_error_InvalidWhileParked:
			return "Invalid while parked";
		case indigo_alpaca_error_InvalidWhileSlaved:
			return "Invalid while slaved";
		case indigo_alpaca_error_InvalidOperation:
			return "Invalid operation";
		case indigo_alpaca_error_ActionNotImplemented:
			return "Action not implemented";
		default:
			return "Unknown code";
	}
}

void indigo_alpaca_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, INFO_DEVICE_INTERFACE_ITEM_NAME)) {
				uint64_t interface = atoll(item->text.value);
				switch (interface) {
					case INDIGO_INTERFACE_CCD:
						alpaca_device->device_type = "Camera";
						break;
					case INDIGO_INTERFACE_DOME:
						alpaca_device->device_type = "Dome";
						break;
					case INDIGO_INTERFACE_WHEEL:
						alpaca_device->device_type = "Filterwheel";
						break;
					case INDIGO_INTERFACE_FOCUSER:
						alpaca_device->device_type = "Focuser";
						break;
					case INDIGO_INTERFACE_ROTATOR:
						alpaca_device->device_type = "Rotator";
						break;
					case INDIGO_INTERFACE_AUX_POWERBOX:
						alpaca_device->device_type = "Switch";
						break;
					case INDIGO_INTERFACE_AO:
					case INDIGO_INTERFACE_MOUNT:
					case INDIGO_INTERFACE_GUIDER:
						alpaca_device->device_type = "Telescope";
						break;
					default:
						alpaca_device->device_type = NULL;
						interface = 0;
				}
				alpaca_device->indigo_interface = interface;
			} else if (!strcmp(item->name, INFO_DEVICE_NAME_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->device_name, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			} else if (!strcmp(item->name, INFO_DEVICE_DRVIER_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->driver_info, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			} else if (!strcmp(item->name, INFO_DEVICE_VERSION_ITEM_NAME)) {
				pthread_mutex_lock(&alpaca_device->mutex);
				strcpy(alpaca_device->driver_version, item->text.value);
				pthread_mutex_unlock(&alpaca_device->mutex);
			}
		}
	} else if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CONNECTION_CONNECTED_ITEM_NAME)) {
					alpaca_device->connected = item->sw.value;
				}
			}
		} else {
			alpaca_device->connected = false;
		}
	} else if (!strncmp(property->name, "WHEEL_", 6)) {
		indigo_alpaca_wheel_update_property(alpaca_device, property);
	} // TBD other device types
}

static indigo_alpaca_error alpaca_get_devicename(indigo_alpaca_device *device, int version, char *value) {
	pthread_mutex_lock(&device->mutex);
	indigo_copy_name(value, device->device_name);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_description(indigo_alpaca_device *device, int version, char *value) {
	pthread_mutex_lock(&device->mutex);
	indigo_copy_name(value, device->indigo_device);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_driverinfo(indigo_alpaca_device *device, int version, char *value) {
	pthread_mutex_lock(&device->mutex);
	indigo_copy_value(value, device->driver_info);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_driverversion(indigo_alpaca_device *device, int version, char *value) {
	pthread_mutex_lock(&device->mutex);
	indigo_copy_value(value, device->driver_version);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, uint32_t *value) {
	*value = ALPACA_INTERFACE_VERSION;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_connected(indigo_alpaca_device *device, int version, bool *value) {
	*value = device->connected;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_connected(indigo_alpaca_device *device, int version, bool value) {
	pthread_mutex_lock(&device->mutex);
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

long indigo_alpaca_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "devicename")) {
		char value[INDIGO_NAME_SIZE];
		indigo_alpaca_error result = alpaca_get_devicename(alpaca_device, version, value);
		return snprintf(buffer, buffer_length, "\"Value:\": \"%s\", \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "description")) {
		char value[INDIGO_NAME_SIZE];
		indigo_alpaca_error result = alpaca_get_description(alpaca_device, version, value);
		return snprintf(buffer, buffer_length, "\"Value:\": \"%s\", \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "driverinfo")) {
		char value[INDIGO_VALUE_SIZE];
		indigo_alpaca_error result = alpaca_get_driverinfo(alpaca_device, version, value);
		return snprintf(buffer, buffer_length, "\"Value:\": \"%s\", \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "driverversion")) {
		char value[INDIGO_VALUE_SIZE];
		indigo_alpaca_error result = alpaca_get_driverversion(alpaca_device, version, value);
		return snprintf(buffer, buffer_length, "\"Value:\": \"%s\", \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "interfaceversion")) {
		uint32_t value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
		return snprintf(buffer, buffer_length, "\"Value:\": %d, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "connected")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_connected(alpaca_device, version, &value);
		return snprintf(buffer, buffer_length, "\"Value:\": %s, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value ? "true" : "false", result, indigo_alpaca_error_string(result));
	}
	long result;
	if ((result = indigo_alpaca_wheel_get_command(alpaca_device, version, command, buffer, buffer_length)))
		return result;
	// TBD other device types
	return 0;
}

long indigo_alpaca_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "connected")) {
		bool value = strcasecmp(param_1, "Connected=true");
		indigo_alpaca_error result = alpaca_set_connected(alpaca_device, version, value);
		return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", result, indigo_alpaca_error_string(result));
	}
	long result;
	if ((result = indigo_alpaca_wheel_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2)))
		return result;
	return 0;
}
