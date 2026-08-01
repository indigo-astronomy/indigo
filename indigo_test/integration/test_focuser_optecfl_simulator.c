// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_optecfl/indigo_focuser_optecfl.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_OPTECFL_SIMULATOR_EXECUTABLE
#define FOCUSER_OPTECFL_SIMULATOR_EXECUTABLE "build/integration/focuser_optecfl_simulator"
#endif

#define X_FOCUSER_TYPE_PROPERTY_NAME "X_FOCUSER_TYPE"

static const simulator_driver_case focuslynx_focuser_1 = {
	"Optec FocusLynx Focuser",
	"indigo_focuser_optecfl",
	"Optec FocusLynx #1",
	indigo_focuser_optecfl,
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

static const simulator_driver_case focuslynx_focuser_2 = {
	"Optec FocusLynx Focuser",
	"indigo_focuser_optecfl",
	"Optec FocusLynx #2",
	indigo_focuser_optecfl,
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

static void assert_focuslynx_properties(void) {
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_TYPE_PROPERTY_NAME, "OA");
	assert_property_has_item(X_FOCUSER_TYPE_PROPERTY_NAME, "OB");
	assert_property_has_item(X_FOCUSER_TYPE_PROPERTY_NAME, "TA");
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
}

static void focuslynx_focuser_passes_serial_compliance_checks(const simulator_driver_case *driver_case, double target_position) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_OPTECFL_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(driver_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_focuslynx_properties();
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, X_FOCUSER_TYPE_PROPERTY_NAME, "OB", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_TYPE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));

	target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, driver_case->device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 1));

cleanup:
	if (context.connected) {
		stop_serial_driver(driver_case);
	}
	stop_external_serial_simulator(&simulator);
}

static void focuslynx_focuser_1_passes_serial_compliance_checks(void) {
	focuslynx_focuser_passes_serial_compliance_checks(&focuslynx_focuser_1, 100);
}

static void focuslynx_focuser_2_passes_serial_compliance_checks(void) {
	focuslynx_focuser_passes_serial_compliance_checks(&focuslynx_focuser_2, 120);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "focuslynx_focuser_1_passes_serial_compliance_checks", focuslynx_focuser_1_passes_serial_compliance_checks },
		{ "focuslynx_focuser_2_passes_serial_compliance_checks", focuslynx_focuser_2_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Optec FocusLynx serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
