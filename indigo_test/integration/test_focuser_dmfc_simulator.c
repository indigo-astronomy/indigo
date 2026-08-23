// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_dmfc/indigo_focuser_dmfc.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_DMFC_SIMULATOR_EXECUTABLE
#define FOCUSER_DMFC_SIMULATOR_EXECUTABLE "build/integration/focuser_dmfc_simulator"
#endif

#define FOCUSER_DMFC_NAME                         "Pegasus DMFC"
#define X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME       "X_FOCUSER_MOTOR_TYPE"
#define X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME   "STEPPER"
#define X_FOCUSER_MOTOR_TYPE_DC_ITEM_NAME        "DC"
#define X_FOCUSER_ENCODER_PROPERTY_NAME          "X_FOCUSER_ENCODER"
#define X_FOCUSER_ENCODER_ENABLED_ITEM_NAME      "ENABLED"
#define X_FOCUSER_ENCODER_DISABLED_ITEM_NAME     "DISABLED"
#define X_FOCUSER_LED_PROPERTY_NAME              "X_FOCUSER_LED"
#define X_FOCUSER_LED_ENABLED_ITEM_NAME          "ENABLED"
#define X_FOCUSER_LED_DISABLED_ITEM_NAME         "DISABLED"

static const simulator_driver_case dmfc_focuser = {
	"PegasusAstro DMFC Focuser",
	"indigo_focuser_dmfc",
	FOCUSER_DMFC_NAME,
	indigo_focuser_dmfc,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void dmfc_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_DMFC_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&dmfc_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME, X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME, X_FOCUSER_MOTOR_TYPE_DC_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_ENCODER_PROPERTY_NAME, X_FOCUSER_ENCODER_ENABLED_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_ENCODER_PROPERTY_NAME, X_FOCUSER_ENCODER_DISABLED_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_LED_PROPERTY_NAME, X_FOCUSER_LED_ENABLED_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_LED_PROPERTY_NAME, X_FOCUSER_LED_DISABLED_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 500));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME, X_FOCUSER_MOTOR_TYPE_STEPPER_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_MOTOR_TYPE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, X_FOCUSER_ENCODER_PROPERTY_NAME, X_FOCUSER_ENCODER_DISABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_ENCODER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, X_FOCUSER_LED_PROPERTY_NAME, X_FOCUSER_LED_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_LED_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dmfc_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&dmfc_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "dmfc_focuser_passes_serial_compliance_checks", dmfc_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("DMFC serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
