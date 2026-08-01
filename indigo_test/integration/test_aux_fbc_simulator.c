// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_fbc/indigo_aux_fbc.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_FBC_SIMULATOR_EXECUTABLE
#define AUX_FBC_SIMULATOR_EXECUTABLE "build/integration/aux_fbc_simulator"
#endif

static const simulator_driver_case fbc_aux = {
	"Lacerta FBC Light",
	"indigo_aux_fbc",
	"Lacerta FBC",
	indigo_aux_fbc,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void fbc_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_FBC_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&fbc_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_IMPULSE_PROPERTY_NAME, AUX_LIGHT_IMPULSE_DURATION_ITEM_NAME);
	assert_property_has_item(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME);

	// Intensity applies immediately (": B <n> #"); the impulse and exposure
	// handlers run timed busy loops, so enumeration is asserted for those.
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, fbc_aux.device_name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME, 75));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_INTENSITY_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&fbc_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "fbc_aux_passes_serial_compliance_checks", fbc_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Lacerta FBC serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
