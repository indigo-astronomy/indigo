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

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_canpuseguide(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->guider.cansetguiderates;
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
					if (property->count == 1)
						alpaca_device->guider.guideratedeclination = item->number.value;
				}
			}
		}
	}
}

long indigo_alpaca_guider_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "canpuseguide")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_canpuseguide(alpaca_device, version, &value);
		return snprintf(buffer, buffer_length, "\"Value\": %s, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value ? "true" : "false", result, indigo_alpaca_error_string(result));
	}
	if (!strcmp(command, "cansetguiderates")) {
		bool value = false;
		indigo_alpaca_error result = alpaca_get_cansetguiderates(alpaca_device, version, &value);
		return snprintf(buffer, buffer_length, "\"Value\": %s, \"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", value ? "true" : "false", result, indigo_alpaca_error_string(result));
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
	if (!strcmp(command, "utcdate")) {
		return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_InvalidOperation, indigo_alpaca_error_string(indigo_alpaca_error_InvalidOperation));
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_guider_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strncmp(command, "slew", 4)) {
		return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
