// Copyright (c) 2016-2025 CloudMakers, s. r. o.
// All rights reserved.

// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_guider_gpusb.driver

// version history
// 3.0 by Peter Polakovic

// TODO: Add libgpusb for windows

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ "include" custom code below

#include <libgpusb.h>

//- "include" custom code above

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_guider_gpusb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_guider_gpusb"
#define DRIVER_LABEL         "Shoestring GPUSB guider"
#define GUIDER_DEVICE_NAME   "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((gpusb_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	libusb_device *usbdev;
	//+ "data" custom code below
	libgpusb_device_context *device_context;
	unsigned short relay_mask;
	//- "data" custom code above
	indigo_timer *guider_connection_handler_timer;
	indigo_timer *guider_guide_dec_handler_timer;
	indigo_timer *guider_guide_ra_handler_timer;
} gpusb_private_data;

#pragma mark - Low level code

//+ "code" custom code below

static bool gpusb_match(libusb_device *dev, const char **name) {
	return libgpusb_guider(dev, name);
}

static bool gpusb_open(indigo_device *device) {
	return libgpusb_open(PRIVATE_DATA->usbdev, &PRIVATE_DATA->device_context);
}

static void gpusb_close(indigo_device *device) {
	libgpusb_set(PRIVATE_DATA->device_context, 0);
	libgpusb_close(PRIVATE_DATA->device_context);
}

static void gpusb_debug(const char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libgpusb: %s\n", message);
}

//- "code" custom code above

#pragma mark - High level code (guider)

// CONNECTION change handler

static void guider_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = gpusb_open(device);
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_guide_dec_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_guide_ra_handler_timer);
		gpusb_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// GUIDER_GUIDE_DEC change handler

static void guider_guide_dec_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ "guider.GUIDER_GUIDE_DEC.on_change" custom code below
	PRIVATE_DATA->relay_mask &= ~(GPUSB_DEC_NORTH | GPUSB_DEC_SOUTH);
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	if (duration > 0) {
		PRIVATE_DATA->relay_mask |= GPUSB_DEC_NORTH;
	} else {
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= GPUSB_DEC_SOUTH;
		}
	}
	libgpusb_set(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask);
	if (duration > 0) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_usleep(duration * 1000.0);
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		PRIVATE_DATA->relay_mask &= ~(GPUSB_DEC_NORTH | GPUSB_DEC_SOUTH);
		libgpusb_set(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask);
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- "guider.GUIDER_GUIDE_DEC.on_change" custom code above
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// GUIDER_GUIDE_RA change handler

static void guider_guide_ra_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ "guider.GUIDER_GUIDE_RA.on_change" custom code below
	PRIVATE_DATA->relay_mask &= ~(GPUSB_RA_EAST | GPUSB_RA_WEST);
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	if (duration > 0) {
		PRIVATE_DATA->relay_mask |= GPUSB_RA_EAST;
	} else {
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= GPUSB_RA_WEST;
		}
	}
	libgpusb_set(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask);
	if (duration > 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_usleep(duration * 1000.0);
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		PRIVATE_DATA->relay_mask &= ~(GPUSB_RA_EAST | GPUSB_RA_WEST);
		libgpusb_set(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask);
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- "guider.GUIDER_GUIDE_RA.on_change" custom code above
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// guider attach API callback

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// guider enumerate API callback

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}

// guider change property API callback

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_connection_handler, &PRIVATE_DATA->guider_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		if (PRIVATE_DATA->guider_guide_dec_handler_timer == NULL) {
			indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_guide_dec_handler, &PRIVATE_DATA->guider_guide_dec_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		if (PRIVATE_DATA->guider_guide_ra_handler_timer == NULL) {
			indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_guide_ra_handler, &PRIVATE_DATA->guider_guide_ra_handler_timer);
		}
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

// guider detach API callback

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	const char *name;
	pthread_mutex_lock(&hotplug_mutex);
	if (gpusb_match(dev, &name)) {
		gpusb_private_data *private_data = indigo_safe_malloc(sizeof(gpusb_private_data));
		private_data->usbdev = dev;
		libusb_ref_device(dev);
			indigo_device *guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			snprintf(guider->name, INDIGO_NAME_SIZE, "%s", name);
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_async((void *)(void *)indigo_attach_device, devices[j] = guider);
					break;
				}
			}
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	gpusb_private_data *private_data = NULL;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		free(private_data);
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

#pragma mark - Main code

// Shoestring GPUSB guider driver entry point

indigo_result indigo_guider_gpusb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ "on_init" custom code below
			libgpusb_debug = &gpusb_debug;
			//- "on_init" custom code above
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, GPUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++) {
				VERIFY_NOT_CONNECTED(devices[i]);
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					hotplug_callback(NULL, PRIVATE_DATA->usbdev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
				}
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
