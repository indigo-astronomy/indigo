// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_primaluce/indigo_focuser_primaluce.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_PRIMALUCE_SIMULATOR_EXECUTABLE
#define FOCUSER_PRIMALUCE_SIMULATOR_EXECUTABLE "build/integration/focuser_primaluce_simulator"
#endif

#define PRIMALUCE_FOCUSER_NAME             "PrimaluceLab Focuser"
#define PRIMALUCE_ROTATOR_NAME             "PrimaluceLab Rotator"
#define X_CONFIG_PROPERTY_NAME             "X_CONFIG"
#define X_CONFIG_M1ACC_ITEM_NAME           "M1ACC"
#define X_STATE_PROPERTY_NAME              "X_STATE"
#define X_STATE_MOTOR_TEMP_ITEM_NAME       "MOTOR_TEMP"
#define X_STATE_VIN_12V_ITEM_NAME          "VIN_12V"
#define X_WIFI_PROPERTY_NAME               "X_WIFI"
#define X_WIFI_OFF_ITEM_NAME               "OFF"
#define X_WIFI_AP_ITEM_NAME                "AP"
#define X_WIFI_AP_PROPERTY_NAME            "X_WIFI_AP"
#define X_WIFI_AP_SSID_ITEM_NAME           "AP_SSID"
#define X_WIFI_AP_PASSWORD_ITEM_NAME       "AP_PASSWORD"
#define X_LEDS_PROPERTY_NAME               "X_LEDS"
#define X_LEDS_DIM_ITEM_NAME               "DIM"
#define X_RUNPRESET_PROPERTY_NAME          "X_RUNPRESET"
#define X_RUNPRESET_M_ITEM_NAME            "M"
#define X_HOLD_CURR_PROPERTY_NAME          "X_HOLD_CURR"
#define X_HOLD_CURR_OFF_ITEM_NAME          "OFF"
#define X_HOLD_CURR_ON_ITEM_NAME           "ON"
#define X_CALIBRATE_F_PROPERTY_NAME        "X_CALIBRATE"
#define X_CALIBRATE_F_START_ITEM_NAME      "START"
#define X_CALIBRATE_R_PROPERTY_NAME        "X_CALIBRATE_A"
#define X_CALIBRATE_R_START_ITEM_NAME      "START"

static const simulator_driver_case primaluce_focuser = {
	"PrimaluceLab Focuser/Rotator",
	"indigo_focuser_primaluce",
	PRIMALUCE_FOCUSER_NAME,
	indigo_focuser_primaluce,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case primaluce_rotator = {
	"PrimaluceLab Focuser/Rotator",
	"indigo_focuser_primaluce",
	PRIMALUCE_ROTATOR_NAME,
	indigo_focuser_primaluce,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void primaluce_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_PRIMALUCE_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&primaluce_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(X_CONFIG_PROPERTY_NAME, X_CONFIG_M1ACC_ITEM_NAME);
	assert_property_has_item(X_STATE_PROPERTY_NAME, X_STATE_MOTOR_TEMP_ITEM_NAME);
	assert_property_has_item(X_STATE_PROPERTY_NAME, X_STATE_VIN_12V_ITEM_NAME);
	assert_property_has_item(X_WIFI_PROPERTY_NAME, X_WIFI_OFF_ITEM_NAME);
	assert_property_has_item(X_WIFI_PROPERTY_NAME, X_WIFI_AP_ITEM_NAME);
	assert_property_has_item(X_WIFI_AP_PROPERTY_NAME, X_WIFI_AP_SSID_ITEM_NAME);
	assert_property_has_item(X_WIFI_AP_PROPERTY_NAME, X_WIFI_AP_PASSWORD_ITEM_NAME);
	assert_property_has_item(X_LEDS_PROPERTY_NAME, X_LEDS_DIM_ITEM_NAME);
	assert_property_has_item(X_RUNPRESET_PROPERTY_NAME, X_RUNPRESET_M_ITEM_NAME);
	assert_property_has_item(X_HOLD_CURR_PROPERTY_NAME, X_HOLD_CURR_OFF_ITEM_NAME);
	assert_property_has_item(X_HOLD_CURR_PROPERTY_NAME, X_HOLD_CURR_ON_ITEM_NAME);
	assert_property_has_item(X_CALIBRATE_F_PROPERTY_NAME, X_CALIBRATE_F_START_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 12));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 0));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_focuser.device_name, X_WIFI_PROPERTY_NAME, X_WIFI_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_WIFI_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_focuser.device_name, X_LEDS_PROPERTY_NAME, X_LEDS_DIM_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_LEDS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_focuser.device_name, X_HOLD_CURR_PROPERTY_NAME, X_HOLD_CURR_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_HOLD_CURR_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_focuser.device_name, X_RUNPRESET_PROPERTY_NAME, X_RUNPRESET_M_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_RUNPRESET_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 18100));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 18100, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 18125, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 18500));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&primaluce_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

static void primaluce_rotator_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_PRIMALUCE_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&primaluce_rotator, primaluce_focuser.device_name, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_ROTATOR);
	assert_property_has_item(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);
	assert_property_has_item(ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(X_CALIBRATE_R_PROPERTY_NAME, X_CALIBRATE_R_START_ITEM_NAME);
	assert_number_item_in_range(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, primaluce_rotator.device_name, ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&primaluce_rotator);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "primaluce_focuser_passes_serial_compliance_checks", primaluce_focuser_passes_serial_compliance_checks },
		{ "primaluce_rotator_passes_serial_compliance_checks", primaluce_rotator_passes_serial_compliance_checks }
	};
	return indigo_run_tests("PrimaLuceLab serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
