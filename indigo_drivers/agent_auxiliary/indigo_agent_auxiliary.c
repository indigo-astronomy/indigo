// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Auxiliary devices control agent
 \file indigo_agent_auxiliary.c
 */

#define DRIVER_VERSION 0x03000006
#define DRIVER_NAME	"indigo_agent_auxiliary"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>

#include "indigo_agent_auxiliary.h"

#define DEVICE_PRIVATE_DATA										((auxiliary_agent_private_data *)device->private_data)

#define AGENT_AUX_SAFETY_PROPERTY							(DEVICE_PRIVATE_DATA->agent_safety_property)
#define AGENT_AUX_SAFETY_SAFE_ITEM						(AGENT_AUX_SAFETY_PROPERTY->items + 0)

typedef struct {
	indigo_property *agent_safety_property;
} auxiliary_agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static bool validate_related_agent(indigo_device *device, indigo_property *info_property, int mask) {
	if (!strncmp(info_property->device, "Imager Agent", 12)) {
		return true;
	}
	return false;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_AUX_1_LIST_PROPERTY->hidden = false;
		FILTER_AUX_2_LIST_PROPERTY->hidden = false;
		FILTER_AUX_3_LIST_PROPERTY->hidden = false;
		FILTER_AUX_4_LIST_PROPERTY->hidden = false;
		FILTER_DEVICE_CONTEXT->validate_related_agent = validate_related_agent;
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- Safety, not computed yet
		AGENT_AUX_SAFETY_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_AUX_SAFETY_PROPERTY_NAME, AGENT_MAIN_GROUP, "Safety", INDIGO_IDLE_STATE, 1);
		indigo_init_light_item(AGENT_AUX_SAFETY_SAFE_ITEM, AGENT_AUX_SAFETY_SAFE_ITEM_NAME, "Safe", INDIGO_IDLE_STATE);
		// --------------------------------------------------------------------------------
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		CONNECTION_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client) {
		return INDIGO_OK;
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_AUX_SAFETY_PROPERTY);
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_cancel_pending_handlers(device);
	indigo_cancel_all_timers(device);
	indigo_release_property(AGENT_AUX_SAFETY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_filter_device_detach(device);
}


// -------------------------------------------------------------------------------- Initialization

static auxiliary_agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_auxiliary(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		AUX_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		indigo_filter_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		AUX_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		indigo_filter_define_property,
		indigo_filter_update_property,
		indigo_filter_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, AUX_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(auxiliary_agent_private_data));
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
