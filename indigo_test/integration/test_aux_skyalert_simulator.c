// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_skyalert/indigo_aux_skyalert.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_SKYALERT_SIMULATOR_EXECUTABLE
#define AUX_SKYALERT_SIMULATOR_EXECUTABLE "build/integration/aux_skyalert_simulator"
#endif

static const simulator_driver_case skyalert_case = {
	"Interactive Astronomy SkyAlert",
	"indigo_aux_skyalert",
	"Interactive Astronomy SkyAlert",
	indigo_aux_skyalert,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void skyalert_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_SKYALERT_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&skyalert_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();

	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_HUMIDITY_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_PRESSURE_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_WIND_SPEED_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_RAIN_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_POWER_ITEM_NAME);

cleanup:
	if (context.connected) {
		stop_serial_driver(&skyalert_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "skyalert_passes_serial_compliance_checks", skyalert_passes_serial_compliance_checks },
	};
	return indigo_run_tests("SkyAlert serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
