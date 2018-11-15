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

/** INDIGO Filter agent base
 \file indigo_filter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "indigo_filter.h"

indigo_result indigo_filter_device_attach(indigo_device *device, unsigned version, indigo_device_interface device_interface) {
	assert(device != NULL);
	if (FILTER_DEVICE_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_filter_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_filter_context));
	}
	FILTER_DEVICE_CONTEXT->device = device;
	FILTER_DEVICE_CONTEXT->device_interface = device_interface;
	if (FILTER_DEVICE_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, 0) == INDIGO_OK) {
			CONNECTION_PROPERTY->hidden = true;
			// -------------------------------------------------------------------------------- Device properties
			FILTER_DEVICE_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CCD_LIST_PROPERTY_NAME, "Main", "Camera list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_DEVICE_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_DEVICE_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_DEVICE_LIST_PROPERTY->items, AGENT_DEVICE_LIST_NONE_ITEM_NAME, "None", true);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_filter_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (indigo_property_match(FILTER_DEVICE_LIST_PROPERTY, property))
		indigo_define_property(device, FILTER_DEVICE_LIST_PROPERTY, NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_filter_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(FILTER_DEVICE_LIST_PROPERTY, property)) {
		*FILTER_DEVICE_CONTEXT->device_name = 0;
		indigo_property *connection_property = indigo_init_switch_property(NULL, "", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		indigo_init_switch_item(connection_property->items, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
		for (int i = 1; i < FILTER_DEVICE_LIST_PROPERTY->count; i++) {
			if (FILTER_DEVICE_LIST_PROPERTY->items[i].sw.value) {
				FILTER_DEVICE_LIST_PROPERTY->items[i].sw.value = false;
				strcpy(connection_property->device, FILTER_DEVICE_LIST_PROPERTY->items[i].name);
				indigo_change_property(client, connection_property);
				break;
			}
		}
		indigo_property_copy_values(FILTER_DEVICE_LIST_PROPERTY, property, false);
		for (int i = 1; i < FILTER_DEVICE_LIST_PROPERTY->count; i++) {
			if (FILTER_DEVICE_LIST_PROPERTY->items[i].sw.value) {
				FILTER_DEVICE_LIST_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, FILTER_DEVICE_LIST_PROPERTY, NULL);
				strcpy(connection_property->device, FILTER_DEVICE_LIST_PROPERTY->items[i].name);
				strcpy(connection_property->items->name, CONNECTION_CONNECTED_ITEM_NAME);
				indigo_change_property(client, connection_property);
				return INDIGO_OK;
			}
		}
		FILTER_DEVICE_LIST_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FILTER_DEVICE_LIST_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (*FILTER_DEVICE_CONTEXT->device_name) {
		indigo_property **agent_cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (agent_cache[i] && indigo_property_match(agent_cache[i], property)) {
				int size = sizeof(indigo_property) + property->count * sizeof(indigo_item);
				indigo_property *copy = (indigo_property *)malloc(size);
				memcpy(copy, property, size);
				strcpy(copy->device, FILTER_DEVICE_CONTEXT->device_name);
				indigo_change_property(client, copy);
				indigo_release_property(copy);
				return INDIGO_OK;
			}
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_filter_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(FILTER_DEVICE_LIST_PROPERTY);
	return indigo_device_detach(device);
}

indigo_result indigo_filter_client_attach(indigo_client *client) {
	assert(client != NULL);
	assert (FILTER_CLIENT_CONTEXT != NULL);
	FILTER_CLIENT_CONTEXT->client = client;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		device_cache[i] = NULL;
		agent_cache[i] = NULL;
	}
	indigo_property all_properties;
	memset(&all_properties, 0, sizeof(all_properties));
	indigo_enumerate_properties(client, &all_properties);
	return INDIGO_OK;
}

indigo_result indigo_filter_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device)
		return INDIGO_OK;
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_item *interface = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
		if (interface) {
			int mask = atoi(interface->text.value);
			if (mask & FILTER_CLIENT_CONTEXT->device_interface) {
				indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_property;
				int count = device_list->count;
				for (int i = 1; i < count; i++) {
					if (!strcmp(property->device, device_list->items[i].name)) {
						return INDIGO_OK;
					}
				}
				if (count < INDIGO_FILTER_MAX_DEVICES) {
					indigo_delete_property(device, device_list, NULL);
					indigo_init_switch_item(device_list->items + count, property->device, property->device, false);
					device_list->count++;
					indigo_define_property(device, device_list, NULL);
				}
				return INDIGO_OK;
			}
		}
	} else 	if (!strcmp(property->group, MAIN_GROUP)) {
		// ignore
	}
	if (*FILTER_CLIENT_CONTEXT->device_name == 0 || strcmp(FILTER_CLIENT_CONTEXT->device_name, property->device))
		return INDIGO_OK;
	bool found = false;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		if (device_cache[i] == property) {
			found = true;
			break;
		}
	}
	if (!found) {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] == NULL) {
				device_cache[i] = property;
				int size = sizeof(indigo_property) + property->count * sizeof(indigo_item);
				indigo_property *copy = (indigo_property *)malloc(size);
				memcpy(copy, property, size);
				strcpy(copy->device, device->name);
				agent_cache[i] = copy;
				if (copy->type == INDIGO_BLOB_VECTOR)
					indigo_add_blob(copy);
				indigo_define_property(device, copy, NULL);
				break;
			}
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device)
		return INDIGO_OK;
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (property->state != INDIGO_BUSY_STATE) {
			indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
			indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_property;
			for (int i = 1; i < device_list->count; i++) {
				if (!strcmp(property->device, device_list->items[i].name) && device_list->items[i].sw.value) {
					if (device_list->state == INDIGO_BUSY_STATE) {
						if (property->state == INDIGO_ALERT_STATE) {
							device_list->state = INDIGO_ALERT_STATE;
							strcpy(FILTER_CLIENT_CONTEXT->device_name, "");
						} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
							strcpy(FILTER_CLIENT_CONTEXT->device_name, property->device);
							indigo_property all_properties;
							memset(&all_properties, 0, sizeof(all_properties));
							strcpy(all_properties.device, property->device);
							indigo_enumerate_properties(client, &all_properties);
							device_list->state = INDIGO_OK_STATE;
						}
						indigo_update_property(device, device_list, NULL);
						break;
					} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
						device_list->state = INDIGO_ALERT_STATE;
						strcpy(FILTER_CLIENT_CONTEXT->device_name, "");
						indigo_update_property(device, device_list, NULL);
						break;
					}
				}
			}
			return INDIGO_OK;
		}
	}
	if (*FILTER_CLIENT_CONTEXT->device_name == 0 || strcmp(FILTER_CLIENT_CONTEXT->device_name, property->device))
		return INDIGO_OK;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		if (device_cache[i] == property) {
			if (agent_cache[i]) {
				memcpy(agent_cache[i]->items, device_cache[i]->items, device_cache[i]->count * sizeof(indigo_item));
				agent_cache[i]->state = device_cache[i]->state;
				indigo_update_property(device, agent_cache[i], NULL);
			}
			break;
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device)
		return INDIGO_OK;
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (*property->name) {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] == property) {
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					if (agent_cache[i]->type == INDIGO_BLOB_VECTOR)
						indigo_delete_blob(agent_cache[i]);
					indigo_delete_property(device, agent_cache[i], NULL);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
				break;
			}
		}
	} else {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] && !strcmp(device_cache[i]->device, property->device)) {
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					if (agent_cache[i]->type == INDIGO_BLOB_VECTOR)
						indigo_delete_blob(agent_cache[i]);
					indigo_delete_property(device, agent_cache[i], NULL);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
			}
		}
	}
	if (*property->name == 0 || !strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_property;
		for (int i = 1; i < device_list->count; i++) {
			if (!strcmp(property->device, device_list->items[i].name)) {
				int size = (device_list->count - i - 1) * sizeof(indigo_item);
				if (size > 0) {
					memcpy(device_list->items + i, device_list->items + i + 1, size);
				}
				indigo_delete_property(device, device_list, NULL);
				device_list->count--;
				indigo_define_property(device, device_list, NULL);
				break;
			}
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_client_detach(indigo_client *client) {
	return INDIGO_OK;
}
