// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <time.h>
#include "serial_simulator_test_common.h"
extern indigo_result TEST_ENTRY(indigo_driver_action, indigo_driver_info *);
static const simulator_driver_case driver = { TEST_NAME, "motion", TEST_NAME, TEST_ENTRY, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static bool wait_progress(double start, double target) {
	for (int i = 0; i < 500; i++) {
		double value = cached_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
		if (value > start && value < target) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static void progress_completion_abort_disconnect(void) {
	external_serial_simulator simulator = { 0 };
	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, TEST_SIMULATOR));
	SERIAL_CHECK_TRUE(start_serial_driver(&driver, simulator.port));
	double start = cached_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	double target = start + 2000;
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_progress(start, target));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target, 1));
	start = target;
	target += 2000;
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_progress(start, target));
	if (has_defined_property(FOCUSER_ABORT_MOTION_PROPERTY_NAME)) {
		indigo_change_switch_property_1(&simulator_test_client, TEST_NAME, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
		SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
		for (int i = 0; i < 500; i++) {
			indigo_property *position = find_cached_property(FOCUSER_POSITION_PROPERTY_NAME);
			if (position && position->state != INDIGO_BUSY_STATE) {
				break;
			}
			indigo_usleep(10000);
		}
		SERIAL_CHECK_TRUE(find_cached_property(FOCUSER_POSITION_PROPERTY_NAME)->state != INDIGO_BUSY_STATE);
		SERIAL_CHECK_TRUE(cached_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME) < target);
	}
	disconnect_serial_device(&driver);
	SERIAL_CHECK_TRUE(connect_serial_device(&driver, simulator.port));
	indigo_change_number_property_1(&simulator_test_client, TEST_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	disconnect_serial_device(&driver);
	SERIAL_CHECK_TRUE(!context.connected);
cleanup:
	stop_serial_driver(&driver);
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = { { "measured motion progress, completion, abort and pending disconnect", progress_completion_abort_disconnect } };
	return indigo_run_tests(TEST_NAME, tests, ARRAY_SIZE(tests));
}
