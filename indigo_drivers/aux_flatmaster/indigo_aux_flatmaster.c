// Copyright (c) 2019-2025 Rumen G. Bogdanovski
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

// This file generated from indigo_aux_flatmaster.driver

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

#define DRIVER_VERSION       0x03000009
#define DRIVER_NAME          "indigo_aux_flatmaster"
#define DRIVER_LABEL         "PegasusAstro FlatMaster"
#define AUX_DEVICE_NAME      "PegasusAstro FlatMaster"
#define PRIVATE_DATA         ((flatmaster_private_data *)device->private_data)

//+ define

#define INTENSITY(val)       ((int)floor((100 - (int)(val) - 0) * (220 - 20) / (100 - 0) + 20))

//- define

#pragma mark - Property definitions

#define AUX_LIGHT_SWITCH_PROPERTY      (PRIVATE_DATA->aux_light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM       (AUX_LIGHT_SWITCH_PROPERTY->items + 0)
#define AUX_LIGHT_SWITCH_OFF_ITEM      (AUX_LIGHT_SWITCH_PROPERTY->items + 1)

#define AUX_LIGHT_INTENSITY_PROPERTY   (PRIVATE_DATA->aux_light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM       (AUX_LIGHT_INTENSITY_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_light_switch_property;
	indigo_property *aux_light_intensity_property;
	//+ data
	char response[12];
	//- data
} flatmaster_private_data;

#pragma mark - Low level code

//+ code

static bool flatmaster_command(indigo_device *device, char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
		if (result > 0) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool flatmaster_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (flatmaster_command(device, "#") && !strcmp("OK_FM", PRIVATE_DATA->response)) {
			if (flatmaster_command(device, "V")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DRIVER_LABEL);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void flatmaster_close(indigo_device *device) {
	flatmaster_command(device, "E:0");
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (aux)

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = flatmaster_open(device);
		if (connection_result) {
			//+ aux.on_connect
			/* FlatMaster does not report intensity and ON/OFF state, so we set it to be consistent */
			if (flatmaster_command(device, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value))) {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (flatmaster_command(device, "E:%d", AUX_LIGHT_SWITCH_ON_ITEM->sw.value)) {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			//- aux.on_connect
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		flatmaster_command(device, "L:%d", INTENSITY(0));
		flatmaster_command(device, "E:0");
		//- aux.on_disconnect
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		flatmaster_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_light_switch_handler(indigo_device *device) {
	AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_SWITCH.on_change
	char command[12];
	// if light is ON, set intensity, otherwise set it to 0
	if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
		sprintf(command, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value));
	} else {
		sprintf(command, "L:%d", INTENSITY(0));
	}
	if (flatmaster_command(device, command)) {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);

	if (!flatmaster_command(device, "E:%d", AUX_LIGHT_SWITCH_ON_ITEM->sw.value)) {
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_LIGHT_SWITCH.on_change
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
}

static void aux_light_intensity_handler(indigo_device *device) {
	AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_INTENSITY.on_change
	// if light is OFF, do not set intensity
	AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
		if (!flatmaster_command(device, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value))) {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- aux.AUX_LIGHT_INTENSITY.on_change
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
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
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_SWITCH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_INTENSITY_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_SWITCH_PROPERTY, aux_light_switch_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_INTENSITY_PROPERTY, aux_light_intensity_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_SWITCH_PROPERTY);
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

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
