// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/rotator_simulator/indigo_rotator_simulator.h>

#include "simulator_test_common.h"

static const char *rotator_connected_properties[] = {
	ROTATOR_ABORT_MOTION_PROPERTY_NAME,
	ROTATOR_POSITION_PROPERTY_NAME,
	ROTATOR_ON_POSITION_SET_PROPERTY_NAME
};

static const char *rotator_hidden_connected_properties[] = {
	ROTATOR_DIRECTION_PROPERTY_NAME,
	ROTATOR_STEPS_PER_REVOLUTION_PROPERTY_NAME,
	ROTATOR_RELATIVE_MOVE_PROPERTY_NAME,
	ROTATOR_BACKLASH_PROPERTY_NAME,
	ROTATOR_LIMITS_PROPERTY_NAME,
	ROTATOR_RAW_POSITION_PROPERTY_NAME,
	ROTATOR_POSITION_OFFSET_PROPERTY_NAME
};

static const simulator_driver_case rotator_simulator = {
	"Field Rotator Simulator",
	"indigo_rotator_simulator",
	"Field Rotator Simulator",
	indigo_rotator_simulator,
	false,
	base_properties_with_instances,
	ARRAY_SIZE(base_properties_with_instances),
	hidden_base_properties,
	ARRAY_SIZE(hidden_base_properties),
	rotator_connected_properties,
	ARRAY_SIZE(rotator_connected_properties),
	rotator_hidden_connected_properties,
	ARRAY_SIZE(rotator_hidden_connected_properties)
};

static void driver_info_reports_simulator_metadata(void) {
	assert_simulator_driver_info(&rotator_simulator);
}

static void simulator_exposes_expected_properties(void) {
	assert_simulator_properties(&rotator_simulator);
}

static double rotator_target_in_range(double preferred_value) {
	indigo_item *position = find_cached_item(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	if (position == NULL) {
		return NAN;
	}
	if (preferred_value >= position->number.min && preferred_value <= position->number.max) {
		return preferred_value;
	}
	return (position->number.min + position->number.max) / 2;
}

static void assert_rotator_moves_to(double target_position) {
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, target_position));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, target_position, 0.001));
}

static void simulator_passes_rotator_compliance_checks(void) {
	static const char *position_items[] = {
		ROTATOR_POSITION_ITEM_NAME
	};
	static const char *on_position_set_items[] = {
		ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME,
		ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME
	};
	start_connected_simulator(&rotator_simulator);

	assert_device_interface(INDIGO_INTERFACE_ROTATOR);
	assert_property_has_items(ROTATOR_POSITION_PROPERTY_NAME, position_items, ARRAY_SIZE(position_items));
	assert_property_has_item(ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME);
	assert_property_has_items(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, on_position_set_items, ARRAY_SIZE(on_position_set_items));
	assert_number_item_in_range(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);

	double original_position = cached_number_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	assert_rotator_moves_to(rotator_target_in_range(90));
	assert_rotator_moves_to(rotator_target_in_range(180));

	double far_position = rotator_target_in_range(270);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, far_position));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_ALERT_STATE));

	double sync_position = rotator_target_in_range(45);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, rotator_simulator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, sync_position));
	ASSERT_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));

	assert_rotator_moves_to(original_position);

	stop_connected_simulator(&rotator_simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "simulator_exposes_expected_properties", simulator_exposes_expected_properties },
		{ "simulator_passes_rotator_compliance_checks", simulator_passes_rotator_compliance_checks }
	};
	return indigo_run_tests("rotator simulator integration tests", tests, ARRAY_SIZE(tests));
}
