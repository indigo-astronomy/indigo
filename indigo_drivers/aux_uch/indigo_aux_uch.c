// Copyright (c) 2023 Rumen G. Bogdanovski
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
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

/** INDIGO PegasusAstro USB Control Hub
 \file indigo_aux_uch.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_aux_uch"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_uch.h"

#define PRIVATE_DATA												((upb_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY						(PRIVATE_DATA->outlet_names_property)
#define AUX_USB_PORT_NAME_1_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_USB_PORT_NAME_2_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_USB_PORT_NAME_3_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_USB_PORT_NAME_4_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_USB_PORT_NAME_5_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_USB_PORT_NAME_6_ITEM						(AUX_OUTLET_NAMES_PROPERTY->items + 5)

#define AUX_USB_PORT_PROPERTY								(PRIVATE_DATA->usb_port_property)
#define AUX_USB_PORT_1_ITEM									(AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM									(AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM									(AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM									(AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM									(AUX_USB_PORT_PROPERTY->items + 4)
#define AUX_USB_PORT_6_ITEM									(AUX_USB_PORT_PROPERTY->items + 5)

#define AUX_INFO_PROPERTY									(PRIVATE_DATA->info_property)
#define AUX_INFO_VOLTAGE_ITEM								(AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_UPTIME_ITEM								(AUX_INFO_PROPERTY->items + 1)
#define AUX_INFO_UPTIME_ITEM_NAME							"UPTIME"

#define X_AUX_REBOOT_PROPERTY								(PRIVATE_DATA->reboot_property)
#define X_AUX_REBOOT_ITEM									(X_AUX_REBOOT_PROPERTY->items + 0)

#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY			(PRIVATE_DATA->save_defaults_property)
#define AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM				(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY->items + 0)

#define AUX_GROUP											"USB Ports"

typedef struct {
	indigo_uni_handle *handle;
	indigo_timer *aux_timer;
	indigo_property *outlet_names_property;
	indigo_property *usb_port_property;
	indigo_property *info_property;
	indigo_property *save_defaults_property;
	indigo_property *reboot_property;
	int count;
	pthread_mutex_t mutex;
} upb_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool uch_command(indigo_device *device, char *command, char *response, int max) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) > 0) {
			if (indigo_uni_read_section(PRIVATE_DATA->handle, response, max, "\n", "\r\n", INDIGO_DELAY(1)) > 0) {
				return true;
			}
		}
	}
	return false;
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- OUTLET_NAMES
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
		// -------------------------------------------------------------------------------- USB PORTS
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
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		indigo_init_sexagesimal_number_item(AUX_INFO_UPTIME_ITEM, AUX_INFO_UPTIME_ITEM_NAME, "Uptime [hours]", 0, 1e10, 0, 0);
		// -------------------------------------------------------------------------------- Device specific
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_AUX_REBOOT", AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, "REBOOT", "Reboot", false);
		// --------------------------------------------------------------------------------
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY = indigo_init_switch_property(NULL, device->name, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY_NAME, AUX_GROUP, "Save current outlet states as default", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM_NAME, "Save", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
/*
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/Pegasus_UCH");
#endif
*/
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_USB_PORT_PROPERTY);
		indigo_define_matching_property(AUX_INFO_PROPERTY);
		indigo_define_matching_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY);
		indigo_define_matching_property(X_AUX_REBOOT_PROPERTY);
	}
	indigo_define_matching_property(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	char response[128];
	bool updateInfo = false;
	bool updateUSBPorts = false;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (uch_command(device, "PA", response, sizeof(response))) {
		char *pnt = NULL, *token = strtok_r(response, ":", &pnt);
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

	if (uch_command(device, "PC", response, sizeof(response))) {
		char *pnt = NULL, *token = strtok_r(response, ":", &pnt);
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
	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	char response[128];
	indigo_lock_master_device(device);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0) {
			PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
			if (PRIVATE_DATA->handle != NULL) {
				bool connected = false;
				int attempt = 0;
				while (!connected) {
					if (uch_command(device, "P#", response, sizeof(response))) {
						if (!strcmp(response, "UCH_OK")) {
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to UCH %s", DEVICE_PORT_ITEM->text.value);
							break;
						}
					}
					if (attempt++ == 3) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "UCH not detected");
						indigo_uni_close(&PRIVATE_DATA->handle);
						break;
					}
					indigo_sleep(1);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "UCH not detected - retrying...");
				}
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			if (uch_command(device, "PA", response, sizeof(response)) && !strncmp(response, "UCH", 3)) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
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
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'PA' response");
					indigo_uni_close(&PRIVATE_DATA->handle);
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'PA' response");
				indigo_uni_close(&PRIVATE_DATA->handle);
			}

			if (uch_command(device, "PV", response, sizeof(response))) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "USB Control Hub");
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 3); // remove "PV:" prefix
				indigo_update_property(device, INFO_PROPERTY, NULL);
			}
			
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);

			uch_command(device, "PL:1", response, sizeof(response));
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);

		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY, NULL);
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			if (PRIVATE_DATA->handle != NULL) {
				uch_command(device, "PL:0", response, sizeof(response));
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
				indigo_uni_close(&PRIVATE_DATA->handle);
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_unlock_master_device(device);
}

static void aux_usb_port_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[128];
	for (int i = 0; i < AUX_USB_PORT_PROPERTY->count; i++) {
		sprintf(command, "U%d:%d", i + 1, AUX_USB_PORT_PROPERTY->items[i].sw.value ? 1 : 0);
		uch_command(device, command, response, sizeof(response));
	}
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_save_defaults_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value) {
		char command[] = "PE:000000";
		char *port_mask = command + 3;
		for (int i = 0; i < AUX_USB_PORT_PROPERTY->count; i++) {
			port_mask[i] = AUX_USB_PORT_PROPERTY->items[i].sw.value ? '1' : '0';
		}
		uch_command(device, command, response, sizeof(response));
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_ITEM->sw.value = false;
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_reboot_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_AUX_REBOOT_ITEM->sw.value) {
		uch_command(device, "PF", response, sizeof(response));
		X_AUX_REBOOT_ITEM->sw.value = false;
		X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
	}
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
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		snprintf(AUX_USB_PORT_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
		snprintf(AUX_USB_PORT_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
		snprintf(AUX_USB_PORT_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_3_ITEM->text.value);
		snprintf(AUX_USB_PORT_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_4_ITEM->text.value);
		snprintf(AUX_USB_PORT_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_5_ITEM->text.value);
		snprintf(AUX_USB_PORT_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_6_ITEM->text.value);
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_USB_PORT
		indigo_property_copy_values(AUX_USB_PORT_PROPERTY, property, false);
		AUX_USB_PORT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_usb_port_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_SAVE_DEFAULTS
		indigo_property_copy_values(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY, property, false);
		AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY, NULL);
		indigo_set_timer(device, 0, aux_save_defaults_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_REBOOT
		indigo_property_copy_values(X_AUX_REBOOT_PROPERTY, property, false);
		X_AUX_REBOOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_reboot_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_USB_PORT_PROPERTY);
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
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_SAVE_OUTLET_STATES_AS_DEFAULT_PROEPRTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation
indigo_result indigo_aux_uch(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static upb_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"USB Control Hub",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	static indigo_device_match_pattern patterns[1] = {0};
	strcpy(patterns[0].vendor_string, "Pegasus Astro");
	strcpy(patterns[0].product_string, "USB Control Hub");
	INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);

	SET_DRIVER_INFO(info, "PegasusAstro USB Control Hub", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(upb_private_data));
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
