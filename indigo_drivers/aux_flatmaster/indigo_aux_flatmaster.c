// Copyright (c) 2019-2025 Rumen G.Bogdanovski
// All rights reserved.

// You can use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_aux_flatmaster.driver

// version history
// 3.0 by Rumen G.Bogdanovski

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_flatmaster.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_aux_flatmaster"
#define DRIVER_LABEL         "PegasusAstro FlatMaster"
#define AUX_DEVICE_NAME      "Pegasus FlatMaster"
#define PRIVATE_DATA         ((flatmaster_private_data *)device->private_data)

//+ "define" custom code below

#define INTENSITY(val)       ((int)floor((100 - (int)(val) - 0) * (220 - 20) / (100 - 0) + 20))

//- "define" custom code above

#pragma mark - Property definitions

// AUX_LIGHT_SWITCH handles definition
#define AUX_LIGHT_SWITCH_PROPERTY      (PRIVATE_DATA->aux_light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM       (AUX_LIGHT_SWITCH_PROPERTY->items + 0)
#define AUX_LIGHT_SWITCH_OFF_ITEM      (AUX_LIGHT_SWITCH_PROPERTY->items + 1)

// AUX_LIGHT_INTENSITY handles definition
#define AUX_LIGHT_INTENSITY_PROPERTY   (PRIVATE_DATA->aux_light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM       (AUX_LIGHT_INTENSITY_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_property *aux_light_switch_property;
	indigo_property *aux_light_intensity_property;
	indigo_timer *aux_connection_handler_timer;
	indigo_timer *aux_light_switch_handler_timer;
	indigo_timer *aux_light_intensity_handler_timer;
} flatmaster_private_data;

#pragma mark - Low level code

//+ "code" custom code below

static bool flatmaster_command(indigo_uni_handle *handle, char *command, char *response, int resp_len) {
	if (indigo_uni_discard(handle) >= 0) {
		if (indigo_uni_printf(handle, "%s\n", command) > 0) {
			if (indigo_uni_read_section(handle, response, resp_len, "\n", "\r\n", INDIGO_DELAY(1)) > 0) {
				return true;
			}
		}
	}
	return false;
}

static bool flatmaster_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		char response[16];
		if (flatmaster_command(PRIVATE_DATA->handle, "#", response, sizeof(response)) && !strcmp("OK_FM", response)) {
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void flatmaster_close(indigo_device *device) {
	char response[16];
	flatmaster_command(PRIVATE_DATA->handle, "E:0", response, sizeof(response));
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- "code" custom code above

#pragma mark - High level code (aux)

// CONNECTION change handler

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = flatmaster_open(device);
		if (connection_result) {
			//+ "aux.on_connect" custom code below
			char command[16], response[16];
			if (flatmaster_command(PRIVATE_DATA->handle, "V", response, sizeof(response))) {
				snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%s", response);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			}
			/* FlatMaster does not report intensity and ON/OFF state, so we set it to be consistent */
			sprintf(command, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value));
			if (flatmaster_command(PRIVATE_DATA->handle, command, response, sizeof(response))) {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			sprintf(command, "E:%d", AUX_LIGHT_SWITCH_ON_ITEM->sw.value);
			if (flatmaster_command(PRIVATE_DATA->handle, command, response, sizeof(response))) {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			//- "aux.on_connect" custom code above
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_light_switch_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_light_intensity_handler_timer);
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		flatmaster_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// AUX_LIGHT_SWITCH change handler

static void aux_light_switch_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	//+ "aux.AUX_LIGHT_SWITCH.on_change" custom code below
	char command[16],	response[16];
	sprintf(command, "E:%d", AUX_LIGHT_SWITCH_ON_ITEM->sw.value);
	if (!flatmaster_command(PRIVATE_DATA->handle, command, response, sizeof(response))) {
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- "aux.AUX_LIGHT_SWITCH.on_change" custom code above
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_LIGHT_INTENSITY change handler

static void aux_light_intensity_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ "aux.AUX_LIGHT_INTENSITY.on_change" custom code below
	char command[16],	response[16];
	sprintf(command, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value));
	if (!flatmaster_command(PRIVATE_DATA->handle, command, response, sizeof(response))) {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- "aux.AUX_LIGHT_INTENSITY.on_change" custom code above
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// aux attach API callback

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ "aux.on_attach" custom code below
		INFO_PROPERTY->count = 6;
		//- "aux.on_attach" custom code above
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity (%)", 0, 100, 1, 50);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// aux enumerate API callback

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_LIGHT_SWITCH_PROPERTY);
		indigo_define_matching_property(AUX_LIGHT_INTENSITY_PROPERTY);
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
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_light_switch_handler_timer == NULL) {
			indigo_property_copy_values(AUX_LIGHT_SWITCH_PROPERTY, property, false);
			AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_light_switch_handler, &PRIVATE_DATA->aux_light_switch_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_light_intensity_handler_timer == NULL) {
			indigo_property_copy_values(AUX_LIGHT_INTENSITY_PROPERTY, property, false);
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_light_intensity_handler, &PRIVATE_DATA->aux_light_intensity_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_SWITCH_PROPERTY);
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

// aux detach API callback

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

// PegasusAstro FlatMaster driver entry point

indigo_result indigo_aux_flatmaster(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static flatmaster_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			patterns[0].vendor_id = 0x0403;
			patterns[0].product_id = 0x6015;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(flatmaster_private_data));
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
