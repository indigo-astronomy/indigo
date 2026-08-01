// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_simulator/indigo_mount_simulator.h>

#include "simulator_test_common.h"

static const char *hidden_mount_base_properties[] = {
	DEVICE_PORT_PROPERTY_NAME,
	DEVICE_BAUDRATE_PROPERTY_NAME,
	DEVICE_PORTS_PROPERTY_NAME
};

static const char *mount_base_properties[] = {
	INFO_PROPERTY_NAME,
	SIMULATION_PROPERTY_NAME,
	CONFIG_PROPERTY_NAME,
	PROFILE_NAME_PROPERTY_NAME,
	PROFILE_PROPERTY_NAME,
	AUTHENTICATION_PROPERTY_NAME,
	ADDITIONAL_INSTANCES_PROPERTY_NAME,
	CONNECTION_PROPERTY_NAME
};

static const char *mount_connected_properties[] = {
	MOUNT_INFO_PROPERTY_NAME,
	GEOGRAPHIC_COORDINATES_PROPERTY_NAME,
	MOUNT_LST_TIME_PROPERTY_NAME,
	MOUNT_PARK_PROPERTY_NAME,
	MOUNT_PARK_SET_PROPERTY_NAME,
	MOUNT_PARK_POSITION_PROPERTY_NAME,
	MOUNT_HOME_PROPERTY_NAME,
	MOUNT_HOME_SET_PROPERTY_NAME,
	MOUNT_HOME_POSITION_PROPERTY_NAME,
	MOUNT_SLEW_RATE_PROPERTY_NAME,
	MOUNT_MOTION_DEC_PROPERTY_NAME,
	MOUNT_MOTION_RA_PROPERTY_NAME,
	MOUNT_TRACK_RATE_PROPERTY_NAME,
	MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME,
	MOUNT_TRACKING_PROPERTY_NAME,
	MOUNT_GUIDE_RATE_PROPERTY_NAME,
	MOUNT_ON_COORDINATES_SET_PROPERTY_NAME,
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME,
	MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME,
	MOUNT_ABORT_MOTION_PROPERTY_NAME,
	MOUNT_ALIGNMENT_MODE_PROPERTY_NAME,
	MOUNT_EPOCH_PROPERTY_NAME,
	MOUNT_SIDE_OF_PIER_PROPERTY_NAME,
	SNOOP_DEVICES_PROPERTY_NAME,
	MOUNT_STATE_PROPERTY_NAME
};

static const char *mount_hidden_connected_properties[] = {
	UTC_TIME_PROPERTY_NAME,
	MOUNT_SET_HOST_TIME_PROPERTY_NAME,
	MOUNT_RAW_COORDINATES_PROPERTY_NAME,
	MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME,
	MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY_NAME,
	MOUNT_ALIGNMENT_RESET_PROPERTY_NAME,
	MOUNT_PEC_PROPERTY_NAME,
	MOUNT_PEC_TRAINING_PROPERTY_NAME
};

static const char *guider_connected_properties[] = {
	GUIDER_GUIDE_DEC_PROPERTY_NAME,
	GUIDER_GUIDE_RA_PROPERTY_NAME,
	GUIDER_RATE_PROPERTY_NAME
};

static const simulator_driver_case mount_simulator = {
	"Mount Simulator",
	"indigo_mount_simulator",
	MOUNT_SIMULATOR_NAME,
	indigo_mount_simulator,
	false,
	mount_base_properties,
	ARRAY_SIZE(mount_base_properties),
	hidden_mount_base_properties,
	ARRAY_SIZE(hidden_mount_base_properties),
	mount_connected_properties,
	ARRAY_SIZE(mount_connected_properties),
	mount_hidden_connected_properties,
	ARRAY_SIZE(mount_hidden_connected_properties)
};

static const simulator_driver_case mount_guider_simulator = {
	"Mount Simulator",
	"indigo_mount_simulator",
	MOUNT_SIMULATOR_GUIDER_NAME,
	indigo_mount_simulator,
	false,
	base_properties_without_instances,
	ARRAY_SIZE(base_properties_without_instances),
	hidden_base_properties_without_instances,
	ARRAY_SIZE(hidden_base_properties_without_instances),
	guider_connected_properties,
	ARRAY_SIZE(guider_connected_properties),
	NULL,
	0
};

static void driver_info_reports_simulator_metadata(void) {
	assert_simulator_driver_info(&mount_simulator);
}

static void mount_exposes_expected_properties(void) {
	assert_simulator_properties(&mount_simulator);
}

static void mount_guider_exposes_expected_properties(void) {
	assert_simulator_properties(&mount_guider_simulator);
}

static double bounded_pulse_value(const char *property_name, const char *item_name) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		return NAN;
	}
	if (item->number.max < 200) {
		return item->number.max;
	}
	return 200;
}

static double bounded_mount_number_value(const char *property_name, const char *item_name, double preferred_value) {
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

static void assert_guide_pulse_resets(const char *property_name, const char *item_name) {
	double pulse = bounded_pulse_value(property_name, item_name);
	ASSERT_TRUE(pulse > 0);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mount_guider_simulator.device_name, property_name, item_name, pulse));
	ASSERT_TRUE(wait_for_property_state(property_name, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(property_name, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(property_name, item_name, 0, 0.001));
}

static void mount_guider_passes_guider_compliance_checks(void) {
	static const char *guide_ra_items[] = {
		GUIDER_GUIDE_EAST_ITEM_NAME,
		GUIDER_GUIDE_WEST_ITEM_NAME
	};
	static const char *guide_dec_items[] = {
		GUIDER_GUIDE_NORTH_ITEM_NAME,
		GUIDER_GUIDE_SOUTH_ITEM_NAME
	};
	start_connected_simulator(&mount_guider_simulator);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_items(GUIDER_GUIDE_RA_PROPERTY_NAME, guide_ra_items, ARRAY_SIZE(guide_ra_items));
	assert_property_has_items(GUIDER_GUIDE_DEC_PROPERTY_NAME, guide_dec_items, ARRAY_SIZE(guide_dec_items));
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);

	assert_guide_pulse_resets(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_guide_pulse_resets(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);
	assert_guide_pulse_resets(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_guide_pulse_resets(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);

	double original_rate = cached_number_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);
	indigo_item *rate_item = find_cached_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);
	ASSERT_TRUE(rate_item != NULL);
	double test_rate = rate_item->number.max / 2;
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mount_guider_simulator.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, test_rate));
	ASSERT_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, test_rate, 0.001));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mount_guider_simulator.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, original_rate));
	ASSERT_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, original_rate, 0.001));

	stop_connected_simulator(&mount_guider_simulator);
}

static void mount_passes_mount_compliance_checks(void) {
	static const char *mount_info_items[] = {
		MOUNT_INFO_MODEL_ITEM_NAME,
		MOUNT_INFO_VENDOR_ITEM_NAME,
		MOUNT_INFO_FIRMWARE_ITEM_NAME
	};
	static const char *geographic_items[] = {
		GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME
	};
	static const char *park_items[] = {
		MOUNT_PARK_PARKED_ITEM_NAME,
		MOUNT_PARK_UNPARKED_ITEM_NAME
	};
	static const char *park_set_items[] = {
		MOUNT_PARK_SET_DEFAULT_ITEM_NAME,
		MOUNT_PARK_SET_CURRENT_ITEM_NAME
	};
	static const char *park_position_items[] = {
		MOUNT_PARK_POSITION_HA_ITEM_NAME,
		MOUNT_PARK_POSITION_DEC_ITEM_NAME
	};
	static const char *home_items[] = {
		MOUNT_HOME_ITEM_NAME
	};
	static const char *home_set_items[] = {
		MOUNT_HOME_SET_DEFAULT_ITEM_NAME,
		MOUNT_HOME_SET_CURRENT_ITEM_NAME
	};
	static const char *home_position_items[] = {
		MOUNT_HOME_POSITION_HA_ITEM_NAME,
		MOUNT_HOME_POSITION_DEC_ITEM_NAME
	};
	static const char *on_coordinates_set_items[] = {
		MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME,
		MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME
	};
	static const char *slew_rate_items[] = {
		MOUNT_SLEW_RATE_GUIDE_ITEM_NAME,
		MOUNT_SLEW_RATE_CENTERING_ITEM_NAME,
		MOUNT_SLEW_RATE_FIND_ITEM_NAME,
		MOUNT_SLEW_RATE_MAX_ITEM_NAME
	};
	static const char *motion_dec_items[] = {
		MOUNT_MOTION_NORTH_ITEM_NAME,
		MOUNT_MOTION_SOUTH_ITEM_NAME
	};
	static const char *motion_ra_items[] = {
		MOUNT_MOTION_WEST_ITEM_NAME,
		MOUNT_MOTION_EAST_ITEM_NAME
	};
	static const char *track_rate_items[] = {
		MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME,
		MOUNT_TRACK_RATE_SOLAR_ITEM_NAME,
		MOUNT_TRACK_RATE_LUNAR_ITEM_NAME,
		MOUNT_TRACK_RATE_KING_ITEM_NAME,
		MOUNT_TRACK_RATE_CUSTOM_ITEM_NAME
	};
	static const char *tracking_items[] = {
		MOUNT_TRACKING_ON_ITEM_NAME,
		MOUNT_TRACKING_OFF_ITEM_NAME
	};
	static const char *guide_rate_items[] = {
		MOUNT_GUIDE_RATE_RA_ITEM_NAME,
		MOUNT_GUIDE_RATE_DEC_ITEM_NAME
	};
	static const char *equatorial_items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	static const char *horizontal_items[] = {
		MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME,
		MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME
	};
	static const char *alignment_mode_items[] = {
		MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM_NAME,
		MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM_NAME,
		MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM_NAME,
		MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM_NAME
	};
	static const char *side_of_pier_items[] = {
		MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME,
		MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME
	};
	static const char *state_items[] = {
		MOUNT_STATE_SLEW_ITEM_NAME,
		MOUNT_STATE_PARK_ITEM_NAME,
		MOUNT_STATE_HOME_ITEM_NAME,
		MOUNT_STATE_TRACKING_ITEM_NAME
	};
	start_connected_simulator(&mount_simulator);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_items(MOUNT_INFO_PROPERTY_NAME, mount_info_items, ARRAY_SIZE(mount_info_items));
	assert_property_has_items(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, geographic_items, ARRAY_SIZE(geographic_items));
	assert_property_has_item(MOUNT_LST_TIME_PROPERTY_NAME, MOUNT_LST_TIME_ITEM_NAME);
	assert_property_has_items(MOUNT_PARK_PROPERTY_NAME, park_items, ARRAY_SIZE(park_items));
	assert_property_has_items(MOUNT_PARK_SET_PROPERTY_NAME, park_set_items, ARRAY_SIZE(park_set_items));
	assert_property_has_items(MOUNT_PARK_POSITION_PROPERTY_NAME, park_position_items, ARRAY_SIZE(park_position_items));
	assert_property_has_items(MOUNT_HOME_PROPERTY_NAME, home_items, ARRAY_SIZE(home_items));
	assert_property_has_items(MOUNT_HOME_SET_PROPERTY_NAME, home_set_items, ARRAY_SIZE(home_set_items));
	assert_property_has_items(MOUNT_HOME_POSITION_PROPERTY_NAME, home_position_items, ARRAY_SIZE(home_position_items));
	assert_property_has_items(MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, on_coordinates_set_items, ARRAY_SIZE(on_coordinates_set_items));
	assert_property_has_items(MOUNT_SLEW_RATE_PROPERTY_NAME, slew_rate_items, ARRAY_SIZE(slew_rate_items));
	assert_property_has_items(MOUNT_MOTION_DEC_PROPERTY_NAME, motion_dec_items, ARRAY_SIZE(motion_dec_items));
	assert_property_has_items(MOUNT_MOTION_RA_PROPERTY_NAME, motion_ra_items, ARRAY_SIZE(motion_ra_items));
	assert_property_has_items(MOUNT_TRACK_RATE_PROPERTY_NAME, track_rate_items, ARRAY_SIZE(track_rate_items));
	assert_property_has_items(MOUNT_TRACKING_PROPERTY_NAME, tracking_items, ARRAY_SIZE(tracking_items));
	assert_property_has_items(MOUNT_GUIDE_RATE_PROPERTY_NAME, guide_rate_items, ARRAY_SIZE(guide_rate_items));
	assert_property_has_item(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME);
	assert_property_has_items(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, equatorial_items, ARRAY_SIZE(equatorial_items));
	assert_property_has_items(MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, horizontal_items, ARRAY_SIZE(horizontal_items));
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);
	assert_property_has_items(MOUNT_ALIGNMENT_MODE_PROPERTY_NAME, alignment_mode_items, ARRAY_SIZE(alignment_mode_items));
	assert_property_has_item(MOUNT_EPOCH_PROPERTY_NAME, MOUNT_EPOCH_ITEM_NAME);
	assert_property_has_items(MOUNT_SIDE_OF_PIER_PROPERTY_NAME, side_of_pier_items, ARRAY_SIZE(side_of_pier_items));
	assert_property_has_items(MOUNT_STATE_PROPERTY_NAME, state_items, ARRAY_SIZE(state_items));
	assert_number_item_in_range(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME);
	assert_number_item_in_range(GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME);
	assert_number_item_in_range(MOUNT_PARK_POSITION_PROPERTY_NAME, MOUNT_PARK_POSITION_HA_ITEM_NAME);
	assert_number_item_in_range(MOUNT_PARK_POSITION_PROPERTY_NAME, MOUNT_PARK_POSITION_DEC_ITEM_NAME);
	assert_number_item_in_range(MOUNT_HOME_POSITION_PROPERTY_NAME, MOUNT_HOME_POSITION_HA_ITEM_NAME);
	assert_number_item_in_range(MOUNT_HOME_POSITION_PROPERTY_NAME, MOUNT_HOME_POSITION_DEC_ITEM_NAME);
	assert_number_item_in_range(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME);
	assert_number_item_in_range(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_DEC_ITEM_NAME);
	assert_number_item_in_range(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	assert_number_item_in_range(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_SLEW_RATE_PROPERTY_NAME, INDIGO_OK_STATE));

	double custom_rate = bounded_mount_number_value(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME, 0.25);
	ASSERT_FALSE(isnan(custom_rate));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME, custom_rate));
	ASSERT_TRUE(wait_for_property_state(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME, custom_rate, 0.001));

	double guide_rate = bounded_mount_number_value(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 0.5);
	ASSERT_FALSE(isnan(guide_rate));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, guide_rate));
	ASSERT_TRUE(wait_for_property_state(MOUNT_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, guide_rate, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	double sync_ra = bounded_mount_number_value(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, 1);
	double sync_dec = bounded_mount_number_value(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, 45);
	const char *coordinate_items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	double coordinate_values[] = {
		sync_ra,
		sync_dec
	};
	ASSERT_FALSE(isnan(sync_ra));
	ASSERT_FALSE(isnan(sync_dec));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, mount_simulator.device_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, ARRAY_SIZE(coordinate_items), coordinate_items, coordinate_values));
	ASSERT_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, sync_ra, 0.001));
	ASSERT_TRUE(wait_for_number_item_value(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, sync_dec, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, mount_simulator.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

	stop_connected_simulator(&mount_simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "mount_exposes_expected_properties", mount_exposes_expected_properties },
		{ "mount_passes_mount_compliance_checks", mount_passes_mount_compliance_checks },
		{ "mount_guider_exposes_expected_properties", mount_guider_exposes_expected_properties },
		{ "mount_guider_passes_guider_compliance_checks", mount_guider_passes_guider_compliance_checks }
	};
	return indigo_run_tests("mount simulator integration tests", tests, ARRAY_SIZE(tests));
}
