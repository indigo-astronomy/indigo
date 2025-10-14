// Copyright (c) 2016-2025 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_aux_dsusb.driver

// TODO: Add libdsusb for windows

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <libdsusb.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_aux_dsusb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_aux_dsusb"
#define DRIVER_LABEL         "Shoestring DSUSB shutter release"
#define AUX_DEVICE_NAME      "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((dsusb_private_data *)device->private_data)

#pragma mark - Property definitions

#define CCD_ABORT_EXPOSURE_PROPERTY    (PRIVATE_DATA->ccd_abort_exposure_property)
#define CCD_ABORT_EXPOSURE_ITEM        (CCD_ABORT_EXPOSURE_PROPERTY->items + 0)

#define CCD_EXPOSURE_PROPERTY          (PRIVATE_DATA->ccd_exposure_property)
#define CCD_EXPOSURE_ITEM              (CCD_EXPOSURE_PROPERTY->items + 0)

#define X_CONFIG_PROPERTY              (PRIVATE_DATA->x_config_property)
#define X_CONFIG_FOCUS_ITEM            (X_CONFIG_PROPERTY->items + 0)

#define X_CONFIG_PROPERTY_NAME         "X_CONFIG"
#define X_CONFIG_FOCUS_ITEM_NAME       "FOCUS"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *ccd_abort_exposure_property;
	indigo_property *ccd_exposure_property;
	indigo_property *x_config_property;
	//+ data
	libdsusb_device_context *device_context;
	//- data
} dsusb_private_data;

#pragma mark - Low level code

//+ code

static bool dsusb_match(libusb_device *dev, const char **name) {
	return libdsusb_shutter(dev, name);
}

static bool dsusb_open(indigo_device *device) {
	return libdsusb_open(PRIVATE_DATA->usbdev, &PRIVATE_DATA->device_context);
}

static void dsusb_close(indigo_device *device) {
	libdsusb_close(PRIVATE_DATA->device_context);
}

static void dsusb_debug(const char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libdsusb: %s\n", message);
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value--;
		if (CCD_EXPOSURE_ITEM->number.value <= 0) {
			CCD_EXPOSURE_ITEM->number.value = 0;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			libdsusb_stop(PRIVATE_DATA->device_context);
		}
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_EXPOSURE_ITEM->number.value > 0) {
			indigo_execute_priority_handler_in(device, 100, CCD_EXPOSURE_ITEM->number.value < 1 ? CCD_EXPOSURE_ITEM->number.value : 1, aux_timer_callback);
		}
	}
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = dsusb_open(device);
		if (connection_result) {
			//+ aux.on_connect
			CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = 0;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			//- aux.on_connect
			indigo_define_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, X_CONFIG_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, X_CONFIG_PROPERTY, NULL);
		//+ aux.on_disconnect
		libdsusb_stop(PRIVATE_DATA->device_context);
		//- aux.on_disconnect
		dsusb_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_ccd_abort_exposure_handler(indigo_device *device) {
	CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.CCD_ABORT_EXPOSURE.on_change
	if (CCD_ABORT_EXPOSURE_ITEM->sw.value && CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, aux_timer_callback);
		libdsusb_stop(PRIVATE_DATA->device_context);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	//- aux.CCD_ABORT_EXPOSURE.on_change
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
}

static void aux_ccd_exposure_handler(indigo_device *device) {
	//+ aux.CCD_EXPOSURE.on_change
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	if (X_CONFIG_FOCUS_ITEM->sw.value) {
		libdsusb_focus(PRIVATE_DATA->device_context);
		indigo_usleep(1000000);
	}
	libdsusb_start(PRIVATE_DATA->device_context);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_execute_priority_handler_in(device, 100, CCD_EXPOSURE_ITEM->number.value < 1 ? CCD_EXPOSURE_ITEM->number.value : 1, aux_timer_callback);
	//- aux.CCD_EXPOSURE.on_change
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void aux_x_config_handler(indigo_device *device) {
	X_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CONFIG_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SHUTTER) == INDIGO_OK) {
		CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Abort exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (CCD_ABORT_EXPOSURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CCD_ABORT_EXPOSURE_ITEM, CCD_ABORT_EXPOSURE_ITEM_NAME, "Abort exposure", false);
		CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Start exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (CCD_EXPOSURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(CCD_EXPOSURE_ITEM, CCD_EXPOSURE_ITEM_NAME, "Start exposure", 0, 1000, 1, 0);
		strcpy(CCD_EXPOSURE_ITEM->number.format, "%g");
		X_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CONFIG_PROPERTY_NAME, AUX_MAIN_GROUP, "Configure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CONFIG_FOCUS_ITEM, X_CONFIG_FOCUS_ITEM_NAME, "Focus before capture", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(CCD_ABORT_EXPOSURE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(CCD_EXPOSURE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CONFIG_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, aux_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, aux_ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CONFIG_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CONFIG_PROPERTY, aux_x_config_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_CONFIG_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(CCD_ABORT_EXPOSURE_PROPERTY);
	indigo_release_property(CCD_EXPOSURE_PROPERTY);
	indigo_release_property(X_CONFIG_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	const char *name;
	pthread_mutex_lock(&hotplug_mutex);
	if (dsusb_match(dev, &name)) {
		dsusb_private_data *private_data = indigo_safe_malloc(sizeof(dsusb_private_data));
		private_data->usbdev = dev;
		libusb_ref_device(dev);
			indigo_device *aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			snprintf(aux->name, INDIGO_NAME_SIZE, "%s", name);
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_async((void *)(void *)indigo_attach_device, devices[j] = aux);
					break;
				}
			}
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	dsusb_private_data *private_data = NULL;
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

indigo_result indigo_aux_dsusb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			libdsusb_debug = &dsusb_debug;
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, DSUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
