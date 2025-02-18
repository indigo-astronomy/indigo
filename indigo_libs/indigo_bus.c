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

#if defined(INDIGO_LINUX)
#define _GNU_SOURCE
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#endif
#if defined(INDIGO_WINDOWS)
#include <io.h>
#include <winsock2.h>
#pragma warning(disable:4996)
#define strcasecmp stricmp
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_token.h>

#define MAX_DEVICES 256
#define MAX_CLIENTS 256
#define MAX_BLOBS	32

#define BUFFER_SIZE	1024

#define isdigit(c) (c >= '0' && c <= '9')
#define isspace(c) (c == ' ')

static indigo_device *devices[MAX_DEVICES];
static indigo_client *clients[MAX_CLIENTS];
static indigo_blob_entry *blobs[MAX_BLOBS];

static pthread_mutex_t bus_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#define client_mutex bus_mutex
#define device_mutex bus_mutex

bool indigo_use_strict_locking = true;

static pthread_mutex_t blob_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void (*indigo_log_message_handler)(indigo_log_levels level, const char *message) = NULL;

char indigo_local_service_name[INDIGO_NAME_SIZE] = "";
bool indigo_reshare_remote_devices = false;
bool indigo_use_host_suffix = true;
bool indigo_is_sandboxed = false;
bool indigo_use_blob_caching = false;
bool indigo_proxy_blob = false;

const char **indigo_main_argv = NULL;
int indigo_main_argc = 0;

#define LOG_MESSAGE_SIZE	(128 * 1024)

char *indigo_last_message = NULL;
char indigo_log_name[255] = { 0 };

extern void indigo_get_version(int *major, int *minor, int *build) {
	if (major) *major = INDIGO_VERSION_CURRENT >> 8;
	if (minor) *minor = INDIGO_VERSION_CURRENT & 0xFF;
	if (build) *build = atoi(INDIGO_BUILD);
}

static void free_log_buffers() {
	indigo_safe_free(indigo_last_message);
}

#if defined(INDIGO_WINDOWS)

// https://stackoverflow.com/questions/10905892/equivalent-of-gettimeday-for-windows

int gettimeofday(struct timeval * tp, struct timezone * tzp) {
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t) file_time.dwLowDateTime);
	time += ((uint64_t) file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long) ((time - EPOCH) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
	return 0;
}
#endif

#if defined(INDIGO_MACOS)
int clock_gettime(clockid_t clk_id, struct timespec *ts) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0)
		return -1;
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
	return 0;
}
#endif

void indigo_log_base(indigo_log_levels level, const char *format, va_list args) {

	static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&log_mutex);
	if (indigo_last_message == NULL) {
		indigo_last_message = indigo_safe_malloc(LOG_MESSAGE_SIZE);
		atexit(free_log_buffers);
	}
	vsnprintf(indigo_last_message, LOG_MESSAGE_SIZE, format, args);
	char *line = indigo_last_message;
	char prefix[32] = { 0 };
	char *arrow = strstr(line, " -> ");
	if (arrow && arrow - line < 28) {
		strncpy(prefix, line, arrow - line + 4);
	} else {
		arrow = strstr(line, " <- ");
		if (arrow && arrow - line < 28) {
			strncpy(prefix, line, arrow - line + 4);
		}
	}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (indigo_use_syslog) {
		static bool initialize = true;
		if (initialize) {
			openlog("INDIGO", LOG_NDELAY, LOG_USER | LOG_PERROR);
			initialize = false;
		}
	}
#endif
	char timestamp[16];
	struct timeval tmnow;
	gettimeofday(&tmnow, NULL);
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		lt = localtime(&rawtime);
	}
	strftime (timestamp, 9, "%H:%M:%S", lt);
#else
	strftime (timestamp, 9, "%H:%M:%S", localtime((const time_t *) &tmnow.tv_sec));
#endif

#ifdef INDIGO_MACOS
	snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%06d", tmnow.tv_usec);
#else
	snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%06ld", tmnow.tv_usec);
#endif
	if (indigo_log_name[0] == '\0') {
		if (indigo_main_argc == 0) {
			strncpy(indigo_log_name, "Application", sizeof(indigo_log_name));
		} else {
#if defined(INDIGO_WINDOWS)
			char *name = strrchr(indigo_main_argv[0], '\\');
#else
			char *name = strrchr(indigo_main_argv[0], '/');
#endif
			if (name != NULL) {
				name++;
			} else {
				name = (char *)indigo_main_argv[0];
			}
			strncpy(indigo_log_name, name, sizeof(indigo_log_name));
		}
	}
	bool first_line = true;
	static char tmp[128];
	while (line) {
		char *eol = strchr(line, '\n');
		if (eol)
			*eol = 0;
		if (*line) {
			if (first_line || *prefix == 0) {
				if (indigo_log_message_handler != NULL) {
					indigo_log_message_handler(level, line);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
				} else if (indigo_use_syslog) {
					syslog(LOG_NOTICE, "%s", line);
#endif
				} else {
					fprintf(stderr, "%s %s: %s\n", timestamp, indigo_log_name, line);
				}
				first_line = false;
			} else {
				if (indigo_log_message_handler != NULL) {
					snprintf(tmp, sizeof(tmp), "%s%s", prefix, line);
					indigo_log_message_handler(level, tmp);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
				} else if (indigo_use_syslog) {
					snprintf(tmp, sizeof(tmp), "%s%s", prefix, line);
					syslog(LOG_NOTICE, "%s", line);
#endif
				} else {
					fprintf(stderr, "%s %s: %s%s\n", timestamp, indigo_log_name, prefix, line);
				}
			}
		}
		if (eol)
			line = eol + 1;
		else
			line = NULL;
	}
	pthread_mutex_unlock(&log_mutex);
}

void indigo_log_message(const char *format, va_list args) {
	indigo_log_base(INDIGO_LOG_PLAIN, format, args);
}

void indigo_error(const char *format, ...) {
	va_list argList;
	va_start(argList, format);
	indigo_log_base(INDIGO_LOG_ERROR, format, argList);
	va_end(argList);
}

void indigo_log(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_INFO) {
		va_list argList;
		va_start(argList, format);
		indigo_log_base(INDIGO_LOG_INFO, format, argList);
		va_end(argList);
	}
}

void indigo_trace(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_TRACE) {
		va_list argList;
		va_start(argList, format);
		indigo_log_base(INDIGO_LOG_TRACE, format, argList);
		va_end(argList);
	}
}

void indigo_debug(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_DEBUG) {
		va_list argList;
		va_start(argList, format);
		indigo_log_base(INDIGO_LOG_DEBUG, format, argList);
		va_end(argList);
	}
}

void indigo_trace_bus(const char *format, ...) {
	if (indigo_log_level >= INDIGO_LOG_TRACE_BUS) {
		va_list argList;
		va_start(argList, format);
		indigo_log_base(INDIGO_LOG_TRACE_BUS, format, argList);
		va_end(argList);
	}
}

void indigo_set_log_level(indigo_log_levels level) {
	indigo_log_level = level;
}

indigo_log_levels indigo_get_log_level() {
	return indigo_log_level;
}

void indigo_trace_property(const char *message, indigo_client *client, indigo_property *property, bool defs, bool items) {
	if (indigo_log_level >= INDIGO_LOG_TRACE_BUS) {
		static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_lock(&log_mutex);
		if (defs) {
			indigo_trace_bus("B <+ %s '%s'.'%s' %s %s %s %d.%d %x %s { // %s", message, property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, property->access_token, (property->type == INDIGO_SWITCH_VECTOR ? indigo_switch_rule_text[property->rule]: ""), property->label);
		} else {
			if (client) {
				indigo_trace_bus("B <+ %s '%s'.'%s' %s %s %s %d.%d %x '%s' {", message, property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, property->access_token, client->name);
			} else {
				indigo_trace_bus("B <+ %s '%s'.'%s' %s %s %s %d.%d %x {", message, property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, property->access_token);
			}
		}
		if (items) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				switch (property->type) {
				case INDIGO_TEXT_VECTOR:
					if (defs) {
						if (item->text.long_value)
							indigo_trace_bus("B <+   '%s' = '%s' + %d extra characters // %s", item->name, item->text.value, item->text.length - 1, item->label);
						else
							indigo_trace_bus("B <+   '%s' = '%s' // %s", item->name, item->text.value, item->label);
					} else {
						if (item->text.long_value)
							indigo_trace_bus("B <+   '%s' = '%s' + %d extra characters",item->name, item->text.value, item->text.length - 1);
						else
							indigo_trace_bus("B <+   '%s' = '%s'",item->name, item->text.value);
					}
					break;
				case INDIGO_NUMBER_VECTOR:
					if (defs)
						indigo_trace_bus("B <+   '%s' = %g, target = %g (%g, %g, %g, '%s') // %s", item->name, item->number.value, item->number.target, item->number.min, item->number.max, item->number.step, item->number.format, item->label);
					else
						indigo_trace_bus("B <+   '%s' = %g, target = %g ",item->name, item->number.value, item->number.target);
					break;
				case INDIGO_SWITCH_VECTOR:
					if (defs)
						indigo_trace_bus("B <+   '%s' = %s // %s", item->name, (item->sw.value ? "On" : "Off"), item->label);
					else
						indigo_trace_bus("B <+   '%s' = %s ",item->name, (item->sw.value ? "On" : "Off"));
					break;
				case INDIGO_LIGHT_VECTOR:
					if (defs)
						indigo_trace_bus("B <+   '%s' = %s // %s", item->name, indigo_property_state_text[item->light.value], item->label);
					else
						indigo_trace_bus("B <+   '%s' = %s ",item->name, indigo_property_state_text[item->light.value]);
					break;
				case INDIGO_BLOB_VECTOR:
					if (defs)
						indigo_trace_bus("B <+   '%s' // %s", item->name, item->label);
					else
						indigo_trace_bus("B <+   '%s' (%ld bytes, '%s', '%s')",item->name, item->blob.size, item->blob.format, item->blob.url);
					break;
				}
			}
		}
		indigo_trace_bus("B <- }");
		pthread_mutex_unlock(&log_mutex);
	}
}

indigo_result indigo_start() {
	for (int i = 1; i < indigo_main_argc; i++) {
		if (!strcmp(indigo_main_argv[i], "-v") || !strcmp(indigo_main_argv[i], "--enable-info")) {
			indigo_log_level = INDIGO_LOG_INFO;
		} else if (!strcmp(indigo_main_argv[i], "-vv") || !strcmp(indigo_main_argv[i], "--enable-debug")) {
			indigo_log_level = INDIGO_LOG_DEBUG;
		} else if (!strcmp(indigo_main_argv[i], "-vvb") || !strcmp(indigo_main_argv[i], "--enable-trace-bus")) {
			indigo_log_level = INDIGO_LOG_TRACE_BUS;
		} else if (!strcmp(indigo_main_argv[i], "-vvv") || !strcmp(indigo_main_argv[i], "--enable-trace")) {
			indigo_log_level = INDIGO_LOG_TRACE;
		}
	}
	pthread_mutex_lock(&device_mutex);
	pthread_mutex_lock(&client_mutex);
	if (!is_started) {
		memset(devices, 0, MAX_DEVICES * sizeof(indigo_device *));
		memset(clients, 0, MAX_CLIENTS * sizeof(indigo_client *));
		memset(blobs, 0, MAX_BLOBS * sizeof(indigo_property *));
		memset(&INDIGO_ALL_PROPERTIES, 0, sizeof(INDIGO_ALL_PROPERTIES));
		is_started = true;
	}
#if defined(INDIGO_WINDOWS)
	WORD version_requested = MAKEWORD(1, 1);
	WSADATA data;
	WSAStartup(version_requested, &data);
#endif
	pthread_mutex_unlock(&client_mutex);
	pthread_mutex_unlock(&device_mutex);
	return INDIGO_OK;
}

indigo_result indigo_attach_device(indigo_device *device) {
	static int max_index = -1;
	if ((!is_started) || (device == NULL))
		return INDIGO_FAILED;
	pthread_mutex_lock(&device_mutex);
	INDIGO_DEBUG(indigo_trace_bus("B <- Attach device '%s'", device->name));
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] == NULL) {
			if (i > max_index) {
				max_index = i;
				INDIGO_TRACE(indigo_trace("%d devices attached", max_index + 1));
			}
			devices[i] = device;
			pthread_mutex_unlock(&device_mutex);
			device->access_token = 0;
			if (device->attach != NULL)
				device->last_result = device->attach(device);
			if (!device->is_remote && device->change_property) {
				indigo_property *property = indigo_init_switch_property(NULL, device->name, CONFIG_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
				indigo_init_switch_item(property->items, CONFIG_LOAD_ITEM_NAME, NULL, true);
				device->change_property(device, NULL, property);
				indigo_release_property(property);
			}
			pthread_mutex_lock(&device_mutex);
			device->access_token = indigo_get_device_token(device->name);
			pthread_mutex_unlock(&device_mutex);
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	indigo_error("[%s:%d] Max device count reached", __FUNCTION__, __LINE__);
	return INDIGO_TOO_MANY_ELEMENTS;
}

indigo_result indigo_attach_client(indigo_client *client) {
	static int max_index = -1;
	if ((!is_started) || (client == NULL))
		return INDIGO_FAILED;
	pthread_mutex_lock(&client_mutex);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i] == NULL) {
			if (i > max_index) {
				max_index = i;
				INDIGO_TRACE(indigo_trace("%d clients attached", max_index + 1));
			}
			clients[i] = client;
			pthread_mutex_unlock(&client_mutex);
			if (client->attach != NULL)
				client->last_result = client->attach(client);
			INDIGO_DEBUG(indigo_trace_bus("B <- Attach client '%s'", client->name));
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&client_mutex);
	indigo_error("[%s:%d] Max client count reached", __FUNCTION__, __LINE__);
	return INDIGO_TOO_MANY_ELEMENTS;
}

indigo_result indigo_detach_device(indigo_device *device) {
	if ((!is_started) || (device == NULL))
		return INDIGO_FAILED;
	pthread_mutex_lock(&device_mutex);
	INDIGO_DEBUG(indigo_trace_bus("B <- Detach device '%s'", device->name));
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] == device) {
			devices[i] = NULL;
			pthread_mutex_unlock(&device_mutex);
			if (device->detach != NULL) {
				indigo_property *all_properties = indigo_init_text_property(NULL, device->name, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
				indigo_delete_property(device, all_properties, NULL);
				indigo_release_property(all_properties);
				device->last_result = device->detach(device);
			}
			return INDIGO_OK;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return INDIGO_NOT_FOUND;
}

indigo_result indigo_detach_client(indigo_client *client) {
	if ((!is_started) || (client == NULL))
		return INDIGO_FAILED;
	pthread_mutex_lock(&client_mutex);
	INDIGO_DEBUG(indigo_trace_bus("B <- Detach client '%s'", client->name));
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
	return INDIGO_NOT_FOUND;
}

indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property) {
	if (!is_started)
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&device_mutex);
	}
	INDIGO_TRACE(indigo_trace_property("Enumerate", client, property, false, false));
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
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&device_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_change_property(indigo_client *client, indigo_property *property) {
	if ((!is_started) || (property == NULL))
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&device_mutex);
	}
	INDIGO_TRACE(indigo_trace_property("Change", client, property, false, true));
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device != NULL && device->change_property != NULL) {
			bool route = *property->device == 0;
			route = route || !strcmp(property->device, device->name);
			route = route || (indigo_use_host_suffix && *device->name == '@' && strstr(property->device, device->name));
			route = route || (!indigo_use_host_suffix && *device->name == '@');
			if (route) {
				if (device->access_token != 0 && device->access_token != property->access_token && property->access_token != indigo_get_master_token()) {
					indigo_send_message(device, "Device '%s' is protected or locked for exclusive access", device->name);
					continue;
				}
				device->last_result = device->change_property(device, client, property);
			}
		}
	}
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&device_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_enable_blob(indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	if ((!is_started) || (property == NULL))
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&device_mutex);
	}
	INDIGO_TRACE(indigo_trace_property("Enable BLOB mode", client, property, false, true));
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
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&device_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_define_property(indigo_device *device, indigo_property *property, const char *format, ...) {
	if ((!is_started) || (property == NULL))
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&client_mutex);
	}
	if (!property->hidden) {
		INDIGO_TRACE(indigo_trace_property("Define", NULL, property, true, true));
		property->defined = true;
		char message[INDIGO_VALUE_SIZE];
		if (format != NULL) {
			va_list args;
			va_start(args, format);
			vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
			va_end(args);
		}
		if (indigo_use_blob_caching && property->type == INDIGO_BLOB_VECTOR && property->perm == INDIGO_WO_PERM) {
			pthread_mutex_lock(&blob_mutex);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				indigo_blob_entry *entry = NULL;
				int free_index = -1;
				for (int j = 0; j < MAX_BLOBS; j++) {
					entry = blobs[j];
					if (entry && entry->item == item) {
						break;
					}
					if (entry == NULL && free_index == -1)
						free_index = j;
					entry = NULL;
				}
				if (entry == NULL && free_index >= 0) {
					entry = indigo_safe_malloc(sizeof(indigo_blob_entry));
					blobs[free_index] = entry;
					memset(entry, 0, sizeof(indigo_blob_entry));
					entry->item = item;
					entry->property = property;
					pthread_mutex_init(&entry->mutext, NULL);
				}
				if (entry == NULL) {
					pthread_mutex_unlock(&blob_mutex);
					if (indigo_use_strict_locking) {
						pthread_mutex_unlock(&client_mutex);
					}
					indigo_error("[%s:%d] Max BLOB count reached", __FUNCTION__, __LINE__);
					return INDIGO_TOO_MANY_ELEMENTS;
				}
			}
			pthread_mutex_unlock(&blob_mutex);
		}
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->define_property != NULL)
				client->last_result = client->define_property(client, device, property, format != NULL ? message : NULL);
		}
	}
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&client_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_update_property(indigo_device *device, indigo_property *property, const char *format, ...) {
	if ((!is_started) || (property == NULL))
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&client_mutex);
	}
	if (!property->hidden) {
		char message[INDIGO_VALUE_SIZE];
		int count = property->count;
		if (property->perm == INDIGO_WO_PERM)
			property->count = 0;
		INDIGO_TRACE(indigo_trace_property("Update", NULL, property, false, true));
		if (format != NULL) {
			va_list args;
			va_start(args, format);
			vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
			va_end(args);
		}
		if (indigo_use_blob_caching && property->type == INDIGO_BLOB_VECTOR && property->perm == INDIGO_RO_PERM && property->state == INDIGO_OK_STATE) {
			pthread_mutex_lock(&blob_mutex);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				indigo_blob_entry *entry = NULL;
				int free_index = -1;
				for (int j = 0; j < MAX_BLOBS; j++) {
					entry = blobs[j];
					if (entry && entry->item == item) {
						break;
					}
					if (entry == NULL && free_index == -1)
						free_index = j;
					entry = NULL;
				}
				if (entry == NULL && free_index >= 0) {
					entry = indigo_safe_malloc(sizeof(indigo_blob_entry));
					blobs[free_index] = entry;
					memset(entry, 0, sizeof(indigo_blob_entry));
					entry->item = item;
					entry->property = property;
					pthread_mutex_init(&entry->mutext, NULL);
				}
				if (entry) {
					pthread_mutex_lock(&entry->mutext);
					if (item->blob.size) {
						entry->content = indigo_safe_realloc(entry->content, entry->size = item->blob.size);
						memcpy(entry->content, item->blob.value, entry->size);
						strcpy(entry->format, item->blob.format);
					} else if (entry->content) {
						free(entry->content);
						entry->size = 0;
						entry->content = NULL;
					}
					pthread_mutex_unlock(&entry->mutext);
				} else {
					pthread_mutex_unlock(&blob_mutex);
					if (indigo_use_strict_locking) {
						pthread_mutex_unlock(&client_mutex);
					}
					indigo_error("[%s:%d] Max BLOB count reached", __FUNCTION__, __LINE__);
					return INDIGO_TOO_MANY_ELEMENTS;
				}
			}
			pthread_mutex_unlock(&blob_mutex);
		}
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->update_property != NULL)
				client->last_result = client->update_property(client, device, property, format != NULL ? message : NULL);
		}
		property->count = count;
	}
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&client_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_delete_property(indigo_device *device, indigo_property *property, const char *format, ...) {
	if ((!is_started) || (property == NULL))
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&client_mutex);
	}
	if (!property->hidden) {
		INDIGO_TRACE(indigo_trace_property("Remove", NULL, property, false, false));
		property->defined = false;
		char message[INDIGO_VALUE_SIZE];
		if (format != NULL) {
			va_list args;
			va_start(args, format);
			vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
			va_end(args);
		}
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->delete_property != NULL)
				client->last_result = client->delete_property(client, device, property, format != NULL ? message : NULL);
		}
	}
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&client_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_send_message(indigo_device *device, const char *format, ...) {
	if (!is_started)
		return INDIGO_FAILED;
	if (indigo_use_strict_locking) {
		pthread_mutex_lock(&client_mutex);
	}
	char message[INDIGO_VALUE_SIZE];
	if (format != NULL) {
		va_list args;
		va_start(args, format);
		vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
		va_end(args);
	}
	INDIGO_DEBUG(indigo_trace_bus("B <- Sent message '%s'", message));
	for (int i = 0; i < MAX_CLIENTS; i++) {
		indigo_client *client = clients[i];
		if (client != NULL && client->send_message != NULL)
			client->last_result = client->send_message(client, device, format != NULL ? message : NULL);
	}
	if (indigo_use_strict_locking) {
		pthread_mutex_unlock(&client_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_stop() {
	INDIGO_DEBUG(indigo_trace_bus("B <- Stop bus"));
	if (is_started) {
		pthread_mutex_lock(&client_mutex);
		for (int i = 0; i < MAX_CLIENTS; i++) {
			indigo_client *client = clients[i];
			if (client != NULL && client->detach != NULL) {
				clients[i] = NULL;
				client->last_result = client->detach(client);
			}
		}
		pthread_mutex_unlock(&client_mutex);
		pthread_mutex_lock(&device_mutex);
		for (int i = 0; i < MAX_DEVICES; i++) {
			indigo_device *device = devices[i];
			if (device != NULL) {
				indigo_error("INDIGO Bus: can't stop, '%s' is attached", device->name);
				pthread_mutex_unlock(&device_mutex);
				return INDIGO_BUSY;
			}
		}
		pthread_mutex_unlock(&device_mutex);
		is_started = false;
	}
	return INDIGO_OK;
}

indigo_property *indigo_init_text_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
	int allocated_count = count;
	if (property == NULL) {
		property = indigo_safe_malloc(size);
	} else {
		property = indigo_resize_property(property, count);
		allocated_count = property->allocated_count;
	}
	memset(property, 0, size);
	indigo_copy_name(property->device, device);
	indigo_copy_name(property->name, name);
	indigo_copy_name(property->group, group ? group : "");
	indigo_copy_value(property->label, label ? label : "");
	property->type = INDIGO_TEXT_VECTOR;
	property->state = state;
	property->perm = perm;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	property->allocated_count = allocated_count;
	return property;
}

indigo_property *indigo_init_number_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	int allocated_count = count;
	if (property == NULL) {
		property = indigo_safe_malloc(size);
	} else {
		property = indigo_resize_property(property, count);
		allocated_count = property->allocated_count;
	}
	memset(property, 0, size);
	indigo_copy_name(property->device, device);
	indigo_copy_name(property->name, name);
	indigo_copy_name(property->group, group ? group : "");
	indigo_copy_value(property->label, label ? label : "");
	property->type = INDIGO_NUMBER_VECTOR;
	property->state = state;
	property->perm = perm;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	property->allocated_count = allocated_count;
	return property;
}

indigo_property *indigo_init_switch_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	int allocated_count = count;
	if (property == NULL) {
		property = indigo_safe_malloc(size);
	} else {
		property = indigo_resize_property(property, count);
		allocated_count = property->allocated_count;
	}
	memset(property, 0, size);
	indigo_copy_name(property->device, device);
	indigo_copy_name(property->name, name);
	indigo_copy_name(property->group, group ? group : "");
	indigo_copy_value(property->label, label ? label : "");
	property->type = INDIGO_SWITCH_VECTOR;
	property->state = state;
	property->perm = perm;
	property->rule = rule;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	property->allocated_count = allocated_count;
	return property;
}

indigo_property *indigo_init_light_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
	assert(device != NULL);
	assert(name != NULL);
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	int allocated_count = count;
	if (property == NULL) {
		property = indigo_safe_malloc(size);
	} else {
		property = indigo_resize_property(property, count);
		allocated_count = property->allocated_count;
	}
	memset(property, 0, size);
	indigo_copy_name(property->device, device);
	indigo_copy_name(property->name, name);
	indigo_copy_name(property->group, group ? group : "");
	indigo_copy_value(property->label, label ? label : "");
	property->type = INDIGO_LIGHT_VECTOR;
	property->perm = INDIGO_RO_PERM;
	property->state = state;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	property->allocated_count = allocated_count;
	return property;
}

indigo_property *indigo_init_blob_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
	return indigo_init_blob_property_p(property, device, name, group, label, state, INDIGO_RO_PERM, count);
}

indigo_property *indigo_init_blob_property_p(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
	assert(device != NULL);
	assert(name != NULL);
	if (perm == INDIGO_RW_PERM) {
		perm = INDIGO_RO_PERM;
	}
	int size = sizeof(indigo_property) + count * sizeof(indigo_item);
	int allocated_count = count;
	if (property == NULL) {
		property = indigo_safe_malloc(size);
	} else {
		property = indigo_resize_property(property, count);
		allocated_count = property->allocated_count;
	}
	memset(property, 0, size);
	indigo_copy_name(property->device, device);
	indigo_copy_name(property->name, name);
	indigo_copy_name(property->group, group ? group : "");
	indigo_copy_value(property->label, label ? label : "");
	property->type = INDIGO_BLOB_VECTOR;
	property->perm = perm;
	property->state = state;
	property->version = INDIGO_VERSION_CURRENT;
	property->count = count;
	property->allocated_count = allocated_count;
	return property;
}

indigo_property *indigo_resize_property(indigo_property *property, int count) {
	assert(property != NULL);
	if (property->count == count)
		return property;
	if (count > property->allocated_count) {
		property = indigo_safe_realloc(property, sizeof(indigo_property) + count * sizeof(indigo_item));
		property->allocated_count = count;
	}
	assert(property != NULL);
	if (count > property->count)
		memset(property->items+property->count, 0, (count - property->count) * sizeof(indigo_item));
	property->count = count;
	return property;
}

indigo_property *indigo_copy_property(indigo_property *copy, indigo_property *property) {
	if (copy == NULL) {
		copy = indigo_safe_malloc(sizeof(indigo_property) + property->allocated_count * sizeof(indigo_item));
	} else {
		copy = indigo_resize_property(copy, property->count);
	}
	memcpy(copy, property, sizeof(indigo_property) + property->count * sizeof(indigo_item));
	if (copy->type == INDIGO_TEXT_VECTOR) {
		for (int k = 0; k < copy->count; k++) {
			indigo_item *item = copy->items + k;
			if (item->text.long_value) {
				item->text.long_value = NULL;
				indigo_set_text_item_value(item, property->items[k].text.long_value);
			}
		}
	}
	return copy;
}

static void release_dependencies(indigo_property *property) {
	if (property->type == INDIGO_BLOB_VECTOR) {
		pthread_mutex_lock(&blob_mutex);
		for (int i = 0; i < property->allocated_count; i++) {
			indigo_item *item = property->items + i;
			for (int j = 0; j < MAX_BLOBS; j++) {
				indigo_blob_entry *entry = blobs[j];
				if (entry && entry->item == item) {
					pthread_mutex_lock(&entry->mutext);
					blobs[j] = NULL;
					indigo_safe_free(entry->content);
					pthread_mutex_unlock(&entry->mutext);
					pthread_mutex_destroy(&entry->mutext);
					indigo_safe_free(entry);
					break;
				}
			}
			if (property->perm == INDIGO_WO_PERM) {
				indigo_safe_free(item->blob.value);
			}
		}
		pthread_mutex_unlock(&blob_mutex);
	} else if (property->type == INDIGO_TEXT_VECTOR) {
		for (int i = 0; i < property->allocated_count; i++) {
			indigo_safe_free(property->items[i].text.long_value);
		}
	}
}

indigo_property *indigo_clear_property(indigo_property *property) {
	release_dependencies(property);
	int allocated_count = property->allocated_count;
	memset(property, 0, sizeof(indigo_property) + allocated_count * sizeof(indigo_item));
	property->allocated_count = allocated_count;
	return property;
}


void indigo_release_property(indigo_property *property) {
	if (property == NULL) {
		return;
	}
	release_dependencies(property);
	indigo_safe_free(property);
}

indigo_blob_entry *indigo_validate_blob(indigo_item *item) {
	for (int j = 0; j < MAX_BLOBS; j++) {
		indigo_blob_entry *entry = blobs[j];
		if (entry && entry->item == item)
			return entry;
	}
	return NULL;
}

indigo_blob_entry *indigo_find_blob(indigo_property *other_property, indigo_item *other_item) {
	assert(other_property != NULL);
	assert(other_item != NULL);
	for (int j = 0; j < MAX_BLOBS; j++) {
		indigo_blob_entry *entry = blobs[j];
		if (entry) {
			indigo_property *property = entry->property;
			indigo_item *item = entry->item;
			if (!strncmp(property->device, other_property->device, INDIGO_NAME_SIZE) && !strncmp(property->name, other_property->name, INDIGO_NAME_SIZE) && !strncmp(item->name, other_item->name, INDIGO_NAME_SIZE))
				return entry;
		}
	}
	return NULL;
}

void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *format, ...) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	indigo_copy_name(item->name, name);
	indigo_copy_value(item->label, label ? label : "");
	va_list args;
	va_start(args, format);
	vsnprintf(item->text.value, INDIGO_VALUE_SIZE, format, args);
	va_end(args);
}

void indigo_init_text_item_raw(indigo_item *item, const char *name, const char *label, const char *value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	indigo_copy_name(item->name, name);
	indigo_copy_value(item->label, label ? label : "");
	indigo_set_text_item_value(item, value);
}

void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	indigo_copy_name(item->name, name);
	indigo_copy_value(item->label, label ? label : "");
	indigo_copy_value(item->number.format, "%g");
	item->number.min = min;
	item->number.max = max;
	item->number.step = step;
	item->number.target = item->number.value = value;
}

void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	indigo_copy_name(item->name, name);
	indigo_copy_value(item->label, label ? label : "");
	item->sw.value = value;
}

void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	indigo_copy_name(item->name, name);
	indigo_copy_value(item->label, label ? label : "");
	item->light.value = value;
}

void indigo_init_blob_item(indigo_item *item, const char *name, const char *label) {
	assert(item != NULL);
	assert(name != NULL);
	memset(item, 0, sizeof(indigo_item));
	indigo_copy_name(item->name, name);
	indigo_copy_value(item->label, label ? label : "");
}

void *indigo_alloc_blob_buffer(long size) {
	size += 2880;
	int mod2880 = size % 2880;
	if (mod2880) {
		return indigo_safe_malloc(size + 2880 - mod2880);
	}
	return indigo_safe_malloc(size);
}

bool indigo_download_blob(char *url, void **value, long *size, char *format) {
	char *host = indigo_safe_malloc(BUFFER_SIZE);
	int port = 80;
	char *file = indigo_safe_malloc(BUFFER_SIZE);
	char *request = indigo_safe_malloc(BUFFER_SIZE);
	char *http_line = indigo_safe_malloc(BUFFER_SIZE);
	char *http_response = indigo_safe_malloc(BUFFER_SIZE);
	long content_len = 0;
	long uncompressed_content_len = 0;
	int http_result = 0;
	char *image_type;
	int socket = -1;
	int res = false;
	sscanf(url, "http://%255[^:]:%5d/%256[^\n]", host, &port, file);
	socket = indigo_open_tcp(host, port);
	if (socket < 0)
		goto clean_return;
	INDIGO_TRACE(indigo_trace("%d <- // open for '%s:%d'", socket, host, port));
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	snprintf(request, BUFFER_SIZE, "GET /%s HTTP/1.1\r\nAccept-Encoding: gzip\r\n\r\n", file);
#else
	snprintf(request, BUFFER_SIZE, "GET /%s HTTP/1.1\r\n\r\n", file);
#endif
	INDIGO_TRACE(indigo_trace("%d <- %s", socket, request));
	res = indigo_write(socket, request, strlen(request));
	if (res == false)
		goto clean_return;
	res = indigo_read_line(socket, http_line, BUFFER_SIZE);
	if (res < 0) {
		res = false;
		goto clean_return;
	}
	INDIGO_TRACE(indigo_trace("%d -> %s", socket, http_line));
	int count = sscanf(http_line, "HTTP/1.1 %d %255[^\n]", &http_result, http_response);
	if ((count != 2) || (http_result != 200)) {
		goto clean_return;
	}
	bool use_gzip = false;
	/* On Raspberry Pi blob compression may take longer. Make sure we do not timeout prematurely */
	struct timeval timeout;
	timeout.tv_sec = 15;
	timeout.tv_usec = 0;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	do {
		res = indigo_read_line(socket, http_line, BUFFER_SIZE);
		if (res < 0) {
			res = false;
			goto clean_return;
		}
		INDIGO_TRACE(indigo_trace("%d -> %s", socket, http_line));
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (!strncasecmp(http_line, "Content-Encoding: gzip", 22)) {
			use_gzip = true;
			continue;
		}
#endif
		if (sscanf(http_line, "Content-Length: %20ld[^\n]", &content_len) == 1)
			continue;
		if (sscanf(http_line, "X-Uncompressed-Content-Length: %20ld[^\n]", &uncompressed_content_len) == 1)
			continue;
	} while (http_line[0] != '\0');
	if (content_len) {
		image_type = strrchr(file, '.');
		if (image_type && format != NULL)
			indigo_copy_name(format, image_type);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (use_gzip) {
			*size = uncompressed_content_len;
			*value = indigo_safe_realloc(*value, *size);
			char *compressed_buffer = indigo_safe_malloc(content_len);
			res = (indigo_read(socket, compressed_buffer, content_len) >= 0) ? true : false;
			if (res) {
				unsigned out_size = (unsigned)uncompressed_content_len;
				indigo_decompress(compressed_buffer, (unsigned)content_len, *value, &out_size);
			}
			free(compressed_buffer);
		} else {
			*size = content_len;
			*value = indigo_safe_realloc(*value, *size);
			INDIGO_TRACE(indigo_trace("%d -> // %d bytes", socket, *size));
			res = (indigo_read(socket, *value, *size) >= 0) ? true : false;
		}
#else
		*size = content_len;
		*value = indigo_safe_realloc(*value, *size);
		INDIGO_TRACE(indigo_trace("%d -> // %d bytes", socket, *size));
		res = (indigo_read(socket, *value, *size) >= 0) ? true : false;
#endif
	} else {
		res = false;
	}

clean_return:
	if (!res || socket < 0)
		INDIGO_TRACE(indigo_trace("%d -> // %s", socket, strerror(errno)));
	if (socket >= 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		shutdown(socket, SHUT_RDWR);
		close(socket);
#endif
#if defined(INDIGO_WINDOWS)
		shutdown(socket, SD_BOTH);
		closesocket(socket);
#endif
	}
	indigo_safe_free(host);
	indigo_safe_free(file);
	indigo_safe_free(request);
	indigo_safe_free(http_line);
	indigo_safe_free(http_response);
	return res;
}

bool indigo_populate_http_blob_item(indigo_item *blob_item) {
	if ((blob_item->blob.url[0] == '\0') || strcmp(blob_item->name, CCD_IMAGE_ITEM_NAME)) {
		indigo_error("%s: url == \"\" or item != \"%s\"", __FUNCTION__, CCD_IMAGE_ITEM_NAME);
		return false;
	}
	return indigo_download_blob(blob_item->blob.url, &blob_item->blob.value, &blob_item->blob.size, blob_item->blob.format);
}

bool indigo_upload_http_blob_item(indigo_item *blob_item) {
	char *host = indigo_safe_malloc(BUFFER_SIZE);
	int port = 80;
	char *file = indigo_safe_malloc(BUFFER_SIZE);
	char *request = indigo_safe_malloc(BUFFER_SIZE);
	char *http_line = indigo_safe_malloc(BUFFER_SIZE);
	char *http_response = indigo_safe_malloc(BUFFER_SIZE);
	int http_result = 0;
	int socket = -1;
	int res = false;
	if ((blob_item->blob.url[0] == '\0') || strcmp(blob_item->name, CCD_IMAGE_ITEM_NAME)) {
		indigo_error("%s(): url == \"\" or item != \"%s\"", __FUNCTION__, CCD_IMAGE_ITEM_NAME);
		goto clean_return;
	}
	sscanf(blob_item->blob.url, "http://%255[^:]:%5d/%256[^\n]", host, &port, file);
	socket = indigo_open_tcp(host, port);
	if (socket < 0)
		goto clean_return;
	INDIGO_TRACE(indigo_trace("%d <- // open for '%s:%d'", socket, host, port));
//#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#if false
	unsigned char *out_buffer = indigo_safe_malloc(blob_item->blob.size);
	unsigned int out_size = blob_item->blob.size;
	if (out_buffer == NULL)
		goto clean_return;
	indigo_compress("image", blob_item->blob.value, (unsigned int)blob_item->blob.size, out_buffer, &out_size);
	snprintf(request, BUFFER_SIZE, "PUT /%s HTTP/1.1\r\nContent-Encoding: gzip\r\nContent-Length: %d\r\nX-Uncompressed-Content-Length: %ld\r\n\r\n", file, out_size, blob_item->blob.size);
	INDIGO_TRACE(indigo_trace("%d <- %s", request));
	res = indigo_write(socket, request, strlen(request));
	if (res == false)
		goto clean_return;
	INDIGO_TRACE(indigo_trace("%d <- // %d bytes", socket, blob_item->blob.size));
	res = indigo_write(socket, (const char *)out_buffer, out_size);
	indigo_safe_free(out_buffer);
	if (res == false)
		goto clean_return;
#else
	snprintf(request, BUFFER_SIZE, "PUT /%s HTTP/1.1\r\nContent-Length: %ld\r\n\r\n", file, blob_item->blob.size);
	INDIGO_TRACE(indigo_trace("%d <- %s", socket, request));
	res = indigo_write(socket, request, strlen(request));
	if (res == false)
		goto clean_return;
	INDIGO_TRACE(indigo_trace("%d <- // %d bytes", socket, blob_item->blob.size));
	res = indigo_write(socket, blob_item->blob.value, blob_item->blob.size);
	if (res == false)
		goto clean_return;
#endif
	res = indigo_read_line(socket, http_line, BUFFER_SIZE);
	INDIGO_TRACE(indigo_trace("%d -> %s", socket, http_line));
	if (res < 0) {
		res = false;
		goto clean_return;
	}
	int count = sscanf(http_line, "HTTP/1.1 %d %255[^\n]", &http_result, http_response);
	if ((count != 2) || (http_result != 200)) {
		goto clean_return;
	}
	do {
		res = indigo_read_line(socket, http_line, BUFFER_SIZE);
		INDIGO_TRACE(indigo_trace("%d -> %s", socket, http_line));
		if (res < 0) {
			res = false;
			goto clean_return;
		}
	} while (http_line[0] != '\0');
clean_return:
	if (!res || socket < 0)
		INDIGO_TRACE(indigo_trace("%d -> // %s", socket, strerror(errno)));
	if (socket >= 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		shutdown(socket, SHUT_RDWR);
		close(socket);
#endif
#if defined(INDIGO_WINDOWS)
		shutdown(socket, SD_BOTH);
		closesocket(socket);
#endif
	}
	indigo_safe_free(host);
	indigo_safe_free(file);
	indigo_safe_free(request);
	indigo_safe_free(http_line);
	indigo_safe_free(http_response);
	return res;
}

static bool indigo_get_hint(char *hints, const char *key, char *value) {
	bool is_key = true;
	bool kv_more = true;
	bool kv_end = false;
	bool is_quoted = false;
	int i = 0;
	char *c = hints;
	char ckey[INDIGO_NAME_SIZE];
	char cval[INDIGO_VALUE_SIZE];
	INDIGO_DEBUG(indigo_debug("%s(): hints = { %s\n }, looking for key '%s'", __FUNCTION__, hints, key));
	while (kv_more) {
		switch (*c) {
			case '\0':
				kv_more = false;
				kv_end = true;
				if (is_key) {
					ckey[i] = '\0';
				} else {
					cval[i] = '\0';
				}
				break;
			case ':':
				is_key = false;
				ckey[i] = '\0';
				i = 0;
				break;
			case ';':
				is_key = true;
				kv_end = true;
				cval[i] = '\0';
				i = 0;
				break;
			case '\\':
				if (*(c++) == '\0') {
					continue;
				} else {
					if (is_key) {
						ckey[i++] = *c;
					} else {
						cval[i++] = *c;
					}
				}
				break;
			case '\"':
				is_quoted = !is_quoted;
				break;
			case ' ':
				if (is_quoted && !is_key) {
					cval[i++] = *c;
				}
				break;
			default:
				if (is_key) {
					ckey[i++] = *c;
				} else {
					cval[i++] = *c;
				}
		}
		c++;
		//printf("kw_end = %d, is_key = %d, is_quoted = %d\n", kv_end, is_key, is_quoted);
		if (kv_end) {
			if (!strncmp(ckey, key, INDIGO_NAME_SIZE)) {
				INDIGO_DEBUG(indigo_debug("%s(): hint found -> %s = %s\n", __FUNCTION__, ckey, cval));
				strncpy(value, cval, INDIGO_VALUE_SIZE);
				return true;
			}
			i=0;
			kv_end = 0;
			ckey[0] = '\0';
			cval[0] = '\0';
		}
	}
	return false;
}

bool indigo_get_property_hint(indigo_property *property, const char *key, char *value) {
	return indigo_get_hint(property->hints, key, value);
}

bool indigo_get_item_hint(indigo_item *item, const char *key, char *value) {
	return indigo_get_hint(item->hints, key, value);
}

bool indigo_property_match(indigo_property *property, indigo_property *other) {
	if (property == NULL)
		return false;
	return other == NULL || ((other->type == 0 || property->type == other->type) && (*other->device == 0 || !strcmp(property->device, other->device)) && (*other->name == 0 || !strcmp(property->name, other->name)));
}

bool indigo_property_match_defined(indigo_property *property, indigo_property *other) {
	if (property == NULL)
		return false;
	if (!property->defined)
		return false;
	return other == NULL || ((other->type == 0 || property->type == other->type) && (*other->device == 0 || !strcmp(property->device, other->device)) && (*other->name == 0 || !strcmp(property->name, other->name)));
}

bool indigo_property_match_changeable(indigo_property *property, indigo_property *other) {
	if (property == NULL)
		return false;
	if (!property->defined)
		return false;
	if (property->perm == INDIGO_RO_PERM)
		return false;
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
	if (value && property->rule != INDIGO_ANY_OF_MANY_RULE) {
		for (int i = 0; i < property->count; i++) {
			property->items[i].sw.value = false;
		}
	}
	item->sw.value = value;
}

indigo_item *indigo_get_item(indigo_property *property, const char *item_name) {
	assert(property != NULL);
	assert(item_name != NULL);
	for (int i = 0; i < property->count; i++)
		if (!strcmp(property->items[i].name, item_name))
			return property->items + i;
	return NULL;
}

bool indigo_get_switch(indigo_property *property, const char *item_name) {
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
			property->access_token = other->access_token;
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
							if (property_item->text.long_value) {
								free(property_item->text.long_value);
								property_item->text.long_value = NULL;
							}
							indigo_copy_value(property_item->text.value, other_item->text.value);
							property_item->text.length = other_item->text.length;
							if (other_item->text.long_value) {
								property_item->text.long_value = indigo_safe_malloc(property_item->text.length);
								memcpy(property_item->text.long_value, other_item->text.long_value, other_item->text.length);
							}
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
						case INDIGO_BLOB_VECTOR:
							property_item->blob.value = indigo_safe_realloc_copy(property_item->blob.value, property_item->blob.size = other_item->blob.size, other_item->blob.value);
							indigo_copy_name(property_item->blob.format, other_item->blob.format);
							break;
						default:
							break;
						}
						break;
					}
				}
			}
		}
	}
}

void indigo_property_copy_targets(indigo_property *property, indigo_property *other, bool with_state) {
	assert(property != NULL);
	assert(other != NULL);
	assert(property->type == INDIGO_NUMBER_VECTOR);
	if (property->perm != INDIGO_RO_PERM) {
		if (property->type == other->type) {
			if (with_state)
				property->state = other->state;
			for (int i = 0; i < other->count; i++) {
				indigo_item *other_item = &other->items[i];
				for (int j = 0; j < property->count; j++) {
					indigo_item *property_item = &property->items[j];
					if (!strcmp(property_item->name, other_item->name)) {
						property_item->number.target = other_item->number.value;
						if (property_item->number.target < property_item->number.min)
							property_item->number.target = property_item->number.min;
						if (property_item->number.target > property_item->number.max)
							property_item->number.target = property_item->number.max;
					}
				}
			}
		}
	}
}

static int item_comparator(const void *item_1, const void *item_2) {
	return strcasecmp(((indigo_item *)item_1)->label, ((indigo_item *)item_2)->label);
}

void indigo_property_sort_items(indigo_property *property, int first) {
	if (property->count - first > 1) {
		qsort(property->items + first, property->count - first, sizeof(indigo_item), item_comparator);
	}
}

char *indigo_get_text_item_value(indigo_item *item) {
	if (item) {
		return item->text.long_value ? item->text.long_value : item->text.value;
	}
	return NULL;
}

void indigo_set_text_item_value(indigo_item *item, const char *value) {
	if (item->text.long_value) {
		free(item->text.long_value);
		item->text.long_value = NULL;
	}
	long length = strlen(value);
	indigo_copy_value(item->text.value, value);
	item->text.length = length + 1;
	if (length >= INDIGO_VALUE_SIZE) {
		item->text.long_value = indigo_safe_malloc(item->text.length);
		strncpy(item->text.long_value, value, length);
		item->text.long_value[length] = 0;
	}
}


indigo_result indigo_change_text_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, const char **values) {
	indigo_property *property = indigo_init_text_property(NULL, device, name, NULL, NULL, 0, 0, count);
	property->access_token = token;
	for (int i = 0; i < count; i++)
		indigo_init_text_item_raw(property->items + i, items[i], NULL, values[i]);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_text_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const char **values) {
	return indigo_change_text_property_with_token(client, device, indigo_get_device_or_master_token(device), name, count, items, values);
}

indigo_result indigo_change_text_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const char *format, ...) {
	indigo_property *property = indigo_init_text_property(NULL, device, name, NULL, NULL, 0, 0, 1);
	property->access_token = token;
	char tmp[INDIGO_VALUE_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(tmp, INDIGO_VALUE_SIZE, format, args);
	va_end(args);
	indigo_init_text_item(property->items, item, NULL, tmp);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_text_property_1_with_token_raw(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const char *value) {
	indigo_property *property = indigo_init_text_property(NULL, device, name, NULL, NULL, 0, 0, 1);
	property->access_token = token;
	indigo_init_text_item_raw(property->items, item, NULL, value);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_text_property_1(indigo_client *client, const char *device, const char *name, const char *item, const char *format, ...) {
	char tmp[INDIGO_VALUE_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(tmp, INDIGO_VALUE_SIZE, format, args);
	va_end(args);
	return indigo_change_text_property_1_with_token(client, device, indigo_get_device_or_master_token(device), name, item, tmp);
}

indigo_result indigo_change_text_property_1_raw(indigo_client *client, const char *device, const char *name, const char *item, const char *value) {
	return indigo_change_text_property_1_with_token_raw(client, device, indigo_get_device_or_master_token(device), name, item, value);
}

indigo_result indigo_change_number_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, const double *values) {
	indigo_property *property = indigo_init_number_property(NULL, device, name, NULL, NULL, 0, 0, count);
	property->access_token = token;
	for (int i = 0; i < count; i++)
		indigo_init_number_item(property->items + i, items[i], NULL, 0, 0, 0, values[i]);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_number_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const double *values) {
	return indigo_change_number_property_with_token(client, device, indigo_get_device_or_master_token(device), name, count, items, values);
}

indigo_result indigo_change_number_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const double value) {
	indigo_property *property = indigo_init_number_property(NULL, device, name, NULL, NULL, 0, 0, 1);
	property->access_token = token;
	indigo_init_number_item(property->items, item, NULL, 0, 0, 0, value);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_number_property_1(indigo_client *client, const char *device, const char *name, const char *item, const double value) {
	return indigo_change_number_property_1_with_token(client, device, indigo_get_device_or_master_token(device), name, item, value);
}

indigo_result indigo_change_blob_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, void **values, const long *sizes, const char **formats, const char **urls) {
	indigo_property *property = indigo_init_blob_property_p(NULL, device, name, NULL, NULL, 0, INDIGO_WO_PERM, count);
	property->access_token = token;
	for (int i = 0; i < count; i++) {
		indigo_item *item = property->items + i;
		indigo_init_blob_item(item, items[i], NULL);
		item->blob.value = indigo_safe_malloc_copy(item->blob.size = sizes[i], values[i]);
		indigo_copy_name(item->blob.format, formats[i]);
		indigo_copy_value(item->blob.url, urls[i]);
	}
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_blob_property(indigo_client *client, const char *device, const char *name, int count, const char **items, void **values, const long *sizes, const char **formats, const char **urls) {
	return indigo_change_blob_property_with_token(client, device, indigo_get_device_token(device), name, count, items, values, sizes, formats, urls);
}

indigo_result indigo_change_blob_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, void *value, const long size, const char *format, const char *url) {
	indigo_property *property = indigo_init_blob_property_p(NULL, device, name, NULL, NULL, 0, INDIGO_WO_PERM, 1);
	property->access_token = token;
	indigo_init_blob_item(property->items, item, NULL);
	property->items->blob.value = indigo_safe_malloc_copy(property->items->blob.size = size, value);
	indigo_copy_name(property->items->blob.format, format);
	indigo_copy_value(property->items->blob.url, url);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_blob_property_1(indigo_client *client, const char *device, const char *name, const char *item, void *value, const long size, const char *format, const char *url) {
	return indigo_change_blob_property_1_with_token(client, device, indigo_get_device_or_master_token(device), name, item, value, size, format, url);
}

indigo_result indigo_change_switch_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, const bool *values) {
	indigo_property *property = indigo_init_switch_property(NULL, device, name, NULL, NULL, 0, 0, 0, count);
	property->access_token = token;
	for (int i = 0; i < count; i++)
		indigo_init_switch_item(property->items + i, items[i], NULL, values[i]);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_switch_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const bool *values) {
	return indigo_change_switch_property_with_token(client, device, indigo_get_device_or_master_token(device), name, count, items, values);
}

indigo_result indigo_change_switch_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const bool value) {
	indigo_property *property = indigo_init_switch_property(NULL, device, name, NULL, NULL, 0, 0, 0, 1);
	property->access_token = token;
	indigo_init_switch_item(property->items, item, NULL, value);
	indigo_result result = indigo_change_property(client, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_change_switch_property_1(indigo_client *client, const char *device, const char *name, const char *item, const bool value) {
	return indigo_change_switch_property_1_with_token(client, device, indigo_get_device_or_master_token(device), name, item, value);
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

int indigo_query_slave_devices(indigo_device *master, indigo_device **slaves, int max) {
	if (indigo_use_strict_locking) {
  pthread_mutex_lock(&device_mutex);
}
	int count = 0;
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device && device != master && device->master_device == master) {
			slaves[count] = device;
			if (count++ >= max) {
  break;
}
		}
	}
	if (indigo_use_strict_locking) {
  pthread_mutex_unlock(&device_mutex);
}
	return count;
}

void indigo_trim_local_service(char *device_name) {
	while (*device_name) {
		if (strncmp(device_name, " @ ", 3) == 0) {
			if (strcmp(device_name + 3, indigo_local_service_name) == 0) {
				*device_name = 0;
			}
			return;
		}
		device_name++;
	}
}

typedef struct {
	void (*handler)(indigo_device *device, indigo_client *client, indigo_property *property);
	indigo_device *device;
	indigo_client *client;
	indigo_property *property;
} property_handler_data_t;

static void _indigo_handle_property_async(property_handler_data_t *phd) {
	assert(phd != NULL);
	phd->handler(phd->device, phd->client, phd->property);
	free(phd);
}

bool indigo_handle_property_async(
	void (*handler)(indigo_device *device, indigo_client *client, indigo_property *property),
	indigo_device *device,
	indigo_client *client,
	indigo_property *property
) {
	property_handler_data_t *phd = indigo_safe_malloc(sizeof(property_handler_data_t));

	phd->handler = handler;
	phd->device = device;
	phd->client = client;
	phd->property = property;

	return INDIGO_ASYNC(_indigo_handle_property_async, phd);
}

bool indigo_async(void *fun(void *data), void *data) {
	pthread_t async_thread;
	if (pthread_create(&async_thread, NULL, fun, data) == 0) {
		pthread_detach(async_thread);
		return true;
	}
	return false;
}

double indigo_stod(char *string) {
	char copy[128];
	strncpy(copy, string, 128);
	string = copy;
	double value = 0;
	char *separator = strpbrk(string, ":*'\xdf");
	if (separator == NULL) {
		value = indigo_atod(string);
	} else {
		*separator++ = 0;
		value = indigo_atod(string);
		separator = strpbrk(string = separator, ":*'");
		if (separator == NULL) {
			if (value < 0)
				value -= indigo_atod(string)/60.0;
			else
				value += indigo_atod(string)/60.0;
		} else {
			*separator++ = 0;
			/* if negative including -0.0f */
			if (signbit(value))
				value -= indigo_atod(string)/60.0 + indigo_atod(separator)/3600.0;
			else
				value += indigo_atod(string)/60.0 + indigo_atod(separator)/3600.0;
		}
	}
	return value;
}

static void fix_dms(double *d, double *m, double *s) {
	if (s && *s >= 60) {
		*s = 0;
		(*m)++;
	}
	if (*m >= 60) {
		*m = 0;
		(*d)++;
	}
}

char* indigo_dtos(double value, const char *format) { // circular use of 4 static buffers!
	double d = fabs(value);
	double m = 60.0 * (d - floor(d));
	double s = 60.0 * (m - floor(m));
	if (format == NULL) {
		format = "%d:%02d:%05.2f";
	}
	char buffer[127], signature[16];
	// compute format signature
	const char *fp = format;
	char *sp = signature;
	while (*fp && sp - signature < sizeof(signature) - 1) {
		if (*fp == '%') {
			fp++;
			while (*fp == '0' || *fp == '-' || *fp == '+') {
				fp++;
			}
			while (isdigit((unsigned char)*fp)) {
				fp++;
			}
			if (*fp == '.') {
				fp++;
				while (isdigit((unsigned char)*fp)) {
					*(sp++) = *(fp++);
				}
			}
			if (*fp) {
				*(sp++) = *(fp++);
			}
		} else {
			fp++;
		}
	}
	*sp = 0;
	// select circular buffer
	static char string_1[128], string_2[128], string_3[128], string_4[128];
	static char *string = string_4;
	if (string == string_1) {
		string = string_2;
	} else if (string == string_2) {
		string = string_3;
	} else if (string == string_3) {
		string = string_4;
	} else if (string == string_4) {
		string = string_1;
	}
	// format number
	if (!strcmp(signature, "d")) {
			snprintf(buffer, sizeof(buffer), format, (int)d);
	} else if (!strcmp(signature, "dd")) {
			m = round(m);
			fix_dms(&d, &m, NULL);
			snprintf(buffer, sizeof(buffer), format, (int)d, (int)m);
	} else if (!strcmp(signature, "ddd")) {
		s = round(s);
		fix_dms(&d, &m, &s);
		snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, (int)s);
	} else if (!strcmp(signature, "d0f")) {
		m = round(m);
		fix_dms(&d, &m, NULL);
		snprintf(buffer, sizeof(buffer), format, (int)d, m);
	} else if (!strcmp(signature, "d1f")) {
		m = (round(m * 10.0)) / 10.0;
		fix_dms(&d, &m, NULL);
		snprintf(buffer, sizeof(buffer), format, (int)d, m);
	} else if (!strcmp(signature, "d2f")) {
		m = (round(m * 100.0)) / 100.0;
		fix_dms(&d, &m, NULL);
		snprintf(buffer, sizeof(buffer), format, (int)d, m);
	} else if (!strcmp(signature, "d3f")) {
		m = (round(m * 1000.0)) / 1000.0;
		fix_dms(&d, &m, NULL);
		snprintf(buffer, sizeof(buffer), format, (int)d, m);
	} else if (!strcmp(signature, "d4f")) {
		m = (round(m * 10000.0)) / 10000.0;
		fix_dms(&d, &m, NULL);
		snprintf(buffer, sizeof(buffer), format, (int)d, m);
	} else if (!strcmp(signature, "dd0f")) {
		s = round(s);
		fix_dms(&d, &m, &s);
		snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, s);
	} else if (!strcmp(signature, "dd1f")) {
		s = (round(s * 10.0)) / 10.0;
		fix_dms(&d, &m, &s);
		snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, s);
	} else if (!strcmp(signature, "dd2f")) {
		s = (round(s * 100.0)) / 100.0;
		fix_dms(&d, &m, &s);
		snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, s);
	} else if (!strcmp(signature, "dd3f")) {
		s = (round(s * 1000.0)) / 1000.0;
		fix_dms(&d, &m, &s);
		snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, s);
	} else if (!strcmp(signature, "dd4f")) {
		s = (round(s * 10000.0)) / 10000.0;
		fix_dms(&d, &m, &s);
		snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, s);
	} else {
		snprintf(buffer, sizeof(buffer), format, d);
	}
	// fix sign
	if (value < 0) {
		if (buffer[0] == '+') {
			buffer[0] = '-';
			snprintf(string, 128, "%s", buffer);
		} else {
			snprintf(string, 128, "-%s", buffer);
		}
	} else {
		snprintf(string, 128, "%s", buffer);
	}
	//printf("%18s -> %s\n", format, string);
	return string;
}

void indigo_usleep(unsigned int delay) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	struct timespec remaining;
	struct timespec requested = {
		(int)(delay / ONE_SECOND_DELAY),
		(delay % ONE_SECOND_DELAY) * 1000
	};
	int ret = nanosleep(&requested, &remaining);
	if (ret < 0) {
		indigo_error("%s(): nanosleep() failed with error: %s", __FUNCTION__, strerror(errno));
	}
#endif
#if defined(INDIGO_WINDOWS)
	unsigned int s = delay / 1000;
	Sleep(s);
#endif
}

double indigo_atod(const char *str) {
	double value = 0;
	int sign = 1;
	while (*str && isspace(*str))
		str++;
	if (*str == '+') {
  str++;
}
	else if (*str == '-') {
		sign = -1;
		str++;
	}
	for (value = 0; *str && isdigit(*str); ++str)
		value = value * 10 + (*str - '0');
	if (*str == '.' || *str == ',') {
		++str;
		double dec;
		for (dec = 0.1; *str && isdigit(*str); ++str, dec /= 10)
			value += dec * (*str - '0');
	}
	if (*str == 'E' || *str == 'e') {
		if (value == 0)
			value = 1;
		int ex = atoi(++str);
		value *= pow(10, ex);
	}
	return sign * value;
}

char *indigo_dtoa(double value, char *str) {
	sprintf(str, "%.10g", value);
	indigo_fix_locale(str);
	return str;
}

#define BUFFER_COUNT 16

static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *large_buffers[BUFFER_COUNT];

static void free_large_buffers(void) {
	for (int i = 0; i < BUFFER_COUNT; i++) {
		if (large_buffers[i]) {
  free(large_buffers[i]);
}
	}
}

void *indigo_alloc_large_buffer(void) {
	pthread_mutex_lock(&buffer_mutex);
	static bool register_atexit = true;
	if (register_atexit) {
		register_atexit = false;
		atexit(free_large_buffers);
	}
	for (int i = 0; i < BUFFER_COUNT; i++) {
		if (large_buffers[i]) {
			void *large_buffer = large_buffers[i];
			large_buffers[i] = NULL;
			pthread_mutex_unlock(&buffer_mutex);
			return large_buffer;
		}
	}
	pthread_mutex_unlock(&buffer_mutex);
	return indigo_safe_malloc(INDIGO_BUFFER_SIZE);
}

void indigo_free_large_buffer(void *large_buffer) {
	pthread_mutex_lock(&buffer_mutex);
	for (int i = 0; i < BUFFER_COUNT; i++) {
		if (large_buffers[i] == NULL) {
			large_buffers[i] = large_buffer;
			pthread_mutex_unlock(&buffer_mutex);
			return;
		}
	}
	pthread_mutex_unlock(&buffer_mutex);
	free(large_buffer);
}

double indigo_pixel_scale(double focal_length_cm, double pixel_size_um) {
	if (focal_length_cm > 0) {
		return 20.6265 * pixel_size_um / focal_length_cm;
	} else {
		return 0;
	}
}

bool indigo_device_name_exists(const char *name) {
	pthread_mutex_lock(&device_mutex);
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) {
  continue;
}
		if (!strncmp(device->name, name, INDIGO_NAME_SIZE)) {
			pthread_mutex_unlock(&device_mutex);
			return true;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return false;
}

bool indigo_make_name_unique(char *name, const char *format, ...) {
	bool used_suffix[MAX_DEVICES - 1] = { false };
	bool is_duplicate = false;
	pthread_mutex_lock(&device_mutex);
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) {
  continue;
}
		if (!strncmp(device->name, name, INDIGO_NAME_SIZE)) {
			is_duplicate = true;
			continue;
		}
		if (format == NULL) {
			// if no default value, reuse gap in sequence
			char *separator = strstr(device->name, " #");
			if (separator) {
				if (!strncmp(device->name, name, separator - device->name)) {
					int suffix = atoi(separator + 2);
					if (suffix > 0 && suffix < MAX_DEVICES - 1)
						used_suffix[suffix - 1] = true;
					continue;
				}
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
	if (!is_duplicate)
		return true;
	char tmp[64];
	if (format == NULL) {
		for (int i = 1; i < MAX_DEVICES; i++) {
			if (!used_suffix[i - 1]) {
				snprintf(tmp, sizeof(tmp), " #%d", i);
				strcat(name, tmp);
				return true;
			}
		}
		indigo_error("Can't make unique name for device %s", name);
		return false;
	}
	va_list args;
	va_start(args, format);
	vsnprintf(tmp, sizeof(tmp), format, args);
	va_end(args);
	strcat(name, " #");
	strcat(name, tmp);
	return true;
}
