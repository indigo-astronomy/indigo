// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_baader/indigo_dome_baader.h>

#include "serial_simulator_test_common.h"

#ifndef DOME_BAADER_SIMULATOR_EXECUTABLE
#define DOME_BAADER_SIMULATOR_EXECUTABLE "build/integration/dome_baader_simulator"
#endif

// Driver-local names not exported from the header
#define X_EMERGENCY_CLOSE_PROPERTY_NAME          "X_EMERGENCY_CLOSE"
#define X_EMERGENCY_RAIN_ITEM_NAME               "RAIN"
#define X_EMERGENCY_WIND_ITEM_NAME               "WIND"
#define X_EMERGENCY_OPERATION_TIMEOUT_ITEM_NAME  "OPERATION_TIMEOUT"
#define X_EMERGENCY_POWERCUT_ITEM_NAME           "POWER_CUT"

static const simulator_driver_case baader_case = {
	DOME_BAADER_NAME,
	"indigo_dome_baader",
	DOME_BAADER_NAME,
	indigo_dome_baader,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void baader_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, DOME_BAADER_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&baader_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	// Driver fires dome_timer_callback at 0.5s after connect; wait for first
	// d#get_eme poll to transition X_EMERGENCY_CLOSE from IDLE to OK.
	SERIAL_CHECK_TRUE(wait_for_property_state(X_EMERGENCY_CLOSE_PROPERTY_NAME, INDIGO_OK_STATE));

	assert_device_interface(INDIGO_INTERFACE_DOME);

	assert_property_has_item(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_FLAP_PROPERTY_NAME, DOME_FLAP_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_FLAP_PROPERTY_NAME, DOME_FLAP_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME);
	assert_property_has_item(X_EMERGENCY_CLOSE_PROPERTY_NAME, X_EMERGENCY_RAIN_ITEM_NAME);
	assert_property_has_item(X_EMERGENCY_CLOSE_PROPERTY_NAME, X_EMERGENCY_WIND_ITEM_NAME);
	assert_property_has_item(X_EMERGENCY_CLOSE_PROPERTY_NAME, X_EMERGENCY_OPERATION_TIMEOUT_ITEM_NAME);
	assert_property_has_item(X_EMERGENCY_CLOSE_PROPERTY_NAME, X_EMERGENCY_POWERCUT_ITEM_NAME);

	// Open shutter (simulator immediately transitions to 100% open)
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, baader_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	// Open flap (shutter is now open, so simulator allows it)
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, baader_case.device_name, DOME_FLAP_PROPERTY_NAME, DOME_FLAP_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_FLAP_PROPERTY_NAME, INDIGO_OK_STATE));

	// Abort (dome is idle; abort completes synchronously)
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, baader_case.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&baader_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "baader_passes_serial_compliance_checks", baader_passes_serial_compliance_checks },
	};
	return indigo_run_tests("Baader Classic Dome serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
