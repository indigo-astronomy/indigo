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

// This file generated from indigo_focuser_fli.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <libfli.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_focuser_fli.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_focuser_fli"
#define DRIVER_LABEL         "FLI Focuser"
#define FOCUSER_DEVICE_NAME  "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((fli_private_data *)device->private_data)

//+ define

#define FLI_VENDOR_ID        0x0f18
#define FLI_ENUM_DOMAIN      (FLIDOMAIN_USB | FLIDEVICE_FOCUSER)
#define FLI_MAX_ENUMERATED   32
// Focusers with a short travel accept at most this many steps per command.
#define FLI_MAX_STEPS_AT_ONCE 4000
#define FLI_POLL_DELAY       0.5
#define FLI_HOME_TIMEOUT_CYCLES 300

//- define

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	//+ data
	flidev_t dev_id;
	char dev_file_name[PATH_MAX];
	char dev_name[PATH_MAX];
	flidomain_t domain;
	long zero_position;
	long target_position;
	// Some focusers accept only a limited number of steps per command, so a
	// long move is issued in chunks and this holds what is still owed.
	long steps_to_go;
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

static void motion_finalizer(indigo_device *device);

static void fli_motion_state(indigo_device *device, indigo_property_state state) {
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

// Everything the focuser needs is read here, so a device that cannot be
// identified releases the SDK handle inside the failed attempt.
static bool fli_open(indigo_device *device) {
	long result = FLIOpen(&PRIVATE_DATA->dev_id, PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIOpen('%s') = %ld", PRIVATE_DATA->dev_file_name, result);
		return false;
	}
	if (FLIGetModel(PRIVATE_DATA->dev_id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE) != 0) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->dev_name);
	}
	bool homed = FLIHomeDevice(PRIVATE_DATA->dev_id) == 0;
	if (homed) {
		// Homing is bounded: a focuser that never reports the end of it
		// must not hold the connection forever.
		long status = 0;
		homed = false;
		for (int i = 0; i < FLI_HOME_TIMEOUT_CYCLES; i++) {
			if (FLIGetDeviceStatus(PRIVATE_DATA->dev_id, &status) != 0) {
				break;
			}
			if (!(status & FLI_FOCUSER_STATUS_MOVING_MASK)) {
				homed = (status & FLI_FOCUSER_STATUS_HOME) != 0;
				break;
			}
			indigo_usleep(100000);
		}
	}
	if (!homed) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser home position not found");
		FLIClose(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
		return false;
	}
	long position = 0, extent = 0;
	if (FLIGetStepperPosition(PRIVATE_DATA->dev_id, &position) != 0 || FLIGetFocuserExtent(PRIVATE_DATA->dev_id, &extent) != 0 || extent <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser position or extent could not be read");
		FLIClose(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
		return false;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Focuser extent %ld", extent);
	PRIVATE_DATA->zero_position = position;
	PRIVATE_DATA->target_position = 0;
	PRIVATE_DATA->steps_to_go = 0;
	FOCUSER_POSITION_ITEM->number.min = 0;
	FOCUSER_POSITION_ITEM->number.max = extent;
	FOCUSER_POSITION_ITEM->number.step = 1;
	FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = 0;
	FOCUSER_STEPS_ITEM->number.min = 0;
	FOCUSER_STEPS_ITEM->number.max = extent;
	FOCUSER_STEPS_ITEM->number.step = 1;
	FOCUSER_STEPS_ITEM->number.value = FOCUSER_STEPS_ITEM->number.target = 0;
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

// Issues at most FLI_MAX_STEPS_AT_ONCE steps and remembers the rest.
static bool fli_step(indigo_device *device, long steps) {
	PRIVATE_DATA->steps_to_go = 0;
	if (labs(steps) > FLI_MAX_STEPS_AT_ONCE) {
		long sign = steps >= 0 ? 1 : -1;
		PRIVATE_DATA->steps_to_go = steps - sign * FLI_MAX_STEPS_AT_ONCE;
		steps = sign * FLI_MAX_STEPS_AT_ONCE;
	}
	long result = FLIStepMotorAsync(PRIVATE_DATA->dev_id, steps);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIStepMotorAsync(%ld, %ld) = %ld", (long)PRIVATE_DATA->dev_id, steps, result);
		PRIVATE_DATA->steps_to_go = 0;
		return false;
	}
	return true;
}

static void fli_start_motion(indigo_device *device, long target) {
	long position = 0;
	if (FLIGetStepperPosition(PRIVATE_DATA->dev_id, &position) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepperPosition(%ld) failed", (long)PRIVATE_DATA->dev_id);
		fli_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	position -= PRIVATE_DATA->zero_position;
	if (target < FOCUSER_POSITION_ITEM->number.min) {
		target = (long)FOCUSER_POSITION_ITEM->number.min;
	} else if (target > FOCUSER_POSITION_ITEM->number.max) {
		target = (long)FOCUSER_POSITION_ITEM->number.max;
	}
	PRIVATE_DATA->target_position = target;
	FOCUSER_POSITION_ITEM->number.value = position;
	FOCUSER_POSITION_ITEM->number.target = target;
	FOCUSER_STEPS_ITEM->number.value = labs(target - position);
	if (target == position) {
		fli_motion_state(device, INDIGO_OK_STATE);
		return;
	}
	if (!fli_step(device, target - position)) {
		fli_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	fli_motion_state(device, INDIGO_BUSY_STATE);
	indigo_execute_handler_in(device, FLI_POLL_DELAY, motion_finalizer);
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	long position = 0, steps_remaining = 0;
	if (FLIGetStepperPosition(PRIVATE_DATA->dev_id, &position) != 0 || FLIGetStepsRemaining(PRIVATE_DATA->dev_id, &steps_remaining) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser progress could not be read");
		PRIVATE_DATA->steps_to_go = 0;
		fli_motion_state(device, INDIGO_ALERT_STATE);
		return;
	}
	position -= PRIVATE_DATA->zero_position;
	FOCUSER_POSITION_ITEM->number.value = position;
	FOCUSER_STEPS_ITEM->number.value = labs(steps_remaining) + labs(PRIVATE_DATA->steps_to_go);
	if (steps_remaining != 0) {
		fli_motion_state(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, FLI_POLL_DELAY, motion_finalizer);
	} else if (PRIVATE_DATA->steps_to_go != 0) {
		if (fli_step(device, PRIVATE_DATA->steps_to_go)) {
			fli_motion_state(device, INDIGO_BUSY_STATE);
			indigo_execute_handler_in(device, FLI_POLL_DELAY, motion_finalizer);
		} else {
			fli_motion_state(device, INDIGO_ALERT_STATE);
		}
	} else {
		fli_motion_state(device, position == PRIVATE_DATA->target_position ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
	}
}

static void fli_close(indigo_device *device) {
	long result = FLIClose(PRIVATE_DATA->dev_id);
	if (result) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIClose(%ld) = %ld", (long)PRIVATE_DATA->dev_id, result);
	}
	PRIVATE_DATA->dev_id = -1;
}

//- code

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
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
		//+ focuser.on_disconnect
		PRIVATE_DATA->steps_to_go = 0;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		//- focuser.on_disconnect
		fli_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	fli_start_motion(device, (long)FOCUSER_POSITION_ITEM->number.target);
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	long steps = (long)FOCUSER_STEPS_ITEM->number.target;
	long position = 0;
	if (FLIGetStepperPosition(PRIVATE_DATA->dev_id, &position) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FLIGetStepperPosition(%ld) failed", (long)PRIVATE_DATA->dev_id);
		fli_motion_state(device, INDIGO_ALERT_STATE);
	} else {
		position -= PRIVATE_DATA->zero_position;
		fli_start_motion(device, FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? position - steps : position + steps);
	}
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		// An urgent abort can overtake a move that is still queued.
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		PRIVATE_DATA->steps_to_go = 0;
		// A zero step move is how the SDK stops the motor.
		long result = FLIStepMotorAsync(PRIVATE_DATA->dev_id, 0);
		long position = 0;
		if (result != 0 || FLIGetStepperPosition(PRIVATE_DATA->dev_id, &position) != 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser could not be stopped");
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			fli_motion_state(device, INDIGO_ALERT_STATE);
		} else {
			position -= PRIVATE_DATA->zero_position;
			PRIVATE_DATA->target_position = position;
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
			FOCUSER_STEPS_ITEM->number.value = 0;
			fli_motion_state(device, INDIGO_OK_STATE);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(FOCUSER_STEPS_ITEM->label, "Relative move (steps)");
		//- focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_STEPS_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

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
		indigo_device *focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
		focuser->private_data = private_data;
		snprintf(focuser->name, INDIGO_NAME_SIZE, "%s", name);
		bool focuser_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = focuser;
				if (indigo_attach_device(focuser) == INDIGO_OK) {
					dev_ref_transferred = true;
					focuser_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!focuser_attached) {
			indigo_safe_free(focuser);
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
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
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

indigo_result indigo_focuser_fli(indigo_driver_action action, indigo_driver_info *info) {

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
