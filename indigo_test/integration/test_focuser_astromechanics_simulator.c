// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_astromechanics/indigo_focuser_astromechanics.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_ASTROMECHANICS_SIMULATOR_EXECUTABLE
#define FOCUSER_ASTROMECHANICS_SIMULATOR_EXECUTABLE "build/integration/focuser_astromechanics_simulator"
#endif

#define FOCUSER_ASTROMECHANICS_NAME          "ASTROMECHANICS Focuser"
#define X_FOCUSER_APERTURE_PROPERTY_NAME     "X_FOCUSER_APERTURE"
#define X_FOCUSER_APERTURE_ITEM_NAME         "APERURE"

static const simulator_driver_case astromechanics_focuser = {
	FOCUSER_ASTROMECHANICS_NAME,
	"indigo_focuser_astromechanics",
	FOCUSER_ASTROMECHANICS_NAME,
	indigo_focuser_astromechanics,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void astromechanics_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_ASTROMECHANICS_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&astromechanics_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_APERTURE_PROPERTY_NAME, X_FOCUSER_APERTURE_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, astromechanics_focuser.device_name, X_FOCUSER_APERTURE_PROPERTY_NAME, X_FOCUSER_APERTURE_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_APERTURE_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&astromechanics_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "astromechanics_focuser_passes_serial_compliance_checks", astromechanics_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("ASTROMECHANICS focuser serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
