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

/** INDIGO Driver base
 \file indigo_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(INDIGO_MACOS)
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#elif defined(INDIGO_LINUX)
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
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
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (indigo_is_sandboxed) {
		return INDIGO_OK;
	}
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (device->lock != NULL) {
		return INDIGO_FAILED;
	}
	char tmp_lock_file[255] = "/tmp/indigo_lock_";
	strncat(tmp_lock_file, device->name, 250);
	indigo_uni_handle *handle = indigo_uni_create_file(tmp_lock_file, INDIGO_LOG_DEBUG);
	if (handle == NULL) {
		return INDIGO_LOCK_ERROR;
	}
	if (!indigo_uni_lock_file(handle)) {
		indigo_uni_close(&handle);
		return INDIGO_LOCK_ERROR;
	}
	device->lock = handle;
	return INDIGO_OK;
#else
	return INDIGO_OK;
#endif
}

indigo_result indigo_global_unlock(indigo_device *device) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (indigo_is_sandboxed) {
		return INDIGO_OK;
	}
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (device->lock == NULL) {
		return INDIGO_FAILED;
	}
	indigo_uni_close(&device->lock);
	char tmp_lock_file[255] = "/tmp/indigo_lock_";
	strncat(tmp_lock_file, device->name, 250);
	indigo_uni_remove(tmp_lock_file);
#endif
	return INDIGO_OK;
}

#if defined(INDIGO_LINUX)
static int port_type(char *path) {
	int fd, res;
	struct serial_struct serinfo = { 0 };
	if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) {
		return -1;
	}
	int sp_type = 0;
	res = ioctl(fd, TIOCGSERIAL, &serinfo);
	if (res != 0) {
		sp_type = -1;
		INDIGO_TRACE(indigo_trace("%s(): path = %s, type = %d, res = %d error = '%s'", __FUNCTION__, path, serinfo.type, res, strerror(errno)));
	} else {
		sp_type = serinfo.type;
		INDIGO_TRACE(indigo_trace("%s(): path = %s, type = %d, res = %d", __FUNCTION__, path, serinfo.type, res));
	}
	close(fd);
	return sp_type;
}
#endif

#define MAX_DEVICE_PORTS	20

extern int indigo_server_tcp_port;
extern bool indigo_is_ephemeral_port;

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
					if (indigo_usbserial_match(serial_info + i, 1, patterns, patterns_count)) {
						INDIGO_DEBUG(indigo_debug( "%s(): Selected port for '%s' matches the pattern, keeping selected: %s", __FUNCTION__, device->name, DEVICE_PORT_ITEM->text.value));
						return true;
					}
				}
			}
		}
	}

	indigo_serial_info *matching = indigo_usbserial_match(serial_info, num_serial_info, patterns, patterns_count);

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

static bool cu_filter(const char *name) {
	return strncmp(name, "cu.", 3) == 0 && strstr(name, "debug") == NULL && strstr(name, "Bluetooth-Incoming-Port") == NULL;
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

indigo_result indigo_load_properties(indigo_device *device, bool default_properties) {
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
		indigo_client *client = indigo_safe_malloc(sizeof(indigo_client));
		strcpy(client->name, CONFIG_READER);
		indigo_adapter_context *context = indigo_safe_malloc(sizeof(indigo_adapter_context));
		context->input = &handle;
		client->client_context = context;
		client->version = INDIGO_VERSION_CURRENT;
		indigo_xml_parse(NULL, client);
		indigo_uni_close(&handle);
		indigo_safe_free(context);
		indigo_safe_free(client);
		result = INDIGO_OK;
	}
	handle = indigo_open_config_file(device->name, profile, false, default_properties ? ".default" : ".config");
	if (handle != NULL) {
		INDIGO_TRACE(indigo_trace("%d -> // Config file for '%s'", handle->index, device->name));
		indigo_client *client = indigo_safe_malloc(sizeof(indigo_client));
		strcpy(client->name, CONFIG_READER);
		indigo_adapter_context *context = indigo_safe_malloc(sizeof(indigo_adapter_context));
		context->input = &handle;
		client->client_context = context;
		client->version = INDIGO_VERSION_CURRENT;
		indigo_xml_parse(NULL, client);
		indigo_uni_close(&handle);
		indigo_safe_free(context);
		indigo_safe_free(client);
		result = INDIGO_OK;
	}
	if (DEVICE_CONTEXT) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	}
	return result;
}

indigo_result indigo_save_property(indigo_device *device, indigo_uni_handle **file_handle, indigo_property *property) {
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
				indigo_uni_printf(handle, "<newTextVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					indigo_uni_printf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, indigo_xml_escape(indigo_get_text_item_value(item)));
				}
				indigo_uni_printf(handle, "</newTextVector>\n");
				break;
			case INDIGO_NUMBER_VECTOR:
				indigo_uni_printf(handle, "<newNumberVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					indigo_uni_printf(handle, "<oneNumber name='%s'>%s</oneNumber>\n", item->name, indigo_dtoa(item->number.value, b1));
				}
				indigo_uni_printf(handle, "</newNumberVector>\n");
				break;
			case INDIGO_SWITCH_VECTOR:
				indigo_uni_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = &property->items[i];
					indigo_uni_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->sw.value ? "On" : "Off");
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
							indigo_uni_printf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, indigo_xml_escape(item->text.value));
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
							indigo_uni_printf(handle, "<oneNumber name='%s'>%s</oneNumber>\n", item->name, indigo_dtoa(item->number.value, b1));
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
							indigo_uni_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->sw.value ? "On" : "Off");
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

void indigo_enumerate_serial_ports(indigo_device *device, indigo_property *property) {
	assert(device != NULL);
	char label[INDIGO_VALUE_SIZE];

	int interface = atoi(INFO_DEVICE_INTERFACE_ITEM->text.value);
	if (interface & INDIGO_INTERFACE_AGENT) {
		INDIGO_DEBUG(indigo_debug("%s(): Skipping port enumeration for '%s'", __FUNCTION__, device->name));
		return;
	}
	DEVICE_PORTS_PROPERTY->count = 1;
	indigo_serial_info serial_info[MAX_DEVICE_PORTS] = {0};
	int serial_count = indigo_enumerate_usbserial_devices(serial_info, MAX_DEVICE_PORTS);
	for (int i = 0; i < serial_count; i++) {
		DEVICE_PORTS_PROPERTY->count++;
		indigo_usbserial_label(serial_info + i, label);
		indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + i + 1, serial_info[i].path, label, false);
		INDIGO_DEBUG(indigo_debug("%s(): Serial port #%d: %s %04X:%04X %s", __FUNCTION__, i, serial_info[i].path, serial_info[i].vendor_id, serial_info[i].product_id, label));
	}
#if defined(INDIGO_LINUX)
	char target[PATH_MAX];
	char path[PATH_MAX];
	char **list;
	int count = indigo_uni_scandir("/dev", &list, NULL);
	if (count >= 0) {
		for (int i = 0; i < count && DEVICE_PORTS_PROPERTY->count < MAX_DEVICE_PORTS; i++) {
			snprintf(path, INDIGO_VALUE_SIZE, "/dev/%s", list[i]);
			if (realpath(path, target)) {
				bool found = false;
				bool is_serial_link = false;
				for (int i = 0; i < serial_count; i++) {
					if (strcmp(serial_info[i].path, path) == 0) {
						found = true;
						break;
					} else if (strcmp(serial_info[i].path, target) == 0) {
						is_serial_link = true;
						break;
					}
				}
				if (!found) {
					if (is_serial_link) {
						int index = DEVICE_PORTS_PROPERTY->count++;
						snprintf(label, INDIGO_VALUE_SIZE, "%s (link to %s)", path, target);
						indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + index, path, label, false);
						INDIGO_DEBUG(indigo_debug("%s(): Serial port #%d: %s link = %s", __FUNCTION__, index, path, target));
					} else {
						int ser_type = port_type(path);
						if (ser_type > PORT_UNKNOWN) {
							int index = DEVICE_PORTS_PROPERTY->count++;
							indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + index, path, path, false);
							INDIGO_DEBUG(indigo_debug("%s(): Serial port #%d: %s type = %d", __FUNCTION__, index, path, ser_type));
						}
					}
				}
			}
			indigo_safe_free(list[i]);
		}
		indigo_safe_free(list);
	}
#elif defined(INDIGO_MACOS)
	char path[PATH_MAX];
	char **list;
	int count = indigo_uni_scandir("/dev", &list, cu_filter);
	if (count >= 0) {
		for (int i = 0; i < count && DEVICE_PORTS_PROPERTY->count < MAX_DEVICE_PORTS; i++) {
			snprintf(path, INDIGO_VALUE_SIZE, "/dev/%s", list[i]);
			bool found = false;
			for (int i = 0; i < serial_count; i++) {
				if (strcmp(serial_info[i].path, path) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				int index = DEVICE_PORTS_PROPERTY->count++;
				snprintf(label, INDIGO_VALUE_SIZE, "%s (Unknown)", path);
				indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + index, path, label, false);
				INDIGO_DEBUG(indigo_debug("%s(): Serial port #%d: %s", __FUNCTION__, index, path));
			}
		}
		indigo_safe_free(list);
	}
#endif
	/* if there are no USB-Serial ports but there are regular serial ports and we want auto selected port, select the first one.
	   Otherwise autoselect the matching port from the available USB-Serial ports.
	*/
	if (device->match_patterns_count == 0 && DEVICE_PORTS_PROPERTY->count > 1 && serial_count == 0 && (DEVICE_PORT_ITEM->text.value[0] == '\0' || !strncmp(DEVICE_PORT_ITEM->text.value, USBSERIAL_AUTO_PREFIX, strlen(USBSERIAL_AUTO_PREFIX)))) {
		snprintf(DEVICE_PORT_ITEM->text.value, INDIGO_VALUE_SIZE, "%s%s", USBSERIAL_AUTO_PREFIX, DEVICE_PORTS_PROPERTY->items[1].name);
		INDIGO_DEBUG(indigo_debug("%s(): No USB-Serial ports found, selecting first tty port: %s", __FUNCTION__, DEVICE_PORT_ITEM->text.value));
	} else {
		indigo_select_matching_usbserial_device(device, serial_info, serial_count);
	}
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

indigo_result indigo_device_attach(indigo_device *device, const char* driver_name, indigo_version version, int interface) {
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
		// -------------------------------------------------------------------------------- DEVICE_INFO
		INFO_PROPERTY = indigo_init_text_property(NULL, device->name, INFO_PROPERTY_NAME, MAIN_GROUP, "Device info", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(INFO_DEVICE_NAME_ITEM, INFO_DEVICE_NAME_ITEM_NAME, "Device name", device->name);
		indigo_init_text_item(INFO_DEVICE_DRIVER_ITEM, INFO_DEVICE_DRIVER_ITEM_NAME, "Driver name", "%s", driver_name);
		unsigned indigo_version = (version >> 16) & 0xFFFF;
		if (indigo_version == 0) {
			indigo_version = 0x0200;
		}
		indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, INFO_DEVICE_VERSION_ITEM_NAME, "Driver version", "%d.%d.%d.%d", INDIGO_VERSION_MAJOR(indigo_version), INDIGO_VERSION_MINOR(indigo_version), INDIGO_VERSION_MAJOR(version), INDIGO_VERSION_MINOR(version));
		indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, INFO_DEVICE_INTERFACE_ITEM_NAME, "Interface", "%u", interface);
		indigo_init_text_item(INFO_DEVICE_MODEL_ITEM, INFO_DEVICE_MODEL_ITEM_NAME, "Model", device->name);
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
		pthread_mutex_init(&DEVICE_CONTEXT->multi_device_mutex, NULL);
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (indigo_property_match(INFO_PROPERTY, property) && !INFO_PROPERTY->hidden)
		indigo_define_property(device, INFO_PROPERTY, NULL);
	if (indigo_property_match(SIMULATION_PROPERTY, property) && !SIMULATION_PROPERTY->hidden)
		indigo_define_property(device, SIMULATION_PROPERTY, NULL);
	if (indigo_property_match(CONFIG_PROPERTY, property) && !CONFIG_PROPERTY->hidden)
		indigo_define_property(device, CONFIG_PROPERTY, NULL);
	if (indigo_property_match(PROFILE_NAME_PROPERTY, property) && !PROFILE_PROPERTY->hidden)
		indigo_define_property(device, PROFILE_NAME_PROPERTY, NULL);
	if (indigo_property_match(PROFILE_PROPERTY, property) && !PROFILE_PROPERTY->hidden)
		indigo_define_property(device, PROFILE_PROPERTY, NULL);
	if (indigo_property_match(DEVICE_PORT_PROPERTY, property) && !DEVICE_PORT_PROPERTY->hidden)
		indigo_define_property(device, DEVICE_PORT_PROPERTY, NULL);
	if (indigo_property_match(DEVICE_BAUDRATE_PROPERTY, property) && !DEVICE_BAUDRATE_PROPERTY->hidden)
		indigo_define_property(device, DEVICE_BAUDRATE_PROPERTY, NULL);
	if (indigo_property_match(DEVICE_PORTS_PROPERTY, property) && !DEVICE_PORTS_PROPERTY->hidden)
		indigo_define_property(device, DEVICE_PORTS_PROPERTY, NULL);
	if (indigo_property_match(AUTHENTICATION_PROPERTY, property) && !AUTHENTICATION_PROPERTY->hidden)
		indigo_define_property(device, AUTHENTICATION_PROPERTY, NULL);
	if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property) && !ADDITIONAL_INSTANCES_PROPERTY->hidden)
		indigo_define_property(device, ADDITIONAL_INSTANCES_PROPERTY, NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property) && !CONNECTION_PROPERTY->hidden)
		indigo_define_property(device, CONNECTION_PROPERTY, NULL);
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
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(SIMULATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SIMULATION
		indigo_property_copy_values(SIMULATION_PROPERTY, property, false);
		SIMULATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SIMULATION_PROPERTY, NULL);
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_LOAD_ITEM, property)) {
			if (CONFIG_PROPERTY->count == 1 || indigo_load_properties(device, false) == INDIGO_OK) {
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			CONFIG_LOAD_ITEM->sw.value = false;
			indigo_update_property(device, CONFIG_PROPERTY, NULL);
		} else if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, SIMULATION_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_BAUDRATE_PROPERTY);
			if (DEVICE_CONTEXT->base_device == NULL) {
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
		} else if (indigo_switch_match(CONFIG_REMOVE_ITEM, property)) {
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
				//indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
				indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
			} else {
				DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, "Serial port %s does not exists", DEVICE_PORT_ITEM->text.value);
			}
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			//indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		}
	} else if (indigo_property_match_changeable(DEVICE_BAUDRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		indigo_property_copy_values(DEVICE_BAUDRATE_PROPERTY, property, false);
		if (*DEVICE_BAUDRATE_ITEM->text.value && strchr(DEVICE_BAUDRATE_ITEM->text.value, '-') == NULL) {
			strcat(DEVICE_BAUDRATE_ITEM->text.value, "-8N1");
		}
		DEVICE_BAUDRATE_PROPERTY->state = INDIGO_OK_STATE;
		//indigo_save_property(device, NULL, DEVICE_BAUDRATE_PROPERTY);
		indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(DEVICE_PORTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		indigo_property_copy_values(DEVICE_PORTS_PROPERTY, property, false);
		if (DEVICE_PORTS_PROPERTY->items->sw.value) {
			indigo_delete_property(device, DEVICE_PORTS_PROPERTY, NULL);
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
			//indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
		}
		indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		DEVICE_PORTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_PORTS_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		AUTHENTICATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUTHENTICATION_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
		assert(DEVICE_CONTEXT->base_device == NULL && (device->master_device == NULL || device->master_device == device));
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
				indigo_attach_device(additional_device);
				((indigo_device_context *)additional_device->device_context)->base_device = device;
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
					indigo_attach_device(additional_slave_device);
					((indigo_device_context *)additional_device->device_context)->base_device = slave_device;
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
	if (DEVICE_CONTEXT->base_device == NULL) {
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
	pthread_mutex_destroy(&DEVICE_CONTEXT->multi_device_mutex);
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
	return false;
}

void indigo_lock_master_device(indigo_device* device) {
	if (device != NULL && device->master_device != NULL && device->master_device->device_context != NULL) {
		pthread_mutex_lock(&MASTER_DEVICE_CONTEXT->multi_device_mutex);
	}
}

void indigo_unlock_master_device(indigo_device* device) {
	if (device != NULL && device->master_device != NULL && device->master_device->device_context != NULL) {
		pthread_mutex_unlock(&MASTER_DEVICE_CONTEXT->multi_device_mutex);
	}
}
