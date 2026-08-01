// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_flipflat/indigo_aux_flipflat.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_FLIPFLAT_SIMULATOR_EXECUTABLE
#define AUX_FLIPFLAT_SIMULATOR_EXECUTABLE "build/integration/aux_flipflat_simulator"
#endif

static const simulator_driver_case flipflat_aux = {
	"Alnitak FlipFlat Cover",
	"indigo_aux_flipflat",
	"FlipFlat",
	indigo_aux_flipflat,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void flipflat_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_FLIPFLAT_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&flipflat_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_COVER_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME);
	assert_property_has_item(AUX_COVER_PROPERTY_NAME, AUX_COVER_CLOSE_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_OFF_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME);

	// Light intensity (">B...") and switch (">L"/">D") apply immediately.
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, flipflat_aux.device_name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME, 75));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_INTENSITY_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, flipflat_aux.device_name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_SWITCH_PROPERTY_NAME, INDIGO_OK_STATE));

	// Opening the cover (">OOOO") drives the motor; the driver polls status
	// (">SOOO") while it moves, so the property goes BUSY and settles at OK.
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, flipflat_aux.device_name, AUX_COVER_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_COVER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_COVER_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&flipflat_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "flipflat_aux_passes_serial_compliance_checks", flipflat_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Alnitak FlipFlat serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
