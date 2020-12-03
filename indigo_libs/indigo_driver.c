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

#include <indigo/indigo_driver.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_io.h>

indigo_result indigo_try_global_lock(indigo_device *device) {
	if (indigo_is_sandboxed)
		return INDIGO_OK;
	char tmp_lock_file[255] = "/tmp/indigo_lock_";
	if (device->master_device) {
		if (device->master_device->lock > 0) return INDIGO_FAILED;
		strncat(tmp_lock_file, device->master_device->name, 250);
	} else {
		if (device->lock > 0) return INDIGO_FAILED;
		strncat(tmp_lock_file, device->name, 250);
	}
	int fd = open(tmp_lock_file, O_CREAT | O_WRONLY, 0600);
	if (fd == -1) {
		return -1;
	}

	static struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	lock.l_pid = getpid();

	int ret = fcntl(fd, F_SETLK, &lock);
	if (ret == -1) {
		int local_errno = errno;
		close(fd);
		fd = -1;
		errno = local_errno;
	}
	if (device->master_device) device->master_device->lock = fd;
	else device->lock = fd;
	if (fd > 0) return INDIGO_OK;
	return INDIGO_LOCK_ERROR;
}


indigo_result indigo_global_unlock(indigo_device *device) {
	if (indigo_is_sandboxed)
		return INDIGO_OK;
	char tmp_lock_file[255] = "/tmp/indigo_lock_";
	if (device->master_device) {
		if (device->master_device->lock <= 0) return INDIGO_FAILED;
		close(device->master_device->lock);
		device->master_device->lock = -1;
		strncat(tmp_lock_file, device->master_device->name, 250);
	} else {
		if (device->lock <= 0) return INDIGO_FAILED;
		close(device->lock);
		device->lock = -1;
		strncat(tmp_lock_file, device->name, 250);
	}
	unlink(tmp_lock_file);
	return INDIGO_OK;
}


#if defined(INDIGO_LINUX)
static int port_type(char *path) {
	int fd, res;
	struct serial_struct serinfo = {0};

	if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) return -1;

	int sp_type = 0;
	res = ioctl(fd, TIOCGSERIAL, &serinfo);
	if (res != 0) {
		sp_type = -1;
		INDIGO_DEBUG(indigo_debug("%s(): path = %s, type = %d, res = %d error = '%s'", __FUNCTION__, path, serinfo.type, res, strerror(errno)));
	} else {
		sp_type = serinfo.type;
		INDIGO_DEBUG(indigo_debug("%s(): path = %s, type = %d, res = %d", __FUNCTION__, path, serinfo.type, res));
	}
	close(fd);
	return sp_type;
}
#endif

#define MAX_DEVICE_PORTS	20

void indigo_enumerate_serial_ports(indigo_device *device, indigo_property *property) {
	property->count = 1;
	char name[PATH_MAX];
#if defined(INDIGO_MACOS)
	io_iterator_t iterator;
	io_object_t serial_device;
	CFMutableDictionaryRef matching_dict = IOServiceMatching(kIOSerialBSDServiceValue);
	CFDictionarySetValue(matching_dict, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
	kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iterator);
	if (kr == 0) {
		while ((serial_device = IOIteratorNext(iterator)) && property->count < MAX_DEVICE_PORTS) {
			CFTypeRef cfs = IORegistryEntryCreateCFProperty (serial_device, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault,0);
			if (cfs) {
				CFStringGetCString(cfs, name, INDIGO_VALUE_SIZE, kCFStringEncodingASCII);
				if (strcmp(name, "/dev/cu.Bluetooth-Incoming-Port") && strcmp(name, "/dev/cu.SSDC") && strstr(name, "-WirelessiAP") == NULL) {
					int i = property->count++;
					indigo_init_switch_item(property->items + i, name, name, false);
				}
				CFRelease(cfs);
			}
			IOObjectRelease(serial_device);
		}
		IOObjectRelease(iterator);
	}
#elif defined(INDIGO_LINUX)
	DIR *dir;
	char target[PATH_MAX];
	char serial_links[MAX_DEVICE_PORTS][PATH_MAX]={0};
	struct dirent *entry;
	int link_num = 0;
	/* Some serial devices seem to report PORT_UNKNOWN but they have simlinks
	   in /dev/serial/by-id/ in that case we consider them real ports
	*/
	dir = opendir("/dev/serial/by-id");
	if (dir) {
		while ((entry = readdir(dir)) != NULL && DEVICE_PORTS_PROPERTY->count < MAX_DEVICE_PORTS) {
			if (entry->d_name[0] != '.') {
				snprintf(name, PATH_MAX, "/dev/serial/by-id/%s", entry->d_name);
				if (realpath(name, target)) {
						strncpy(serial_links[link_num], target, PATH_MAX);
						link_num++;
				}
			}
		}
		closedir(dir);
	}

	dir = opendir("/dev");
	while ((entry = readdir(dir)) != NULL && DEVICE_PORTS_PROPERTY->count < MAX_DEVICE_PORTS) {
		snprintf(name, INDIGO_VALUE_SIZE, "/dev/%s", entry->d_name);
		if (!realpath(name, target)) continue;
		if (!strstr(target, "/tty")) continue;
		int ser_type = port_type(name);
		bool is_serial = false;
		/* port is unknown -> will be considerd serial port if there is a link in /dev/serial/by-id */
		if (ser_type <= PORT_UNKNOWN) {
			for (int i = 0; i < link_num; i++) {
				if (!strncmp(target, serial_links[i], PATH_MAX)) {
					is_serial = true;
					INDIGO_DEBUG(indigo_debug("%s(): path = %s, IS SERIAL (has link)", __FUNCTION__, name));
					break;
				}
				INDIGO_DEBUG(indigo_debug("%s(): target = %s link = %s,", __FUNCTION__, name, target, serial_links[i]));
			}
		} else {
			is_serial = true;
			INDIGO_DEBUG(indigo_debug("%s(): path = %s, IS SERIAL (type = %d)", __FUNCTION__, name, ser_type));
		}
		if (is_serial) {
			int i = DEVICE_PORTS_PROPERTY->count++;
			indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + i, name, name, false);
			if (i == 0)
				strncpy(DEVICE_PORT_ITEM->text.value, name, INDIGO_VALUE_SIZE);
		}
	}
	closedir(dir);
#else
	/* freebsd */
#endif
}

indigo_result indigo_device_attach(indigo_device *device, const char* driver_name, indigo_version version, int interface) {
	assert(device != NULL);
	assert(device != NULL);
	if (DEVICE_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_device_context));
		assert(DEVICE_CONTEXT != NULL);
		memset(device->device_context, 0, sizeof(indigo_device_context));
	}
	if (DEVICE_CONTEXT != NULL) {
		// -------------------------------------------------------------------------------- CONNECTION
		CONNECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, CONNECTION_PROPERTY_NAME, MAIN_GROUP, "Connection status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (CONNECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONNECTION_CONNECTED_ITEM, CONNECTION_CONNECTED_ITEM_NAME, "Connected", false);
		indigo_init_switch_item(CONNECTION_DISCONNECTED_ITEM, CONNECTION_DISCONNECTED_ITEM_NAME, "Disconnected", true);
		// -------------------------------------------------------------------------------- DEVICE_INFO
		INFO_PROPERTY = indigo_init_text_property(NULL, device->name, INFO_PROPERTY_NAME, MAIN_GROUP, "Device info", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(INFO_DEVICE_NAME_ITEM, INFO_DEVICE_NAME_ITEM_NAME, "Device name", device->name);
		indigo_init_text_item(INFO_DEVICE_DRIVER_ITEM, INFO_DEVICE_DRVIER_ITEM_NAME, "Driver name", "%s", driver_name);
		indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, INFO_DEVICE_VERSION_ITEM_NAME, "Driver version", "%d.%d.%d.%d", INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MAJOR(version), INDIGO_VERSION_MINOR(version));
		indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, INFO_DEVICE_INTERFACE_ITEM_NAME, "Interface", "%u", interface);
		indigo_init_text_item(INFO_DEVICE_MODEL_ITEM, INFO_DEVICE_MODEL_ITEM_NAME, "Model", device->name);
		indigo_init_text_item(INFO_DEVICE_FW_REVISION_ITEM, INFO_DEVICE_FW_REVISION_ITEM_NAME, "Firmware Rev.", "N/A");
		indigo_init_text_item(INFO_DEVICE_HW_REVISION_ITEM, INFO_DEVICE_HW_REVISION_ITEM_NAME, "Hardware Rev.", "N/A");
		indigo_init_text_item(INFO_DEVICE_SERIAL_NUM_ITEM, INFO_DEVICE_SERIAL_NUM_ITEM_NAME, "Serial No.", "N/A");
		/* Decrease count as other items are rare if you need them just set count to 8 in the driver */
		INFO_PROPERTY->count = 4;
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY = indigo_init_switch_property(NULL, device->name, SIMULATION_PROPERTY_NAME, MAIN_GROUP, "Simulation status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (SIMULATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		SIMULATION_PROPERTY->hidden = true;
		indigo_init_switch_item(SIMULATION_ENABLED_ITEM, SIMULATION_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(SIMULATION_DISABLED_ITEM, SIMULATION_DISABLED_ITEM_NAME, "Disabled", true);
		// -------------------------------------------------------------------------------- CONFIG
		CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, CONFIG_PROPERTY_NAME, MAIN_GROUP, "Configuration control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONFIG_LOAD_ITEM, CONFIG_LOAD_ITEM_NAME, "Load", false);
		indigo_init_switch_item(CONFIG_SAVE_ITEM, CONFIG_SAVE_ITEM_NAME, "Save", false);
		indigo_init_switch_item(CONFIG_REMOVE_ITEM, CONFIG_REMOVE_ITEM_NAME, "Remove", false);
		// -------------------------------------------------------------------------------- PROFILE
		PROFILE_PROPERTY = indigo_init_switch_property(NULL, device->name, PROFILE_PROPERTY_NAME, MAIN_GROUP, "Profile selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, PROFILE_COUNT);
		if (PROFILE_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 0; i < PROFILE_COUNT; i++) {
			char name[INDIGO_NAME_SIZE], label [INDIGO_NAME_SIZE];
			sprintf(name, PROFILE_ITEM_NAME, i);
			sprintf(label, "Profile #%d", i);
			indigo_init_switch_item(PROFILE_ITEM + i, name, label, i == 0);
		}
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY = indigo_init_switch_property(NULL, device->name, DEVICE_PORTS_PROPERTY_NAME, MAIN_GROUP, "Serial ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICE_PORTS);
		if (DEVICE_PORTS_PROPERTY == NULL)
			return INDIGO_FAILED;
		DEVICE_PORTS_PROPERTY->hidden = true;
		indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items, DEVICE_PORTS_REFRESH_ITEM_NAME, "Refresh", false);
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY = indigo_init_text_property(NULL, device->name, DEVICE_PORT_PROPERTY_NAME, MAIN_GROUP, "Serial port", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (DEVICE_PORT_PROPERTY == NULL)
			return INDIGO_FAILED;
		DEVICE_PORT_PROPERTY->hidden = true;
		indigo_init_text_item(DEVICE_PORT_ITEM, DEVICE_PORT_ITEM_NAME, "Device name or URL", DEVICE_PORTS_PROPERTY->count > 1 ? DEVICE_PORTS_PROPERTY->items[1].name : DEFAULT_TTY);
		if (*DEVICE_PORT_ITEM->text.value == '/' && access(DEVICE_PORT_ITEM->text.value, R_OK)) {
			DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY = indigo_init_text_property(NULL, device->name, DEVICE_BAUDRATE_PROPERTY_NAME, MAIN_GROUP, "Serial port baud rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (DEVICE_BAUDRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		DEVICE_BAUDRATE_PROPERTY->hidden = true;
		indigo_init_text_item(DEVICE_BAUDRATE_ITEM, DEVICE_BAUDRATE_ITEM_NAME, "Baud rate (bps)", "9600-8N1");
		// -------------------------------------------------------------------------------- SECURITY
		AUTHENTICATION_PROPERTY = indigo_init_text_property(NULL, device->name, AUTHENTICATION_PROPERTY_NAME, MAIN_GROUP, "Device authorization", INDIGO_OK_STATE, INDIGO_WO_PERM, 2);
		if (AUTHENTICATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		AUTHENTICATION_PROPERTY->hidden = true;
		indigo_init_text_item(AUTHENTICATION_PASSWORD_ITEM, AUTHENTICATION_PASSWORD_ITEM_NAME, "Password", "");
		indigo_init_text_item(AUTHENTICATION_USER_ITEM, AUTHENTICATION_USER_ITEM_NAME, "User name", "");
		pthread_mutex_init(&DEVICE_CONTEXT->config_mutex, NULL);
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
	if (indigo_property_match(CONNECTION_PROPERTY, property) && !CONNECTION_PROPERTY->hidden)
		indigo_define_property(device, CONNECTION_PROPERTY, NULL);
	return INDIGO_OK;
}

indigo_result indigo_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (CONNECTION_PROPERTY->state == INDIGO_ALERT_STATE)
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
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
	} else if (indigo_property_match_rw(SIMULATION_PROPERTY, property)) {
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
			indigo_save_property(device, NULL, SIMULATION_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
			indigo_save_property(device, NULL, DEVICE_BAUDRATE_PROPERTY);
			if (DEVICE_CONTEXT->property_save_file_handle) {
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
				close(DEVICE_CONTEXT->property_save_file_handle);
				DEVICE_CONTEXT->property_save_file_handle = 0;
			} else {
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			CONFIG_SAVE_ITEM->sw.value = false;
		} else if (indigo_switch_match(CONFIG_REMOVE_ITEM, property)) {
			if (indigo_remove_properties(device) == INDIGO_OK)
				CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			else
				CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			CONFIG_REMOVE_ITEM->sw.value = false;
		}
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
	} else if (indigo_property_match(PROFILE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- PROFILE
		indigo_property_copy_values(PROFILE_PROPERTY, property, false);
		PROFILE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, PROFILE_PROPERTY, NULL);
	} else if (indigo_property_match_rw(DEVICE_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_PORT
		indigo_property_copy_values(DEVICE_PORT_PROPERTY, property, false);
		if (*DEVICE_PORT_ITEM->text.value == '/') {
			if (!access(DEVICE_PORT_ITEM->text.value, R_OK)) {
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
	} else if (indigo_property_match_rw(DEVICE_BAUDRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		indigo_property_copy_values(DEVICE_BAUDRATE_PROPERTY, property, false);
		DEVICE_BAUDRATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, NULL);
	} else if (indigo_property_match(DEVICE_PORTS_PROPERTY, property)) {
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
					strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
					DEVICE_PORTS_PROPERTY->items[i].sw.value = false;
				}
			}
		}
		if (*DEVICE_PORT_ITEM->text.value == '/' && access(DEVICE_PORT_ITEM->text.value, R_OK)) {
			DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		DEVICE_PORTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_PORTS_PROPERTY, NULL);
	} else if (indigo_property_match(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		PROFILE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUTHENTICATION_PROPERTY, NULL);
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
	indigo_release_property(DEVICE_BAUDRATE_PROPERTY);
	indigo_release_property(DEVICE_PORTS_PROPERTY);
	indigo_release_property(SIMULATION_PROPERTY);
	indigo_release_property(CONFIG_PROPERTY);
	indigo_release_property(PROFILE_PROPERTY);
	indigo_release_property(AUTHENTICATION_PROPERTY);
	indigo_property *all_properties = indigo_init_text_property(NULL, device->name, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
	indigo_delete_property(device, all_properties, NULL);
	indigo_release_property(all_properties);
	pthread_mutex_destroy(&DEVICE_CONTEXT->config_mutex);
	free(DEVICE_CONTEXT);
	device->device_context = NULL;
	return INDIGO_OK;
}

extern int indigo_server_tcp_port;
extern bool indigo_is_ephemeral_port;

static bool make_config_file_name(char *device_name, int profile, const char *suffix, char *path, int size) {
	int path_end = snprintf(path, size, "%s/.indigo", getenv("HOME"));
	int handle = mkdir(path, 0777);
	if (handle == 0 || errno == EEXIST) {
		if (indigo_server_tcp_port == 7624 || indigo_is_ephemeral_port) {
			if (profile)
				snprintf(path + path_end, size - path_end, "/%s#%d%s", device_name, profile, suffix);
			else
				snprintf(path + path_end, size - path_end, "/%s%s", device_name, suffix);
		} else {
			if (profile)
				snprintf(path + path_end, size - path_end, "/%s#%d_%d%s", device_name, profile, indigo_server_tcp_port, suffix);
			else
				snprintf(path + path_end, size - path_end, "/%s_%d%s", device_name, indigo_server_tcp_port, suffix);
		}
		char *space = strchr(path, ' ');
		while (space != NULL) {
			*space = '_';
			space = strchr(space+1, ' ');
		}
		return true;
	}
	return false;
}

int indigo_open_config_file(char *device_name, int profile, int mode, const char *suffix) {
	static char path[512];
	if (make_config_file_name(device_name, profile, suffix, path, sizeof(path))) {
		int handle = open(path, mode, 0644);
		if (handle < 0)
			INDIGO_DEBUG(indigo_debug("Can't %s %s (%s)", mode == O_RDONLY ? "open" : "create", path, strerror(errno)));
		return handle;
	} else {
		INDIGO_DEBUG(indigo_debug("Can't create %s (%s)", path, strerror(errno)));
	}
	return -1;
}

indigo_result indigo_load_properties(indigo_device *device, bool default_properties) {
	assert(device != NULL);
	int profile = 0;
	if (DEVICE_CONTEXT) {
		pthread_mutex_lock(&DEVICE_CONTEXT->config_mutex);
		for (int i = 0; i < PROFILE_COUNT; i++)
			if (PROFILE_PROPERTY->items[i].sw.value) {
				profile = i;
				break;
			}
	}
	int handle = indigo_open_config_file(device->name, profile, O_RDONLY, default_properties ? ".default" : ".config");
	if (handle > 0) {
		INDIGO_TRACE(indigo_trace("Config file open for '%s' with descriptor %d", device->name, handle));
		indigo_client *client = malloc(sizeof(indigo_client));
		memset(client, 0, sizeof(indigo_client));
		strcpy(client->name, CONFIG_READER);
		indigo_adapter_context *context = malloc(sizeof(indigo_adapter_context));
		context->input = handle;
		client->client_context = context;
		client->version = INDIGO_VERSION_CURRENT;
		indigo_xml_parse(NULL, client);
		close(handle);
		free(context);
		free(client);
	}
	if (DEVICE_CONTEXT)
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	return handle > 0 ? INDIGO_OK : INDIGO_FAILED;
}

indigo_result indigo_save_property(indigo_device *device, int *file_handle, indigo_property *property) {
	if (property == NULL)
		return INDIGO_FAILED;
	if (DEVICE_CONTEXT && pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex)) {
		INDIGO_ERROR(indigo_error("Failed to save property %s for %s (locked)", property->name, device->name));
		return INDIGO_FAILED;
	}
	if (!property->hidden && property->perm != INDIGO_RO_PERM) {
		char b1[32];
		if (file_handle == NULL)
			file_handle = &DEVICE_CONTEXT->property_save_file_handle;
		int handle = *file_handle;
		if (handle == 0) {
			int profile = 0;
			if (DEVICE_CONTEXT) {
				for (int i = 0; i < PROFILE_COUNT; i++)
					if (PROFILE_PROPERTY->items[i].sw.value) {
						profile = i;
						break;
					}
			}
			*file_handle = handle = indigo_open_config_file(property->device, profile, O_WRONLY | O_CREAT | O_TRUNC, ".config");
			if (handle == 0) {
				if (DEVICE_CONTEXT)
					pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
				return INDIGO_FAILED;
			}
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
				indigo_printf(handle, "<oneNumber name='%s'>%s</oneNumber>\n", item->name, indigo_dtoa(item->number.value, b1));
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
	if (DEVICE_CONTEXT)
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	return INDIGO_OK;
}

indigo_result indigo_save_property_items(indigo_device*device, int *file_handle, indigo_property *property, const int count, const char **items) {
	if (property == NULL)
		return INDIGO_FAILED;
	if (DEVICE_CONTEXT && pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex)) {
		INDIGO_ERROR(indigo_error("Failed to save property %s items for %s (locked)", property->name, device->name));
		return INDIGO_FAILED;
	}
	if (!property->hidden && property->perm != INDIGO_RO_PERM) {
		char b1[32];
		if (file_handle == NULL)
			file_handle = &DEVICE_CONTEXT->property_save_file_handle;
		int handle = *file_handle;
		if (handle == 0) {
			int profile = 0;
			if (DEVICE_CONTEXT) {
				for (int i = 0; i < PROFILE_COUNT; i++)
					if (PROFILE_PROPERTY->items[i].sw.value) {
						profile = i;
						break;
					}
			}
			*file_handle = handle = indigo_open_config_file(property->device, profile, O_WRONLY | O_CREAT | O_TRUNC, ".config");
			if (handle == 0) {
				if (DEVICE_CONTEXT)
					pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
				return INDIGO_FAILED;
			}
		}
		switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			indigo_printf(handle, "<newTextVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				for (int j = 0; j < count; j++) {
					if (!strncmp(items[j], item->name, INDIGO_NAME_SIZE)) {
						indigo_printf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, indigo_xml_escape(item->text.value));
						break;
					}
				}
			}
			indigo_printf(handle, "</newTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			indigo_printf(handle, "<newNumberVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				for (int j = 0; j < count; j++) {
					if (!strncmp(items[j], item->name, INDIGO_NAME_SIZE)) {
						indigo_printf(handle, "<oneNumber name='%s'>%s</oneNumber>\n", item->name, indigo_dtoa(item->number.value, b1));
						break;
					}
				}
			}
			indigo_printf(handle, "</newNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			indigo_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", indigo_xml_escape(property->device), property->name, indigo_property_state_text[property->state]);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				for (int j = 0; j < count; j++) {
					if (!strncmp(items[j], item->name, INDIGO_NAME_SIZE)) {
						indigo_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->sw.value ? "On" : "Off");
						break;
					}
				}
			}
			indigo_printf(handle, "</newSwitchVector>\n");
			break;
		default:
			break;
		}
	}
	if (DEVICE_CONTEXT)
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	return INDIGO_OK;
}

indigo_result indigo_remove_properties(indigo_device *device) {
	assert(device != NULL);
	int profile = 0;
	if (DEVICE_CONTEXT) {
		if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex))
			return INDIGO_FAILED;
		for (int i = 0; i < PROFILE_COUNT; i++)
			if (PROFILE_PROPERTY->items[i].sw.value) {
				profile = i;
				break;
			}
	}
	static char path[512];
	if (make_config_file_name(device->name, profile, ".config", path, sizeof(path))) {
		if (unlink(path) == 0) {
			if (DEVICE_CONTEXT)
				pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
			return INDIGO_OK;
		}
	}
	if (DEVICE_CONTEXT)
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	return INDIGO_FAILED;
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
		indigo_async(hotplug_thread, NULL);
		thread_started = true;
	}
}

/* TO BE REMOVED!
time_t indigo_utc(time_t *ltime) {
	struct tm tm_now;
	time_t now;

	if (ltime != NULL) now = *ltime;
	else time(&now);

	gmtime_r(&now, &tm_now);
	return timegm(&tm_now);
}
*/
void indigo_timetoisogm(time_t tstamp, char *isotime, int isotime_len) {
	struct tm tm_stamp;

	gmtime_r(&tstamp, &tm_stamp);
	strftime(isotime, isotime_len, "%Y-%m-%dT%H:%M:%S", &tm_stamp);
}

time_t indigo_isogmtotime(char *isotime) {
	struct tm tm_ts;

	memset(&tm_ts, 0, sizeof(tm_ts));
	if (sscanf(isotime, "%d-%d-%dT%d:%d:%d", &tm_ts.tm_year, &tm_ts.tm_mon, &tm_ts.tm_mday, &tm_ts.tm_hour, &tm_ts.tm_min, &tm_ts.tm_sec) == 6) {
		tm_ts.tm_mon -= 1;             /* mon is 0..11 */
		tm_ts.tm_year -= 1900;         /* year since 1900 */
		return (timegm(&tm_ts));
	}

	return -1;
}

void indigo_timetoisolocal(time_t tstamp, char *isotime, int isotime_len) {
	struct tm tm_stamp;

	localtime_r(&tstamp, &tm_stamp);
	tm_stamp.tm_isdst = 0;
	tm_stamp.tm_zone = 0;
	snprintf(isotime, isotime_len, "%04d-%02d-%02dT%02d:%02d:%02d", tm_stamp.tm_year + 1900, tm_stamp.tm_mon + 1, tm_stamp.tm_mday, tm_stamp.tm_hour, tm_stamp.tm_min, tm_stamp.tm_sec);
}

time_t indigo_isolocaltotime(char *isotime) {
	struct tm tm_ts;

	memset(&tm_ts, 0, sizeof(tm_ts));
	if (sscanf(isotime, "%d-%d-%dT%d:%d:%d", &tm_ts.tm_year, &tm_ts.tm_mon, &tm_ts.tm_mday, &tm_ts.tm_hour, &tm_ts.tm_min, &tm_ts.tm_sec) == 6) {
		tm_ts.tm_mon -= 1;             /* mon is 0..11 */
		tm_ts.tm_year -= 1900;         /* year since 1900 */
		tm_ts.tm_isdst = -1;
		return (mktime(&tm_ts));
	}

	return -1;
}

bool indigo_ignore_connection_change(indigo_device *device, indigo_property *request) {
	indigo_item *connected_item = NULL;
	indigo_item *disconnected_item = NULL;

	if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE)
		return true;

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
		if (connected_item->sw.value == disconnected_item->sw.value)
			return true;
		/* REQUESTED == CURRENT is not a state change.
		   Checking DISCONNECTED is not needed, as we already made sure CONNECTED = !DISCONNECTED
		*/
		if (connected_item->sw.value == CONNECTION_CONNECTED_ITEM->sw.value)
			return true;
	}

	// if only CONNECTED is set
	if (connected_item != NULL && disconnected_item == NULL) {
		// CURRENT == REQUSTED is not a state change.
		if (CONNECTION_CONNECTED_ITEM->sw.value == connected_item->sw.value)
			return true;
		/* request CONNECTED = OFF is illegal:
		   1. If current CONNECTED = OFF -> not a state change.
		   2. If current CONNECTED = ON -> reults in invalid state CONNECTED = OFF and DISCONNECTED = OFF.
		*/
		if (connected_item->sw.value == false)
			return true;
	}

	// if only DISCONNECTED is set - analogous to CONNECTED
	if (connected_item == NULL && disconnected_item != NULL) {
		if (CONNECTION_DISCONNECTED_ITEM->sw.value == disconnected_item->sw.value)
			return true;
		if (disconnected_item->sw.value == false)
			return true;
	}

	/* reqquest is valid */
	return false;
}
