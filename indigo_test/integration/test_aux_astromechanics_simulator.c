// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_astromechanics/indigo_aux_astromechanics.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_ASTROMECHANICS_SIMULATOR_EXECUTABLE
#define AUX_ASTROMECHANICS_SIMULATOR_EXECUTABLE "build/integration/aux_astromechanics_simulator"
#endif

static const simulator_driver_case astromechanics_aux = {
	"ASTROMECHANICS LPM Sky Quality",
	"indigo_aux_astromechanics",
	"ASTROMECHANICS LPM",
	indigo_aux_astromechanics,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void astromechanics_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_ASTROMECHANICS_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&astromechanics_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_SKY_BORTLE_CLASS_ITEM_NAME);

	// The driver polls "V#" on a timer; reaching OK state proves the reply was parsed.
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_WEATHER_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&astromechanics_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "astromechanics_aux_passes_serial_compliance_checks", astromechanics_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("ASTROMECHANICS LPM serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
