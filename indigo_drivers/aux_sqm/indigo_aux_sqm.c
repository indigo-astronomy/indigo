// Copyright (c) 2019-2025 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_aux_sqm.driver

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_sqm.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000012
#define DRIVER_NAME          "indigo_aux_sqm"
#define DRIVER_LABEL         "Unihedron SQM"
#define AUX_DEVICE_NAME      "Unihedron SQM"
#define PRIVATE_DATA         ((sqm_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Sensor"

//- define

#pragma mark - Property definitions

// AUX_INFO handles definition
#define AUX_INFO_PROPERTY                (PRIVATE_DATA->aux_info_property)
#define X_AUX_SENSOR_FREQUENCY_ITEM      (AUX_INFO_PROPERTY->items + 0)
#define X_AUX_SENSOR_COUNTS_ITEM         (AUX_INFO_PROPERTY->items + 1)
#define X_AUX_SENSOR_PERIOD_ITEM         (AUX_INFO_PROPERTY->items + 2)

#define X_AUX_SENSOR_FREQUENCY_ITEM_NAME "X_AUX_SENSOR_FREQUENCY"
#define X_AUX_SENSOR_COUNTS_ITEM_NAME    "X_AUX_SENSOR_COUNTS"
#define X_AUX_SENSOR_PERIOD_ITEM_NAME    "X_AUX_SENSOR_PERIOD"

// AUX_WEATHER handles definition
#define AUX_WEATHER_PROPERTY              (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_SKY_BRIGHTNESS_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_SKY_TEMPERATURE_ITEM  (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_SKY_BORTLE_CLASS_ITEM (AUX_WEATHER_PROPERTY->items + 2)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_property *aux_info_property;
	indigo_property *aux_weather_property;
	indigo_timer *aux_timer;
	indigo_timer *aux_connection_handler_timer;
} sqm_private_data;

#pragma mark - Low level code

//+ code

static bool sqm_command(indigo_device *device, const char *command, char *response, int max) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0) {
			if (indigo_uni_read_section(PRIVATE_DATA->handle, response, max, "\n", "\r\n", INDIGO_DELAY(1)) > 0) {
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				return true;
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return false;
}

static bool sqm_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		char response[128];
		if (sqm_command(device, "ix", response, sizeof(response))) {
			if (response[0] == 'i' && response[1] == ',') {
				return true;
			}
		}
		indigo_send_message(device, "Handshake failed");
		return false;
	}
	return true;
}

static void sqm_close(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0) {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

//- code

#pragma mark - High level code (aux)

// aux state checking timer callback

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ aux.on_timer
	char response[128] = { 0 }, *pnt;
	if (sqm_command(device, "rx", response, sizeof(response))) {
		char *tok = strtok_r(response, ",", &pnt);
		if (tok == NULL) {
			AUX_INFO_PROPERTY->state = INDIGO_ALERT_STATE;
			AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (*tok == 'r') {
			AUX_WEATHER_SKY_BRIGHTNESS_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
			AUX_WEATHER_SKY_BORTLE_CLASS_ITEM->number.value = indigo_aux_sky_bortle(AUX_WEATHER_SKY_BRIGHTNESS_ITEM->number.value);
			X_AUX_SENSOR_FREQUENCY_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
			X_AUX_SENSOR_COUNTS_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
			X_AUX_SENSOR_PERIOD_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
			AUX_WEATHER_SKY_TEMPERATURE_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
			AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
			AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		AUX_INFO_PROPERTY->state = INDIGO_ALERT_STATE;
		AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	indigo_reschedule_timer(device, 10, &PRIVATE_DATA->aux_timer);
	//- aux.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// CONNECTION change handler

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = sqm_open(device);
		if (connection_result) {
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		sqm_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// aux attach API callback

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensor readings", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_AUX_SENSOR_FREQUENCY_ITEM, X_AUX_SENSOR_FREQUENCY_ITEM_NAME, "SQM sensor frequency [Hz]", 0, 1000000000, 0, 0);
		strcpy(X_AUX_SENSOR_FREQUENCY_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_AUX_SENSOR_COUNTS_ITEM, X_AUX_SENSOR_COUNTS_ITEM_NAME, "SQM sensor period [counts]", 0, 1000000000, 0, 0);
		strcpy(X_AUX_SENSOR_COUNTS_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_AUX_SENSOR_PERIOD_ITEM, X_AUX_SENSOR_PERIOD_ITEM_NAME, "SQM sensor period [sec]", 0, 1000000000, 0, 0);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Sky quality", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_SKY_BRIGHTNESS_ITEM, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness [m/arcsec\u00B2]", -20, 30, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sky temperature [\u00B0C]", -100, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_BORTLE_CLASS_ITEM, AUX_WEATHER_SKY_BORTLE_CLASS_ITEM_NAME, "Sky Bortle class", 1, 9, 0, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// aux enumerate API callback

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_INFO_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

// aux change property API callback

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_connection_handler, &PRIVATE_DATA->aux_connection_handler_timer);
		}
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

// aux detach API callback

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

// Unihedron SQM driver entry point

indigo_result indigo_aux_sqm(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static sqm_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(sqm_private_data));
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
