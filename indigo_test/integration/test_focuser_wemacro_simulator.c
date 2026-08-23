// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_wemacro/indigo_focuser_wemacro.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_WEMACRO_SIMULATOR_EXECUTABLE
#define FOCUSER_WEMACRO_SIMULATOR_EXECUTABLE "build/integration/focuser_wemacro_simulator"
#endif

#define X_RAIL_CONFIG_PROPERTY_NAME              "X_RAIL_CONFIG"
#define X_RAIL_CONFIG_BACK_ITEM_NAME             "BACK"
#define X_RAIL_CONFIG_BEEP_ITEM_NAME             "BEEP"
#define X_RAIL_SHUTTER_PROPERTY_NAME             "X_RAIL_SHUTTER"
#define X_RAIL_SHUTTER_ITEM_NAME                 "SHUTTER"
#define X_RAIL_EXECUTE_PROPERTY_NAME             "X_RAIL_EXECUTE"
#define X_RAIL_EXECUTE_SETTLE_TIME_ITEM_NAME     "SETTLE_TIME"
#define X_RAIL_EXECUTE_PER_STEP_ITEM_NAME        "SHUTTER_PER_STEP"
#define X_RAIL_EXECUTE_INTERVAL_ITEM_NAME        "SHUTTER_INTERVAL"
#define X_RAIL_EXECUTE_LENGTH_ITEM_NAME          "LENGTH"
#define X_RAIL_EXECUTE_COUNT_ITEM_NAME           "COUNT"

static const simulator_driver_case wemacro_focuser = {
	FOCUSER_WEMACRO_NAME,
	"indigo_focuser_wemacro",
	FOCUSER_WEMACRO_NAME,
	indigo_focuser_wemacro,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void wemacro_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_WEMACRO_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&wemacro_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(X_RAIL_CONFIG_PROPERTY_NAME, X_RAIL_CONFIG_BACK_ITEM_NAME);
	assert_property_has_item(X_RAIL_CONFIG_PROPERTY_NAME, X_RAIL_CONFIG_BEEP_ITEM_NAME);
	assert_property_has_item(X_RAIL_SHUTTER_PROPERTY_NAME, X_RAIL_SHUTTER_ITEM_NAME);
	assert_property_has_item(X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_SETTLE_TIME_ITEM_NAME);
	assert_property_has_item(X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_PER_STEP_ITEM_NAME);
	assert_property_has_item(X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_INTERVAL_ITEM_NAME);
	assert_property_has_item(X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_LENGTH_ITEM_NAME);
	assert_property_has_item(X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_COUNT_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wemacro_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 2));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wemacro_focuser.device_name, X_RAIL_CONFIG_PROPERTY_NAME, X_RAIL_CONFIG_BEEP_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_RAIL_CONFIG_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wemacro_focuser.device_name, X_RAIL_SHUTTER_PROPERTY_NAME, X_RAIL_SHUTTER_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_RAIL_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wemacro_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wemacro_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wemacro_focuser.device_name, X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_LENGTH_ITEM_NAME, 10));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_RAIL_EXECUTE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wemacro_focuser.device_name, X_RAIL_EXECUTE_PROPERTY_NAME, X_RAIL_EXECUTE_COUNT_ITEM_NAME, 3));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_RAIL_EXECUTE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wemacro_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&wemacro_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "wemacro_focuser_passes_serial_compliance_checks", wemacro_focuser_passes_serial_compliance_checks }
	};
	return indigo_run_tests("WeMacro Rail serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
