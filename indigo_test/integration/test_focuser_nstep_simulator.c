// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_nstep/indigo_focuser_nstep.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_NSTEP_SIMULATOR_EXECUTABLE
#define FOCUSER_NSTEP_SIMULATOR_EXECUTABLE "build/integration/focuser_nstep_simulator"
#endif

#define X_FOCUSER_STEPPING_MODE_PROPERTY_NAME  "X_FOCUSER_STEPPING_MODE"
#define X_FOCUSER_STEPPING_MODE_WAVE_ITEM_NAME "WAVE"
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME "HALF"
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM_NAME "FULL"

#define X_FOCUSER_PHASE_WIRING_PROPERTY_NAME   "X_FOCUSER_PHASE_WIRING"
#define X_FOCUSER_PHASE_WIRING_0_ITEM_NAME     "0"
#define X_FOCUSER_PHASE_WIRING_1_ITEM_NAME     "1"
#define X_FOCUSER_PHASE_WIRING_2_ITEM_NAME     "2"

static const simulator_driver_case nstep_focuser = {
	"Rigel Systems nSTEP Focuser",
	"indigo_focuser_nstep",
	"nSTEP",
	indigo_focuser_nstep,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void nstep_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_NSTEP_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&nstep_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, X_FOCUSER_STEPPING_MODE_WAVE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, X_FOCUSER_STEPPING_MODE_FULL_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_PHASE_WIRING_PROPERTY_NAME, X_FOCUSER_PHASE_WIRING_0_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_PHASE_WIRING_PROPERTY_NAME, X_FOCUSER_PHASE_WIRING_1_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_PHASE_WIRING_PROPERTY_NAME, X_FOCUSER_PHASE_WIRING_2_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 120));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, X_FOCUSER_STEPPING_MODE_HALF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_STEPPING_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, X_FOCUSER_PHASE_WIRING_PROPERTY_NAME, X_FOCUSER_PHASE_WIRING_2_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_PHASE_WIRING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 8));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME, 4));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_COMPENSATION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 75, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 30));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nstep_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nstep_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nstep_focuser_passes_serial_compliance_checks", nstep_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("Rigel Systems nSTEP serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
