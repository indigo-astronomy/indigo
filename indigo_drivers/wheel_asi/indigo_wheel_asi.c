//  Copyright (C) 2016 Rumen G. Bogdanovski
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
//  2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** INDIGO ASI filter wheel driver
 \file indigo_wheel_asi.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <hidapi/hidapi.h>

#include <asi_efw/EFW_filter.h>

#include "indigo_driver_xml.h"

#include "indigo_wheel_asi.h"


#define ASI_VENDOR_ID                   0x03c3
#define ASI_PRODUCT_ID                  0x1f01

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((asi_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
	int dev_id;
	int current_slot, target_slot;
	int count;
} asi_private_data;

// -------------------------------------------------------------------------------- INDIGO Wheel device implementation

static void wheel_timer_callback(indigo_device *device) {
	EFWGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	asi_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_wheel_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);

		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (EFWOpen(0)) {
				EFWGetID(0, &(PRIVATE_DATA->dev_id));
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->count = 5;
				PRIVATE_DATA->target_slot = 1;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_timer(device, 0.5, wheel_timer_callback);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			EFWClose(PRIVATE_DATA->dev_id);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
		
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
			EFWSetPosition(PRIVATE_DATA->dev_id,PRIVATE_DATA->target_slot);
			indigo_set_timer(device, 0.5, wheel_timer_callback);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static indigo_device *device = NULL;

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device wheel_template = {
		"ASI Filter Wheel", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		wheel_detach
	};
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			if (device != NULL)
				return 0;
			device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &wheel_template, sizeof(indigo_device));
			device->device_context = malloc(sizeof(asi_private_data));
			assert(device->device_context);
			memset(device->device_context, 0, sizeof(asi_private_data));
			indigo_attach_device(device);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			if (device == NULL)
				return 0;
			indigo_detach_device(device);
			free(device->device_context);
			free(device);
			device = NULL;
		}
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_asi(bool state) {
	static bool current_state = false;
	if (state == current_state)
		return INDIGO_OK;
	if ((current_state = state)) {
		device = NULL;
		libusb_init(NULL);
		hid_init();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, ASI_PRODUCT_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_wheel_asi: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		indigo_start_usb_event_handler();
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
	} else {
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_wheel_asi: libusb_hotplug_deregister_callback [%d]", __LINE__));
		if (device)
			hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		return INDIGO_OK;
	}
}


