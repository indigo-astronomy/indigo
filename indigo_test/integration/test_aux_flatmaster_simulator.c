// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_flatmaster/indigo_aux_flatmaster.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_FLATMASTER_SIMULATOR_EXECUTABLE
#define AUX_FLATMASTER_SIMULATOR_EXECUTABLE "build/integration/aux_flatmaster_simulator"
#endif

static const simulator_driver_case flatmaster_aux = {
	"PegasusAstro FlatMaster Light",
	"indigo_aux_flatmaster",
	"PegasusAstro FlatMaster",
	indigo_aux_flatmaster,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void flatmaster_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_FLATMASTER_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&flatmaster_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_OFF_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, flatmaster_aux.device_name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_SWITCH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, flatmaster_aux.device_name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME, 75));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_INTENSITY_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, flatmaster_aux.device_name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_SWITCH_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&flatmaster_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "flatmaster_aux_passes_serial_compliance_checks", flatmaster_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("PegasusAstro FlatMaster serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
