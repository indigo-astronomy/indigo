// Copyright (c) 2022 CloudMakers, s. r. o.
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

/** INDIGO Configuration agent
 \file indigo_agent_config.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME	"indigo_agent_config"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>

#include <indigo/indigo_agent.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_client.h>

#include "indigo_agent_config.h"

#define DEVICE_PRIVATE_DATA												private_data
#define CLIENT_PRIVATE_DATA												private_data

#define AGENT_CONFIG_SETUP_PROPERTY								(DEVICE_PRIVATE_DATA->setup)
#define AGENT_CONFIG_SETUP_AUTOSAVE_ITEM					(AGENT_CONFIG_SETUP_PROPERTY->items+0)
#define AGENT_CONFIG_SETUP_UNLOAD_DRIVERS_ITEM		(AGENT_CONFIG_SETUP_PROPERTY->items+1)

#define AGENT_CONFIG_SAVE_PROPERTY								(DEVICE_PRIVATE_DATA->save_config)
#define AGENT_CONFIG_SAVE_NAME_ITEM			    			(AGENT_CONFIG_SAVE_PROPERTY->items+0)

#define AGENT_CONFIG_DELETE_PROPERTY							(DEVICE_PRIVATE_DATA->remove_config)
#define AGENT_CONFIG_DELETE_NAME_ITEM			    		(AGENT_CONFIG_DELETE_PROPERTY->items+0)

#define AGENT_CONFIG_LOAD_PROPERTY								(DEVICE_PRIVATE_DATA->load_config)

#define AGENT_CONFIG_LAST_CONFIG_PROPERTY					(DEVICE_PRIVATE_DATA->last_config)
#define AGENT_CONFIG_LAST_CONFIG_NAME_ITEM			  (AGENT_CONFIG_LAST_CONFIG_PROPERTY->items+0)

#define AGENT_CONFIG_DRIVERS_PROPERTY							(DEVICE_PRIVATE_DATA->drivers)

#define AGENT_CONFIG_PROFILES_PROPERTY						(DEVICE_PRIVATE_DATA->profiles)

#define AGENT_CONFIG_PROPERTY_NAME								"AGENT_CONFIG %s" // remember to change all "12" and "13" occurences if this format is changed

#define MAX_AGENTS																16
#define AGENT_CONFIG_AGENTS_PROPERTIES						(DEVICE_PRIVATE_DATA->agents)

#define EXTENSION																	".saved"

typedef struct {
	indigo_property *setup;
	indigo_property *save_config;
	indigo_property *remove_config;
	indigo_property *load_config;
	indigo_property *last_config;
	indigo_property *drivers;
	indigo_property *profiles;
	indigo_property *agents[MAX_AGENTS];
	char server[INDIGO_NAME_SIZE];
	int restore_count;
	indigo_property *restore_properties[MAX_AGENTS];
	pthread_mutex_t restore_mutex;
	pthread_mutex_t data_mutex;
	bool failure;
} config_agent_private_data;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;
static config_agent_private_data *private_data = NULL;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static void replace_spaces(char *str) {
	while (*str) {
		if (isspace(*str)) *str = '_';
		str++;
	}
}

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		indigo_save_property(device, NULL, AGENT_CONFIG_SETUP_PROPERTY);
		if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	}
}

static int configuration_filter(const struct dirent *entry) {
	return strstr(entry->d_name, EXTENSION) != NULL;
}

static void populate_list(indigo_device *device) {
	struct dirent **entries;
	char folder[256];
	snprintf(folder, sizeof(folder), "%s/.indigo/", getenv("HOME"));
	int count = scandir(folder, &entries, configuration_filter, alphasort);
	if (count >= 0) {
		AGENT_CONFIG_LOAD_PROPERTY = indigo_resize_property(AGENT_CONFIG_LOAD_PROPERTY, count);
		char file_name[INDIGO_VALUE_SIZE + INDIGO_NAME_SIZE];
		struct stat file_stat;
		int valid_count = 0;
		for (int i = 0; i < count; i++) {
			strcpy(file_name, folder);
			strcat(file_name, entries[i]->d_name);
			if (stat(file_name, &file_stat) >= 0 && file_stat.st_size > 0) {
				char *ext = strstr(entries[i]->d_name, EXTENSION);
				if (ext)
					*ext = 0;
				indigo_init_switch_item(AGENT_CONFIG_LOAD_PROPERTY->items + valid_count, entries[i]->d_name, entries[i]->d_name, false);
				valid_count++;
			}
			free(entries[i]);
		}
		AGENT_CONFIG_LOAD_PROPERTY->count = valid_count;
		free(entries);
	}
}

static void load_configuration(indigo_device *device) {
	// request deselect everything from all agents first
	indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, "Unloading current configuration, please wait...");
	for (int i = 0; i < MAX_AGENTS; i++) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
		indigo_property *agent = DEVICE_PRIVATE_DATA->agents[i];
		if (agent) {
			indigo_property *copy = indigo_safe_malloc_copy(sizeof(indigo_property) + agent->count * sizeof(indigo_item), agent);
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
			char *device_name = copy->name + 13;
			for (int j = 0; j < copy->count; j++) {
				indigo_item *item = copy->items + j;
				if (strcmp(item->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
					if (*item->text.value) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Deselecting '%s' from '%s'", item->text.value, device_name);
						indigo_change_switch_property_1(agent_client, device_name, item->name, FILTER_DEVICE_LIST_NONE_ITEM_NAME, true);
					}
				} else {
					char *rest = NULL;
					for (char *token = strtok_r(item->text.value, ";", &rest); token; token = strtok_r(NULL, ";", &rest)) {
						if (token == NULL || *token == 0) {
							break;
						}
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Deselecting '%s' from '%s'", token, device_name);
						indigo_change_switch_property_1(agent_client, device_name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, token, false);
					}
				}
			}
		} else {
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		}
	}
	// check everything is deselected, wait up to 10s
	bool done = false;
	for (int k = 0; k < 20; k++) {
		done = true;
		for (int i = 0; i < MAX_AGENTS; i++) {
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
			indigo_property *agent = DEVICE_PRIVATE_DATA->agents[i];
			if (agent) {
				for (int j = 0; j < agent->count; j++) {
					indigo_item *item = agent->items + j;
					if (*item->text.value) {
						if (k == 0)
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Waiting for %s to disconnect", item->text.value);
						done = false;
					}
				}
			}
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		}
		if (done) {
			break;
		}
		indigo_usleep(500000);
	}
	if (!done) {
		for (int i = 0; i < AGENT_CONFIG_LOAD_PROPERTY->count; i++)
			AGENT_CONFIG_LOAD_PROPERTY->items[i].sw.value = false;
		AGENT_CONFIG_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, "Can't deselect active devices before loading new configuration");
		return;
	}
	indigo_usleep(ONE_SECOND_DELAY);
	// load saved configuration
	DEVICE_PRIVATE_DATA->failure = false;
	for (int i = 0; i < AGENT_CONFIG_LOAD_PROPERTY->count; i++) {
		indigo_item *item = AGENT_CONFIG_LOAD_PROPERTY->items + i;
		if (item->sw.value) {
			indigo_uni_handle *handle = indigo_open_config_file(item->name, 0, false, EXTENSION);
			if (handle != NULL) {
				indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, "Loading configuration '%s', please wait...", item->name);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Loading saved configuration from %s%s", item->name, EXTENSION);
				strncpy(AGENT_CONFIG_LAST_CONFIG_NAME_ITEM->text.value, item->name, INDIGO_NAME_SIZE);
				indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
				indigo_client *client = indigo_safe_malloc(sizeof(indigo_client));
				strcpy(client->name, CONFIG_READER);
				indigo_adapter_context *context = indigo_safe_malloc(sizeof(indigo_adapter_context));
				context->input = handle;
				client->client_context = context;
				client->version = INDIGO_VERSION_CURRENT;
				DEVICE_PRIVATE_DATA->restore_count = 0;
				indigo_xml_parse(NULL, client);
				indigo_uni_close(&handle);
				free(context);
				free(client);
				// wait for all the changes to be applied
				indigo_usleep(500000);
				bool done = false;
				while (!done) {
					done = true;
					for (int j = 0; j < DEVICE_PRIVATE_DATA->restore_count; j++) {
						if (DEVICE_PRIVATE_DATA->restore_properties[j]) {
							done = false;
							break;
						}
					}
					if (done) {
						break;
					}
					indigo_usleep(100000);
				}
				strncpy(AGENT_CONFIG_LAST_CONFIG_NAME_ITEM->text.value, item->name, INDIGO_VALUE_SIZE);
			}
			item->sw.value = false;
		}
	}
	if (DEVICE_PRIVATE_DATA->failure) {
		AGENT_CONFIG_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
		AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, "Configuration did not load properly. Are all devices connected?");
	} else {
		AGENT_CONFIG_LOAD_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, "Configuration loaded");
	}
	indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
}

static void process_configuration_property(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->restore_mutex);
	for (int i = 0; i < DEVICE_PRIVATE_DATA->restore_count; i++) {
		indigo_property *property = DEVICE_PRIVATE_DATA->restore_properties[i];
		if (property) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Restoring '%s'", property->name);
			if (!strcmp(property->name, AGENT_CONFIG_DRIVERS_PROPERTY_NAME)) {
				indigo_property *copy = indigo_safe_malloc_copy(sizeof(indigo_property) + property->count * sizeof(indigo_item), property);
				strcpy(copy->name, SERVER_DRIVERS_PROPERTY_NAME);
				strcpy(copy->device, DEVICE_PRIVATE_DATA->server);
				if (!AGENT_CONFIG_SETUP_UNLOAD_DRIVERS_ITEM->sw.value) {
					for (int j = 0; j < AGENT_CONFIG_DRIVERS_PROPERTY->count; j++) {
						indigo_item *item = AGENT_CONFIG_DRIVERS_PROPERTY->items + j;
						if (item->sw.value) {
							for (int k = 0; k < copy->count; k++) {
								indigo_item *copy_item = copy->items + k;
								if (!strcmp(item->name, copy_item->name)) {
									copy_item->sw.value = true;
									break;
								}
							}
						}
					}
				}
				indigo_change_property(agent_client, copy); // it expects this call is actually synchronous on a local bus
				indigo_safe_free(copy);
			} else if (!strcmp(property->name, AGENT_CONFIG_PROFILES_PROPERTY_NAME)) {
				for (int j = 0; j < property->count; j++) {
					indigo_item *item = property->items + j;
					// wait up to 10s until the device is present
					bool done = false;
					for (int k = 0; k < 20; k++) {
						pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
						for (int l = 0; l < AGENT_CONFIG_PROFILES_PROPERTY->count; l++) {
							if (!strcmp(item->name, AGENT_CONFIG_PROFILES_PROPERTY->items[l].name)) {
								done = true;
								break;
							}
						}
						pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
						if (done) {
							break;
						}
						if (k == 0)
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Waiting for '%s'", item->name);
						indigo_usleep(500000);
					}
					if (done) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Selecting '%s' to '%s'", item->text.value, item->name);
						indigo_change_switch_property_1(agent_client, item->name, PROFILE_PROPERTY_NAME, item->text.value, true); // it expects this call is actually synchronous on a local bus
					} else {
						DEVICE_PRIVATE_DATA->failure = true;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' profile can't be restored", item->text.value, item->name);
					}
				}
			} else {
				char *device_name = property->name + 13;
				for (int j = 0; j < property->count; j++) {
					indigo_item *item = property->items + j;
					if (strcmp(item->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
						if (*item->text.value) {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Selecting '%s' to '%s'.'%s'", item->text.value, device_name, item->name);
							indigo_change_switch_property_1(agent_client, device_name, item->name, item->text.value, true);
						} else {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Selecting 'NONE' to '%s'.'%s'", device_name, item->name);
							indigo_change_switch_property_1(agent_client, device_name, item->name, FILTER_DEVICE_LIST_NONE_ITEM_NAME, true);
						}
					}
				}
				for (int j = 0; j < property->count; j++) {
					indigo_item *item = property->items + j;
					if (!strcmp(item->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
						char *rest = NULL;
						for (char *token = strtok_r(item->text.value, ";", &rest); token; token = strtok_r(NULL, ";", &rest)) {
							if (token == NULL || *token == 0) {
								break;
							}
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Selecting '%s' from '%s'", item->text.value, device_name);
							indigo_change_switch_property_1(agent_client, device_name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, token, true);
						}
						break;
					}
				}
				// wait up to 10s if agent property is busy
				for (int k = 0; k < 20; k++) {
					indigo_usleep(500000);
					bool done = true;
					pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
					for (int j = 0; j < MAX_AGENTS; j++) {
						indigo_property *agent = agent = DEVICE_PRIVATE_DATA->agents[j];
						if (agent && !strcmp(property->name, agent->name)) {
							if (agent->state == INDIGO_ALERT_STATE) {
								DEVICE_PRIVATE_DATA->failure = true;
								INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failed to restore '%s'", property->name);
							} else if (agent->state == INDIGO_OK_STATE) {
								INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' restored", property->name);
							} else {
								done = false;
							}
							break;
						}
					}
					pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
					if (done) {
						break;
					}
				}
			}
			indigo_release_property(property);
			DEVICE_PRIVATE_DATA->restore_properties[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->restore_mutex);
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		AGENT_CONFIG_SETUP_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CONFIG_SETUP_PROPERTY_NAME, MAIN_GROUP, "Agent configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_CONFIG_SETUP_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_CONFIG_SETUP_AUTOSAVE_ITEM, AGENT_CONFIG_SETUP_AUTOSAVE_ITEM_NAME, "Autosave device configurations on profile save", false);
		indigo_init_switch_item(AGENT_CONFIG_SETUP_UNLOAD_DRIVERS_ITEM, AGENT_CONFIG_SETUP_UNLOAD_DRIVERS_ITEM_NAME, "Unload unused drivers", false);

		AGENT_CONFIG_SAVE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_CONFIG_SAVE_PROPERTY_NAME, MAIN_GROUP, "Save as configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_CONFIG_SAVE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_CONFIG_SAVE_NAME_ITEM, AGENT_CONFIG_SAVE_NAME_ITEM_NAME, "Name", "");

		AGENT_CONFIG_DELETE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_CONFIG_DELETE_PROPERTY_NAME, MAIN_GROUP, "Delete configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_CONFIG_DELETE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_CONFIG_DELETE_NAME_ITEM, AGENT_CONFIG_DELETE_NAME_ITEM_NAME, "Name", "");

		AGENT_CONFIG_LAST_CONFIG_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_CONFIG_LAST_CONFIG_PROPERTY_NAME, MAIN_GROUP, "Last configuration used", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (AGENT_CONFIG_LAST_CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_CONFIG_LAST_CONFIG_NAME_ITEM, AGENT_CONFIG_LAST_CONFIG_NAME_ITEM_NAME, "Name", "");

		AGENT_CONFIG_LOAD_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CONFIG_LOAD_PROPERTY_NAME, MAIN_GROUP, "Load configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 16);
		if (AGENT_CONFIG_LOAD_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_CONFIG_LOAD_PROPERTY->count = 0;
		populate_list(device);

		AGENT_CONFIG_DRIVERS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CONFIG_DRIVERS_PROPERTY_NAME, "Configuration", "Drivers", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ANY_OF_MANY_RULE, 0);
		if (AGENT_CONFIG_DRIVERS_PROPERTY == NULL)
			return INDIGO_FAILED;

		AGENT_CONFIG_PROFILES_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_CONFIG_PROFILES_PROPERTY_NAME, "Configuration", "Profiles", INDIGO_OK_STATE, INDIGO_RO_PERM, 128);
		if (AGENT_CONFIG_PROFILES_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_CONFIG_PROFILES_PROPERTY->count = 0;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->restore_mutex, NULL);
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->data_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);;
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_define_matching_property(AGENT_CONFIG_SETUP_PROPERTY);
	indigo_define_matching_property(AGENT_CONFIG_SAVE_PROPERTY);
	indigo_define_matching_property(AGENT_CONFIG_DELETE_PROPERTY);
	indigo_define_matching_property(AGENT_CONFIG_LOAD_PROPERTY);
	indigo_define_matching_property(AGENT_CONFIG_LAST_CONFIG_PROPERTY);
	indigo_define_matching_property(AGENT_CONFIG_DRIVERS_PROPERTY);
	indigo_define_matching_property(AGENT_CONFIG_PROFILES_PROPERTY);
	for (int i = 0; i < MAX_AGENTS; i++)
		if (AGENT_CONFIG_AGENTS_PROPERTIES[i] && indigo_property_match(AGENT_CONFIG_AGENTS_PROPERTIES[i], property))
			indigo_define_property(device, AGENT_CONFIG_AGENTS_PROPERTIES[i], NULL);
	return indigo_agent_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(AGENT_CONFIG_SETUP_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_CONFIG_SETUP_PROPERTY, property, false);
		AGENT_CONFIG_SETUP_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_CONFIG_SETUP_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CONFIG_SAVE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_CONFIG_SAVE_PROPERTY, property, false);
		char message[INDIGO_VALUE_SIZE] = "";
		if (*AGENT_CONFIG_SAVE_NAME_ITEM->text.value) {
			replace_spaces(AGENT_CONFIG_SAVE_NAME_ITEM->text.value);
			if (AGENT_CONFIG_SETUP_AUTOSAVE_ITEM->sw.value) {
				for (int i = 0; i < MAX_AGENTS; i++) {
					pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
					indigo_property *agent = DEVICE_PRIVATE_DATA->agents[i];
					if (agent) {
						for (int j = 0; j < agent->count; j++) {
							indigo_item *item = agent->items + j;
							if (strcmp(item->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
								if (*item->text.value) {
									INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Saving '%s' configuration", item->text.value);
									indigo_change_switch_property_1(agent_client, item->text.value, CONFIG_PROPERTY_NAME, CONFIG_SAVE_ITEM_NAME, true);
								}
							}
						}
					}
					pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
				}
			}
			indigo_uni_handle *handle = indigo_open_config_file(AGENT_CONFIG_SAVE_NAME_ITEM->text.value, 0, true, EXTENSION);
			if (handle != NULL) {
				pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
				AGENT_CONFIG_DRIVERS_PROPERTY->perm = INDIGO_RW_PERM;
				indigo_save_property(device, &handle, AGENT_CONFIG_DRIVERS_PROPERTY);
				AGENT_CONFIG_DRIVERS_PROPERTY->perm = INDIGO_RO_PERM;
				AGENT_CONFIG_PROFILES_PROPERTY->perm = INDIGO_RW_PERM;
				indigo_save_property(device, &handle, AGENT_CONFIG_PROFILES_PROPERTY);
				AGENT_CONFIG_PROFILES_PROPERTY->perm = INDIGO_RO_PERM;
				for (int i = 0; i < MAX_AGENTS; i++) {
					indigo_property *agent = DEVICE_PRIVATE_DATA->agents[i];
					if (agent) {
						agent->perm = INDIGO_RW_PERM;
						indigo_save_property(device, &handle, agent);
						agent->perm = INDIGO_RO_PERM;
					}
				}
				pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
				indigo_uni_close(&handle);
				snprintf(message, INDIGO_VALUE_SIZE, "Active configuration saved as '%s'", AGENT_CONFIG_SAVE_NAME_ITEM->text.value);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Active configuration saved to %s%s", AGENT_CONFIG_SAVE_NAME_ITEM->text.value, EXTENSION);
				AGENT_CONFIG_SAVE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				snprintf(message, INDIGO_VALUE_SIZE, "Failed to save active configuration as '%s'", AGENT_CONFIG_SAVE_NAME_ITEM->text.value);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to save active configuration to %s%s", AGENT_CONFIG_SAVE_NAME_ITEM->text.value, EXTENSION);
				AGENT_CONFIG_SAVE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			snprintf(message, INDIGO_VALUE_SIZE, "Configuration name '%s' is not valid", AGENT_CONFIG_SAVE_NAME_ITEM->text.value);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid name for active configuration");
			AGENT_CONFIG_SAVE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_delete_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
		populate_list(device);
		indigo_define_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
		if (message[0] == '\0') {
			indigo_update_property(device, AGENT_CONFIG_SAVE_PROPERTY, NULL);
		} else {
			indigo_update_property(device, AGENT_CONFIG_SAVE_PROPERTY, message);
		}
		if (AGENT_CONFIG_SAVE_PROPERTY->state == INDIGO_OK_STATE) {
			AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			strncpy(AGENT_CONFIG_LAST_CONFIG_NAME_ITEM->text.value, AGENT_CONFIG_SAVE_NAME_ITEM->text.value, INDIGO_VALUE_SIZE);
			indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CONFIG_LOAD_PROPERTY, property)) {
		if (AGENT_CONFIG_LOAD_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_CONFIG_LOAD_PROPERTY, property, false);
			AGENT_CONFIG_LOAD_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
			AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_CONFIG_LAST_CONFIG_NAME_ITEM->text.value[0] = '\0';
			indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
			indigo_set_timer(device, 0, load_configuration, NULL);
		} else {
			indigo_update_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CONFIG_DELETE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_CONFIG_DELETE_PROPERTY, property, false);
		replace_spaces(AGENT_CONFIG_DELETE_NAME_ITEM->text.value);
		char message[INDIGO_VALUE_SIZE] = "";
		if (strchr(AGENT_CONFIG_DELETE_NAME_ITEM->text.value, '/')) {
			snprintf(message, INDIGO_VALUE_SIZE, "Invalid configuration name '%s'", AGENT_CONFIG_DELETE_NAME_ITEM->text.value);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid configuration name %s", AGENT_CONFIG_DELETE_NAME_ITEM->text.value);
			AGENT_CONFIG_DELETE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			char path[256];
			snprintf(path, sizeof(path), "%s/.indigo/%s%s", getenv("HOME"), AGENT_CONFIG_DELETE_NAME_ITEM->text.value, EXTENSION);
			if (unlink(path)) {
				snprintf(message, INDIGO_VALUE_SIZE, "Failed to remove configuration '%s'", AGENT_CONFIG_DELETE_NAME_ITEM->text.value);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't remove saved configuration %s", path);
				AGENT_CONFIG_DELETE_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				snprintf(message, INDIGO_VALUE_SIZE, "Configuration '%s' deleted", AGENT_CONFIG_DELETE_NAME_ITEM->text.value);
				AGENT_CONFIG_DELETE_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_delete_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
			populate_list(device);
			indigo_define_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
		}
		if (message[0] == '\0') {
			indigo_update_property(device, AGENT_CONFIG_DELETE_PROPERTY, NULL);
		} else {
			indigo_update_property(device, AGENT_CONFIG_DELETE_PROPERTY, message);
		}
		if (
			AGENT_CONFIG_DELETE_PROPERTY->state == INDIGO_OK_STATE &&
			!strncmp(AGENT_CONFIG_DELETE_NAME_ITEM->text.value, AGENT_CONFIG_LAST_CONFIG_NAME_ITEM->text.value, INDIGO_VALUE_SIZE)
		) {
			AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			AGENT_CONFIG_LAST_CONFIG_NAME_ITEM->text.value[0] = '\0';
			indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (!strncmp(property->name, "AGENT_CONFIG", 12)) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
		DEVICE_PRIVATE_DATA->restore_properties[DEVICE_PRIVATE_DATA->restore_count++] = indigo_safe_malloc_copy(sizeof(indigo_property) + property->count * sizeof(indigo_item), property);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		indigo_set_timer(device, 0, process_configuration_property, NULL);
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_CONFIG_SETUP_PROPERTY);
	indigo_release_property(AGENT_CONFIG_SAVE_PROPERTY);
	indigo_release_property(AGENT_CONFIG_DELETE_PROPERTY);
	indigo_release_property(AGENT_CONFIG_LAST_CONFIG_PROPERTY);
	indigo_release_property(AGENT_CONFIG_LOAD_PROPERTY);
	indigo_release_property(AGENT_CONFIG_DRIVERS_PROPERTY);
	indigo_release_property(AGENT_CONFIG_PROFILES_PROPERTY);
	for (int i = 0; i < MAX_AGENTS; i++)
		if (AGENT_CONFIG_AGENTS_PROPERTIES[i]) {
			indigo_release_property(AGENT_CONFIG_AGENTS_PROPERTIES[i]);
		}
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->restore_mutex);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->data_mutex);
	return indigo_agent_detach(device);;
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static void update_drivers(indigo_device *device, indigo_property *property) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
	indigo_delete_property(device, AGENT_CONFIG_DRIVERS_PROPERTY, NULL);
	AGENT_CONFIG_DRIVERS_PROPERTY = indigo_resize_property(AGENT_CONFIG_DRIVERS_PROPERTY, property->count);
	memcpy(AGENT_CONFIG_DRIVERS_PROPERTY->items, property->items, property->count * sizeof(indigo_item));
	strcpy(DEVICE_PRIVATE_DATA->server, property->device);
	indigo_define_property(device, AGENT_CONFIG_DRIVERS_PROPERTY, NULL);
	AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_IDLE_STATE;
	indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
}

static void add_profile(indigo_device *device, indigo_property *property) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
	indigo_delete_property(device, AGENT_CONFIG_PROFILES_PROPERTY, NULL);
	indigo_item *profile = NULL;
	for (int i = 0; i < AGENT_CONFIG_PROFILES_PROPERTY->count; i++) {
		indigo_item *item = AGENT_CONFIG_PROFILES_PROPERTY->items + i;
		if (!strcmp(item->name, property->device)) {
			profile = item;
			break;
		}
	}
	if (profile == NULL) {
		AGENT_CONFIG_PROFILES_PROPERTY = indigo_resize_property(AGENT_CONFIG_PROFILES_PROPERTY, AGENT_CONFIG_PROFILES_PROPERTY->count + 1);
		profile = AGENT_CONFIG_PROFILES_PROPERTY->items + AGENT_CONFIG_PROFILES_PROPERTY->count - 1;
		indigo_init_text_item(profile, property->device, property->device, "");
	}
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		if (item->sw.value) {
			indigo_set_text_item_value(profile, item->name);
			break;
		}
	}
	indigo_define_property(device, AGENT_CONFIG_PROFILES_PROPERTY, NULL);
	AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_IDLE_STATE;
	indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
}

static void add_device(indigo_device *device, indigo_property *property) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
	indigo_property *agent = NULL;
	char name[INDIGO_NAME_SIZE];
	sprintf(name, AGENT_CONFIG_PROPERTY_NAME, property->device);
	for (int i = 0; i < MAX_AGENTS; i++) {
		indigo_property *prop = AGENT_CONFIG_AGENTS_PROPERTIES[i];
		if (prop && !strcmp(prop->name, name)) {
			agent = prop;
			indigo_delete_property(device, agent, NULL);
			break;
		}
	}
	if (agent == NULL) {
		for (int i = 0; i < MAX_AGENTS; i++) {
			if (AGENT_CONFIG_AGENTS_PROPERTIES[i] == NULL) {
				agent = indigo_init_text_property(NULL, device->name, name, "Configuration", property->device, INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
				agent->count = 0;
				AGENT_CONFIG_AGENTS_PROPERTIES[i] = agent;
				break;
			}
		}
	}
	if (agent == NULL) {
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Too many agents detected");
		return;
	}
	indigo_item *filter = NULL;
	for (int i = 0; i < agent->count; i++) {
		indigo_item *item = agent->items + i;
		if (!strcmp(item->name, property->name)) {
			filter = item;
			break;
		}
	}
	if (filter == NULL) {
		for (int i = 0; i < MAX_AGENTS; i++) {
			if (AGENT_CONFIG_AGENTS_PROPERTIES[i] == agent) {
				agent = indigo_resize_property(agent, agent->count + 1);
				AGENT_CONFIG_AGENTS_PROPERTIES[i] = agent;
				filter = agent->items +  agent->count - 1;
				indigo_init_text_item(filter, property->name, property->label, "");
				break;
			}
		}
	}
	*filter->text.value = 0;
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		if (item->sw.value) {
			if (*filter->text.value)
				strcat(filter->text.value, ";");
			if (strcmp(item->name, "NONE"))
				strcat(filter->text.value, item->name);
		}
	}
	agent->state = property->state;
	indigo_define_property(device, agent, NULL);
	AGENT_CONFIG_LAST_CONFIG_PROPERTY->state = INDIGO_IDLE_STATE;
	indigo_update_property(device, AGENT_CONFIG_LAST_CONFIG_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strchr(property->device, '@') == NULL) {
		if (!strcmp(property->name, SERVER_DRIVERS_PROPERTY_NAME)) {
			update_drivers(agent_device, property);
		} else if (!strcmp(property->name, PROFILE_PROPERTY_NAME)) {
			add_profile(agent_device, property);
		} else if (!strncmp(property->name, "FILTER_", 6) && strstr(property->name, "_LIST")) {
			add_device(agent_device, property);
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strchr(property->device, '@') == NULL) {
		if (!strcmp(property->name, SERVER_DRIVERS_PROPERTY_NAME)) {
			update_drivers(agent_device, property);
		} else if (!strcmp(property->name, PROFILE_PROPERTY_NAME)) {
			add_profile(agent_device, property);
		} else if (!strncmp(property->name, "FILTER_", 6) && strstr(property->name, "_LIST")) {
			add_device(agent_device, property);
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strchr(property->device, '@') == NULL) {
		if (!strcmp(property->name, SERVER_DRIVERS_PROPERTY_NAME)) {
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
			indigo_delete_property(agent_device, AGENT_CONFIG_DRIVERS_PROPERTY, NULL);
			AGENT_CONFIG_DRIVERS_PROPERTY->count = 0;
			indigo_define_property(agent_device, AGENT_CONFIG_DRIVERS_PROPERTY, NULL);
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		}
		if (*property->name == 0 || !strcmp(property->name, PROFILE_PROPERTY_NAME)) {
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
			indigo_delete_property(agent_device, AGENT_CONFIG_PROFILES_PROPERTY, NULL);
			for (int i = 0; i < AGENT_CONFIG_PROFILES_PROPERTY->count; i++) {
				indigo_item *item = AGENT_CONFIG_PROFILES_PROPERTY->items + i;
				if (!strcmp(item->name, property->device)) {
					int count = AGENT_CONFIG_PROFILES_PROPERTY->count - i - 1;
					if (count > 0)
						memmove(AGENT_CONFIG_PROFILES_PROPERTY->items + i, AGENT_CONFIG_PROFILES_PROPERTY->items + i + 1, count * sizeof(indigo_item));
					AGENT_CONFIG_PROFILES_PROPERTY->count--;
					break;
				}
			}
			indigo_define_property(agent_device, AGENT_CONFIG_PROFILES_PROPERTY, NULL);
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		}
		if (*property->name == 0 || (!strncmp(property->name, "FILTER_", 6) && strstr(property->name, "_LIST"))) {
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->data_mutex);
			for (int i = 0; i < MAX_AGENTS; i++) {
				indigo_property *agent = AGENT_CONFIG_AGENTS_PROPERTIES[i];
				if (agent && !strcmp(agent->label, property->device)) {
					if (*property->name == 0) {
						indigo_delete_property(agent_device, agent, NULL);
						indigo_release_property(agent);
						AGENT_CONFIG_AGENTS_PROPERTIES[i] = NULL;
					} else {
						indigo_delete_property(agent_device, agent, NULL);
						for (int j = 0; j < agent->count; j++) {
							indigo_item *item = agent->items + j;
							if (!strcmp(item->name, property->name)) {
								int count = agent->count - j - 1;
								if (count > 0)
									memmove(agent->items + j, agent->items + j + 1, count * sizeof(indigo_item));
								agent->count--;
								break;
							}
						}
						indigo_define_property(agent_device, agent, NULL);
					}
					break;
				}
			}
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->data_mutex);
		}
	}
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

indigo_result indigo_agent_config(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		CONFIG_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		CONFIG_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		NULL,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		NULL
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, CONFIG_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(config_agent_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
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
