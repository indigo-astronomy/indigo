// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_sqm/indigo_aux_sqm.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_SQM_SIMULATOR_EXECUTABLE
#define AUX_SQM_SIMULATOR_EXECUTABLE "build/integration/aux_sqm_simulator"
#endif

// Driver-local item names, not exported from the header
#define X_AUX_SENSOR_FREQUENCY_ITEM_NAME "X_AUX_SENSOR_FREQUENCY"
#define X_AUX_SENSOR_COUNTS_ITEM_NAME    "X_AUX_SENSOR_COUNTS"
#define X_AUX_SENSOR_PERIOD_ITEM_NAME    "X_AUX_SENSOR_PERIOD"

static const simulator_driver_case sqm_case = {
	"Unihedron SQM",
	"indigo_aux_sqm",
	"Unihedron SQM",
	indigo_aux_sqm,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void sqm_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_SQM_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&sqm_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	// The driver fires aux_timer_callback immediately after connect to poll "rx".
	// Wait for AUX_WEATHER to reach OK so property values are populated.
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_WEATHER_PROPERTY_NAME, INDIGO_OK_STATE));

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();

	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_SKY_BORTLE_CLASS_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, X_AUX_SENSOR_FREQUENCY_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, X_AUX_SENSOR_COUNTS_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, X_AUX_SENSOR_PERIOD_ITEM_NAME);

cleanup:
	if (context.connected) {
		stop_serial_driver(&sqm_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "sqm_passes_serial_compliance_checks", sqm_passes_serial_compliance_checks },
	};
	return indigo_run_tests("SQM serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
