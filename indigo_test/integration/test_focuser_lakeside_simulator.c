// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_lakeside/indigo_focuser_lakeside.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_LAKESIDE_SIMULATOR_EXECUTABLE
#define FOCUSER_LAKESIDE_SIMULATOR_EXECUTABLE "build/integration/focuser_lakeside_simulator"
#endif

#define X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME   "X_FOCUSER_ACTIVE_SLOPE"
#define X_FOCUSER_ACTIVE_SLOPE_1_ITEM_NAME     "1"
#define X_FOCUSER_ACTIVE_SLOPE_2_ITEM_NAME     "2"
#define X_FOCUSER_DEADBAND_ITEM_NAME           "DEADBAND"

static const simulator_driver_case lakeside_focuser = {
	"LakesideAstro Focuser",
	"indigo_focuser_lakeside",
	"LakesideAstro Focuser",
	indigo_focuser_lakeside,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void lakeside_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_LAKESIDE_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&lakeside_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, X_FOCUSER_DEADBAND_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_PERIOD_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME, X_FOCUSER_ACTIVE_SLOPE_1_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME, X_FOCUSER_ACTIVE_SLOPE_2_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME, 8));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_COMPENSATION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lakeside_focuser.device_name, X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME, X_FOCUSER_ACTIVE_SLOPE_2_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_ACTIVE_SLOPE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 20));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 32748, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lakeside_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&lakeside_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "lakeside_focuser_passes_serial_compliance_checks", lakeside_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("LakesideAstro serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
