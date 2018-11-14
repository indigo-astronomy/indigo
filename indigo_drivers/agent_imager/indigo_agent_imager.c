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

#define MAX_DEVICES							32
#define MAX_CACHED_PROPERTIES		128

#define DEVICE_PRIVATE_DATA							((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA							((agent_private_data *)client->client_context)

#define AGENT_CCD_LIST_PROPERTY					(DEVICE_PRIVATE_DATA->agent_ccd_list_property)

#define AGENT_CCD_BATCH_PROPERTY				(DEVICE_PRIVATE_DATA->agent_ccd_batch_property)
#define AGENT_CCD_BATCH_COUNT_ITEM      (AGENT_CCD_BATCH_PROPERTY->items+0)
#define AGENT_CCD_BATCH_EXPOSURE_ITEM   (AGENT_CCD_BATCH_PROPERTY->items+1)
#define AGENT_CCD_BATCH_DELAY_ITEM     	(AGENT_CCD_BATCH_PROPERTY->items+2)

#define AGENT_START_PROCESS_PROPERTY		(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_START_CCD_EXPOSURE_ITEM   (AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_START_CCD_STREAMING_ITEM  (AGENT_START_PROCESS_PROPERTY->items+1)

#define AGENT_ABORT_PROCESS_PROPERTY		(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      	(AGENT_ABORT_PROCESS_PROPERTY->items+0)


typedef struct {
	indigo_device *device;
	indigo_client *client;
	char ccd[INDIGO_NAME_SIZE];
	indigo_property *agent_ccd_list_property;
	indigo_property *agent_ccd_batch_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *device_property_cache[MAX_CACHED_PROPERTIES];
	indigo_property *agent_property_cache[MAX_CACHED_PROPERTIES];
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent common code

static void exposure_batch(indigo_device *device) {
	indigo_property **cache = DEVICE_PRIVATE_DATA->device_property_cache;
	for (int j = 0; j < MAX_CACHED_PROPERTIES; j++) {
		if (cache[j] && !strcmp(cache[j]->device, DEVICE_PRIVATE_DATA->ccd) && !strcmp(cache[j]->name, CCD_EXPOSURE_PROPERTY_NAME)) {
			indigo_property *remote_exposure_property = cache[j];
			indigo_property *local_exposure_property = indigo_init_number_property(NULL, remote_exposure_property->device, remote_exposure_property->name, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, remote_exposure_property->count);
			if (local_exposure_property) {
				memcpy(local_exposure_property, remote_exposure_property, sizeof(indigo_property) + remote_exposure_property->count * sizeof(indigo_item));
				AGENT_CCD_BATCH_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
				int count = AGENT_CCD_BATCH_COUNT_ITEM->number.target;
				for (int i = count; AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && i > 0; i--) {
					AGENT_CCD_BATCH_COUNT_ITEM->number.value = i;
					double time = AGENT_CCD_BATCH_EXPOSURE_ITEM->number.target;
					AGENT_CCD_BATCH_EXPOSURE_ITEM->number.value = time;
					indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
					local_exposure_property->items[0].number.value = time;
					indigo_change_property(DEVICE_PRIVATE_DATA->client, local_exposure_property);
					while (remote_exposure_property->state == INDIGO_BUSY_STATE && time > 0) {
						if (time > 1) {
							usleep(1000000);
							time -= 1;
							AGENT_CCD_BATCH_EXPOSURE_ITEM->number.value = time;
							indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
						} else {
							usleep(10000);
							time -= 0.01;
						}
					}
					AGENT_CCD_BATCH_EXPOSURE_ITEM->number.value = 0;
					indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
					if (i > 1) {
						time = AGENT_CCD_BATCH_DELAY_ITEM->number.target;
						AGENT_CCD_BATCH_DELAY_ITEM->number.value = time;
						indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
						while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && time > 0) {
							if (time > 1) {
								usleep(1000000);
								time -= 1;
								AGENT_CCD_BATCH_DELAY_ITEM->number.value = time;
								indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
							} else {
								usleep(10000);
								time -= 0.01;
							}
						}
						AGENT_CCD_BATCH_DELAY_ITEM->number.value = 0;
						indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
					}
				}
				AGENT_CCD_BATCH_COUNT_ITEM->number.value = AGENT_CCD_BATCH_COUNT_ITEM->number.target;
				AGENT_CCD_BATCH_EXPOSURE_ITEM->number.value = AGENT_CCD_BATCH_EXPOSURE_ITEM->number.target;
				AGENT_CCD_BATCH_DELAY_ITEM->number.value = AGENT_CCD_BATCH_DELAY_ITEM->number.target;
				AGENT_CCD_BATCH_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
				if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				indigo_release_property(local_exposure_property);
			}
			return;
		}
	}
	AGENT_CCD_BATCH_COUNT_ITEM->number.value = AGENT_CCD_BATCH_COUNT_ITEM->number.target;
	AGENT_CCD_BATCH_EXPOSURE_ITEM->number.value = AGENT_CCD_BATCH_EXPOSURE_ITEM->number.target;
	AGENT_CCD_BATCH_DELAY_ITEM->number.value = AGENT_CCD_BATCH_DELAY_ITEM->number.target;
	AGENT_CCD_BATCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
}

static void streaming_batch(indigo_device *device) {
	
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		AGENT_CCD_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CCD_LIST_PROPERTY_NAME, "Main", "Camera list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICES);
		if (AGENT_CCD_LIST_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_CCD_LIST_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_CCD_LIST_PROPERTY->items, AGENT_DEVICE_LIST_NONE_ITEM_NAME, "None", true);
		// -------------------------------------------------------------------------------- Batch properties
		AGENT_CCD_BATCH_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_CCD_BATCH_PROPERTY_NAME, "Batch", "Batch settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_CCD_BATCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_CCD_BATCH_COUNT_ITEM, AGENT_CCD_BATCH_COUNT_ITEM_NAME, "Frame count", 0, 1000, 1, 1);
		indigo_init_number_item(AGENT_CCD_BATCH_EXPOSURE_ITEM, AGENT_CCD_BATCH_EXPOSURE_ITEM_NAME, "Exposure time", 0, 3600, 0, 1);
		indigo_init_number_item(AGENT_CCD_BATCH_DELAY_ITEM, AGENT_CCD_BATCH_DELAY_ITEM_NAME, "Delay after each exposure", 0, 3600, 0, 0);
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Batch", "Start batch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_START_CCD_EXPOSURE_ITEM, AGENT_START_CCD_EXPOSURE_ITEM_NAME, "Start batch", false);
		indigo_init_switch_item(AGENT_START_CCD_STREAMING_ITEM, AGENT_START_CCD_STREAMING_ITEM_NAME, "Start streaming", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Batch", "Abort batch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort batch", false);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client!= NULL && client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_CCD_LIST_PROPERTY, property))
		indigo_define_property(device, AGENT_CCD_LIST_PROPERTY, NULL);
	if (indigo_property_match(AGENT_CCD_BATCH_PROPERTY, property))
		indigo_define_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_CCD_LIST_PROPERTY, property)) {
		strcpy(DEVICE_PRIVATE_DATA->ccd, "");
		indigo_property *connection_property = indigo_init_switch_property(NULL, "", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		indigo_init_switch_item(connection_property->items + 0, CONNECTION_CONNECTED_ITEM_NAME, NULL, false);
		indigo_init_switch_item(connection_property->items + 1, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
		for (int i = 1; i < AGENT_CCD_LIST_PROPERTY->count; i++) {
			if (AGENT_CCD_LIST_PROPERTY->items[i].sw.value) {
				AGENT_CCD_LIST_PROPERTY->items[i].sw.value = false;
				strncpy(connection_property->device, AGENT_CCD_LIST_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, connection_property);
				break;
			}
		}
		indigo_property_copy_values(AGENT_CCD_LIST_PROPERTY, property, false);
		for (int i = 1; i < AGENT_CCD_LIST_PROPERTY->count; i++) {
			if (AGENT_CCD_LIST_PROPERTY->items[i].sw.value) {
				indigo_set_switch(connection_property, connection_property->items, true);
				AGENT_CCD_LIST_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_CCD_LIST_PROPERTY, NULL);
				strncpy(connection_property->device, AGENT_CCD_LIST_PROPERTY->items[i].name, INDIGO_NAME_SIZE);
				indigo_change_property(client, connection_property);
				return INDIGO_OK;
			}
		}
		AGENT_CCD_LIST_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_CCD_LIST_PROPERTY, NULL);
		return INDIGO_OK;
	} else 	if (indigo_property_match(AGENT_CCD_BATCH_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_CCD_BATCH_PROPERTY, property, false);
		AGENT_CCD_BATCH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_CCD_BATCH_PROPERTY, NULL);
	} else 	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
			if (AGENT_START_CCD_EXPOSURE_ITEM->sw.value) {
				AGENT_START_CCD_EXPOSURE_ITEM->sw.value = false;
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, exposure_batch);
			} else if (AGENT_START_CCD_STREAMING_ITEM->sw.value) {
				AGENT_START_CCD_EXPOSURE_ITEM->sw.value = false;
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, streaming_batch);
			}
		}
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	} else 	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property *abort_property = indigo_init_switch_property(NULL, DEVICE_PRIVATE_DATA->ccd, CCD_ABORT_EXPOSURE_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (abort_property) {
				indigo_init_switch_item(abort_property->items, CCD_ABORT_EXPOSURE_ITEM_NAME, "", true);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, abort_property);
				indigo_release_property(abort_property);
			}
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		}
		AGENT_ABORT_PROCESS_ITEM->sw.value = false;
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	} else if (*DEVICE_PRIVATE_DATA->ccd) {
		indigo_property **agent_cache = DEVICE_PRIVATE_DATA->agent_property_cache;
		for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
			if (agent_cache[i] && indigo_property_match(agent_cache[i], property)) {
				int size = sizeof(indigo_property) + property->count * sizeof(indigo_item);
				indigo_property *copy = (indigo_property *)malloc(size);
				memcpy(copy, property, size);
				strcpy(copy->device, DEVICE_PRIVATE_DATA->ccd);
				indigo_change_property(client, copy);
				indigo_release_property(copy);
				return INDIGO_OK;
			}
		}
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_CCD_LIST_PROPERTY);
	indigo_release_property(AGENT_CCD_BATCH_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	return indigo_agent_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_client_attach(indigo_client *client) {
	indigo_property all_properties;
	indigo_property **device_cache = CLIENT_PRIVATE_DATA->device_property_cache;
	indigo_property **agent_cache = CLIENT_PRIVATE_DATA->device_property_cache;
	for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
		device_cache[i] = NULL;
		agent_cache[i] = NULL;
	}
	memset(&all_properties, 0, sizeof(all_properties));
	indigo_enumerate_properties(client, &all_properties);
	return INDIGO_OK;
}

static indigo_result agent_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	device = CLIENT_PRIVATE_DATA->device;
	indigo_property **device_cache = CLIENT_PRIVATE_DATA->device_property_cache;
	indigo_property **agent_cache = CLIENT_PRIVATE_DATA->agent_property_cache;
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_item *interface = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
		if (interface) {
			int mask = atoi(interface->text.value);
			if (mask & INDIGO_INTERFACE_CCD) {
				indigo_property *device_list = CLIENT_PRIVATE_DATA->agent_ccd_list_property;
				int count = device_list->count;
				for (int i = 1; i < count; i++) {
					if (!strcmp(property->device, device_list->items[i].name)) {
						return INDIGO_OK;
					}
				}
				if (count < MAX_DEVICES) {
					indigo_delete_property(device, device_list, NULL);
					indigo_init_switch_item(device_list->items + count, property->device, property->device, false);
					device_list->count++;
					indigo_define_property(device, device_list, NULL);
				}
				return INDIGO_OK;
			}
		}
	}
	if (*CLIENT_PRIVATE_DATA->ccd == 0 || strcmp(CLIENT_PRIVATE_DATA->ccd, property->device))
		return INDIGO_OK;
	bool found = false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "define %s.%s", property->device, property->name);
	for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
		if (device_cache[i] == property) {
			found = true;
			break;
		}
	}
	if (!found) {
		for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
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

static indigo_result agent_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	device = CLIENT_PRIVATE_DATA->device;
	indigo_property **device_cache = CLIENT_PRIVATE_DATA->device_property_cache;
	indigo_property **agent_cache = CLIENT_PRIVATE_DATA->agent_property_cache;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "update %s.%s", property->device, property->name);
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (property->state != INDIGO_BUSY_STATE) {
			indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
			indigo_property *device_list = CLIENT_PRIVATE_DATA->agent_ccd_list_property;
			for (int i = 1; i < device_list->count; i++) {
				if (!strcmp(property->device, device_list->items[i].name) && device_list->items[i].sw.value) {
					if (device_list->state == INDIGO_BUSY_STATE) {
						if (property->state == INDIGO_ALERT_STATE) {
							device_list->state = INDIGO_ALERT_STATE;
							strcpy(CLIENT_PRIVATE_DATA->ccd, "");
						} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
							strcpy(CLIENT_PRIVATE_DATA->ccd, property->device);
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
						strcpy(CLIENT_PRIVATE_DATA->ccd, "");
						indigo_update_property(device, device_list, NULL);
						break;
					}
				}
			}
			return INDIGO_OK;
		}
	}
	if (*CLIENT_PRIVATE_DATA->ccd == 0 || strcmp(CLIENT_PRIVATE_DATA->ccd, property->device))
		return INDIGO_OK;
	for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
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

static indigo_result agent_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	device = CLIENT_PRIVATE_DATA->device;
	indigo_property **device_cache = CLIENT_PRIVATE_DATA->device_property_cache;
	indigo_property **agent_cache = CLIENT_PRIVATE_DATA->agent_property_cache;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "delete %s.%s", property->device, property->name);
	if (*property->name) {
		for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
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
		for (int i = 0; i < MAX_CACHED_PROPERTIES; i++) {
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
		indigo_property *device_list = CLIENT_PRIVATE_DATA->agent_ccd_list_property;
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

static indigo_result agent_client_detach(indigo_client *client) {
	// TBD
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

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
