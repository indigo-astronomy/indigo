// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_optec/indigo_focuser_optec.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_OPTEC_SIMULATOR_EXECUTABLE
#define FOCUSER_OPTEC_SIMULATOR_EXECUTABLE "build/integration/focuser_optec_simulator"
#endif

static const simulator_driver_case optec_focuser = {
	"Optec TCF-S Focuser",
	"indigo_focuser_optec",
	"Optec TCF-S",
	indigo_focuser_optec,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void optec_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_OPTEC_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&optec_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME, -12));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_COMPENSATION_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 5025, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, optec_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 30));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 4995, 1));

cleanup:
	if (context.connected) {
		stop_serial_driver(&optec_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "optec_focuser_passes_serial_compliance_checks", optec_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("Optec TCF-S serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
