// Copyright (c) 2021 Rumen G. Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_rotator.c
 */

#include <indigo/indigo_rotator_driver.h>

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 1;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_ismoving(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->rotator.ismoving;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canreverse(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->rotator.canreverse;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_reverse(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->rotator.canreverse) {
		*value = device->rotator.reversed;
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}

	*value = false;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_get_position(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->rotator.position;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_targetposition(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->rotator.targetposition;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_stepsize(indigo_alpaca_device *device, int version, double *value) {
	if (!device->connected) {
		return indigo_alpaca_error_NotConnected;
	}
	return indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_set_reverse(indigo_alpaca_device *device, int version, bool value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->rotator.ismoving) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	if (device->rotator.canreverse) {
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_REVERSED_ITEM_NAME, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_NotImplemented;
}

static indigo_alpaca_error alpaca_move_absolute(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->rotator.ismoving) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}
	if ((device->rotator.min > value) || (device->rotator.max < value)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (device->rotator.position != value) {
		device->rotator.ismoving = true;
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true);
		indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, value);
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_move_relative(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->rotator.ismoving) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidOperation;
	}

	double absolute_value = device->rotator.position + value;
	if ((device->rotator.min > absolute_value) || (device->rotator.max < absolute_value)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (device->rotator.position != absolute_value) {
		device->rotator.ismoving = true;
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true);
		indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, absolute_value);
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

//static indigo_alpaca_error alpaca_sync(indigo_alpaca_device *device, int version, double value) {
//	pthread_mutex_lock(&device->mutex);
//	if (!device->connected) {
//		pthread_mutex_unlock(&device->mutex);
//		return indigo_alpaca_error_NotConnected;
//	}
//
//	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, true);
//	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, value);
//	pthread_mutex_unlock(&device->mutex);
//	return indigo_alpaca_error_OK;
//}

static indigo_alpaca_error alpaca_halt(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->rotator.ismoving, false, 30);
}

void indigo_alpaca_rotator_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, ROTATOR_POSITION_PROPERTY_NAME)) {
		alpaca_device->rotator.ismoving = property->state == INDIGO_BUSY_STATE;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, ROTATOR_POSITION_ITEM_NAME)) {
				alpaca_device->rotator.position = item->number.value;
				alpaca_device->rotator.targetposition = item->number.target;
				alpaca_device->rotator.mechanicalposition = item->number.value;
				if (!alpaca_device->rotator.haslimits) {
					alpaca_device->rotator.min = item->number.min;
					alpaca_device->rotator.max = item->number.max;
				}
			}
		}
	}
	if (!strcmp(property->name, ROTATOR_DIRECTION_PROPERTY_NAME)) {
		alpaca_device->rotator.canreverse = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, ROTATOR_DIRECTION_NORMAL_ITEM_NAME)) {
				alpaca_device->rotator.reversed = !item->sw.value;
			}
		}
	}
	if (!strcmp(property->name, ROTATOR_LIMITS_PROPERTY_NAME)) {
		alpaca_device->rotator.haslimits = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, ROTATOR_LIMITS_MAX_POSITION_ITEM_NAME)) {
				alpaca_device->rotator.max = item->number.value;
			}
			if (!strcmp(item->name, ROTATOR_LIMITS_MIN_POSITION_ITEM_NAME)) {
				alpaca_device->rotator.min = item->number.value;
			}
		}
	}
}

long indigo_alpaca_rotator_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canreverse")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_canreverse(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "reverse")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_reverse(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "ismoving")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_ismoving(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "position")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_position(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "targetposition")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_targetposition(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "mechanicalposition")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_position(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "stepsize")) {
		double value = 0;
		indigo_alpaca_error result = alpaca_get_stepsize(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_rotator_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "reverse")) {
		bool value = !strcasecmp(param_1, "Reverse=true");
		indigo_alpaca_error result = alpaca_set_reverse(alpaca_device, version, value);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "sync")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Position=%lf", &value) == 1)
			result = alpaca_move_relative(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "move")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Position=%lf", &value) == 1)
			result = alpaca_move_relative(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (
		!strcmp(command, "moveabsolute") ||
		!strcmp(command, "movemechanical")
	) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Position=%lf", &value) == 1)
			result = alpaca_move_absolute(alpaca_device, version, value);
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
