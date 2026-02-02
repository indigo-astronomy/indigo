// Copyright (c) 2023-2025 Rumen G. Bogdanovski
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

// This file generated from indigo_aux_uch.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_uch.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_aux_uch"
#define DRIVER_LABEL         "PegasusAstro USB Control Hub"
#define AUX_DEVICE_NAME      "USB Control Hub"
#define PRIVATE_DATA         ((uch_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "USB Ports"

//- define

#pragma mark - Property definitions

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_USB_PORT_NAME_1_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_USB_PORT_NAME_2_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_USB_PORT_NAME_3_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_USB_PORT_NAME_4_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_USB_PORT_NAME_5_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_USB_PORT_NAME_6_ITEM       (AUX_OUTLET_NAMES_PROPERTY->items + 5)

#define AUX_USB_PORT_PROPERTY          (PRIVATE_DATA->aux_usb_port_property)
#define AUX_USB_PORT_1_ITEM            (AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM            (AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM            (AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM            (AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM            (AUX_USB_PORT_PROPERTY->items + 4)
#define AUX_USB_PORT_6_ITEM            (AUX_USB_PORT_PROPERTY->items + 5)

#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_UPTIME_ITEM           (AUX_INFO_PROPERTY->items + 1)

#define AUX_INFO_UPTIME_ITEM_NAME      "UPTIME"

#define X_AUX_REBOOT_PROPERTY          (PRIVATE_DATA->x_aux_reboot_property)
#define X_AUX_REBOOT_ITEM              (X_AUX_REBOOT_PROPERTY->items + 0)

#define X_AUX_REBOOT_PROPERTY_NAME     "X_AUX_REBOOT"
#define X_AUX_REBOOT_ITEM_NAME         "REBOOT"

#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY (PRIVATE_DATA->aux_save_outlet_states_as_default_property)
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM     (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_usb_port_property;
	indigo_property *aux_info_property;
	indigo_property *x_aux_reboot_property;
	indigo_property *aux_save_outlet_states_as_default_property;
	//+ data
	char response[128];
	//- data
} uch_private_data;

#pragma mark - Low level code

//+ code

static bool uch_command(indigo_device *device, char *command, ...) {
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

static bool uch_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (uch_command(device, "P#")) {
			if (!strcmp(PRIVATE_DATA->response, "UCH_OK")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DRIVER_LABEL);
				if (uch_command(device, "PV")) {
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response + 3);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					uch_command(device, "PL:1");
					return true;
				}
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void uch_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	uch_command(device, "PL:0");
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	bool updateInfo = false;
	bool updateUSBPorts = false;
	if (uch_command(device, "PA")) {
		char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
		if ((token = strtok_r(NULL, ":", &pnt))) { // Voltage
			double value = indigo_atod(token);
			if (AUX_INFO_VOLTAGE_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_VOLTAGE_ITEM->number.value = value;
			}
		}	
		if ((token = strtok_r(NULL, ":", &pnt))) { // USB status
			bool state = token[0] == '1';
			if (AUX_USB_PORT_1_ITEM->sw.value != state) {
				AUX_USB_PORT_1_ITEM->sw.value = state;
				updateUSBPorts = true;
			}
			state = token[1] == '1';
			if (AUX_USB_PORT_2_ITEM->sw.value != state) {
				AUX_USB_PORT_2_ITEM->sw.value = state;
				updateUSBPorts = true;
			}
			state = token[2] == '1';
			if (AUX_USB_PORT_3_ITEM->sw.value != state) {
				AUX_USB_PORT_3_ITEM->sw.value = state;
				updateUSBPorts = true;
			}
			state = token[3] == '1';
			if (AUX_USB_PORT_4_ITEM->sw.value != state) {
				AUX_USB_PORT_4_ITEM->sw.value = state;
				updateUSBPorts = true;
			}
			state = token[4] == '1';
			if (AUX_USB_PORT_5_ITEM->sw.value != state) {
				AUX_USB_PORT_5_ITEM->sw.value = state;
				updateUSBPorts = true;
			}
			state = token[5] == '1';
			if (AUX_USB_PORT_6_ITEM->sw.value != state) {
				AUX_USB_PORT_6_ITEM->sw.value = state;
				updateUSBPorts = true;
			}
		}
	}
	if (uch_command(device, "PC")) {
		char *pnt = NULL, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
		if ((token = strtok_r(NULL, ":", &pnt))) {
			double value = indigo_atod(token)/(1000*3600); // convert to seconds;
			if (AUX_INFO_UPTIME_ITEM->number.value != value) {
				updateInfo = true;
				AUX_INFO_UPTIME_ITEM->number.value = value;
			}
		}
	}
	if (updateInfo) {
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	}
	if (updateUSBPorts) {
		AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 2, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = uch_open(device);
		if (connection_result) {
			//+ aux.on_connect
			if (uch_command(device, "PA") && !strncmp(PRIVATE_DATA->response, "UCH", 3)) {
				char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
				if ((token = strtok_r(NULL, ":", &pnt))) { // Voltage
					AUX_INFO_VOLTAGE_ITEM->number.value = indigo_atod(token);
				}	
				if ((token = strtok_r(NULL, ":", &pnt))) { // USB status
					AUX_USB_PORT_1_ITEM->sw.value =  token[0] == '1';
					AUX_USB_PORT_2_ITEM->sw.value =  token[1] == '1';
					AUX_USB_PORT_3_ITEM->sw.value =  token[2] == '1';
					AUX_USB_PORT_4_ITEM->sw.value =  token[3] == '1';
					AUX_USB_PORT_5_ITEM->sw.value =  token[4] == '1';
					AUX_USB_PORT_6_ITEM->sw.value =  token[5] == '1';
				}
			}
			//- aux.on_connect
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			indigo_define_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
		uch_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	snprintf(AUX_USB_PORT_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
	snprintf(AUX_USB_PORT_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
	snprintf(AUX_USB_PORT_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_3_ITEM->text.value);
	snprintf(AUX_USB_PORT_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_4_ITEM->text.value);
	snprintf(AUX_USB_PORT_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_5_ITEM->text.value);
	snprintf(AUX_USB_PORT_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_usb_port_handler(indigo_device *device) {
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_USB_PORT.on_change
	for (int i = 0; i < AUX_USB_PORT_PROPERTY->count; i++) {
		uch_command(device, "U%d:%d", i + 1, AUX_USB_PORT_PROPERTY->items[i].sw.value ? 1 : 0);
	}
	//- aux.AUX_USB_PORT.on_change
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
}

static void aux_x_aux_reboot_handler(indigo_device *device) {
	X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_REBOOT.on_change
	if (X_AUX_REBOOT_ITEM->sw.value) {
		uch_command(device, "PF");
		X_AUX_REBOOT_ITEM->sw.value = false;
	}
	//- aux.X_AUX_REBOOT.on_change
	indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
}

static void aux_save_outlet_states_as_default_handler(indigo_device *device) {
	AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SAVE_OUTLET_STATES_AS_DEFAULT.on_change
	if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value) {
		char command[] = "PE:000000";
		char *port_mask = command + 3;
		for (int i = 0; i < AUX_USB_PORT_PROPERTY->count; i++) {
			port_mask[i] = AUX_USB_PORT_PROPERTY->items[i].sw.value ? '1' : '0';
		}
		uch_command(device, command);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value = false;
	}
	//- aux.AUX_SAVE_OUTLET_STATES_AS_DEFAULT.on_change
	indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_USB_PORT_NAME_1_ITEM, AUX_USB_PORT_NAME_1_ITEM_NAME, "Port #1", "Port #1");
		indigo_init_text_item(AUX_USB_PORT_NAME_2_ITEM, AUX_USB_PORT_NAME_2_ITEM_NAME, "Port #2", "Port #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_3_ITEM, AUX_USB_PORT_NAME_3_ITEM_NAME, "Port #3", "Port #3");
		indigo_init_text_item(AUX_USB_PORT_NAME_4_ITEM, AUX_USB_PORT_NAME_4_ITEM_NAME, "Port #4", "Port #4");
		indigo_init_text_item(AUX_USB_PORT_NAME_5_ITEM, AUX_USB_PORT_NAME_5_ITEM_NAME, "Port #5", "Port #5");
		indigo_init_text_item(AUX_USB_PORT_NAME_6_ITEM, AUX_USB_PORT_NAME_6_ITEM_NAME, "Port #6", "Port #6");
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 6);
		if (AUX_USB_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "Port #1", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "Port #2", true);
		indigo_init_switch_item(AUX_USB_PORT_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "Port #3", true);
		indigo_init_switch_item(AUX_USB_PORT_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "Port #4", true);
		indigo_init_switch_item(AUX_USB_PORT_5_ITEM, AUX_USB_PORT_5_ITEM_NAME, "Port #5", true);
		indigo_init_switch_item(AUX_USB_PORT_6_ITEM, AUX_USB_PORT_6_ITEM_NAME, "Port #6", true);
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_INFO_UPTIME_ITEM, AUX_INFO_UPTIME_ITEM_NAME, "Uptime [hours]", 0, 0, 0, 0);
		strcpy(AUX_INFO_UPTIME_ITEM->number.format, "%12.9m");
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_REBOOT_PROPERTY_NAME, AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, X_AUX_REBOOT_ITEM_NAME, "Reboot", false);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY_NAME, AUX_GROUP, "Save current outlet states as default", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM_NAME, "Save", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_USB_PORT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_REBOOT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
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
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_USB_PORT_PROPERTY, aux_usb_port_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_REBOOT_PROPERTY, aux_x_aux_reboot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY, aux_save_outlet_states_as_default_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_USB_PORT_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_uch(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static uch_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "USB Control Hub");
			patterns[0].vendor_id = 0x0403;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(uch_private_data));
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
