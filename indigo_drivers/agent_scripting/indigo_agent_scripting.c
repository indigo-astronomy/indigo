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

#define DRIVER_VERSION 0x0003
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

#define SCRIPT_GROUP															"Scripts"
#define PRIVATE_DATA															private_data

#define MAX_USER_SCRIPT_COUNT											128
#define MAX_CACHED_SCRIPT_COUNT										126
#define MAX_TIMER_COUNT														32

#define AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY				(PRIVATE_DATA->agent_add_script_property)
#define AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM			(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY->items+0)
#define AGENT_SCRIPTING_ADD_SCRIPT_ITEM						(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY->items+1)

#define AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY		(PRIVATE_DATA->agent_execute_script_property)
#define AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY		(PRIVATE_DATA->agent_delete_script_property)
#define AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY		(PRIVATE_DATA->agent_on_load_script_property)
#define AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY	(PRIVATE_DATA->agent_on_unload_script_property)

#define AGENT_SCRIPTING_SCRIPT_PROPERTY(i)				(PRIVATE_DATA->agent_scripts_property[i])
#define AGENT_SCRIPTING_SCRIPT_NAME_ITEM(i)				(AGENT_SCRIPTING_SCRIPT_PROPERTY(i)->items+0)
#define AGENT_SCRIPTING_SCRIPT_ITEM(i)						(AGENT_SCRIPTING_SCRIPT_PROPERTY(i)->items+1)

static int AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME_LENGTH = strlen(AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME) - 2;

static char boot_js[] = {
#include "boot.js.dat"
	0
};

typedef struct {
	indigo_property *agent_add_script_property;
	indigo_property *agent_execute_script_property;
	indigo_property *agent_delete_script_property;
	indigo_property *agent_on_load_script_property;
	indigo_property *agent_on_unload_script_property;
	indigo_property *agent_scripts_property[MAX_USER_SCRIPT_COUNT];
	indigo_property *agent_cached_property[MAX_CACHED_SCRIPT_COUNT];
	indigo_timer *timers[MAX_TIMER_COUNT];
	duk_context *ctx;
	pthread_mutex_t mutex;
} agent_private_data;

static agent_private_data *private_data = NULL;
static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		for (int i = 0; i < MAX_USER_SCRIPT_COUNT; i++) {
			indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(i);
			if (script_property) {
				char name[INDIGO_NAME_SIZE];
				indigo_copy_name(name, script_property->name);
				indigo_copy_name(script_property->name, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME);
				indigo_save_property(device, NULL, script_property);
				indigo_copy_name(script_property->name, name);
			}
		}
		indigo_save_property(device, NULL, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY);
		indigo_save_property(device, NULL, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY);
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

// -------------------------------------------------------------------------------- Duktape bindings

static void push_state(indigo_property_state state) {
	switch (state) {
		case INDIGO_IDLE_STATE:
			duk_push_string(PRIVATE_DATA->ctx, "Idle");
			break;
		case INDIGO_OK_STATE:
			duk_push_string(PRIVATE_DATA->ctx, "Ok");
			break;
		case INDIGO_BUSY_STATE:
			duk_push_string(PRIVATE_DATA->ctx, "Busy");
			break;
		case INDIGO_ALERT_STATE:
			duk_push_string(PRIVATE_DATA->ctx, "Alert");
			break;
	}
}

static void push_items(indigo_property *property, bool use_target) {
	duk_push_object(PRIVATE_DATA->ctx);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				duk_push_string(PRIVATE_DATA->ctx, item->text.value);
				break;
			case INDIGO_NUMBER_VECTOR:
				duk_push_number(PRIVATE_DATA->ctx, use_target ? item->number.target : item->number.value);
				break;
			case INDIGO_SWITCH_VECTOR:
				duk_push_boolean(PRIVATE_DATA->ctx, item->sw.value);
				break;
			case INDIGO_LIGHT_VECTOR:
				push_state(item->light.value);
				break;
			case INDIGO_BLOB_VECTOR:
				duk_push_string(PRIVATE_DATA->ctx, item->blob.url);
				break;
		}
		duk_put_prop_string(PRIVATE_DATA->ctx, -2, item->name);
	}
}

static indigo_property_state require_state(duk_context *ctx, duk_idx_t idx) {
	const char *state = duk_require_string(ctx, idx);
	if (!strcasecmp(state, "Ok")) {
		return INDIGO_OK_STATE;
	}
	if (!strcasecmp(state, "Busy")) {
		return INDIGO_BUSY_STATE;
	}
	if (!strcasecmp(state, "Alert")) {
		return INDIGO_ALERT_STATE;
	}
	return INDIGO_IDLE_STATE;
}

static indigo_property_perm require_perm(duk_context *ctx, duk_idx_t idx) {
	const char *state = duk_require_string(ctx, idx);
	if (!strcasecmp(state, "RO")) {
		return INDIGO_RO_PERM;
	}
	if (!strcasecmp(state, "WO")) {
		return INDIGO_WO_PERM;
	}
	return INDIGO_RW_PERM;
}

static indigo_rule require_rule(duk_context *ctx, duk_idx_t idx) {
	const char *state = duk_require_string(ctx, idx);
	if (!strcasecmp(state, "ONE_OF_MANY")) {
		return INDIGO_ONE_OF_MANY_RULE;
	}
	if (!strcasecmp(state, "AT_MOST_ONE")) {
		return INDIGO_AT_MOST_ONE_RULE;
	}
	return INDIGO_ANY_OF_MANY_RULE;
}


// function indigo_error(message)

static duk_ret_t error_message(duk_context *ctx) {
	const char *message = duk_require_string(ctx, 0);
	indigo_error(message);
	return 0;
}

// function indigo_log(message)

static duk_ret_t log_message(duk_context *ctx) {
	const char *message = duk_require_string(ctx, 0);
	indigo_log(message);
	return 0;
}

// function indigo_debug(message)

static duk_ret_t debug_message(duk_context *ctx) {
	const char *message = duk_require_string(ctx, 0);
	indigo_debug(message);
	return 0;
}

// function indigo_trace(message)

static duk_ret_t trace_message(duk_context *ctx) {
	const char *message = duk_require_string(ctx, 0);
	indigo_trace(message);
	return 0;
}

// function indigo_send_message(message)

static duk_ret_t send_message(duk_context *ctx) {
	const char *message = duk_require_string(ctx, 0);
	indigo_send_message(agent_device, message);
	return 0;
}

// function indigo_enumerate_properties(device_name, property_name)

static duk_ret_t emumerate_properties(duk_context *ctx) {
	const char *device = duk_is_null_or_undefined(ctx, 0) ? "" : duk_require_string(ctx, 0);
	const char *property = duk_is_null_or_undefined(ctx, 1) ? "" : duk_require_string(ctx, 1);
	indigo_property property_template = { 0 };
	indigo_copy_name(property_template.device, device);
	indigo_copy_name(property_template.name, property);
	indigo_enumerate_properties(agent_client, &property_template);
	return 0;
}

// function indigo_enable_blob(device_name, property_name, state)

static duk_ret_t enable_blob(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	const bool state = duk_require_boolean(ctx, 2);
	indigo_property property_template = { 0 };
	indigo_copy_name(property_template.device, device);
	indigo_copy_name(property_template.name, property);
	indigo_enable_blob(agent_client, &property_template, state);
	return 0;
}

// function indigo_change_text_property(device_name, property_name, items)

static duk_ret_t change_text_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	char *names[INDIGO_MAX_ITEMS];
	char *values[INDIGO_MAX_ITEMS];
	duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
	int i = 0;
	while (duk_next(ctx, -1, true)) {
		const char *key = duk_require_string(ctx, -2);
		const char *value = duk_require_string(ctx, -1);
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

// function indigo_change_number_property(device_name, property_name, items)

static duk_ret_t change_number_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	char *names[INDIGO_MAX_ITEMS];
	double values[INDIGO_MAX_ITEMS];
	duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
	int i = 0;
	while (duk_next(ctx, -1, true)) {
		const char *key = duk_require_string(ctx, -2);
		double value = duk_require_number(ctx, -1);
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

// function indigo_change_switch_property(device_name, property_name, items)

static duk_ret_t change_switch_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	char *names[INDIGO_MAX_ITEMS];
	bool values[INDIGO_MAX_ITEMS];
	duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
	int i = 0;
	while (duk_next(ctx, -1, true)) {
		const char *key = duk_require_string(ctx, -2);
		double value = duk_require_boolean(ctx, -1);
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

//function indigo_define_text_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, message)

static duk_ret_t define_text_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	const char *property_group = duk_require_string(ctx, 2);
	const char *property_label = duk_require_string(ctx, 3);
	indigo_property_state state = require_state(ctx, 6);
	indigo_property_perm perm = require_perm(ctx, 7);
	const char *message = duk_get_string(ctx, 8);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp == NULL || (!strcmp(tmp->device, device) && !strcmp(tmp->name, property))) {
			PRIVATE_DATA->agent_cached_property[i] = tmp = indigo_init_text_property(tmp, device, property, property_group, property_label, state, perm, INDIGO_MAX_ITEMS);
			duk_enum(ctx, 4, DUK_ENUM_OWN_PROPERTIES_ONLY );
			tmp->count = 0;
			while (duk_next(ctx, -1, true)) {
				indigo_item *item = tmp->items + tmp->count;
				const char *key = duk_require_string(ctx, -2);
				indigo_copy_name(item->name, key);
				indigo_copy_value(item->text.value, duk_to_string(ctx, -1));
				duk_get_prop_string(ctx, 5, key);
				duk_get_prop_string(ctx, -1, "label");
				indigo_copy_value(item->label, duk_to_string(ctx, -1));
				duk_pop(ctx); // label
				duk_pop(ctx); // item defs
				duk_pop_2(ctx); // item
				tmp->count++;
			}
			indigo_define_property(agent_device, tmp, message);
			return 0;
		}
	}
	return DUK_RET_ERROR;
}

//function indigo_define_number_property(device_name, property_name, property_group, property_label, items, item_labels, state, perm, message)

static duk_ret_t define_number_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	const char *property_group = duk_require_string(ctx, 2);
	const char *property_label = duk_require_string(ctx, 3);
	indigo_property_state state = require_state(ctx, 6);
	indigo_property_perm perm = require_perm(ctx, 7);
	const char *message = duk_get_string(ctx, 8);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp == NULL || (!strcmp(tmp->device, device) && !strcmp(tmp->name, property))) {
			PRIVATE_DATA->agent_cached_property[i] = tmp = indigo_init_number_property(tmp, device, property, property_group, property_label, state, perm, INDIGO_MAX_ITEMS);
			duk_enum(ctx, 4, DUK_ENUM_OWN_PROPERTIES_ONLY );
			tmp->count = 0;
			while (duk_next(ctx, -1, true)) {
				indigo_item *item = tmp->items + tmp->count;
				const char *key = duk_require_string(ctx, -2);
				indigo_copy_name(item->name, key);
				item->number.value = duk_to_number(ctx, -1);;
				duk_get_prop_string(ctx, 5, key);
				duk_get_prop_string(ctx, -1, "label");
				indigo_copy_value(item->label, duk_to_string(ctx, -1));
				duk_pop(ctx); // label
				duk_get_prop_string(ctx, -1, "format");
				indigo_copy_value(item->number.format, duk_to_string(ctx, -1));
				duk_pop(ctx); // format
				duk_get_prop_string(ctx, -1, "min");
				item->number.min = duk_to_number(ctx, -1);
				duk_pop(ctx); // min
				duk_get_prop_string(ctx, -1, "max");
				item->number.max = duk_to_number(ctx, -1);
				duk_pop(ctx); // max
				duk_get_prop_string(ctx, -1, "step");
				item->number.step = duk_to_number(ctx, -1);
				duk_pop(ctx); // step
				duk_pop(ctx); // item defs
				duk_pop_2(ctx); // item
				tmp->count++;
			}
			indigo_define_property(agent_device, tmp, message);
			return 0;
		}
	}
	return DUK_RET_ERROR;
}

//function indigo_define_switch_property(device_name, property_name, property_group, property_label, items, item_labels, state, perm, rule, message)

static duk_ret_t define_switch_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	const char *property_group = duk_require_string(ctx, 2);
	const char *property_label = duk_require_string(ctx, 3);
	indigo_property_state state = require_state(ctx, 6);
	indigo_property_perm perm = require_perm(ctx, 7);
	indigo_rule rule = require_rule(ctx, 8);
	const char *message = duk_get_string(ctx, 9);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp == NULL || (!strcmp(tmp->device, device) && !strcmp(tmp->name, property))) {
			PRIVATE_DATA->agent_cached_property[i] = tmp = indigo_init_switch_property(tmp, device, property, property_group, property_label, state, perm, rule, INDIGO_MAX_ITEMS);
			duk_enum(ctx, 4, DUK_ENUM_OWN_PROPERTIES_ONLY );
			tmp->count = 0;
			while (duk_next(ctx, -1, true)) {
				indigo_item *item = tmp->items + tmp->count;
				const char *key = duk_require_string(ctx, -2);
				indigo_copy_name(item->name, key);
				item->number.value = duk_to_boolean(ctx, -1);;
				duk_get_prop_string(ctx, 5, key);
				duk_get_prop_string(ctx, -1, "label");
				indigo_copy_value(item->label, duk_to_string(ctx, -1));
				duk_pop(ctx); // label
				duk_pop(ctx); // item defs
				duk_pop_2(ctx); // item
				tmp->count++;
			}
			indigo_define_property(agent_device, tmp, message);
			return 0;
		}
	}
	return DUK_RET_ERROR;
}

//function indigo_define_light_property(device_name, property_name, property_group, property_label, items, item_labels, state, message)

static duk_ret_t define_light_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	const char *property_group = duk_require_string(ctx, 2);
	const char *property_label = duk_require_string(ctx, 3);
	indigo_property_state state = require_state(ctx, 6);
	const char *message = duk_get_string(ctx, 7);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp == NULL || (!strcmp(tmp->device, device) && !strcmp(tmp->name, property))) {
			PRIVATE_DATA->agent_cached_property[i] = tmp = indigo_init_light_property(tmp, device, property, property_group, property_label, state, INDIGO_MAX_ITEMS);
			duk_enum(ctx, 4, DUK_ENUM_OWN_PROPERTIES_ONLY );
			tmp->count = 0;
			while (duk_next(ctx, -1, true)) {
				indigo_item *item = tmp->items + tmp->count;
				const char *key = duk_require_string(ctx, -2);
				indigo_copy_name(item->name, key);
				item->light.value = require_state(ctx, -1);;
				duk_get_prop_string(ctx, 5, key);
				duk_get_prop_string(ctx, -1, "label");
				indigo_copy_value(item->label, duk_to_string(ctx, -1));
				duk_pop(ctx); // label
				duk_pop(ctx); // item defs
				duk_pop_2(ctx); // item
				tmp->count++;
			}
			indigo_define_property(agent_device, tmp, message);
			return 0;
		}
	}
	return DUK_RET_ERROR;
}

//function indigo_update_text_property(device_name, property_name, items, state, message)

static duk_ret_t update_text_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	indigo_property_state state = require_state(ctx, 3);
	const char *message = duk_get_string(ctx, 4);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp && !strcmp(tmp->device, device) && !strcmp(tmp->name, property)) {
			duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
			while (duk_next(ctx, -1, true)) {
				const char *name = duk_require_string(ctx, -2);
				for (int j = 0; j < tmp->count; j++) {
					indigo_item *item = tmp->items + j;
					if (!strcmp(item->name, name)) {
						indigo_copy_name(tmp->items[j].name, name);
						indigo_copy_value(tmp->items[j].text.value, duk_to_string(ctx, -1));
						break;
					}
				}
				duk_pop_2(ctx); // item
			}
			tmp->state = state;
			indigo_update_property(agent_device, tmp, message);
		}
	}
	return 0;
}

//function indigo_update_number_property(device_name, property_name, items, state, message)

static duk_ret_t update_number_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	indigo_property_state state = require_state(ctx, 3);
	const char *message = duk_get_string(ctx, 4);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp && !strcmp(tmp->device, device) && !strcmp(tmp->name, property)) {
			duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY);
			while (duk_next(ctx, -1, true)) {
				const char *name = duk_require_string(ctx, -2);
				for (int j = 0; j < tmp->count; j++) {
					indigo_item *item = tmp->items + j;
					if (!strcmp(item->name, name)) {
						indigo_copy_name(tmp->items[j].name, name);
						tmp->items[j].number.value = duk_to_number(ctx, -1);
						break;
					}
				}
				duk_pop_2(ctx); // item
			}
			tmp->state = state;
			indigo_update_property(agent_device, tmp, message);
		}
	}
	return 0;
}

//function indigo_update_switch_property(device_name, property_name, items, state, message)

static duk_ret_t update_switch_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	indigo_property_state state = require_state(ctx, 3);
	const char *message = duk_get_string(ctx, 4);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp && !strcmp(tmp->device, device) && !strcmp(tmp->name, property)) {
			duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
			while (duk_next(ctx, -1, true)) {
				const char *name = duk_require_string(ctx, -2);
				for (int j = 0; j < tmp->count; j++) {
					indigo_item *item = tmp->items + j;
					if (!strcmp(item->name, name)) {
						indigo_copy_name(tmp->items[j].name, name);
						tmp->items[j].sw.value = duk_to_boolean(ctx, -1);
						break;
					}
				}
				duk_pop_2(ctx); // item
			}
			tmp->state = state;
			indigo_update_property(agent_device, tmp, message);
		}
	}
	return 0;
}

//function indigo_update_light_property(device_name, property_name, items, state, message)

static duk_ret_t update_light_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_require_string(ctx, 1);
	indigo_property_state state = require_state(ctx, 3);
	const char *message = duk_get_string(ctx, 4);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp && !strcmp(tmp->device, device) && !strcmp(tmp->name, property)) {
			duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY );
			while (duk_next(ctx, -1, true)) {
				const char *name = duk_require_string(ctx, -2);
				for (int j = 0; j < tmp->count; j++) {
					indigo_item *item = tmp->items + j;
					if (!strcmp(item->name, name)) {
						indigo_copy_name(tmp->items[j].name, name);
						tmp->items[j].sw.value = require_state(ctx, -1);
						break;
					}
				}
				duk_pop_2(ctx); // item
			}
			tmp->state = state;
			indigo_update_property(agent_device, tmp, message);
		}
	}
	return 0;
}

//function indigo_delete_property(device_name, property_name, message)

static duk_ret_t delete_property(duk_context *ctx) {
	const char *device = duk_require_string(ctx, 0);
	const char *property = duk_get_string(ctx, 1);
	const char *message = duk_get_string(ctx, 2);
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *tmp = PRIVATE_DATA->agent_cached_property[i];
		if (tmp && !strcmp(tmp->device, device) && !strcmp(tmp->name, property)) {
			indigo_delete_property(agent_device, tmp, message);
			indigo_release_property(tmp);
			PRIVATE_DATA->agent_cached_property[i] = NULL;
		}
	}
	return 0;
}

// function indigo_set_timer(function, delay);

static void timer_handler(indigo_device *device, void *data) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	uintptr_t index = (uintptr_t)data;
	duk_push_global_object(PRIVATE_DATA->ctx);
	duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_timers");
	duk_push_number(PRIVATE_DATA->ctx, (double)(index - 1));
	duk_get_prop(PRIVATE_DATA->ctx, -2);
	if (duk_pcall(PRIVATE_DATA->ctx, 0)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "timer call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
	}
	duk_pop_3(PRIVATE_DATA->ctx);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static duk_ret_t set_timer(duk_context *ctx) {
	for (uintptr_t index = 0; index < MAX_TIMER_COUNT; index++) {
		if (PRIVATE_DATA->timers[index] == NULL) {
			duk_push_global_object(PRIVATE_DATA->ctx);
			duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_timers");
			duk_push_number(PRIVATE_DATA->ctx, (double)index);
			duk_dup(PRIVATE_DATA->ctx, 0);
			duk_put_prop(PRIVATE_DATA->ctx, -3);
			double delay = duk_require_number(ctx, 1);
			if (indigo_set_timer_with_data(agent_device, delay, timer_handler, PRIVATE_DATA->timers + index, (void *)(index + 1))) {
				duk_push_int(ctx, (int)index);
				return 1;
			}
			break;
		}
	}
	return DUK_RET_ERROR;
}

// function indigo_cancel_timer(timer);

static duk_ret_t cancel_timer(duk_context *ctx) {
	int i = duk_require_int(ctx, 0);
	if (PRIVATE_DATA->timers[i]) {
		if (indigo_cancel_timer(agent_device, PRIVATE_DATA->timers + i)) {
			return 0;
		}
	}
	return DUK_RET_ERROR;
}

static bool execute_script(indigo_property *property) {
	bool result = true;
	char *script = indigo_get_text_item_value(property->count == 1 ? property->items : property->items + 1);
	if (script && *script) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		if (duk_peval_string(PRIVATE_DATA->ctx, script)) {
			indigo_send_message(agent_device, "Failed to execute script '%s' (%s)", property->label, duk_safe_to_string(PRIVATE_DATA->ctx, -1));
			result = false;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	}
	return result;
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Script properties
		AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME, AGENT_MAIN_GROUP, "Add script", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM_NAME, "Name", "");
		indigo_init_text_item_raw(AGENT_SCRIPTING_ADD_SCRIPT_ITEM, AGENT_SCRIPTING_ADD_SCRIPT_ITEM_NAME, "Script", "");
		AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY_NAME, AGENT_MAIN_GROUP, "Execute script", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_MAX_ITEMS - 2);
		if (AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count = 0;
		AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY_NAME, AGENT_MAIN_GROUP, "Delete script", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_MAX_ITEMS - 2);
		if (AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->count = 0;
		AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY_NAME, AGENT_MAIN_GROUP, "Exececute on agent load", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_ITEMS);
		if (AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME, "New script", true);
		AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY_NAME, AGENT_MAIN_GROUP, "Execute on agent unload", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_ITEMS);
		if (AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME, "New script", false);
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&PRIVATE_DATA->mutex, &Attr);
		if ((PRIVATE_DATA->ctx = duk_create_heap_default())) {
      pthread_mutex_lock(&PRIVATE_DATA->mutex);
			duk_push_c_function(PRIVATE_DATA->ctx, error_message, 1);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_error");
			duk_push_c_function(PRIVATE_DATA->ctx, log_message, 1);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_log");
			duk_push_c_function(PRIVATE_DATA->ctx, debug_message, 1);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_debug");
			duk_push_c_function(PRIVATE_DATA->ctx, trace_message, 1);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_trace");
			duk_push_c_function(PRIVATE_DATA->ctx, send_message, 1);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_send_message");
			duk_push_c_function(PRIVATE_DATA->ctx, emumerate_properties, 2);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_enumerate_properties");
			duk_push_c_function(PRIVATE_DATA->ctx, enable_blob, 3);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_enable_blob");
			duk_push_c_function(PRIVATE_DATA->ctx, change_text_property, 3);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_change_text_property");
			duk_push_c_function(PRIVATE_DATA->ctx, change_number_property, 3);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_change_number_property");
			duk_push_c_function(PRIVATE_DATA->ctx, change_switch_property, 3);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_change_switch_property");
			duk_push_c_function(PRIVATE_DATA->ctx, define_text_property, 9);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_define_text_property");
			duk_push_c_function(PRIVATE_DATA->ctx, define_number_property, 9);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_define_number_property");
			duk_push_c_function(PRIVATE_DATA->ctx, define_switch_property, 10);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_define_switch_property");
			duk_push_c_function(PRIVATE_DATA->ctx, define_light_property, 8);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_define_light_property");
			duk_push_c_function(PRIVATE_DATA->ctx, update_text_property, 5);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_update_text_property");
			duk_push_c_function(PRIVATE_DATA->ctx, update_number_property, 5);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_update_number_property");
			duk_push_c_function(PRIVATE_DATA->ctx, update_switch_property, 5);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_update_switch_property");
			duk_push_c_function(PRIVATE_DATA->ctx, update_light_property, 5);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_update_light_property");
			duk_push_c_function(PRIVATE_DATA->ctx, delete_property, 3);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_delete_property");
			duk_push_c_function(PRIVATE_DATA->ctx, set_timer, 2);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_set_timer");
			duk_push_c_function(PRIVATE_DATA->ctx, cancel_timer, 1);
			duk_put_global_string(PRIVATE_DATA->ctx, "indigo_cancel_timer");
			if (duk_peval_string(PRIVATE_DATA->ctx, boot_js)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "boot.js executed");
			}
      pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		}
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, property))
		indigo_define_property(device, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, NULL);
	if (indigo_property_match(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, property))
		indigo_define_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
	if (indigo_property_match(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, property))
		indigo_define_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
	if (indigo_property_match(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, property))
		indigo_define_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
	if (indigo_property_match(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, property))
		indigo_define_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
	for (int i = 0; i < MAX_USER_SCRIPT_COUNT; i++) {
		indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(i);
		if (script_property)
			indigo_define_property(device, script_property, NULL);
	}
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *cached_property = PRIVATE_DATA->agent_cached_property[i];
		if (cached_property)
			indigo_define_property(device, cached_property, NULL);
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	duk_push_global_object(PRIVATE_DATA->ctx);
	if (duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_on_enumerate_properties")) {
		duk_push_string(PRIVATE_DATA->ctx, device->name);
		duk_push_string(PRIVATE_DATA->ctx, property ? property->name : NULL);
		if (duk_pcall(PRIVATE_DATA->ctx, 2)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_on_enumerate_properties() call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
		}
	}
	duk_pop_2(PRIVATE_DATA->ctx);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_LOAD_ITEM, property)) {
			indigo_device_change_property(device, client, property);
			AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, "Executing on-load scripts");
			for (int i = 1; i < AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->count; i++) {
				indigo_item *item = AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items + i;
				if (item->sw.value) {
					int j = atoi(item->name + AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME_LENGTH);
					indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(j);
					if (script_property) {
						execute_script(script_property);
					}
				}
			}
			AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
			return INDIGO_OK;
		}
	} else if (indigo_property_match(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SCRIPTING_ADD_SCRIPT
		indigo_property_copy_values(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, property, false);
		if (AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value[0] == 0) {
			AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, "Empty script name");
			return INDIGO_OK;
		}
		int empty_slot = -1;
		for (int i = 0; i < MAX_USER_SCRIPT_COUNT; i++) {
			indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(i);
			if (script_property) {
				if (!strcmp(script_property->items[0].text.value, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value)) {
					AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, "Script %s already exists", AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value);
					return INDIGO_OK;
				}
			} else if (empty_slot == -1) {
				empty_slot = i;
			}
		}
		if (empty_slot == -1) {
			AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, "Too many scripts defined");
			return INDIGO_OK;
		} else {
			char name[INDIGO_NAME_SIZE];
			sprintf(name, AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME, empty_slot);
			indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(empty_slot) = indigo_init_text_property(NULL, device->name, name, SCRIPT_GROUP, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value, INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			indigo_init_text_item(script_property->items + 0, AGENT_SCRIPTING_SCRIPT_NAME_ITEM_NAME, "Name", AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value);
			indigo_init_text_item_raw(script_property->items + 1, AGENT_SCRIPTING_SCRIPT_ITEM_NAME, "Script", indigo_get_text_item_value(AGENT_SCRIPTING_ADD_SCRIPT_ITEM));
			indigo_define_property(device, script_property, NULL);
			int j = AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count;
			indigo_delete_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
			indigo_delete_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
			indigo_delete_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
			indigo_delete_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
			AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count++;
			AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->count++;
			AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->count++;
			AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->count++;
			indigo_init_switch_item(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->items + j, name, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value, false);
			indigo_init_switch_item(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->items + j, name, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value, false);
			indigo_init_switch_item(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items + (j + 1), name, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items->sw.value);
			indigo_init_switch_item(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items + (j + 1), name, AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM->text.value, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items->sw.value);
			indigo_property_sort_items(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, 0);
			indigo_property_sort_items(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, 0);
			indigo_property_sort_items(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, 1);
			indigo_property_sort_items(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, 1);
			indigo_define_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
			indigo_define_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
			indigo_define_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
			indigo_define_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
		}
		indigo_set_text_item_value(AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM, "");
		indigo_set_text_item_value(AGENT_SCRIPTING_ADD_SCRIPT_ITEM, "");
		AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY, NULL);
		save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SCRIPTING_EXECUTE_SCRIPT
		indigo_property_copy_values(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, property, false);
		AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
		for (int i = 0; i < AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count; i++) {
			indigo_item *item = AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->items + i;
			if (item->sw.value) {
				item->sw.value = false;
				int j = atoi(item->name + AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME_LENGTH);
				indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(j);
				if (script_property) {
					if (!execute_script(script_property)) {
						AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
						return INDIGO_OK;
					}
					break;
				}
			}
		}
		AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SCRIPTING_DELETE_SCRIPT
		indigo_property_copy_values(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, property, false);
		for (int i = 0; i < AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->count; i++) {
			indigo_item *item = AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->items + i;
			if (item->sw.value) {
				int j = atoi(item->name + AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME_LENGTH);
				indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(j);
				if (script_property) {
					indigo_delete_property(device, script_property, NULL);
					indigo_release_property(script_property);
					AGENT_SCRIPTING_SCRIPT_PROPERTY(j) = NULL;
					indigo_delete_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
					indigo_delete_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
					indigo_delete_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
					indigo_delete_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
					int count = AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count;
					if (i + 1 < count) {
						indigo_item tmp[count - i - 1];
						memcpy(tmp, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->items + i + 1, sizeof(indigo_item) * (count - i - 1));
						memcpy(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->items + i, tmp, sizeof(indigo_item) * (count - i - 1));
						memcpy(tmp, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->items + i + 1, sizeof(indigo_item) * (count - i - 1));
						memcpy(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->items + i, tmp, sizeof(indigo_item) * (count - i - 1));
						memcpy(tmp, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items + i + 2, sizeof(indigo_item) * (count - i - 2));
						memcpy(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items + (i + 1), tmp, sizeof(indigo_item) * (count - i - 2));
						memcpy(tmp, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items + i + 2, sizeof(indigo_item) * (count - i - 2));
						memcpy(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items + (i + 1), tmp, sizeof(indigo_item) * (count - i - 2));
					}
					AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count--;
					AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->count--;
					AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->count--;
					AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->count--;
					AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
					indigo_define_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
					indigo_define_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
					indigo_define_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
					break;
				}
			}
		}
		save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SCRIPTING_ON_LOAD_SCRIPT
		indigo_property_copy_values(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, property, false);
		AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
		save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SCRIPTING_ON_UNLOAD_SCRIPT
		indigo_property_copy_values(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, property, false);
		AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
		save_config(device);
		return INDIGO_OK;
	} else {
		for (int i = 0; i < MAX_USER_SCRIPT_COUNT; i++) {
			indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(i);
			if (script_property && indigo_property_match(script_property, property)) {
				indigo_property_copy_values(script_property, property, false);
				script_property->state = INDIGO_OK_STATE;
				if (strcmp(script_property->label, script_property->items[0].text.value)) {
					indigo_delete_property(device, script_property, NULL);
					indigo_copy_value(script_property->label, script_property->items[0].text.value);
					for (int j = 0; j < AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->count; j++) {
						indigo_item *item = AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY->items + j;
						if (!strcmp(script_property->name, item->name)) {
							indigo_delete_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
							indigo_delete_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
							indigo_delete_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
							indigo_delete_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
							strcpy(item->label, script_property->label);
							strcpy(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY->items[j].label, script_property->label);
							strcpy(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY->items[j + 1].label, script_property->label);
							strcpy(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items[j + 1].label, script_property->label);
							indigo_property_sort_items(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, 0);
							indigo_property_sort_items(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, 0);
							indigo_property_sort_items(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, 1);
							indigo_property_sort_items(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, 1);
							indigo_define_property(device, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY, NULL);
							indigo_define_property(device, AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY, NULL);
							indigo_define_property(device, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY, NULL);
							indigo_define_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
							break;
						}
					}
					indigo_define_property(device, script_property, NULL);
				} else {
					indigo_update_property(device, script_property, NULL);
				}
				save_config(device);
				return INDIGO_OK;
			}
		}
	}
  if (PRIVATE_DATA->ctx) {
    pthread_mutex_lock(&PRIVATE_DATA->mutex);
    duk_push_global_object(PRIVATE_DATA->ctx);
    if (duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_on_change_property")) {
      duk_push_string(PRIVATE_DATA->ctx, property->device);
      duk_push_string(PRIVATE_DATA->ctx, property->name);
      push_items(property, true);
      push_state(property->state);
      if (duk_pcall(PRIVATE_DATA->ctx, 4)) {
        INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_on_change_property() call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
      }
    }
    duk_pop_2(PRIVATE_DATA->ctx);
    pthread_mutex_unlock(&PRIVATE_DATA->mutex);
  }
	return indigo_device_change_property(device, client, property);
}

static indigo_result agent_enable_blob(indigo_device *device, indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	assert(device != NULL);
	return INDIGO_OK;
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	if (PRIVATE_DATA->ctx) {
		AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, "Executing on-unload scripts");
		for (int i = 1; i < AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->count; i++) {
			indigo_item *item = AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->items + i;
			if (item->sw.value) {
				int j = atoi(item->name + AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME_LENGTH);
				indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(j);
				if (script_property) {
					execute_script(script_property);
				}
			}
		}
		AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY, NULL);
		duk_destroy_heap(PRIVATE_DATA->ctx);
	}
	for (int i = 0; i < MAX_TIMER_COUNT; i++) {
		if (PRIVATE_DATA->timers[i])
			indigo_cancel_timer_sync(agent_device, PRIVATE_DATA->timers + i);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	indigo_release_property(AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY);
	indigo_release_property(AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY);
	indigo_release_property(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY);
	indigo_release_property(AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY);
	indigo_release_property(AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY);
	for (int i = 0; i < MAX_USER_SCRIPT_COUNT; i++) {
		indigo_property *script_property = AGENT_SCRIPTING_SCRIPT_PROPERTY(i);
		if (script_property)
			indigo_release_property(script_property);
	}
	for (int i = 0; i < MAX_CACHED_SCRIPT_COUNT; i++) {
		indigo_property *cached_property = PRIVATE_DATA->agent_cached_property[i];
		if (cached_property)
			indigo_release_property(cached_property);
	}

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
	duk_push_global_object(PRIVATE_DATA->ctx);
	if (duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_on_define_property")) {
		duk_push_string(PRIVATE_DATA->ctx, property->device);
		duk_push_string(PRIVATE_DATA->ctx, property->name);
		push_items(property, false);
		push_state(property->state);
		duk_push_string(PRIVATE_DATA->ctx, property->perm == INDIGO_RW_PERM ? "RW" : property->perm == INDIGO_RO_PERM ? "RO" : "WO");
		duk_push_string(PRIVATE_DATA->ctx, message);
		if (duk_pcall(PRIVATE_DATA->ctx, 6)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_on_define_property() call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
		}
	}
	duk_pop_2(PRIVATE_DATA->ctx);
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	duk_push_global_object(PRIVATE_DATA->ctx);
	if (duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_on_update_property")) {
		duk_push_string(PRIVATE_DATA->ctx, property->device);
		duk_push_string(PRIVATE_DATA->ctx, property->name);
		push_items(property, false);
		push_state(property->state);
		duk_push_string(PRIVATE_DATA->ctx, message);
		if (duk_pcall(PRIVATE_DATA->ctx, 5)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_on_update_property() call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
		}
	}
	duk_pop_2(PRIVATE_DATA->ctx);
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	duk_push_global_object(PRIVATE_DATA->ctx);
	if (duk_get_prop_string(PRIVATE_DATA->ctx, -1, "indigo_on_delete_property")) {
		duk_push_string(PRIVATE_DATA->ctx, property->device);
		duk_push_string(PRIVATE_DATA->ctx, property->name);
		duk_push_string(PRIVATE_DATA->ctx, message);
		if (duk_pcall(PRIVATE_DATA->ctx, 3)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_on_delete_property() call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
		}
	}
	duk_pop_2(PRIVATE_DATA->ctx);
	return INDIGO_OK;
}

static indigo_result agent_send_message(indigo_client *client, indigo_device *device, const char *message) {
	duk_push_global_object(PRIVATE_DATA->ctx);
	if (duk_get_prop_string(PRIVATE_DATA->ctx, -1, "agent_on_send_message")) {
		duk_push_string(PRIVATE_DATA->ctx, device->name);
		duk_push_string(PRIVATE_DATA->ctx, message);
		if (duk_pcall(PRIVATE_DATA->ctx, 2)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "agent_on_send_message() call failed (%s)", duk_safe_to_string(PRIVATE_DATA->ctx, -1));
		}
	}
	duk_pop_2(PRIVATE_DATA->ctx);
	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

indigo_result indigo_agent_scripting(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		SCRIPTING_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		agent_enable_blob,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		SCRIPTING_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		agent_send_message,
		agent_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, SCRIPTING_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

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
