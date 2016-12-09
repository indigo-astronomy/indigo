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
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
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

#define USE_POLL

#define NANO 1000000000L

#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)

#define time_diff(later, earier) ((later.tv_sec - earier.tv_sec) * 1000L + (later.tv_nsec - earier.tv_nsec) / 1000000L)

static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_timer *free_timers = NULL;

static void *timer_thread(indigo_device *device) {
	struct timespec now, sleep_time;
	indigo_timer *timer;
	long milis;
#ifdef USE_POLL
	struct pollfd pollfd;
	pollfd.fd = DEVICE_CONTEXT->timer_pipe[0];
	pollfd.events = POLLIN;
#else
	fd_set set;
	struct timeval timeout;
#endif
	char execute = 1;
	while (execute) {
		while (true) {
			pthread_mutex_lock(&DEVICE_CONTEXT->timer_mutex);
			timer = DEVICE_CONTEXT->timer_queue;
			if (timer == NULL) {
				pthread_mutex_unlock(&DEVICE_CONTEXT->timer_mutex);
				milis = 100000;
				break;
			}
			clock_gettime(CLOCK_MONOTONIC, &now);
			milis = time_diff(timer->time, now);
			if (milis <= 0) {
				DEVICE_CONTEXT->timer_queue = timer->next;
				pthread_mutex_unlock(&DEVICE_CONTEXT->timer_mutex);
				timer->callback(device);
				pthread_mutex_lock(&timer_mutex);
				timer->next = free_timers;
				free_timers = timer;
				pthread_mutex_unlock(&timer_mutex);
				continue;
			}
			pthread_mutex_unlock(&DEVICE_CONTEXT->timer_mutex);
			break;
		}
		INDIGO_TRACE(indigo_trace("sleep for %ldms", milis));
		clock_gettime(CLOCK_MONOTONIC, &sleep_time);
#ifdef USE_POLL
		if (poll(&pollfd, 1, (int)milis)) {
#else
		FD_ZERO (&set);
		FD_SET (DEVICE_CONTEXT->timer_pipe[0], &set);
		timeout.tv_sec = milis / 1000;
		timeout.tv_usec = (milis % 1000);
		if (select(FD_SETSIZE, &set, NULL, NULL, &timeout)) {
#endif
			int res = read(DEVICE_CONTEXT->timer_pipe[0], &execute, 1);
		} else {
			clock_gettime(CLOCK_MONOTONIC, &now);
			milis = time_diff(now, sleep_time) - milis;
			INDIGO_ERROR(if (milis > 20) indigo_error("timeout %ldms late", milis));
		}
	}
	while ((timer = DEVICE_CONTEXT->timer_queue) != NULL) {
		DEVICE_CONTEXT->timer_queue = timer->next;
		timer->next = free_timers;
		free_timers = timer;
	}
	close(DEVICE_CONTEXT->timer_pipe[0]);
	close(DEVICE_CONTEXT->timer_pipe[1]);
	return NULL;
}

indigo_timer *indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback) {
	assert(device != NULL);

	pthread_mutex_lock(&timer_mutex);
	indigo_timer *timer = free_timers;
	if (timer != NULL) {
		free_timers = free_timers->next;
	} else {
		timer = malloc(sizeof(indigo_timer));
		assert(timer != NULL);
	}
	pthread_mutex_unlock(&timer_mutex);

	struct timespec now, time;
	clock_gettime(CLOCK_MONOTONIC, &now);
	time.tv_sec = now.tv_sec + (int)delay;
	time.tv_nsec = now.tv_nsec + (long)((delay - (int)delay) * NANO);
	if (time.tv_nsec > NANO) {
		time.tv_sec++;
		time.tv_nsec -= NANO;
	}
	timer->time = time;
	timer->device = device;
	timer->callback = callback;

	INDIGO_TRACE(indigo_trace("timer queued to fire in %ldms", time_diff(timer->time, now)));

	pthread_mutex_lock(&DEVICE_CONTEXT->timer_mutex);
	indigo_timer *queue = DEVICE_CONTEXT->timer_queue, *end = NULL;
	while (queue != NULL && (timer->time.tv_sec > queue->time.tv_sec || (timer->time.tv_sec == queue->time.tv_sec && timer->time.tv_nsec > queue->time.tv_nsec))) {
		end = queue;
		queue = queue->next;
	}
	if (end == NULL) {
		if (queue == NULL)
			timer->next = NULL;
		else
			timer->next = queue;
		DEVICE_CONTEXT->timer_queue = timer;
	} else {
		timer->next = queue;
		end->next = timer;
	}
	pthread_mutex_unlock(&DEVICE_CONTEXT->timer_mutex);
	char data = 1;
	int res = write(DEVICE_CONTEXT->timer_pipe[1], &data, 1);
	return timer;
}

void indigo_cancel_timer(indigo_device *device, indigo_timer *timer) {
	assert(device != NULL);

	pthread_mutex_lock(&DEVICE_CONTEXT->timer_mutex);
	indigo_timer *queue = DEVICE_CONTEXT->timer_queue, *end = NULL;
	while (queue != NULL && queue != timer) {
		end = queue;
		queue = queue->next;
	}
	if (queue != NULL) {
		pthread_mutex_lock(&timer_mutex);
		if (end == NULL) {
			DEVICE_CONTEXT->timer_queue = queue->next;
		} else {
			end->next = queue->next;
		}
		queue->next = free_timers;
		free_timers = queue;
		pthread_mutex_unlock(&timer_mutex);
		INDIGO_TRACE(indigo_trace("timer canceled"));
	}
	pthread_mutex_unlock(&DEVICE_CONTEXT->timer_mutex);
}

#elif defined(INDIGO_MACOS)

void *dispatch_function(indigo_timer *timer) {
	if (!timer->canceled) {
		timer->callback(timer->device);
	}
	free(timer);
	return NULL;
}

indigo_timer *indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback) {
	indigo_timer *timer = malloc(sizeof(indigo_timer));
	assert(timer != NULL);
	timer->device = device;
	timer->callback = callback;
	timer->canceled = false;
	long nanos = delay * NANO;
	dispatch_after_f(dispatch_time(0, nanos), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), timer, (dispatch_function_t)dispatch_function);
	INDIGO_TRACE(indigo_trace("timer queued to fire in %ldms", nanos/1000000));
	return timer;
}

void indigo_cancel_timer(indigo_device *device, indigo_timer *timer) {
	timer->canceled = true;
	INDIGO_TRACE(indigo_trace("timer canceled"));
}

#endif

#if defined(INDIGO_LINUX)
bool is_serial(char *path) {
	int fd;
	struct serial_struct serinfo;

	if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) return false;

	bool is_sp = false;
	if (ioctl(fd, TIOCGSERIAL, &serinfo) == 0) is_sp = true;
	if (serinfo.type == PORT_UNKNOWN) is_sp = false;

	close(fd);
	return is_sp;
}
#endif

indigo_result indigo_device_attach(indigo_device *device, indigo_version version, int interface) {
	assert(device != NULL);
	assert(device != NULL);
	if (DEVICE_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_device_context));
		assert(device->device_context != NULL);
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
		INFO_PROPERTY = indigo_init_text_property(NULL, device->name, INFO_PROPERTY_NAME, MAIN_GROUP, "Device info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
		if (INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(INFO_DEVICE_NAME_ITEM, INFO_DEVICE_NAME_ITEM_NAME, "Name", device->name);
		indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, INFO_DEVICE_VERSION_ITEM_NAME, "Version", "%d.%d.%d.%d", INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MAJOR(version), INDIGO_VERSION_MINOR(version));
		indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, INFO_DEVICE_INTERFACE_ITEM_NAME, "Interface", "%d", interface);
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
		indigo_init_text_item(DEVICE_PORT_ITEM, DEVICE_PORT_ITEM_NAME, "Serial port", DEFAULT_TTY);
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
			while ((serial_device = IOIteratorNext(iterator))) {
				CFTypeRef cfs = IORegistryEntryCreateCFProperty (serial_device, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault,0);
				if (cfs) {
					CFStringGetCString(cfs, name, INDIGO_VALUE_SIZE, kCFStringEncodingASCII);
					if (strcmp(name, "/dev/cu.Bluetooth-Incoming-Port") && strcmp(name, "/dev/cu.iPhone-WirelessiAP")) {
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
			if (!strncmp(name, "tty", 3)) continue;
			snprintf(name, INDIGO_VALUE_SIZE, "/dev/%s", entry->d_name);
			if (is_serial(name)) {
				int i = DEVICE_PORTS_PROPERTY->count++;
				indigo_init_switch_item(DEVICE_PORTS_PROPERTY->items + i, name, name, false);
				if (i == 0)
					strcpy(DEVICE_PORT_ITEM->text.value, name);
			}
		}
#else
    /* freebsd */
#endif

#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
		if (pipe(DEVICE_CONTEXT->timer_pipe) != 0)
			return INDIGO_FAILED;
		if (pthread_mutex_init(&DEVICE_CONTEXT->timer_mutex, NULL) != 0)
			return INDIGO_FAILED;
		if (pthread_create(&DEVICE_CONTEXT->timer_thread, NULL, (void*)(void *)timer_thread, device) != 0)
			return INDIGO_FAILED;
#endif
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
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
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
	} else if (indigo_property_match(DEBUG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DEBUG
		indigo_property_copy_values(DEBUG_PROPERTY, property, false);
		DEBUG_PROPERTY->state = INDIGO_OK_STATE;
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
			if (DEBUG_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, DEBUG_PROPERTY);
			if (SIMULATION_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, SIMULATION_PROPERTY);
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
		if (!access(DEVICE_PORT_ITEM->text.value, R_OK)) {
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		} else {
			DEVICE_PORT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, "%s does not exists", DEVICE_PORT_ITEM->text.value);
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
#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
	char data = 0;
	int res = write(DEVICE_CONTEXT->timer_pipe[1], &data, 1);
	pthread_join(DEVICE_CONTEXT->timer_thread, NULL);
	pthread_mutex_destroy(&DEVICE_CONTEXT->timer_mutex);
#endif
	indigo_delete_property(device, &INDIGO_ALL_PROPERTIES, NULL);
	indigo_release_property(CONNECTION_PROPERTY);
	indigo_release_property(INFO_PROPERTY);
	indigo_release_property(DEVICE_PORT_PROPERTY);
	indigo_release_property(DEVICE_PORTS_PROPERTY);
	indigo_release_property(DEBUG_PROPERTY);
	indigo_release_property(SIMULATION_PROPERTY);
	indigo_release_property(CONFIG_PROPERTY);
	indigo_device_context *context = DEVICE_CONTEXT;
	device->device_context = context->private_data;
	free(context);
	return INDIGO_OK;
}

static void xprintf(int handle, const char *format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, 1024, format, args);
	va_end(args);
	int len = write(handle, buffer, length);
	INDIGO_DEBUG(indigo_debug("saved: %s", buffer));
}

static int open_config_file(char *device_name, int mode, const char *suffix) {
	static char path[128];
	int path_end = snprintf(path, sizeof(path), "%s/.indigo", getenv("HOME"));
	int handle = mkdir(path, 0777);
	if (handle == 0 || errno == EEXIST) {
		snprintf(path + path_end, sizeof(path) - path_end, "/%s%s", device_name, suffix);
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
	int handle = open_config_file(device->name, O_RDONLY, default_properties ? ".default" : ".config");
	if (handle > 0) {
		indigo_client *client = malloc(sizeof(indigo_client));
		memset(client, 0, sizeof(indigo_client));
		indigo_adapter_context *context = malloc(sizeof(indigo_adapter_context));
		context->input = handle;
		client->client_context = context;
		indigo_xml_parse(NULL, client);
		close(handle);
		free(context);
		free(client);
	}
	return handle > 0 ? INDIGO_OK : INDIGO_FAILED;
}

indigo_result indigo_save_property(indigo_device*device, int *file_handle, indigo_property *property) {
	if (file_handle == NULL)
		file_handle = &DEVICE_CONTEXT->property_save_file_handle;
	int handle = *file_handle;
	if (handle == 0) {
		*file_handle = handle = open_config_file(property->device, O_WRONLY | O_CREAT | O_TRUNC, ".config");
		if (handle == 0)
			return INDIGO_FAILED;
	}
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		xprintf(handle, "<newTextVector device='%s' name='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			xprintf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, item->text.value);
		}
		xprintf(handle, "</newTextVector>\n");
		break;
	case INDIGO_NUMBER_VECTOR:
		xprintf(handle, "<newNumberVector device='%s' name='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			xprintf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", item->name, item->number.value);
		}
		xprintf(handle, "</newNumberVector>\n");
		break;
	case INDIGO_SWITCH_VECTOR:
		xprintf(handle, "<newSwitchVector device='%s' name='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			xprintf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->sw.value ? "On" : "Off");
		}
		xprintf(handle, "</newSwitchVector>\n");
		break;
	default:
		break;
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



