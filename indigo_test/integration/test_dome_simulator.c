// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_simulator/indigo_dome_simulator.h>

#include "simulator_test_common.h"

static const char *dome_connected_properties[] = {
	DOME_SPEED_PROPERTY_NAME,
	DOME_DIRECTION_PROPERTY_NAME,
	DOME_STEPS_PROPERTY_NAME,
	DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME,
	DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME,
	DOME_SLAVING_PROPERTY_NAME,
	DOME_SLAVING_PARAMETERS_PROPERTY_NAME,
	DOME_ABORT_MOTION_PROPERTY_NAME,
	DOME_SHUTTER_PROPERTY_NAME,
	DOME_PARK_PROPERTY_NAME,
	DOME_DIMENSION_PROPERTY_NAME,
	GEOGRAPHIC_COORDINATES_PROPERTY_NAME,
	SNOOP_DEVICES_PROPERTY_NAME
};

static const char *dome_hidden_connected_properties[] = {
	DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY_NAME,
	DOME_FLAP_PROPERTY_NAME,
	DOME_PARK_POSITION_PROPERTY_NAME,
	DOME_HOME_PROPERTY_NAME,
	UTC_TIME_PROPERTY_NAME,
	DOME_SET_HOST_TIME_PROPERTY_NAME
};

static const simulator_driver_case dome_simulator = {
	"Dome Simulator",
	"indigo_dome_simulator",
	"Dome Simulator",
	indigo_dome_simulator,
	false,
	base_properties_with_instances,
	ARRAY_SIZE(base_properties_with_instances),
	hidden_base_properties,
	ARRAY_SIZE(hidden_base_properties),
	dome_connected_properties,
	ARRAY_SIZE(dome_connected_properties),
	dome_hidden_connected_properties,
	ARRAY_SIZE(dome_hidden_connected_properties)
};

static void driver_info_reports_simulator_metadata(void) {
	assert_simulator_driver_info(&dome_simulator);
}

static void simulator_exposes_expected_properties(void) {
	assert_simulator_properties(&dome_simulator);
}

static double bounded_dome_number_value(const char *property_name, const char *item_name, double preferred_value) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		return NAN;
	}
	if (preferred_value < item->number.min) {
		return item->number.min;
	}
	if (preferred_value > item->number.max) {
		return item->number.max;
	}
	return preferred_value;
}

static void simulator_passes_dome_compliance_checks(void) {
	static const char *direction_items[] = {
		DOME_DIRECTION_MOVE_CLOCKWISE_ITEM_NAME,
		DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM_NAME
	};
	static const char *equatorial_items[] = {
		DOME_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		DOME_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	static const char *horizontal_items[] = {
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME
	};
	static const char *slaving_items[] = {
		DOME_SLAVING_ENABLE_ITEM_NAME,
		DOME_SLAVING_DISABLE_ITEM_NAME
	};
	static const char *shutter_items[] = {
		DOME_SHUTTER_OPENED_ITEM_NAME,
		DOME_SHUTTER_CLOSED_ITEM_NAME
	};
	static const char *park_items[] = {
		DOME_PARK_PARKED_ITEM_NAME,
		DOME_PARK_UNPARKED_ITEM_NAME
	};
	static const char *dimension_items[] = {
		DOME_RADIUS_ITEM_NAME,
		DOME_SHUTTER_WIDTH_ITEM_NAME,
		DOME_MOUNT_PIVOT_OFFSET_NS_ITEM_NAME,
		DOME_MOUNT_PIVOT_OFFSET_EW_ITEM_NAME,
		DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM_NAME,
		DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM_NAME
	};
	static const char *geographic_items[] = {
		GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME
	};
	start_connected_simulator(&dome_simulator);

	assert_device_interface(INDIGO_INTERFACE_DOME);
	assert_property_has_item(DOME_SPEED_PROPERTY_NAME, DOME_SPEED_ITEM_NAME);
	assert_property_has_items(DOME_DIRECTION_PROPERTY_NAME, direction_items, ARRAY_SIZE(direction_items));
	assert_property_has_item(DOME_STEPS_PROPERTY_NAME, DOME_STEPS_ITEM_NAME);
	assert_property_has_items(DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME, equatorial_items, ARRAY_SIZE(equatorial_items));
	assert_property_has_items(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, horizontal_items, ARRAY_SIZE(horizontal_items));
	assert_property_has_items(DOME_SLAVING_PROPERTY_NAME, slaving_items, ARRAY_SIZE(slaving_items));
	assert_property_has_item(DOME_SLAVING_PARAMETERS_PROPERTY_NAME, DOME_SLAVING_THRESHOLD_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_items(DOME_SHUTTER_PROPERTY_NAME, shutter_items, ARRAY_SIZE(shutter_items));
	assert_property_has_items(DOME_PARK_PROPERTY_NAME, park_items, ARRAY_SIZE(park_items));
	assert_property_has_items(DOME_DIMENSION_PROPERTY_NAME, dimension_items, ARRAY_SIZE(dimension_items));
	assert_property_has_items(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, geographic_items, ARRAY_SIZE(geographic_items));
	assert_number_item_in_range(DOME_SPEED_PROPERTY_NAME, DOME_SPEED_ITEM_NAME);
	assert_number_item_in_range(DOME_STEPS_PROPERTY_NAME, DOME_STEPS_ITEM_NAME);
	assert_number_item_in_range(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME);
	assert_number_item_in_range(DOME_SLAVING_PARAMETERS_PROPERTY_NAME, DOME_SLAVING_THRESHOLD_ITEM_NAME);
	assert_number_item_in_range(DOME_DIMENSION_PROPERTY_NAME, DOME_RADIUS_ITEM_NAME);

	double fast_speed = bounded_dome_number_value(DOME_SPEED_PROPERTY_NAME, DOME_SPEED_ITEM_NAME, 30);
	ASSERT_FALSE(isnan(fast_speed));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dome_simulator.device_name, DOME_SPEED_PROPERTY_NAME, DOME_SPEED_ITEM_NAME, fast_speed));
	ASSERT_TRUE(wait_for_property_state(DOME_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_SLAVING_PROPERTY_NAME, DOME_SLAVING_ENABLE_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_SLAVING_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_SLAVING_PROPERTY_NAME, DOME_SLAVING_DISABLE_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_SLAVING_PROPERTY_NAME, INDIGO_OK_STATE));

	double target_az = bounded_dome_number_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 3);
	ASSERT_FALSE(isnan(target_az));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dome_simulator.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, target_az));
	ASSERT_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, target_az, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	double slow_speed = bounded_dome_number_value(DOME_SPEED_PROPERTY_NAME, DOME_SPEED_ITEM_NAME, 1);
	double far_az = bounded_dome_number_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 180);
	ASSERT_FALSE(isnan(slow_speed));
	ASSERT_FALSE(isnan(far_az));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dome_simulator.device_name, DOME_SPEED_PROPERTY_NAME, DOME_SPEED_ITEM_NAME, slow_speed));
	ASSERT_TRUE(wait_for_property_state(DOME_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dome_simulator.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, far_az));
	ASSERT_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dome_simulator.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_property_not_busy(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME));

	stop_connected_simulator(&dome_simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "simulator_exposes_expected_properties", simulator_exposes_expected_properties },
		{ "simulator_passes_dome_compliance_checks", simulator_passes_dome_compliance_checks }
	};
	return indigo_run_tests("dome simulator integration tests", tests, ARRAY_SIZE(tests));
}
