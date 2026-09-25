// Copyright (C) 2016-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_wheel_fli.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <libfli.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_wheel_fli.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000D
#define DRIVER_NAME          "indigo_wheel_fli"
#define DRIVER_LABEL         "FLI Filter Wheel"
#define WHEEL_DEVICE_NAME    "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((fli_private_data *)device->private_data)

//+ define

#define FLI_VENDOR_ID        0x0f18
#define FLI_ENUM_DOMAIN      (FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL)
#define FLI_MAX_ENUMERATED   32

//- define

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	//+ data
	flidev_t dev_id;
	char dev_file_name[PATH_MAX];
	char dev_name[PATH_MAX];
	flidomain_t domain;
	long slot_count;
	long current_slot;
	//- data
} fli_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

// The SDK enumerates through a list that has to be created, walked and
// deleted; the results are kept here for the plug and unplug handlers,
// which the generated driver queue serialises.
static flidomain_t enumerated_domains[FLI_MAX_ENUMERATED];
static char enumerated_file_names[FLI_MAX_ENUMERATED][PATH_MAX];
static char enumerated_device_names[FLI_MAX_ENUMERATED][PATH_MAX];
static int enumerated_count;

static int fli_enumerate(void) {
	enumerated_count = 0;
	long result = FLICreateList(FLI_ENUM_DOMAIN);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLICreateList(%d) = %ld", FLI_ENUM_DOMAIN, result);
		return 0;
	}
	result = FLIListFirst(enumerated_domains, enumerated_file_names[0], PATH_MAX, enumerated_device_names[0], PATH_MAX);
	while (result == 0) {
		enumerated_count++;
		if (enumerated_count == FLI_MAX_ENUMERATED) {
			break;
		}
		result = FLIListNext(enumerated_domains + enumerated_count, enumerated_file_names[enumerated_count], PATH_MAX, enumerated_device_names[enumerated_count], PATH_MAX);
	}
	FLIDeleteList();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d device(s) enumerated", enumerated_count);
	return enumerated_count;
}

static bool fli_is_enumerated(const char *file_name) {
	int count = fli_enumerate();
	for (int i = 0; i < count; i++) {
		if (!strncmp(enumerated_file_names[i], file_name, PATH_MAX)) {
			return true;
		}
	}
	return false;
}

// Everything the wheel needs is read here, so a device that cannot be
// identified releases the SDK handle inside the failed attempt.
static bool fli_open(indigo_device *device) {
	long result = FLIOpen(&PRIVATE_DATA->dev_id, PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIOpen('%s') = %ld", PRIVATE_DATA->dev_file_name, result);
		return false;
	}
	long slot_count = 0;
	if (FLIGetFilterCount(PRIVATE_DATA->dev_id, &slot_count) != 0 || slot_count <= 0 || slot_count > WHEEL_SLOT_NAME_PROPERTY->allocated_count || slot_count > WHEEL_SLOT_OFFSET_PROPERTY->allocated_count) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetFilterCount(%ld) reported %ld slots", (long)PRIVATE_DATA->dev_id, slot_count);
		FLIClose(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
		return false;
	}
	long position = -1;
	if (FLIGetFilterPos(PRIVATE_DATA->dev_id, &position) != 0 || position < 0) {
		// An uninitialised wheel reports a negative position and has to
		// be sent home before it can be used.
		if (FLISetFilterPos(PRIVATE_DATA->dev_id, 0) != 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetFilterPos(%ld, 0) failed", (long)PRIVATE_DATA->dev_id);
			FLIClose(PRIVATE_DATA->dev_id);
			PRIVATE_DATA->dev_id = -1;
			return false;
		}
		position = 0;
	}
	PRIVATE_DATA->slot_count = slot_count;
	PRIVATE_DATA->current_slot = position + 1;
	WHEEL_SLOT_ITEM->number.min = 1;
	WHEEL_SLOT_ITEM->number.max = slot_count;
	WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = (int)slot_count;
	WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot;
	if (FLIGetModel(PRIVATE_DATA->dev_id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE) != 0) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->dev_name);
	}
	if (FLIGetSerialString(PRIVATE_DATA->dev_id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE) != 0) {
		INFO_DEVICE_SERIAL_NUM_ITEM->text.value[0] = 0;
	}
	long firmware_revision = 0, hardware_revision = 0;
	if (FLIGetFWRevision(PRIVATE_DATA->dev_id, &firmware_revision) == 0) {
		snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%ld", firmware_revision);
	} else {
		INFO_DEVICE_FW_REVISION_ITEM->text.value[0] = 0;
	}
	if (FLIGetHWRevision(PRIVATE_DATA->dev_id, &hardware_revision) == 0) {
		snprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%ld", hardware_revision);
	} else {
		INFO_DEVICE_HW_REVISION_ITEM->text.value[0] = 0;
	}
	indigo_update_property(device, INFO_PROPERTY, NULL);
	return true;
}

static void fli_close(indigo_device *device) {
	long result = FLIClose(PRIVATE_DATA->dev_id);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIClose(%ld) = %ld", (long)PRIVATE_DATA->dev_id, result);
	}
	PRIVATE_DATA->dev_id = -1;
}

//- code

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = fli_open(device);
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ wheel.on_disconnect
		PRIVATE_DATA->slot_count = 0;
		PRIVATE_DATA->current_slot = 0;
		//- wheel.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			WHEEL_SLOT_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		fli_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	//+ wheel.WHEEL_SLOT.on_change
	long slot = (long)WHEEL_SLOT_ITEM->number.target;
	if (slot == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_ITEM->number.value = slot;
	} else {
		long result = FLISetFilterPos(PRIVATE_DATA->dev_id, slot - 1);
		if (result) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLISetFilterPos(%ld, %ld) = %ld", (long)PRIVATE_DATA->dev_id, slot - 1, result);
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		// The reached position is always read back, so a wheel that
		// stopped somewhere else is reported rather than assumed.
		long position = 0;
		if (FLIGetFilterPos(PRIVATE_DATA->dev_id, &position) != 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetFilterPos(%ld) failed", (long)PRIVATE_DATA->dev_id);
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->current_slot = position + 1;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
			if (PRIVATE_DATA->current_slot != slot) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	//- wheel.WHEEL_SLOT.on_change
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 8;
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, wheel_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

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
	fli_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (fli_private_data *)indigo_safe_malloc(sizeof(fli_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == FLI_VENDOR_ID)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		int count = fli_enumerate();
		for (int i = 0; i < count; i++) {
			bool attached = false;
			for (int slot = 0; slot < MAX_DEVICES; slot++) {
				if (devices[slot] != NULL && !strncmp(((fli_private_data *)devices[slot]->private_data)->dev_file_name, enumerated_file_names[i], PATH_MAX)) {
					attached = true;
					break;
				}
			}
			if (attached) {
				continue;
			}
			private_data->dev_id = -1;
			private_data->domain = enumerated_domains[i];
			snprintf(private_data->dev_file_name, PATH_MAX, "%s", enumerated_file_names[i]);
			snprintf(private_data->dev_name, PATH_MAX, "%s", enumerated_device_names[i]);
			snprintf(name, INDIGO_NAME_SIZE, "%s", enumerated_device_names[i]);
			indigo_make_name_unique(name, "%s", enumerated_file_names[i]);
			plug_result = true;
			break;
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *wheel = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		wheel->private_data = private_data;
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s", name);
		bool wheel_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = wheel;
				if (indigo_attach_device(wheel) == INDIGO_OK) {
					dev_ref_transferred = true;
					wheel_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!wheel_attached) {
			indigo_safe_free(wheel);
		}
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	fli_private_data *private_data = NULL;
	fli_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (!unplug_result && last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				unplug_result = !fli_is_enumerated(private_data->dev_file_name);
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
				bool recorded = false;
				for (int k = 0; k < removed_count; k++) {
					if (removed[k] == private_data) {
						recorded = true;
						break;
					}
				}
				if (!recorded) {
					removed[removed_count++] = private_data;
				}
			}
		}
	}
	for (int k = 0; k < removed_count; k++) {
		libusb_unref_device(removed[k]->usbdev);
		indigo_safe_free(removed[k]);
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

indigo_result indigo_wheel_fli(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
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

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
