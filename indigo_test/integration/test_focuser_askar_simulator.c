// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_askar/indigo_focuser_askar.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_ASKAR_SIMULATOR_EXECUTABLE
#define FOCUSER_ASKAR_SIMULATOR_EXECUTABLE "build/integration/focuser_askar_simulator"
#endif

#define X_FOCUSER_MOTOR_MODE_PROPERTY_NAME              "X_FOCUSER_MOTOR_MODE"
#define X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM_NAME "HIGH_PERFORMANCE"
#define X_FOCUSER_MOTOR_MODE_BALANCED_ITEM_NAME         "BALANCED"

static const simulator_driver_case askar_focuser = {
	"Askar-WAF Focuser",
	"indigo_focuser_askar",
	"Askar-WAF",
	indigo_focuser_askar,
	false,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static void askar_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_ASKAR_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&askar_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
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
	assert_property_has_item(X_FOCUSER_MOTOR_MODE_PROPERTY_NAME, X_FOCUSER_MOTOR_MODE_HIGH_PERFORMANCE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_MOTOR_MODE_PROPERTY_NAME, X_FOCUSER_MOTOR_MODE_BALANCED_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	double sync_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000);
	SERIAL_CHECK_TRUE(!isnan(sync_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, askar_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, askar_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, askar_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, askar_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, askar_focuser.device_name, X_FOCUSER_MOTOR_MODE_PROPERTY_NAME, X_FOCUSER_MOTOR_MODE_BALANCED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_MOTOR_MODE_PROPERTY_NAME, INDIGO_OK_STATE));

	double target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position + 400);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, askar_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, askar_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 1));

cleanup:
	if (context.connected) {
		stop_serial_driver(&askar_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "askar_focuser_passes_serial_compliance_checks", askar_focuser_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Askar-WAF serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
