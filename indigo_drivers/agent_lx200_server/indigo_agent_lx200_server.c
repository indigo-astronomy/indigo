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

/** INDIGO LX200 Server agent
 \file indigo_agent_lx200_server.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_lx200_server"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_agent_lx200_server.h"

#define DEVICE_PRIVATE_DATA										((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((agent_private_data *)client->client_context)

#define LX200_DEVICES_PROPERTY								(DEVICE_PRIVATE_DATA->devices_property)
#define LX200_DEVICES_MOUNT_ITEM							(LX200_DEVICES_PROPERTY->items+0)
#define LX200_DEVICES_GUIDER_ITEM							(LX200_DEVICES_PROPERTY->items+1)

#define LX200_SERVER_PROPERTY									(DEVICE_PRIVATE_DATA->server_property)
#define LX200_SERVER_PORT_ITEM								(LX200_SERVER_PROPERTY->items+0)

typedef struct {
	indigo_property *devices_property;
	indigo_property *server_property;
	indigo_device *device;
	indigo_client *client;
} agent_private_data;

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		LX200_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, LX200_DEVICES_PROPERTY_NAME, MAIN_GROUP, "Devices", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
		if (LX200_DEVICES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(LX200_DEVICES_MOUNT_ITEM, LX200_DEVICES_MOUNT_ITEM_NAME, "Mount", "Mount Simulator");
		indigo_init_text_item(LX200_DEVICES_GUIDER_ITEM, LX200_DEVICES_GUIDER_ITEM_NAME, "Guider", "Mount Simulator (guider)");

		LX200_SERVER_PROPERTY = indigo_init_text_property(NULL, device->name, LX200_SERVER_PROPERTY_NAME, MAIN_GROUP, "Server", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
		if (LX200_SERVER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(LX200_SERVER_PORT_ITEM, LX200_SERVER_PORT_ITEM_NAME, "Server port", "4030");
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
		if (indigo_property_match(LX200_DEVICES_PROPERTY, property))
			indigo_define_property(device, LX200_DEVICES_PROPERTY, NULL);
		if (indigo_property_match(LX200_SERVER_PROPERTY, property))
			indigo_define_property(device, LX200_SERVER_PROPERTY, NULL);
	}
	return result;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(LX200_DEVICES_PROPERTY, property)) {
		indigo_property_copy_values(LX200_DEVICES_PROPERTY, property, false);

		LX200_DEVICES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LX200_DEVICES_PROPERTY, NULL);
	} else if (indigo_property_match(LX200_SERVER_PROPERTY, property)) {
		indigo_property_copy_values(LX200_SERVER_PROPERTY, property, false);
		
		LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LX200_SERVER_PROPERTY, NULL);
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(LX200_DEVICES_PROPERTY);
	indigo_release_property(LX200_SERVER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_agent_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_client_attach(indigo_client *client) {
	return INDIGO_OK;
}

static indigo_result agent_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;

	return INDIGO_OK;
}

static indigo_result agent_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;

	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;

	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_lx200_server(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		LX200_SERVER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		LX200_SERVER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		agent_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "LX200 Server Agent", __FUNCTION__, DRIVER_VERSION, last_action);

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
