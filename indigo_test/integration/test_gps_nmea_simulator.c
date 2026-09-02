// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/gps_nmea/indigo_gps_nmea.h>

#include "serial_simulator_test_common.h"

#ifndef GPS_NMEA_SIMULATOR_EXECUTABLE
#define GPS_NMEA_SIMULATOR_EXECUTABLE "build/integration/gps_nmea_simulator"
#endif

#define GPS_SELECTED_SYSTEM_PROPERTY_NAME "X_GPS_SELECTED_SYSTEM"
#define GPS_ADVANCED_STATUS_PROPERTY_NAME "GPS_ADVANCED_STATUS"

static const simulator_driver_case nmea_gps = {
	"Generic NMEA 0183 GPS",
	"indigo_gps_nmea",
	"NMEA GPS",
	indigo_gps_nmea,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static bool wait_for_switch_item_value(const char *property_name, const char *item_name, bool value) {
	for (int i = 0; i < 100; i++) {
		indigo_item *item = find_cached_item(property_name, item_name);
		if (item != NULL && item->sw.value == value) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static void nmea_gps_passes_serial_compliance_checks(void) {
	static const char *gps_status_items[] = {
		GPS_STATUS_NO_FIX_ITEM_NAME,
		GPS_STATUS_2D_FIX_ITEM_NAME,
		GPS_STATUS_3D_FIX_ITEM_NAME
	};
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, GPS_NMEA_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&nmea_gps, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GPS);
	assert_property_has_item(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME);
	assert_property_has_item(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME);
	assert_property_has_item(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME);
	assert_property_has_item(UTC_TIME_PROPERTY_NAME, UTC_TIME_ITEM_NAME);
	assert_property_has_item(GPS_STATUS_PROPERTY_NAME, GPS_STATUS_3D_FIX_ITEM_NAME);
	assert_property_has_item(GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_ENABLED_ITEM_NAME);
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "AUTO");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "MULTIPLE");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "GPS");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "GALILEO");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "GLONASS");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "BEIDOU");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "NAVIC");
	assert_property_has_item(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "QZSS");

	SERIAL_CHECK_TRUE(wait_for_property_state(GPS_STATUS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(UTC_TIME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, 48.1173, 0.0001));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, 11.5167, 0.0001));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, 545, 1));
	assert_any_light_item_active(GPS_STATUS_PROPERTY_NAME, gps_status_items, ARRAY_SIZE(gps_status_items));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nmea_gps.device_name, GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(GPS_ADVANCED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(GPS_ADVANCED_STATUS_PROPERTY_NAME, INDIGO_OK_STATE));
	assert_property_has_item(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME);
	assert_property_has_item(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME);
	assert_property_has_item(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_PDOP_ITEM_NAME);
	assert_property_has_item(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_HDOP_ITEM_NAME);
	assert_property_has_item(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_VDOP_ITEM_NAME);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME, 8, 0));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME, 8, 0));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nmea_gps.device_name, GPS_SELECTED_SYSTEM_PROPERTY_NAME, "GPS", true));
	SERIAL_CHECK_TRUE(wait_for_switch_item_value(GPS_SELECTED_SYSTEM_PROPERTY_NAME, "GPS", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(GPS_STATUS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nmea_gps);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nmea_gps_passes_serial_compliance_checks", nmea_gps_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Generic NMEA 0183 GPS serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
