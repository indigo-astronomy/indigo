// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_mypro2/indigo_focuser_mypro2.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_MYPRO2_SIMULATOR_EXECUTABLE
#define FOCUSER_MYPRO2_SIMULATOR_EXECUTABLE "build/integration/focuser_mypro2_simulator"
#endif

#define X_STEP_MODE_PROPERTY_NAME        "X_STEP_MODE"
#define X_STEP_MODE_FULL_ITEM_NAME       "FULL"
#define X_STEP_MODE_HALF_ITEM_NAME       "HALF"
#define X_STEP_MODE_FOURTH_ITEM_NAME     "FOURTH"
#define X_STEP_MODE_EIGTH_ITEM_NAME      "EIGTH"
#define X_STEP_MODE_16TH_ITEM_NAME       "16TH"
#define X_STEP_MODE_32TH_ITEM_NAME       "32TH"
#define X_STEP_MODE_64TH_ITEM_NAME       "64TH"
#define X_STEP_MODE_128TH_ITEM_NAME      "128TH"

#define X_COILS_MODE_PROPERTY_NAME       "X_COILS_MODE"
#define X_COILS_MODE_IDLE_OFF_ITEM_NAME  "OFF_WHEN_IDLE"
#define X_COILS_MODE_ALWAYS_ON_ITEM_NAME "ALWAYS_ON"

#define X_SETTLE_TIME_PROPERTY_NAME      "X_SETTLE_TIME"
#define X_SETTLE_TIME_ITEM_NAME          "SETTLE_TIME"

static const simulator_driver_case mypro2_focuser = {
	"myFocuserPro2 Focuser",
	"indigo_focuser_mypro2",
	FOCUSER_MFP2_NAME,
	indigo_focuser_mypro2,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void mypro2_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_MYPRO2_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&mypro2_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_THRESHOLD_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_FULL_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_HALF_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_FOURTH_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_EIGTH_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_16TH_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_32TH_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_64TH_ITEM_NAME);
	assert_property_has_item(X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_128TH_ITEM_NAME);
	assert_property_has_item(X_COILS_MODE_PROPERTY_NAME, X_COILS_MODE_IDLE_OFF_ITEM_NAME);
	assert_property_has_item(X_COILS_MODE_PROPERTY_NAME, X_COILS_MODE_ALWAYS_ON_ITEM_NAME);
	assert_property_has_item(X_SETTLE_TIME_PROPERTY_NAME, X_SETTLE_TIME_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 2));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, X_STEP_MODE_PROPERTY_NAME, X_STEP_MODE_16TH_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_STEP_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, X_COILS_MODE_PROPERTY_NAME, X_COILS_MODE_ALWAYS_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_COILS_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, X_SETTLE_TIME_PROPERTY_NAME, X_SETTLE_TIME_ITEM_NAME, 120));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SETTLE_TIME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME, 4));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_COMPENSATION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1500));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mypro2_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&mypro2_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "mypro2_focuser_passes_serial_compliance_checks", mypro2_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("myFocuserPro2 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
