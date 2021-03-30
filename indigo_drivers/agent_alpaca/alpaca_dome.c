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
// 2.0 by Rumen G.Bogdanovski <rumen@skyarchive.org>

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_dome.c
 */

#include <indigo/indigo_dome_driver.h>

#include "alpaca_common.h"

typedef enum {
	SHUTTER_OPEN = 0,
	SHUTTER_CLOSED = 1,
	SHUTTER_OPENING = 2,
	SHUTTER_CLOSING = 3,
	SHUTTER_ERROR = 4
} alpaca_dome_status;

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, uint32_t *value) {
	*value = 1;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_slewing(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.isshuttermoving || device->dome.isrotating || device->dome.isflapmoving;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_altitide(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->dome.cansetaltitude) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->dome.altitude;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetaltitude(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.cansetaltitude;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_azimuth(indigo_alpaca_device *device, int version, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->dome.cansetazimuth) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->dome.azimuth;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetazimuth(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.cansetazimuth;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_athome(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->dome.canfindhome) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->dome.athome;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_atpark(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->dome.canpark) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->dome.atpark;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetpark(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.cansetpark;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansetshutter(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.cansetshutter;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canslave(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.canslave;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_cansyncazimuth(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.cansyncazimuth;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canfindhome(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.canfindhome;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canpark(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = device->dome.canpark;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_shutterstatus(indigo_alpaca_device *device, int version, uint32_t *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->dome.cansetshutter) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->dome.shutterstatus;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_slaved(indigo_alpaca_device *device, int version, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!device->dome.canslave) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	*value = device->dome.slaved;
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

void indigo_alpaca_dome_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM_NAME)) {
				alpaca_device->dome.cansetazimuth = true;
			} else if (!strcmp(item->name, DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM_NAME)) {
				alpaca_device->dome.cansyncazimuth = true;
			}
		}
	} else if (!strcmp(property->name, DOME_PARK_PROPERTY_NAME)) {
		alpaca_device->dome.canpark = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, DOME_PARK_PARKED_ITEM_NAME)) {
					alpaca_device->dome.atpark = item->sw.value;
					alpaca_device->dome.canpark = true;
				} else if (!strcmp(item->name, DOME_PARK_UNPARKED_ITEM_NAME)) {
					alpaca_device->dome.canpark = true;
				}
			}
		}
	} else if (!strcmp(property->name, DOME_PARK_POSITION_PROPERTY_NAME)) {
		alpaca_device->dome.cansetpark = true;

	} else if (!strcmp(property->name, DOME_HOME_PROPERTY_NAME)) {
		alpaca_device->dome.canfindhome = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, DOME_HOME_ITEM_NAME)) {
					alpaca_device->dome.athome = item->sw.value;
				}
			}
		} else {
			alpaca_device->dome.athome = false;
		}
	} else if (!strcmp(property->name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, DOME_HORIZONTAL_COORDINATES_ALT_ITEM_NAME)) {
				alpaca_device->dome.altitude = item->number.value;
				if (property->perm == INDIGO_RW_PERM) {
					alpaca_device->dome.cansetaltitude = true;
				}
			} else if (!strcmp(item->name, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME)) {
				alpaca_device->dome.azimuth = item->number.value;
				if (property->perm == INDIGO_RW_PERM) {
					alpaca_device->dome.cansetazimuth = true;
				}
			}
		}
		if (property->state == INDIGO_BUSY_STATE) {
			alpaca_device->dome.isrotating = true;
		} else {
			alpaca_device->dome.isrotating = false;
		}
	} else if (!strcmp(property->name, DOME_SLAVING_PROPERTY_NAME)) {
		alpaca_device->dome.canslave = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, DOME_SLAVING_ENABLE_ITEM_NAME)) {
					alpaca_device->dome.slaved = item->sw.value;
					alpaca_device->dome.canslave = true;
				} else if (!strcmp(item->name, DOME_SLAVING_DISABLE_ITEM_NAME)) {
					alpaca_device->dome.canslave = true;
				}
			}
		}
	} else if (!strcmp(property->name, DOME_SHUTTER_PROPERTY_NAME)) {
		alpaca_device->dome.cansetshutter = true;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, DOME_SHUTTER_CLOSED_ITEM_NAME)) {
					if (item->sw.value) {
						if (property->state == INDIGO_BUSY_STATE) {
							alpaca_device->dome.shutterstatus = SHUTTER_CLOSING;
						} else {
							alpaca_device->dome.shutterstatus = SHUTTER_CLOSED;
						}
					}
				} else if (!strcmp(item->name, DOME_SHUTTER_OPENED_ITEM_NAME)) {
					if (item->sw.value) {
						if (property->state == INDIGO_BUSY_STATE) {
							alpaca_device->dome.shutterstatus = SHUTTER_OPENING;
						} else {
							alpaca_device->dome.shutterstatus = SHUTTER_OPEN;
						}
					}
				} else {
					if (property->state == INDIGO_BUSY_STATE) {
						alpaca_device->dome.shutterstatus = SHUTTER_OPENING;
					} else {
						alpaca_device->dome.shutterstatus = SHUTTER_OPEN;
					}
				}
			}
			if (property->state == INDIGO_ALERT_STATE) {
				alpaca_device->dome.shutterstatus = SHUTTER_ERROR;
			}
			if (property->state == INDIGO_BUSY_STATE) {
				alpaca_device->dome.isshuttermoving = true;
			} else {
				alpaca_device->dome.isshuttermoving = false;
			}
		}
	} else if(!strcmp(property->name, DOME_FLAP_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE) {
			alpaca_device->dome.isflapmoving = true;
		} else {
			alpaca_device->dome.isflapmoving = false;
		}
	}
}

long indigo_alpaca_dome_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		uint32_t value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "altitude")) {
		double value;
		indigo_alpaca_error result = alpaca_get_altitide(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "azimuth")) {
		double value;
		indigo_alpaca_error result = alpaca_get_azimuth(alpaca_device, version, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "athome")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_athome(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "atpark")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_atpark(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canfindhome")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_canfindhome(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canpark")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_canpark(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetaltitude")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_cansetaltitude(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetazimuth")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_cansetazimuth(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetpark")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_cansetpark(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansetshutter")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_cansetshutter(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canslave")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_canslave(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "cansyncazimuth")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_cansyncazimuth(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "slaved")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_slaved(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "shutterstatus")) {
		uint32_t value;
		indigo_alpaca_error result = alpaca_get_shutterstatus(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "slewing")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_slewing(alpaca_device, version, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_dome_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {

	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
