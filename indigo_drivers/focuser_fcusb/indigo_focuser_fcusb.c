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

/** INDIGO FCUSB focuser driver
 \file indigo_focuser_fcusb.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_ccd_fcusb"

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

#include <libfcusb.h>

#include "indigo_driver_xml.h"
#include "indigo_focuser_fcusb.h"

#define PRIVATE_DATA													((fcusb_private_data *)device->private_data)

#define X_FOCUSER_FREQUENCY_PROPERTY					(PRIVATE_DATA->frequency_property)
#define X_FOCUSER_FREQUENCY_1_ITEM						(X_FOCUSER_FREQUENCY_PROPERTY->items+0)
#define X_FOCUSER_FREQUENCY_4_ITEM						(X_FOCUSER_FREQUENCY_PROPERTY->items+1)
#define X_FOCUSER_FREQUENCY_16_ITEM						(X_FOCUSER_FREQUENCY_PROPERTY->items+2)

typedef struct {
	libusb_device *dev;
	libfcusb_device_context *device_context;
	indigo_timer *focuser_timer;
	indigo_property *frequency_property;

} fcusb_private_data;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	libfcusb_stop(PRIVATE_DATA->device_context);
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_FOCUSER_FREQUENCY
		X_FOCUSER_FREQUENCY_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_FREQUENCY", FOCUSER_MAIN_GROUP, "Frequency", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_FREQUENCY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_1_ITEM, "FREQUENCY_1", "1.6 kHz (1x)", true);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_4_ITEM, "FREQUENCY_4", "6 kHz (4x)", false);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_16_ITEM, "FREQUENCY_16", "25 kHz (16x)", false);
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.max = 255;
		strncpy(FOCUSER_SPEED_ITEM->label, "Power (0-255)", INDIGO_VALUE_SIZE);
		strncpy(FOCUSER_SPEED_PROPERTY->label, "Power", INDIGO_VALUE_SIZE);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_FREQUENCY_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (libfcusb_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context)) {
				libfcusb_set_power(PRIVATE_DATA->device_context, FOCUSER_SPEED_ITEM->number.value);
				if (X_FOCUSER_FREQUENCY_1_ITEM->sw.value)
					libfcusb_set_frequency(PRIVATE_DATA->device_context, 1);
				else if (X_FOCUSER_FREQUENCY_4_ITEM->sw.value)
					libfcusb_set_frequency(PRIVATE_DATA->device_context, 4);
				else if (X_FOCUSER_FREQUENCY_4_ITEM->sw.value)
					libfcusb_set_frequency(PRIVATE_DATA->device_context, 16);
				indigo_define_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_delete_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
			libfcusb_close(PRIVATE_DATA->device_context);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_FOCUSER_FREQUENCY_PROPERTY);
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_ITEM->number.value > 0) {
			libfcusb_set_power(PRIVATE_DATA->device_context, FOCUSER_SPEED_ITEM->number.value);
			if (X_FOCUSER_FREQUENCY_1_ITEM->sw.value)
				libfcusb_set_frequency(PRIVATE_DATA->device_context, 1);
			else if (X_FOCUSER_FREQUENCY_4_ITEM->sw.value)
				libfcusb_set_frequency(PRIVATE_DATA->device_context, 4);
			else if (X_FOCUSER_FREQUENCY_4_ITEM->sw.value)
				libfcusb_set_frequency(PRIVATE_DATA->device_context, 16);
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				libfcusb_move_in(PRIVATE_DATA->device_context);
			} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
				libfcusb_move_out(PRIVATE_DATA->device_context);
			}
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, FOCUSER_STEPS_ITEM->number.value / 1000, focuser_timer_callback);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
			libfcusb_stop(PRIVATE_DATA->device_context);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_FREQUENCY
	} else if (indigo_property_match(X_FOCUSER_FREQUENCY_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_FREQUENCY_PROPERTY, property, false);
		X_FOCUSER_FREQUENCY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(X_FOCUSER_FREQUENCY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   3

static indigo_device *devices[MAX_DEVICES];

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			const char *name;
      if (libfcusb_focuser(dev, &name)) {
        fcusb_private_data *private_data = malloc(sizeof(fcusb_private_data));
        assert(private_data != NULL);
        memset(private_data, 0, sizeof(fcusb_private_data));
        private_data->dev = dev;
        libusb_ref_device(dev);
        indigo_device *device = malloc(sizeof(indigo_device));
        assert(device != NULL);
        memcpy(device, &focuser_template, sizeof(indigo_device));
        strncpy(device->name, name, INDIGO_NAME_SIZE);
        device->private_data = private_data;
        for (int j = 0; j < MAX_DEVICES; j++) {
          if (devices[j] == NULL) {
            indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
            break;
          }
        }
      }
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			fcusb_private_data *private_data = NULL;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					if (PRIVATE_DATA->dev == dev) {
						private_data = PRIVATE_DATA;
						indigo_detach_device(device);
						free(device);
						devices[j] = NULL;
						break;
					}
				}
			}
			if (private_data != NULL) {
				libusb_unref_device(dev);
				free(private_data);
			}
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

static void debug(const char *message) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libfcusb: %s\n", message);
}

indigo_result indigo_focuser_fcusb(indigo_driver_action action, indigo_driver_info *info) {
	libfcusb_debug = &debug;
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Shoestring FCUSB Focuser", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FCUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
