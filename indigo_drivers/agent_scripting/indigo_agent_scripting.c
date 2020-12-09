// Copyright (c) 2020 CloudMakers, s. r. o.
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

/** INDIGO Scripting agent
 \file indigo_agent_scripting.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_scripting"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>

#include "duktape.h"
#include "indigo_agent_scripting.h"

#define PRIVATE_DATA													((agent_private_data *)device->private_data)

#define AGENT_SCRIPTING_SCRIPT_PROPERTY				(PRIVATE_DATA->agent_script_property)
#define AGENT_SCRIPTING_SCRIPT_ITEM				  	(AGENT_SCRIPTING_SCRIPT_PROPERTY->items+0)

typedef struct {
	indigo_property *agent_script_property;
} agent_private_data;

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		indigo_save_property(device, NULL, AGENT_SCRIPTING_SCRIPT_PROPERTY);
		if (DEVICE_CONTEXT->property_save_file_handle) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			close(DEVICE_CONTEXT->property_save_file_handle);
			DEVICE_CONTEXT->property_save_file_handle = 0;
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	}
}

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		AGENT_SCRIPTING_SCRIPT_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME, MAIN_GROUP, "Script", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_SCRIPTING_SCRIPT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_SCRIPTING_SCRIPT_ITEM, AGENT_SCRIPTING_SCRIPT_ITEM_NAME, "Script", "");
		strcpy(AGENT_SCRIPTING_SCRIPT_ITEM->hints, "widget: multiline-edit-box");
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(AGENT_SCRIPTING_SCRIPT_PROPERTY, property))
		indigo_define_property(device, AGENT_SCRIPTING_SCRIPT_PROPERTY, NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

static duk_ret_t send_message(duk_context *ctx) {
	const char *message = duk_to_string(ctx, 0);
	indigo_send_message(agent_device, "%s", message);
	return 0;
}

static duk_ret_t change_text_property(duk_context *ctx) {
	const char *device = duk_to_string(ctx, 0);
	const char *property = duk_to_string(ctx, 1);
	char *names[INDIGO_MAX_ITEMS];
	char *values[INDIGO_MAX_ITEMS];
	duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
	int i = 0;
	while (duk_next(ctx, -1, true)) {
		const char *key = duk_to_string(ctx, -2);
		const char *value = duk_to_string(ctx, -1);
		names[i] = strdup(key);
		values[i] = strdup(value);
		duk_pop_2(ctx);
		i++;
	}
	indigo_change_text_property(agent_client, device, property, i, (const char **)names, (const char **)values);
	for (int j = 0; j < i; j++) {
		if (names[j])
			free(names[j]);
		if (values[j])
			free(values[j]);
	}
	return 0;
}

static duk_ret_t change_number_property(duk_context *ctx) {
	const char *device = duk_to_string(ctx, 0);
	const char *property = duk_to_string(ctx, 1);
	char *names[INDIGO_MAX_ITEMS];
	double values[INDIGO_MAX_ITEMS];
	duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
	int i = 0;
	while (duk_next(ctx, -1, true)) {
		const char *key = duk_to_string(ctx, -2);
		double value = duk_to_number(ctx, -1);
		names[i] = strdup(key);
		values[i] = value;
		duk_pop_2(ctx);
		i++;
	}
	indigo_change_number_property(agent_client, device, property, i, (const char **)names, (const double *)values);
	for (int j = 0; j < i; j++) {
		if (names[j])
			free(names[j]);
	}
	return 0;
}

static duk_ret_t change_switch_property(duk_context *ctx) {
	const char *device = duk_to_string(ctx, 0);
	const char *property = duk_to_string(ctx, 1);
	char *names[INDIGO_MAX_ITEMS];
	bool values[INDIGO_MAX_ITEMS];
	duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
	int i = 0;
	while (duk_next(ctx, -1, true)) {
		const char *key = duk_to_string(ctx, -2);
		double value = duk_to_boolean(ctx, -1);
		names[i] = strdup(key);
		values[i] = value;
		duk_pop_2(ctx);
		i++;
	}
	indigo_change_switch_property(agent_client, device, property, i, (const char **)names, (const bool *)values);
	for (int j = 0; j < i; j++) {
		if (names[j])
			free(names[j]);
	}
	return 0;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(AGENT_SCRIPTING_SCRIPT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SITE_DATA_SOURCE
		indigo_property_copy_values(AGENT_SCRIPTING_SCRIPT_PROPERTY, property, false);
		save_config(device);
		AGENT_SCRIPTING_SCRIPT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_SCRIPT_PROPERTY, NULL);
		char *script = malloc(INDIGO_VALUE_SIZE + (AGENT_SCRIPTING_SCRIPT_ITEM->text.extra_value ? AGENT_SCRIPTING_SCRIPT_ITEM->text.extra_size : 0));
		if (script) {
			strcpy(script, AGENT_SCRIPTING_SCRIPT_ITEM->text.value);
			if (AGENT_SCRIPTING_SCRIPT_ITEM->text.extra_value) {
				strcat(script, AGENT_SCRIPTING_SCRIPT_ITEM->text.extra_value);
			}
			duk_context *ctx = duk_create_heap_default();
			if (ctx) {
				duk_push_c_function(ctx, send_message, 1);
				duk_put_global_string(ctx, "indigo_send_message");
				duk_push_c_function(ctx, change_text_property, 3);
				duk_put_global_string(ctx, "indigo_change_text_property");
				duk_push_c_function(ctx, change_number_property, 3);
				duk_put_global_string(ctx, "indigo_change_number_property");
				duk_push_c_function(ctx, change_switch_property, 3);
				duk_put_global_string(ctx, "indigo_change_switch_property");
				if (duk_peval_string(ctx, script)) {
					indigo_send_message(device, "%s", duk_safe_to_string(ctx, -1));
				}
				duk_destroy_heap(ctx);
				AGENT_SCRIPTING_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AGENT_SCRIPTING_SCRIPT_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			free(script);
		} else {
			AGENT_SCRIPTING_SCRIPT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, AGENT_SCRIPTING_SCRIPT_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result agent_enable_blob(indigo_device *device, indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	assert(device != NULL);
	return INDIGO_OK;
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_SCRIPTING_SCRIPT_PROPERTY);
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_client_attach(indigo_client *client) {
	assert(client != NULL);
	indigo_property all_properties;
	memset(&all_properties, 0, sizeof(all_properties));
	indigo_enumerate_properties(client, &all_properties);
	return INDIGO_OK;
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return INDIGO_OK;
}

static indigo_result agent_send_message(indigo_client *client, indigo_device *device, const char *message) {
	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

indigo_result indigo_agent_scripting(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ALIGNMENT_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		agent_enable_blob,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ALIGNMENT_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		agent_send_message,
		agent_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, ALIGNMENT_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

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
