// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/rotator_falcon/indigo_rotator_falcon.h>

#include "serial_simulator_test_common.h"

#ifndef ROTATOR_FALCON2_SIMULATOR_EXECUTABLE
#define ROTATOR_FALCON2_SIMULATOR_EXECUTABLE "build/integration/rotator_falcon2_simulator"
#endif

static const simulator_driver_case falcon_rotator = {
	"PegasusAstro Falcon rotator",
	"indigo_rotator_falcon",
	"Pegasus Falcon rotator",
	indigo_rotator_falcon,
	false,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static void falcon2_rotator_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, ROTATOR_FALCON2_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&falcon_rotator, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_ROTATOR);
	assert_serial_rotator_class_property_completeness();
	assert_property_has_item(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	assert_property_has_item(ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_number_item_in_range(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);

	double target_position = bounded_number_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 1);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, falcon_rotator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, falcon_rotator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, falcon_rotator.device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, falcon_rotator.device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 12, 0.1));

cleanup:
	if (context.connected) {
		stop_serial_driver(&falcon_rotator);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "falcon2_rotator_passes_serial_compliance_checks", falcon2_rotator_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Falcon2 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
