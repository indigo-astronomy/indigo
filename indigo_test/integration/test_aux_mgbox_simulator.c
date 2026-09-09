// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <indigo_drivers/aux_mgbox/indigo_aux_mgbox.h>
#include "serial_simulator_test_common.h"
#include "aux_test_isolation.h"

static const simulator_driver_case primary = { "MGBox Weather", "indigo_aux_mgbox", "MGBox Weather", indigo_aux_mgbox, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static void normal(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&primary, aux_simulator.port));
	assert_device_interface(INDIGO_INTERFACE_AUX);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_PRESSURE_ITEM_NAME, 962.76, .01));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME, 31.8, .01));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_HUMIDITY_ITEM_NAME, 40.8, .01));
	SERIAL_CHECK_EQ_INT(indigo_change_number_property_1(&simulator_test_client, primary.device_name, "X_WEATHER_CALIBRATION", AUX_WEATHER_PRESSURE_ITEM_NAME, 2.5), INDIGO_OK);
	SERIAL_CHECK_TRUE(wait_for_number_item_value("X_WEATHER_CALIBRATION", AUX_WEATHER_PRESSURE_ITEM_NAME, 2.5, .01));
	printf("Data and control assertions passed; checking disconnect/shutdown\n");
cleanup:
	aux_stop(&primary);
}

static void rejected_connection(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&primary));
	SERIAL_CHECK_TRUE(!connect_serial_device(&primary, aux_simulator.port));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(!context.connected);
cleanup:
	aux_stop(&primary);
}

static const simulator_driver_case gps = { "MGBox GPS", "indigo_aux_mgbox", "MGBox GPS", indigo_aux_mgbox, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static void gps_readings(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&gps, aux_simulator.port));
	assert_device_interface(INDIGO_INTERFACE_GPS);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, 48, .0001));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, 17, .0001));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, 250, .01));
	printf("GPS assertions passed; checking disconnect/shutdown\n");
cleanup:
	aux_stop(&gps);
}

static void invalid_port(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&primary));
	SERIAL_CHECK_TRUE(aux_reject(&primary, "/dev/indigo-nonexistent-aux-test"));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
cleanup:
	aux_stop(&primary);
}

int main(void) {
	const aux_simulated_case tests[] = {
		{ "normal", normal, "normal" },
		{ "gps_readings", gps_readings, "normal" },
		{ "short_weather", normal, "short-weather" },
		{ "short_gps", gps_readings, "short-gps" },
		{ "invalid_port", invalid_port, "normal" },
	};
	return run_aux_simulated("mgbox", "build/integration/aux_mgbox_simulator", tests, ARRAY_SIZE(tests));
}
