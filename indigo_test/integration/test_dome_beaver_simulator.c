// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_beaver/indigo_dome_beaver.h>

#include "serial_simulator_test_common.h"

#ifndef DOME_BEAVER_SIMULATOR_EXECUTABLE
#define DOME_BEAVER_SIMULATOR_EXECUTABLE "build/integration/dome_beaver_simulator"
#endif

#define X_SHUTTER_CALIBRATE_PROPERTY_NAME   "X_SHUTTER_CALIBRATE"
#define X_SHUTTER_CALIBRATE_ITEM_NAME       "CALIBRATE"
#define X_ROTATOR_CALIBRATE_PROPERTY_NAME   "X_ROTATOR_CALIBRATE"
#define X_ROTATOR_CALIBRATE_ITEM_NAME       "CALIBRATE"
#define X_FAILURE_MESSAGE_PROPERTY_NAME     "X_FAILURE_MESSAGES"
#define X_FAILURE_MESSAGE_ROTATOR_ITEM_NAME "ROTATOR"
#define X_FAILURE_MESSAGE_SHUTTER_ITEM_NAME "SHUTTER"
#define X_CLEAR_FAILURE_PROPERTY_NAME       "X_CLEAR_FAILURES"
#define X_CLEAR_FAILURE_ITEM_NAME           "CLEAR"
#define X_CONDITIONS_SAFETY_PROPERTY_NAME   "X_CONDITIONS_SAFETY"
#define X_SAFE_CW_ITEM_NAME                 "CLOUD_WATCHER"
#define X_SAFE_HYDREON_ITEM_NAME            "HYDREON"

static const simulator_driver_case beaver_case = {
	DOME_BEAVER_NAME,
	"indigo_dome_beaver",
	DOME_BEAVER_NAME,
	indigo_dome_beaver,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void beaver_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, DOME_BEAVER_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&beaver_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_DOME);
	assert_property_has_item(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME);
	assert_property_has_item(DOME_STEPS_PROPERTY_NAME, DOME_STEPS_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME);
	assert_property_has_item(DOME_HOME_PROPERTY_NAME, DOME_HOME_ITEM_NAME);
	assert_property_has_item(DOME_PARK_POSITION_PROPERTY_NAME, DOME_PARK_POSITION_AZ_ITEM_NAME);
	assert_property_has_item(DOME_ON_COORDINATES_SET_PROPERTY_NAME, DOME_ON_COORDINATES_SET_GOTO_ITEM_NAME);
	assert_property_has_item(DOME_SLAVING_PARAMETERS_PROPERTY_NAME, DOME_SLAVING_THRESHOLD_ITEM_NAME);
	assert_property_has_item(X_SHUTTER_CALIBRATE_PROPERTY_NAME, X_SHUTTER_CALIBRATE_ITEM_NAME);
	assert_property_has_item(X_ROTATOR_CALIBRATE_PROPERTY_NAME, X_ROTATOR_CALIBRATE_ITEM_NAME);
	assert_property_has_item(X_FAILURE_MESSAGE_PROPERTY_NAME, X_FAILURE_MESSAGE_ROTATOR_ITEM_NAME);
	assert_property_has_item(X_FAILURE_MESSAGE_PROPERTY_NAME, X_FAILURE_MESSAGE_SHUTTER_ITEM_NAME);
	assert_property_has_item(X_CLEAR_FAILURE_PROPERTY_NAME, X_CLEAR_FAILURE_ITEM_NAME);
	assert_property_has_item(X_CONDITIONS_SAFETY_PROPERTY_NAME, X_SAFE_CW_ITEM_NAME);
	assert_property_has_item(X_CONDITIONS_SAFETY_PROPERTY_NAME, X_SAFE_HYDREON_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, beaver_case.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 42));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, DOME_HOME_PROPERTY_NAME, DOME_HOME_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HOME_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HOME_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, X_ROTATOR_CALIBRATE_PROPERTY_NAME, X_ROTATOR_CALIBRATE_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_ROTATOR_CALIBRATE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_ROTATOR_CALIBRATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, X_SHUTTER_CALIBRATE_PROPERTY_NAME, X_SHUTTER_CALIBRATE_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SHUTTER_CALIBRATE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SHUTTER_CALIBRATE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, X_CLEAR_FAILURE_PROPERTY_NAME, X_CLEAR_FAILURE_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_CLEAR_FAILURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_CLEAR_FAILURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, beaver_case.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&beaver_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "beaver_passes_serial_compliance_checks", beaver_passes_serial_compliance_checks },
	};
	return indigo_run_tests("NexDome Beaver serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
