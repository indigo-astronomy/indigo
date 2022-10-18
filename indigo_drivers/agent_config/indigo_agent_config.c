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

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_config"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_agent.h>

#include "indigo_agent_config.h"

#define DEVICE_PRIVATE_DATA												((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA												((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_CONFIG_SAVE_PROPERTY								(DEVICE_PRIVATE_DATA->save_config)
#define AGENT_CONFIG_SAVE_NAME_ITEM			    			(AGENT_CONFIG_SAVE_PROPERTY->items+0)

#define AGENT_CONFIG_LOAD_PROPERTY								(DEVICE_PRIVATE_DATA->load_config)

#define AGENT_CONFIG_REMOVE_PROPERTY							(DEVICE_PRIVATE_DATA->remove_config)
#define AGENT_CONFIG_REMOVE_NAME_ITEM			    		(AGENT_CONFIG_REMOVE_PROPERTY->items+0)


typedef struct {
	indigo_property *save_config;
	indigo_property *load_config;
	indigo_property *remove_config;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		AGENT_CONFIG_SAVE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_CONFIG_SAVE_PROPERTY_NAME, "Agent", "Save", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_CONFIG_SAVE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_CONFIG_SAVE_NAME_ITEM, AGENT_CONFIG_SAVE_NAME_ITEM_NAME, "Name", "");
		AGENT_CONFIG_LOAD_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CONFIG_LOAD_PROPERTY_NAME, "Agent", "Load", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 16);
		if (AGENT_CONFIG_LOAD_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_CONFIG_LOAD_PROPERTY->count = 0;
		
		// TBD
		
		AGENT_CONFIG_REMOVE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_CONFIG_REMOVE_PROPERTY_NAME, "Agent", "Remove", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_CONFIG_REMOVE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_CONFIG_REMOVE_NAME_ITEM, AGENT_CONFIG_REMOVE_NAME_ITEM_NAME, "Name", "");
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);;
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(AGENT_CONFIG_SAVE_PROPERTY, property))
		indigo_define_property(device, AGENT_CONFIG_SAVE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_CONFIG_LOAD_PROPERTY, property))
		indigo_define_property(device, AGENT_CONFIG_LOAD_PROPERTY, NULL);
	if (indigo_property_match(AGENT_CONFIG_REMOVE_PROPERTY, property))
		indigo_define_property(device, AGENT_CONFIG_REMOVE_PROPERTY, NULL);
	return INDIGO_OK;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(AGENT_CONFIG_SAVE_PROPERTY, property)) {
		// TBD
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CONFIG_LOAD_PROPERTY, property)) {
		// TBD
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_CONFIG_REMOVE_PROPERTY, property)) {
		// TBD
		return INDIGO_OK;
	}
	return INDIGO_OK;
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_CONFIG_SAVE_PROPERTY);
	indigo_release_property(AGENT_CONFIG_LOAD_PROPERTY);
	indigo_release_property(AGENT_CONFIG_REMOVE_PROPERTY);
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	// TBD
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	// TBD
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

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
		NULL,
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
			private_data = indigo_safe_malloc(sizeof(agent_private_data));
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
