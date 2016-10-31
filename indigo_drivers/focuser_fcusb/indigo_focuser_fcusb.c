//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//	You can use this software under the terms of 'INDIGO Astronomy
//  open-source license' (see LICENSE.md).
//
//	THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
//	OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//	ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO StarlighXpress filter wheel driver
 \file indigo_ccd_sx.c
 */

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

#include <libfcusb/libfcusb.h>

#include "indigo_driver_xml.h"
#include "indigo_focuser_fcusb.h"

#undef PRIVATE_DATA
#define PRIVATE_DATA													((fcusb_private_data *)DEVICE_CONTEXT->private_data)

#define X_FOCUSER_FREQUENCY_PROPERTY					(PRIVATE_DATA->focuser_abort_motion_property)
#define X_FOCUSER_FREQUENCY_1_ITEM						(X_FOCUSER_FREQUENCY_PROPERTY->items+0)
#define X_FOCUSER_FREQUENCY_4_ITEM						(X_FOCUSER_FREQUENCY_PROPERTY->items+1)
#define X_FOCUSER_FREQUENCY_16_ITEM						(X_FOCUSER_FREQUENCY_PROPERTY->items+2)

typedef struct {
	libfcusb_device_context *device_context;
	indigo_timer *focuser_timer;
	libusb_device *dev;
	indigo_property *focuser_abort_motion_property;

} fcusb_private_data;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	libfcusb_stop(PRIVATE_DATA->device_context);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	fcusb_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_focuser_device_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- X_FOCUSER_FREQUENCY
		X_FOCUSER_FREQUENCY_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_FREQUENCY", FOCUSER_MAIN_GROUP, "Frequency", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_FOCUSER_FREQUENCY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_1_ITEM, "FREQUENCY_1", "1.6 kHz (1x)", true);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_4_ITEM, "FREQUENCY_4", "6 kHz (4x)", false);
		indigo_init_switch_item(X_FOCUSER_FREQUENCY_16_ITEM, "FREQUENCY_16", "25 kHz (16x)", false);
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.max = 255;
		strncpy(FOCUSER_SPEED_ITEM->label, "Power (0-255)", INDIGO_VALUE_SIZE);
		strncpy(FOCUSER_SPEED_PROPERTY->label, "Power", INDIGO_VALUE_SIZE);
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_focuser_device_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (libfcusb_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context)) {
				libfcusb_led_off(PRIVATE_DATA->device_context);
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
			}
		} else {
			indigo_delete_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
			libfcusb_close(PRIVATE_DATA->device_context);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
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
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		PRIVATE_DATA->focuser_timer = indigo_set_timer(device, FOCUSER_STEPS_ITEM->number.value / 1000, focuser_timer_callback);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, PRIVATE_DATA->focuser_timer);
			libfcusb_stop(PRIVATE_DATA->device_context);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
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
	indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_delete_property(device, X_FOCUSER_FREQUENCY_PROPERTY, NULL);
	free(X_FOCUSER_FREQUENCY_PROPERTY);
	return indigo_focuser_device_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   3

static indigo_device *devices[MAX_DEVICES];

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device focuser_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		focuser_attach,
		indigo_focuser_device_enumerate_properties,
		focuser_change_property,
		focuser_detach
	};
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			const char *name;
			libfcusb_focuser(dev, &name);
			fcusb_private_data *private_data = malloc(sizeof(fcusb_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(fcusb_private_data));
			private_data->dev = dev;
			libusb_ref_device(dev);
			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &focuser_template, sizeof(indigo_device));
			strcpy(device->name, name);
			device->device_context = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
					break;
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

indigo_result indigo_focuser_fcusb(bool state) {
	libfcusb_use_syslog = indigo_use_syslog;
	static bool current_state = false;
	if (state == current_state)
		return INDIGO_OK;
	if ((current_state = state)) {
		current_state = state;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		libusb_init(NULL);
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FCUSB_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_focuser_fcusb: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		indigo_start_usb_even_handler();
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
	} else {
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_focuser_fcusb: libusb_hotplug_deregister_callback [%d]", __LINE__));
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		return INDIGO_OK;
	}
}
