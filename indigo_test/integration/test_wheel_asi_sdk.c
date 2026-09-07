// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdbool.h>
#include <stdatomic.h>
#include <EFW_filter.h>
#include <indigo/indigo_usb_utils.h>
#include <indigo_drivers/wheel_asi/indigo_wheel_asi.h>
#include "../test_runner.h"

static libusb_hotplug_callback_fn usb_callback;
static int usb_devices[6];
static atomic_int visible_count = 1, attached_mask, attach_attempts, fail_attach;
static atomic_int close_calls, lock_count;

static int device_index(indigo_device *device) {
	int index = -1;
	sscanf(device->name, "EFW SDK test %d", &index);
	return index;
}

indigo_result efw_test_attach(indigo_device *device) {
	atomic_fetch_add(&attach_attempts, 1);
	if (atomic_exchange(&fail_attach, 0)) {
		return INDIGO_FAILED;
	}
	indigo_result result = indigo_attach_device(device);
	if (result == INDIGO_OK) {
		atomic_fetch_or(&attached_mask, 1 << device_index(device));
	}
	return result;
}

indigo_result efw_test_detach(indigo_device *device) {
	int index = device_index(device);
	indigo_result result = indigo_detach_device(device);
	atomic_fetch_and(&attached_mask, ~(1 << index));
	return result;
}

void efw_test_usb_start(void) {
}

indigo_result efw_test_lock(indigo_device *device) {
	atomic_fetch_add(&lock_count, 1);
	return INDIGO_OK;
}

indigo_result efw_test_unlock(indigo_device *device) {
	atomic_fetch_sub(&lock_count, 1);
	return INDIGO_OK;
}

int LIBUSB_CALL efw_test_usb_register(libusb_context *ctx, int events, int flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	usb_callback = callback;
	callback(NULL, (libusb_device *)&usb_devices[0], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	return LIBUSB_SUCCESS;
}

int efw_test_usb_register_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	return efw_test_usb_register(ctx, events, flags, vid, pid, cls, callback, data, handle);
}

int efw_test_usb_deregister_poll(libusb_context *ctx, libusb_hotplug_callback_handle handle) {
	return LIBUSB_SUCCESS;
}

void LIBUSB_CALL efw_test_usb_deregister(libusb_context *ctx, libusb_hotplug_callback_handle handle) {
}

libusb_device *LIBUSB_CALL efw_test_usb_ref(libusb_device *device) {
	return device;
}

void LIBUSB_CALL efw_test_usb_unref(libusb_device *device) {
}

int LIBUSB_CALL efw_test_usb_descriptor(libusb_device *device, struct libusb_device_descriptor *descriptor) {
	memset(descriptor, 0, sizeof(*descriptor));
	descriptor->idVendor = 0x03c3;
	descriptor->idProduct = 0x1f10;
	return LIBUSB_SUCCESS;
}

int EFWGetNum(void) {
	return atomic_load(&visible_count);
}

EFW_ERROR_CODE EFWGetID(int index, int *id) {
	if (usb_devices[index] == -1) {
		return EFW_ERROR_INVALID_ID;
	}
	*id = index;
	return EFW_SUCCESS;
}

EFW_ERROR_CODE EFWOpen(int id) {
	return EFW_SUCCESS;
}

EFW_ERROR_CODE EFWClose(int id) {
	atomic_fetch_add(&close_calls, 1);
	return EFW_SUCCESS;
}

const char *EFWGetSDKVersion(void) {
	return "test SDK";
}

EFW_ERROR_CODE EFWGetProperty(int id, EFW_INFO *info) {
	memset(info, 0, sizeof(*info));
	info->ID = id;
	info->slotNum = 5;
	snprintf(info->Name, sizeof(info->Name), "EFW SDK test %d", id);
	return EFW_SUCCESS;
}

int EFWGetProductIDs(int *ids) {
	ids[0] = 0x1f10;
	return 1;
}

EFW_ERROR_CODE EFWGetPosition(int id, int *position) {
	*position = 0;
	return EFW_SUCCESS;
}

EFW_ERROR_CODE EFWSetPosition(int id, int position) {
	return EFW_SUCCESS;
}

EFW_ERROR_CODE EFWCalibrate(int id) {
	return EFW_SUCCESS;
}

EFW_ERROR_CODE EFWSetID(int id, EFW_ID alias) {
	return EFW_SUCCESS;
}

static bool wait_atomic(atomic_int *value, int expected) {
	for (int i = 0; i < 200; i++) {
		if (atomic_load(value) == expected) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static void hotplug_capacity_and_failed_attach_retry(void) {
	atomic_store(&visible_count, 2);
	atomic_store(&fail_attach, 1);
	int attempts = atomic_load(&attach_attempts);
	usb_callback(NULL, (libusb_device *)&usb_devices[1], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&attach_attempts, attempts + 1));
	usb_callback(NULL, (libusb_device *)&usb_devices[1], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&attached_mask, 3));
	for (int i = 2; i < 5; i++) {
		atomic_store(&visible_count, i + 1);
		usb_callback(NULL, (libusb_device *)&usb_devices[i], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		ASSERT_TRUE(wait_atomic(&attached_mask, (1 << (i + 1)) - 1));
	}
	atomic_store(&visible_count, 6);
	int closed = atomic_load(&close_calls);
	usb_callback(NULL, (libusb_device *)&usb_devices[5], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&close_calls, closed + 1));
	usb_callback(NULL, (libusb_device *)&usb_devices[0], LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	ASSERT_TRUE(wait_atomic(&attached_mask, 30));
	// Keep SDK enumeration from advertising the removed ID during the replacement arrival.
	usb_devices[0] = -1;
	usb_callback(NULL, (libusb_device *)&usb_devices[5], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&attached_mask, 62));
}

int main(void) {
	indigo_start();
	indigo_wheel_asi(INDIGO_DRIVER_INIT, NULL);
	if (!wait_atomic(&attached_mask, 1)) {
		indigo_wheel_asi(INDIGO_DRIVER_SHUTDOWN, NULL);
		indigo_stop();
		return 1;
	}
	const indigo_test_case tests[] = {
		{ "failed attach and full capacity retry", hotplug_capacity_and_failed_attach_retry }
	};
	int result = indigo_run_tests("ASI EFW SDK hotplug", tests, sizeof(tests) / sizeof(tests[0]));
	indigo_wheel_asi(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return result;
}
