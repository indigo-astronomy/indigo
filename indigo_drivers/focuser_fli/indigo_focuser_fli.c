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
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

/** INDIGO FLI focuser driver
 \file indigo_focuser_fli.c
 */

#define MAX_PATH                      255     /* Maximal Path Length */

#define DRIVER_NAME		"indigo_focuser_fli"
#define DRIVER_VERSION             0x0001
#define FLI_VENDOR_ID              0x0f18

#define POLL_TIME                       1     /* Seconds */

#define MAX_STEPS_AT_ONCE            4000


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


#define PRIVATE_DATA		((fli_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected            gp_bits

typedef struct {
	flidev_t dev_id;
	char dev_file_name[MAX_PATH];
	char dev_name[MAX_PATH];
	flidomain_t domain;
	long zero_position;
	long steps_to_go;     /* some focusers can not do full extent stpes in one go use this for the second move call */
	indigo_timer *focuser_timer;
	pthread_mutex_t usb_mutex;
} fli_private_data;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void fli_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	long res = FLIClose(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
}


static void focuser_timer_callback(indigo_device *device) {
	long steps_remaining;
	long value;
	flidev_t id = PRIVATE_DATA->dev_id;

	int res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = FLIGetStepperPosition(id, &value);
	value -= PRIVATE_DATA->zero_position;
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepperPosition(%d) = %d", id, res);
		FOCUSER_POSITION_ITEM->number.value = 0;
	} else {
		FOCUSER_POSITION_ITEM->number.value = value;
	}
	res = FLIGetStepsRemaining(id, &steps_remaining);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepsRemaining(%d) = %d", id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_STEPS_ITEM->number.value = steps_remaining + labs(PRIVATE_DATA->steps_to_go);
		if (steps_remaining) {
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, POLL_TIME, focuser_timer_callback);
		} else if (PRIVATE_DATA->steps_to_go) {
			int steps = PRIVATE_DATA->steps_to_go;
			if (labs(steps) > MAX_STEPS_AT_ONCE) {
				int sign = (steps >= 0) ? 1 : -1;
				PRIVATE_DATA->steps_to_go = steps;
				steps = sign * MAX_STEPS_AT_ONCE;
				PRIVATE_DATA->steps_to_go -= steps;
			} else {
				PRIVATE_DATA->steps_to_go = 0;
			}
			res = FLIStepMotorAsync(PRIVATE_DATA->dev_id, steps);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIStepMotorAsync(%d) = %d", id, res);
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, POLL_TIME, focuser_timer_callback);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		/* Use all info property fields */
		INFO_PROPERTY->count = 7;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;

		strncpy(FOCUSER_STEPS_ITEM->label, "Relative move (steps)", INDIGO_VALUE_SIZE);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void fli_focuser_connect(indigo_device *device) {
	flidev_t id;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	long res = FLIOpen(&id, PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	if (!res) {
		PRIVATE_DATA->dev_id = id;
		device->is_connected = true;
		res = FLIGetModel(id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetModel(%d) = %d", id, res);
		}

		/* TODO: Do not home if Atlas Focuser */
		res = FLIHomeDevice(id);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIHomeDevice(%d) = %d", id, res);
		}

		long value;
		//do {
		//	usleep(100000);
		//	res = FLIGetStepsRemaining(id, &value);
		//	if (res) {
		//		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetDeviceStatus(%d) = %d", id, res);
		//	}
		//	//INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser steps left %d", value);
		//} while (value != 0);  /* wait while finding home position */

		do {
			usleep(100000);
			res = FLIGetDeviceStatus(id, &value);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetDeviceStatus(%d) = %d", id, res);
			}
			//INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetDeviceStatus(%d) = %d", id, res);
		} while (value & FLI_FOCUSER_STATUS_MOVING_MASK);  /* wait while moving */

		if (!(value & FLI_FOCUSER_STATUS_HOME)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser home position not found (status = %d)", value);
		}

		res = FLIGetStepperPosition(id, &value);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepperPosition(%d) = %d", id, res);
			value = 0;
		}
		PRIVATE_DATA->zero_position = value;

		res = FLIGetFocuserExtent(id, &value);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetFocuserExtent(%d) = %d", id, res);
			value = 1000;
		}
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Focuser Extent %d", value);
		FOCUSER_POSITION_ITEM->number.max = value;
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.value = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;

		FOCUSER_STEPS_ITEM->number.max = value;
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.value = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;

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

		sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
		sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);

		indigo_update_property(device, INFO_PROPERTY, NULL);

		FOCUSER_STEPS_PROPERTY->state = INDIGO_IDLE_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_IDLE_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);

		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, "Connected");
	} else {
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, "Connect failed!");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	long res;
	flidev_t id;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, "Connecting to focuser, this may take time!");
				//indigo_set_timer(device, 0, fli_focuser_connect);
				fli_focuser_connect(device);
			}
		} else {
			if (device->is_connected) {
				fli_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		res = 0;
		long value = 0;
		long current_value;
		if (FOCUSER_STEPS_ITEM->number.value >= 0) {
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				value = -1 * (long)(FOCUSER_STEPS_ITEM->number.value);
			} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
				value = (long)(FOCUSER_STEPS_ITEM->number.value);
			}

			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = FLIGetStepperPosition(PRIVATE_DATA->dev_id, &current_value);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepperPosition(%d) = %d", id, res);
			}
			current_value -= PRIVATE_DATA->zero_position;

			/* do not go over the max extent */
			if (FOCUSER_POSITION_ITEM->number.max < (current_value + value)) {
				value -= current_value + value - FOCUSER_POSITION_ITEM->number.max;
				FOCUSER_STEPS_ITEM->number.value = (double)labs(value);
			}

			/* do not go below 0 */
			if ((current_value + value) < 0) {
				value -= current_value + value;
				FOCUSER_STEPS_ITEM->number.value = (double)labs(value);
			}

			PRIVATE_DATA->steps_to_go = 0;
			/* focusers with max < 10000 can only go 4095 steps at once */
			if ((FOCUSER_POSITION_ITEM->number.max < 10000) && (labs(value) > MAX_STEPS_AT_ONCE)) {
				int sign = (value >= 0) ? 1 : -1;
				PRIVATE_DATA->steps_to_go = value;
				value = sign * MAX_STEPS_AT_ONCE;
				PRIVATE_DATA->steps_to_go -= value;
			}

			res = FLIStepMotorAsync(PRIVATE_DATA->dev_id, value);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIStepMotorAsync(%d) = %d", id, res);
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			}
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

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
		if ((FOCUSER_POSITION_ITEM->number.value >= 0) &&
		    (FOCUSER_POSITION_ITEM->number.value <= FOCUSER_POSITION_ITEM->number.max)) {
			res = FLIGetStepperPosition(PRIVATE_DATA->dev_id, &value);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepperPosition(%d) = %d", id, res);
			}
			value -= PRIVATE_DATA->zero_position;
			value = FOCUSER_POSITION_ITEM->number.value - value;

			PRIVATE_DATA->steps_to_go = 0;
			/* focusers with max < 10000 can only go 4095 steps at once */
			if ((FOCUSER_POSITION_ITEM->number.max < 10000) && (labs(value) > MAX_STEPS_AT_ONCE)) {
				int sign = (value >= 0) ? 1 : -1;
				PRIVATE_DATA->steps_to_go = value;
				value = sign * MAX_STEPS_AT_ONCE;
				PRIVATE_DATA->steps_to_go -= value;
			}

			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = FLIStepMotorAsync(PRIVATE_DATA->dev_id, value);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIStepMotorAsync(%d) = %d", id, res);
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			}
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, POLL_TIME, focuser_timer_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			PRIVATE_DATA->steps_to_go = 0;
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = FLIStepMotorAsync(PRIVATE_DATA->dev_id, 0);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			if (res) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIStepMotorAsync(%d) = %d", id, res);
			}
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, "Focuser stopped");

		return INDIGO_OK;
	// ---------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached.", device->name);

	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   32

static const flidomain_t enum_domain = FLIDOMAIN_USB | FLIDEVICE_FOCUSER;
static int num_devices = 0;
static char fli_file_names[MAX_DEVICES][MAX_PATH] = {""};
static char fli_dev_names[MAX_DEVICES][MAX_PATH] = {""};
static flidomain_t fli_domains[MAX_DEVICES] = {0};

static indigo_device *devices[MAX_DEVICES] = {NULL};


static void enumerate_devices() {
	/* There is a mem leak heree!!! 8,192 constant + 20 bytes on every new connected device */
	num_devices = 0;
	long res = FLICreateList(enum_domain);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLICreateList(%d) = %d",enum_domain , res);
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
		"", false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	};

	struct libusb_device_descriptor descriptor;
	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DEBUG_DRIVER(int rc =) libusb_get_device_descriptor(dev, &descriptor);
			if (descriptor.idVendor != FLI_VENDOR_ID) break;

			int slot = find_available_device_slot();
			if (slot < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			char file_name[MAX_PATH];
			int idx = find_plugged_device(file_name);
			if (idx < 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No FLI focuser plugged.");
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &focuser_template, sizeof(indigo_device));
			sprintf(device->name, "%s #%d", fli_dev_names[idx], slot);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' @ %s attached.", device->name , fli_file_names[idx]);
			fli_private_data *private_data = malloc(sizeof(fli_private_data));
			assert(private_data);
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
				if (slot < 0)
					continue;
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
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No FLI Focuser unplugged!");
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
		INDIGO_DEBUG_DRIVER(indigo_debug("libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("libusb_hotplug_deregister_callback"));
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
