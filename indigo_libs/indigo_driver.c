// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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

// Configuration serialization refactored by OpenAI Codex (2026).

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Driver base
 \file indigo_driver.c
 */

#if defined(INDIGO_LINUX) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(INDIGO_MACOS)
#include <dlfcn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#elif defined(INDIGO_LINUX)
#include <dlfcn.h>
#include <unistd.h>
#include <libudev.h>
#endif

#if defined(INDIGO_MACOS)
#define DEFAULT_TTY "/dev/cu.usbserial"
#elif defined(INDIGO_FREEBSD)
#define DEFAULT_TTY "/dev/ttyu1"
#elif defined(INDIGO_LINUX)
#define DEFAULT_TTY "/dev/ttyUSB0"
#else
#define DEFAULT_TTY "/dev/tty"
#endif

#include <indigo/indigo_driver.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#define MAX_SLAVE_DEVICES 10

#ifdef _MSC_VER
#pragma warning(disable: 6011)
#endif

pthread_mutex_t indigo_device_enumeration_mutex = PTHREAD_MUTEX_INITIALIZER;

indigo_result indigo_try_global_lock(indigo_device *device) {
	// Deprecated no-op kept for source compatibility with out-of-tree drivers.
	return INDIGO_OK;
}

indigo_result indigo_global_unlock(indigo_device *device) {
	// Deprecated no-op kept for source compatibility with out-of-tree drivers.
	return INDIGO_OK;
}

#define MAX_DEVICE_PORTS	20

extern int indigo_server_tcp_port;
extern bool indigo_is_ephemeral_port;

// library internal, defined in indigo_bus.c
extern void indigo_lock_bus(void);
extern void indigo_unlock_bus(void);

// -------------------------------------------------------------------------------- shared serial port list

// The serial ports are enumerated once per process and the list is shared by all devices showing DEVICE_PORTS.
// A USB hot-plug event or a refresh request rescans it and, if it changed, DEVICE_PORTS of every such device is
// rebuilt from the new list. The list and the devices are guarded by the bus lock, the scan runs without it.

#define SERIAL_PORTS_SETTLE_TIME	1.0
#define SERIAL_PORTS_RETRY_TIME		2.0

typedef struct {
	char name[INDIGO_NAME_SIZE];
	char label[INDIGO_VALUE_SIZE];
} serial_port;

typedef struct {
	indigo_serial_info usb[MAX_DEVICE_PORTS];
	int usb_count;
	serial_port ports[MAX_DEVICE_PORTS - 1];
	int port_count;
} serial_port_list;

typedef struct {
	char driver[INDIGO_NAME_SIZE];
	serial_port *ports;
	int count;
} additional_ports;

static serial_port_list *serial_ports = NULL;
static indigo_device **serial_port_devices = NULL;
static int serial_port_devices_count = 0;
static int serial_port_devices_size = 0;
static additional_ports *driver_ports = NULL;
static int driver_ports_count = 0;

static void add_serial_port(serial_port_list *list, const char *name, const char *label) {
	if (list->port_count < MAX_DEVICE_PORTS - 1) {
		serial_port *port = list->ports + list->port_count++;
		INDIGO_COPY_NAME(port->name, name);
		INDIGO_COPY_VALUE(port->label, label);
	}
}

#if defined(INDIGO_LINUX)
static void add_serial_link(serial_port_list *list, const char *path, const char *node) {
	for (int i = 0; i < list->port_count; i++) {
		if (!strcmp(list->ports[i].name, path)) {
			return;
		}
	}
	char label[INDIGO_VALUE_SIZE];
	snprintf(label, sizeof(label), "%s (link to %s)", path, node);
	add_serial_port(list, path, label);
}

// Native serial ports are taken from udev and links to serial ports in /dev from udev and from /dev itself, so no
// device node is opened.
static void add_native_serial_ports(serial_port_list *list) {
	char nodes[MAX_DEVICE_PORTS][INDIGO_NAME_SIZE];
	int node_count = 0;
	for (int i = 0; i < list->usb_count && node_count < MAX_DEVICE_PORTS; i++) {
		INDIGO_COPY_NAME(nodes[node_count++], list->usb[i].path);
	}
	struct udev *udev = udev_new();
	if (udev != NULL) {
		struct udev_enumerate *enumerate = udev_enumerate_new(udev);
		udev_enumerate_add_match_subsystem(enumerate, "tty");
		udev_enumerate_scan_devices(enumerate);
		struct udev_list_entry *entry;
		udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumerate)) {
			struct udev_device *dev = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
			if (dev == NULL) {
				continue;
			}
			const char *node = udev_device_get_devnode(dev);
			if (node != NULL) {
				bool listed = false;
				for (int i = 0; i < list->usb_count; i++) {
					if (!strcmp(list->usb[i].path, node)) {
						listed = true;
						break;
					}
				}
				if (!listed) {
					// a UART of the serial core has a "type" attribute, which is 0 (PORT_UNKNOWN) for a port without hardware
					const char *type = udev_device_get_sysattr_value(dev, "type");
					if (type != NULL && atoi(type) > 0) {
						add_serial_port(list, node, node);
						if (node_count < MAX_DEVICE_PORTS) {
							INDIGO_COPY_NAME(nodes[node_count++], node);
						}
						listed = true;
					}
				}
				if (listed) {
					// only links directly in /dev, e.g. /dev/serial0 or one from a user rule, not /dev/serial/by-id
					struct udev_list_entry *link;
					udev_list_entry_foreach(link, udev_device_get_devlinks_list_entry(dev)) {
						const char *path = udev_list_entry_get_name(link);
						if (!strncmp(path, "/dev/", 5) && strchr(path + 5, '/') == NULL) {
							add_serial_link(list, path, node);
						}
					}
				}
			}
			udev_device_unref(dev);
		}
		udev_enumerate_unref(enumerate);
		udev_unref(udev);
	}
	// udev knows only the links it created, links made by hand or by a script, or all links in a container without
	// the udev database, are found in /dev; only symbolic links are resolved, except those into /proc like /dev/stdin,
	// which lead to the file descriptors of this process
	char **names;
	int count = indigo_uni_scandir("/dev", &names, NULL);
	if (count < 0) {
		return;
	}
	for (int i = 0; i < count; i++) {
		char path[PATH_MAX];
		char link[PATH_MAX];
		char target[PATH_MAX];
		struct stat info;
		snprintf(path, sizeof(path), "/dev/%s", names[i]);
		ssize_t length = lstat(path, &info) == 0 && S_ISLNK(info.st_mode) ? readlink(path, link, sizeof(link) - 1) : -1;
		if (length > 0) {
			link[length] = '\0';
		}
		if (length > 0 && strncmp(link, "/proc/", 6) && realpath(path, target) != NULL) {
			for (int j = 0; j < node_count; j++) {
				if (!strcmp(nodes[j], target)) {
					add_serial_link(list, path, nodes[j]);
					break;
				}
			}
		}
		indigo_safe_free(names[i]);
	}
	indigo_safe_free(names);
}
#elif defined(INDIGO_MACOS)
static bool cu_filter(const char *name) {
	return strncmp(name, "cu.", 3) == 0 && strstr(name, "debug") == NULL && strstr(name, "Bluetooth-Incoming-Port") == NULL;
}

static void add_native_serial_ports(serial_port_list *list) {
	char **names;
	int count = indigo_uni_scandir("/dev", &names, cu_filter);
	if (count < 0) {
		return;
	}
	for (int i = 0; i < count; i++) {
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "/dev/%s", names[i]);
		bool listed = false;
		for (int j = 0; j < list->usb_count; j++) {
			if (!strcmp(list->usb[j].path, path)) {
				listed = true;
				break;
			}
		}
		if (!listed) {
			char label[INDIGO_VALUE_SIZE];
			snprintf(label, sizeof(label), "%s (Unknown)", path);
			add_serial_port(list, path, label);
		}
		indigo_safe_free(names[i]);
	}
	indigo_safe_free(names);
}
#else
static void add_native_serial_ports(serial_port_list *list) {
	// Windows lists all COM ports with the USB ones
}
#endif

static serial_port_list *scan_serial_ports(void) {
	serial_port_list *list = indigo_safe_malloc(sizeof(serial_port_list));
	memset(list, 0, sizeof(serial_port_list));
	list->usb_count = indigo_enumerate_usbserial_devices(list->usb, MAX_DEVICE_PORTS);
	if (list->usb_count < 0) {
		list->usb_count = 0;
	}
	for (int i = 0; i < list->usb_count; i++) {
		char label[INDIGO_VALUE_SIZE];
		indigo_usbserial_label(list->usb + i, label);
		add_serial_port(list, list->usb[i].path, label);
	}
	add_native_serial_ports(list);
	return list;
}

static bool same_serial_ports(serial_port_list *a, serial_port_list *b) {
	if (a == NULL || b == NULL || a->port_count != b->port_count) {
		return false;
	}
	for (int i = 0; i < a->port_count; i++) {
		if (strcmp(a->ports[i].name, b->ports[i].name) || strcmp(a->ports[i].label, b->ports[i].label)) {
			return false;
		}
	}
	return true;
}

// the bus lock must be held, the list is scanned if there is none yet
static serial_port_list *current_serial_ports(void) {
	if (serial_ports == NULL) {
		serial_ports = scan_serial_ports();
	}
	return serial_ports;
}

static bool indigo_select_matching_usbserial_device(indigo_device *device, indigo_serial_info *serial_info, int num_serial_info) {
	indigo_device_match_pattern *patterns = (indigo_device_match_pattern*)device->match_patterns;
	int patterns_count = device->match_patterns_count;

	if (serial_info == NULL || num_serial_info == 0) {
		return false;
	}

	/* If the device port is not empty and is not prefixed with "auto://", do nothing */
	if (DEVICE_PORT_ITEM->text.value[0] != '\0' && strncmp(DEVICE_PORT_ITEM->text.value, USBSERIAL_AUTO_PREFIX, strlen(USBSERIAL_AUTO_PREFIX))) {
		INDIGO_DEBUG(indigo_debug("%s(): Selected port for '%s' is not empty and is not prefixed with 'auto://', keeping selected: %s", __FUNCTION__, device->name, DEVICE_PORT_ITEM->text.value));
		return true;
	}

	bool port_exists = false;
	/* If the selected device port is prefixed with "auto://", but it matches the pattern do not change it */
	if (!strncmp(DEVICE_PORT_ITEM->text.value, USBSERIAL_AUTO_PREFIX, strlen(USBSERIAL_AUTO_PREFIX))) {
		char target[PATH_MAX] = {0};
		char *path = DEVICE_PORT_ITEM->text.value + strlen(USBSERIAL_AUTO_PREFIX);
		if (indigo_uni_realpath(path, target)) {
			INDIGO_DEBUG(indigo_debug("%s(): Selected port %s for '%s' resolves to %s", __FUNCTION__, path, device->name, target));
			for (int i = 0; i < num_serial_info; i++) {
				if (!strcmp(serial_info[i].path, target)) {
					port_exists = true;
					int matched_pattern_index = -1;
					if (indigo_usbserial_match_index(serial_info + i, 1, patterns, patterns_count, &matched_pattern_index)) {
						INDIGO_DEBUG(indigo_debug( "%s(): Selected port for '%s' matches the pattern, keeping selected: %s", __FUNCTION__, device->name, DEVICE_PORT_ITEM->text.value));
						device->matched_pattern_index = matched_pattern_index;
						return true;
					}
				}
			}
		}
	}

	indigo_serial_info *matching = indigo_usbserial_match_index(serial_info, num_serial_info, patterns, patterns_count, &device->matched_pattern_index);

	/* if nothing is matched select first USB-Serial port */
	if (matching == NULL) {
		if (port_exists) {
			INDIGO_DEBUG(indigo_debug("%s(): No matching port found for '%s', keeping selected: %s", __FUNCTION__, device->name, DEVICE_PORT_ITEM->text.value));
			return true;
		}
		INDIGO_DEBUG(indigo_debug("%s(): Selected port does not exist and no matching port found for '%s', selecting first USB-Serial port", __FUNCTION__, device->name));
		matching = &serial_info[0];
	}
	if (matching) {
		char buffer[INDIGO_VALUE_SIZE] = {0};
		snprintf(buffer, INDIGO_VALUE_SIZE-1, "%s%s", USBSERIAL_AUTO_PREFIX, matching->path);
		INDIGO_DEBUG(indigo_debug("%s(): Selected new port for '%s': %s", __FUNCTION__, device->name, buffer));
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, buffer);
		return true;
	}
	return false;
}

static int indigo_find_matching_pattern_index(indigo_device *device, const char *path) {
	indigo_device_match_pattern *patterns = (indigo_device_match_pattern*)device->match_patterns;
	int patterns_count = device->match_patterns_count;
	int matched_pattern_index = -1;

	if (path == NULL || patterns == NULL || patterns_count == 0) {
		return -1;
	}

	const char *real_path = path;
	if (!strncmp(path, USBSERIAL_AUTO_PREFIX, strlen(USBSERIAL_AUTO_PREFIX))) {
		real_path = path + strlen(USBSERIAL_AUTO_PREFIX);
	}

	char target[PATH_MAX] = { 0 };
	bool has_realpath = indigo_uni_realpath(real_path, target);
	indigo_lock_bus();
	serial_port_list *list = current_serial_ports();
	for (int i = 0; i < list->usb_count; i++) {
		if (!strcmp(list->usb[i].path, real_path) || (has_realpath && !strcmp(list->usb[i].path, target))) {
			indigo_usbserial_match_index(list->usb + i, 1, patterns, patterns_count, &matched_pattern_index);
			indigo_unlock_bus();
			INDIGO_DEBUG(indigo_debug("%s(): path = %s matches pattern index %d for '%s'", __FUNCTION__, path, matched_pattern_index, device->name));
			return matched_pattern_index;
		}
	}
	indigo_unlock_bus();
	INDIGO_DEBUG(indigo_debug("%s(): path = %s not found in enumerated USB-Serial devices for '%s'", __FUNCTION__, path, device->name));
	return -1;
}

static bool make_config_file_name(char *device_name, int profile, const char *suffix, char *path, int size) {
	int path_end = snprintf(path, size, "%s", indigo_uni_config_folder());
	if (indigo_uni_mkdir(path)) {
		if (indigo_server_tcp_port == 7624 || indigo_is_ephemeral_port) {
			if (profile) {
				snprintf(path + path_end, size - path_end, "%c%s#%d%s", INDIGO_PATH_SEPATATOR, device_name, profile, suffix);
			} else {
				snprintf(path + path_end, size - path_end, "%c%s%s", INDIGO_PATH_SEPATATOR, device_name, suffix);
			}
		} else {
			if (profile) {
				snprintf(path + path_end, size - path_end, "%c%s#%d_%d%s", INDIGO_PATH_SEPATATOR, device_name, profile, indigo_server_tcp_port, suffix);
			} else {
				snprintf(path + path_end, size - path_end, "%c%s_%d%s", INDIGO_PATH_SEPATATOR, device_name, indigo_server_tcp_port, suffix);
			}
		}
		char *space = strchr(path, ' ');
		while (space != NULL) {
			*space = '_';
			space = strchr(space + 1, ' ');
		}
		return true;
	}
	return false;
}

bool test_config_file(char *device_name, int profile, char *suffix) {
	static char path[512];
	if (make_config_file_name(device_name, profile, suffix, path, sizeof(path))) {
		return indigo_uni_is_readable(path);
	}
	return false;
}

indigo_uni_handle *indigo_open_config_file(char *device_name, int profile, bool create, const char *suffix) {
	static char path[512];
	if (make_config_file_name(device_name, profile, suffix, path, sizeof(path))) {
		indigo_uni_handle *handle = create ? indigo_uni_create_file(path, INDIGO_LOG_TRACE) : indigo_uni_open_file(path, INDIGO_LOG_TRACE);
		if (handle == NULL) {
			INDIGO_TRACE(indigo_trace("Can't %s %s", create ? "create" : "open", path));
		}
		return handle;
	} else {
		indigo_error("Can't create config file");
	}
	return NULL;
}

static indigo_result load_properties(indigo_device *device, bool default_properties, indigo_client *reader, indigo_result (*dispatch)(indigo_client *, indigo_property *)) {
	assert(device != NULL);
	int profile = 0;
	if (DEVICE_CONTEXT) {
		pthread_mutex_lock(&DEVICE_CONTEXT->config_mutex);
		for (int i = 0; i < PROFILE_COUNT; i++) {
			if (PROFILE_PROPERTY->items[i].sw.value) {
				profile = i;
				break;
			}
		}
	}
	indigo_result result = INDIGO_FAILED;
	indigo_uni_handle *handle = indigo_open_config_file(device->name, profile, false, ".common");
	if (handle != NULL) {
		INDIGO_TRACE(indigo_trace("%d -> // Common config file for '%s'", handle->index, device->name));
		indigo_client *client = reader ? reader : indigo_safe_malloc(sizeof(indigo_client));
		strcpy(client->name, CONFIG_READER);
		indigo_adapter_context *context = reader ? reader->client_context : indigo_safe_malloc(sizeof(indigo_adapter_context));
		context->input = handle;
		client->client_context = context;
		client->version = INDIGO_VERSION_CURRENT;
		indigo_xml_parse_with_callback(NULL, client, dispatch);
		indigo_uni_close(&handle);
		context->input = NULL;
		if (reader == NULL) {
			indigo_safe_free(context);
			indigo_safe_free(client);
		}
		result = INDIGO_OK;
	}
	handle = indigo_open_config_file(device->name, profile, false, default_properties ? ".default" : ".config");
	if (handle != NULL) {
		INDIGO_TRACE(indigo_trace("%d -> // Config file for '%s'", handle->index, device->name));
		indigo_client *client = reader ? reader : indigo_safe_malloc(sizeof(indigo_client));
		strcpy(client->name, CONFIG_READER);
		indigo_adapter_context *context = reader ? reader->client_context : indigo_safe_malloc(sizeof(indigo_adapter_context));
		context->input = handle;
		client->client_context = context;
		client->version = INDIGO_VERSION_CURRENT;
		indigo_xml_parse_with_callback(NULL, client, dispatch);
		indigo_uni_close(&handle);
		context->input = NULL;
		if (reader == NULL) {
			indigo_safe_free(context);
			indigo_safe_free(client);
		}
		result = INDIGO_OK;
	}
	if (DEVICE_CONTEXT) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	}
	return result;
}

indigo_result indigo_load_properties(indigo_device *device, bool default_properties) {
	return load_properties(device, default_properties, NULL, NULL);
}

typedef struct config_request {
	indigo_property *property;
	struct config_request *next;
} config_request;

// INDIGO_COPY_*_PROCESS_CHANGE and an explicit rejection both publish before the request returns, so
// a property that produced no update at all within this window was dropped rather than accepted. The
// window is generous because a driver may publish its first update from a queued handler instead.
#define CONFIG_RESTORE_ACK_TIMEOUT 5.0

typedef struct {
	indigo_adapter_context adapter; // XML reader context must be first.
	indigo_client client;
	pthread_mutex_t mutex;
	config_request *first, *last;
	bool active, connected, failed, probed, available, dispatched, updated, rejected;
	indigo_property_state state;
	double deadline, ack_deadline;
	char unrestored[INDIGO_VALUE_SIZE];
} config_restore;

static indigo_result config_restore_observe_property(indigo_client *client, indigo_property *property, bool update) {
	config_restore *restore = client->client_context;
	pthread_mutex_lock(&restore->mutex);
	if (restore->active && restore->first && !strcmp(property->device, restore->first->property->device)) {
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && restore->connected) {
			indigo_item *connected = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
			if (connected && !connected->sw.value) {
				restore->failed = true;
			}
		}
		if (!strcmp(property->name, restore->first->property->name)) {
			restore->available = !property->hidden && property->perm != INDIGO_RO_PERM;
			if (!restore->dispatched || update) {
				restore->state = property->state;
			}
			if (restore->dispatched && update) {
				restore->updated = true;
				if (property->state == INDIGO_ALERT_STATE) {
					// One refused property must not discard the settings that follow it in the file.
					restore->rejected = true;
				}
			}
		}
	}
	pthread_mutex_unlock(&restore->mutex);
	return INDIGO_OK;
}

static indigo_result config_restore_observe(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return config_restore_observe_property(client, property, false);
}

static indigo_result config_restore_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return config_restore_observe_property(client, property, true);
}

static indigo_result config_restore_delete(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	config_restore *restore = client->client_context;
	pthread_mutex_lock(&restore->mutex);
	if (restore->active && restore->first && !strcmp(property->device, restore->first->property->device) && (!*property->name || (restore->dispatched && !strcmp(property->name, restore->first->property->name)))) {
		restore->failed = true;
	}
	pthread_mutex_unlock(&restore->mutex);
	return INDIGO_OK;
}

static indigo_result config_restore_collect(indigo_client *client, indigo_property *property) {
	config_restore *restore = client->client_context;
	if (property->type != INDIGO_TEXT_VECTOR && property->type != INDIGO_NUMBER_VECTOR && property->type != INDIGO_SWITCH_VECTOR) {
		return INDIGO_FAILED;
	}
	config_request *request = indigo_safe_malloc(sizeof(config_request));
	request->property = indigo_copy_property(NULL, property);
	if (restore->last) {
		restore->last->next = request;
	} else {
		restore->first = request;
	}
	restore->last = request;
	return INDIGO_OK;
}

static void config_restore_pop(config_restore *restore) {
	config_request *request = restore->first;
	restore->first = request->next;
	if (restore->first == NULL) {
		restore->last = NULL;
	}
	indigo_release_property(request->property);
	indigo_safe_free(request);
	restore->probed = restore->available = restore->dispatched = restore->updated = restore->rejected = false;
	restore->state = INDIGO_IDLE_STATE;
}

// Record a property the driver refused or never answered and continue with the rest of the file.
static void config_restore_skip(config_restore *restore) {
	const char *name = restore->first->property->name;
	size_t used = strlen(restore->unrestored);
	if (used + strlen(name) + 3 < INDIGO_VALUE_SIZE) {
		snprintf(restore->unrestored + used, INDIGO_VALUE_SIZE - used, "%s%s", used ? ", " : "", name);
	}
	config_restore_pop(restore);
}

static void config_restore_handler(indigo_device *device) {
	config_restore *restore = DEVICE_CONTEXT->config_restore;
	pthread_mutex_lock(&restore->mutex);
	while (restore->first && !restore->failed && indigo_monotonic_time() < restore->deadline) {
		indigo_property *request = restore->first->property;
		if (!restore->probed) {
			restore->probed = true;
			pthread_mutex_unlock(&restore->mutex);
			indigo_enumerate_properties(&restore->client, request);
			pthread_mutex_lock(&restore->mutex);
			// A saved property may no longer exist in the current device mode.
			if (!restore->available || !strcmp(request->name, CONFIG_PROPERTY_NAME)) {
				config_restore_pop(restore);
				continue;
			}
		}
		if (!restore->dispatched && restore->state != INDIGO_BUSY_STATE) {
			restore->dispatched = true;
			restore->ack_deadline = indigo_monotonic_time() + CONFIG_RESTORE_ACK_TIMEOUT;
			request->access_token = indigo_get_device_or_master_token(request->device);
			pthread_mutex_unlock(&restore->mutex);
			indigo_change_property(&restore->client, request);
			pthread_mutex_lock(&restore->mutex);
		}
		if (restore->dispatched && restore->updated && restore->state != INDIGO_BUSY_STATE) {
			if (restore->rejected) {
				config_restore_skip(restore);
			} else {
				config_restore_pop(restore);
			}
		} else if (restore->dispatched && !restore->updated && indigo_monotonic_time() >= restore->ack_deadline) {
			// The driver accepted the request without publishing anything, so waiting for it would
			// burn the whole deadline and lose every setting after it in file order.
			config_restore_skip(restore);
		} else if (!restore->failed) {
			pthread_mutex_unlock(&restore->mutex);
			indigo_execute_background_handler_in(device, 0.01, config_restore_handler);
			return;
		}
	}
	bool aborted = restore->failed || restore->first != NULL;
	char unrestored[INDIGO_VALUE_SIZE];
	strcpy(unrestored, restore->unrestored);
	while (restore->first) {
		config_restore_pop(restore);
	}
	restore->active = false;
	CONFIG_LOAD_ITEM->sw.value = false;
	CONFIG_PROPERTY->state = (aborted || *unrestored) ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	pthread_mutex_unlock(&restore->mutex);
	if (aborted) {
		indigo_update_property(device, CONFIG_PROPERTY, "Configuration restore failed or timed out");
	} else if (*unrestored) {
		indigo_update_property(device, CONFIG_PROPERTY, "Configuration restored, except '%s' which the driver did not accept", unrestored);
	} else {
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	}
}

static void start_config_restore(indigo_device *device) {
	config_restore *restore = DEVICE_CONTEXT->config_restore;
	if (restore == NULL) {
		restore = indigo_safe_malloc(sizeof(config_restore));
		pthread_mutex_init(&restore->mutex, NULL);
		strcpy(restore->client.name, CONFIG_READER);
		restore->client.client_context = restore;
		restore->client.version = INDIGO_VERSION_CURRENT;
		restore->client.define_property = config_restore_observe;
		restore->client.update_property = config_restore_update;
		restore->client.delete_property = config_restore_delete;
		DEVICE_CONTEXT->config_restore = restore;
		if (indigo_attach_client(&restore->client) != INDIGO_OK) {
			pthread_mutex_destroy(&restore->mutex);
			indigo_safe_free(restore);
			DEVICE_CONTEXT->config_restore = NULL;
			CONFIG_LOAD_ITEM->sw.value = false;
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CONFIG_PROPERTY, "Can't observe configuration restore");
			return;
		}
	}
	CONFIG_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CONFIG_PROPERTY, NULL);
	// Parse first, then dispatch in file order and wait for each request's completion.
	bool failed = CONFIG_PROPERTY->count != 1 && load_properties(device, false, &restore->client, config_restore_collect) != INDIGO_OK;
	pthread_mutex_lock(&restore->mutex);
	restore->failed = failed;
	restore->connected = IS_CONNECTED;
	restore->deadline = indigo_monotonic_time() + 120;
	restore->unrestored[0] = 0;
	restore->active = true;
	pthread_mutex_unlock(&restore->mutex);
	config_restore_handler(device);
}

static void release_config_restore(indigo_device *device) {
	config_restore *restore = DEVICE_CONTEXT->config_restore;
	if (restore) {
		indigo_detach_client(&restore->client);
		while (restore->first) {
			config_restore_pop(restore);
		}
		pthread_mutex_destroy(&restore->mutex);
		indigo_safe_free(restore);
		DEVICE_CONTEXT->config_restore = NULL;
	}
}

indigo_result indigo_save_property(indigo_device *device, indigo_uni_handle **file_handle, indigo_property *property) {
	if (property == NULL) {
		return INDIGO_FAILED;
	}
	if (DEVICE_CONTEXT && pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex)) {
		INDIGO_DEBUG(indigo_debug("Config file is locked, property '%s.%s' not saved", device->name, property->name));
		return INDIGO_FAILED;
	}
	indigo_result result = INDIGO_OK;
	if (!property->hidden && property->perm != INDIGO_RO_PERM) {
		char b1[32];
		if (file_handle == NULL) {
			file_handle = &DEVICE_CONTEXT->property_save_file_handle;
		}
		indigo_uni_handle *handle = *file_handle;
		if (handle == NULL) {
			int profile = 0;
			if (DEVICE_CONTEXT) {
				for (int i = 0; i < PROFILE_COUNT; i++)
					if (PROFILE_PROPERTY->items[i].sw.value) {
						profile = i;
						break;
					}
			}
			bool common_property = false;
			if (!strcmp(property->name, PROFILE_NAME_PROPERTY_NAME)) {
				common_property = true;
			}
			*file_handle = handle = indigo_open_config_file(property->device, profile, true, common_property ? ".common" : ".config");
			if (handle == NULL) {
				if (DEVICE_CONTEXT) {
					pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
				}
				return INDIGO_FAILED;
			}
		}
		switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				if (indigo_uni_printf(handle, "<newTextVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]) <= 0) {
					result = INDIGO_FAILED;
				}
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					if (indigo_uni_printf(handle, "<oneText name='%s'>%s</oneText>\n", indigo_xml_escape_b(1, item->name), indigo_xml_escape_b(5, indigo_get_text_item_value(item))) <= 0) {
						result = INDIGO_FAILED;
					}
				}
				if (indigo_uni_printf(handle, "</newTextVector>\n") <= 0) {
					result = INDIGO_FAILED;
				}
				break;
			case INDIGO_NUMBER_VECTOR:
				if (indigo_uni_printf(handle, "<newNumberVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]) <= 0) {
					result = INDIGO_FAILED;
				}
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					if (indigo_uni_printf(handle, "<oneNumber name='%s'>%s</oneNumber>\n", indigo_xml_escape_b(1, item->name), indigo_dtoa(item->number.value, b1)) <= 0) {
						result = INDIGO_FAILED;
					}
				}
				if (indigo_uni_printf(handle, "</newNumberVector>\n") <= 0) {
					result = INDIGO_FAILED;
				}
				break;
			case INDIGO_SWITCH_VECTOR:
				if (indigo_uni_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]) <= 0) {
					result = INDIGO_FAILED;
				}
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					if (indigo_uni_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_xml_escape_b(1, item->name), item->sw.value ? "On" : "Off") <= 0) {
						result = INDIGO_FAILED;
					}
				}
				if (indigo_uni_printf(handle, "</newSwitchVector>\n") <= 0) {
					result = INDIGO_FAILED;
				}
				break;
			default:
				break;
		}
	}
	if (DEVICE_CONTEXT) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	}
	return result;
}

indigo_result indigo_save_property_items(indigo_device*device, indigo_uni_handle *file_handle, indigo_property *property, const int count, const char **items) {
	if (property == NULL) {
		return INDIGO_FAILED;
	}
	if (DEVICE_CONTEXT && pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex)) {
		INDIGO_DEBUG(indigo_debug("Config file is locked, property '%s.%s' not saved", device->name, property->name));
		return INDIGO_FAILED;
	}
	if (!property->hidden && property->perm != INDIGO_RO_PERM) {
		char b1[32];
		if (file_handle == NULL) {
			file_handle = DEVICE_CONTEXT->property_save_file_handle;
		}
		indigo_uni_handle *handle = file_handle;
		if (handle == NULL) {
			int profile = 0;
			if (DEVICE_CONTEXT) {
				for (int i = 0; i < PROFILE_COUNT; i++) {
					if (PROFILE_PROPERTY->items[i].sw.value) {
						profile = i;
						break;
					}
				}
			}
			file_handle = handle = indigo_open_config_file(property->device, profile, true, ".config");
			if (handle == NULL) {
				if (DEVICE_CONTEXT) {
					pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
				}
				return INDIGO_FAILED;
			}
		}
		switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				indigo_uni_printf(handle, "<newTextVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					for (int j = 0; j < count; j++) {
						if (!strncmp(items[j], item->name, INDIGO_NAME_SIZE)) {
							indigo_uni_printf(handle, "<oneText name='%s'>%s</oneText>\n", indigo_xml_escape_b(1, item->name), indigo_xml_escape(item->text.value));
							break;
						}
					}
				}
				indigo_uni_printf(handle, "</newTextVector>\n");
				break;
			case INDIGO_NUMBER_VECTOR:
				indigo_uni_printf(handle, "<newNumberVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					for (int j = 0; j < count; j++) {
						if (!strncmp(items[j], item->name, INDIGO_NAME_SIZE)) {
							indigo_uni_printf(handle, "<oneNumber name='%s'>%s</oneNumber>\n", indigo_xml_escape_b(1, item->name), indigo_dtoa(item->number.value, b1));
							break;
						}
					}
				}
				indigo_uni_printf(handle, "</newNumberVector>\n");
				break;
			case INDIGO_SWITCH_VECTOR:
				indigo_uni_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					for (int j = 0; j < count; j++) {
						if (!strncmp(items[j], item->name, INDIGO_NAME_SIZE)) {
							indigo_uni_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_xml_escape_b(1, item->name), item->sw.value ? "On" : "Off");
							break;
						}
					}
				}
				indigo_uni_printf(handle, "</newSwitchVector>\n");
				break;
			default:
				break;
		}
	}
	if (DEVICE_CONTEXT) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	}
	return INDIGO_OK;
}

indigo_result indigo_remove_properties(indigo_device *device) {
	assert(device != NULL);
	int profile = 0;
	if (DEVICE_CONTEXT) {
		if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex)) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < PROFILE_COUNT; i++)
			if (PROFILE_PROPERTY->items[i].sw.value) {
				profile = i;
				break;
			}
	}
	static char path[512];
	if (make_config_file_name(device->name, profile, ".config", path, sizeof(path))) {
		if (indigo_uni_remove(path)) {
			if (DEVICE_CONTEXT) {
				pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
			}
			return INDIGO_OK;
		}
	}
	if (DEVICE_CONTEXT) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	}
	return INDIGO_FAILED;
}

static const char *device_driver_name(indigo_device *device) {
	return INFO_DEVICE_DRIVER_ITEM->text.value;
}

// the bus lock must be held
static additional_ports *find_additional_ports(const char *driver) {
	for (int i = 0; i < driver_ports_count; i++) {
		if (!strcmp(driver_ports[i].driver, driver)) {
			return driver_ports + i;
		}
	}
	return NULL;
}

// the bus lock must be held, the device must be disconnected
static void select_auto_port(indigo_device *device, serial_port_list *list) {
	/* if there are no USB-Serial ports but there are regular serial ports and we want auto selected port, select the first one.
	   Otherwise autoselect the matching port from the available USB-Serial ports. Only the serial ports are candidates,
	   the additional ports of the driver, e.g. askar:// URLs, are not serial ports and can not get the auto:// prefix.
	*/
	if (device->match_patterns_count == 0 && list->port_count > 0 && list->usb_count == 0 && (DEVICE_PORT_ITEM->text.value[0] == '\0' || !strncmp(DEVICE_PORT_ITEM->text.value, USBSERIAL_AUTO_PREFIX, strlen(USBSERIAL_AUTO_PREFIX)))) {
		snprintf(DEVICE_PORT_ITEM->text.value, INDIGO_VALUE_SIZE, "%s%s", USBSERIAL_AUTO_PREFIX, list->ports[0].name);
		INDIGO_DEBUG(indigo_debug("%s(): No USB-Serial ports found, selecting first tty port: %s", __FUNCTION__, DEVICE_PORT_ITEM->text.value));
	} else {
		indigo_select_matching_usbserial_device(device, list->usb, list->usb_count);
	}
}

// the bus lock must be held and DEVICE_PORTS must not be defined, as it may be reallocated; the selected port of
// a connected device is never changed
static void fill_device_ports(indigo_device *device, serial_port_list *list) {
	additional_ports *additional = find_additional_ports(device_driver_name(device));
	int count = 1 + list->port_count + (additional ? additional->count : 0);
	if (count > DEVICE_PORTS_PROPERTY->allocated_count) {
		DEVICE_PORTS_PROPERTY = indigo_resize_property(DEVICE_PORTS_PROPERTY, count);
	}
	DEVICE_PORTS_PROPERTY->count = 1;
	for (int i = 0; i < list->port_count; i++) {
		indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + DEVICE_PORTS_PROPERTY->count++, list->ports[i].name, list->ports[i].label, false);
	}
	for (int i = 0; additional && i < additional->count; i++) {
		indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + DEVICE_PORTS_PROPERTY->count++, additional->ports[i].name, additional->ports[i].label, false);
	}
	if (!IS_DISCONNECTED) {
		return;
	}
	select_auto_port(device, list);
}

// the bus lock must be held
static void refresh_device_ports(indigo_device *device) {
	char port[INDIGO_VALUE_SIZE];
	INDIGO_COPY_VALUE(port, DEVICE_PORT_ITEM->text.value);
	indigo_delete_property(device, DEVICE_PORTS_PROPERTY, NULL);
	fill_device_ports(device, serial_ports);
	indigo_define_property(device, DEVICE_PORTS_PROPERTY, NULL);
	if (strcmp(port, DEVICE_PORT_ITEM->text.value)) {
		device->matched_pattern_index = indigo_find_matching_pattern_index(device, DEVICE_PORT_ITEM->text.value);
		if (*DEVICE_PORT_ITEM->text.value == '/' && !indigo_uni_is_readable(DEVICE_PORT_ITEM->text.value)) {
			DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
	}
}

// -------------------------------------------------------------------------------- lost serial ports

// The port of every device connected over a serial port, resolved when it connected, and the devices disconnected
// because their port disappeared, which end ALERT instead of OK. Both are guarded by the bus lock.

typedef struct {
	indigo_device *device;
	char port[PATH_MAX];
} device_port;

typedef struct {
	device_port *entries;
	int count;
	int size;
} device_port_list;

static device_port_list connected_ports = { 0 };
static device_port_list lost_ports = { 0 };

// The serial port name a device uses, comparable with the names of the serial port list: auto:// removed, a link
// resolved, on Windows "\\.\" and a trailing ':' removed; empty for a network URL or no port.
static void serial_port_name(const char *name, char *result) {
	result[0] = '\0';
	if (!strncmp(name, USBSERIAL_AUTO_PREFIX, strlen(USBSERIAL_AUTO_PREFIX))) {
		name += strlen(USBSERIAL_AUTO_PREFIX);
	}
	if (*name == '\0' || strstr(name, "://") != NULL) {
		return;
	}
#if defined(INDIGO_WINDOWS)
	if (!strncmp(name, "\\\\.\\", 4)) {
		name += 4;
	}
	snprintf(result, PATH_MAX, "%s", name);
	size_t length = strlen(result);
	if (length > 0 && result[length - 1] == ':') {
		result[length - 1] = '\0';
	}
	for (char *c = result; *c; c++) {
		*c = (char)toupper((unsigned char)*c);
	}
#else
	if (realpath(name, result) == NULL) {
		snprintf(result, PATH_MAX, "%s", name);
	}
#endif
}

static device_port *find_device_port(device_port_list *list, indigo_device *device) {
	for (int i = 0; i < list->count; i++) {
		if (list->entries[i].device == device) {
			return list->entries + i;
		}
	}
	return NULL;
}

static void set_device_port(device_port_list *list, indigo_device *device, const char *port) {
	device_port *entry = find_device_port(list, device);
	if (entry == NULL) {
		if (list->count == list->size) {
			list->size += 16;
			list->entries = indigo_safe_realloc(list->entries, list->size * sizeof(device_port));
		}
		entry = list->entries + list->count++;
		entry->device = device;
	}
	snprintf(entry->port, sizeof(entry->port), "%s", port);
}

static void remove_device_port(device_port_list *list, indigo_device *device) {
	device_port *entry = find_device_port(list, device);
	if (entry != NULL) {
		*entry = list->entries[--list->count];
	}
}

static bool device_is_connected(indigo_device *device) {
	return IS_CONNECTED;
}

static bool serial_port_listed(serial_port_list *list, const char *port) {
	for (int i = 0; list != NULL && i < list->port_count; i++) {
		char name[PATH_MAX];
		serial_port_name(list->ports[i].name, name);
		if (!strcmp(name, port)) {
			return true;
		}
	}
	return false;
}

// The bus lock must be held. Disconnects every connected device whose port was in the previous serial port list and
// is missing from the new one, and the connected devices that share the port through it as their master device. They
// end ALERT, see serial_port_connection_changed(). A port that was not listed, e.g. a network URL, is never affected.
static void disconnect_lost_ports(serial_port_list *previous, serial_port_list *current) {
	indigo_device *lost[64];
	int lost_count = 0;
	for (int i = 0; previous != NULL && i < connected_ports.count && lost_count < 64; i++) {
		device_port *entry = connected_ports.entries + i;
		if (!serial_port_listed(previous, entry->port) || serial_port_listed(current, entry->port)) {
			continue;
		}
		INDIGO_ERROR(indigo_error("%s: serial port %s disappeared", entry->device->name, entry->port));
		indigo_device *slaves[MAX_SLAVE_DEVICES];
		int slave_count = indigo_query_slave_devices(entry->device, slaves, MAX_SLAVE_DEVICES - 1);
		set_device_port(&lost_ports, entry->device, entry->port);
		lost[lost_count++] = entry->device;
		for (int j = 0; j < slave_count && lost_count < 64; j++) {
			if (device_is_connected(slaves[j])) {
				set_device_port(&lost_ports, slaves[j], entry->port);
				lost[lost_count++] = slaves[j];
			}
		}
	}
	// requested once the lists are no longer iterated, a driver may handle the request synchronously
	for (int i = 0; i < lost_count; i++) {
		indigo_device_disconnect(NULL, lost[i]->name);
	}
}

// Called by the base class with every published CONNECTION: records the port of a device that connected over a serial
// port, and for a device disconnected because its port disappeared returns true with the port, so the regular end of
// the disconnection is published ALERT.
static bool serial_port_connection_changed(indigo_device *device, char *lost_port) {
	bool lost = false;
	indigo_lock_bus();
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (CONNECTION_PROPERTY->state == INDIGO_OK_STATE) {
			remove_device_port(&lost_ports, device);
			char port[PATH_MAX];
			serial_port_name(DEVICE_PORT_PROPERTY->hidden ? "" : DEVICE_PORT_ITEM->text.value, port);
			if (*port) {
				set_device_port(&connected_ports, device, port);
			} else {
				remove_device_port(&connected_ports, device);
			}
		}
	} else {
		remove_device_port(&connected_ports, device);
		device_port *entry = find_device_port(&lost_ports, device);
		if (entry != NULL && CONNECTION_PROPERTY->state == INDIGO_OK_STATE) {
			snprintf(lost_port, PATH_MAX, "%s", entry->port);
			remove_device_port(&lost_ports, device);
			lost = true;
		}
	}
	indigo_unlock_bus();
	return lost;
}

// Rescans the serial ports and, if they changed, rebuilds DEVICE_PORTS of all registered devices except the given one.
// The scan runs without the bus lock unless the caller holds it. Returns true if the list changed.
static bool update_serial_ports(indigo_device *except) {
	serial_port_list *list = scan_serial_ports();
	indigo_lock_bus();
	bool changed = !same_serial_ports(serial_ports, list);
	if (changed) {
		disconnect_lost_ports(serial_ports, list);
		indigo_safe_free(serial_ports);
		serial_ports = list;
		list = NULL;
		INDIGO_DEBUG(indigo_debug("%s(): Serial ports changed, updating %d devices", __FUNCTION__, serial_port_devices_count));
		for (int i = 0; i < serial_port_devices_count; i++) {
			if (serial_port_devices[i] != except) {
				refresh_device_ports(serial_port_devices[i]);
			}
		}
	}
	indigo_unlock_bus();
	indigo_safe_free(list);
	return changed;
}

#if defined(INDIGO_USB_HOTPLUG_POLLING)
static pthread_mutex_t serial_ports_event_mutex = PTHREAD_MUTEX_INITIALIZER;
static int serial_ports_events = 0;
static bool serial_ports_worker_running = false;

// A USB-Serial port appears a moment after its USB device, so the ports are rescanned after a short delay, and once
// more later if nothing changed yet. Events arriving meanwhile are handled by the same worker.
static void *serial_ports_worker(void *data) {
	indigo_rename_thread("Serial ports");
	pthread_mutex_lock(&serial_ports_event_mutex);
	int handled = serial_ports_events;
	pthread_mutex_unlock(&serial_ports_event_mutex);
	while (true) {
		indigo_lock_bus();
		bool used = serial_port_devices_count > 0;
		if (!used) {
			// nobody shows the ports, the next device to do so scans them again
			indigo_safe_free(serial_ports);
			serial_ports = NULL;
		}
		indigo_unlock_bus();
		if (used) {
			indigo_usleep(INDIGO_DELAY(SERIAL_PORTS_SETTLE_TIME));
			if (!update_serial_ports(NULL)) {
				indigo_usleep(INDIGO_DELAY(SERIAL_PORTS_RETRY_TIME));
				update_serial_ports(NULL);
			}
		}
		pthread_mutex_lock(&serial_ports_event_mutex);
		if (handled == serial_ports_events) {
			serial_ports_worker_running = false;
			pthread_mutex_unlock(&serial_ports_event_mutex);
			break;
		}
		handled = serial_ports_events;
		pthread_mutex_unlock(&serial_ports_event_mutex);
	}
	return NULL;
}

// called by the USB hot-plug thread for every USB device, it must not block
static int LIBUSB_CALL serial_ports_hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	pthread_mutex_lock(&serial_ports_event_mutex);
	serial_ports_events++;
	bool start = !serial_ports_worker_running;
	serial_ports_worker_running = true;
	pthread_mutex_unlock(&serial_ports_event_mutex);
	if (start && !indigo_async(serial_ports_worker, NULL)) {
		pthread_mutex_lock(&serial_ports_event_mutex);
		serial_ports_worker_running = false;
		pthread_mutex_unlock(&serial_ports_event_mutex);
	}
	return 0;
}

static void register_serial_ports_hotplug(void) {
	indigo_start_usb_event_handler();
	libusb_hotplug_callback_handle handle;
	int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, serial_ports_hotplug_callback, NULL, &handle);
	if (rc < 0) {
		indigo_error("Can't register serial port hot-plug callback (%s)", libusb_error_name(rc));
	}
}
#endif

void indigo_enumerate_serial_ports(indigo_device *device, indigo_property *property) {
	assert(device != NULL);
	int interface = atoi(INFO_DEVICE_INTERFACE_ITEM->text.value);
	if (interface & INDIGO_INTERFACE_AGENT) {
		INDIGO_DEBUG(indigo_debug("%s(): Skipping port enumeration for '%s'", __FUNCTION__, device->name));
		return;
	}
#if defined(INDIGO_USB_HOTPLUG_POLLING)
	static pthread_once_t hotplug_once = PTHREAD_ONCE_INIT;
	pthread_once(&hotplug_once, register_serial_ports_hotplug);
#endif
	indigo_lock_bus();
	bool registered = false;
	for (int i = 0; i < serial_port_devices_count; i++) {
		if (serial_port_devices[i] == device) {
			registered = true;
			break;
		}
	}
	if (!registered) {
		if (serial_port_devices_count == serial_port_devices_size) {
			serial_port_devices_size += 16;
			serial_port_devices = indigo_safe_realloc(serial_port_devices, serial_port_devices_size * sizeof(indigo_device *));
		}
		serial_port_devices[serial_port_devices_count++] = device;
	}
	fill_device_ports(device, current_serial_ports());
	indigo_unlock_bus();
}

void indigo_set_additional_ports(indigo_device *device, int count, const char **names, const char **labels) {
	assert(device != NULL);
	assert(count == 0 || (names != NULL && labels != NULL));
	const char *driver = device_driver_name(device);
	indigo_lock_bus();
	additional_ports *additional = find_additional_ports(driver);
	if (additional == NULL && count > 0) {
		driver_ports = indigo_safe_realloc(driver_ports, (driver_ports_count + 1) * sizeof(additional_ports));
		additional = driver_ports + driver_ports_count++;
		memset(additional, 0, sizeof(additional_ports));
		INDIGO_COPY_NAME(additional->driver, driver);
	}
	if (additional != NULL) {
		indigo_safe_free(additional->ports);
		additional->ports = NULL;
		additional->count = 0;
		if (count > 0) {
			additional->ports = indigo_safe_malloc(count * sizeof(serial_port));
			for (int i = 0; i < count; i++) {
				INDIGO_COPY_NAME(additional->ports[i].name, names[i]);
				INDIGO_COPY_VALUE(additional->ports[i].label, labels[i]);
			}
			additional->count = count;
		} else {
			*additional = driver_ports[--driver_ports_count];
		}
	}
	INDIGO_DEBUG(indigo_debug("%s(): %d additional ports of %s", __FUNCTION__, count, driver));
	for (int i = 0; i < serial_port_devices_count; i++) {
		indigo_device *registered = serial_port_devices[i];
		if (!strcmp(device_driver_name(registered), driver)) {
			refresh_device_ports(registered);
		}
	}
	indigo_unlock_bus();
}

// called by indigo_device_detach() before the device context is released, the additional ports of the driver are
// released with its last device
static void unregister_serial_ports_device(indigo_device *device) {
	indigo_lock_bus();
	// A driver disconnects its devices before it detaches them; a device still recorded as connected over a serial
	// port was not, and the port its driver opened is most likely left open. That is a driver bug the developer has
	// to fix, so it is logged and then asserted. The port is not closed here: the handle belongs to the driver, which
	// may still close it later. A build with NDEBUG, e.g. the Windows release build, only logs it.
	device_port *connected = find_device_port(&connected_ports, device);
	if (connected != NULL) {
		indigo_error("Driver %s misbehaves: device '%s' was detached while still connected, its serial port %s is probably left open", device_driver_name(device), device->name, connected->port);
	}
	assert(connected == NULL && "a driver must disconnect a device before it detaches it");
	remove_device_port(&connected_ports, device);
	remove_device_port(&lost_ports, device);
	for (int i = 0; i < serial_port_devices_count; i++) {
		if (serial_port_devices[i] == device) {
			serial_port_devices[i] = serial_port_devices[--serial_port_devices_count];
			break;
		}
	}
	const char *driver = device_driver_name(device);
	additional_ports *additional = find_additional_ports(driver);
	bool used = false;
	for (int i = 0; additional && !used && i < serial_port_devices_count; i++) {
		used = !strcmp(device_driver_name(serial_port_devices[i]), driver);
	}
	if (additional != NULL && !used) {
		indigo_safe_free(additional->ports);
		*additional = driver_ports[--driver_ports_count];
	}
	indigo_unlock_bus();
}

int indigo_compensate_backlash(int requested_position, int current_position, int backlash, bool *is_last_move_poitive) {
	uint32_t target_position = requested_position;
	int move = requested_position - current_position;
	if (move > 0) {
		INDIGO_DEBUG(indigo_debug("Moving (+), last_move (+) = %d", *is_last_move_poitive));
		if (*is_last_move_poitive) {
			target_position = requested_position;
		} else {
			target_position = requested_position + backlash;
		}
		*is_last_move_poitive = true;
	} else if (move < 0) {
		INDIGO_DEBUG(indigo_debug("Moving (-), last_move (+) = %d", *is_last_move_poitive));
		if (*is_last_move_poitive) {
			target_position = requested_position - backlash;
		} else {
			target_position = requested_position;
		}
		*is_last_move_poitive = false;
	}
	INDIGO_DEBUG(indigo_debug(
		"target = %d, requested = %d, position = %d, backlash = %d",
		target_position,
		requested_position,
		current_position,
		backlash
	));
	return target_position;
}

indigo_result indigo_device_attach(indigo_device *device, const char* driver_name, indigo_version version, int interface_mask) {
	assert(device != NULL);
	assert(device != NULL);
	if (DEVICE_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_device_context));
	}
	if (DEVICE_CONTEXT != NULL) {
		// -------------------------------------------------------------------------------- CONNECTION
		CONNECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, CONNECTION_PROPERTY_NAME, MAIN_GROUP, "Connection status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (CONNECTION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CONNECTION_CONNECTED_ITEM, CONNECTION_CONNECTED_ITEM_NAME, "Connected", false);
		indigo_init_switch_item(CONNECTION_DISCONNECTED_ITEM, CONNECTION_DISCONNECTED_ITEM_NAME, "Disconnected", true);
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY = indigo_init_text_property(NULL, device->name, INFO_PROPERTY_NAME, MAIN_GROUP, "Device info", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(INFO_DEVICE_NAME_ITEM, INFO_DEVICE_NAME_ITEM_NAME, "Device name", "%s", device->name);
		indigo_init_text_item(INFO_DEVICE_DRIVER_ITEM, INFO_DEVICE_DRIVER_ITEM_NAME, "Driver name", "%s", driver_name);
		unsigned indigo_version = (version >> 16) & 0xFFFF;
		if (indigo_version == 0) {
			indigo_version = 0x0200;
		}
		indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, INFO_DEVICE_VERSION_ITEM_NAME, "Driver version", "%d.%d.%d.%d", INDIGO_VERSION_MAJOR(indigo_version), INDIGO_VERSION_MINOR(indigo_version), INDIGO_VERSION_MAJOR(version), INDIGO_VERSION_MINOR(version));
		indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, INFO_DEVICE_INTERFACE_ITEM_NAME, "Interface", "%u", interface_mask);
		indigo_init_text_item(INFO_DEVICE_MODEL_ITEM, INFO_DEVICE_MODEL_ITEM_NAME, "Model", "%s", device->name);
		indigo_init_text_item(INFO_DEVICE_FW_REVISION_ITEM, INFO_DEVICE_FW_REVISION_ITEM_NAME, "Firmware Rev.", "N/A");
		indigo_init_text_item(INFO_DEVICE_HW_REVISION_ITEM, INFO_DEVICE_HW_REVISION_ITEM_NAME, "Hardware Rev.", "N/A");
		indigo_init_text_item(INFO_DEVICE_SERIAL_NUM_ITEM, INFO_DEVICE_SERIAL_NUM_ITEM_NAME, "Serial No.", "N/A");
		/* Decrease count as other items are rare if you need them just set count to 8 in the driver */
		INFO_PROPERTY->count = 4;
		// -------------------------------------------------------------------------------- SIMULATION
		bool is_simulator = strstr(device->name, "Simulator") != NULL;
		SIMULATION_PROPERTY = indigo_init_switch_property(NULL, device->name, SIMULATION_PROPERTY_NAME, MAIN_GROUP, "Simulation status", INDIGO_OK_STATE, is_simulator ? INDIGO_RO_PERM : INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (SIMULATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		SIMULATION_PROPERTY->hidden = !is_simulator;
		indigo_init_switch_item(SIMULATION_ENABLED_ITEM, SIMULATION_ENABLED_ITEM_NAME, "Enabled", is_simulator);
		indigo_init_switch_item(SIMULATION_DISABLED_ITEM, SIMULATION_DISABLED_ITEM_NAME, "Disabled", !is_simulator);
		// -------------------------------------------------------------------------------- CONFIG
		CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, CONFIG_PROPERTY_NAME, MAIN_GROUP, "Configuration control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CONFIG_SAVE_ITEM, CONFIG_SAVE_ITEM_NAME, "Save", false);
		indigo_init_switch_item(CONFIG_LOAD_ITEM, CONFIG_LOAD_ITEM_NAME, "Load", false);
		indigo_init_switch_item(CONFIG_REMOVE_ITEM, CONFIG_REMOVE_ITEM_NAME, "Remove", false);
		if (!test_config_file(device->name, 0, ".config")) {
			CONFIG_PROPERTY->count = 1;
		}
		// -------------------------------------------------------------------------------- PROFILE_NAME
		PROFILE_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, PROFILE_NAME_PROPERTY_NAME, MAIN_GROUP, "Profile names", INDIGO_OK_STATE, INDIGO_RW_PERM, PROFILE_COUNT);
		if (PROFILE_NAME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < PROFILE_COUNT; i++) {
			char name[INDIGO_NAME_SIZE], label[INDIGO_NAME_SIZE], value[INDIGO_VALUE_SIZE];
			sprintf(name, PROFILE_NAME_ITEM_NAME, i);
			sprintf(label, "Name #%d", i);
			sprintf(value, "Profile #%d", i);
			indigo_init_text_item(PROFILE_NAME_ITEM + i, name, label, value);
		}
		// -------------------------------------------------------------------------------- PROFILE
		PROFILE_PROPERTY = indigo_init_switch_property(NULL, device->name, PROFILE_PROPERTY_NAME, MAIN_GROUP, "Profile selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, PROFILE_COUNT);
		if (PROFILE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < PROFILE_COUNT; i++) {
			char name[INDIGO_NAME_SIZE];
			sprintf(name, PROFILE_ITEM_NAME, i);
			indigo_init_switch_item(PROFILE_ITEM + i, name, (PROFILE_NAME_ITEM + i)->text.value, i == 0);
		}
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY = indigo_init_text_property(NULL, device->name, DEVICE_PORT_PROPERTY_NAME, MAIN_GROUP, "Serial port", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (DEVICE_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		DEVICE_PORT_PROPERTY->hidden = true;
		indigo_init_text_item(DEVICE_PORT_ITEM, DEVICE_PORT_ITEM_NAME, "Device name or URL", USBSERIAL_AUTO_PREFIX);
		if (*DEVICE_PORT_ITEM->text.value == '/' && !indigo_uni_is_readable(DEVICE_PORT_ITEM->text.value)) {
			DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY = indigo_init_switch_property(NULL, device->name, DEVICE_PORTS_PROPERTY_NAME, MAIN_GROUP, "Serial ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICE_PORTS);
		if (DEVICE_PORTS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		DEVICE_PORTS_PROPERTY->hidden = true;
		indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items, DEVICE_PORTS_REFRESH_ITEM_NAME, "Refresh", false);
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY = indigo_init_text_property(NULL, device->name, DEVICE_BAUDRATE_PROPERTY_NAME, MAIN_GROUP, "Serial port baud rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (DEVICE_BAUDRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		DEVICE_BAUDRATE_PROPERTY->hidden = true;
		indigo_init_text_item(DEVICE_BAUDRATE_ITEM, DEVICE_BAUDRATE_ITEM_NAME, "Baud rate (bps)", "9600-8N1");
		// -------------------------------------------------------------------------------- SECURITY
		AUTHENTICATION_PROPERTY = indigo_init_text_property(NULL, device->name, AUTHENTICATION_PROPERTY_NAME, MAIN_GROUP, "Device authorization", INDIGO_OK_STATE, INDIGO_WO_PERM, 2);
		if (AUTHENTICATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		AUTHENTICATION_PROPERTY->hidden = true;
		indigo_init_text_item(AUTHENTICATION_PASSWORD_ITEM, AUTHENTICATION_PASSWORD_ITEM_NAME, "Password", "");
		indigo_init_text_item(AUTHENTICATION_USER_ITEM, AUTHENTICATION_USER_ITEM_NAME, "User name", "");
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
		ADDITIONAL_INSTANCES_PROPERTY = indigo_init_number_property(NULL, device->name, ADDITIONAL_INSTANCES_PROPERTY_NAME, MAIN_GROUP, "Additional instances", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (ADDITIONAL_INSTANCES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		ADDITIONAL_INSTANCES_PROPERTY->hidden = true;
		indigo_init_number_item(ADDITIONAL_INSTANCES_COUNT_ITEM, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, "Count", 0, MAX_ADDITIONAL_INSTANCES, 1, 0);
		pthread_mutex_init(&DEVICE_CONTEXT->config_mutex, NULL);
		pthread_mutex_init(&DEVICE_CONTEXT->device_mutex, NULL);
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	INDIGO_DEFINE_MATCHING_PROPERTY(INFO_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(SIMULATION_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(CONFIG_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(PROFILE_NAME_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(PROFILE_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(DEVICE_PORT_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(DEVICE_BAUDRATE_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(DEVICE_PORTS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUTHENTICATION_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(ADDITIONAL_INSTANCES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(CONNECTION_PROPERTY);
	return INDIGO_OK;
}

indigo_result indigo_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (CONNECTION_PROPERTY->state == INDIGO_ALERT_STATE) {
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		indigo_token token = indigo_get_device_token(device->name);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (token > 0) {
				device->access_token = token;
			} else {
				device->access_token = property->access_token;
			}
		} else {
			device->access_token = token;
		}
		char lost_port[PATH_MAX];
		if (serial_port_connection_changed(device, lost_port)) {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, "Connection to %s lost", lost_port);
		} else {
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		}
	} else if (indigo_property_match_changeable(SIMULATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SIMULATION
		indigo_property_copy_values(SIMULATION_PROPERTY, property, false);
		SIMULATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SIMULATION_PROPERTY, NULL);
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (CONFIG_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, CONFIG_PROPERTY, "Configuration restore is in progress");
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONFIG_PROPERTY, property, false);
		if (CONFIG_LOAD_ITEM->sw.value) {
			start_config_restore(device);
		} else if (CONFIG_SAVE_ITEM->sw.value) {
			indigo_save_property(device, NULL, SIMULATION_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_BAUDRATE_PROPERTY);
			if (device->base_device == NULL) {
				indigo_save_property(device, NULL, ADDITIONAL_INSTANCES_PROPERTY);
			}
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			CONFIG_SAVE_ITEM->sw.value = false;
			if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
				indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
				if (CONFIG_PROPERTY->count != 3) {
					indigo_delete_property(device, CONFIG_PROPERTY, NULL);
					CONFIG_PROPERTY->count = 3;
					indigo_define_property(device, CONFIG_PROPERTY, NULL);
				} else {
					indigo_update_property(device, CONFIG_PROPERTY, NULL);
				}
			} else {
				indigo_update_property(device, CONFIG_PROPERTY, NULL);
			}
		} else if (CONFIG_REMOVE_ITEM->sw.value) {
			if (indigo_remove_properties(device) == INDIGO_OK) {
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			CONFIG_REMOVE_ITEM->sw.value = false;
			indigo_delete_property(device, CONFIG_PROPERTY, NULL);
			CONFIG_PROPERTY->count = 1;
			indigo_define_property(device, CONFIG_PROPERTY, NULL);
		}
	} else if (indigo_property_match_changeable(PROFILE_NAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- PROFILE_NAME_PROPERTY
		indigo_property_copy_values(PROFILE_NAME_PROPERTY, property, false);
		indigo_delete_property(device, PROFILE_PROPERTY, NULL);
		for (int i = 0; i < PROFILE_COUNT; i++) {
			indigo_item *profile_item = PROFILE_ITEM + i;
			indigo_item *name_item = PROFILE_NAME_ITEM + i;
			if (strlen(name_item->text.value) == 0)
				sprintf(name_item->text.value, "Profile #%d", i);
			INDIGO_COPY_NAME(profile_item->label, name_item->text.value);
		}
		indigo_define_property(device, PROFILE_PROPERTY, NULL);
		if (strcmp(client->name, CONFIG_READER)) {
			indigo_save_property(device, NULL, PROFILE_NAME_PROPERTY);
			if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
				PROFILE_NAME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
			} else {
				PROFILE_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			PROFILE_NAME_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, PROFILE_NAME_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(PROFILE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- PROFILE
		indigo_property_copy_values(PROFILE_PROPERTY, property, false);
		for (int i = 0; i < PROFILE_PROPERTY->count; i++) {
			if (PROFILE_PROPERTY->items[i].sw.value) {
				if (test_config_file(device->name, 0, ".config")) {
					if (CONFIG_PROPERTY->count != 3) {
						indigo_delete_property(device, CONFIG_PROPERTY, NULL);
						CONFIG_PROPERTY->count = 3;
						indigo_define_property(device, CONFIG_PROPERTY, NULL);
					}
				} else {
					if (CONFIG_PROPERTY->count != 1) {
						indigo_delete_property(device, CONFIG_PROPERTY, NULL);
						CONFIG_PROPERTY->count = 1;
						indigo_define_property(device, CONFIG_PROPERTY, NULL);
					}
				}
				break;
			}
		}

		PROFILE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, PROFILE_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(DEVICE_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_PORT
		indigo_property_copy_values(DEVICE_PORT_PROPERTY, property, false);
		if (*DEVICE_PORT_ITEM->text.value == '/') {
			if (indigo_uni_is_readable(DEVICE_PORT_ITEM->text.value)) {
				DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
			} else {
				DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, "Serial port %s does not exists", DEVICE_PORT_ITEM->text.value);
			}
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		}
		device->matched_pattern_index = indigo_find_matching_pattern_index(device, DEVICE_PORT_ITEM->text.value);
	} else if (indigo_property_match_changeable(DEVICE_BAUDRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		indigo_property_copy_values(DEVICE_BAUDRATE_PROPERTY, property, false);
		if (*DEVICE_BAUDRATE_ITEM->text.value && strchr(DEVICE_BAUDRATE_ITEM->text.value, '-') == NULL) {
			strcat(DEVICE_BAUDRATE_ITEM->text.value, "-8N1");
		}
		DEVICE_BAUDRATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(DEVICE_PORTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		indigo_property_copy_values(DEVICE_PORTS_PROPERTY, property, false);
		if (DEVICE_PORTS_PROPERTY->items->sw.value) {
			indigo_delete_property(device, DEVICE_PORTS_PROPERTY, NULL);
			// a rescan that finds a change updates the other devices as well
			update_serial_ports(device);
			indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
			DEVICE_PORTS_PROPERTY->items->sw.value = false;
			indigo_define_property(device, DEVICE_PORTS_PROPERTY, NULL);
		} else {
			for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
				if (DEVICE_PORTS_PROPERTY->items[i].sw.value) {
					INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
					DEVICE_PORTS_PROPERTY->items[i].sw.value = false;
				}
			}
		}
		if (*DEVICE_PORT_ITEM->text.value == '/' && !indigo_uni_is_readable(DEVICE_PORT_ITEM->text.value)) {
			DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		}
		device->matched_pattern_index = indigo_find_matching_pattern_index(device, DEVICE_PORT_ITEM->text.value);
		indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		// a driver that set DEVICE_PORTS BUSY before passing the change on ends it itself, e.g. after its own discovery
		if (DEVICE_PORTS_PROPERTY->state != INDIGO_BUSY_STATE) {
			DEVICE_PORTS_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, DEVICE_PORTS_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		AUTHENTICATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUTHENTICATION_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
		assert(device->base_device == NULL && (device->master_device == NULL || device->master_device == device));
		indigo_device *slave_devices[MAX_SLAVE_DEVICES];
		int slave_count = indigo_query_slave_devices(device, slave_devices, MAX_SLAVE_DEVICES);
		int saved_count = (int)ADDITIONAL_INSTANCES_COUNT_ITEM->number.value;
		indigo_property_copy_values(ADDITIONAL_INSTANCES_PROPERTY, property, false);
		int count = (int)ADDITIONAL_INSTANCES_COUNT_ITEM->number.value;
		// remove all devices over 'count' -----------------------------------------
		for (int i = count; i < MAX_ADDITIONAL_INSTANCES; i++) {
			// make sure removed device is not connected
			indigo_device *additional_device = DEVICE_CONTEXT->additional_device_instances[i];
			if (additional_device != NULL) {
				if (((indigo_device_context *)additional_device->device_context)->connection_property->items->sw.value) {
					ADDITIONAL_INSTANCES_COUNT_ITEM->number.target = ADDITIONAL_INSTANCES_COUNT_ITEM->number.value = saved_count;
					ADDITIONAL_INSTANCES_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, ADDITIONAL_INSTANCES_PROPERTY, "Device %s is connected", additional_device->name);
					return INDIGO_OK;
				}
				// make sure slave devices of removed device are not connected
				for (int j = 0; j < slave_count; j++) {
					indigo_device *slave_device = slave_devices[j];
					indigo_device *additional_slave_device = ((indigo_device_context *)slave_device->device_context)->additional_device_instances[i];
					if (additional_slave_device != NULL && ((indigo_device_context *)additional_slave_device->device_context)->connection_property->items->sw.value) {
						ADDITIONAL_INSTANCES_COUNT_ITEM->number.target = ADDITIONAL_INSTANCES_COUNT_ITEM->number.value = saved_count;
						ADDITIONAL_INSTANCES_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, ADDITIONAL_INSTANCES_PROPERTY, "Device %s is connected", additional_slave_device->name);
						return INDIGO_OK;
					}
				}
			}
		}
		for (int i = count; i < MAX_ADDITIONAL_INSTANCES; i++) {
			// remove master device
			indigo_device *additional_device = DEVICE_CONTEXT->additional_device_instances[i];
			if (additional_device != NULL) {
				// remove slave devices first
				int slave_count = indigo_query_slave_devices(device, slave_devices, MAX_SLAVE_DEVICES);
				for (int j = 0; j < slave_count; j++) {
					indigo_device *slave_device = slave_devices[j];
					indigo_device *additional_slave_device = ((indigo_device_context *)slave_device->device_context)->additional_device_instances[i];
					if (additional_slave_device != NULL) {
						if (indigo_detach_device(additional_slave_device) != INDIGO_NOT_FOUND) {
							free(additional_slave_device);
						}
						((indigo_device_context *)slave_device->device_context)->additional_device_instances[i] = NULL;
					}
				}
				if (indigo_detach_device(additional_device) != INDIGO_NOT_FOUND) {
					free(additional_device->private_data);
					free(additional_device);
				}
				DEVICE_CONTEXT->additional_device_instances[i] = NULL;
			}
		}
		// all devices over 'count' removed -----------------------------------------
		// add devices up to 'count' -----------------------------------------
		for (int i = 0; i < count; i++) {
			if (DEVICE_CONTEXT->additional_device_instances[i] == NULL) {
				indigo_device *additional_device = indigo_safe_malloc_copy(sizeof(indigo_device), device);
				void *private_data = indigo_safe_malloc(MALLOCED_SIZE(device->private_data));
				snprintf(additional_device->name, INDIGO_NAME_SIZE, "%s #%d", device->name, i + 2);
				additional_device->lock = NULL;
				additional_device->is_remote = false;
				additional_device->gp_bits = 0;
				additional_device->last_result = 0;
				additional_device->access_token = 0;
				additional_device->device_context = NULL;
				additional_device->private_data = private_data;
				if (device->master_device == NULL) {
					additional_device->master_device = NULL;
				} else if (device->master_device == device) {
					additional_device->master_device = additional_device;
				}
				additional_device->base_device = device;
				indigo_attach_device(additional_device);
				DEVICE_CONTEXT->additional_device_instances[i] = additional_device;
				// add slaved devices
				for (int j = 0; j < slave_count; j++) {
					indigo_device *slave_device = slave_devices[j];
					indigo_device *additional_slave_device = indigo_safe_malloc_copy(sizeof(indigo_device), slave_device);
					snprintf(additional_slave_device->name, INDIGO_NAME_SIZE, "%s #%d", slave_device->name, i + 2);
					additional_slave_device->lock = NULL;
					additional_slave_device->is_remote = false;
					additional_slave_device->gp_bits = 0;
					additional_slave_device->last_result = 0;
					additional_slave_device->access_token = 0;
					additional_slave_device->device_context = NULL;
					additional_slave_device->private_data = private_data;
					additional_slave_device->master_device = additional_device;
					additional_device->base_device = slave_device;
					indigo_attach_device(additional_slave_device);
					((indigo_device_context *)slave_device->device_context)->additional_device_instances[i] = additional_slave_device;
				}
			}
		}
		ADDITIONAL_INSTANCES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ADDITIONAL_INSTANCES_PROPERTY, NULL);
		// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}

indigo_result indigo_device_detach(indigo_device *device) {
	assert(device != NULL);
	unregister_serial_ports_device(device);
	if (device->base_device == NULL) {
		for (int i = 0; i < MAX_ADDITIONAL_INSTANCES; i++) {
			indigo_device *additional_device = DEVICE_CONTEXT->additional_device_instances[i];
			if (additional_device != NULL) {
				if (indigo_detach_device(additional_device) != INDIGO_NOT_FOUND) {
					if (additional_device->master_device == NULL || additional_device->master_device == additional_device) {
						indigo_safe_free(additional_device->private_data);
					}
					indigo_safe_free(additional_device);
				}
				DEVICE_CONTEXT->additional_device_instances[i] = NULL;
			}
		}
	}
	indigo_cancel_background_handler(device, NULL);
	release_config_restore(device);
	indigo_queue_delete(&DEVICE_CONTEXT->queue);
	indigo_cancel_all_timers(device);
	indigo_release_property(CONNECTION_PROPERTY);
	indigo_release_property(INFO_PROPERTY);
	indigo_release_property(DEVICE_PORT_PROPERTY);
	indigo_release_property(DEVICE_BAUDRATE_PROPERTY);
	indigo_release_property(DEVICE_PORTS_PROPERTY);
	indigo_release_property(SIMULATION_PROPERTY);
	indigo_release_property(CONFIG_PROPERTY);
	indigo_release_property(PROFILE_NAME_PROPERTY);
	indigo_release_property(PROFILE_PROPERTY);
	indigo_release_property(AUTHENTICATION_PROPERTY);
	indigo_release_property(ADDITIONAL_INSTANCES_PROPERTY);
	pthread_mutex_destroy(&DEVICE_CONTEXT->config_mutex);
	pthread_mutex_destroy(&DEVICE_CONTEXT->device_mutex);
	free(DEVICE_CONTEXT);
	device->device_context = NULL;
	return INDIGO_OK;
}

void indigo_start_usb_event_handler() {
	static bool thread_started = false;
	if (!thread_started) {
		libusb_init(NULL);
		indigo_async(indigo_usb_hotplug_thread, NULL);
		thread_started = true;
	}
}

bool indigo_pin_library(const void *address) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	Dl_info info;
	if (!dladdr(address, &info) || !info.dli_fname) {
		INDIGO_ERROR(indigo_error("Can't locate the shared library to pin"));
		return false;
	}
	// RTLD_NODELETE marks the library as never unloaded; the extra reference is intentionally
	// leaked, because releasing it would restore exactly the unload this call exists to prevent.
	if (dlopen(info.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE) == NULL) {
		INDIGO_ERROR(indigo_error("Can't pin %s (%s)", info.dli_fname, dlerror()));
		return false;
	}
	INDIGO_DEBUG(indigo_debug("Pinned %s", info.dli_fname));
	return true;
#elif defined(INDIGO_WINDOWS)
	HMODULE module = NULL;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCSTR)address, &module)) {
		INDIGO_ERROR(indigo_error("Can't pin the shared library (%s)", indigo_last_windows_error()));
		return false;
	}
	return true;
#else
#pragma message ("TODO: indigo_pin_library()")
	return false;
#endif
}

void indigo_timetoisogm(time_t tstamp, char* isotime, int isotime_len) {
	struct tm tm_stamp;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	gmtime_r(&tstamp, &tm_stamp);
#elif defined(INDIGO_WINDOWS)
	gmtime_s(&tm_stamp, &tstamp);
#else
#pragma message ("TODO: indigo_timetoisogm()")
#endif
	strftime(isotime, isotime_len, "%Y-%m-%dT%H:%M:%S", &tm_stamp);
}

time_t indigo_isogmtotime(char* isotime) {
	struct tm tm_ts;
	memset(&tm_ts, 0, sizeof(tm_ts));
	if (sscanf(isotime, "%d-%d-%dT%d:%d:%d", &tm_ts.tm_year, &tm_ts.tm_mon, &tm_ts.tm_mday, &tm_ts.tm_hour, &tm_ts.tm_min, &tm_ts.tm_sec) == 6) {
		tm_ts.tm_mon -= 1;
		tm_ts.tm_year -= 1900;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		return timegm(&tm_ts);
#elif defined(INDIGO_WINDOWS)
		return _mkgmtime(&tm_ts);
#else
#pragma message ("TODO: indigo_isogmtotime()")
#endif
	}
	return -1;
}

void indigo_timetoisolocal(time_t tstamp, char* isotime, int isotime_len) {
	struct tm tm_stamp;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	localtime_r(&tstamp, &tm_stamp);
	tm_stamp.tm_zone = 0;
#elif defined(INDIGO_WINDOWS)
	localtime_s(&tm_stamp, &tstamp);
#else
#pragma message ("TODO: indigo_timetoisolocal()")
#endif
	tm_stamp.tm_isdst = 0;
	snprintf(isotime, isotime_len, "%04d-%02d-%02dT%02d:%02d:%02d", tm_stamp.tm_year + 1900, tm_stamp.tm_mon + 1, tm_stamp.tm_mday, tm_stamp.tm_hour, tm_stamp.tm_min, tm_stamp.tm_sec);
}

time_t indigo_isolocaltotime(char* isotime) {
	struct tm tm_stamp;
	memset(&tm_stamp, 0, sizeof(tm_stamp));
	if (sscanf(isotime, "%d-%d-%dT%d:%d:%d", &tm_stamp.tm_year, &tm_stamp.tm_mon, &tm_stamp.tm_mday, &tm_stamp.tm_hour, &tm_stamp.tm_min, &tm_stamp.tm_sec) == 6) {
		tm_stamp.tm_mon -= 1;
		tm_stamp.tm_year -= 1900;
		tm_stamp.tm_isdst = -1;
		return (mktime(&tm_stamp));
	}
	return -1;
}

long indigo_get_timezone() {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	tzset();
	return timezone;
#elif defined(INDIGO_WINDOWS)
	_tzset();
	TIME_ZONE_INFORMATION tzInfo;
	DWORD result = GetTimeZoneInformation(&tzInfo);
	return tzInfo.Bias * 60;
#endif
}

int indigo_get_utc_offset(void) {
	static int offset = 25;
	if (offset == 25) {
		time_t secs = time(NULL);
		struct tm tm;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		localtime_r(&secs, &tm);
#elif defined(INDIGO_WINDOWS)
		localtime_s(&tm, &secs);
#else
#pragma message ("TODO: indigo_get_utc_offset()")
#endif
		offset = (int)(-indigo_get_timezone() / 3600) + tm.tm_isdst;
	}
	return offset;
}

int indigo_get_dst_state(void) {
	static int dst = -1;
	if (dst == -1) {
		time_t secs = time(NULL);
		struct tm tm;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		localtime_r(&secs, &tm);
#elif defined(INDIGO_WINDOWS)
		localtime_s(&tm, &secs);
#else
#pragma message ("TODO: indigo_get_dst_state()")
#endif
		dst = tm.tm_isdst;
	}
	return dst;
}

// A port of plain auto:// is resolved when the device connects, so a client that sets it or a port plugged in
// without a hot-plug event does not leave the driver to open an empty path. Any other port is kept.
static void resolve_empty_auto_port(indigo_device *device) {
	if (DEVICE_PORT_PROPERTY == NULL || strcmp(DEVICE_PORT_ITEM->text.value, USBSERIAL_AUTO_PREFIX)) {
		return;
	}
	indigo_lock_bus();
	bool registered = false;
	for (int i = 0; i < serial_port_devices_count && !registered; i++) {
		registered = serial_port_devices[i] == device;
	}
	indigo_unlock_bus();
	if (!registered) {
		return;
	}
	// a rescan that finds a change updates the other devices as well
	update_serial_ports(device);
	indigo_lock_bus();
	select_auto_port(device, current_serial_ports());
	indigo_unlock_bus();
	if (strcmp(DEVICE_PORT_ITEM->text.value, USBSERIAL_AUTO_PREFIX)) {
		device->matched_pattern_index = indigo_find_matching_pattern_index(device, DEVICE_PORT_ITEM->text.value);
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
	} else {
		DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DEVICE_PORT_PROPERTY, "No matching serial port found");
	}
}

bool indigo_ignore_connection_change(indigo_device *device, indigo_property *request) {
	indigo_item *connected_item = NULL;
	indigo_item *disconnected_item = NULL;
	if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
		return true;
	}
	for (int i = 0; i < request->count; i++) {
		indigo_item *item = request->items + i;
		if (!strcmp(item->name, CONNECTION_CONNECTED_ITEM_NAME)) {
			connected_item = item;
		} else if (!strcmp(item->name, CONNECTION_DISCONNECTED_ITEM_NAME)) {
			disconnected_item = item;
		}
	}
	// if CONNECTED and DISCONNECTED are set
	if (connected_item != NULL && disconnected_item != NULL) {
		// ON & ON and OFF & OFF is not valid
		if (connected_item->sw.value == disconnected_item->sw.value) {
			return true;
		}
		/* REQUESTED == CURRENT is not a state change.
		   Checking DISCONNECTED is not needed, as we already made sure CONNECTED = !DISCONNECTED
		*/
		if (connected_item->sw.value == CONNECTION_CONNECTED_ITEM->sw.value) {
			return true;
		}
	}
	// if only CONNECTED is set
	if (connected_item != NULL && disconnected_item == NULL) {
		// CURRENT == REQUSTED is not a state change.
		if (CONNECTION_CONNECTED_ITEM->sw.value == connected_item->sw.value) {
			return true;
		}
		/* request CONNECTED = OFF is illegal:
		   1. If current CONNECTED = OFF -> not a state change.
		   2. If current CONNECTED = ON -> reults in invalid state CONNECTED = OFF and DISCONNECTED = OFF.
		*/
		if (connected_item->sw.value == false) {
			return true;
		}
	}
	// if only DISCONNECTED is set - analogous to CONNECTED
	if (connected_item == NULL && disconnected_item != NULL) {
		if (CONNECTION_DISCONNECTED_ITEM->sw.value == disconnected_item->sw.value) {
			return true;
		}
		if (disconnected_item->sw.value == false) {
			return true;
		}
	}
	/* reqquest is valid */
	if (connected_item != NULL && connected_item->sw.value) {
		resolve_empty_auto_port(device);
	}
	return false;
}

void indigo_lock_device(indigo_device* device) {
	pthread_mutex_lock(&DEVICE_CONTEXT->device_mutex);
}

void indigo_unlock_device(indigo_device* device) {
	pthread_mutex_unlock(&DEVICE_CONTEXT->device_mutex);
}

void indigo_lock_master_device(indigo_device* device) {
	if (device->master_device != NULL && device->master_device->device_context != NULL) {
		pthread_mutex_lock(&MASTER_DEVICE_CONTEXT->device_mutex);
	} else {
		pthread_mutex_lock(&DEVICE_CONTEXT->device_mutex);
	}
}

void indigo_unlock_master_device(indigo_device* device) {
	if (device->master_device != NULL && device->master_device->device_context != NULL) {
		pthread_mutex_unlock(&MASTER_DEVICE_CONTEXT->device_mutex);
	} else {
		pthread_mutex_unlock(&DEVICE_CONTEXT->device_mutex);
	}
}

void indigo_set_device_timer(indigo_device *device, double delay, indigo_timer_callback handler, indigo_timer **timer) {
	if (device->master_device != NULL && device->master_device->device_context != NULL) {
		indigo_set_timer_with_mutex(device, delay, handler, timer, &MASTER_DEVICE_CONTEXT->device_mutex);
	} else {
		indigo_set_timer_with_mutex(device, delay, handler, timer, &DEVICE_CONTEXT->device_mutex);
	}
}

void indigo_execute_handler_in(indigo_device *device, double delay, indigo_timer_callback handler) {
	indigo_device *element_device = device;
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (DEVICE_CONTEXT->queue == NULL) {
		DEVICE_CONTEXT->queue = indigo_queue_create(device);
	}
	indigo_queue_add(DEVICE_CONTEXT->queue, element_device, INDIGO_TASK_PRIORITY_TIME, delay, handler, &DEVICE_CONTEXT->device_mutex);
}

void indigo_execute_handler_with_data_in(indigo_device *device, double delay, indigo_timer_with_data_callback handler, void *data) {
	indigo_device *element_device = device;
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (DEVICE_CONTEXT->queue == NULL) {
		DEVICE_CONTEXT->queue = indigo_queue_create(device);
	}
	indigo_queue_add_with_data(DEVICE_CONTEXT->queue, element_device, INDIGO_TASK_PRIORITY_TIME, delay, handler, data, &DEVICE_CONTEXT->device_mutex);
}

void indigo_execute_priority_handler_in(indigo_device *device, int priority, double delay, indigo_timer_callback handler) {
	indigo_device *element_device = device;
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (DEVICE_CONTEXT->queue == NULL) {
		DEVICE_CONTEXT->queue = indigo_queue_create(device);
	}
	indigo_queue_add(DEVICE_CONTEXT->queue, element_device, priority, delay, handler, &DEVICE_CONTEXT->device_mutex);
}

void indigo_execute_priority_handler_with_data_in(indigo_device *device, int priority, double delay, indigo_timer_with_data_callback handler, void *data) {
	indigo_device *element_device = device;
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (DEVICE_CONTEXT->queue == NULL) {
		DEVICE_CONTEXT->queue = indigo_queue_create(device);
	}
	indigo_queue_add_with_data(DEVICE_CONTEXT->queue, element_device, priority, delay, handler, data, &DEVICE_CONTEXT->device_mutex);
}

void indigo_execute_priority_handler(indigo_device *device, int priority, indigo_timer_callback handler) {
	indigo_execute_priority_handler_in(device, priority, 0, handler);
}

void indigo_execute_priority_handler_with_data(indigo_device *device, int priority, indigo_timer_with_data_callback handler, void *data) {
	indigo_execute_priority_handler_with_data_in(device, priority, 0, handler, data);
}

void indigo_execute_handler(indigo_device *device, indigo_timer_callback handler) {
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_NORMAL, 0, handler);
}

void indigo_execute_handler_with_data(indigo_device *device, indigo_timer_with_data_callback handler, void *data) {
	indigo_execute_priority_handler_with_data_in(device, INDIGO_TASK_PRIORITY_NORMAL, 0, handler, data);
}

void indigo_cancel_pending_handlers(indigo_device *device) {
	indigo_device *element_device = device;
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	indigo_queue_remove(DEVICE_CONTEXT->queue, element_device, NULL);
}

void indigo_cancel_pending_handler(indigo_device *device, indigo_timer_callback callback) {
	indigo_device *element_device = device;
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	indigo_queue_remove(DEVICE_CONTEXT->queue, element_device, callback);
}
