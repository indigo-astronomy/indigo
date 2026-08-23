// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_skyroof/indigo_dome_skyroof.h>

#include "serial_simulator_test_common.h"

#ifndef DOME_SKYROOF_SIMULATOR_EXECUTABLE
#define DOME_SKYROOF_SIMULATOR_EXECUTABLE "build/integration/dome_skyroof_simulator"
#endif

#define DOME_SKYROOF_NAME                   "SkyRoof"
#define HEATER_CONTROL_PROPERTY_NAME        "HEATER_CONTROL"
#define HEATER_CONTROL_OFF_ITEM_NAME        "OFF"
#define HEATER_CONTROL_ON_ITEM_NAME         "ON"

static const simulator_driver_case skyroof_case = {
	DOME_SKYROOF_NAME,
	"indigo_dome_skyroof",
	DOME_SKYROOF_NAME,
	indigo_dome_skyroof,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void skyroof_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, DOME_SKYROOF_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&skyroof_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_DOME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(HEATER_CONTROL_PROPERTY_NAME, HEATER_CONTROL_OFF_ITEM_NAME);
	assert_property_has_item(HEATER_CONTROL_PROPERTY_NAME, HEATER_CONTROL_ON_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, skyroof_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, skyroof_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, skyroof_case.device_name, HEATER_CONTROL_PROPERTY_NAME, HEATER_CONTROL_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(HEATER_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, skyroof_case.device_name, HEATER_CONTROL_PROPERTY_NAME, HEATER_CONTROL_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(HEATER_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, skyroof_case.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&skyroof_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "skyroof_passes_serial_compliance_checks", skyroof_passes_serial_compliance_checks },
	};
	return indigo_run_tests("SkyRoof serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
