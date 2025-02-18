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
 \file alpaca_lightbox.c
 */

#include <indigo/indigo_aux_driver.h>

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 1;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_brightness(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->covercalibrator.brightness;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxbrightness(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->covercalibrator.maxbrightness;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_calibratorstate(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->covercalibrator.calibratorstate;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_coverstate(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->covercalibrator.coverstate;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}


static indigo_alpaca_error alpaca_calibratoron(indigo_alpaca_device *device, int version, int value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->covercalibrator.calibratorstate == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (value > device->covercalibrator.maxbrightness) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME, true);
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return 	indigo_alpaca_wait_for_int32(&device->covercalibrator.calibratorstate, 3, 30);
}

static indigo_alpaca_error alpaca_calibratoroff(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->covercalibrator.calibratorstate == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_int32(&device->covercalibrator.calibratorstate, 1, 30);
}

static indigo_alpaca_error alpaca_closecover(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->covercalibrator.coverstate == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_COVER_PROPERTY_NAME, AUX_COVER_CLOSE_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_opencover(indigo_alpaca_device *device, int version) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (device->covercalibrator.coverstate == 0) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_COVER_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME, true);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_haltcover(indigo_alpaca_device *device, int version) {
	if (!device->connected) {
		return indigo_alpaca_error_NotConnected;
	}
	return indigo_alpaca_error_NotImplemented;
}

void indigo_alpaca_lightbox_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, AUX_COVER_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			alpaca_device->covercalibrator.coverstate = 2;
		} else if (property->state == INDIGO_ALERT_STATE) {
			alpaca_device->covercalibrator.coverstate = 5;
		} else {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, AUX_COVER_CLOSE_ITEM_NAME)) {
					if (item->sw.value) {
						alpaca_device->covercalibrator.coverstate = 1;
						return;
					}
				} else if (!strcmp(item->name, AUX_COVER_OPEN_ITEM_NAME)) {
					if (item->sw.value) {
						alpaca_device->covercalibrator.coverstate = 3;
						return;;
					}
				}
			}
			alpaca_device->covercalibrator.coverstate = 4;
		}
	} else if (!strcmp(property->name, AUX_LIGHT_SWITCH_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			alpaca_device->covercalibrator.calibratorstate = 2;
		} else if (property->state == INDIGO_ALERT_STATE) {
			alpaca_device->covercalibrator.calibratorstate = 5;
		} else {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, AUX_LIGHT_SWITCH_OFF_ITEM_NAME)) {
					if (item->sw.value) {
						alpaca_device->covercalibrator.calibratorstate = 1;
						alpaca_device->covercalibrator.brightness = 0;
						return;
					}
				} else if (!strcmp(item->name, AUX_LIGHT_SWITCH_ON_ITEM_NAME)) {
					if (item->sw.value) {
						alpaca_device->covercalibrator.calibratorstate = 3;
						return;;
					}
				}
			}
			alpaca_device->covercalibrator.calibratorstate = 4;
		}
	} else if (!strcmp(property->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, AUX_LIGHT_INTENSITY_ITEM_NAME)) {
				alpaca_device->covercalibrator.brightness = item->number.value;
				alpaca_device->covercalibrator.maxbrightness = item->number.max;
			}
		}
	}
}

long indigo_alpaca_lightbox_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "brightness")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_brightness(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxbrightness")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_maxbrightness(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "calibratorstate")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_calibratorstate(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "coverstate")) {
		int value = 0;
		indigo_alpaca_error result = alpaca_get_coverstate(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	return 0;
}

long indigo_alpaca_lightbox_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "calibratoroff")) {
		indigo_alpaca_error result = alpaca_calibratoroff(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "calibratoron")) {
		int value = 1;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Brightness=%d", &value) == 1)
			result = alpaca_calibratoron(alpaca_device, version, value);
		else
			result = indigo_alpaca_error_InvalidValue;
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "closecover")) {
		indigo_alpaca_error result = alpaca_closecover(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "opencover")) {
		indigo_alpaca_error result = alpaca_opencover(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "haltcover")) {
		indigo_alpaca_error result = alpaca_haltcover(alpaca_device, version);
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return 0;
}
