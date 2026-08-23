// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_dsd/indigo_focuser_dsd.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_DSD_SIMULATOR_EXECUTABLE
#define FOCUSER_DSD_SIMULATOR_EXECUTABLE "build/integration/focuser_dsd_simulator"
#endif

#define DSD_STEP_MODE_PROPERTY_NAME              "DSD_STEP_MODE"
#define DSD_STEP_MODE_FULL_ITEM_NAME             "FULL"
#define DSD_STEP_MODE_EIGTH_ITEM_NAME            "EIGTH"
#define DSD_COILS_MODE_PROPERTY_NAME             "DSD_COILS_MODE"
#define DSD_COILS_MODE_IDLE_OFF_ITEM_NAME        "OFF_WHEN_IDLE"
#define DSD_COILS_MODE_ALWAYS_ON_ITEM_NAME       "ALWAYS_ON"
#define DSD_CURRENT_CONTROL_PROPERTY_NAME        "DSD_CURRENT_CONTROL"
#define DSD_CURRENT_CONTROL_MOVE_ITEM_NAME       "MOVE_CURRENT"
#define DSD_CURRENT_CONTROL_HOLD_ITEM_NAME       "HOLD_CURRENT"
#define DSD_TIMINGS_PROPERTY_NAME                "DSD_TIMINGS"
#define DSD_TIMINGS_SETTLE_ITEM_NAME             "SETTLE_TIME"
#define DSD_TIMINGS_COILS_TOUT_ITEM_NAME         "COILS_POWER_TIMEOUT"

static const simulator_driver_case dsd_focuser = {
	"Deep Sky Dad Focuser",
	"indigo_focuser_dsd",
	FOCUSER_DSD_NAME,
	indigo_focuser_dsd,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void dsd_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_DSD_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&dsd_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(DSD_STEP_MODE_PROPERTY_NAME, DSD_STEP_MODE_FULL_ITEM_NAME);
	assert_property_has_item(DSD_STEP_MODE_PROPERTY_NAME, DSD_STEP_MODE_EIGTH_ITEM_NAME);
	assert_property_has_item(DSD_COILS_MODE_PROPERTY_NAME, DSD_COILS_MODE_IDLE_OFF_ITEM_NAME);
	assert_property_has_item(DSD_COILS_MODE_PROPERTY_NAME, DSD_COILS_MODE_ALWAYS_ON_ITEM_NAME);
	assert_property_has_item(DSD_CURRENT_CONTROL_PROPERTY_NAME, DSD_CURRENT_CONTROL_MOVE_ITEM_NAME);
	assert_property_has_item(DSD_CURRENT_CONTROL_PROPERTY_NAME, DSD_CURRENT_CONTROL_HOLD_ITEM_NAME);
	assert_property_has_item(DSD_TIMINGS_PROPERTY_NAME, DSD_TIMINGS_SETTLE_ITEM_NAME);
	assert_property_has_item(DSD_TIMINGS_PROPERTY_NAME, DSD_TIMINGS_COILS_TOUT_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 3));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 0));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, DSD_STEP_MODE_PROPERTY_NAME, DSD_STEP_MODE_FULL_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DSD_STEP_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, DSD_COILS_MODE_PROPERTY_NAME, DSD_COILS_MODE_ALWAYS_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DSD_COILS_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, DSD_CURRENT_CONTROL_PROPERTY_NAME, DSD_CURRENT_CONTROL_MOVE_ITEM_NAME, 60));
	SERIAL_CHECK_TRUE(wait_for_property_state(DSD_CURRENT_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, DSD_TIMINGS_PROPERTY_NAME, DSD_TIMINGS_SETTLE_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_state(DSD_TIMINGS_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, dsd_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&dsd_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "dsd_focuser_passes_serial_compliance_checks", dsd_focuser_passes_serial_compliance_checks },
	};
	return indigo_run_tests("DSD serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
