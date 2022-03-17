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

/** INDIGO DSUSB shutter driver
 \file indigo_aux_dsusb.c
 */

#define DRIVER_VERSION 0x0008
#define DRIVER_NAME "indigo_aux_dsusb"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <libusb-1.0/libusb.h>
#include <libdsusb.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_aux_dsusb.h"

#define PRIVATE_DATA												((dsusb_private_data *)device->private_data)

#define X_CCD_EXPOSURE_PROPERTY							(PRIVATE_DATA->exposure_property)
#define X_CCD_EXPOSURE_ITEM									(X_CCD_EXPOSURE_PROPERTY->items+0)

#define X_CCD_ABORT_EXPOSURE_PROPERTY				(PRIVATE_DATA->abort_exposure_property)
#define X_CCD_ABORT_EXPOSURE_ITEM						(X_CCD_ABORT_EXPOSURE_PROPERTY->items+0)

typedef struct {
	libusb_device *dev;
	libdsusb_device_context *device_context;
	indigo_property *exposure_property;
	indigo_property *abort_exposure_property;
	indigo_timer *timer_callback;
	pthread_mutex_t mutex;
} dsusb_private_data;

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SHUTTER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_CCD_EXPOSURE
		X_CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Start exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_CCD_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_CCD_EXPOSURE_ITEM, CCD_EXPOSURE_ITEM_NAME, "Start exposure", 0, 10000, 1, 0);
		strcpy(X_CCD_EXPOSURE_ITEM->number.format, "%g");
		// -------------------------------------------------------------------------------- X_CCD_ABORT_EXPOSURE
		X_CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Abort exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CCD_ABORT_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CCD_ABORT_EXPOSURE_ITEM, CCD_ABORT_EXPOSURE_ITEM_NAME, "Abort exposure", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_CCD_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		if (indigo_property_match(X_CCD_ABORT_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_CCD_EXPOSURE_ITEM->number.value--;
		if (X_CCD_EXPOSURE_ITEM->number.value <= 0) {
			X_CCD_EXPOSURE_ITEM->number.value = 0;
			X_CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			libdsusb_stop(PRIVATE_DATA->device_context);
		}
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		if (X_CCD_EXPOSURE_ITEM->number.value > 0)
			indigo_reschedule_timer(device, X_CCD_EXPOSURE_ITEM->number.value < 1 ? X_CCD_EXPOSURE_ITEM->number.value : 1, &PRIVATE_DATA->timer_callback);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (libdsusb_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context)) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
			indigo_define_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer_callback);
		libdsusb_stop(PRIVATE_DATA->device_context);
		libdsusb_close(PRIVATE_DATA->device_context);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_exposure_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	libdsusb_focus(PRIVATE_DATA->device_context);
	indigo_usleep(100000);
	libdsusb_start(PRIVATE_DATA->device_context);
	indigo_set_timer(device, X_CCD_EXPOSURE_ITEM->number.value < 1 ? X_CCD_EXPOSURE_ITEM->number.value : 1, aux_timer_callback, &PRIVATE_DATA->timer_callback);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_CCD_ABORT_EXPOSURE_ITEM->sw.value && X_CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_timer(device, &PRIVATE_DATA->timer_callback);
		libdsusb_stop(PRIVATE_DATA->device_context);
		X_CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
	}
	X_CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	X_CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_CCD_EXPOSURE
	} else if (indigo_property_match(X_CCD_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(X_CCD_EXPOSURE_PROPERTY, property, false);
		X_CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_exposure_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(X_CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(X_CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		X_CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_abort_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(X_CCD_EXPOSURE_PROPERTY);
	indigo_release_property(X_CCD_ABORT_EXPOSURE_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   3

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);
	const char *name;
	pthread_mutex_lock(&hotplug_mutex);
	if (libdsusb_shutter(dev, &name)) {
		dsusb_private_data *private_data = indigo_safe_malloc(sizeof(dsusb_private_data));
		private_data->dev = dev;
		libusb_ref_device(dev);
		indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
		indigo_copy_name(device->name, name);
		device->private_data = private_data;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
				break;
			}
		}
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->dev == dev) {
				indigo_detach_device(device);
				if (PRIVATE_DATA != NULL) {
					libusb_unref_device(dev);
					free(PRIVATE_DATA);
				}
				free(device);
				devices[j] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

static void debug(const char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libdsusb: %s\n", message);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_dsusb(indigo_driver_action action, indigo_driver_info *info) {
	libdsusb_debug = &debug;
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Shoestring DSUSB shutter release", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, DSUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		for (int i = 0; i < MAX_DEVICES; i++) {
			if (devices[i] != NULL) {
				indigo_device *device = devices[i];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
