// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO Snoop agent
 \file indigo_agent_snoop.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_snoop"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_agent_snoop.h"

#define DEVICE_PRIVATE_DATA											((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA											((agent_private_data *)client->client_context)

#define SNOOP_ADD_RULE_PROPERTY									(DEVICE_PRIVATE_DATA->add_rule_property)
#define SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM				(SNOOP_ADD_RULE_PROPERTY->items+0)
#define SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM			(SNOOP_ADD_RULE_PROPERTY->items+1)
#define SNOOP_ADD_RULE_TARGET_DEVICE_ITEM				(SNOOP_ADD_RULE_PROPERTY->items+2)
#define SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM			(SNOOP_ADD_RULE_PROPERTY->items+3)

#define SNOOP_REMOVE_RULE_PROPERTY							(DEVICE_PRIVATE_DATA->remove_rule_property)
#define SNOOP_REMOVE_RULE_SOURCE_DEVICE_ITEM		(SNOOP_REMOVE_RULE_PROPERTY->items+0)
#define SNOOP_REMOVE_RULE_SOURCE_PROPERTY_ITEM	(SNOOP_REMOVE_RULE_PROPERTY->items+1)
#define SNOOP_REMOVE_RULE_TARGET_DEVICE_ITEM		(SNOOP_REMOVE_RULE_PROPERTY->items+2)
#define SNOOP_REMOVE_RULE_TARGET_PROPERTY_ITEM	(SNOOP_REMOVE_RULE_PROPERTY->items+3)

#define SNOOP_RULES_PROPERTY										(DEVICE_PRIVATE_DATA->rules_property)

typedef struct rule {
	char source_device_name[INDIGO_NAME_SIZE];
	char source_property_name[INDIGO_NAME_SIZE];
	indigo_device *source_device;
	indigo_property *source_property;
	char target_device_name[INDIGO_NAME_SIZE];
	char target_property_name[INDIGO_NAME_SIZE];
	indigo_device *target_device;
	indigo_property *target_property;
	indigo_property_state state;
	struct rule *next;
} rule;

typedef struct {
	indigo_property *add_rule_property;
	indigo_property *remove_rule_property;
	indigo_property *rules_property;
	indigo_device *device;
	indigo_client *client;
	rule *rules;
} agent_private_data;

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result forward_property(indigo_device *device, indigo_client *client, rule *r) {
	assert(client != NULL);
	assert(r != NULL);
	assert(r->source_device != NULL);
	assert(r->source_property != NULL);
	assert(r->target_device != NULL);
	assert(r->target_property != NULL);
	int size = sizeof(indigo_property) + r->source_property->count * sizeof(indigo_item);
	indigo_property *property = malloc(size);
	assert(property != NULL);
	memcpy(property, r->source_property, size);
	strncpy(property->device, r->target_device_name, INDIGO_NAME_SIZE);
	strncpy(property->name, r->target_property_name, INDIGO_NAME_SIZE);
	indigo_trace_property("Property set by rule", property, false, true);
	indigo_result result = r->target_device->last_result = r->target_device->change_property(r->target_device, client, property);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Forward: '%s'.%s > '%s'.%s", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
	free(property);
	return result;
}

static void sync_rules(indigo_device *device) {
	rule *r = DEVICE_PRIVATE_DATA->rules;
	int index = 0;
	char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
	while (r) {
		snprintf(name, INDIGO_NAME_SIZE, "RULE_%d", index);
		snprintf(label, INDIGO_VALUE_SIZE, "%s.%s > %s.%s", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
		indigo_init_light_item(SNOOP_RULES_PROPERTY->items + index, name, label, r->state);
		index++;
		r = r->next;
	}
	SNOOP_RULES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_delete_property(device, SNOOP_RULES_PROPERTY, NULL);
	indigo_define_property(device, SNOOP_RULES_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		SNOOP_ADD_RULE_PROPERTY = indigo_init_text_property(NULL, device->name, SNOOP_ADD_RULE_PROPERTY_NAME, MAIN_GROUP, "Add rule", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
		if (SNOOP_ADD_RULE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM, SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM_NAME, "Source device", "Mount Simulator");
		indigo_init_text_item(SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM, SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM_NAME, "Source property", "MOUNT_EQUATORIAL_COORDINATES");
		indigo_init_text_item(SNOOP_ADD_RULE_TARGET_DEVICE_ITEM, SNOOP_ADD_RULE_TARGET_DEVICE_ITEM_NAME, "Target device", "Dome Simulator");
		indigo_init_text_item(SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM, SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM_NAME, "Target property", "DOME_EQUATORIAL_COORDINATES");
		SNOOP_REMOVE_RULE_PROPERTY = indigo_init_text_property(NULL, device->name, SNOOP_REMOVE_RULE_PROPERTY_NAME, MAIN_GROUP, "Remove rule", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
		if (SNOOP_REMOVE_RULE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(SNOOP_REMOVE_RULE_SOURCE_DEVICE_ITEM, SNOOP_REMOVE_RULE_SOURCE_DEVICE_ITEM_NAME, "Source device", "Mount Simulator");
		indigo_init_text_item(SNOOP_REMOVE_RULE_SOURCE_PROPERTY_ITEM, SNOOP_REMOVE_RULE_SOURCE_PROPERTY_ITEM_NAME, "Source property", "MOUNT_EQUATORIAL_COORDINATES");
		indigo_init_text_item(SNOOP_REMOVE_RULE_TARGET_DEVICE_ITEM, SNOOP_REMOVE_RULE_TARGET_DEVICE_ITEM_NAME, "Target device", "Dome Simulator");
		indigo_init_text_item(SNOOP_REMOVE_RULE_TARGET_PROPERTY_ITEM, SNOOP_REMOVE_RULE_TARGET_PROPERTY_ITEM_NAME, "Target property", "DOME_EQUATORIAL_COORDINATES");
		SNOOP_RULES_PROPERTY = indigo_init_light_property(NULL, device->name, SNOOP_RULES_PROPERTY_NAME, MAIN_GROUP, "Rules", INDIGO_IDLE_STATE, 0);
		if (SNOOP_RULES_PROPERTY == NULL)
			return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client!= NULL && client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	indigo_result result = INDIGO_OK;
	if ((result = indigo_agent_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (indigo_property_match(SNOOP_ADD_RULE_PROPERTY, property))
			indigo_define_property(device, SNOOP_ADD_RULE_PROPERTY, NULL);
		if (indigo_property_match(SNOOP_REMOVE_RULE_PROPERTY, property))
			indigo_define_property(device, SNOOP_REMOVE_RULE_PROPERTY, NULL);
		if (indigo_property_match(SNOOP_RULES_PROPERTY, property))
			indigo_define_property(device, SNOOP_RULES_PROPERTY, NULL);
	}
	return result;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(SNOOP_ADD_RULE_PROPERTY, property)) {
		indigo_property_copy_values(SNOOP_ADD_RULE_PROPERTY, property, false);
		rule *r = DEVICE_PRIVATE_DATA->rules;
		while (r) {
			if (!strcmp(r->source_device_name, SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM->text.value) &&
				!strcmp(r->source_property_name, SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM->text.value) &&
				!strcmp(r->target_device_name, SNOOP_ADD_RULE_TARGET_DEVICE_ITEM->text.value) &&
				!strcmp(r->target_property_name, SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM->text.value)) {
					SNOOP_ADD_RULE_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, SNOOP_ADD_RULE_PROPERTY, "Duplicate rule");
					return INDIGO_OK;
				}
			r = r->next;
		}
		r = malloc(sizeof(rule));
		assert(r != NULL);
		strncpy(r->source_device_name, SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM->text.value, INDIGO_NAME_SIZE);
		strncpy(r->source_property_name, SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM->text.value, INDIGO_NAME_SIZE);
		r->source_device = NULL;
		r->source_property = NULL;
		strncpy(r->target_device_name, SNOOP_ADD_RULE_TARGET_DEVICE_ITEM->text.value, INDIGO_NAME_SIZE);
		strncpy(r->target_property_name, SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM->text.value, INDIGO_NAME_SIZE);
		r->target_device = NULL;
		r->target_property = NULL;
		r->state = INDIGO_IDLE_STATE;
		r->next = DEVICE_PRIVATE_DATA->rules;
		DEVICE_PRIVATE_DATA->rules = r;
		SNOOP_RULES_PROPERTY = indigo_resize_property(SNOOP_RULES_PROPERTY, SNOOP_RULES_PROPERTY->count + 1);
		sync_rules(device);
		SNOOP_ADD_RULE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SNOOP_ADD_RULE_PROPERTY, NULL);
		indigo_property INDIGO_ALL_PROPERTIES;
		memset(&INDIGO_ALL_PROPERTIES, 0, sizeof(INDIGO_ALL_PROPERTIES));
		INDIGO_ALL_PROPERTIES.version = INDIGO_VERSION_CURRENT;
		strcpy(INDIGO_ALL_PROPERTIES.device, r->source_device_name);
		indigo_enumerate_properties(DEVICE_PRIVATE_DATA->client, &INDIGO_ALL_PROPERTIES);
		strcpy(INDIGO_ALL_PROPERTIES.device, r->target_device_name);
		indigo_enumerate_properties(DEVICE_PRIVATE_DATA->client, &INDIGO_ALL_PROPERTIES);
	} else if (indigo_property_match(SNOOP_REMOVE_RULE_PROPERTY, property)) {
		indigo_property_copy_values(SNOOP_REMOVE_RULE_PROPERTY, property, false);
		rule *r = DEVICE_PRIVATE_DATA->rules;
		rule *rr = NULL;
		while (r) {
			if (!strcmp(r->source_device_name, SNOOP_REMOVE_RULE_SOURCE_DEVICE_ITEM->text.value) &&
					!strcmp(r->source_property_name, SNOOP_REMOVE_RULE_SOURCE_PROPERTY_ITEM->text.value) &&
					!strcmp(r->target_device_name, SNOOP_REMOVE_RULE_TARGET_DEVICE_ITEM->text.value) &&
					!strcmp(r->target_property_name, SNOOP_REMOVE_RULE_TARGET_PROPERTY_ITEM->text.value)) {
				break;
			}
			rr = r;
			r = r->next;
		}
		if (r) {
			if (rr)
				rr->next = r->next;
			else
				DEVICE_PRIVATE_DATA->rules = r->next;
			SNOOP_RULES_PROPERTY = indigo_resize_property(SNOOP_RULES_PROPERTY, SNOOP_RULES_PROPERTY->count - 1);
			sync_rules(device);
			SNOOP_REMOVE_RULE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, SNOOP_REMOVE_RULE_PROPERTY, NULL);
		} else {
			SNOOP_REMOVE_RULE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, SNOOP_REMOVE_RULE_PROPERTY, "No such rule");
		}
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(SNOOP_ADD_RULE_PROPERTY);
	indigo_release_property(SNOOP_REMOVE_RULE_PROPERTY);
	indigo_release_property(SNOOP_RULES_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_agent_detach(device);
}

static indigo_result agent_client_attach(indigo_client *client) {
	return INDIGO_OK;
}

static indigo_result agent_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	rule *r = CLIENT_PRIVATE_DATA->rules;
	int index = 0;
	while (r) {
		bool changed = false;
		if (!strcmp(r->source_device_name, property->device) && !strcmp(r->source_property_name, property->name)) {
			changed = r->source_device == NULL;
			r->source_device = device;
			r->source_property = property;
		} else if (!strcmp(r->target_device_name, property->device) && !strcmp(r->target_property_name, property->name)) {
			changed = r->target_device == NULL;
			r->target_device = device;
			r->target_property = property;
		}
		if (changed) {
			if (r->source_property && r->target_property) {
				CLIENT_PRIVATE_DATA->rules_property->items[index].light.value = r->state = INDIGO_OK_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, CLIENT_PRIVATE_DATA->rules_property, "Rule '%s'.%s > '%s'.%s is active", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
				forward_property(device, client, r);
			} else {
				CLIENT_PRIVATE_DATA->rules_property->items[index].light.value = r->state = INDIGO_BUSY_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, CLIENT_PRIVATE_DATA->rules_property, NULL);
			}
		}
		r = r->next;
		index++;
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	rule *r = CLIENT_PRIVATE_DATA->rules;
	while (r) {
		if (r->source_property == property) {
			if (r->target_property) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Rule '%s'.%s > '%s'.%s used", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
				return forward_property(device, client, r);
			}
			break;
		} else {
			r = r->next;
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	rule *r = CLIENT_PRIVATE_DATA->rules;
	int index = 0;
	while (r) {
		if (!strcmp(r->source_device_name, property->device) && !strcmp(r->source_property_name, property->name)) {
			r->source_device = NULL;
			r->source_property = NULL;
			if (r->target_property) {
				CLIENT_PRIVATE_DATA->rules_property->items[index].light.value = r->state = INDIGO_BUSY_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, CLIENT_PRIVATE_DATA->rules_property, "Rule '%s'.%s > '%s'.%s isn't active", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
			} else {
				CLIENT_PRIVATE_DATA->rules_property->items[index].light.value = r->state = INDIGO_IDLE_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, CLIENT_PRIVATE_DATA->rules_property, NULL);
			}
		} else if (!strcmp(r->target_device_name, property->device) && !strcmp(r->target_property_name, property->name)) {
			r->target_device = NULL;
			r->target_property = NULL;
			if (r->source_property) {
				CLIENT_PRIVATE_DATA->rules_property->items[index].light.value = r->state = INDIGO_BUSY_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, CLIENT_PRIVATE_DATA->rules_property, "Rule '%s'.%s > '%s'.%s isn't active", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
			} else {
				CLIENT_PRIVATE_DATA->rules_property->items[index].light.value = r->state = INDIGO_IDLE_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, CLIENT_PRIVATE_DATA->rules_property, NULL);
			}
		}
		r = r->next;
		index++;
	}
	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	rule *r = CLIENT_PRIVATE_DATA->rules;
	while (r) {
		rule *rr = r->next;
		free(r);
		r = rr;
	}
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_snoop(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		SNOOP_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		SNOOP_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		agent_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Snoop agent", __FUNCTION__, DRIVER_VERSION, last_action);

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
