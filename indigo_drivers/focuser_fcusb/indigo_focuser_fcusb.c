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

// This file generated from indigo_focuser_fcusb.driver

// version history
// 3.0 by Peter Polakovic

// TODO: Add libfcusb for windows

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ "include" custom code below

#include <libfcusb.h>

//- "include" custom code above

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_focuser_fcusb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_focuser_fcusb"
#define DRIVER_LABEL         "Shoestring FCUSB focuser"
#define FOCUSER_DEVICE_NAME  "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((fcusb_private_data *)device->private_data)

#pragma mark - Property definitions

// X_FOCUSER_FREQUENCY handles definition
#define X_FOCUSER_FREQUENCY_PROPERTY      (PRIVATE_DATA->x_focuser_frequency_property)
#define X_FOCUSER_FREQUENCY_1_ITEM        (X_FOCUSER_FREQUENCY_PROPERTY->items + 0)
#define X_FOCUSER_FREQUENCY_4_ITEM        (X_FOCUSER_FREQUENCY_PROPERTY->items + 1)
#define X_FOCUSER_FREQUENCY_16_ITEM       (X_FOCUSER_FREQUENCY_PROPERTY->items + 2)

#define X_FOCUSER_FREQUENCY_PROPERTY_NAME "X_FOCUSER_FREQUENCY"
#define X_FOCUSER_FREQUENCY_1_ITEM_NAME   "FREQUENCY_1"
#define X_FOCUSER_FREQUENCY_4_ITEM_NAME   "FREQUENCY_4"
#define X_FOCUSER_FREQUENCY_16_ITEM_NAME  "FREQUENCY_16"

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	libusb_device *usbdev;
	//+ "data" custom code below
	libfcusb_device_context *device_context;
	//- "data" custom code above
	indigo_property *x_focuser_frequency_property;
	indigo_timer *focuser_connection_handler_timer;
	indigo_timer *focuser_abort_motion_handler_timer;
	indigo_timer *focuser_steps_handler_timer;
	indigo_timer *focuser_x_focuser_frequency_handler_timer;
} fcusb_private_data;

#pragma mark - Low level code

//+ "code" custom code below

static bool fcusb_match(libusb_device *dev, const char **name) {
	return libfcusb_focuser(dev, name);
}

static bool fcusb_open(indigo_device *device) {
	return libfcusb_open(PRIVATE_DATA->usbdev, &PRIVATE_DATA->device_context);
}

static void fcusb_close(indigo_device *device) {
	libfcusb_stop(PRIVATE_DATA->device_context);
	libfcusb_close(PRIVATE_DATA->device_context);
}

static void fcusb_debug(const char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libfcusb: %s\n", message);
}

//- "code" custom code above

#pragma mark - High level code (focuser)

// CONNECTION change handler

static void focuser_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = fcusb_open(device);
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_abort_motion_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_steps_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_x_focuser_frequency_handler_timer);
		indigo_delete_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
		fcusb_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// FOCUSER_ABORT_MOTION change handler

static void focuser_abort_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ "focuser.FOCUSER_ABORT_MOTION.on_change" custom code below
	if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	//- "focuser.FOCUSER_ABORT_MOTION.on_change" custom code above
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// FOCUSER_STEPS change handler

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ "focuser.FOCUSER_STEPS.on_change" custom code below
	if (FOCUSER_STEPS_ITEM->number.value > 0) {
		libfcusb_set_power(PRIVATE_DATA->device_context, FOCUSER_SPEED_ITEM->number.value);
		if (X_FOCUSER_FREQUENCY_1_ITEM->sw.value)
			libfcusb_set_frequency(PRIVATE_DATA->device_context, 1);
		else if (X_FOCUSER_FREQUENCY_4_ITEM->sw.value)
			libfcusb_set_frequency(PRIVATE_DATA->device_context, 4);
		else if (X_FOCUSER_FREQUENCY_16_ITEM->sw.value)
			libfcusb_set_frequency(PRIVATE_DATA->device_context, 16);
		if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
			libfcusb_move_in(PRIVATE_DATA->device_context);
		} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
			libfcusb_move_out(PRIVATE_DATA->device_context);
		}
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		int delay = FOCUSER_STEPS_ITEM->number.target;
		while (delay > 0) {
			if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
				break;
			}
			indigo_usleep(1000);
			delay--;
		}
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		libfcusb_stop(PRIVATE_DATA->device_context);
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- "focuser.FOCUSER_STEPS.on_change" custom code above
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// X_FOCUSER_FREQUENCY change handler

static void focuser_x_focuser_frequency_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	X_FOCUSER_FREQUENCY_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// focuser attach API callback

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ "focuser.on_attach" custom code below
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.max = 255;
		indigo_copy_value(FOCUSER_SPEED_ITEM->label, "Power (0-255)");
		indigo_copy_value(FOCUSER_SPEED_PROPERTY->label, "Power");
		//- "focuser.on_attach" custom code above
		X_FOCUSER_FREQUENCY_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_FREQUENCY_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Frequency", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_FREQUENCY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_1_ITEM, X_FOCUSER_FREQUENCY_1_ITEM_NAME, "1.6 kHz (1x)", true);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_4_ITEM, X_FOCUSER_FREQUENCY_4_ITEM_NAME, "6 kHz (4x)", false);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_16_ITEM, X_FOCUSER_FREQUENCY_16_ITEM_NAME, "25 kHz (16x)", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// focuser enumerate API callback

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(X_FOCUSER_FREQUENCY_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

// focuser change property API callback

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_connection_handler, &PRIVATE_DATA->focuser_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		if (PRIVATE_DATA->focuser_abort_motion_handler_timer == NULL) {
			indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_abort_motion_handler, &PRIVATE_DATA->focuser_abort_motion_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		if (PRIVATE_DATA->focuser_steps_handler_timer == NULL) {
			indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_steps_handler, &PRIVATE_DATA->focuser_steps_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_FREQUENCY_PROPERTY, property)) {
		if (PRIVATE_DATA->focuser_x_focuser_frequency_handler_timer == NULL) {
			indigo_property_copy_values(X_FOCUSER_FREQUENCY_PROPERTY, property, false);
			X_FOCUSER_FREQUENCY_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_x_focuser_frequency_handler, &PRIVATE_DATA->focuser_x_focuser_frequency_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_FOCUSER_FREQUENCY_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

// focuser detach API callback

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_FREQUENCY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	const char *name;
	pthread_mutex_lock(&hotplug_mutex);
	if (fcusb_match(dev, &name)) {
		fcusb_private_data *private_data = indigo_safe_malloc(sizeof(fcusb_private_data));
		private_data->usbdev = dev;
		libusb_ref_device(dev);
			indigo_device *focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			snprintf(focuser->name, INDIGO_NAME_SIZE, "%s", name);
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_async((void *)(void *)indigo_attach_device, devices[j] = focuser);
					break;
				}
			}
	}
	pthread_mutex_unlock(&hotplug_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&hotplug_mutex);
	fcusb_private_data *private_data = NULL;
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

// Shoestring FCUSB focuser driver entry point

indigo_result indigo_focuser_fcusb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ "on_init" custom code below
			libfcusb_debug = &fcusb_debug;
			//- "on_init" custom code above
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FCUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
