// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO ASCOM ALPACA bridge agent
 \file indigo_agent_alpaca.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_alpaca"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <sys/socket.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_server_tcp.h>

#include "indigo_agent_alpaca.h"

#define INDIGO_PRINTF(...) if (!indigo_printf(__VA_ARGS__)) goto failure

#define PRIVATE_DATA													private_data

#define AGENT_DISCOVERY_PROPERTY							(PRIVATE_DATA->discovery_property)
#define AGENT_DISCOVERY_PORT_ITEM							(AGENT_DISCOVERY_PROPERTY->items+0)

typedef struct {
	indigo_property *discovery_property;
	pthread_mutex_t mutex;
} agent_private_data;

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

// -------------------------------------------------------------------------------- ALPACA bridge implementation

static void alpaca_setup_handler(int socket, char *method, char *path) {
	INDIGO_PRINTF(socket, "HTTP/1.1 301 Moved Permanently\r\n");
	INDIGO_PRINTF(socket, "Location: /mng.html\r\n");
	INDIGO_PRINTF(socket, "Content-Type: text/plain\r\n");
	INDIGO_PRINTF(socket, "\r\n");
	INDIGO_PRINTF(socket, "Redirecting to /mng.html\r\n", path);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "% -> OK", path);
	return;
failure:
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s -> Failed", path);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		AGENT_DISCOVERY_PROPERTY = indigo_init_number_property(NULL, device->name, "AGENT_ALPACA_DISCOVERY", MAIN_GROUP, "Discovery Configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_DISCOVERY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_DISCOVERY_PORT_ITEM, "PORT", "Discovery port", 0, 0xFFFF, 0, 32227);
		// --------------------------------------------------------------------------------
		indigo_server_add_handler("/setup", alpaca_setup_handler);
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client == agent_client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_DISCOVERY_PROPERTY, property))
		indigo_define_property(device, AGENT_DISCOVERY_PROPERTY, NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == agent_client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_DISCOVERY_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_DISCOVERY_PROPERTY, property, false);
		// TBD
		AGENT_DISCOVERY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_DISCOVERY_PROPERTY, NULL);
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_server_remove_resource("/setup");
	indigo_release_property(AGENT_DISCOVERY_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	// --------------------------------------------------------------------------------
	// TBD
	// --------------------------------------------------------------------------------
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	// --------------------------------------------------------------------------------
	// TBD
	// --------------------------------------------------------------------------------
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	// --------------------------------------------------------------------------------
	// TBD
	// --------------------------------------------------------------------------------
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

indigo_result indigo_agent_alpaca(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ALPACA_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ALPACA_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		NULL,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		NULL
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ASCOM ALPACA bridge agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

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
