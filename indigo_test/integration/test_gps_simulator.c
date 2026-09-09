// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/gps_simulator/indigo_gps_simulator.h>

#include "serial_simulator_test_common.h"

#define GPS_ADVANCED_STATUS_PROPERTY_NAME "GPS_ADVANCED_STATUS"

static const char *gps_connected_properties[] = {
	GEOGRAPHIC_COORDINATES_PROPERTY_NAME,
	UTC_TIME_PROPERTY_NAME,
	GPS_STATUS_PROPERTY_NAME,
	GPS_ADVANCED_PROPERTY_NAME
};

static const char *gps_hidden_connected_properties[] = {
	GPS_ADVANCED_STATUS_PROPERTY_NAME
};

static const simulator_driver_case gps_simulator = {
	"GPS Simulator",
	"indigo_gps_simulator",
	"GPS Simulator",
	indigo_gps_simulator,
	false,
	base_properties_with_instances,
	ARRAY_SIZE(base_properties_with_instances),
	hidden_base_properties,
	ARRAY_SIZE(hidden_base_properties),
	gps_connected_properties,
	ARRAY_SIZE(gps_connected_properties),
	gps_hidden_connected_properties,
	ARRAY_SIZE(gps_hidden_connected_properties)
};

static void driver_info_reports_simulator_metadata(void) {
	assert_simulator_driver_info(&gps_simulator);
}

static void simulator_exposes_expected_properties(void) {
	assert_simulator_properties(&gps_simulator);
}

static void simulator_passes_gps_compliance_checks(void) {
	static const char *geographic_items[] = {
		GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_ACCURACY_ITEM_NAME
	};
	static const char *gps_status_items[] = {
		GPS_STATUS_NO_FIX_ITEM_NAME,
		GPS_STATUS_2D_FIX_ITEM_NAME,
		GPS_STATUS_3D_FIX_ITEM_NAME
	};
	static const char *gps_advanced_status_items[] = {
		GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME,
		GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME,
		GPS_ADVANCED_STATUS_PDOP_ITEM_NAME,
		GPS_ADVANCED_STATUS_HDOP_ITEM_NAME,
		GPS_ADVANCED_STATUS_VDOP_ITEM_NAME
	};
	start_connected_simulator(&gps_simulator);

	assert_device_interface(INDIGO_INTERFACE_GPS);
	assert_property_has_items(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, geographic_items, ARRAY_SIZE(geographic_items));
	assert_property_has_item(UTC_TIME_PROPERTY_NAME, UTC_TIME_ITEM_NAME);
	assert_property_has_items(GPS_STATUS_PROPERTY_NAME, gps_status_items, ARRAY_SIZE(gps_status_items));
	assert_property_has_item(GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_ENABLED_ITEM_NAME);
	assert_property_has_item(GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_DISABLED_ITEM_NAME);

	ASSERT_TRUE(wait_for_property_state(GPS_STATUS_PROPERTY_NAME, INDIGO_OK_STATE));
	assert_number_item_in_range(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME);
	assert_number_item_in_range(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME);
	assert_number_item_in_range(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME);
	assert_number_item_in_range(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_ACCURACY_ITEM_NAME);
	assert_any_light_item_active(GPS_STATUS_PROPERTY_NAME, gps_status_items, ARRAY_SIZE(gps_status_items));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, gps_simulator.device_name, GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_ENABLED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(GPS_ADVANCED_PROPERTY_NAME, INDIGO_OK_STATE));
	if (has_defined_property(GPS_ADVANCED_STATUS_PROPERTY_NAME)) {
		assert_property_has_items(GPS_ADVANCED_STATUS_PROPERTY_NAME, gps_advanced_status_items, ARRAY_SIZE(gps_advanced_status_items));
		assert_number_item_in_range(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME);
		assert_number_item_in_range(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME);
		assert_number_item_in_range(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_PDOP_ITEM_NAME);
		assert_number_item_in_range(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_HDOP_ITEM_NAME);
		assert_number_item_in_range(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_VDOP_ITEM_NAME);
	}

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, gps_simulator.device_name, GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_DISABLED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(GPS_ADVANCED_PROPERTY_NAME, INDIGO_OK_STATE));

	stop_connected_simulator(&gps_simulator);
}

static bool wait_for_fix(const char *item_name, indigo_property_state state) {
	for (int i = 0; i < 1300; i++) {
		indigo_item *item = find_cached_item(GPS_STATUS_PROPERTY_NAME, item_name);
		if (item != NULL && item->light.value == state) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static void simulator_fix_lifecycle_and_reconnect(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&gps_simulator));
	SERIAL_CHECK_TRUE(connect_serial_device(&gps_simulator, NULL));
	SERIAL_CHECK_EQ_INT(INDIGO_BUSY, indigo_gps_simulator(INDIGO_DRIVER_SHUTDOWN, NULL));
	SERIAL_CHECK_TRUE(wait_for_fix(GPS_STATUS_NO_FIX_ITEM_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, 0, 0));
	SERIAL_CHECK_TRUE(wait_for_property_state(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	indigo_change_switch_property_1(&simulator_test_client, gps_simulator.device_name, GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_ENABLED_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(wait_for_property_state(GPS_ADVANCED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(has_defined_property(GPS_ADVANCED_STATUS_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_fix(GPS_STATUS_2D_FIX_ITEM_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, 43.6255, 0.0005));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, 22.6755, 0.0005));
	SERIAL_CHECK_TRUE(wait_for_property_state(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_fix(GPS_STATUS_3D_FIX_ITEM_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(UTC_TIME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME, 3.5, 0.5));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME, 7.5, 0.5));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_PDOP_ITEM_NAME, 2.5, 0.5));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_HDOP_ITEM_NAME, 3.5, 0.5));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GPS_ADVANCED_STATUS_PROPERTY_NAME, GPS_ADVANCED_STATUS_VDOP_ITEM_NAME, 3.5, 0.5));
	indigo_item *utc = find_cached_item(UTC_TIME_PROPERTY_NAME, UTC_TIME_ITEM_NAME);
	SERIAL_CHECK_TRUE(utc != NULL && strncmp(utc->text.value, "1970", 4));
	indigo_change_switch_property_1(&simulator_test_client, gps_simulator.device_name, GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_DISABLED_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(wait_for_property_state(GPS_ADVANCED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(find_cached_property(GPS_ADVANCED_STATUS_PROPERTY_NAME) == NULL);
	disconnect_serial_device(&gps_simulator);
	int updates = context.update_count;
	indigo_usleep(1100000);
	SERIAL_CHECK_EQ_INT(updates, context.update_count);
	SERIAL_CHECK_TRUE(connect_serial_device(&gps_simulator, NULL));
	SERIAL_CHECK_TRUE(wait_for_fix(GPS_STATUS_NO_FIX_ITEM_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, 0, 0));
	SERIAL_CHECK_TRUE(wait_for_property_state(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
cleanup:
	stop_serial_driver(&gps_simulator);
}

int main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	alarm(45);
	const indigo_test_case tests[] = {
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "simulator_exposes_expected_properties", simulator_exposes_expected_properties },
		{ "simulator_passes_gps_compliance_checks", simulator_passes_gps_compliance_checks },
		{ "simulator_fix_lifecycle_and_reconnect", simulator_fix_lifecycle_and_reconnect }
	};
	return indigo_run_tests("GPS simulator integration tests", tests, ARRAY_SIZE(tests));
}
