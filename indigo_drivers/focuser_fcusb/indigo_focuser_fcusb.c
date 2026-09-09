// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_fcusb.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <libfcusb.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_focuser_fcusb.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_focuser_fcusb"
#define DRIVER_LABEL         "Shoestring FCUSB focuser"
#define FOCUSER_DEVICE_NAME  "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((fcusb_private_data *)device->private_data)

#pragma mark - Property definitions

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
	libusb_device *usbdev;
	indigo_property *x_focuser_frequency_property;
	//+ data
	libfcusb_device_context *device_context;
	//- data
} fcusb_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

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

//- code

//+ focuser.code

static void focuser_motion_finalizer(indigo_device *device) {
	bool ok = libfcusb_stop(PRIVATE_DATA->device_context);
	FOCUSER_STEPS_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_STEPS_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = fcusb_open(device);
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
		fcusb_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, focuser_motion_finalizer);
	if (!libfcusb_stop(PRIVATE_DATA->device_context)) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_STEPS_PROPERTY, INDIGO_ALERT_STATE, NULL);
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int frequency = X_FOCUSER_FREQUENCY_16_ITEM->sw.value ? 16 : (X_FOCUSER_FREQUENCY_4_ITEM->sw.value ? 4 : 1);
	bool ok = FOCUSER_STEPS_ITEM->number.value > 0 && libfcusb_set_power(PRIVATE_DATA->device_context, FOCUSER_SPEED_ITEM->number.value) && libfcusb_set_frequency(PRIVATE_DATA->device_context, frequency);
	if (ok) {
		ok = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? libfcusb_move_in(PRIVATE_DATA->device_context) : libfcusb_move_out(PRIVATE_DATA->device_context);
	}
	if (ok) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, FOCUSER_STEPS_ITEM->number.value / 1000.0, focuser_motion_finalizer);
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	//- focuser.FOCUSER_STEPS.on_change
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.max = 255;
		INDIGO_COPY_VALUE(FOCUSER_SPEED_ITEM->label, "Power (0-255)");
		INDIGO_COPY_VALUE(FOCUSER_SPEED_PROPERTY->label, "Power");
		//- focuser.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_STEPS_PROPERTY->hidden = false;
		X_FOCUSER_FREQUENCY_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_FREQUENCY_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Frequency", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_FREQUENCY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_1_ITEM, X_FOCUSER_FREQUENCY_1_ITEM_NAME, "1.6 kHz (1x)", true);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_4_ITEM, X_FOCUSER_FREQUENCY_4_ITEM_NAME, "6 kHz (4x)", false);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_16_ITEM, X_FOCUSER_FREQUENCY_16_ITEM_NAME, "25 kHz (16x)", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_FREQUENCY_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, focuser_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_FREQUENCY_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_FREQUENCY_PROPERTY, property, false);
		X_FOCUSER_FREQUENCY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_FOCUSER_FREQUENCY_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_FREQUENCY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	fcusb_private_data *private_data = NULL;
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] && ((fcusb_private_data *)devices[i]->private_data)->usbdev == dev) {
			libusb_unref_device(dev);
			return;
		}
	}
	const char *name;
	if (fcusb_match(dev, &name)) {
		private_data = indigo_safe_malloc(sizeof(fcusb_private_data));
		private_data->usbdev = dev;
			indigo_device *focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			snprintf(focuser->name, INDIGO_NAME_SIZE, "%s", name);
			bool focuser_attached = false;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					devices[j] = focuser;
					if (indigo_attach_device(focuser) == INDIGO_OK) {
						dev_ref_transferred = true;
						focuser_attached = true;
					} else {
						devices[j] = NULL;
					}
					break;
				}
			}
			if (!focuser_attached) {
				indigo_safe_free(focuser);
				indigo_safe_free(private_data);
				libusb_unref_device(dev);
				return;
			}
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	fcusb_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		indigo_safe_free(private_data);
	}
	libusb_unref_device(dev);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_plug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_unplug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_focuser_fcusb(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			libfcusb_debug = &fcusb_debug;
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FCUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
