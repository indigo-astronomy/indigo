// Copyright (c) 2017 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen Bogdanovski <rumen@skyarchive.org>

/** INDIGO FLI focuser driver
 \file indigo_focuser_fli.c
 */

#define MAX_PATH                      255     /* Maximal Path Length */

#define DRIVER_VERSION             0x0001

#define FLI_VENDOR_ID              0x0f18

#define POLL_TIME                       1     /* Seconds */


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

#include <libfli/libfli.h>

#include "indigo_driver_xml.h"
#include "indigo_focuser_fli.h"


#undef PRIVATE_DATA
#define PRIVATE_DATA													((fli_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
	flidev_t dev_id;
	char dev_file_name[MAX_PATH];
	char dev_name[MAX_PATH];
	flidomain_t domain;
	indigo_timer *focuser_timer;
	pthread_mutex_t usb_mutex;
} fli_private_data;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void fli_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	long res = FLIClose(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_focuser_fli: FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res));
	}
}


static void focuser_timer_callback(indigo_device *device) {
	long steps_remaining;
	long value;
	flidev_t id = PRIVATE_DATA->dev_id;

	int res;
	res = FLIGetStepperPosition(id, &value);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetStepperPosition(%d) = %d", id, res));
		FOCUSER_POSITION_ITEM->number.value = 0;
	} else {
		FOCUSER_POSITION_ITEM->number.value = value;
	}

	res = FLIGetStepsRemaining(id, &steps_remaining);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetStepperPosition(%d) = %d", id, res));
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_STEPS_ITEM->number.value = steps_remaining;
		if (steps_remaining) {
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, POLL_TIME, focuser_timer_callback);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	fli_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		/* Use all info property fields */
		INFO_PROPERTY->count = 7;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;

		strncpy(FOCUSER_STEPS_ITEM->label, "Relative move (steps)", INDIGO_VALUE_SIZE);
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	long res;
	flidev_t id;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = FLIOpen(&id, PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
			if (!res) {
				PRIVATE_DATA->dev_id = id;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

				res = FLIGetModel(id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetModel(%d) = %d", id, res));
				}

				/* TODO: Do not home if Atlas Focuser */
				res = FLIHomeDevice(id);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIHomeDevice(%d) = %d", id, res));
				}

				long value;
				do {
					usleep(10000);
					res = FLIGetDeviceStatus(id, &value);
					if (res) {
						INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetDeviceStatus(%d) = %d", id, res));
					}
				} while (value != FLI_FOCUSER_STATUS_HOME);  /* wait while finding home position */

				res = FLIGetStepperPosition(id, &value);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetStepperPosition(%d) = %d", id, res));
				}
				FOCUSER_POSITION_ITEM->number.value = value;

				res = FLIGetFocuserExtent(id, &value);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetFocuserExtent(%d) = %d", id, res));
					value = 1000;
				}
				INDIGO_LOG(indigo_log("indigo_ccd_fli: Focuse Extent %d", value));
				FOCUSER_POSITION_ITEM->number.max = value;
				FOCUSER_POSITION_ITEM->number.min = 0;
				FOCUSER_POSITION_ITEM->number.step = 1;

				res = FLIGetSerialString(id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetSerialString(%d) = %d", id, res));
				}

				long hw_rev, fw_rev;
				res = FLIGetFWRevision(id, &fw_rev);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetFWRevision(%d) = %d", id, res));
				}

				res = FLIGetHWRevision(id, &hw_rev);
				if (res) {
					INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetHWRevision(%d) = %d", id, res));
				}

				sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
				sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);

				indigo_update_property(device, INFO_PROPERTY, NULL);

			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		} else {
			fli_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		res = 0;
		long value = 0;
		if (FOCUSER_STEPS_ITEM->number.value >= 0) {
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				value = -1 * (long)(FOCUSER_STEPS_ITEM->number.value);
			} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
				value = (long)(FOCUSER_STEPS_ITEM->number.value);
			}

			res = FLIStepMotorAsync(PRIVATE_DATA->dev_id, value);
			if (res) {
				INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIStepMotorAsync(%d) = %d", id, res));
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, POLL_TIME, focuser_timer_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		res = 0;
		long value = 0;
		if (FOCUSER_POSITION_ITEM->number.value >= 0) {
			res = FLIGetStepperPosition(PRIVATE_DATA->dev_id, &value);
			if (res) {
				INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIGetStepperPosition(%d) = %d", id, res));
			}

			value = FOCUSER_POSITION_ITEM->number.value - value;
			res = FLIStepMotorAsync(PRIVATE_DATA->dev_id, value);
			if (res) {
				INDIGO_LOG(indigo_log("indigo_ccd_fli: FLIStepMotorAsync(%d) = %d", id, res));
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, POLL_TIME, focuser_timer_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
			//libfcusb_stop(PRIVATE_DATA->device_context);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, "FLI does not support ABORT_MORION");
		return INDIGO_OK;
	// ---------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_LOG(indigo_log("%s detached", device->name));

	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   32

static const flidomain_t enum_domain = FLIDOMAIN_USB | FLIDEVICE_FOCUSER;
static int num_devices = 0;
static char fli_file_names[MAX_DEVICES][MAX_PATH] = {""};
static char fli_dev_names[MAX_DEVICES][MAX_PATH] = {""};
static flidomain_t fli_domains[MAX_DEVICES] = {0};

static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


static void enumerate_devices() {
	/* There is a mem leak heree!!! 8,192 constant + 20 bytes on every new connected device */
	num_devices = 0;
	long res = FLICreateList(enum_domain);
	if (res) {
		INDIGO_LOG(indigo_log("indigo_focuser_fli: FLICreateList(%d) = %d",enum_domain , res));
	}
	if(FLIListFirst(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) {
		do {
			num_devices++;
		} while((FLIListNext(&fli_domains[num_devices], fli_file_names[num_devices], MAX_PATH, fli_dev_names[num_devices], MAX_PATH) == 0) && (num_devices < MAX_DEVICES));
	}
	FLIDeleteList();
	/* FOR DEBUG only!
	FLICreateList(FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL);
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

	static indigo_device focuser_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		focuser_detach
	};

	struct libusb_device_descriptor descriptor;

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor != FLI_VENDOR_ID) break;

			int slot = find_available_device_slot();
			if (slot < 0) {
				INDIGO_LOG(indigo_log("indigo_focuser_fli: No available device slots available."));
				return 0;
			}

			char file_name[MAX_PATH];
			int idx = find_plugged_device(file_name);
			if (idx < 0) {
				INDIGO_DEBUG(indigo_debug("indigo_focuser_fli: No FLI Camera plugged."));
				return 0;
			}

			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &focuser_template, sizeof(indigo_device));
			sprintf(device->name, "%s #%d", fli_dev_names[idx], slot);
			INDIGO_LOG(indigo_log("indigo_focuser_fli: '%s' @ %s attached.", device->name , fli_file_names[idx]));
			device->device_context = malloc(sizeof(fli_private_data));
			assert(device->device_context);
			memset(device->device_context, 0, sizeof(fli_private_data));
			((fli_private_data*)device->device_context)->dev_id = 0;
			((fli_private_data*)device->device_context)->domain = fli_domains[idx];
			strncpy(((fli_private_data*)device->device_context)->dev_file_name, fli_file_names[idx], MAX_PATH);
			strncpy(((fli_private_data*)device->device_context)->dev_name, fli_dev_names[idx], MAX_PATH);
			indigo_attach_device(device);
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
				if (*device == NULL)
					return 0;
				indigo_detach_device(*device);
				free((*device)->device_context);
				free(*device);
				libusb_unref_device(dev);
				*device = NULL;
				removed = true;
			}
			if (!removed) {
				INDIGO_DEBUG(indigo_debug("indigo_focuser_fli: No FLI Camera unplugged!"));
			}
		}
	}
	return 0;
};


static void remove_all_devices() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL) continue;
		indigo_detach_device(*device);
		free((*device)->device_context);
		free(*device);
		*device = NULL;
	}
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_focuser_fli(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "FLI Focuser", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, FLI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_focuser_fli: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_focuser_fli: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
