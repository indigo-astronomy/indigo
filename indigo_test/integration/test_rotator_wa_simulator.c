// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/rotator_wa/indigo_rotator_wa.h>

#include "serial_simulator_test_common.h"

#ifndef ROTATOR_WA_SIMULATOR_EXECUTABLE
#define ROTATOR_WA_SIMULATOR_EXECUTABLE "build/integration/rotator_wa_simulator"
#endif

#define WA_SET_ZERO_POSITION_PROPERTY_NAME "X_SET_ZERO_POSITION"
#define WA_SET_ZERO_POSITION_ITEM_NAME "SET_ZERO_POSITION"

static const simulator_driver_case wa_rotator = {
	"WandererAstro Rotator",
	"indigo_rotator_wa",
	"WandererAstro rotator",
	indigo_rotator_wa,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void wa_rotator_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, ROTATOR_WA_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&wa_rotator, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_ROTATOR);
	assert_serial_rotator_class_property_completeness();
	assert_property_has_item(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	assert_property_has_item(ROTATOR_RAW_POSITION_PROPERTY_NAME, ROTATOR_RAW_POSITION_ITEM_NAME);
	assert_property_has_item(ROTATOR_POSITION_OFFSET_PROPERTY_NAME, ROTATOR_POSITION_OFFSET_ITEM_NAME);
	assert_property_has_item(ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_NORMAL_ITEM_NAME);
	assert_property_has_item(ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_REVERSED_ITEM_NAME);
	assert_property_has_item(ROTATOR_BACKLASH_PROPERTY_NAME, ROTATOR_BACKLASH_ITEM_NAME);
	assert_property_has_item(ROTATOR_RELATIVE_MOVE_PROPERTY_NAME, ROTATOR_RELATIVE_MOVE_ITEM_NAME);
	assert_property_has_item(ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_property_has_item(WA_SET_ZERO_POSITION_PROPERTY_NAME, WA_SET_ZERO_POSITION_ITEM_NAME);
	assert_number_item_in_range(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	assert_number_item_in_range(ROTATOR_BACKLASH_PROPERTY_NAME, ROTATOR_BACKLASH_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_BACKLASH_PROPERTY_NAME, ROTATOR_BACKLASH_ITEM_NAME, 2));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_DIRECTION_REVERSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 12, 0.1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_RELATIVE_MOVE_PROPERTY_NAME, ROTATOR_RELATIVE_MOVE_ITEM_NAME, 3));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_RELATIVE_MOVE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 15, 0.1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 22));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 22, 0.1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wa_rotator.device_name, WA_SET_ZERO_POSITION_PROPERTY_NAME, WA_SET_ZERO_POSITION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(WA_SET_ZERO_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 0, 0.1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wa_rotator.device_name, ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&wa_rotator);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "wa_rotator_passes_serial_compliance_checks", wa_rotator_passes_serial_compliance_checks }
	};
	return indigo_run_tests("WandererAstro rotator serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
