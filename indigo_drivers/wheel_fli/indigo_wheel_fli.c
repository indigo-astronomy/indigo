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
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** INDIGO FLI filter wheel driver
 \file indigo_wheel_fli.c
 */

#define DRIVER_VERSION 0x0001

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
#include <libfli/libfli.h>
#include "indigo_driver_xml.h"
#include "indigo_wheel_fli.h"

#define FLI_VENDOR_ID                   0x0f18

#define PRIVATE_DATA        ((fli_private_data *)device->private_data)

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
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	long res = FLISetFilterPos(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_slot-1);
	if (res) {
		INDIGO_ERROR(indigo_error("indigo_wheel_fli: FLISetFilterPos(%d) = %d", PRIVATE_DATA->dev_id, res));
	}

	res = FLIGetFilterPos(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_slot));
	if (res) {
		INDIGO_ERROR(indigo_error("indigo_wheel_fli: FLIGetFilterPos(%d) = %d", PRIVATE_DATA->dev_id, res));
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
	if (indigo_wheel_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		/* Use all info property fields */
		INFO_PROPERTY->count = 7;

		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);

		int index = find_index_by_device_fname(PRIVATE_DATA->dev_file_name);
		if (index < 0) {
			return INDIGO_NOT_FOUND;
		}

		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);

			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			long res = FLIOpen(&(PRIVATE_DATA->dev_id), PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (!res) {
				flidev_t id = PRIVATE_DATA->dev_id;
				long int num_slots;

				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

				FLIGetFilterCount(id, &num_slots);
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->count = (int)num_slots;
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
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetModel(%d) = %d", id, res));
				}

				res = FLIGetSerialString(id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
				if (res) {
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetSerialString(%d) = %d", id, res));
				}

				long hw_rev, fw_rev;
				res = FLIGetFWRevision(id, &fw_rev);
				if (res) {
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetFWRevision(%d) = %d", id, res));
				}

				res = FLIGetHWRevision(id, &hw_rev);
				if (res) {
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetHWRevision(%d) = %d", id, res));
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
				sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);
				indigo_update_property(device, INFO_PROPERTY, NULL);

				WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				indigo_set_timer(device, 0, wheel_timer_callback);

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				INDIGO_ERROR(indigo_error("indigo_wheel_fli: FLIOpen(%d) = %d", PRIVATE_DATA->dev_id, res));
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				return INDIGO_FAILED;
			}
		} else {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			long res = FLIClose(PRIVATE_DATA->dev_id);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res) {
				INDIGO_ERROR(indigo_error("indigo_wheel_fli: FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res));
			}
			PRIVATE_DATA->dev_id = -1;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		}

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
			PRIVATE_DATA->wheel_timer = indigo_set_timer(device, 0, wheel_timer_callback);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}


static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(NULL, device->name);
	INDIGO_LOG(indigo_log("indigo_wheel_fli: '%s' detached.", device->name));
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

static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


static void enumerate_devices() {
	/* There is a mem leak heree!!! 8,192 constant + 12 bytes on every new connected device */
	num_devices = 0;
	long res = FLICreateList(enum_domain);
	if (res) {
		INDIGO_ERROR(indigo_error("indigo_wheel_fli: FLICreateList(%d) = %d",enum_domain , res));
	}
	if(FLIListFirst(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) {
		do {
			num_devices++;
		} while((FLIListNext(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) && (num_devices < MAX_DEVICES));
	}
	FLIDeleteList();
	/* FOR DEBUG only!
	FLICreateList(FLIDOMAIN_USB | FLIDEVICE_CAMERA);
	if(FLIListFirst(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) {
		do {
			num_devices++;
		} while((FLIListNext(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) && (num_devices < MAX_DEVICES));
	}
	FLIDeleteList();
	*/
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


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {

	static indigo_device wheel_template = {
		"", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		wheel_detach
	};

	struct libusb_device_descriptor descriptor;

	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor != FLI_VENDOR_ID) break;

			int slot = find_available_device_slot();
			if (slot < 0) {
				INDIGO_ERROR(indigo_error("indigo_wheel_fli: No available device slots available."));
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			char file_name[MAX_PATH];
			int idx = find_plugged_device(file_name);
			if (idx < 0) {
				INDIGO_DEBUG(indigo_debug("indigo_wheel_fli: No FLI FW plugged."));
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &wheel_template, sizeof(indigo_device));
			sprintf(device->name, "%s #%d", fli_dev_names[idx], slot);
			INDIGO_LOG(indigo_log("indigo_wheel_fli: '%s' @ %s attached.", device->name , fli_file_names[idx]));
			fli_private_data *private_data = malloc(sizeof(fli_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(fli_private_data));
			private_data->dev_id = 0;
			private_data->domain = fli_domains[idx];
			strncpy(private_data->dev_file_name, fli_file_names[idx], MAX_PATH);
			strncpy(private_data->dev_name, fli_dev_names[idx], MAX_PATH);
			device->private_data = private_data;
			indigo_async((void *)(void *)indigo_attach_device, device);
			devices[slot]=device;
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			int slot, id;
			char file_name[MAX_PATH];
			bool removed = false;
			while ((id = find_unplugged_device(file_name)) != -1) {
				slot = find_device_slot(file_name);
				if (slot < 0) continue;
				indigo_device **device = &devices[slot];
				if (*device == NULL) {
					pthread_mutex_unlock(&device_mutex);
					return 0;
				}
				indigo_detach_device(*device);
				free((*device)->private_data);
				free(*device);
				libusb_unref_device(dev);
				*device = NULL;
				removed = true;
			}
			if (!removed) {
				INDIGO_DEBUG(indigo_debug("indigo_wheel_fli: No FLI FW unplugged!"));
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
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

indigo_result indigo_wheel_fli(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "FLI Filter Wheel", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_wheel_fli: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_wheel_fli: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
