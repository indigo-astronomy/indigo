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

/** INDIGO SkyAlert driver
 \file indigo_aux_skyalert.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_aux_skyalert"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_skyalert.h"

#define PRIVATE_DATA                               ((skyalert_private_data *)device->private_data)

#define AUX_WEATHER_PROPERTY                       (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM               (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM                  (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_RAIN_ITEM                      (AUX_WEATHER_PROPERTY->items + 2)
#define AUX_WEATHER_WIND_SPEED_ITEM                (AUX_WEATHER_PROPERTY->items + 3)
#define AUX_WEATHER_PRESSURE_ITEM                  (AUX_WEATHER_PROPERTY->items + 4)
#define AUX_WEATHER_SKY_TEMPERATURE_ITEM           (AUX_WEATHER_PROPERTY->items + 5)

#define AUX_INFO_PROPERTY                          (PRIVATE_DATA->info_property)
#define AUX_INFO_SKY_BRIGHTNESS_ITEM               (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_POWER_ITEM                        (AUX_INFO_PROPERTY->items + 1)


typedef struct {
	indigo_uni_handle *handle;
	indigo_property *weather_property;
	indigo_property *info_property;
	indigo_timer *timer_callback;
	pthread_mutex_t mutex;
} skyalert_private_data;


// -------------------------------------------------------------------------------- serial interface

static double skyalert_read_value(indigo_uni_handle *handle, char *response, int max) {
	if (indigo_uni_read_section(handle, response, sizeof(response), "\r", "\r", INDIGO_DELAY(1))) {
		return indigo_atod(response);
	}
	return -1;
}

// -------------------------------------------------------------------------------- async handlers

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	char response[32] = { 0 };
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && indigo_uni_printf(PRIVATE_DATA->handle, "send\r") > 0 && indigo_uni_read_section(PRIVATE_DATA->handle, response, sizeof(response), "\r", "\r", INDIGO_DELAY(1)) >= 0 && !strcmp(response, "Data")) {
		AUX_WEATHER_TEMPERATURE_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_WEATHER_SKY_TEMPERATURE_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_WEATHER_RAIN_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_INFO_SKY_BRIGHTNESS_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_WEATHER_HUMIDITY_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_WEATHER_WIND_SPEED_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_INFO_POWER_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		if (indigo_uni_read_section(PRIVATE_DATA->handle, response, sizeof(response), "\r", "\r", INDIGO_DELAY(1))) {
			indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, response);
		}
		AUX_WEATHER_PRESSURE_ITEM->number.value = skyalert_read_value(PRIVATE_DATA->handle, response, sizeof(response));
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
		AUX_INFO_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	if (PRIVATE_DATA->timer_callback) {
		indigo_reschedule_timer(device, 10, &PRIVATE_DATA->timer_callback);
	} else {
		indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->timer_callback);
	}
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle == NULL) {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_delete_property(device, INFO_PROPERTY, NULL);
			aux_timer_callback(device);
			indigo_define_property(device, INFO_PROPERTY, NULL);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer_callback);
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Interactive Astronomy SkyAlert");
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Info", "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_SKY_BRIGHTNESS_ITEM, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness [raw]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_POWER_ITEM, AUX_INFO_POWER_ITEM_NAME, "Power [1 = ok, 0 = failure]", 0, 1, 0, 0);
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Info", "Weather conditions", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [hPa]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_WIND_SPEED_ITEM, AUX_WEATHER_WIND_SPEED_ITEM_NAME, "Wind speed [raw]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_RAIN_ITEM, AUX_WEATHER_RAIN_ITEM_NAME, "Dampness [raw]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sky temperature [\u00B0C]", 0, 0, 0, 0);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
	if (DEVICE_PORTS_PROPERTY->count > 1) {
		/* 0 is refresh button */
		indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name);
	} else {
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
	}
#endif
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_INFO_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_skyalert(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static skyalert_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Interactive Astronomy SkyAlert",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);

	SET_DRIVER_INFO(info, "Interactive Astronomy SkyAlert", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(skyalert_private_data));
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
