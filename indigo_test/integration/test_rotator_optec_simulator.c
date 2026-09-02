// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/rotator_optec/indigo_rotator_optec.h>

#include "serial_simulator_test_common.h"

#ifndef ROTATOR_OPTEC_SIMULATOR_EXECUTABLE
#define ROTATOR_OPTEC_SIMULATOR_EXECUTABLE "build/integration/rotator_optec_simulator"
#endif

#define OPTEC_HOME_PROPERTY_NAME "X_HOME"
#define OPTEC_HOME_ITEM_NAME "HOME"
#define OPTEC_RATE_PROPERTY_NAME "X_RATE"
#define OPTEC_RATE_ITEM_NAME "RATE"
#define OPTEC_ROTATE_PROPERTY_NAME "X_ROTATE"
#define OPTEC_ROTATE_ITEM_NAME "ROTATE"

static const simulator_driver_case optec_rotator = {
	"Optec Pyxis Rotator",
	"indigo_rotator_optec",
	"Optec Pyxis",
	indigo_rotator_optec,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void optec_rotator_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, ROTATOR_OPTEC_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&optec_rotator, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_ROTATOR);
	assert_property_has_item(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	assert_property_has_item(ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_NORMAL_ITEM_NAME);
	assert_property_has_item(ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_REVERSED_ITEM_NAME);
	assert_property_has_item(OPTEC_HOME_PROPERTY_NAME, OPTEC_HOME_ITEM_NAME);
	assert_property_has_item(OPTEC_RATE_PROPERTY_NAME, OPTEC_RATE_ITEM_NAME);
	assert_property_has_item(OPTEC_ROTATE_PROPERTY_NAME, OPTEC_ROTATE_ITEM_NAME);
	assert_not_defined_property(ROTATOR_ABORT_MOTION_PROPERTY_NAME);
	assert_not_defined_property(ROTATOR_ON_POSITION_SET_PROPERTY_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, optec_rotator.device_name, OPTEC_RATE_PROPERTY_NAME, OPTEC_RATE_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(OPTEC_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_rotator.device_name, ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_REVERSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, optec_rotator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 12, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, optec_rotator.device_name, OPTEC_HOME_PROPERTY_NAME, OPTEC_HOME_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(OPTEC_HOME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, optec_rotator.device_name, OPTEC_ROTATE_PROPERTY_NAME, OPTEC_ROTATE_ITEM_NAME, 3));
	SERIAL_CHECK_TRUE(wait_for_property_state(OPTEC_ROTATE_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&optec_rotator);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "optec_rotator_passes_serial_compliance_checks", optec_rotator_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Optec Pyxis rotator serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
