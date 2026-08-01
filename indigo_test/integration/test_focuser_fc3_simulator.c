// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_fc3/indigo_focuser_fc3.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_FC3_SIMULATOR_EXECUTABLE
#define FOCUSER_FC3_SIMULATOR_EXECUTABLE "build/integration/focuser_fc3_simulator"
#endif

static const simulator_driver_case focuscube3_focuser = {
	"PegasusAstro FocusCube v3 Focuser",
	"indigo_focuser_fc3",
	"Pegasus FocusCube3",
	indigo_focuser_fc3,
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

static void focuscube3_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_FC3_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&focuscube3_focuser, simulator.port));
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
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	double target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 200);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, focuscube3_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, focuscube3_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, focuscube3_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, focuscube3_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&focuscube3_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "focuscube3_focuser_passes_serial_compliance_checks", focuscube3_focuser_passes_serial_compliance_checks }
	};
	return indigo_run_tests("FocusCube 3 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
