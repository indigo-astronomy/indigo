// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <stdatomic.h>
#include <stdint.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_usb_utils.h>
#include <indigo/indigo_uni_io.h>
#include <libdsusb.h>
#include <libfcusb.h>
#include <libgpusb.h>
#include <libatik.h>
#include "serial_simulator_test_common.h"
#include "abort_queue_test_common.h"

extern indigo_result TEST_ENTRY(indigo_driver_action, indigo_driver_info *);
static const simulator_driver_case driver = { TEST_NAME, "USB output", TEST_NAME, TEST_ENTRY, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static atomic_int references, attached, opened, closed, io_calls, invalid_io, fail_queue, fail_register, fail_attach, fail_open, fail_io;
static atomic_int fail_stop, fail_read, fail_nth;
static atomic_int output, power, frequency, slot = 1, slot_target = 1, slot_count = 7, pending_polls;
static libusb_hotplug_callback_fn callback;
static indigo_queue *queue;
static indigo_uni_handle handle;
static libdsusb_device_context dsusb_context;
static libfcusb_device_context fcusb_context;
static libgpusb_device_context gpusb_context;
void (*libdsusb_debug)(const char *);
void (*libfcusb_debug)(const char *);
void (*libgpusb_debug)(const char *);

static bool operation(void) {
	atomic_fetch_add(&io_calls, 1);
	if (opened == closed) {
		atomic_fetch_add(&invalid_io, 1);
		return false;
	}
	if (fail_nth > 0 && atomic_fetch_sub(&fail_nth, 1) == 1) {
		return false;
	}
	return !atomic_exchange(&fail_io, 0);
}

indigo_queue *output_queue_create(indigo_device *device) {
	if (atomic_exchange(&fail_queue, 0)) {
		return NULL;
	}
	return queue = indigo_queue_create(device);
}

void output_usb_start(void) { }

int LIBUSB_CALL output_usb_register(libusb_context *ctx, int events, int flags, int vid, int pid, int cls, libusb_hotplug_callback_fn fn, void *data, libusb_hotplug_callback_handle *registration) {
	if (atomic_exchange(&fail_register, 0)) {
		return LIBUSB_ERROR_OTHER;
	}
	callback = fn;
	*registration = 1;
	callback(NULL, (libusb_device *)(uintptr_t)1, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	return 0;
}

void LIBUSB_CALL output_usb_deregister(libusb_context *ctx, libusb_hotplug_callback_handle registration) {
	callback = NULL;
}

int output_usb_register_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vid, int pid, int cls, libusb_hotplug_callback_fn fn, void *data, libusb_hotplug_callback_handle *registration) {
	return output_usb_register(ctx, events, flags, vid, pid, cls, fn, data, registration);
}

int output_usb_deregister_poll(libusb_context *ctx, libusb_hotplug_callback_handle registration) {
	output_usb_deregister(ctx, registration);
	return 0;
}

libusb_device * LIBUSB_CALL output_usb_ref(libusb_device *device) {
	atomic_fetch_add(&references, 1);
	return device;
}

void LIBUSB_CALL output_usb_unref(libusb_device *device) {
	atomic_fetch_sub(&references, 1);
}

indigo_result output_usb_path(libusb_device *device, char *path) {
	strcpy(path, "1");
	return INDIGO_OK;
}

indigo_result output_attach(indigo_device *device) {
	if (atomic_exchange(&fail_attach, 0)) {
		return INDIGO_FAILED;
	}
	indigo_result result = indigo_attach_device(device);
	if (result == INDIGO_OK) {
		atomic_fetch_add(&attached, 1);
	}
	return result;
}

indigo_result output_detach(indigo_device *device) {
	indigo_result result = indigo_detach_device(device);
	atomic_fetch_sub(&attached, 1);
	return result;
}

bool libdsusb_shutter(libusb_device *dev, const char **name) { *name = TEST_NAME; return true; }
bool libfcusb_focuser(libusb_device *dev, const char **name) { *name = TEST_NAME; return true; }
bool libgpusb_guider(libusb_device *dev, const char **name) { *name = TEST_NAME; return true; }

static bool open_output(void) {
	if (atomic_exchange(&fail_open, 0)) {
		return false;
	}
	atomic_fetch_add(&opened, 1);
	return true;
}

bool libdsusb_open(libusb_device *dev, libdsusb_device_context **ctx) { *ctx = &dsusb_context; return open_output(); }
bool libfcusb_open(libusb_device *dev, libfcusb_device_context **ctx) { *ctx = &fcusb_context; return open_output(); }
bool libgpusb_open(libusb_device *dev, libgpusb_device_context **ctx) { *ctx = &gpusb_context; return open_output(); }
void libdsusb_close(libdsusb_device_context *ctx) { atomic_fetch_add(&closed, 1); }
void libfcusb_close(libfcusb_device_context *ctx) { atomic_fetch_add(&closed, 1); }
void libgpusb_close(libgpusb_device_context *ctx) { atomic_fetch_add(&closed, 1); }

static bool set_output(int value) {
	if (!operation() || (value == 0 && atomic_exchange(&fail_stop, 0))) { return false; }
	atomic_store(&output, value);
	return true;
}

bool libdsusb_start(libdsusb_device_context *ctx) { return set_output(1); }
bool libdsusb_focus(libdsusb_device_context *ctx) { return set_output(2); }
bool libdsusb_stop(libdsusb_device_context *ctx) { return set_output(0); }
bool libfcusb_stop(libfcusb_device_context *ctx) { return set_output(0); }
bool libfcusb_move_in(libfcusb_device_context *ctx) { return set_output(-1); }
bool libfcusb_move_out(libfcusb_device_context *ctx) { return set_output(1); }
bool libfcusb_set_power(libfcusb_device_context *ctx, unsigned value) { if (!operation()) { return false; } atomic_store(&power, value); return true; }
bool libfcusb_set_frequency(libfcusb_device_context *ctx, unsigned value) { if (!operation()) { return false; } atomic_store(&frequency, value); return true; }
bool libgpusb_set(libgpusb_device_context *ctx, int value) { return set_output(value); }

indigo_uni_handle *output_hid_open(const int vid, const int pid, int level) {
	if (!open_output()) { return NULL; }
	handle.hid_device = &handle;
	return &handle;
}

void output_hid_close(indigo_uni_handle **h) {
	if (*h) { atomic_fetch_add(&closed, 1); *h = NULL; }
}

bool libatik_wheel_query(hid_device *h, int *count, int *current) {
	if (!operation() || atomic_exchange(&fail_read, 0)) { return false; }
	*count = slot_count;
	if (pending_polls > 0) { atomic_fetch_sub(&pending_polls, 1); } else { slot = slot_target; }
	*current = slot;
	return true;
}

bool libatik_wheel_set(hid_device *h, int target) {
	if (!operation()) { return false; }
	slot_target = target;
	pending_polls = 2;
	return true;
}

long output_hid_write(indigo_uni_handle *h, const char *buffer, long length) {
	if (!operation()) { return -1; }
	if ((unsigned char)buffer[0] & 0x80) { slot_target = (unsigned char)buffer[0] & 0x7f; pending_polls = 2; }
	return length;
}

long output_hid_read(indigo_uni_handle *h, void *buffer, long length) {
	if (!operation() || atomic_exchange(&fail_read, 0)) { return -1; }
	if (pending_polls > 0) { atomic_fetch_sub(&pending_polls, 1); } else { slot = slot_target; }
	((unsigned char *)buffer)[0] = slot;
	((unsigned char *)buffer)[1] = slot_count;
	return 2;
}

static bool start_output(void) {
	if (!bring_up_serial_driver(&driver)) { return false; }
	indigo_queue_drain(queue);
	return connect_serial_device(&driver, NULL);
}

static void lifecycle(void) {
	SERIAL_CHECK_TRUE(start_output());
	SERIAL_CHECK_EQ_INT(1, attached);
	SERIAL_CHECK_EQ_INT(INDIGO_BUSY, TEST_ENTRY(INDIGO_DRIVER_SHUTDOWN, NULL));
	SERIAL_CHECK_TRUE(callback != NULL);
	callback(NULL, (libusb_device *)(uintptr_t)1, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_queue_drain(queue);
	SERIAL_CHECK_EQ_INT(1, attached);
	disconnect_serial_device(&driver);
	atomic_store(&fail_open, 1);
	SERIAL_CHECK_TRUE(!connect_serial_device(&driver, NULL));
	SERIAL_CHECK_TRUE(connect_serial_device(&driver, NULL));
	callback(NULL, (libusb_device *)(uintptr_t)1, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	indigo_queue_drain(queue);
	SERIAL_CHECK_EQ_INT(0, attached);
	SERIAL_CHECK_EQ_INT(opened, closed);
	callback(NULL, (libusb_device *)(uintptr_t)1, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_queue_drain(queue);
	SERIAL_CHECK_TRUE(connect_serial_device(&driver, NULL));
cleanup:
	stop_serial_driver(&driver);
	ASSERT_EQ_INT(0, attached);
	ASSERT_EQ_INT(0, references);
	ASSERT_EQ_INT(opened, closed);
	ASSERT_EQ_INT(0, invalid_io);
}

static bool wait_output(int expected) {
	for (int i = 0; i < 2000; i++) {
		if (output == expected) { return true; }
		indigo_usleep(1000);
	}
	return false;
}

static void operations(void) {
	SERIAL_CHECK_TRUE(start_output());
#if TEST_KIND == 1
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 0.2);
	SERIAL_CHECK_TRUE(wait_output(1));
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_EXPOSURE", INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_output(0));
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_EXPOSURE", INDIGO_OK_STATE));
	atomic_store(&fail_io, 1);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 0.2);
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_EXPOSURE", INDIGO_ALERT_STATE));
#elif TEST_KIND == 2
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_STEPS", "STEPS", 200);
	SERIAL_CHECK_TRUE(wait_output(-1));
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_STEPS", INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(255, power);
	SERIAL_CHECK_EQ_INT(1, frequency);
	SERIAL_CHECK_TRUE(wait_output(0));
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_STEPS", INDIGO_OK_STATE));
	atomic_store(&fail_io, 1);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_STEPS", "STEPS", 200);
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_STEPS", INDIGO_ALERT_STATE));
#elif TEST_KIND == 3
	const char *props[] = { "GUIDER_GUIDE_RA", "GUIDER_GUIDE_RA", "GUIDER_GUIDE_DEC", "GUIDER_GUIDE_DEC" };
	const char *items[] = { "EAST", "WEST", "NORTH", "SOUTH" };
	int masks[] = { GPUSB_RA_EAST, GPUSB_RA_WEST, GPUSB_DEC_NORTH, GPUSB_DEC_SOUTH };
	for (int i = 0; i < 4; i++) {
		indigo_change_number_property_1(&simulator_test_client, TEST_NAME, props[i], items[i], 200);
		SERIAL_CHECK_TRUE(wait_output(masks[i]));
		SERIAL_CHECK_TRUE(wait_for_property_state(props[i], INDIGO_BUSY_STATE));
		SERIAL_CHECK_TRUE(wait_output(0));
		SERIAL_CHECK_TRUE(wait_for_property_state(props[i], INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(wait_for_number_item_value(props[i], items[i], 0, 0));
	}
	atomic_store(&fail_io, 1);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_RA", "EAST", 200);
	SERIAL_CHECK_TRUE(wait_for_property_state("GUIDER_GUIDE_RA", INDIGO_ALERT_STATE));
#else
	for (int target = 2; target <= 7; target += 5) {
		indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "WHEEL_SLOT", "SLOT", target);
		SERIAL_CHECK_TRUE(wait_for_property_state("WHEEL_SLOT", INDIGO_BUSY_STATE));
		SERIAL_CHECK_TRUE(wait_for_property_state("WHEEL_SLOT", INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(wait_for_number_item_value("WHEEL_SLOT", "SLOT", target, 0));
	}
	atomic_store(&fail_io, 1);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "WHEEL_SLOT", "SLOT", 3);
	SERIAL_CHECK_TRUE(wait_for_property_state("WHEEL_SLOT", INDIGO_ALERT_STATE));
#endif
cleanup:
	stop_serial_driver(&driver);
	ASSERT_EQ_INT(opened, closed);
	ASSERT_EQ_INT(0, invalid_io);
}


static void init_rollback(void) {
	atomic_store(&fail_queue, 1);
	SERIAL_CHECK_TRUE(!bring_up_serial_driver(&driver));
	SERIAL_CHECK_EQ_INT(0, attached);
	SERIAL_CHECK_TRUE(callback == NULL);
	atomic_store(&fail_register, 1);
	SERIAL_CHECK_TRUE(!bring_up_serial_driver(&driver));
	SERIAL_CHECK_EQ_INT(0, references);
	SERIAL_CHECK_TRUE(callback == NULL);
	atomic_store(&fail_attach, 1);
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&driver));
	indigo_queue_drain(queue);
	SERIAL_CHECK_EQ_INT(0, attached);
	SERIAL_CHECK_EQ_INT(0, references);
	tear_down_serial_driver(&driver);
	SERIAL_CHECK_TRUE(start_output());
cleanup:
	stop_serial_driver(&driver);
	ASSERT_EQ_INT(0, references);
	ASSERT_EQ_INT(0, attached);
	ASSERT_EQ_INT(opened, closed);
}

static void interruption(void) {
	SERIAL_CHECK_TRUE(start_output());
#if TEST_KIND == 1
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "X_CONFIG", "FOCUS", true);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 2);
	SERIAL_CHECK_TRUE(wait_output(2));
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
	SERIAL_CHECK_TRUE(wait_output(0));
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_EXPOSURE", INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_ABORT_EXPOSURE", INDIGO_OK_STATE));
	int calls_after_abort = io_calls;
	indigo_usleep(1200000);
	SERIAL_CHECK_EQ_INT(0, output);
	SERIAL_CHECK_EQ_INT(calls_after_abort, io_calls);
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "X_CONFIG", "FOCUS", false);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 3);
	SERIAL_CHECK_TRUE(wait_output(1));
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
	SERIAL_CHECK_TRUE(wait_output(0));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 3);
	SERIAL_CHECK_TRUE(wait_output(1));
	indigo_usleep(1200000);
	SERIAL_CHECK_TRUE(wait_for_number_item_value("CCD_EXPOSURE", "EXPOSURE", 2, 0));
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
	SERIAL_CHECK_TRUE(wait_output(0));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 0.2);
	SERIAL_CHECK_TRUE(wait_output(1));
	atomic_store(&fail_stop, 1);
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_EXPOSURE", INDIGO_ALERT_STATE));
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
	SERIAL_CHECK_TRUE(wait_output(0));
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_ABORT_EXPOSURE", INDIGO_OK_STATE));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 2);
	SERIAL_CHECK_TRUE(wait_for_property_state("CCD_EXPOSURE", INDIGO_BUSY_STATE));
#elif TEST_KIND == 2
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "X_FOCUSER_FREQUENCY", "FREQUENCY_16", true);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_SPEED", "SPEED", 100);
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_DIRECTION", "MOVE_OUTWARD", true);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_STEPS", "STEPS", 2000);
	SERIAL_CHECK_TRUE(wait_output(1));
	SERIAL_CHECK_EQ_INT(100, power);
	SERIAL_CHECK_EQ_INT(16, frequency);
	indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_ABORT_MOTION", "ABORT_MOTION", true);
	SERIAL_CHECK_TRUE(wait_output(0));
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_STEPS", INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_ABORT_MOTION", INDIGO_OK_STATE));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_STEPS", "STEPS", 200);
	SERIAL_CHECK_TRUE(wait_output(1));
	atomic_store(&fail_stop, 1);
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_STEPS", INDIGO_ALERT_STATE));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "FOCUSER_STEPS", "STEPS", 2000);
	SERIAL_CHECK_TRUE(wait_for_property_state("FOCUSER_STEPS", INDIGO_BUSY_STATE));
#elif TEST_KIND == 3
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_RA", "EAST", 1000);
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_EAST));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_DEC", "NORTH", 1000);
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_EAST | GPUSB_DEC_NORTH));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_RA", "WEST", 200);
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_WEST | GPUSB_DEC_NORTH));
	SERIAL_CHECK_TRUE(wait_output(GPUSB_DEC_NORTH));
	SERIAL_CHECK_TRUE(wait_for_property_state("GUIDER_GUIDE_RA", INDIGO_OK_STATE));
	int before_zero = io_calls;
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_DEC", "NORTH", 0);
	SERIAL_CHECK_TRUE(wait_output(0));
	SERIAL_CHECK_TRUE(wait_for_property_state("GUIDER_GUIDE_DEC", INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(before_zero + 1, io_calls);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_RA", "EAST", 500);
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_EAST));
	fail_io = 1;
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_RA", "WEST", 1000);
	SERIAL_CHECK_TRUE(wait_for_property_state("GUIDER_GUIDE_RA", INDIGO_ALERT_STATE));
	SERIAL_CHECK_EQ_INT(GPUSB_RA_EAST, output);
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_DEC", "NORTH", 100);
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_EAST | GPUSB_DEC_NORTH));
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_EAST));
	SERIAL_CHECK_TRUE(wait_output(0));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "GUIDER_GUIDE_RA", "EAST", 2000);
	SERIAL_CHECK_TRUE(wait_output(GPUSB_RA_EAST));
#else
	int target = slot == 2 ? 3 : 2;
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "WHEEL_SLOT", "SLOT", target);
	SERIAL_CHECK_TRUE(wait_for_property_state("WHEEL_SLOT", INDIGO_BUSY_STATE));
	atomic_store(&fail_read, 1);
	SERIAL_CHECK_TRUE(wait_for_property_state("WHEEL_SLOT", INDIGO_ALERT_STATE));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, "WHEEL_SLOT", "SLOT", 4);
	SERIAL_CHECK_TRUE(wait_for_property_state("WHEEL_SLOT", INDIGO_BUSY_STATE));
#endif
	callback(NULL, (libusb_device *)(uintptr_t)1, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	indigo_queue_drain(queue);
	SERIAL_CHECK_EQ_INT(opened, closed);
	SERIAL_CHECK_EQ_INT(0, attached);
	int calls = io_calls;
	indigo_usleep(250000);
	SERIAL_CHECK_EQ_INT(calls, io_calls);
	callback(NULL, (libusb_device *)(uintptr_t)1, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_queue_drain(queue);
	SERIAL_CHECK_TRUE(connect_serial_device(&driver, NULL));
cleanup:
	atomic_store(&fail_stop, 0);
	atomic_store(&fail_read, 0);
	stop_serial_driver(&driver);
	ASSERT_EQ_INT(opened, closed);
	ASSERT_EQ_INT(0, invalid_io);
}

#if TEST_KIND == 1
static void queued_abort(void) {
	SERIAL_CHECK_TRUE(start_output());
	SERIAL_CHECK_TRUE(check_queued_abort(TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 30, false, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", false));
	SERIAL_CHECK_TRUE(check_queued_abort(TEST_NAME, "CCD_EXPOSURE", "EXPOSURE", 30, false, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true));
cleanup:
	stop_serial_driver(&driver);
}
#endif

int main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	alarm(60);
	const indigo_test_case tests[] = {
#if TEST_KIND == 1
		{ "queued exposure abort and false abort", queued_abort },
#endif
		{ "discovery, duplicate, connection rollback, active removal and reload", lifecycle }, { "output mapping, delayed completion and errors", operations }, { "initialization and attach rollback", init_rollback }, { "settings, abort, stop/read errors and removal during operation", interruption } };
	return indigo_run_tests(TEST_NAME, tests, ARRAY_SIZE(tests));
}
