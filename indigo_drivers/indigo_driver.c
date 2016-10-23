//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

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

#if defined(INDIGO_DARWIN)
#include "libusb.h"
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "indigo_driver.h"
#include "indigo_xml.h"

#define USE_POLL

#undef INDIGO_DEBUG
#define INDIGO_DEBUG(c) c

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
		INDIGO_DEBUG(indigo_debug("sleep for %ldms", milis));
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
			read(DEVICE_CONTEXT->timer_pipe[0], &execute, 1);
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

	INDIGO_DEBUG(indigo_debug("timer queued to fire in %ldms", time_diff(timer->time, now)));

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
	write(DEVICE_CONTEXT->timer_pipe[1], &data, 1);
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
		INDIGO_DEBUG(indigo_debug("timer canceled"));
	}
	pthread_mutex_unlock(&DEVICE_CONTEXT->timer_mutex);
}

#elif defined(INDIGO_DARWIN)

void *dispatch_function(indigo_timer *timer) {
	if (!timer->canceled) {
		timer->callback(timer->device);
	}
	free(timer);
	return NULL;
}

indigo_timer *indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback) {
	indigo_timer *timer = malloc(sizeof(indigo_timer));
	timer->device = device;
	timer->callback = callback;
	timer->canceled = false;
	long nanos = delay * NANO;
	dispatch_after_f(dispatch_time(0, nanos), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), timer, (dispatch_function_t)dispatch_function);
	INDIGO_DEBUG(indigo_debug("timer queued to fire in %ldms", nanos/1000000));
	return timer;
}

void indigo_cancel_timer(indigo_device *device, indigo_timer *timer) {
	timer->canceled = true;
	INDIGO_DEBUG(indigo_debug("timer canceled"));
}

#endif

indigo_result indigo_device_attach(indigo_device *device, indigo_version version, int interface) {
	assert(device != NULL);
	assert(device != NULL);
	if (DEVICE_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_device_context));
		memset(device->device_context, 0, sizeof(indigo_device_context));
	}
	if (DEVICE_CONTEXT != NULL) {
		// -------------------------------------------------------------------------------- CONNECTION
		CONNECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, "CONNECTION", MAIN_GROUP, "Connection status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (CONNECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONNECTION_CONNECTED_ITEM, "CONNECTED", "Connected", false);
		indigo_init_switch_item(CONNECTION_DISCONNECTED_ITEM, "DISCONNECTED", "Disconnected", true);
		// -------------------------------------------------------------------------------- DEVICE_INFO
		INFO_PROPERTY = indigo_init_text_property(NULL, device->name, "DEVICE_INFO", MAIN_GROUP, "Device info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
		if (INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(INFO_DEVICE_NAME_ITEM, "NAME", "Name", device->name);
		indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, "VERSION", "Version", "%d.%d", (version >> 8) & 0xFF, version & 0xFF);
		indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, "INTERFACE", "Interface", "%d", interface);
		// -------------------------------------------------------------------------------- DEBUG
		DEBUG_PROPERTY = indigo_init_switch_property(NULL, device->name, "DEBUG", MAIN_GROUP, "Debug status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DEBUG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(DEBUG_ENABLED_ITEM, "ENABLED", "Enabled", false);
		indigo_init_switch_item(DEBUG_DISABLED_ITEM, "DISABLED", "Disabled", true);
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY = indigo_init_switch_property(NULL, device->name, "SIMULATION", MAIN_GROUP, "Simulation status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (SIMULATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		SIMULATION_PROPERTY->hidden = true;
		indigo_init_switch_item(SIMULATION_ENABLED_ITEM, "ENABLED", "Enabled", false);
		indigo_init_switch_item(SIMULATION_DISABLED_ITEM, "DISABLED", "Disabled", true);
		// -------------------------------------------------------------------------------- CONFIG
		CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, "CONFIG", MAIN_GROUP, "Configuration control", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONFIG_LOAD_ITEM, "LOAD", "Load", false);
		indigo_init_switch_item(CONFIG_SAVE_ITEM, "SAVE", "Save", false);
		indigo_init_switch_item(CONFIG_DEFAULT_ITEM, "DEFAULT", "Default", false);
		// --------------------------------------------------------------------------------
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
	if (indigo_property_match(CONNECTION_PROPERTY, property))
		indigo_define_property(device, CONNECTION_PROPERTY, NULL);
	if (indigo_property_match(INFO_PROPERTY, property))
		indigo_define_property(device, INFO_PROPERTY, NULL);
	if (indigo_property_match(DEBUG_PROPERTY, property))
		indigo_define_property(device, DEBUG_PROPERTY, NULL);
	if (indigo_property_match(SIMULATION_PROPERTY, property) && !SIMULATION_PROPERTY->hidden)
		indigo_define_property(device, SIMULATION_PROPERTY, NULL);
	if (indigo_property_match(CONFIG_PROPERTY, property))
		indigo_define_property(device, CONFIG_PROPERTY, NULL);
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
				indigo_save_property(device, DEBUG_PROPERTY);
			if (SIMULATION_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, SIMULATION_PROPERTY);
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
		// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}

indigo_result indigo_device_detach(indigo_device *device) {
	assert(device != NULL);
#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
	char data = 0;
	write(DEVICE_CONTEXT->timer_pipe[1], &data, 1);
	pthread_join(DEVICE_CONTEXT->timer_thread, NULL);
	pthread_mutex_destroy(&DEVICE_CONTEXT->timer_mutex);
#endif
	indigo_delete_property(device, CONNECTION_PROPERTY, NULL);
	indigo_delete_property(device, INFO_PROPERTY, NULL);
	indigo_delete_property(device, DEBUG_PROPERTY, NULL);
	if (!SIMULATION_PROPERTY->hidden)
		indigo_delete_property(device, SIMULATION_PROPERTY, NULL);
	indigo_delete_property(device, CONFIG_PROPERTY, NULL);
	free(CONNECTION_PROPERTY);
	free(INFO_PROPERTY);
	free(DEBUG_PROPERTY);
	free(SIMULATION_PROPERTY);
	free(CONFIG_PROPERTY);
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
	write(handle, buffer, length);
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
			indigo_error("Can't create %s (%s)", path, strerror(errno));
	} else {
		indigo_error("Can't create %s (%s)", path, strerror(errno));
	}
	return handle;
}

indigo_result indigo_load_properties(indigo_device *device, bool default_properties) {
	assert(device != NULL);
	int handle = open_config_file(INFO_DEVICE_NAME_ITEM->text.value, O_RDONLY, default_properties ? ".default" : ".config");
	if (handle > 0) {
		indigo_xml_parse(handle, NULL, NULL);
		close(handle);
	}
	return handle > 0 ? INDIGO_OK : INDIGO_FAILED;
}

indigo_result indigo_save_property(indigo_device*device, indigo_property *property) {
	int handle = DEVICE_CONTEXT->property_save_file_handle;
	if (handle == 0) {
		DEVICE_CONTEXT->property_save_file_handle = handle = open_config_file(property->device, O_WRONLY | O_CREAT | O_TRUNC, ".config");
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

void indigo_start_usb_even_handler() {
	static bool thread_started = false;
	if (!thread_started) {
		pthread_t hotplug_thread_handle;
		pthread_create(&hotplug_thread_handle, NULL, hotplug_thread, NULL);
		thread_started = true;
	}
}
	
void indigo_async(void *fun(void *data), void *data) {
	pthread_t async_thread;
	pthread_create(&async_thread, NULL, fun, data);
}



