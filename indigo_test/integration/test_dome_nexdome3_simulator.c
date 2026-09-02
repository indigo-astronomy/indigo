// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_nexdome3/indigo_dome_nexdome3.h>

#include "serial_simulator_test_common.h"

#ifndef DOME_NEXDOME3_SIMULATOR_EXECUTABLE
#define DOME_NEXDOME3_SIMULATOR_EXECUTABLE "build/integration/dome_nexdome3_simulator"
#endif

#define NEXDOME_FIND_HOME_PROPERTY_NAME           "NEXDOME_FIND_HOME"
#define NEXDOME_FIND_HOME_ITEM_NAME               "FIND_HOME"
#define NEXDOME_HOME_POSITION_PROPERTY_NAME       "NEXDOME_HOME_POSITION"
#define NEXDOME_HOME_POSITION_ITEM_NAME           "POSITION"
#define NEXDOME_ACCELERATION_PROPERTY_NAME        "NEXDOME_ACCELERATION_TIME"
#define NEXDOME_ACCELERATION_ROTATOR_ITEM_NAME    "ROTATOR"
#define NEXDOME_ACCELERATION_SHUTTER_ITEM_NAME    "SHUTTER"
#define NEXDOME_VELOCITY_PROPERTY_NAME            "NEXDOME_VELOCITY"
#define NEXDOME_VELOCITY_ROTATOR_ITEM_NAME        "ROTATOR"
#define NEXDOME_VELOCITY_SHUTTER_ITEM_NAME        "SHUTTER"
#define NEXDOME_RANGE_PROPERTY_NAME               "NEXDOME_RANGE"
#define NEXDOME_RANGE_ROTATOR_ITEM_NAME           "ROTATOR"
#define NEXDOME_RANGE_SHUTTER_ITEM_NAME           "SHUTTER"
#define NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME      "NEXDOME_MOVE_THRESHOLD"
#define NEXDOME_MOVE_THRESHOLD_ITEM_NAME          "THRESHOLD"
#define NEXDOME_POWER_PROPERTY_NAME               "NEXDOME_BATTERY_POWER"
#define NEXDOME_POWER_VOLTAGE_ITEM_NAME           "VOLTAGE"
#define NEXDOME_SETTINGS_PROPERTY_NAME            "NEXDOME_SETTINGS"
#define NEXDOME_SETTINGS_LOAD_ITEM_NAME           "LOAD_EEPROM"
#define NEXDOME_SETTINGS_SAVE_ITEM_NAME           "SAVE_EEPROM"
#define NEXDOME_SETTINGS_DEFAULT_ITEM_NAME        "LOAD_DEFAULT"
#define NEXDOME_RAIN_PROPERTY_NAME                "NEXDOME_RAIN_SENSOR"
#define NEXDOME_RAIN_ALERT_ITEM_NAME              "RAIN_ALERT"
#define NEXDOME_XB_STATE_PROPERTY_NAME            "NEXDOME_XB_STATE"
#define NEXDOME_XB_STATE_ITEM_NAME                "XB_STATE"

static const simulator_driver_case nexdome3_case = {
	DOME_NEXDOME3_NAME,
	"indigo_dome_nexdome3",
	DOME_NEXDOME3_NAME,
	indigo_dome_nexdome3,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static double bounded_dome_number_value(const char *property_name, const char *item_name, double preferred_value) {
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

static void nexdome3_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	static const char *settings_items[] = {
		NEXDOME_SETTINGS_LOAD_ITEM_NAME,
		NEXDOME_SETTINGS_SAVE_ITEM_NAME,
		NEXDOME_SETTINGS_DEFAULT_ITEM_NAME
	};

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, DOME_NEXDOME3_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&nexdome3_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_DOME);

	assert_property_has_item(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME);
	assert_property_has_item(DOME_STEPS_PROPERTY_NAME, DOME_STEPS_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME);
	assert_property_has_item(DOME_SLAVING_PARAMETERS_PROPERTY_NAME, DOME_SLAVING_THRESHOLD_ITEM_NAME);
	assert_property_has_item(NEXDOME_FIND_HOME_PROPERTY_NAME, NEXDOME_FIND_HOME_ITEM_NAME);
	assert_property_has_item(NEXDOME_HOME_POSITION_PROPERTY_NAME, NEXDOME_HOME_POSITION_ITEM_NAME);
	assert_property_has_item(NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME, NEXDOME_MOVE_THRESHOLD_ITEM_NAME);
	assert_property_has_item(NEXDOME_POWER_PROPERTY_NAME, NEXDOME_POWER_VOLTAGE_ITEM_NAME);
	assert_property_has_item(NEXDOME_ACCELERATION_PROPERTY_NAME, NEXDOME_ACCELERATION_ROTATOR_ITEM_NAME);
	assert_property_has_item(NEXDOME_ACCELERATION_PROPERTY_NAME, NEXDOME_ACCELERATION_SHUTTER_ITEM_NAME);
	assert_property_has_item(NEXDOME_VELOCITY_PROPERTY_NAME, NEXDOME_VELOCITY_ROTATOR_ITEM_NAME);
	assert_property_has_item(NEXDOME_VELOCITY_PROPERTY_NAME, NEXDOME_VELOCITY_SHUTTER_ITEM_NAME);
	assert_property_has_item(NEXDOME_RANGE_PROPERTY_NAME, NEXDOME_RANGE_ROTATOR_ITEM_NAME);
	assert_property_has_item(NEXDOME_RANGE_PROPERTY_NAME, NEXDOME_RANGE_SHUTTER_ITEM_NAME);
	assert_property_has_items(NEXDOME_SETTINGS_PROPERTY_NAME, settings_items, ARRAY_SIZE(settings_items));
	assert_property_has_item(NEXDOME_RAIN_PROPERTY_NAME, NEXDOME_RAIN_ALERT_ITEM_NAME);
	assert_property_has_item(NEXDOME_XB_STATE_PROPERTY_NAME, NEXDOME_XB_STATE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome3_case.device_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome3_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome3_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	double target_az = bounded_dome_number_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 30);
	SERIAL_CHECK_TRUE(!isnan(target_az));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexdome3_case.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, target_az));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, target_az, 0.1));

	double threshold = bounded_dome_number_value(NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME, NEXDOME_MOVE_THRESHOLD_ITEM_NAME, 700);
	SERIAL_CHECK_TRUE(!isnan(threshold));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexdome3_case.device_name, NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME, NEXDOME_MOVE_THRESHOLD_ITEM_NAME, threshold));
	SERIAL_CHECK_TRUE(wait_for_property_state(NEXDOME_MOVE_THRESHOLD_PROPERTY_NAME, INDIGO_OK_STATE));

	double far_az = bounded_dome_number_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 90);
	SERIAL_CHECK_TRUE(!isnan(far_az));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexdome3_case.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, far_az));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome3_case.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nexdome3_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nexdome3_passes_serial_compliance_checks", nexdome3_passes_serial_compliance_checks },
	};
	return indigo_run_tests("NexDome 3 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
