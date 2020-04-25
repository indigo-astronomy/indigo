// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO SQM driver
 \file indigo_aux_sqm.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_aux_sqm"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_aux_sqm.h"

#define PRIVATE_DATA												((sqm_private_data *)device->private_data)

#define AUX_INFO_PROPERTY										(PRIVATE_DATA->info_property)
#define X_AUX_SKY_BRIGHTNESS_ITEM						(AUX_INFO_PROPERTY->items + 0)
#define X_AUX_SENSOR_FREQUENCY_ITEM					(AUX_INFO_PROPERTY->items + 1)
#define X_AUX_SENSOR_COUNTS_ITEM						(AUX_INFO_PROPERTY->items + 2)
#define X_AUX_SENSOR_PERIOD_ITEM						(AUX_INFO_PROPERTY->items + 3)
#define X_AUX_SKY_TEMPERATURE_ITEM					(AUX_INFO_PROPERTY->items + 4)

typedef struct {
	int handle;
	indigo_property *info_property;
	indigo_timer *timer_callback;
	pthread_mutex_t mutex;
} sqm_private_data;

// -------------------------------------------------------------------------------- INDIGO aux device implementation


static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Sky quality", "Sky quality", INDIGO_OK_STATE, INDIGO_RO_PERM, 5);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_AUX_SKY_BRIGHTNESS_ITEM, "X_AUX_SKY_BRIGHTNESS", "Sky brightness [m/arcsec\u00B2]", -20, 30, 0, 0);
		indigo_init_number_item(X_AUX_SENSOR_FREQUENCY_ITEM, "X_AUX_SENSOR_FREQUENCY", "SQM sensor frequency [Hz]", 0, 1000000000, 0, 0);
		strcpy(X_AUX_SENSOR_FREQUENCY_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_AUX_SENSOR_COUNTS_ITEM, "X_AUX_SENSOR_COUNTS", "SQM sensor period [counts]", 0, 1000000000, 0, 0);
		strcpy(X_AUX_SENSOR_COUNTS_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_AUX_SENSOR_PERIOD_ITEM, "X_AUX_SENSOR_PERIOD", "SQM sensor period [sec]", 0, 1000000000, 0, 0);
		indigo_init_number_item(X_AUX_SKY_TEMPERATURE_ITEM, "X_AUX_SKY_TEMPERATURE", "Sky temperature [\u00B0C]", -100, 100, 0, 0);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
	if (DEVICE_PORTS_PROPERTY->count > 1) {
		/* 0 is refresh button */
		strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name, INDIGO_VALUE_SIZE);
	} else {
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
	}
#endif
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_INFO_PROPERTY, property))
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	char buffer[60], *pnt;
	memset(buffer, 0, sizeof(buffer));
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	indigo_printf(PRIVATE_DATA->handle, "rx");
	indigo_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", buffer);
	if (*strtok_r(buffer, ",", &pnt) != 'r') {
		AUX_INFO_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		X_AUX_SKY_BRIGHTNESS_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
		X_AUX_SENSOR_FREQUENCY_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
		X_AUX_SENSOR_COUNTS_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
		X_AUX_SENSOR_PERIOD_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
		X_AUX_SKY_TEMPERATURE_ITEM->number.value = indigo_atod(strtok_r(NULL, ",", &pnt));
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	indigo_reschedule_timer(device, 10, &PRIVATE_DATA->timer_callback);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200);
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
			char buffer[60];
			indigo_printf(PRIVATE_DATA->handle, "ix");
			indigo_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer));
			if (*buffer == 'i') {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unit info: %s", buffer);
			} else {
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->timer_callback = indigo_set_timer(device, 0, aux_timer_callback);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer_callback);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_connection_handler);
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
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_sqm(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static sqm_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Unihedron SQM",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);

	SET_DRIVER_INFO(info, "Unihedron SQM", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(sqm_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(sqm_private_data));
			aux = malloc(sizeof(indigo_device));
			assert(aux != NULL);
			memcpy(aux, &aux_template, sizeof(indigo_device));
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
