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

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_sqm"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "indigo_driver_xml.h"
#include "indigo_io.h"
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
} sqm_private_data;

// -------------------------------------------------------------------------------- INDIGO aux device implementation


static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	char buffer[58];
	indigo_printf(PRIVATE_DATA->handle, "rx");
	indigo_read(PRIVATE_DATA->handle, buffer, 57);
	
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer_callback);
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION, 0) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Sky quality", "Sky quality", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_AUX_SKY_BRIGHTNESS_ITEM, "X_AUX_SKY_BRIGHTNESS", "Quality [mag/arcsec^2]", -20, 30, 0, 0);
		indigo_init_number_item(X_AUX_SENSOR_FREQUENCY_ITEM, "X_AUX_SENSOR_FREQUENCY", "Frequence [Hz]", 0, 100000, 0, 0);
		indigo_init_number_item(X_AUX_SENSOR_COUNTS_ITEM, "X_AUX_SENSOR_COUNTS", "Period", 0, 100000, 0, 0);
		indigo_init_number_item(X_AUX_SENSOR_PERIOD_ITEM, "X_AUX_SENSOR_PERIOD", "Period [s]", 0, 100000, 0, 0);
		indigo_init_number_item(X_AUX_SKY_TEMPERATURE_ITEM, "X_AUX_SKY_TEMPERATURE", "Temperature [X]", -100, 100, 0, 0);
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_aux");
#endif
		// --------------------------------------------------------------------------------
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

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200);
			if (PRIVATE_DATA->handle > 0) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
				
					// TBD
				
				indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->timer_callback = indigo_set_timer(device, 1, aux_timer_callback);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_cancel_timer(device, &PRIVATE_DATA->timer_callback);
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(AUX_INFO_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

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
