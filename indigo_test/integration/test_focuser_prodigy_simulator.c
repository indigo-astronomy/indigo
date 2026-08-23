// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <math.h>

#include <indigo_drivers/focuser_prodigy/indigo_focuser_prodigy.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE
#define FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE "build/integration/focuser_prodigy_simulator"
#endif

#define PRODIGY_FOCUSER_NAME              "Pegasus Prodigy Focuser"
#define PRODIGY_POWERBOX_NAME             "Pegasus Prodigy Powerbox"
#define X_FOCUSER_PARK_PROPERTY_NAME      "X_FOCUSER_PARK"
#define X_FOCUSER_PARK_ITEM_NAME          "PARK"
#define X_AUX_REBOOT_PROPERTY_NAME        "X_AUX_REBOOT"
#define X_AUX_REBOOT_ITEM_NAME            "REBOOT"

static const simulator_driver_case prodigy_focuser = {
	"PegasusAstro Prodigy Microfocuser",
	"indigo_focuser_prodigy",
	PRODIGY_FOCUSER_NAME,
	indigo_focuser_prodigy,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case prodigy_powerbox = {
	"PegasusAstro Prodigy Microfocuser",
	"indigo_focuser_prodigy",
	PRODIGY_POWERBOX_NAME,
	indigo_focuser_prodigy,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void prodigy_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&prodigy_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
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
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(X_FOCUSER_PARK_PROPERTY_NAME, X_FOCUSER_PARK_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	double sync_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	SERIAL_CHECK_TRUE(!isnan(sync_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 500));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position, 1));

	double target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position + 200);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position + 50, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, X_FOCUSER_PARK_PROPERTY_NAME, X_FOCUSER_PARK_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_FOCUSER_PARK_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 0, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&prodigy_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

static void prodigy_powerbox_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&prodigy_powerbox, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWER_OUTLET_NAME_1_ITEM_NAME);
	assert_property_has_item(AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWER_OUTLET_NAME_2_ITEM_NAME);
	assert_property_has_item(AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_USB_PORT_NAME_1_ITEM_NAME);
	assert_property_has_item(AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_USB_PORT_NAME_2_ITEM_NAME);
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME);
	assert_property_has_item(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_2_ITEM_NAME);
	assert_property_has_item(X_AUX_REBOOT_PROPERTY_NAME, X_AUX_REBOOT_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_USB_PORT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_USB_PORT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, X_AUX_REBOOT_PROPERTY_NAME, X_AUX_REBOOT_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_AUX_REBOOT_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&prodigy_powerbox);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "prodigy_focuser_passes_serial_compliance_checks", prodigy_focuser_passes_serial_compliance_checks },
		{ "prodigy_powerbox_passes_serial_compliance_checks", prodigy_powerbox_passes_serial_compliance_checks }
	};
	return indigo_run_tests("PegasusAstro Prodigy serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
