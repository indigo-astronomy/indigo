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
 \file alpaca_focuser.c
 */

#include <indigo/indigo_focuser_driver.h>

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 1;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_absolute(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->focuser.absolute;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_ismoving(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->focuser.ismoving;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxincrement(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->focuser.maxincrement;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxstep(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->focuser.maxstep;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_position(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->focuser.absolute) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->focuser.position;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_stepsize(indigo_alpaca_device *device, int version, double *value) {
	if (!device->connected) {
		return indigo_alpaca_error_NotConnected;
	}
	return indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_get_tempcomp(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->focuser.tempcompavailable) {
		*value = false;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	*value = device->focuser.tempcomp;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_tempcomp(indigo_alpaca_device *device, int version, bool value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->focuser.tempcompavailable) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_MODE_PROPERTY_NAME, value ? FOCUSER_MODE_AUTOMATIC_ITEM_NAME : FOCUSER_MODE_MANUAL_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->focuser.tempcomp, value, 30);
}

static indigo_alpaca_error alpaca_get_tempcompavailable(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->focuser.tempcompavailable;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_temperature(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->focuser.temperatureavailable) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->focuser.temperature;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_move(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->focuser.tempcompavailable && device->focuser.tempcomp) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	if (device->focuser.absolute) {
		if (value < 0)
			value = 0;
		if (value > device->focuser.maxstep) {
			value = device->focuser.maxstep;
		}
		if (value != device->focuser.position) {
			device->focuser.ismoving = true;
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true);
			indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, value + device->focuser.offset);
		}
	} else {
		if (value > 0) {
			device->focuser.ismoving = true;
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
			indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, value);
		} else if (value < 0) {
			if (value < -device->focuser.maxincrement) {
				pthread_mutex_unlock(&device->mutex);
				return indigo_alpaca_error_InvalidValue;
			}
			device->focuser.ismoving = true;
			indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
			indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, -value);
		}
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_halt(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->focuser.ismoving, false, 30);
}

void indigo_alpaca_focuser_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, FOCUSER_POSITION_PROPERTY_NAME)) {
		if (property->perm == INDIGO_RW_PERM) {
			alpaca_device->focuser.absolute = true;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, FOCUSER_POSITION_ITEM_NAME)) {
					alpaca_device->focuser.offset = (int)item->number.min;
					alpaca_device->focuser.maxstep = (int)item->number.max - alpaca_device->focuser.offset;
					alpaca_device->focuser.maxincrement = (int)item->number.max - alpaca_device->focuser.offset;
					alpaca_device->focuser.position = (int)item->number.value - alpaca_device->focuser.offset;
				}
			}
		} else {
			alpaca_device->focuser.absolute = false;
		}
	} else if (!strcmp(property->name, FOCUSER_STEPS_PROPERTY_NAME)) {
		alpaca_device->focuser.ismoving = property->state == INDIGO_BUSY_STATE;
		if (!alpaca_device->focuser.absolute && alpaca_device->focuser.maxincrement == 0) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, FOCUSER_STEPS_ITEM_NAME)) {
					alpaca_device->focuser.offset = 0;
					alpaca_device->focuser.maxstep = (int)item->number.max;
					alpaca_device->focuser.maxincrement = (int)item->number.max;
				}
			}
		}
	} else if (!strcmp(property->name, FOCUSER_TEMPERATURE_PROPERTY_NAME)) {
		alpaca_device->focuser.temperatureavailable = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, FOCUSER_TEMPERATURE_ITEM_NAME)) {
				alpaca_device->focuser.temperature = item->number.value;
			}
		}
	} else if (!strcmp(property->name, FOCUSER_MODE_PROPERTY_NAME)) {
		alpaca_device->focuser.tempcompavailable = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, FOCUSER_MODE_AUTOMATIC_ITEM_NAME)) {
				alpaca_device->focuser.tempcomp = item->sw.value;
				break;
			}
			if (!strcmp(item->name, FOCUSER_MODE_MANUAL_ITEM_NAME)) {
				alpaca_device->focuser.tempcomp = !item->sw.value;
				break;
			}
		}
	}
}

long indigo_alpaca_focuser_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "absolute")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_absolute(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "ismoving")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_ismoving(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxincrement")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_maxincrement(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxstep")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_maxstep(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "position")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_position(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "stepsize")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_stepsize(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "tempcomp")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_tempcomp(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "tempcompavailable")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_tempcompavailable(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "temperature")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_temperature(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_focuser_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "tempcomp")) {
		bool value = !strcasecmp(param_1, "TempComp=true");
		indigo_alpaca_error result = alpaca_set_tempcomp(alpaca_device, version, value);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "move")) {
		int value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Position=%d", &value) == 1)
			result = alpaca_move(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "halt")) {
		indigo_alpaca_error result = alpaca_halt(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
