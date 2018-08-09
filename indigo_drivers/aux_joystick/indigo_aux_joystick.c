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

/** INDIGO HID Joystick driver
 \file indigo_aux_joystick.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_joystick"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <hidapi/hidapi.h>

#include "indigo_driver_xml.h"
#include "indigo_usb_utils.h"
#include "indigo_aux_joystick.h"

// -------------------------------------------------------------------------------- HID JOYSTICK interface implementation

#define PRIVATE_DATA        ((joystick_private_data *)device->private_data)

typedef struct {
	libusb_device *dev;
	hid_device *handle;
} joystick_private_data;

static bool joystick_open(indigo_device *device) {
	return false;
}

static void joystick_close(indigo_device *device) {
	hid_close(PRIVATE_DATA->handle);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	assert(device != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (joystick_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			joystick_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

//static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   10

static indigo_device *devices[MAX_DEVICES];

static void *plug_thread_func(libusb_device *dev) {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	usleep(100000);
	struct libusb_device_descriptor descriptor;
	int rc = libusb_get_device_descriptor(dev, &descriptor);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "libusb_get_device_descriptor ->  %s (%x:%x)", rc < 0 ? libusb_error_name(rc) : "OK", descriptor.idVendor, descriptor.idProduct);
	struct hid_device_info *info = hid_enumerate(descriptor.idVendor, descriptor.idProduct);
	if (info == NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "hid_enumerate ->  NULL");
		return 0;
	}
	if (info->usage != 0x04) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not a joystick");
		hid_free_enumeration(info);
		return 0;
	}
	joystick_private_data *private_data = malloc(sizeof(joystick_private_data));
	assert(private_data != NULL);
	memset(private_data, 0, sizeof(joystick_private_data));
	private_data->dev = dev;
	libusb_ref_device(dev);
	indigo_device *device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &aux_template, sizeof(indigo_device));
	char usb_path[INDIGO_NAME_SIZE];
	indigo_get_usb_path(dev, usb_path);
	snprintf(device->name, INDIGO_NAME_SIZE, "%S #%s", info->product_string, usb_path);
	device->private_data = private_data;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] == NULL) {
			indigo_attach_device(devices[j] = device);
			break;
		}
	}
	return NULL;
}

static void *unplug_thread_func(libusb_device *dev) {
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->dev == dev) {
				indigo_detach_device(device);
				free(PRIVATE_DATA);
				free(device);
				devices[j] = NULL;
				libusb_unref_device(dev);
			}
		}
	}
	return NULL;
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			indigo_async((void *)(void *)plug_thread_func, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			indigo_async((void *)(void *)unplug_thread_func, dev);
			break;
		}
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_aux_joystick(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "HID Joystick", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		hid_init();
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					unplug_thread_func(PRIVATE_DATA->dev);
				}
			}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
