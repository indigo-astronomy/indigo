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

/** INDIGO Artesky Flat Box USB aux driver
 \file indigo_aux_arteskyflat.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME "indigo_aux_arteskyflat"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>
#include "indigo_aux_arteskyflat.h"

#define PRIVATE_DATA												((arteskyflat_private_data *)device->private_data)

#define AUX_LIGHT_SWITCH_PROPERTY          	(PRIVATE_DATA->light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM            (AUX_LIGHT_SWITCH_PROPERTY->items+0)
#define AUX_LIGHT_SWITCH_OFF_ITEM           (AUX_LIGHT_SWITCH_PROPERTY->items+1)

#define AUX_LIGHT_INTENSITY_PROPERTY				(PRIVATE_DATA->light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM						(AUX_LIGHT_INTENSITY_PROPERTY->items+0)

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *light_switch_property;
	indigo_property *light_intensity_property;
	pthread_mutex_t mutex;
} arteskyflat_private_data;

static bool artesky_command(indigo_device *device, char *command, char *response) {
	if (indigo_uni_discard(PRIVATE_DATA->handle, 1000) < 0) {
		return false;
	}
	if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) < 0) {
		return false;
	}
	if (indigo_uni_read_section(PRIVATE_DATA->handle, response, 10, "\n", "\r\n", 1000000) < 0) {
		return false;
	}
	return true;
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity", 0, 255, 1, 0);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyARTESKYFLAT");
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
		indigo_define_matching_property(AUX_LIGHT_SWITCH_PROPERTY);
		indigo_define_matching_property(AUX_LIGHT_INTENSITY_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_connection_handler(indigo_device *device) {
	char command[16], response[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		for (int i = 0; i < 2; i++) {
			PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
			if (PRIVATE_DATA->handle != NULL) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
				sprintf(command, ">B%03d", (int)(AUX_LIGHT_INTENSITY_ITEM->number.value));
				if (artesky_command(device, command, response) && *response == '*') {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Artesky Flat Box detected");
					AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
					break;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
					indigo_uni_close(&PRIVATE_DATA->handle);
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		artesky_command(device, ">D000", response);
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_switch_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16],	response[16];
	strcpy(command, AUX_LIGHT_SWITCH_ON_ITEM->sw.value ? ">L000" : ">D000");
	if (artesky_command(device, command, response) && *response == '*')
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	else
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_intensity_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16],	response[16];
	sprintf(command, ">B%03d", (int)(AUX_LIGHT_INTENSITY_ITEM->number.value));
	if (artesky_command(device, command, response))
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	else
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
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
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_SWITCH_PROPERTY, property, false);
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_switch_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_INTENSITY_PROPERTY, property, false);
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_intensity_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_arteskyflat(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static arteskyflat_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Artesky Flat Box",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	
	SET_DRIVER_INFO(info, "Artesky Flat Box USB", __FUNCTION__, DRIVER_VERSION, false, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(arteskyflat_private_data));
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
