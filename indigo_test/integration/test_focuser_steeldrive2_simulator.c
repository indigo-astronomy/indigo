// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <math.h>

#include <indigo_drivers/focuser_steeldrive2/indigo_focuser_steeldrive2.h>

#include "serial_simulator_test_common.h"

#ifndef FOCUSER_STEELDRIVE2_SIMULATOR_EXECUTABLE
#define FOCUSER_STEELDRIVE2_SIMULATOR_EXECUTABLE "build/integration/focuser_steeldrive2_simulator"
#endif

#define X_NAME_PROPERTY_NAME               "X_NAME"
#define X_NAME_ITEM_NAME                   "NAME"
#define X_SAVED_VALUES_PROPERTY_NAME       "X_SAVED_VALUES"
#define X_SELECT_TC_SENSOR_PROPERTY_NAME   "X_SELECT_TC_SENSOR"
#define X_RESET_PROPERTY_NAME              "X_RESET"
#define X_USE_ENDSTOP_PROPERTY_NAME        "X_USE_ENDSTOP"
#define X_START_ZEROING_PROPERTY_NAME      "X_START_ZEROING"
#define X_USE_PID_PROPERTY_NAME            "X_USE_PID"
#define X_PID_SETTINGS_PROPERTY_NAME       "X_PID_SETTINGS"
#define X_SELECT_PID_SENSOR_PROPERTY_NAME  "X_SELECT_PID_SENSOR"
#define X_SELECT_AMB_SENSOR_PROPERTY_NAME  "X_SELECT_AMB_SENSOR"
#define X_USE_AUTO_DEW_PROPERTY_NAME       "X_USE_AUTO_DEW"

static const simulator_driver_case steeldrive2_focuser = {
	"Baader Planetarium SteelDriveII Focuser",
	"indigo_focuser_steeldrive2",
	"SteelDriveII (focuser)",
	indigo_focuser_steeldrive2,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case steeldrive2_aux = {
	"Baader Planetarium SteelDriveII Focuser",
	"indigo_focuser_steeldrive2",
	"SteelDriveII (aux)",
	indigo_focuser_steeldrive2,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void steeldrive2_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_STEELDRIVE2_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&steeldrive2_focuser, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_PERIOD_ITEM_NAME);
	assert_property_has_item(X_NAME_PROPERTY_NAME, X_NAME_ITEM_NAME);
	assert_property_has_item(X_SAVED_VALUES_PROPERTY_NAME, "FOCUS");
	assert_property_has_item(X_SELECT_TC_SENSOR_PROPERTY_NAME, "AVG");
	assert_property_has_item(X_RESET_PROPERTY_NAME, "RESET");
	assert_property_has_item(X_USE_ENDSTOP_PROPERTY_NAME, "ENABLED");
	assert_property_has_item(X_START_ZEROING_PROPERTY_NAME, "START");
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, steeldrive2_focuser.device_name, X_NAME_PROPERTY_NAME, X_NAME_ITEM_NAME, "SD2_TEST"));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_NAME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_focuser.device_name, X_SAVED_VALUES_PROPERTY_NAME, "BKLGT", 60));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SAVED_VALUES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME, 3));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_COMPENSATION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, X_SELECT_TC_SENSOR_PROPERTY_NAME, "SENSOR_0", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SELECT_TC_SENSOR_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, X_USE_ENDSTOP_PROPERTY_NAME, "ENABLED", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_USE_ENDSTOP_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1175, 1));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&steeldrive2_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

static void steeldrive2_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, FOCUSER_STEELDRIVE2_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&steeldrive2_aux, steeldrive2_focuser.device_name, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(X_USE_AUTO_DEW_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(X_USE_PID_PROPERTY_NAME, "ENABLED");
	assert_property_has_item(X_PID_SETTINGS_PROPERTY_NAME, "PID_DEW_OFS");
	assert_property_has_item(X_PID_SETTINGS_PROPERTY_NAME, "PID TARGET");
	assert_property_has_item(X_SELECT_PID_SENSOR_PROPERTY_NAME, "AVG");
	assert_property_has_item(X_SELECT_AMB_SENSOR_PROPERTY_NAME, "SENSOR_0");

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_aux.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 40));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_aux.device_name, X_USE_AUTO_DEW_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_USE_AUTO_DEW_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_aux.device_name, X_USE_PID_PROPERTY_NAME, "ENABLED", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_USE_PID_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, steeldrive2_aux.device_name, X_PID_SETTINGS_PROPERTY_NAME, "PID TARGET", 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_PID_SETTINGS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_aux.device_name, X_SELECT_PID_SENSOR_PROPERTY_NAME, "SENSOR_1", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SELECT_PID_SENSOR_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, steeldrive2_aux.device_name, X_SELECT_AMB_SENSOR_PROPERTY_NAME, "SENSOR_0", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_SELECT_AMB_SENSOR_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&steeldrive2_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "steeldrive2_focuser_passes_serial_compliance_checks", steeldrive2_focuser_passes_serial_compliance_checks },
		{ "steeldrive2_aux_passes_serial_compliance_checks", steeldrive2_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("SteelDriveII serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
