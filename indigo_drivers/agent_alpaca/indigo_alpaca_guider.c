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
 \file alpaca_guider.c
 */

#include <indigo/indigo_guider_driver.h>

#include "indigo_alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 3;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canpulseguide(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->guider.canpulseguide;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetguiderates(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->guider.cansetguiderates;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_ispulseguiding(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->guider.canpulseguide) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->guider.ispulseguiding;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}


static indigo_alpaca_error alpaca_get_guideratedeclination(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->guider.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->guider.guideratedeclination;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_guideraterightascension(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->guider.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->guider.guideraterightascension;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_guideratedeclination(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->guider.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GUIDER_RATE_PROPERTY_NAME, GUIDER_DEC_RATE_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_guideraterightascension(indigo_alpaca_device *device, int version, double value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->guider.cansetguiderates) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_pulseguide(indigo_alpaca_device *device, int version, int direction, double duration) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->guider.canpulseguide) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (duration > 0) {
		switch (direction) {
			case 0:
				device->guider.ispulseguiding = true;
				indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, duration);
				break;
			case 1:
				device->guider.ispulseguiding = true;
				indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, duration);
				break;
			case 2:
				device->guider.ispulseguiding = true;
				indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, duration);
				break;
			case 3:
				device->guider.ispulseguiding = true;
				indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, duration);
				break;
			default:
				pthread_mutex_unlock(&device->mutex);
				return indigo_alpaca_error_InvalidValue;
		}
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

void indigo_alpaca_guider_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, GUIDER_RATE_PROPERTY_NAME)) {
		alpaca_device->guider.cansetguiderates = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, GUIDER_DEC_RATE_ITEM_NAME)) {
					alpaca_device->guider.guideratedeclination = item->number.value;
				} else if (!strcmp(item->name, GUIDER_RATE_ITEM_NAME)) {
					alpaca_device->guider.guideraterightascension = item->number.value;
					if (property->count == 1) {
						alpaca_device->guider.guideratedeclination = item->number.value;
					}
				}
			}
		}
	} else if (!strcmp(property->name, GUIDER_GUIDE_RA_PROPERTY_NAME)) {
		alpaca_device->guider.canpulseguide = true;
		alpaca_device->guider.ispulseguiding = property->state == INDIGO_BUSY_STATE;
	} else if (!strcmp(property->name, GUIDER_GUIDE_DEC_PROPERTY_NAME)) {
		alpaca_device->guider.canpulseguide = true;
		alpaca_device->guider.ispulseguiding = property->state == INDIGO_BUSY_STATE;
	}
}

long indigo_alpaca_guider_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canpulseguide")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_canpulseguide(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetguiderates")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cansetguiderates(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "ispulseguiding")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_ispulseguiding(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "guideratedeclination")) {
		double value = false;
		indigo_alpaca_error result = alpaca_get_guideratedeclination(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "guideraterightascension")) {
		double value = false;
		indigo_alpaca_error result = alpaca_get_guideraterightascension(alpaca_device, version, &value);
	return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "atpark")) {
		return snprintf(buffer, buffer_length, "\"Value\": false, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "trackingrates")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "axisrates")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strncmp(command, "can", 3)) {
		return snprintf(buffer, buffer_length, "\"Value\": false, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "declination")) {
		return snprintf(buffer, buffer_length, "\"Value\": 0, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "rightascension")) {
		return snprintf(buffer, buffer_length, "\"Value\": 0, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "declinationrate")) {
		return snprintf(buffer, buffer_length, "\"Value\": 0, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "rightascensionrate")) {
		return snprintf(buffer, buffer_length, "\"Value\": 0, \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_guider_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "guideratedeclination")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "GuideRateDeclination=%lf", &value) == 1) {
			result = alpaca_set_guideratedeclination(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "guideraterightascensionrate")) {
		double value = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "GuideRateRightAscension=%lf", &value) == 1) {
			result = alpaca_set_guideraterightascension(alpaca_device, version, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "pulseguide")) {
		int direction = 0;
		double duration = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "Direction=%d", &direction) == 1 && sscanf(param_2, "Duration=%lf", &duration) == 1) {
			result = alpaca_pulseguide(alpaca_device, version, direction, duration);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strncmp(command, "slew", 4)) {
		return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
