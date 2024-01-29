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
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CONNECTION_CONNECTED_ITEM_NAME)) {
					alpaca_device->connected = item->sw.value;
					break;
				}
				if (!strcmp(item->name, CONNECTION_DISCONNECTED_ITEM_NAME)) {
					alpaca_device->connected = !item->sw.value;
					break;
				}
			}
		} else {
			alpaca_device->connected = false;
		}
	} else if (!strcmp(property->name, UTC_TIME_PROPERTY_NAME)) {
		alpaca_device->mount.cansetguiderates = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, UTC_TIME_ITEM_NAME)) {
					pthread_mutex_lock(&alpaca_device->mutex);
					strncpy(alpaca_device->utcdate, item->text.value, sizeof(alpaca_device->utcdate));
					pthread_mutex_unlock(&alpaca_device->mutex);
				}
			}
		}
	} else if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
					alpaca_device->longitude = item->number.value <= 180 ? item->number.value : item->number.value - 360;
				} else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
					alpaca_device->latitude = item->number.value;
				} else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
					alpaca_device->elevation = item->number.value;
				}
			}
		}
	} else if (!strncmp(property->name, "CCD_", 4)) {
		indigo_alpaca_ccd_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "WHEEL_", 6)) {
		indigo_alpaca_wheel_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "FOCUSER_", 8)) {
		indigo_alpaca_focuser_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "ROTATOR_", 8)) {
		indigo_alpaca_rotator_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "MOUNT_", 6)) {
		indigo_alpaca_mount_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "GUIDER_", 7)) {
		indigo_alpaca_guider_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "DOME_", 5)) {
		indigo_alpaca_dome_update_property(alpaca_device, property);
	} else if (!strncmp(property->name, "AUX_", 4)) {
		indigo_alpaca_lightbox_update_property(alpaca_device, property);
		indigo_alpaca_switch_update_property(alpaca_device, property);
	} // TBD other device types
}

bool indigo_alpaca_wait_for_bool(bool *reference, bool value, int timeout) {
	for (int i = 0; i < timeout; i++) {
		if (*reference == value)
			return indigo_alpaca_error_OK;
		indigo_usleep(500000);
	}
	return indigo_alpaca_error_ValueNotSet;
}

bool indigo_alpaca_wait_for_int32(int32_t *reference, int32_t value, int timeout) {
	for (int i = 0; i < timeout; i++) {
		if (*reference == value)
			return indigo_alpaca_error_OK;
		indigo_usleep(500000);
	}
	return indigo_alpaca_error_ValueNotSet;
}

bool indigo_alpaca_wait_for_double(double *reference, double value, int timeout) {
	for (int i = 0; i < timeout; i++) {
		if (*reference == value)
			return indigo_alpaca_error_OK;
		indigo_usleep(500000);
	}
	return indigo_alpaca_error_ValueNotSet;
}

static indigo_alpaca_error alpaca_get_name(indigo_alpaca_device *device, int version, char *value) {
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

static indigo_alpaca_error alpaca_get_connected(indigo_alpaca_device *device, int version, bool *value) {
	*value = device->connected;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_utcdate(indigo_alpaca_device *device, int version, char *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (*device->utcdate == 0) {
		indigo_timetoisogm(time(NULL), device->utcdate, sizeof(device->utcdate));
	}
	strcpy(value, device->utcdate);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_latitude(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->latitude;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_longitude(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->longitude;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_elevation(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->elevation;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_connected(indigo_alpaca_device *device, int version, bool value) {
	pthread_mutex_lock(&device->mutex);
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, CONNECTION_PROPERTY_NAME, value ? CONNECTION_CONNECTED_ITEM_NAME : CONNECTION_DISCONNECTED_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->connected, value, 30);
}

static indigo_alpaca_error alpaca_set_latitude(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < -90 || value > 90) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_longitude(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < -180 || value > 180) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (value < 0)
		value += 360;
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_elevation(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (value < -300 || value > 10000) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}


long indigo_alpaca_append_error(char *buffer, long buffer_length, indigo_alpaca_error result) {
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", result, indigo_alpaca_error_string(result));
}

long indigo_alpaca_append_value_bool(char *buffer, long buffer_length, bool value, indigo_alpaca_error result) {
	return snprintf(buffer, buffer_length, "\"Value\": %s, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value ? "true" : "false", result, indigo_alpaca_error_string(result));
}

long indigo_alpaca_append_value_int(char *buffer, long buffer_length, int value, indigo_alpaca_error result) {
	return snprintf(buffer, buffer_length, "\"Value\": %d, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
}

long indigo_alpaca_append_value_double(char *buffer, long buffer_length, double value, indigo_alpaca_error result) {
	return snprintf(buffer, buffer_length, "\"Value\": %f, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
}

long indigo_alpaca_append_value_string(char *buffer, long buffer_length, char *value, indigo_alpaca_error result) {
	if (value) {
		return snprintf(buffer, buffer_length, "\"Value\": \"%s\", \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value, result, indigo_alpaca_error_string(result));
	} else {
		return snprintf(buffer, buffer_length, "\"Value\": \"\", \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", result, indigo_alpaca_error_string(result));
	}
}

long indigo_alpaca_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, int id, char *buffer, long buffer_length) {
	if (!strcmp(command, "name")) {
		char value[INDIGO_NAME_SIZE] = {0};
		indigo_alpaca_error result = alpaca_get_name(alpaca_device, version, value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "description")) {
		char value[INDIGO_NAME_SIZE] = {0};
		indigo_alpaca_error result = alpaca_get_description(alpaca_device, version, value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "driverinfo")) {
		char value[INDIGO_VALUE_SIZE] = {0};
		indigo_alpaca_error result = alpaca_get_driverinfo(alpaca_device, version, value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "driverversion")) {
		char value[INDIGO_VALUE_SIZE] = {0};
		indigo_alpaca_error result = alpaca_get_driverversion(alpaca_device, version, value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "connected")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_connected(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "utcdate")) {
		char value[64] = {0};
		indigo_alpaca_error result = alpaca_get_utcdate(alpaca_device, version, value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "sitelatitude")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_latitude(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "sitelongitude")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_longitude(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "siteelevation")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_elevation(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	long result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_CCD) &&
		(result = indigo_alpaca_ccd_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_WHEEL) &&
		(result = indigo_alpaca_wheel_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_FOCUSER) &&
		(result = indigo_alpaca_focuser_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_ROTATOR) &&
		(result = indigo_alpaca_rotator_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_MOUNT) &&
		(result = indigo_alpaca_mount_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_GUIDER) &&
		(result = indigo_alpaca_guider_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_DOME) &&
		(result = indigo_alpaca_dome_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_LIGHTBOX) &&
		(result = indigo_alpaca_lightbox_get_command(alpaca_device, version, command, buffer, buffer_length))
	) return result;
	if (
		(IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_POWERBOX) || IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_GPIO)) &&
		(result = indigo_alpaca_switch_get_command(alpaca_device, version, command, id, buffer, buffer_length))
	) return result;
	// TBD other device types
	return 0;
}

long indigo_alpaca_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "connected")) {
		bool value = !strcasecmp(param_1, "Connected=true");
		indigo_alpaca_error result = alpaca_set_connected(alpaca_device, version, value);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "sitelatitude")) {
		double value;
		indigo_alpaca_error result;
		if (sscanf(param_1, "SiteLatitude=%lf", &value) == 1)
			result = alpaca_set_latitude(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "sitelongitude")) {
		double value;
		indigo_alpaca_error result;
		if (sscanf(param_1, "SiteLongitude=%lf", &value) == 1)
			result = alpaca_set_longitude(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "siteelevation")) {
		double value;
		indigo_alpaca_error result;
		if (sscanf(param_1, "SiteElevation=%lf", &value) == 1)
			result = alpaca_set_elevation(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	long result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_CCD) &&
		(result = indigo_alpaca_ccd_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_WHEEL) &&
		(result = indigo_alpaca_wheel_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_FOCUSER) &&
		(result = indigo_alpaca_focuser_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_ROTATOR) &&
		(result = indigo_alpaca_rotator_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_MOUNT) &&
		(result = indigo_alpaca_mount_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_GUIDER) &&
		(result = indigo_alpaca_guider_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_DOME) &&
		(result = indigo_alpaca_dome_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_LIGHTBOX) &&
		(result = indigo_alpaca_lightbox_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	if (
		(IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_POWERBOX) || IS_DEVICE_TYPE(alpaca_device, INDIGO_INTERFACE_AUX_GPIO)) &&
		(result = indigo_alpaca_switch_set_command(alpaca_device, version, command, buffer, buffer_length, param_1, param_2))
	) return result;
	return 0;
}
