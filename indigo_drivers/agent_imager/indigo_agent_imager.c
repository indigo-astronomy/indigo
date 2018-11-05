// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO Imager agent
 \file indigo_agent_imager.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_imager"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_agent_imager.h"

#define DEVICE_PRIVATE_DATA									((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA									((agent_private_data *)client->client_context)

#define AGENT_CCD_PROPERTY									(DEVICE_PRIVATE_DATA->agent_ccd_property)
#define AGENT_WHEEL_PROPERTY								(DEVICE_PRIVATE_DATA->agent_wheel_property)
#define AGENT_FOCUSER_PROPERTY							(DEVICE_PRIVATE_DATA->agent_focuser_property)

typedef struct {
	indigo_device *device;
	indigo_client *client;
	indigo_property *agent_ccd_property;
	indigo_property *agent_wheel_property;
	indigo_property *agent_focuser_property;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		AGENT_CCD_PROPERTY = indigo_init_switch_property(NULL, device->name, "AGENT_CCD", "Devices", "Imaging cameras", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AGENT_CCD_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_CCD_PROPERTY->items, "NONE", "None", true);
		AGENT_WHEEL_PROPERTY = indigo_init_switch_property(NULL, device->name, "AGENT_WHEEL", "Devices", "Filter wheels", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AGENT_WHEEL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_WHEEL_PROPERTY->items, "NONE", "None", true);
		AGENT_FOCUSER_PROPERTY = indigo_init_switch_property(NULL, device->name, "AGENT_FOCUSER", "Devices", "Focusers", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (AGENT_FOCUSER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_FOCUSER_PROPERTY->items, "NONE", "None", true);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_device_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client!= NULL && client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_CCD_PROPERTY, property))
		indigo_define_property(device, AGENT_CCD_PROPERTY, NULL);
	if (indigo_property_match(AGENT_WHEEL_PROPERTY, property))
		indigo_define_property(device, AGENT_WHEEL_PROPERTY, NULL);
	if (indigo_property_match(AGENT_FOCUSER_PROPERTY, property))
		indigo_define_property(device, AGENT_FOCUSER_PROPERTY, NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_CCD_PROPERTY, property)) {
		indigo_property *ccd_connection = indigo_init_switch_property(NULL, "*", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		indigo_init_switch_item(ccd_connection->items + 0, CONNECTION_CONNECTED_ITEM_NAME, NULL, false);
		indigo_init_switch_item(ccd_connection->items + 1, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
		for (int i = 1; i < AGENT_CCD_PROPERTY->count; i++) {
			if (AGENT_CCD_PROPERTY->items[i].sw.value) {
				AGENT_CCD_PROPERTY->items[i].sw.value = false;
				AGENT_CCD_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_CCD_PROPERTY, NULL);
				strncpy(ccd_connection->device, AGENT_CCD_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, ccd_connection);
				break;
			}
		}
		indigo_property_copy_values(AGENT_CCD_PROPERTY, property, false);
		for (int i = 1; i < AGENT_CCD_PROPERTY->count; i++) {
			if (AGENT_CCD_PROPERTY->items[i].sw.value) {
				indigo_set_switch(ccd_connection, ccd_connection->items, true);
				AGENT_CCD_PROPERTY->state = INDIGO_BUSY_STATE;
				strncpy(ccd_connection->device, AGENT_CCD_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_update_property(device, AGENT_CCD_PROPERTY, NULL);
				indigo_change_property(client, ccd_connection);
				break;
			}
		}
	} else if (indigo_property_match(AGENT_WHEEL_PROPERTY, property)) {
		indigo_property *wheel_connection = indigo_init_switch_property(NULL, "*", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		indigo_init_switch_item(wheel_connection->items + 0, CONNECTION_CONNECTED_ITEM_NAME, NULL, false);
		indigo_init_switch_item(wheel_connection->items + 1, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
		for (int i = 1; i < AGENT_WHEEL_PROPERTY->count; i++) {
			if (AGENT_WHEEL_PROPERTY->items[i].sw.value) {
				AGENT_WHEEL_PROPERTY->items[i].sw.value = false;
				AGENT_WHEEL_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_WHEEL_PROPERTY, NULL);
				strncpy(wheel_connection->device, AGENT_WHEEL_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, wheel_connection);
				break;
			}
		}
		indigo_property_copy_values(AGENT_WHEEL_PROPERTY, property, false);
		for (int i = 1; i < AGENT_WHEEL_PROPERTY->count; i++) {
			if (AGENT_WHEEL_PROPERTY->items[i].sw.value) {
				indigo_set_switch(wheel_connection, wheel_connection->items, true);
				AGENT_WHEEL_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_WHEEL_PROPERTY, NULL);
				strncpy(wheel_connection->device, AGENT_WHEEL_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, wheel_connection);
				break;
			}
		}
	} else if (indigo_property_match(AGENT_FOCUSER_PROPERTY, property)) {
		indigo_property *focuser_connection = indigo_init_switch_property(NULL, "*", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		indigo_init_switch_item(focuser_connection->items + 0, CONNECTION_CONNECTED_ITEM_NAME, NULL, false);
		indigo_init_switch_item(focuser_connection->items + 1, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
		for (int i = 1; i < AGENT_FOCUSER_PROPERTY->count; i++) {
			if (AGENT_FOCUSER_PROPERTY->items[i].sw.value) {
				AGENT_FOCUSER_PROPERTY->items[i].sw.value = false;
				AGENT_FOCUSER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_FOCUSER_PROPERTY, NULL);
				strncpy(focuser_connection->device, AGENT_FOCUSER_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, focuser_connection);
				break;
			}
		}
		indigo_property_copy_values(AGENT_FOCUSER_PROPERTY, property, false);
		for (int i = 1; i < AGENT_FOCUSER_PROPERTY->count; i++) {
			if (AGENT_FOCUSER_PROPERTY->items[i].sw.value) {
				indigo_set_switch(focuser_connection, focuser_connection->items, true);
				AGENT_FOCUSER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_FOCUSER_PROPERTY, NULL);
				strncpy(focuser_connection->device, AGENT_FOCUSER_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, focuser_connection);
				break;
			}
		}
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_CCD_PROPERTY);
	indigo_release_property(AGENT_WHEEL_PROPERTY);
	indigo_release_property(AGENT_FOCUSER_PROPERTY);
	return indigo_agent_detach(device);
}

static indigo_result agent_client_attach(indigo_client *client) {
	return INDIGO_OK;
}

static indigo_result agent_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_item *interface = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
		indigo_property *device_list;
		if (interface) {
			int mask = atoi(interface->text.value);
			if (mask & INDIGO_INTERFACE_CCD) {
				bool found = false;
				device_list = CLIENT_PRIVATE_DATA->agent_ccd_property;
				for (int i = 1; i < device_list->count; i++) {
					if (!strcmp(property->device, device_list->items[i].name)) {
						found = true;
						break;
					}
				}
				if (!found) {
					int index = device_list->count;
					device_list = indigo_resize_property(device_list, index + 1);
					indigo_init_switch_item(device_list->items + index, property->device, property->device, false);
					indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					CLIENT_PRIVATE_DATA->agent_ccd_property = device_list;
				}
			}
			if (mask & INDIGO_INTERFACE_WHEEL) {
				bool found = false;
				device_list = CLIENT_PRIVATE_DATA->agent_wheel_property;
				for (int i = 1; i < device_list->count; i++) {
					if (!strcmp(property->device, device_list->items[i].name)) {
						found = true;
						break;
					}
				}
				if (!found) {
					int index = device_list->count;
					device_list = indigo_resize_property(device_list, index + 1);
					indigo_init_switch_item(device_list->items + index, property->device, property->device, false);
					indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					CLIENT_PRIVATE_DATA->agent_wheel_property = device_list;
				}
			}
			if (mask & INDIGO_INTERFACE_FOCUSER) {
				bool found = false;
				device_list = CLIENT_PRIVATE_DATA->agent_focuser_property;
				for (int i = 1; i < device_list->count; i++) {
					if (!strcmp(property->device, device_list->items[i].name)) {
						found = true;
						break;
					}
				}
				if (!found) {
					int index = device_list->count;
					device_list = indigo_resize_property(device_list, index + 1);
					indigo_init_switch_item(device_list->items + index, property->device, property->device, false);
					indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					CLIENT_PRIVATE_DATA->agent_focuser_property = device_list;
				}
			}
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (property->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property *device_list;
		indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
		device_list = CLIENT_PRIVATE_DATA->agent_ccd_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name) && device_list->items[i].sw.value) {
				if (device_list->state == INDIGO_BUSY_STATE) {
					if (property->state == INDIGO_ALERT_STATE) {
						device_list->state = INDIGO_ALERT_STATE;
					} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
						device_list->state = INDIGO_OK_STATE;
					}
					indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					return INDIGO_OK;
				} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
					device_list->state = INDIGO_ALERT_STATE;
					indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					return INDIGO_OK;
				}
			}
		}
		device_list = CLIENT_PRIVATE_DATA->agent_wheel_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name) && device_list->items[i].sw.value) {
				if (device_list->state == INDIGO_BUSY_STATE) {
					if (property->state == INDIGO_ALERT_STATE) {
						device_list->state = INDIGO_ALERT_STATE;
					} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
						device_list->state = INDIGO_OK_STATE;
					}
					indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					return INDIGO_OK;
				} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
					device_list->state = INDIGO_ALERT_STATE;
					indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					return INDIGO_OK;
				}
			}
		}
		device_list = CLIENT_PRIVATE_DATA->agent_focuser_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name) && device_list->items[i].sw.value) {
				if (device_list->state == INDIGO_BUSY_STATE) {
					if (property->state == INDIGO_ALERT_STATE) {
						device_list->state = INDIGO_ALERT_STATE;
					} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
						device_list->state = INDIGO_OK_STATE;
					}
					indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					return INDIGO_OK;
				} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
					device_list->state = INDIGO_ALERT_STATE;
					indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
					return INDIGO_OK;
				}
			}
		}
	}
	return INDIGO_OK;
}


static indigo_result agent_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	if (*property->name == 0 || !strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_property *device_list;
		device_list = CLIENT_PRIVATE_DATA->agent_ccd_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name)) {
				int size = (device_list->count - i - 1) * sizeof(indigo_item);
				if (size > 0) {
					memcpy(device_list->items + i, device_list->items + i + 1, size);
				}
				device_list->count--;
				indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				return INDIGO_OK;
			}
		}
		device_list = CLIENT_PRIVATE_DATA->agent_wheel_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name)) {
				int size = (device_list->count - i - 1) * sizeof(indigo_item);
				if (size > 0) {
					memcpy(device_list->items + i, device_list->items + i + 1, size);
				}
				device_list->count--;
				indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				return INDIGO_OK;
			}
		}
		device_list = CLIENT_PRIVATE_DATA->agent_focuser_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name)) {
				int size = (device_list->count - i - 1) * sizeof(indigo_item);
				if (size > 0) {
					memcpy(device_list->items + i, device_list->items + i + 1, size);
				}
				device_list->count--;
				indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				return INDIGO_OK;
			}
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	// TBD
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_imager(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		IMAGER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		IMAGER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		agent_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Imager agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(agent_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(agent_private_data));
			agent_device = malloc(sizeof(indigo_device));
			assert(agent_device != NULL);
			private_data->device = agent_device;
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
			private_data->client = agent_client;
			memcpy(agent_client, &agent_client_template, sizeof(indigo_client));
			agent_client->client_context = private_data;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
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
