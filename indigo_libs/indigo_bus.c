// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO Bus
 \file indigo_bus.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>

#include "indigo_bus.h"
#include "indigo_names.h"
#include "indigo_io.h"

#define MAX_DEVICES 32
#define MAX_CLIENTS 8
#define MAX_BLOBS	32

#define BUFFER_SIZE	1024

static indigo_device *devices[MAX_DEVICES];
static indigo_client *clients[MAX_CLIENTS];
static indigo_property *blobs[MAX_BLOBS];
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool is_started = false;

char *indigo_property_type_text[] = {
	"UNDEFINED",
	"TEXT",
	"NUMBER",
	"SWITCH",
	"LIGHT",
	"BLOB"
};

char *indigo_property_state_text[] = {
	"Idle",
	"Ok",
	"Busy",
	"Alert"
};

char *indigo_property_perm_text[] = {
	"UNDEFINED",
	"ro",
	"rw",
	"wo"
};

char *indigo_switch_rule_text[] = {
	"UNDEFINED",
	"OneOfMany",
	"AtMostOne",
	"AnyOfMany"
};

indigo_property INDIGO_ALL_PROPERTIES;

static indigo_log_levels indigo_log_level = INDIGO_LOG_ERROR;
bool indigo_use_syslog = false;

void (*indigo_log_message_handler)(const char *message) = NULL;

char indigo_local_service_name[INDIGO_NAME_SIZE] = "";
bool indigo_use_host_suffix = true;

const char **indigo_main_argv = NULL;
int indigo_main_argc = 0;

char indigo_last_message[128 * 1024];
char indigo_log_name[255] = {0};

static void log_message(const char *format, va_list args) {
	static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&log_mutex);
	vsnprintf(indigo_last_message, sizeof(indigo_last_message), format, args);
	char *line = indigo_last_message;
	if (indigo_log_message_handler != NULL) {
		indigo_log_message_handler(indigo_last_message);
	} else if (indigo_use_syslog) {
		static bool initialize = true;
		if (initialize)
			openlog("INDIGO", LOG_NDELAY, LOG_USER | LOG_PERROR);
		while (line) {
			char *eol = strchr(line, '\n');
			if (eol)
				*eol = 0;
			if (eol > line)
				syslog (LOG_NOTICE, "%s", indigo_last_message);
			syslog (LOG_NOTICE, "%s", line);
			if (eol)
				line = eol + 1;
			else
				line = NULL;
		}
	} else {
		char timestamp[16];
		struct timeval tmnow;
		gettimeofday(&tmnow, NULL);
		strftime (timestamp, 9, "%H:%M:%S", localtime(&tmnow.tv_sec));
#ifdef INDIGO_MACOS
		snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%06d", tmnow.tv_usec);
#else
		snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%06ld", tmnow.tv_usec);
#endif
		if (indigo_log_name[0] == '\0') {
			if (indigo_main_argc == 0) {
				strncpy(indigo_log_name, "Application", sizeof(indigo_log_name));
			} else {
				char *name = strrchr(indigo_main_argv[0], '/');
				if (name != NULL) {
					name++;
				} else {
					name = (char *)indigo_main_argv[0];
				}
				strncpy(indigo_log_name, name, sizeof(indigo_log_name));
			}
		}
		while (line) {
			char *eol = strchr(line, '\n');
			if (eol)
				*eol = 0;
			if (*line)
				fprintf(stderr, "%s %s: %s\n", timestamp, indigo_log_name, line);
			if (eol)
				line = eol + 1;
			else
				line = NULL;
		}
	}
	pthread_mutex_unlock(&log_mutex);
}

void indigo_error(const char *format, ...) {
	va_list argList;
	va_start(argList, format);
	log_message(format, argList);
	va_end(argList);
}

void indigo_log(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_INFO) {
		va_list argList;
		va_start(argList, format);
		log_message(format, argList);
		va_end(argList);
	}
}

void indigo_trace(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_TRACE) {
		va_list argList;
		va_start(argList, format);
		log_message(format, argList);
		va_end(argList);
	}
}

void indigo_debug(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_DEBUG) {
		va_list argList;
		va_start(argList, format);
		log_message(format, argList);
		va_end(argList);
	}
}

void indigo_set_log_level(indigo_log_levels level) {
	indigo_log_level = level;
}

indigo_log_levels indigo_get_log_level() {
	return indigo_log_level;
}

void indigo_trace_property(const char *message, indigo_property *property, bool defs, bool items) {
	if (indigo_log_level >= INDIGO_LOG_TRACE) {
		if (message != NULL)
			indigo_trace(message);
		if (defs)
			indigo_trace("'%s'.'%s' %s %s %s %d.%d %s { // %s", property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, (property->type == INDIGO_SWITCH_VECTOR ? indigo_switch_rule_text[property->rule]: ""), property->label);
		else
			indigo_trace("'%s'.'%s' %s %s %s %d.%d %s {", property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, (property->type == INDIGO_SWITCH_VECTOR ? indigo_switch_rule_text[property->rule]: ""));
		if (items) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				switch (property->type) {
				case INDIGO_TEXT_VECTOR:
					if (defs)
						indigo_trace("  '%s' = '%s' // %s", item->name, item->text.value, item->label);
					else
						indigo_trace("  '%s' = '%s' ",item->name, item->text.value);
					break;
				case INDIGO_NUMBER_VECTOR:
					if (defs)
						indigo_trace("  '%s' = %g (%g, %g, %g) // %s", item->name, item->number.value, item->number.min, item->number.max, item->number.step, item->label);
					else
						indigo_trace("  '%s' = %g ",item->name, item->number.value);
					break;
				case INDIGO_SWITCH_VECTOR:
					if (defs)
						indigo_trace("  '%s' = %s // %s", item->name, (item->sw.value ? "On" : "Off"), item->label);
					else
						indigo_trace("  '%s' = %s ",item->name, (item->sw.value ? "On" : "Off"));
					break;
				case INDIGO_LIGHT_VECTOR:
					if (defs)
						indigo_trace("  '%s' = %s // %s", item->name, indigo_property_state_text[item->light.value], item->label);
					else
						indigo_trace("  '%s' = %s ",item->name, indigo_property_state_text[item->light.value]);
					break;
				case INDIGO_BLOB_VECTOR:
					if (defs)
						indigo_trace("  '%s' // %s", item->name, item->label);
					else
						indigo_trace("  '%s' (%ld bytes, '%s', '%s')",item->name, item->blob.size, item->blob.format, item->blob.url);
					break;
				}
			}
		}
		indigo_trace("}");
	}
}

indigo_result indigo_start() {
	for (int i = 1; i < indigo_main_argc; i++) {
		if (!strcmp(indigo_main_argv[i], "-v") || !strcmp(indigo_main_argv[i], "--enable-info")) {
			indigo_log_level = INDIGO_LOG_INFO;
		} else if (!strcmp(indigo_main_argv[i], "-vv") || !strcmp(indigo_main_argv[i], "--enable-debug")) {
			indigo_log_level = INDIGO_LOG_DEBUG;
		} else if (!strcmp(indigo_main_argv[i], "-vvv") || !strcmp(indigo_main_argv[i], "--enable-trace")) {
			indigo_log_level = INDIGO_LOG_TRACE;
		}
	}
	pthread_mutex_lock(&client_mutex);
	if (!is_started) {
		memset(devices, 0, MAX_DEVICES * sizeof(indigo_device *));
		memset(clients, 0, MAX_CLIENTS * sizeof(indigo_client *));
		memset(blobs, 0, MAX_BLOBS * sizeof(indigo_property *));
		memset(&INDIGO_ALL_PROPERTIES, 0, sizeof(INDIGO_ALL_PROPERTIES));
		INDIGO_ALL_PROPERTIES.version = INDIGO_VERSION_CURRENT;
		is_started = true;
	}
	pthread_mutex_unlock(&client_mutex);
	return INDIGO_OK;
}

indigo_result indigo_attach_device(indigo_device *device) {
	assert(device != NULL);
	pthread_mutex_lock(&device_mutex);
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] == NULL) {
			devices[i] = device;
			pthread_mutex_unlock(&device_mutex);
			if (device->attach != NULL)
				device->last_result = device->attach(device);
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return INDIGO_TOO_MANY_ELEMENTS;
}

indigo_result indigo_attach_client(indigo_client *client) {
	assert(client != NULL);
	pthread_mutex_lock(&client_mutex);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i] == NULL) {
			clients[i] = client;
			pthread_mutex_unlock(&client_mutex);
			if (client->attach != NULL)
				client->last_result = client->attach(client);
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&client_mutex);
	return INDIGO_TOO_MANY_ELEMENTS;
}

indigo_result indigo_detach_device(indigo_device *device) {
	assert(device != NULL);
	pthread_mutex_lock(&device_mutex);
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] == device) {
			if (device->detach != NULL)
				device->last_result = device->detach(device);
			devices[i] = NULL;
			pthread_mutex_unlock(&device_mutex);
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return INDIGO_OK;
}

indigo_result indigo_detach_client(indigo_client *client) {
	assert(client != NULL);
	pthread_mutex_lock(&client_mutex);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i] == client) {
			clients[i] = NULL;
			pthread_mutex_unlock(&client_mutex);
			if (client->detach != NULL)
				client->last_result = client->detach(client);
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&client_mutex);
	return INDIGO_OK;
}

indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property) {
	property->version = client ? client->version : INDIGO_VERSION_CURRENT;
	INDIGO_TRACE(indigo_trace_property("INDIGO Bus: property enumeration request", property, false, true));
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device != NULL && device->enumerate_properties != NULL) {
			bool route = *property->device == 0;
			route = route || !strcmp(property->device, device->name);
			route = route || (indigo_use_host_suffix && *device->name == '@' && strstr(property->device, device->name));
			route = route || (!indigo_use_host_suffix && *device->name == '@');
			if (route)
				device->last_result = device->enumerate_properties(device, client, property);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_change_property(indigo_client *client, indigo_property *property) {
	assert(property != NULL);
	property->version = client ? client->version : INDIGO_VERSION_CURRENT;
	INDIGO_TRACE(indigo_trace_property("INDIGO Bus: property change request", property, false, true));
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device != NULL && device->enumerate_properties != NULL) {
			bool route = *property->device == 0;
			route = route || !strcmp(property->device, device->name);
			route = route || (indigo_use_host_suffix && *device->name == '@' && strstr(property->device, device->name));
			route = route || (!indigo_use_host_suffix && *device->name == '@');
			if (route)
				device->last_result = device->change_property(device, client, property);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_enable_blob(indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	assert(property != NULL);
	property->version = client ? client->version : INDIGO_VERSION_CURRENT;
	INDIGO_TRACE(indigo_trace_property("INDIGO Bus: enable BLOB mode change request", property, false, true));
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device != NULL && device->enable_blob != NULL) {
			bool route = *property->device == 0;
			route = route || !strcmp(property->device, device->name);
			route = route || (indigo_use_host_suffix && *device->name == '@' && strstr(property->device, device->name));
			route = route || (!indigo_use_host_suffix && *device->name == '@');
			if (route)
				device->last_result = device->enable_blob(device, client, property, mode);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_define_property(indigo_device *device, indigo_property *property, const char *format, ...) {
	assert(property != NULL);
	if (!property->hidden) {
		INDIGO_TRACE(indigo_trace_property("INDIGO Bus: property definition", property, true, true));
		char message[INDIGO_VALUE_SIZE];
		if (format != NULL) {
			va_list args;
			va_start(args, format);
			vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
			va_end(args);
		}
		property->version = device ? device->version : INDIGO_VERSION_CURRENT;
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->define_property != NULL)
				client->last_result = client->define_property(client, device, property, format != NULL ? message : NULL);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_update_property(indigo_device *device, indigo_property *property, const char *format, ...) {
	assert(property != NULL);
	if (!property->hidden) {
		char message[INDIGO_VALUE_SIZE];
		INDIGO_TRACE(indigo_trace_property("INDIGO Bus: property update", property, false, true));
		if (format != NULL) {
			va_list args;
			va_start(args, format);
			vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
			va_end(args);
		}
		property->version = device ? device->version : INDIGO_VERSION_CURRENT;
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->update_property != NULL)
				client->last_result = client->update_property(client, device, property, format != NULL ? message : NULL);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_delete_property(indigo_device *device, indigo_property *property, const char *format, ...) {
	assert(property != NULL);
	if (!property->hidden) {
		char message[INDIGO_VALUE_SIZE];
		INDIGO_TRACE(indigo_trace_property("INDIGO Bus: property removal", property, false, false));
		if (format != NULL) {
			va_list args;
			va_start(args, format);
			vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
			va_end(args);
		}
		property->version = device ? device->version : INDIGO_VERSION_CURRENT;
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->delete_property != NULL)
				client->last_result = client->delete_property(client, device, property, format != NULL ? message : NULL);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_send_message(indigo_device *device, const char *format, ...) {
	INDIGO_DEBUG(indigo_debug("INDIGO Bus: message sent"));
	char message[INDIGO_VALUE_SIZE];
	if (format != NULL) {
		va_list args;
		va_start(args, format);
		vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
		va_end(args);
	}
	for (int i = 0; i < MAX_CLIENTS; i++) {
		indigo_client *client = clients[i];
		if (client != NULL && client->send_message != NULL)
			client->last_result = client->send_message(client, device, format != NULL ? message : NULL);
	}
	return INDIGO_OK;
}

indigo_result indigo_stop() {
	pthread_mutex_lock(&client_mutex);
	if (is_started) {
		for (int i = 0; i < MAX_DEVICES; i++) {
			indigo_device *device = devices[i];
			if (device != NULL && device->detach != NULL)
				device->last_result = device->detach(device);
		}
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->detach != NULL)
				client->last_result = client->detach(client);
		}
		pthread_mutex_unlock(&client_mutex);
		is_started = false;
	}
	return INDIGO_OK;
}

indigo_property *indigo_init_text_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
	if (property == NULL) {
		property = malloc(size);
		assert(property != NULL);
	}
	memset(property, 0, size);
	strncpy(property->device, device, INDIGO_NAME_SIZE);
	strncpy(property->name, name, INDIGO_NAME_SIZE);
	strncpy(property->group, group ? group : "", INDIGO_NAME_SIZE);
	strncpy(property->label, label ? label : "", INDIGO_VALUE_SIZE);
	property->type = INDIGO_TEXT_VECTOR;
	property->state = state;
	property->perm = perm;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	return property;
}

indigo_property *indigo_init_number_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	if (property == NULL) {
		property = malloc(size);
		assert(property != NULL);
	}
	memset(property, 0, size);
	strncpy(property->device, device, INDIGO_NAME_SIZE);
	strncpy(property->name, name, INDIGO_NAME_SIZE);
	strncpy(property->group, group ? group : "", INDIGO_NAME_SIZE);
	strncpy(property->label, label ? label : "", INDIGO_VALUE_SIZE);
	property->type = INDIGO_NUMBER_VECTOR;
	property->state = state;
	property->perm = perm;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	return property;
}

indigo_property *indigo_init_switch_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	if (property == NULL) {
		property = malloc(size);
		assert(property != NULL);
	}
	memset(property, 0, size);
	strncpy(property->device, device, INDIGO_NAME_SIZE);
	strncpy(property->name, name, INDIGO_NAME_SIZE);
	strncpy(property->group, group ? group : "", INDIGO_NAME_SIZE);
	strncpy(property->label, label ? label : "", INDIGO_VALUE_SIZE);
	property->type = INDIGO_SWITCH_VECTOR;
	property->state = state;
	property->perm = perm;
	property->rule = rule;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	return property;
}

indigo_property *indigo_init_light_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	if (property == NULL) {
		property = malloc(size);
		assert(property != NULL);
	}
	memset(property, 0, size);
	strncpy(property->device, device, INDIGO_NAME_SIZE);
	strncpy(property->name, name, INDIGO_NAME_SIZE);
	strncpy(property->group, group ? group : "", INDIGO_NAME_SIZE);
	strncpy(property->label, label ? label : "", INDIGO_VALUE_SIZE);
	property->type = INDIGO_LIGHT_VECTOR;
	property->perm = INDIGO_RO_PERM;
	property->state = state;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	return property;
}

indigo_property *indigo_init_blob_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	if (property == NULL) {
		property = malloc(size);
		assert(property != NULL);
	}
	memset(property, 0, size);
	strncpy(property->device, device, INDIGO_NAME_SIZE);
	strncpy(property->name, name, INDIGO_NAME_SIZE);
	strncpy(property->group, group ? group : "", INDIGO_NAME_SIZE);
	strncpy(property->label, label ? label : "", INDIGO_VALUE_SIZE);
	property->type = INDIGO_BLOB_VECTOR;
	property->perm = INDIGO_RO_PERM;
	property->state = state;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	for (int i = 0; i < MAX_BLOBS; i++)
		if (blobs[i] == NULL) {
			blobs[i] = property;
			break;
		}
	return property;
}

indigo_property *indigo_resize_property(indigo_property *property, int count) {
	assert(property != NULL);
	property = realloc(property, sizeof(indigo_property) + count * sizeof(indigo_item));
	assert(property != NULL);
	if (count > property->count)
		memset(property->items+property->count, 0, (count - property->count) * sizeof(indigo_item));
	property->count = count;
	return property;
}

void indigo_release_property(indigo_property *property) {
	assert(property != NULL);
	for (int i = 0; i < MAX_BLOBS; i++)
		if (blobs[i] == property) {
			blobs[i] = NULL;
			break;
		}
	free(property);
}

indigo_result indigo_validate_blob(indigo_item *item) {
	for (int i = 0; i < MAX_BLOBS; i++) {
		indigo_property *property = blobs[i];
		if (property != NULL) {
			for (int j = 0; j < property->count; j++) {
				if (item == &property->items[j])
					return INDIGO_OK;
			}
		}
	}
	return INDIGO_FAILED;
}


void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *format, ...) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	strncpy(item->name, name, INDIGO_NAME_SIZE);
	strncpy(item->label, label ? label : "", INDIGO_VALUE_SIZE);
	va_list args;
	va_start(args, format);
	vsnprintf(item->text.value, INDIGO_VALUE_SIZE, format, args);
	va_end(args);
}

void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	strncpy(item->name, name, INDIGO_NAME_SIZE);
	strncpy(item->label, label ? label : "", INDIGO_VALUE_SIZE);
	strncpy(item->number.format, "%g", INDIGO_VALUE_SIZE);
	item->number.min = min;
	item->number.max = max;
	item->number.step = step;
	item->number.target = item->number.value = value;
}

void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	strncpy(item->name, name, INDIGO_NAME_SIZE);
	strncpy(item->label, label ? label : "", INDIGO_VALUE_SIZE);
	item->sw.value = value;
}

void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	strncpy(item->name, name, INDIGO_NAME_SIZE);
	strncpy(item->label, label ? label : "", INDIGO_VALUE_SIZE);
	item->light.value = value;
}

void indigo_init_blob_item(indigo_item *item, const char *name, const char *label) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	strncpy(item->name, name, INDIGO_NAME_SIZE);
	strncpy(item->label, label ? label : "", INDIGO_VALUE_SIZE);
}

void *indigo_alloc_blob_buffer(long size) {
	int mod2880 = size % 2880;
	if (mod2880) {
		return malloc(size + 2880 - mod2880);
	}
	return malloc(size);
}

bool indigo_populate_http_blob_item(indigo_item *blob_item) {
	char host[BUFFER_SIZE] = {0};
	int port = 80;
	char file[BUFFER_SIZE] = {0};
	char request[BUFFER_SIZE];
	char http_line[BUFFER_SIZE];
	char http_response[BUFFER_SIZE];
	long content_len = 0;;
	int http_result = 0;
	char *image_type;
	int socket;
	int res;
	int count;

	if ((blob_item->blob.url[0] == '\0') || strcmp(blob_item->name, CCD_IMAGE_ITEM_NAME)) {
		INDIGO_DEBUG(indigo_debug("%s(): url == \"\" or item != \"%s\"", __FUNCTION__, CCD_IMAGE_ITEM_NAME));
		return false;
	}
	sscanf(blob_item->blob.url, "http://%255[^:]:%5d/%1024[^\n]", host, &port, file);
	socket = indigo_open_tcp(host, port);
	if (socket < 0) {
		return false;
	}

	snprintf(request, BUFFER_SIZE, "GET /%s HTTP/1.1\r\n\r\n", file);
	res = indigo_write(socket, request, strlen(request));
	if (res == false)
		goto clean_return;

	res = indigo_read_line(socket, http_line, BUFFER_SIZE);
	if (res < 0)
		goto clean_return;

	count = sscanf(http_line, "HTTP/1.1 %d %255[^\n]", &http_result, http_response);
	if ((count != 2) || (http_result != 200)){
		INDIGO_DEBUG(indigo_debug("%s(): http_line = \"%s\"", __FUNCTION__, http_line));
		shutdown(socket, SHUT_RDWR);
		close(socket);
		return false;
	}
	INDIGO_DEBUG(indigo_debug("%s(): http_result = %d, response = \"%s\"", __FUNCTION__, http_result, http_response));

	do {
		res = indigo_read_line(socket, http_line, BUFFER_SIZE);
		if (res < 0)
			goto clean_return;
		INDIGO_DEBUG(indigo_debug("%s(): http_line = \"%s\"", __FUNCTION__, http_line));
		count = sscanf(http_line, "Content-Length: %20ld[^\n]", &content_len);
	} while (http_line[0] != '\0');

	INDIGO_DEBUG(indigo_debug("%s(): content_len = %ld", __FUNCTION__, content_len));

	if (content_len) {
		image_type = strrchr(file, '.');
		if (image_type) strncpy(blob_item->blob.format, image_type, INDIGO_NAME_SIZE);
		blob_item->blob.size = content_len;
		blob_item->blob.value = realloc(blob_item->blob.value, blob_item->blob.size);
		res = (indigo_read(socket, blob_item->blob.value, blob_item->blob.size) >= 0) ? true : false;
	} else {
		res = false;
	}

	clean_return:
	INDIGO_DEBUG(indigo_debug("%s() = %d", __FUNCTION__, res));
	shutdown(socket, SHUT_RDWR);
	close(socket);
	return res;
}


bool indigo_property_match(indigo_property *property, indigo_property *other) {
	assert(property != NULL);
	return other == NULL || ((other->type == 0 || property->type == other->type) && (*other->device == 0 || !strcmp(property->device, other->device)) && (*other->name == 0 || !strcmp(property->name, other->name)));
}

bool indigo_switch_match(indigo_item *item, indigo_property *other) {
	assert(item != NULL);
	assert(other != NULL);
	assert(other->type == INDIGO_SWITCH_VECTOR);
	for (int i = 0; i < other->count; i++) {
		indigo_item *other_item = other->items+i;
		if (!strcmp(item->name, other_item->name)) {
			return other_item->sw.value;
		}
	}
	return false;
}

void indigo_set_switch(indigo_property *property, indigo_item *item, bool value) {
	assert(property != NULL);
	assert(property->type == INDIGO_SWITCH_VECTOR);
	if (property->rule != INDIGO_ANY_OF_MANY_RULE) {
		for (int i = 0; i < property->count; i++) {
			property->items[i].sw.value = false;
		}
	}
	item->sw.value = value;
}

bool indigo_get_switch(indigo_property *property, char *item_name) {
	assert(property != NULL);
	assert(property->type == INDIGO_SWITCH_VECTOR);
	assert(item_name != NULL);
	for (int i = 0; i < property->count; i++)
		if (!strcmp(property->items[i].name, item_name))
			return property->items[i].sw.value;
	return false;
}

void indigo_property_copy_values(indigo_property *property, indigo_property *other, bool with_state) {
	assert(property != NULL);
	assert(other != NULL);
	if (property->perm != INDIGO_RO_PERM) {
		if (property->type == other->type) {
			if (with_state)
				property->state = other->state;
			if (property->type == INDIGO_SWITCH_VECTOR && property->rule != INDIGO_ANY_OF_MANY_RULE) {
				for (int j = 0; j < property->count; j++) {
					property->items[j].sw.value = false;
				}
			}
			for (int i = 0; i < other->count; i++) {
				indigo_item *other_item = &other->items[i];
				for (int j = 0; j < property->count; j++) {
					indigo_item *property_item = &property->items[j];
					if (!strcmp(property_item->name, other_item->name)) {
						switch (property->type) {
						case INDIGO_TEXT_VECTOR:
							strncpy(property_item->text.value, other_item->text.value, INDIGO_VALUE_SIZE);
							break;
						case INDIGO_NUMBER_VECTOR:
							property_item->number.target = property_item->number.value = other_item->number.value;
							if (property_item->number.value < property_item->number.min)
								property_item->number.target = property_item->number.value = property_item->number.min;
							if (property_item->number.value > property_item->number.max)
								property_item->number.target = property_item->number.value = property_item->number.max;
							break;
						case INDIGO_SWITCH_VECTOR:
							property_item->sw.value = other_item->sw.value;
							break;
						case INDIGO_LIGHT_VECTOR:
							property_item->light.value = other_item->light.value;
							break;
						case INDIGO_BLOB_VECTOR:
							strncpy(property_item->blob.format, other_item->blob.format, INDIGO_NAME_SIZE);
							strncpy(property_item->blob.url, other_item->blob.url, INDIGO_NAME_SIZE);
							property_item->blob.size = other_item->blob.size;
							property_item->blob.value = other_item->blob.value;
							break;
						}
						break;
					}
				}
			}
		}
	}
}

indigo_result indigo_change_text_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const char **values) {
	indigo_property *property = indigo_init_text_property(NULL, device, name, NULL, NULL, 0, 0, count);
	for (int i = 0; i < count; i++)
		indigo_init_text_item(&property->items[i], items[i], NULL, values[i]);
	indigo_result result = indigo_change_property(client, property);
	free(property);
	return result;
}

indigo_result indigo_change_number_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const double *values) {
	indigo_property *property = indigo_init_number_property(NULL, device, name, NULL, NULL, 0, 0, count);
	for (int i = 0; i < count; i++)
		indigo_init_number_item(&property->items[i], items[i], NULL, 0, 0, 0, values[i]);
	indigo_result result = indigo_change_property(client, property);
	free(property);
	return result;
}

indigo_result indigo_change_switch_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const bool *values) {
	indigo_property *property = indigo_init_switch_property(NULL, device, name, NULL, NULL, 0, 0, 0, count);
	for (int i = 0; i < count; i++)
		indigo_init_switch_item(&property->items[i], items[i], NULL, values[i]);
	indigo_result result = indigo_change_property(client, property);
	free(property);
	return result;
}

indigo_result indigo_device_connect(indigo_client *client, char *device) {
	static const char *items[] = { CONNECTION_CONNECTED_ITEM_NAME, CONNECTION_DISCONNECTED_ITEM_NAME };
	static bool values[] = { true, false };
	return indigo_change_switch_property(client, device, CONNECTION_PROPERTY_NAME, 2, items, values);
}

indigo_result indigo_device_disconnect(indigo_client *client, char *device) {
	static const char *items[] = { CONNECTION_CONNECTED_ITEM_NAME, CONNECTION_DISCONNECTED_ITEM_NAME };
	static bool values[] = { false, true };
	return indigo_change_switch_property(client, device, CONNECTION_PROPERTY_NAME, 2, items, values);
}
