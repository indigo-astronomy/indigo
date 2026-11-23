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

#define DRIVER_VERSION 0x03000007
#define DRIVER_NAME	"indigo_agent_auxiliary"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>

#include "indigo_agent_auxiliary.h"

#define DEVICE_PRIVATE_DATA										((auxiliary_agent_private_data *)device->private_data)

#define AGENT_AUX_SAFETY_PROPERTY							(DEVICE_PRIVATE_DATA->agent_safety_property)
#define AGENT_AUX_SAFETY_SAFE_ITEM						(AGENT_AUX_SAFETY_PROPERTY->items + 0)
#define AGENT_AUX_SAFETY_CONDITION_ITEMS			(AGENT_AUX_SAFETY_PROPERTY->items + 1)

#define AUX_SAFETY_SLOT_COUNT									(INDIGO_FILTER_AUX_4_INDEX - INDIGO_FILTER_AUX_1_INDEX + 1)
#define AUX_SAFETY_CONDITION_COUNT						4

typedef struct {
	indigo_property *agent_safety_property;
	pthread_mutex_t safety_mutex;
	bool safety_has_source;
	// SAFE light of the source in each FILTER_AUX_x_LIST slot at the last computation, IDLE without a source
	indigo_property_state source_light[AUX_SAFETY_SLOT_COUNT];
	bool source_unsafe[AUX_SAFETY_SLOT_COUNT];
	char source_name[AUX_SAFETY_SLOT_COUNT][INDIGO_NAME_SIZE];
	// the latest reason message of each source and its SAFE light when the message arrived
	char reason[AUX_SAFETY_SLOT_COUNT][INDIGO_VALUE_SIZE];
	char reason_device[AUX_SAFETY_SLOT_COUNT][INDIGO_NAME_SIZE];
	indigo_property_state reason_light[AUX_SAFETY_SLOT_COUNT];
	// the last published message used a reason written by the agent for the source, and that message
	bool reason_written[AUX_SAFETY_SLOT_COUNT];
	char published_message[INDIGO_VALUE_SIZE];
} auxiliary_agent_private_data;

static const char *condition_item_names[AUX_SAFETY_CONDITION_COUNT] = { AUX_WEATHER_SAFETY_CLOUDS_ITEM_NAME, AUX_WEATHER_SAFETY_RAIN_ITEM_NAME, AUX_WEATHER_SAFETY_WIND_ITEM_NAME, AUX_WEATHER_SAFETY_HUMIDITY_ITEM_NAME };

static const char *condition_labels[AUX_SAFETY_CONDITION_COUNT] = { "clouds", "rain", "wind", "humidity" };

// -------------------------------------------------------------------------------- Weather safety

static void safety_append(char *message, size_t message_size, const char *format, ...) {
	size_t length = strlen(message);
	if (length + 1 < message_size) {
		va_list args;
		va_start(args, format);
		vsnprintf(message + length, message_size - length, format, args);
		va_end(args);
	}
}

static indigo_property_state source_item_light(indigo_property *source, const char *name) {
	indigo_item *item = indigo_get_item(source, name);
	return item != NULL ? item->light.value : INDIGO_IDLE_STATE;
}

// worst wins, as in indigo_aux_evaluate_weather_safety(): ALERT over OK over IDLE
static void combine_light(indigo_property_state *verdict, indigo_property_state light) {
	if (light == INDIGO_ALERT_STATE || (light == INDIGO_OK_STATE && *verdict == INDIGO_IDLE_STATE)) {
		*verdict = light;
	}
}

static int source_slot(indigo_device *device, const char *name) {
	for (int slot = 0; slot < AUX_SAFETY_SLOT_COUNT; slot++) {
		if (*name && !strcmp(FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_AUX_1_INDEX + slot], name)) {
			return slot;
		}
	}
	return -1;
}

// AUX_WEATHER_SAFETY of the device selected in the slot, once its selection is complete
static indigo_property *safety_source(indigo_device *device, int slot) {
	int index = INDIGO_FILTER_AUX_1_INDEX + slot;
	char *name = FILTER_DEVICE_CONTEXT->device_name[index];
	if (*name == 0 || FILTER_DEVICE_CONTEXT->filter_device_list_properties[index]->state != INDIGO_OK_STATE || source_slot(device, name) != slot) {
		return NULL;
	}
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		indigo_property *cached = FILTER_DEVICE_CONTEXT->device_property_cache[i];
		if (cached != NULL && !strcmp(cached->device, name) && !strcmp(cached->name, AUX_WEATHER_SAFETY_PROPERTY_NAME)) {
			return cached;
		}
	}
	return NULL;
}

// the source's own reason belongs to its current SAFE light
static bool source_reason_available(indigo_device *device, int slot, indigo_property *source) {
	return !strcmp(DEVICE_PRIVATE_DATA->reason_device[slot], source->device) && DEVICE_PRIVATE_DATA->reason_light[slot] == DEVICE_PRIVATE_DATA->source_light[slot];
}

// the reason of the source with the device name appended: the source's own message, or one written by the agent from the lights without values
static void append_source_reason(indigo_device *device, int slot, indigo_property *source, char *message, size_t message_size) {
	safety_append(message, message_size, "%s", *message ? "; " : "");
	indigo_property_state light = DEVICE_PRIVATE_DATA->source_light[slot];
	DEVICE_PRIVATE_DATA->reason_written[slot] = !source_reason_available(device, slot, source);
	if (!DEVICE_PRIVATE_DATA->reason_written[slot]) {
		safety_append(message, message_size, "%s", DEVICE_PRIVATE_DATA->reason[slot]);
	} else if (light == INDIGO_ALERT_STATE) {
		safety_append(message, message_size, "Unsafe:");
		bool first = true;
		for (int i = 0; i < AUX_SAFETY_CONDITION_COUNT; i++) {
			if (source_item_light(source, condition_item_names[i]) == INDIGO_ALERT_STATE) {
				safety_append(message, message_size, "%s%s", first ? " " : ", ", condition_labels[i]);
				first = false;
			}
		}
	} else if (light == INDIGO_OK_STATE) {
		safety_append(message, message_size, "Safe: all conditions within limits");
	} else {
		safety_append(message, message_size, "Unknown: no weather condition is monitored");
	}
	safety_append(message, message_size, " (%s)", source->device);
}

// Combines AUX_WEATHER_SAFETY of the selected aux devices into AGENT_AUX_SAFETY and publishes it at once when it changes.
// A source sends its reason only after the update that changes its SAFE light, so that update is published with a reason written
// by the agent; when the source's own reason arrives (reason_arrived) and gives a different message, it follows in a second update.
static void compute_safety(indigo_device *device, bool reason_arrived) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->safety_mutex);
	indigo_property *sources[AUX_SAFETY_SLOT_COUNT];
	indigo_property_state safe = INDIGO_IDLE_STATE;
	indigo_property_state conditions[AUX_SAFETY_CONDITION_COUNT] = { INDIGO_IDLE_STATE, INDIGO_IDLE_STATE, INDIGO_IDLE_STATE, INDIGO_IDLE_STATE };
	bool unsafe_changed = false;
	int source_count = 0;
	for (int slot = 0; slot < AUX_SAFETY_SLOT_COUNT; slot++) {
		indigo_property *source = sources[slot] = safety_source(device, slot);
		indigo_property_state light = INDIGO_IDLE_STATE;
		if (source != NULL) {
			source_count++;
			light = source_item_light(source, AUX_WEATHER_SAFETY_SAFE_ITEM_NAME);
			combine_light(&safe, light);
			for (int i = 0; i < AUX_SAFETY_CONDITION_COUNT; i++) {
				combine_light(conditions + i, source_item_light(source, condition_item_names[i]));
			}
		} else {
			*DEVICE_PRIVATE_DATA->reason_device[slot] = 0;
		}
		// a stored reason belongs to the light it arrived with; the source sends a new one after the update that changes the light
		if (light != DEVICE_PRIVATE_DATA->source_light[slot]) {
			*DEVICE_PRIVATE_DATA->reason_device[slot] = 0;
		}
		DEVICE_PRIVATE_DATA->source_light[slot] = light;
		// the set of unsafe sources, by slot and device name
		bool unsafe = light == INDIGO_ALERT_STATE;
		const char *name = source != NULL ? source->device : "";
		if (unsafe != DEVICE_PRIVATE_DATA->source_unsafe[slot] || (unsafe && strcmp(name, DEVICE_PRIVATE_DATA->source_name[slot]))) {
			unsafe_changed = true;
		}
		DEVICE_PRIVATE_DATA->source_unsafe[slot] = unsafe;
		INDIGO_COPY_NAME(DEVICE_PRIVATE_DATA->source_name[slot], name);
	}
	bool lost = source_count == 0 && DEVICE_PRIVATE_DATA->safety_has_source;
	DEVICE_PRIVATE_DATA->safety_has_source = source_count > 0;
	indigo_property_state state = source_count > 0 ? INDIGO_OK_STATE : (lost ? INDIGO_ALERT_STATE : AGENT_AUX_SAFETY_PROPERTY->state);
	bool safe_changed = AGENT_AUX_SAFETY_SAFE_ITEM->light.value != safe;
	bool changed = safe_changed || AGENT_AUX_SAFETY_PROPERTY->state != state;
	AGENT_AUX_SAFETY_SAFE_ITEM->light.value = safe;
	for (int i = 0; i < AUX_SAFETY_CONDITION_COUNT; i++) {
		changed = changed || AGENT_AUX_SAFETY_CONDITION_ITEMS[i].light.value != conditions[i];
		AGENT_AUX_SAFETY_CONDITION_ITEMS[i].light.value = conditions[i];
	}
	AGENT_AUX_SAFETY_PROPERTY->state = state;
	// the source's own reason replaces one written by the agent in the last message
	bool follow_up = false;
	for (int slot = 0; reason_arrived && slot < AUX_SAFETY_SLOT_COUNT; slot++) {
		follow_up = follow_up || (sources[slot] != NULL && DEVICE_PRIVATE_DATA->reason_written[slot] && DEVICE_PRIVATE_DATA->source_light[slot] == safe && source_reason_available(device, slot, sources[slot]));
	}
	char message[INDIGO_VALUE_SIZE] = "";
	if (lost) {
		safety_append(message, sizeof(message), "Weather safety source lost");
	} else if (source_count > 0 && (safe_changed || unsafe_changed || follow_up)) {
		// the reasons of the sources that decide the combined verdict
		for (int slot = 0; slot < AUX_SAFETY_SLOT_COUNT; slot++) {
			DEVICE_PRIVATE_DATA->reason_written[slot] = false;
			if (sources[slot] != NULL && DEVICE_PRIVATE_DATA->source_light[slot] == safe) {
				append_source_reason(device, slot, sources[slot], message, sizeof(message));
			}
		}
		// a follow-up that gives the same text is not repeated
		if (follow_up && !safe_changed && !unsafe_changed && !strcmp(message, DEVICE_PRIVATE_DATA->published_message)) {
			*message = 0;
		}
	}
	if (*message) {
		INDIGO_COPY_VALUE(DEVICE_PRIVATE_DATA->published_message, message);
		indigo_update_property(device, AGENT_AUX_SAFETY_PROPERTY, "%s", message);
	} else if (changed) {
		indigo_update_property(device, AGENT_AUX_SAFETY_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->safety_mutex);
}

static bool safety_relevant(indigo_client *client, indigo_device *device, indigo_property *property) {
	return device != FILTER_CLIENT_CONTEXT->device && (*property->name == 0 || !strcmp(property->name, AUX_WEATHER_SAFETY_PROPERTY_NAME) || !strcmp(property->name, CONNECTION_PROPERTY_NAME));
}

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
		// -------------------------------------------------------------------------------- Safety, combined from AUX_WEATHER_SAFETY of the selected aux devices
		AGENT_AUX_SAFETY_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_AUX_SAFETY_PROPERTY_NAME, AGENT_MAIN_GROUP, "Safety", INDIGO_IDLE_STATE, 1 + AUX_SAFETY_CONDITION_COUNT);
		indigo_init_light_item(AGENT_AUX_SAFETY_SAFE_ITEM, AGENT_AUX_SAFETY_SAFE_ITEM_NAME, "Safe", INDIGO_IDLE_STATE);
		indigo_init_light_item(AGENT_AUX_SAFETY_CONDITION_ITEMS + 0, AGENT_AUX_SAFETY_CLOUDS_ITEM_NAME, "Clouds", INDIGO_IDLE_STATE);
		indigo_init_light_item(AGENT_AUX_SAFETY_CONDITION_ITEMS + 1, AGENT_AUX_SAFETY_RAIN_ITEM_NAME, "Rain", INDIGO_IDLE_STATE);
		indigo_init_light_item(AGENT_AUX_SAFETY_CONDITION_ITEMS + 2, AGENT_AUX_SAFETY_WIND_ITEM_NAME, "Wind", INDIGO_IDLE_STATE);
		indigo_init_light_item(AGENT_AUX_SAFETY_CONDITION_ITEMS + 3, AGENT_AUX_SAFETY_HUMIDITY_ITEM_NAME, "Humidity", INDIGO_IDLE_STATE);
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->safety_mutex, NULL);
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
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->safety_mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_filter_device_detach(device);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = indigo_filter_change_property(device, client, property);
	for (int index = INDIGO_FILTER_AUX_1_INDEX; index <= INDIGO_FILTER_AUX_4_INDEX; index++) {
		if (indigo_property_match(FILTER_DEVICE_CONTEXT->filter_device_list_properties[index], property)) {
			compute_safety(device, false);
			break;
		}
	}
	return result;
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_result result = indigo_filter_define_property(client, device, property, message);
	if (safety_relevant(client, device, property)) {
		compute_safety(FILTER_CLIENT_CONTEXT->device, false);
	}
	return result;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_result result = indigo_filter_update_property(client, device, property, message);
	if (safety_relevant(client, device, property)) {
		compute_safety(FILTER_CLIENT_CONTEXT->device, false);
	}
	return result;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_result result = indigo_filter_delete_property(client, device, property, message);
	if (safety_relevant(client, device, property)) {
		compute_safety(FILTER_CLIENT_CONTEXT->device, false);
	}
	return result;
}

static indigo_result agent_send_message(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (property != NULL && message != NULL && *message && device != FILTER_CLIENT_CONTEXT->device && !strcmp(property->name, AUX_WEATHER_SAFETY_PROPERTY_NAME)) {
		indigo_device *agent = FILTER_CLIENT_CONTEXT->device;
		int slot = source_slot(agent, property->device);
		if (slot >= 0) {
			auxiliary_agent_private_data *data = (auxiliary_agent_private_data *)agent->private_data;
			pthread_mutex_lock(&data->safety_mutex);
			INDIGO_COPY_VALUE(data->reason[slot], message);
			INDIGO_COPY_NAME(data->reason_device[slot], property->device);
			data->reason_light[slot] = source_item_light(property, AUX_WEATHER_SAFETY_SAFE_ITEM_NAME);
			pthread_mutex_unlock(&data->safety_mutex);
			compute_safety(agent, true);
		}
	}
	return INDIGO_OK;
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
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		AUX_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		agent_send_message,
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
