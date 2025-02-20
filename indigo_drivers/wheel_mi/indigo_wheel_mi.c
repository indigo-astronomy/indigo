// Copyright (c) 2024 Moravian Instruments
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
// 2.0 by Jakub Smutny <info@gxccd.com>

/** INDIGO Moravian Instruments SFW driver
 \file indigo_wheel_mi.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_wheel_mi"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_usb_utils.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>

#include "indigo_wheel_mi.h"

#include <gxccd.h>

#define MI_VID						0x1347
#define PRIVATE_DATA				((mi_private_data *)device->private_data)

#define SFW_COMMANDS_GROUP			"MI_SFW_COMMANDS"
#define SFW_REINIT_SWITCH_PROPERTY	(PRIVATE_DATA->reinit_switch_property)
#define SFW_REINIT_SWITCH_ITEM		(SFW_REINIT_SWITCH_PROPERTY->items+0)
#define SFW_REINIT_SWITCH_ITEM_NAME	"MI_SFW_REINIT"

typedef struct {
	int eid;
	fwheel_t *wheel;
	int slot;
	indigo_timer *goto_timer, *reinit_timer;
	indigo_property *reinit_switch_property;
	uint8_t bus;
	uint8_t addr;
} mi_private_data;

static void mi_report_error(indigo_device *device, indigo_property *property) {
	char buffer[128];
	gxfw_get_last_error(PRIVATE_DATA->wheel, buffer, sizeof(buffer));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gxfw_get_last_error(..., -> %s)", buffer);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, buffer);
}

// -------------------------------------------------------------------------------- INDIGO Wheel device implementation

static void wheel_goto_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}

	int slot = WHEEL_SLOT_ITEM->number.target;
	int res = gxfw_set_filter(PRIVATE_DATA->wheel, slot - 1);
	if (res) {
		mi_report_error(device, WHEEL_SLOT_PROPERTY);
		return;
	}

	PRIVATE_DATA->slot = slot;
	WHEEL_SLOT_ITEM->number.value = slot;
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_reinit_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}

	int num_filters;
	int res = gxfw_reinit_filter_wheel(PRIVATE_DATA->wheel, &num_filters);
	if (res) {
		mi_report_error(device, SFW_REINIT_SWITCH_PROPERTY);
		return;
	}

	PRIVATE_DATA->slot = 1;
	WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = num_filters;
	WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 1;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);

	SFW_REINIT_SWITCH_ITEM->sw.value = false;
	SFW_REINIT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, SFW_REINIT_SWITCH_PROPERTY, NULL);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SFW_REINIT_SWITCH
		SFW_REINIT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, SFW_COMMANDS_GROUP, MAIN_GROUP, "Commands", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (SFW_REINIT_SWITCH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(SFW_REINIT_SWITCH_ITEM, SFW_REINIT_SWITCH_ITEM_NAME, "Reinit Filter Wheel", false);
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 8;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(SFW_REINIT_SWITCH_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, NULL, NULL);
}

static void wheel_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			PRIVATE_DATA->wheel = NULL;
		} else {
			PRIVATE_DATA->wheel = gxfw_initialize_usb(PRIVATE_DATA->eid);
		}
		if (PRIVATE_DATA->wheel) {
			int int_value;
			int fw_ver[4];

			gxfw_get_string_parameter(PRIVATE_DATA->wheel, FW_GSP_DESCRIPTION, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);

			gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_VERSION_1, &fw_ver[0]);
			gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_VERSION_2, &fw_ver[1]);
			gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_VERSION_3, &fw_ver[2]);
			gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_VERSION_4, &fw_ver[3]);
			snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%d.%d.%d.%d", fw_ver[0], fw_ver[1], fw_ver[2], fw_ver[3]);

			gxfw_get_string_parameter(PRIVATE_DATA->wheel, FW_GSP_SERIAL_NUMBER, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
			indigo_update_property(device, INFO_PROPERTY, NULL);

			SFW_REINIT_SWITCH_ITEM->sw.value = false;
			indigo_define_property(device, SFW_REINIT_SWITCH_PROPERTY, NULL);

			gxfw_get_integer_parameter(PRIVATE_DATA->wheel, FW_GIP_FILTERS, &int_value);
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = int_value;
			WHEEL_SLOT_ITEM->number.min = WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 1;
			PRIVATE_DATA->slot = 1;

			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);

			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

			indigo_set_timer(device, 0, wheel_goto_callback, &PRIVATE_DATA->goto_timer);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->goto_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->reinit_timer);
		if (PRIVATE_DATA->wheel) {
			gxfw_release(PRIVATE_DATA->wheel);
			PRIVATE_DATA->wheel = NULL;
		}
		indigo_global_unlock(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, 0, wheel_goto_callback, &PRIVATE_DATA->goto_timer);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- SFW_REINIT_SWITCH_PROPERTY
	} else if (indigo_property_match_changeable(SFW_REINIT_SWITCH_PROPERTY, property)) {
		indigo_property_copy_values(SFW_REINIT_SWITCH_PROPERTY, property, false);
		SFW_REINIT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, SFW_REINIT_SWITCH_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_reinit_callback, &PRIVATE_DATA->reinit_timer);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10

static indigo_device *devices[MAX_DEVICES];
static int new_eid = -1;

static void callback(int eid) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device && PRIVATE_DATA->eid == eid) {
			return;
		}
	}
	new_eid = eid;
}

static pthread_mutex_t indigo_device_enumeration_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	new_eid = -1;
	gxfw_enumerate_usb(callback);
	if (new_eid != -1) {
		fwheel_t *wheel = gxfw_initialize_usb(new_eid);
		if (wheel) {
			char name[128] = "MI ";
			gxfw_get_string_parameter(wheel, FW_GSP_DESCRIPTION, name + 3, sizeof(name) - 3);
			gxfw_release(wheel);
			mi_private_data *private_data = indigo_safe_malloc(sizeof(mi_private_data));
			private_data->eid = new_eid;
			private_data->bus = libusb_get_bus_number(dev);
			private_data->addr = libusb_get_device_address(dev);
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
			snprintf(device->name, INDIGO_NAME_SIZE, "%s", name);
			indigo_make_name_unique(device->name, "%d", new_eid);
			device->private_data = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	uint8_t bus = libusb_get_bus_number(dev);
	uint8_t addr = libusb_get_device_address(dev);
	for (int i = MAX_DEVICES - 1; i >=0; i--) {
		indigo_device *device = devices[i];
		if (device && PRIVATE_DATA->bus == bus && PRIVATE_DATA->addr == addr) {
			indigo_detach_device(device);
			mi_private_data *private_data = PRIVATE_DATA;
			free(private_data);
			free(device);
			devices[i] = NULL;
		}
	}
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
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

indigo_result indigo_wheel_mi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Moravian Instruments SFW", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, MI_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++)
				VERIFY_NOT_CONNECTED(devices[i]);
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");

			for (int i = MAX_DEVICES - 1; i >=0; i--) {
				indigo_device *device = devices[i];
				if (device) {
					indigo_detach_device(device);
					mi_private_data *private_data = PRIVATE_DATA;
					free(private_data);
					free(device);
					devices[i] = NULL;
				}
			}
			break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
