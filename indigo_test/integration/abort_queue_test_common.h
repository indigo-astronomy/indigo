// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#ifndef INDIGO_ABORT_QUEUE_TEST_COMMON_H
#define INDIGO_ABORT_QUEUE_TEST_COMMON_H
#include <stdatomic.h>
#include <indigo/indigo_driver.h>

static indigo_device *abort_test_device;
static const char *abort_test_name;
static atomic_bool abort_test_entered, abort_test_released, abort_test_drained;

static indigo_result abort_test_capture(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->device, abort_test_name)) {
		abort_test_device = device;
	}
	return INDIGO_OK;
}

static void abort_test_block(indigo_device *device) {
	atomic_store(&abort_test_entered, true);
	while (!atomic_load(&abort_test_released)) {
		indigo_usleep(1000);
	}
}

static void abort_test_drain(indigo_device *device) {
	atomic_store(&abort_test_drained, true);
}

static bool abort_test_wait(atomic_bool *flag) {
	for (int i = 0; i < 10000; i++) {
		if (atomic_load(flag)) {
			return true;
		}
		indigo_usleep(1000);
	}
	return false;
}

static bool check_queued_abort(const char *name, const char *start_property, const char *start_item, double value, bool switch_start, const char *abort_property, const char *abort_item, bool requested) {
	indigo_client observer = { .name = "Queued abort observer", .version = INDIGO_VERSION_CURRENT, .define_property = abort_test_capture };
	abort_test_name = name;
	abort_test_device = NULL;
	indigo_attach_client(&observer);
	indigo_enumerate_properties(&observer, &INDIGO_ALL_PROPERTIES);
	indigo_detach_client(&observer);
	if (abort_test_device == NULL) {
		return false;
	}
	atomic_store(&abort_test_entered, false);
	atomic_store(&abort_test_released, false);
	atomic_store(&abort_test_drained, false);
	indigo_execute_handler(abort_test_device, abort_test_block);
	bool ok = abort_test_wait(&abort_test_entered);
	if (ok) {
		if (switch_start) {
			ok = indigo_change_switch_property_1(&simulator_test_client, name, start_property, start_item, true) == INDIGO_OK;
		} else {
			ok = indigo_change_number_property_1(&simulator_test_client, name, start_property, start_item, value) == INDIGO_OK;
		}
		ok = ok && wait_for_property_state(start_property, INDIGO_BUSY_STATE);
		ok = ok && indigo_change_switch_property_1(&simulator_test_client, name, abort_property, abort_item, requested) == INDIGO_OK;
	}
	indigo_execute_handler(abort_test_device, abort_test_drain);
	atomic_store(&abort_test_released, true);
	ok = abort_test_wait(&abort_test_drained) && ok;
	if (requested) {
		ok = wait_for_property_not_busy(start_property) && ok;
	} else {
		ok = wait_for_property_state(start_property, INDIGO_BUSY_STATE) && ok;
		indigo_change_switch_property_1(&simulator_test_client, name, abort_property, abort_item, true);
		ok = wait_for_property_not_busy(start_property) && ok;
	}
	return ok && wait_for_property_not_busy(abort_property);
}
#endif
