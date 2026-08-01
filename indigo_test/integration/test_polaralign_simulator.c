// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/polaralign_simulator/indigo_polaralign_simulator.h>

#include "simulator_test_common.h"

static const char *polaralign_connected_properties[] = {
	POLARALIGN_OFFSET_PROPERTY_NAME,
	POLARALIGN_ABORT_MOTION_PROPERTY_NAME,
	POLARALIGN_STEPS_PER_DEGREE_PROPERTY_NAME,
	POLARALIGN_DIRECTION_ALT_PROPERTY_NAME,
	POLARALIGN_DIRECTION_AZ_PROPERTY_NAME,
	POLARALIGN_RESET_POSITION_ALT_PROPERTY_NAME,
	POLARALIGN_RESET_POSITION_AZ_PROPERTY_NAME,
	POLARALIGN_LIMITS_PROPERTY_NAME
};

static const simulator_driver_case polaralign_simulator = {
	"Polar Aligner Simulator",
	"indigo_polaralign_simulator",
	SIMULATOR_POLARALIGN_NAME,
	indigo_polaralign_simulator,
	true,
	base_properties_with_instances,
	ARRAY_SIZE(base_properties_with_instances),
	hidden_base_properties,
	ARRAY_SIZE(hidden_base_properties),
	polaralign_connected_properties,
	ARRAY_SIZE(polaralign_connected_properties),
	NULL,
	0
};

static void driver_info_reports_simulator_metadata(void) {
	assert_simulator_driver_info(&polaralign_simulator);
}

static void simulator_exposes_expected_properties(void) {
	assert_simulator_properties(&polaralign_simulator);
}

static double bounded_polaralign_number_value(const char *property_name, const char *item_name, double preferred_value) {
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

static void set_polaralign_offset(double altitude, double azimuth, indigo_property_state expected_state) {
	const char *items[] = {
		POLARALIGN_OFFSET_ALT_ITEM_NAME,
		POLARALIGN_OFFSET_AZ_ITEM_NAME
	};
	double values[] = {
		altitude,
		azimuth
	};
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_OFFSET_PROPERTY_NAME, ARRAY_SIZE(items), items, values));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_OFFSET_PROPERTY_NAME, expected_state));
}

static void simulator_passes_polaralign_compliance_checks(void) {
	static const char *offset_items[] = {
		POLARALIGN_OFFSET_ALT_ITEM_NAME,
		POLARALIGN_OFFSET_AZ_ITEM_NAME
	};
	static const char *steps_per_degree_items[] = {
		POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM_NAME,
		POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM_NAME
	};
	static const char *direction_items[] = {
		POLARALIGN_DIRECTION_NORMAL_ITEM_NAME,
		POLARALIGN_DIRECTION_REVERSED_ITEM_NAME
	};
	static const char *limit_items[] = {
		POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM_NAME,
		POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM_NAME,
		POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM_NAME,
		POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM_NAME
	};
	start_connected_simulator(&polaralign_simulator);

	assert_device_interface(INDIGO_INTERFACE_POLARALIGN);
	assert_property_has_items(POLARALIGN_OFFSET_PROPERTY_NAME, offset_items, ARRAY_SIZE(offset_items));
	assert_property_has_item(POLARALIGN_ABORT_MOTION_PROPERTY_NAME, POLARALIGN_ABORT_MOTION_ITEM_NAME);
	assert_property_has_items(POLARALIGN_STEPS_PER_DEGREE_PROPERTY_NAME, steps_per_degree_items, ARRAY_SIZE(steps_per_degree_items));
	assert_property_has_items(POLARALIGN_DIRECTION_ALT_PROPERTY_NAME, direction_items, ARRAY_SIZE(direction_items));
	assert_property_has_items(POLARALIGN_DIRECTION_AZ_PROPERTY_NAME, direction_items, ARRAY_SIZE(direction_items));
	assert_property_has_item(POLARALIGN_RESET_POSITION_ALT_PROPERTY_NAME, POLARALIGN_RESET_POSITION_ITEM_NAME);
	assert_property_has_item(POLARALIGN_RESET_POSITION_AZ_PROPERTY_NAME, POLARALIGN_RESET_POSITION_ITEM_NAME);
	assert_property_has_items(POLARALIGN_LIMITS_PROPERTY_NAME, limit_items, ARRAY_SIZE(limit_items));
	assert_number_item_in_range(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_ALT_ITEM_NAME);
	assert_number_item_in_range(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_AZ_ITEM_NAME);
	assert_number_item_in_range(POLARALIGN_STEPS_PER_DEGREE_PROPERTY_NAME, POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM_NAME);
	assert_number_item_in_range(POLARALIGN_STEPS_PER_DEGREE_PROPERTY_NAME, POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM_NAME);

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_DIRECTION_ALT_PROPERTY_NAME, POLARALIGN_DIRECTION_REVERSED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_DIRECTION_ALT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_DIRECTION_AZ_PROPERTY_NAME, POLARALIGN_DIRECTION_REVERSED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_DIRECTION_AZ_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_DIRECTION_ALT_PROPERTY_NAME, POLARALIGN_DIRECTION_NORMAL_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_DIRECTION_ALT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_DIRECTION_AZ_PROPERTY_NAME, POLARALIGN_DIRECTION_NORMAL_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_DIRECTION_AZ_PROPERTY_NAME, INDIGO_OK_STATE));

	double target_alt = bounded_polaralign_number_value(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_ALT_ITEM_NAME, 0);
	double target_az = bounded_polaralign_number_value(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_AZ_ITEM_NAME, 0);
	ASSERT_FALSE(isnan(target_alt));
	ASSERT_FALSE(isnan(target_az));
	set_polaralign_offset(target_alt, target_az, INDIGO_OK_STATE);
	ASSERT_TRUE(wait_for_number_item_value(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_ALT_ITEM_NAME, target_alt, 0.001));
	ASSERT_TRUE(wait_for_number_item_value(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_AZ_ITEM_NAME, target_az, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_RESET_POSITION_ALT_PROPERTY_NAME, POLARALIGN_RESET_POSITION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_RESET_POSITION_ALT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_ALT_ITEM_NAME, 0, 0.001));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_RESET_POSITION_AZ_PROPERTY_NAME, POLARALIGN_RESET_POSITION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_RESET_POSITION_AZ_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_OFFSET_AZ_ITEM_NAME, 0, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, polaralign_simulator.device_name, POLARALIGN_ABORT_MOTION_PROPERTY_NAME, POLARALIGN_ABORT_MOTION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(POLARALIGN_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

	stop_connected_simulator(&polaralign_simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "simulator_exposes_expected_properties", simulator_exposes_expected_properties },
		{ "simulator_passes_polaralign_compliance_checks", simulator_passes_polaralign_compliance_checks }
	};
	return indigo_run_tests("polar aligner simulator integration tests", tests, ARRAY_SIZE(tests));
}
