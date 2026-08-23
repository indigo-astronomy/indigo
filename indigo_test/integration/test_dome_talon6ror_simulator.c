// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_talon6ror/indigo_dome_talon6ror.h>

#include "serial_simulator_test_common.h"

#ifndef DOME_TALON6ROR_SIMULATOR_EXECUTABLE
#define DOME_TALON6ROR_SIMULATOR_EXECUTABLE "build/integration/dome_talon6ror_simulator"
#endif

#define DOME_TALON6ROR_NAME                 "Talon6 ROR"
#define X_SENSORS_PROPERTY_NAME             "X_SENSORS"
#define X_SENSORS_POWER_CONDITION_ITEM_NAME "POWER_CONDITION"
#define X_SENSORS_WEATHER_CONDITION_ITEM_NAME "WEATHER_CONDITION"
#define X_SENSORS_PARKED_SENSOR_ITEM_NAME   "PARKED_SENSOR"
#define X_SENSORS_OPEN_SENSOR_ITEM_NAME     "OPEN_SENSOR"
#define X_SENSORS_CLOSE_SENSOR_ITEM_NAME    "CLOSE_SENSOR"
#define X_MOTOR_CONF_PROPERTY_NAME          "X_MOTOR_CONF"
#define X_MOTOR_CONF_KP_ITEM_NAME           "KP"
#define X_MOTOR_CONF_MAX_SPEED_ITEM_NAME    "MAX_SPEED"
#define X_MOTOR_CONF_REVERSE_ITEM_NAME      "REVERSE"
#define X_DELAY_CONF_PROPERTY_NAME          "X_DELAY_CONF"
#define X_DELAY_CONF_PARK_ITEM_NAME         "PARK"
#define X_DELAY_CONF_TIMEOUT_ITEM_NAME      "TIMEOUT"
#define X_CLOSE_COND_PROPERTY_NAME          "X_CLOSE_COND"
#define X_CLOSE_COND_WEATHER_ITEM_NAME      "WEATHER"
#define X_CLOSE_COND_POWER_ITEM_NAME        "POWER"
#define X_CLOSE_COND_TIMEOUT_ITEM_NAME      "TIMEOUT"
#define X_CLOSE_TIMER_PROPERTY_NAME         "X_TIMER_COND"
#define X_POSITION_PROPERTY_NAME            "X_POSITION_PROPERTY"
#define X_POSITION_ITEM_NAME                "POSITION"
#define X_STATUS_PROPERTY_NAME              "X_STATUS_PROPERTY"
#define X_STATUS_VOLTAGE_ITEM_NAME          "VOLTAGE"

static const simulator_driver_case talon6ror_case = {
	DOME_TALON6ROR_NAME,
	"indigo_dome_talon6ror",
	DOME_TALON6ROR_NAME,
	indigo_dome_talon6ror,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void talon6ror_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, DOME_TALON6ROR_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&talon6ror_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_DOME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(X_SENSORS_PROPERTY_NAME, X_SENSORS_POWER_CONDITION_ITEM_NAME);
	assert_property_has_item(X_SENSORS_PROPERTY_NAME, X_SENSORS_WEATHER_CONDITION_ITEM_NAME);
	assert_property_has_item(X_SENSORS_PROPERTY_NAME, X_SENSORS_PARKED_SENSOR_ITEM_NAME);
	assert_property_has_item(X_SENSORS_PROPERTY_NAME, X_SENSORS_OPEN_SENSOR_ITEM_NAME);
	assert_property_has_item(X_SENSORS_PROPERTY_NAME, X_SENSORS_CLOSE_SENSOR_ITEM_NAME);
	assert_property_has_item(X_MOTOR_CONF_PROPERTY_NAME, X_MOTOR_CONF_KP_ITEM_NAME);
	assert_property_has_item(X_MOTOR_CONF_PROPERTY_NAME, X_MOTOR_CONF_MAX_SPEED_ITEM_NAME);
	assert_property_has_item(X_MOTOR_CONF_PROPERTY_NAME, X_MOTOR_CONF_REVERSE_ITEM_NAME);
	assert_property_has_item(X_DELAY_CONF_PROPERTY_NAME, X_DELAY_CONF_PARK_ITEM_NAME);
	assert_property_has_item(X_DELAY_CONF_PROPERTY_NAME, X_DELAY_CONF_TIMEOUT_ITEM_NAME);
	assert_property_has_item(X_CLOSE_COND_PROPERTY_NAME, X_CLOSE_COND_WEATHER_ITEM_NAME);
	assert_property_has_item(X_CLOSE_COND_PROPERTY_NAME, X_CLOSE_COND_POWER_ITEM_NAME);
	assert_property_has_item(X_CLOSE_COND_PROPERTY_NAME, X_CLOSE_COND_TIMEOUT_ITEM_NAME);
	assert_property_has_item(X_CLOSE_TIMER_PROPERTY_NAME, X_CLOSE_COND_TIMEOUT_ITEM_NAME);
	assert_property_has_item(X_POSITION_PROPERTY_NAME, X_POSITION_ITEM_NAME);
	assert_property_has_item(X_STATUS_PROPERTY_NAME, X_STATUS_VOLTAGE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, talon6ror_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, talon6ror_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, talon6ror_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, talon6ror_case.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, talon6ror_case.device_name, X_MOTOR_CONF_PROPERTY_NAME, X_MOTOR_CONF_KP_ITEM_NAME, 181));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_MOTOR_CONF_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, talon6ror_case.device_name, X_DELAY_CONF_PROPERTY_NAME, X_DELAY_CONF_TIMEOUT_ITEM_NAME, 11));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_DELAY_CONF_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, talon6ror_case.device_name, X_CLOSE_COND_PROPERTY_NAME, X_CLOSE_COND_TIMEOUT_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_CLOSE_COND_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&talon6ror_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "talon6ror_passes_serial_compliance_checks", talon6ror_passes_serial_compliance_checks },
	};
	return indigo_run_tests("Talon6 ROR serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
