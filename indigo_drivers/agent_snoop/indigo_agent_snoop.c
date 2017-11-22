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

#define DEVICE_PRIVATE_DATA					((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA					((agent_private_data *)client->client_context)

typedef struct rule {
	char source_device_name[INDIGO_NAME_SIZE];
	char source_property_name[INDIGO_NAME_SIZE];
	indigo_device *source_device;
	indigo_property *source_property;
	char target_device_name[INDIGO_NAME_SIZE];
	char target_property_name[INDIGO_NAME_SIZE];
	indigo_device *target_device;
	indigo_property *target_property;
	bool is_active;
	struct rule *next;
} rule;

typedef struct {
	rule *rules;
} agent_private_data;

static rule *add_rule(indigo_client *client, const char *source_device, const char *source_property, const char *target_device, const char *target_property, bool is_active) {
	rule *result = malloc(sizeof(rule));
	strncpy(result->source_device_name, source_device, INDIGO_NAME_SIZE);
	strncpy(result->source_property_name, source_property, INDIGO_NAME_SIZE);
	result->source_device = NULL;
	result->source_property = NULL;
	strncpy(result->target_device_name, target_device, INDIGO_NAME_SIZE);
	strncpy(result->target_property_name, target_property, INDIGO_NAME_SIZE);
	result->target_device = NULL;
	result->target_property = NULL;
	result->is_active = is_active;
	result->next = CLIENT_PRIVATE_DATA->rules;
	CLIENT_PRIVATE_DATA->rules = result;
	return result;
}

static indigo_result forward_property(indigo_client *client, rule *r) {
	int size = sizeof(indigo_property) + r->source_property->count * sizeof(indigo_item);
	indigo_property *property = malloc(size);
	memcpy(property, r->source_property, size);
	strcpy(property->device, r->target_device_name);
	strcpy(property->name, r->target_property_name);
	int ll = indigo_get_log_level();
	indigo_set_log_level(INDIGO_LOG_TRACE);
	indigo_trace_property("forward", property, false, true);
	indigo_set_log_level(ll);
	indigo_result result = r->target_device->last_result = r->target_device->change_property(r->target_device, client, property);
	free(property);
	return result;
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// TBD
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	// TBD
	return indigo_agent_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// TBD
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_agent_detach(device);
}

static indigo_result agent_client_attach(indigo_client *client) {
	// Example only
	add_rule(client, "Mount Simulator", "MOUNT_HORIZONTAL_COORDINATES", "Dome Simulator", "DOME_HORIZONTAL_COORDINATES", true);
	return INDIGO_OK;
}

static indigo_result agent_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	rule *r = CLIENT_PRIVATE_DATA->rules;
	bool rule_complete = false;
	while (r) {
		if (!strcmp(r->source_device_name, property->device) && !strcmp(r->source_property_name, property->name)) {
			r->source_device = device;
			r->source_property = property;
			if (r->target_property)
				rule_complete = true;
			break;
		} else if (!strcmp(r->target_device_name, property->device) && !strcmp(r->target_property_name, property->name)) {
			r->target_device = device;
			r->target_property = property;
			if (r->source_property)
				rule_complete = true;
			break;
		} else {
			r = r->next;
		}
	}
	if (rule_complete && r->is_active) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Rule '%s'.%s > '%s'.%s is active", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
		forward_property(client, r);
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	rule *r = CLIENT_PRIVATE_DATA->rules;
	while (r) {
		if (r->source_property == property) {
			if (r->is_active) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Rule '%s'.%s > '%s'.%s used", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
				return forward_property(client, r);
			}
			break;
		} else {
			r = r->next;
		}
	}
	return INDIGO_OK;
}

indigo_result agent_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	rule *r = CLIENT_PRIVATE_DATA->rules;
	bool rule_incomplete = false;
	while (r) {
		if (!strcmp(r->source_device_name, property->device) && !strcmp(r->source_property_name, property->name)) {
			r->source_device = NULL;
			r->source_property = NULL;
			if (r->target_property)
				rule_incomplete = true;
			break;
		} else if (!strcmp(r->target_device_name, property->device) && !strcmp(r->target_property_name, property->name)) {
			r->target_device = NULL;
			r->target_property = NULL;
			if (r->source_property)
				rule_incomplete = true;
			break;
		} else {
			r = r->next;
		}
	}
	if (rule_incomplete && r->is_active) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Rule '%s'.%s > '%s'.%s isn't active", r->source_device_name, r->source_property_name, r->target_device_name, r->target_property_name);
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
	static indigo_device agent_device_template = {
		SNOOP_AGENT_NAME, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	};
	
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
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			
			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
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
