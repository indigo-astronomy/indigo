// Copyright (C) 2016 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO FLI filter wheel driver
 \file indigo_wheel_fli.c
 */

#define DRIVER_VERSION 0x0007
#define DRIVER_NAME		"indigo_wheel_fli"

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

#define MAX_PATH 255

#include <indigo/indigo_driver_xml.h>

#include "indigo_wheel_fli.h"
#include <libfli.h>

#define FLI_VENDOR_ID                   0x0f18

#define PRIVATE_DATA                    ((fli_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	flidev_t dev_id;
	char dev_file_name[MAX_PATH];
	char dev_name[MAX_PATH];
	flidomain_t domain;
	long int current_slot, target_slot;
	int count;
	indigo_timer *wheel_timer;
	pthread_mutex_t usb_mutex;
} fli_private_data;

static int find_index_by_device_fname(char *fname);
// -------------------------------------------------------------------------------- INDIGO Wheel device implementation

static void wheel_timer_callback(indigo_device *device) {
	if (!device->is_connected) return;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	long res = FLISetFilterPos(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot-1);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetFilterPos(%d) = %d", PRIVATE_DATA->dev_id, res);
	}

	res = FLIGetFilterPos(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetFilterPos(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	PRIVATE_DATA->current_slot++;

	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}


static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		/* Use all info property fields */
		INFO_PROPERTY->count = 8;

		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	int index = find_index_by_device_fname(PRIVATE_DATA->dev_file_name);
	if (index < 0) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;;
	} else {
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
					long res = FLIOpen(&(PRIVATE_DATA->dev_id), PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (!res) {
						flidev_t id = PRIVATE_DATA->dev_id;
						long int num_slots;
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

						FLIGetFilterCount(id, &num_slots);
						WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->count = (int)num_slots;
						WHEEL_SLOT_ITEM->number.min = 1;
						FLIGetFilterPos(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_slot));
						if (PRIVATE_DATA->target_slot < 0) {
							FLISetFilterPos(id, 0);
							PRIVATE_DATA->target_slot = 1;
							PRIVATE_DATA->current_slot = 1;
							WHEEL_SLOT_ITEM->number.value = 1;
						} else {
							PRIVATE_DATA->target_slot++;
							WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot = PRIVATE_DATA->target_slot;
						}
						res = FLIGetModel(id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
						if (res) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetModel(%d) = %d", id, res);
						}

						res = FLIGetSerialString(id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
						if (res) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetSerialString(%d) = %d", id, res);
						}

						long hw_rev, fw_rev;
						res = FLIGetFWRevision(id, &fw_rev);
						if (res) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetFWRevision(%d) = %d", id, res);
						}

						res = FLIGetHWRevision(id, &hw_rev);
						if (res) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetHWRevision(%d) = %d", id, res);
						}
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

						sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
						sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);
						indigo_update_property(device, INFO_PROPERTY, NULL);

						WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);

						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
						device->is_connected = true;

						indigo_set_timer(device, 0, wheel_timer_callback, NULL);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
						return;
					}
				}
			}
		} else {
			if (device->is_connected) {
				device->is_connected = false;
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				long res = FLIClose(PRIVATE_DATA->dev_id);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				if (res) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res);
				}
				PRIVATE_DATA->dev_id = -1;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				indigo_global_unlock(device);
			}
		}
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
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
			indigo_set_timer(device, 0, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
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

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   32

static const flidomain_t enum_domain = FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL;
static int num_devices = 0;
static char fli_file_names[MAX_DEVICES][MAX_PATH] = {""};
static char fli_dev_names[MAX_DEVICES][MAX_PATH] = {""};
static flidomain_t fli_domains[MAX_DEVICES] = {0};

static indigo_device *devices[MAX_DEVICES] = {NULL};


static void enumerate_devices() {
	/* There is a mem leak heree!!! 8,192 constant + 12 bytes on every new connected device */
	num_devices = 0;
	long res = FLICreateList(enum_domain);
	if (res)
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLICreateList(%d) = %d", enum_domain , res);
	else
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "FLICreateList(%d) = %d", enum_domain , res);
	res = FLIListFirst(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "FLIListFirst(-> %d, -> '%s', ->'%s') = %d", fli_domains[num_devices], fli_file_names[num_devices], fli_dev_names[num_devices], res);
	if (res == 0) {
		do {
			num_devices++;
			res = FLIListNext(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "FLIListNext(-> %d, -> '%s', ->'%s') = %d", fli_domains[num_devices], fli_file_names[num_devices], fli_dev_names[num_devices], res);
		} while ((res == 0) && (num_devices < MAX_DEVICES));
	}
	res = FLIDeleteList();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "FLIDeleteList() = %d", res);
}

static int find_plugged_device(char *fname) {
	enumerate_devices();
	for (int dev_no = 0; dev_no < num_devices; dev_no++) {
		bool found = false;
		for(int slot = 0; slot < MAX_DEVICES; slot++) {
			indigo_device *device = devices[slot];
			if (device == NULL) continue;
			if (!strncmp(PRIVATE_DATA->dev_file_name, fli_file_names[dev_no], MAX_PATH)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		} else {
			assert(fname!=NULL);
			strncpy(fname, fli_file_names[dev_no], MAX_PATH);
			return dev_no;
		}
	}
	return -1;
}


static int find_index_by_device_fname(char *fname) {
	for (int dev_no = 0; dev_no < num_devices; dev_no++) {
		if (!strncmp(fli_file_names[dev_no], fname, MAX_PATH)) {
			return dev_no;
		}
	}
	return -1;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(char *fname) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (!strncmp(PRIVATE_DATA->dev_file_name, fname, 255)) return slot;
	}
	return -1;
}


static int find_unplugged_device(char *fname) {
	enumerate_devices();
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		bool found = false;
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		for (int dev_no = 0; dev_no < num_devices; dev_no++) {
			if (!strncmp(PRIVATE_DATA->dev_file_name, fli_file_names[dev_no], MAX_PATH)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		} else {
			assert(fname!=NULL);
			strncpy(fname, PRIVATE_DATA->dev_file_name, MAX_PATH);
			return slot;
		}
	}
	return -1;
}

static void process_plug_event(indigo_device *unused) {
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	pthread_mutex_lock(&device_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}

	char file_name[MAX_PATH];
	int idx = find_plugged_device(file_name);
	if (idx < 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No FLI Camera plugged.");
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
	sprintf(device->name, "%s #%d", fli_dev_names[idx], slot);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' @ %s attached", device->name , fli_file_names[idx]);
	fli_private_data *private_data = indigo_safe_malloc(sizeof(fli_private_data));
	private_data->dev_id = 0;
	private_data->domain = fli_domains[idx];
	strncpy(private_data->dev_file_name, fli_file_names[idx], MAX_PATH);
	strncpy(private_data->dev_name, fli_dev_names[idx], MAX_PATH);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	pthread_mutex_lock(&device_mutex);
	int slot, id;
	char file_name[MAX_PATH];
	bool removed = false;
	while ((id = find_unplugged_device(file_name)) != -1) {
		slot = find_device_slot(file_name);
		if (slot < 0) continue;
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&device_mutex);
			return;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
		removed = true;
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No FLI Camera unplugged!");
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {

	struct libusb_device_descriptor descriptor;

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor != FLI_VENDOR_ID)
				break;
			indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
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
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}


static libusb_hotplug_callback_handle callback_handle;

extern void (*debug_ext)(int level, char *format, va_list arg);

static void _debug_ext(int level, char *format, va_list arg) {
	char _format[1024];
	snprintf(_format, sizeof(_format), "FLISDK: %s", format);
	if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
		INDIGO_DEBUG_DRIVER(indigo_log_message(_format, arg));
	}
}

indigo_result indigo_wheel_fli(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "FLI Filter Wheel", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		debug_ext = _debug_ext;
		FLISetDebugLevel(NULL, FLIDEBUG_ALL);
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
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
