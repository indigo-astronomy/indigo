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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Driver base
 \file indigo_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#elif defined(INDIGO_LINUX)
#include <libusb-1.0/libusb.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <dirent.h>
#else
#include <libusb-1.0/libusb.h>
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

#include "indigo_driver.h"
#include "indigo_xml.h"
#include "indigo_names.h"
#include "indigo_io.h"

#if defined(INDIGO_LINUX)
bool is_serial(char *path) {
	int fd;
	struct serial_struct serinfo;

	if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) return false;

	bool is_sp = false;
	if ((ioctl(fd, TIOCGSERIAL, &serinfo) == 0) && (serinfo.type != PORT_UNKNOWN)) is_sp = true;

	close(fd);
	return is_sp;
}
#endif

indigo_result indigo_device_attach(indigo_device *device, indigo_version version, int interface) {
	assert(device != NULL);
	assert(device != NULL);
	if (DEVICE_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_device_context));
		assert(DEVICE_CONTEXT != NULL);
		memset(device->device_context, 0, sizeof(indigo_device_context));
	}
	if (DEVICE_CONTEXT != NULL) {
		// -------------------------------------------------------------------------------- CONNECTION
		CONNECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, CONNECTION_PROPERTY_NAME, MAIN_GROUP, "Connection status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (CONNECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONNECTION_CONNECTED_ITEM, CONNECTION_CONNECTED_ITEM_NAME, "Connected", false);
		indigo_init_switch_item(CONNECTION_DISCONNECTED_ITEM, CONNECTION_DISCONNECTED_ITEM_NAME, "Disconnected", true);
		// -------------------------------------------------------------------------------- DEVICE_INFO
		INFO_PROPERTY = indigo_init_text_property(NULL, device->name, INFO_PROPERTY_NAME, MAIN_GROUP, "Device info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 7);
		if (INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(INFO_DEVICE_NAME_ITEM, INFO_DEVICE_NAME_ITEM_NAME, "Device name", device->name);
		indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, INFO_DEVICE_VERSION_ITEM_NAME, "Driver version", "%d.%d.%d.%d", INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MAJOR(version), INDIGO_VERSION_MINOR(version));
		indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, INFO_DEVICE_INTERFACE_ITEM_NAME, "Interface", "%d", interface);
		indigo_init_text_item(INFO_DEVICE_MODEL_ITEM, INFO_DEVICE_MODEL_ITEM_NAME, "Model", device->name);
		indigo_init_text_item(INFO_DEVICE_FW_REVISION_ITEM, INFO_DEVICE_FW_REVISION_ITEM_NAME, "Firmware Rev.", "N/A");
		indigo_init_text_item(INFO_DEVICE_HW_REVISION_ITEM, INFO_DEVICE_HW_REVISION_ITEM_NAME, "Hardware Rev.", "N/A");
		indigo_init_text_item(INFO_DEVICE_SERIAL_NUM_ITEM, INFO_DEVICE_SERIAL_NUM_ITEM_NAME, "Serial No.", "N/A");
		/* Decrease count as other items are rare if you need them just set count to 7 in the dirver */
		INFO_PROPERTY->count = 3;
		// -------------------------------------------------------------------------------- DEBUG
		DEBUG_PROPERTY = indigo_init_switch_property(NULL, device->name, DEBUG_PROPERTY_NAME, MAIN_GROUP, "Debug status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DEBUG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(DEBUG_ENABLED_ITEM, DEBUG_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(DEBUG_DISABLED_ITEM, DEBUG_DISABLED_ITEM_NAME, "Disabled", true);
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY = indigo_init_switch_property(NULL, device->name, SIMULATION_PROPERTY_NAME, MAIN_GROUP, "Simulation status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (SIMULATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		SIMULATION_PROPERTY->hidden = true;
		indigo_init_switch_item(SIMULATION_ENABLED_ITEM, SIMULATION_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(SIMULATION_DISABLED_ITEM, SIMULATION_DISABLED_ITEM_NAME, "Disabled", true);
		// -------------------------------------------------------------------------------- CONFIG
		CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, CONFIG_PROPERTY_NAME, MAIN_GROUP, "Configuration control", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONFIG_LOAD_ITEM, CONFIG_LOAD_ITEM_NAME, "Load", false);
		indigo_init_switch_item(CONFIG_SAVE_ITEM, CONFIG_SAVE_ITEM_NAME, "Save", false);
		indigo_init_switch_item(CONFIG_DEFAULT_ITEM, CONFIG_DEFAULT_ITEM_NAME, "Default", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY = indigo_init_text_property(NULL, device->name, DEVICE_PORT_PROPERTY_NAME, MAIN_GROUP, "Serial port", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
		if (DEVICE_PORT_PROPERTY == NULL)
			return INDIGO_FAILED;
		DEVICE_PORT_PROPERTY->hidden = true;
		indigo_init_text_item(DEVICE_PORT_ITEM, DEVICE_PORT_ITEM_NAME, "Device name or URL", DEFAULT_TTY);
		// -------------------------------------------------------------------------------- DEVICE_PORTS
#define MAX_DEVICE_PORTS	20
		DEVICE_PORTS_PROPERTY = indigo_init_switch_property(NULL, device->name, DEVICE_PORTS_PROPERTY_NAME, MAIN_GROUP, "Serial ports", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICE_PORTS);
		if (DEVICE_PORTS_PROPERTY == NULL)
			return INDIGO_FAILED;
		DEVICE_PORTS_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->count = 0;
		char name[INDIGO_VALUE_SIZE];
#if defined(INDIGO_MACOS)
		io_iterator_t iterator;
		io_object_t serial_device;
		CFMutableDictionaryRef matching_dict = IOServiceMatching(kIOSerialBSDServiceValue);
		CFDictionarySetValue(matching_dict, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
		kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iterator);
		if (kr == 0) {
			while ((serial_device = IOIteratorNext(iterator)) && DEVICE_PORTS_PROPERTY->count < MAX_DEVICE_PORTS) {
				CFTypeRef cfs = IORegistryEntryCreateCFProperty (serial_device, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault,0);
				if (cfs) {
					CFStringGetCString(cfs, name, INDIGO_VALUE_SIZE, kCFStringEncodingASCII);
					if (strcmp(name, "/dev/cu.Bluetooth-Incoming-Port")) {
						int i = DEVICE_PORTS_PROPERTY->count++;
						indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + i, name, name, false);
						if (i == 0)
							strcpy(DEVICE_PORT_ITEM->text.value, name);
					}
					CFRelease(cfs);
				}
				IOObjectRelease(serial_device);
			}
			IOObjectRelease(iterator);
		}
#elif defined(INDIGO_LINUX)
		DIR *dir = opendir ("/dev");
		struct dirent *entry;
		while ((entry = readdir (dir)) != NULL && DEVICE_PORTS_PROPERTY->count < MAX_DEVICE_PORTS) {
			if (strncmp(entry->d_name, "tty", 3)) continue;
			snprintf(name, INDIGO_VALUE_SIZE, "/dev/%s", entry->d_name);
			if (is_serial(name)) {
				int i = DEVICE_PORTS_PROPERTY->count++;
				indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + i, name, name, false);
				if (i == 0)
					strcpy(DEVICE_PORT_ITEM->text.value, name);
			}
		}
		closedir(dir);
#else
    /* freebsd */
#endif

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property) && !CONNECTION_PROPERTY->hidden)
		indigo_define_property(device, CONNECTION_PROPERTY, NULL);
	if (indigo_property_match(INFO_PROPERTY, property) && !INFO_PROPERTY->hidden)
		indigo_define_property(device, INFO_PROPERTY, NULL);
	if (indigo_property_match(DEBUG_PROPERTY, property) && !DEBUG_PROPERTY->hidden)
		indigo_define_property(device, DEBUG_PROPERTY, NULL);
	if (indigo_property_match(SIMULATION_PROPERTY, property) && !SIMULATION_PROPERTY->hidden)
		indigo_define_property(device, SIMULATION_PROPERTY, NULL);
	if (indigo_property_match(CONFIG_PROPERTY, property) && !CONFIG_PROPERTY->hidden)
		indigo_define_property(device, CONFIG_PROPERTY, NULL);
	if (indigo_property_match(DEVICE_PORT_PROPERTY, property) && !DEVICE_PORT_PROPERTY->hidden)
		indigo_define_property(device, DEVICE_PORT_PROPERTY, NULL);
	if (indigo_property_match(DEVICE_PORTS_PROPERTY, property) && !DEVICE_PORTS_PROPERTY->hidden)
		indigo_define_property(device, DEVICE_PORTS_PROPERTY, NULL);
	return INDIGO_OK;
}

indigo_result indigo_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
	} else if (indigo_property_match(DEBUG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEBUG
		indigo_property_copy_values(DEBUG_PROPERTY, property, false);
		DEBUG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_log_level = indigo_debug_level = DEBUG_ENABLED_ITEM->sw.value;
		indigo_update_property(device, DEBUG_PROPERTY, NULL);
	} else if (indigo_property_match(SIMULATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SIMULATION
		indigo_property_copy_values(SIMULATION_PROPERTY, property, false);
		SIMULATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SIMULATION_PROPERTY, NULL);
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_LOAD_ITEM, property)) {
			if (indigo_load_properties(device, false) == INDIGO_OK)
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			else
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			CONFIG_LOAD_ITEM->sw.value = false;
		} else if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, DEBUG_PROPERTY);
			indigo_save_property(device, NULL, SIMULATION_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
			if (DEVICE_CONTEXT->property_save_file_handle) {
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
				close(DEVICE_CONTEXT->property_save_file_handle);
			} else {
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			CONFIG_SAVE_ITEM->sw.value = false;
		} else if (indigo_switch_match(CONFIG_DEFAULT_ITEM, property)) {
			if (indigo_load_properties(device, true) == INDIGO_OK)
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			else
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			CONFIG_DEFAULT_ITEM->sw.value = false;
		}
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	} else if (indigo_property_match(DEVICE_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_PORT
		indigo_property_copy_values(DEVICE_PORT_PROPERTY, property, false);
		if (*DEVICE_PORT_ITEM->text.value == '/') {
			if (!access(DEVICE_PORT_ITEM->text.value, R_OK)) {
				DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
			} else {
				DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, "%s does not exists", DEVICE_PORT_ITEM->text.value);
			}
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		}
	} else if (indigo_property_match(DEVICE_PORTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		indigo_property_copy_values(DEVICE_PORTS_PROPERTY, property, false);
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (DEVICE_PORTS_PROPERTY->items[i].sw.value) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
				DEVICE_PORTS_PROPERTY->items[i].sw.value = false;
			}
		}
		DEVICE_PORTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_PORTS_PROPERTY, NULL);
		// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}

indigo_result indigo_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_cancel_all_timers(device);
	indigo_release_property(CONNECTION_PROPERTY);
	indigo_release_property(INFO_PROPERTY);
	indigo_release_property(DEVICE_PORT_PROPERTY);
	indigo_release_property(DEVICE_PORTS_PROPERTY);
	indigo_release_property(DEBUG_PROPERTY);
	indigo_release_property(SIMULATION_PROPERTY);
	indigo_release_property(CONFIG_PROPERTY);
	indigo_property *all_properties = indigo_init_text_property(NULL, device->name, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
	indigo_delete_property(device, all_properties, NULL);
	indigo_release_property(all_properties);
	free(DEVICE_CONTEXT);
	return INDIGO_OK;
}

extern int indigo_server_tcp_port;
extern bool indigo_is_ephemeral_port;

int indigo_open_config_file(char *device_name, int mode, const char *suffix) {
	static char path[128];
	int path_end = snprintf(path, sizeof(path), "%s/.indigo", getenv("HOME"));
	int handle = mkdir(path, 0777);
	if (handle == 0 || errno == EEXIST) {
		if (indigo_server_tcp_port == 7624 || indigo_is_ephemeral_port)
			snprintf(path + path_end, sizeof(path) - path_end, "/%s%s", device_name, suffix);
		else
			snprintf(path + path_end, sizeof(path) - path_end, "/%s_%d%s", device_name, indigo_server_tcp_port, suffix);
		char *space = strchr(path, ' ');
		while (space != NULL) {
			*space = '_';
			space = strchr(space+1, ' ');
		}
		handle = open(path, mode, 0644);
		if (handle < 0)
			INDIGO_DEBUG(indigo_debug("Can't create %s (%s)", path, strerror(errno)));
	} else {
		INDIGO_DEBUG(indigo_debug("Can't create %s (%s)", path, strerror(errno)));
	}
	return handle;
}

indigo_result indigo_load_properties(indigo_device *device, bool default_properties) {
	assert(device != NULL);
	int handle = indigo_open_config_file(device->name, O_RDONLY, default_properties ? ".default" : ".config");
	if (handle > 0) {
		indigo_client *client = malloc(sizeof(indigo_client));
		memset(client, 0, sizeof(indigo_client));
		indigo_adapter_context *context = malloc(sizeof(indigo_adapter_context));
		context->input = handle;
		client->client_context = context;
		client->version = INDIGO_VERSION_CURRENT;
		indigo_xml_parse(NULL, client);
		close(handle);
		free(context);
		free(client);
	}
	return handle > 0 ? INDIGO_OK : INDIGO_FAILED;
}

indigo_result indigo_save_property(indigo_device*device, int *file_handle, indigo_property *property) {
	if (!property->hidden && property->perm != INDIGO_RO_PERM) {
		if (file_handle == NULL)
			file_handle = &DEVICE_CONTEXT->property_save_file_handle;
		int handle = *file_handle;
		if (handle == 0) {
			*file_handle = handle = indigo_open_config_file(property->device, O_WRONLY | O_CREAT | O_TRUNC, ".config");
			if (handle == 0)
				return INDIGO_FAILED;
		}
		switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			indigo_printf(handle, "<newTextVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_printf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, indigo_xml_escape(item->text.value));
			}
			indigo_printf(handle, "</newTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			indigo_printf(handle, "<newNumberVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_printf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", item->name, item->number.value);
			}
			indigo_printf(handle, "</newNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			indigo_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->sw.value ? "On" : "Off");
			}
			indigo_printf(handle, "</newSwitchVector>\n");
			break;
		default:
			break;
		}
	}
	return INDIGO_OK;
}

static void *hotplug_thread(void *arg) {
	while (true) {
		libusb_handle_events(NULL);
	}
	return NULL;
}

void indigo_start_usb_event_handler() {
	static bool thread_started = false;
	if (!thread_started) {
		libusb_init(NULL);
		pthread_t hotplug_thread_handle;
		pthread_create(&hotplug_thread_handle, NULL, hotplug_thread, NULL);
		thread_started = true;
	}
}

void indigo_async(void *fun(void *data), void *data) {
	pthread_t async_thread;
	pthread_create(&async_thread, NULL, fun, data);
}

double indigo_stod(char *string) {
	char copy[128];
	strncpy(copy, string, 128);
	string = copy;
	double value = 0;
	char *separator = strpbrk(string, ":*' ");
	if (separator == NULL) {
		value = atof(string);
	} else {
		*separator++ = 0;
		value = atof(string);
		separator = strpbrk(string = separator, ":*' ");
		if (separator == NULL) {
			value += atof(string)/60.0;
		} else {
			*separator++ = 0;
			value += atof(string)/60.0 + atof(separator)/3600.0;
		}
	}
	return value;
}

char* indigo_dtos(double value, char *format) {
	double d = fabs(value);
	double m = 60.0 * (d - floor(d));
	double s = 60.0 * (m - floor(m));
	if (value < 0)
		d = -d;
	static char string[128];
	if (format == NULL)
		snprintf(string, 128, "%d:%02d:%05.2f", (int)d, (int)m, s);
	else
		snprintf(string, 128, format, (int)d, (int)m, s);
	return string;
}

time_t indigo_utc(time_t *ltime) {
	struct tm tm_now;
	time_t now;

	if (ltime != NULL) now = *ltime;
	else time(&now);

	gmtime_r(&now, &tm_now);
	return mktime(&tm_now);
}

void indigo_timetoiso(time_t tstamp, char *isotime, int isotime_len) {
	struct tm tm_stamp;

	localtime_r(&tstamp, &tm_stamp);
	strftime(isotime, isotime_len, "%Y-%m-%dT%H:%M:%S", &tm_stamp);
}

time_t indigo_isototime(char *isotime) {
	struct tm tm_ts;

	memset(&tm_ts, 0, sizeof(tm_ts));
	if (sscanf(isotime, "%d-%d-%dT%d:%d:%d", &tm_ts.tm_year, &tm_ts.tm_mon, &tm_ts.tm_mday, &tm_ts.tm_hour, &tm_ts.tm_min, &tm_ts.tm_sec) == 6) {
		tm_ts.tm_mon -= 1;             /* mon is 0..11 */
		tm_ts.tm_year -= 1900;         /* year since 1900 */
		return (mktime(&tm_ts));
	}

	return -1;
}
