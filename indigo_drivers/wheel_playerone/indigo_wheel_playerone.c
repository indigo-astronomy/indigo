// Copyright (C) 2023 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Plaayer One filter wheel driver
 \file indigo_wheel_playerone.c
 */

#define DRIVER_VERSION 0x03000009
#define DRIVER_NAME "indigo_wheel_playerone"

#define PONE_HANDLE_MAX 24

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_playerone.h"

#include <PlayerOnePW.h>

#define PONE_VENDOR_ID                   0xa0a0

#define PRIVATE_DATA        ((pone_private_data *)device->private_data)

#define POA_CUSTOM_SUFFIX_PROPERTY     (PRIVATE_DATA->playerone_custom_suffix_property)
#define POA_CUSTOM_SUFFIX_ITEM         (POA_CUSTOM_SUFFIX_PROPERTY->items+0)
#define POA_CUSTOM_SUFFIX_NAME         "SUFFIX"

// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	char model[256];
	int dev_handle;
	int current_slot, target_slot;
	int count;
	indigo_timer *wheel_timer;
	pthread_mutex_t usb_mutex;
	indigo_property *playerone_custom_suffix_property;
} pone_private_data;


// -------------------------------------------------------------------------------- INDIGO Wheel device implementation

static void wheel_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = POAGetCurrentPosition(PRIVATE_DATA->dev_handle, &(PRIVATE_DATA->current_slot));
	if (res != PW_OK && res != PW_ERROR_IS_MOVING) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetCurrentPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_handle, PRIVATE_DATA->current_slot, res);
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Set filter failed");
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetCurrentPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_handle, PRIVATE_DATA->current_slot, res);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	/* PRIVATE_DATA->current_slot is modified only if PW_OK is returned,
	   this prevents counting while FW is moving
	*/
	if (res == PW_OK) {
		PRIVATE_DATA->current_slot++;
	}
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->wheel_timer));
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}


static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(POA_CUSTOM_SUFFIX_PROPERTY);
	}
	return indigo_wheel_enumerate_properties(device, NULL, NULL);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		INFO_PROPERTY->count = 6;
		const char *sdk_version = POAGetPWSDKVer();
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);

		// --------------------------------------------------------------------------------- POA_CUSTOM_SUFFIX
		POA_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, "POA_CUSTOM_SUFFIX", WHEEL_MAIN_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (POA_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(POA_CUSTOM_SUFFIX_ITEM, POA_CUSTOM_SUFFIX_NAME, "Suffix", "");
		// --------------------------------------------------------------------------------
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	PWProperties info;
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

			if (indigo_try_global_lock(device) != INDIGO_OK) {
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				int res = POAGetPWPropertiesByHandle(PRIVATE_DATA->dev_handle, &info);
				if (res != PW_OK) {
					info.Handle = -1;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetPWPropertiesByHandle(%d) = %d", PRIVATE_DATA->dev_handle, res);
				}
				PRIVATE_DATA->dev_handle = info.Handle;
				res = POAOpenPW(PRIVATE_DATA->dev_handle);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res == PW_OK) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAOpenPW(%d) = %d", PRIVATE_DATA->dev_handle, res);
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->count = info.PositionCount;

					/* On connect wheel goes to position 1 - we need to wait while moving but not more than 15s */
					PWState state;
					const int max_wait = 30;
					const float retry_delay = 0.5;
					int count = 0;
					do {
						indigo_sleep(retry_delay);
						PWErrors res = POAGetPWState(PRIVATE_DATA->dev_handle, &state);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWState(%d, -> %d) = %d", PRIVATE_DATA->dev_handle, state, res);
						count++;
					} while (state == PW_STATE_MOVING && count < max_wait);

					if (count >= max_wait) {
						indigo_update_property(device, CONNECTION_PROPERTY, "WARNING: Did not move to initial position in %.0f seconds.", max_wait * retry_delay);
					}

					res = POAGetCurrentPosition(PRIVATE_DATA->dev_handle, &(PRIVATE_DATA->target_slot));
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetCurrentPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_handle, PRIVATE_DATA->target_slot, res);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					PRIVATE_DATA->target_slot++;
					WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->target_slot;

					res = POAGetPWCustomName(PRIVATE_DATA->dev_handle, POA_CUSTOM_SUFFIX_ITEM->text.value, INDIGO_NAME_SIZE);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWCustomName(%d, -> '%s') = %d", PRIVATE_DATA->dev_handle, POA_CUSTOM_SUFFIX_ITEM->text.value, res);
					indigo_define_property(device, POA_CUSTOM_SUFFIX_PROPERTY, NULL);

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;
					indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAOpenPW(%d) = %d", PRIVATE_DATA->dev_handle, res);
					indigo_global_unlock(device);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_delete_property(device, POA_CUSTOM_SUFFIX_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = POAClosePW(PRIVATE_DATA->dev_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAClosePW(%d) = %d", PRIVATE_DATA->dev_handle, res);
			res = POAGetPWPropertiesByHandle(PRIVATE_DATA->dev_handle, &info);
			if (res != PW_OK) {
				info.Handle = -1;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAGetPWPropertiesByHandle(%d) = %d", PRIVATE_DATA->dev_handle, res);
			} else {
				PRIVATE_DATA->dev_handle = info.Handle;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWPropertiesByHandle(%d) = %d", PRIVATE_DATA->dev_handle, res);
			}
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
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
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_slot = (int)WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = POAGotoPosition(PRIVATE_DATA->dev_handle, PRIVATE_DATA->target_slot-1);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGotoPosition(%d, %d) = %d", PRIVATE_DATA->dev_handle, PRIVATE_DATA->target_slot-1, res);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
			// ------------------------------------------------------------------------------- POA_CUSTOM_SUFFIX
	} else if (indigo_property_match_changeable(POA_CUSTOM_SUFFIX_PROPERTY, property)) {
		indigo_property_copy_values(POA_CUSTOM_SUFFIX_PROPERTY, property, false);
		POA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
		int length = (int)strlen(POA_CUSTOM_SUFFIX_ITEM->text.value);
		if (length > 24) {
			POA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, POA_CUSTOM_SUFFIX_PROPERTY, "Custom siffux is too long.");
			return INDIGO_OK;
		}
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = POASetPWCustomName(PRIVATE_DATA->dev_handle, POA_CUSTOM_SUFFIX_ITEM->text.value, length);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "POASetPWCustomName(%d, \"%s\", %d) > %d", PRIVATE_DATA->dev_handle, POA_CUSTOM_SUFFIX_ITEM->text.value, length, res);
			POA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, POA_CUSTOM_SUFFIX_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POASetPWCustomName(%d, \"%s\", %d) > %d", PRIVATE_DATA->dev_handle, POA_CUSTOM_SUFFIX_ITEM->text.value, length, res);
			POA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
			if (length > 0) {
				indigo_update_property(device, POA_CUSTOM_SUFFIX_PROPERTY, "FW name suffix '#%s' will be used on replug", POA_CUSTOM_SUFFIX_ITEM->text.value);
			} else {
				indigo_update_property(device, POA_CUSTOM_SUFFIX_PROPERTY, "FW name suffix cleared, will be used on replug");
			}
		}
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
	indigo_release_property(POA_CUSTOM_SUFFIX_PROPERTY);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


const int pw_products[1] = {0xf001};
const int pw_pid_count = 1;


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_handles[PONE_HANDLE_MAX];


static int find_plugged_device_handle() {
	int new_handle = NO_DEVICE;
	PWProperties props;
	int count = POAGetPWCount();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWCount() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = POAGetPWProperties(index, &props);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWProperties(%d, -> %d) = %d", index, props.Handle, res);
		if (res == PW_OK && !connected_handles[props.Handle]) {
			new_handle = props.Handle;
			connected_handles[props.Handle] = true;
			break;
		}
	}
	return new_handle;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(int handle) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) {
			continue;
		}
		if (PRIVATE_DATA->dev_handle == handle) return slot;
	}
	return -1;
}


//static bool device_name_exists(const char *name) {
//	for(int slot = 0; slot < MAX_DEVICES; slot++) {
//		indigo_device *device = devices[slot];
//		if (device == NULL) {
//      continue;
//    }
//		if (!strncmp(device->name, name, INDIGO_NAME_SIZE)) return true;
//	}
//	return false;
//}


static int find_unplugged_device_handle() {
	bool dev_tmp[PONE_HANDLE_MAX] = { false };
	int count = POAGetPWCount();
	PWProperties props;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWCount() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = POAGetPWProperties(index, &props);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWProperties(%d, -> %d) = %d", index, props.Handle, res);
		if (res == PW_OK) {
			dev_tmp[props.Handle] = true;
		}
	}
	int handle = -1;
	for (int index = 0; index < PONE_HANDLE_MAX; index++) {
		if (connected_handles[index] && !dev_tmp[index]) {
			handle = index;
			connected_handles[handle] = false;
			break;
		}
	}
	return handle;
}

static void split_device_name(const char *fill_device_name, char *device_name, char *suffix) {
	if (fill_device_name == NULL || device_name == NULL || suffix == NULL) {
		return;
	}

	char name_buf[256];
	strncpy(name_buf, fill_device_name, sizeof(name_buf));
	char *suffix_start = strchr(name_buf, '[');
	char *suffix_end = strrchr(name_buf, ']');

	if (suffix_start == NULL || suffix_end == NULL) {
		strncpy(device_name, name_buf, 256);
		suffix[0] = '\0';
		return;
	}
	suffix_start[0] = '\0';
	/* remove pace id name and suffix are space separated */
	if (suffix_start > name_buf && suffix_start[-1] == ' ') {
		suffix_start[-1] = '\0';
	}
	suffix_end[0] = '\0';
	suffix_start++;

	strncpy(device_name, name_buf, 256);
	strncpy(suffix, suffix_start, 16);
}

static pthread_mutex_t indigo_device_enumeration_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(indigo_device *unused) {
	PWProperties info;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	}
	int handle = find_plugged_device_handle();
	if (handle == NO_DEVICE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	}
	int res = POAOpenPW(handle);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "POAOpenPW(%d) = %d", handle, res);
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAOpenPW(%d) = %d", handle, res);
	}
	while (true) {
		res = POAGetPWPropertiesByHandle(handle, &info);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POAGetPWPropertiesByHandle(%d, -> { %d, '%s', %d }) = %d", handle, info.Handle, info.Name, info.PositionCount, res);
		if (res == PW_OK) {
			POAClosePW(handle);
			break;
		}
		if (res != PW_ERROR_IS_MOVING) {
			POAClosePW(handle);
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
			return;
		}
		indigo_sleep(1);
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
	char name[256] = {0};
	char suffix[16] = {0};
	char device_name[INDIGO_NAME_SIZE] = {0};
	split_device_name(info.Name, name, suffix);
	if (suffix[0] != '\0') {
		snprintf(device_name, INDIGO_NAME_SIZE, "%s #%s", name, suffix);
	} else {
		snprintf(device_name, INDIGO_NAME_SIZE, "%s", name);
	}
	sprintf(device->name, "%s", device_name);
	indigo_make_name_unique(device->name, "%d", handle);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	pone_private_data *private_data = indigo_safe_malloc(sizeof(pone_private_data));
	private_data->dev_handle = handle;
	strncpy(private_data->model, name, sizeof(private_data->model));
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	int slot, handle;
	bool removed = false;
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	while ((handle = find_unplugged_device_handle()) != -1) {
		slot = find_device_slot(handle);
		if (slot < 0) {
			continue;
		}
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
			return;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
		removed = true;
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No Pheoenix FW unplugged (maybe Player One camera)!");
	}
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			for (int i = 0; i < pw_pid_count; i++) {
				if (descriptor.idVendor != PONE_VENDOR_ID || pw_products[i] != descriptor.idProduct) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No Pheoenix FW unplugged (maybe Player One camera)!");
					continue;
				}
				indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			indigo_set_timer(NULL, 0.5, process_unplug_event, NULL);
			break;
		}
	}
	return 0;
};


static void remove_all_devices() {
	for (int index = 0; index < MAX_DEVICES; index++) {
		indigo_device **device = &devices[index];
		if (*device == NULL) {
			continue;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
	for (int index = 0; index < PONE_HANDLE_MAX; index++) {
		connected_handles[index] = false;
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_wheel_playerone(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Player One Filter Wheel", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			const char *sdk_version = POAGetPWSDKVer();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Player One filter wheel SDK v. %s", sdk_version);

			for (int index = 0; index < PONE_HANDLE_MAX; index++) {
				connected_handles[index] = false;
			}

			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, PONE_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++) {
				VERIFY_NOT_CONNECTED(devices[i]);
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			remove_all_devices();
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
