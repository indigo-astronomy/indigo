// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <math.h>

#include <indigo_drivers/focuser_robofocus/indigo_focuser_robofocus.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_ROBOFOCUS_SIMULATOR_EXECUTABLE
#define FOCUSER_ROBOFOCUS_SIMULATOR_EXECUTABLE "build/integration/focuser_robofocus_simulator"
#endif

#define X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME       "X_FOCUSER_POWER_CHANNELS"
#define X_FOCUSER_CONFIG_PROPERTY_NAME               "X_FOCUSER_CONFIG"
#define X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM_NAME        "DUTY_CYCLE"
#define X_FOCUSER_CONFIG_STEP_DELAY_ITEM_NAME        "STEP_DELAY"
#define X_FOCUSER_CONFIG_STEP_SIZE_ITEM_NAME         "STEP_SIZE"
#define X_FOCUSER_CONFIG_BACKLASH_ITEM_NAME          "BACKLASH"

static const simulator_driver_case robofocus_focuser = {
	"RoboFocus Focuser",
	"indigo_focuser_robofocus",
	"RoboFocus",
	indigo_focuser_robofocus,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void robofocus_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_ROBOFOCUS_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&robofocus_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, "1");
	assert_property_has_item(X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, "2");
	assert_property_has_item(X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, "3");
	assert_property_has_item(X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, "4");
	assert_property_has_item(X_FOCUSER_CONFIG_PROPERTY_NAME, X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_CONFIG_PROPERTY_NAME, X_FOCUSER_CONFIG_STEP_DELAY_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_CONFIG_PROPERTY_NAME, X_FOCUSER_CONFIG_STEP_SIZE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_CONFIG_PROPERTY_NAME, X_FOCUSER_CONFIG_BACKLASH_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	double target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, robofocus_focuser.device_name, FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 5000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_LIMITS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, robofocus_focuser.device_name, X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, "1", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_POWER_CHANNELS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, robofocus_focuser.device_name, X_FOCUSER_CONFIG_PROPERTY_NAME, X_FOCUSER_CONFIG_BACKLASH_ITEM_NAME, 35));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_CONFIG_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, robofocus_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, robofocus_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, robofocus_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position + 25, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, robofocus_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, robofocus_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&robofocus_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "robofocus_focuser_passes_serial_compliance_checks", robofocus_focuser_passes_serial_compliance_checks }
	};
	return indigo_run_tests("RoboFocus serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
