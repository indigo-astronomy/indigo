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
#include "simulator_test_common.h"

static libusb_hotplug_callback_fn usb_callback;
static int usb_devices[6];
static atomic_int visible_count = 1, attached_mask, attach_attempts, fail_attach;
static atomic_int close_calls, lock_count, open_calls, set_position_calls, requested_slot, current_slot;
static atomic_bool moving;
static const simulator_driver_case efw = { "ZWO ASI Filter Wheel", "indigo_wheel_asi", "EFW SDK test 0", indigo_wheel_asi, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

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
	atomic_fetch_add(&open_calls, 1);
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
	*position = atomic_load(&moving) ? -1 : atomic_load(&current_slot);
	return EFW_SUCCESS;
}

EFW_ERROR_CODE EFWSetPosition(int id, int position) {
	atomic_store(&requested_slot, position);
	atomic_store(&moving, true);
	atomic_fetch_add(&set_position_calls, 1);
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

static void connect_change_slot_disconnect(void) {
	int opened = atomic_load(&open_calls);
	int closed = atomic_load(&close_calls);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, efw.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(true));
	ASSERT_EQ_INT(opened + 1, atomic_load(&open_calls));
	ASSERT_EQ_INT(1, atomic_load(&lock_count));
	ASSERT_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 1, 0));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, efw.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3));
	ASSERT_TRUE(wait_atomic(&set_position_calls, 1));
	ASSERT_EQ_INT(2, atomic_load(&requested_slot));
	ASSERT_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_BUSY_STATE));
	atomic_store(&current_slot, atomic_load(&requested_slot));
	atomic_store(&moving, false);
	ASSERT_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3, 0));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, efw.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(false));
	ASSERT_EQ_INT(closed + 1, atomic_load(&close_calls));
	ASSERT_EQ_INT(0, atomic_load(&lock_count));
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
	reset_simulator_context(&efw);
	indigo_start();
	indigo_attach_client(&simulator_test_client);
	indigo_wheel_asi(INDIGO_DRIVER_INIT, NULL);
	if (!wait_atomic(&attached_mask, 1)) {
		indigo_wheel_asi(INDIGO_DRIVER_SHUTDOWN, NULL);
		indigo_detach_client(&simulator_test_client);
		indigo_stop();
		release_cached_properties();
		return 1;
	}
	const indigo_test_case tests[] = {
		{ "connect, change slot, disconnect", connect_change_slot_disconnect },
		{ "failed attach and full capacity retry", hotplug_capacity_and_failed_attach_retry }
	};
	int result = indigo_run_tests("ASI EFW SDK integration", tests, sizeof(tests) / sizeof(tests[0]));
	if (context.connected) {
		indigo_change_switch_property_1(&simulator_test_client, efw.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
		wait_for_simulator_connection_state(false);
	}
	indigo_wheel_asi(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	return result;
}
