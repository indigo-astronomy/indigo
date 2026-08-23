// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_nfocus/indigo_focuser_nfocus.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_NFOCUS_SIMULATOR_EXECUTABLE
#define FOCUSER_NFOCUS_SIMULATOR_EXECUTABLE "build/integration/focuser_nfocus_simulator"
#endif

static const simulator_driver_case nfocus_focuser = {
	"Rigel Systems nFOCUS Focuser",
	"indigo_focuser_nfocus",
	"nFOCUS",
	indigo_focuser_nfocus,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void nfocus_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_NFOCUS_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&nfocus_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nfocus_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 120));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nfocus_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nfocus_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nfocus_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nfocus_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 30));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nfocus_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nfocus_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nfocus_focuser_passes_serial_compliance_checks", nfocus_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("Rigel Systems nFOCUS serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
