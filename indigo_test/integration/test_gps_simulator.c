// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/gps_simulator/indigo_gps_simulator.h>

#include "simulator_test_common.h"

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

int main(void) {
	const indigo_test_case tests[] = {
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "simulator_exposes_expected_properties", simulator_exposes_expected_properties },
		{ "simulator_passes_gps_compliance_checks", simulator_passes_gps_compliance_checks }
	};
	return indigo_run_tests("GPS simulator integration tests", tests, ARRAY_SIZE(tests));
}
