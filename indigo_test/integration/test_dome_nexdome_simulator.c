// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/dome_nexdome/indigo_dome_nexdome.h>

#include "serial_simulator_test_common.h"

#ifndef DOME_NEXDOME_SIMULATOR_EXECUTABLE
#define DOME_NEXDOME_SIMULATOR_EXECUTABLE "build/integration/dome_nexdome_simulator"
#endif

#define NEXDOME_REVERSED_PROPERTY_NAME            "NEXDOME_REVERSED"
#define NEXDOME_REVERSED_YES_ITEM_NAME            "YES"
#define NEXDOME_REVERSED_NO_ITEM_NAME             "NO"
#define NEXDOME_RESET_SHUTTER_COMM_PROPERTY_NAME  "NEXDOME_RESET_SHUTTER_COMM"
#define NEXDOME_RESET_SHUTTER_COMM_ITEM_NAME      "RESET"
#define NEXDOME_FIND_HOME_PROPERTY_NAME           "NEXDOME_FIND_HOME"
#define NEXDOME_FIND_HOME_ITEM_NAME               "FIND_HOME"
#define NEXDOME_CALLIBRATE_PROPERTY_NAME          "NEXDOME_CALLIBRATE"
#define NEXDOME_CALLIBRATE_ITEM_NAME              "CALLIBRATE"
#define NEXDOME_POWER_PROPERTY_NAME               "NEXDOME_POWER"
#define NEXDOME_POWER_ROTATOR_ITEM_NAME           "ROTATOR_VOLTAGE"
#define NEXDOME_POWER_SHUTTER_ITEM_NAME           "SHUTTER_VOLTAGE"

static const simulator_driver_case nexdome_case = {
	DOME_NEXDOME_NAME,
	"indigo_dome_nexdome",
	DOME_NEXDOME_NAME,
	indigo_dome_nexdome,
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

static void nexdome_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	static const char *reversed_items[] = {
		NEXDOME_REVERSED_YES_ITEM_NAME,
		NEXDOME_REVERSED_NO_ITEM_NAME
	};

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, DOME_NEXDOME_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&nexdome_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_DOME);

	assert_property_has_item(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME);
	assert_property_has_item(DOME_STEPS_PROPERTY_NAME, DOME_STEPS_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME);
	assert_property_has_item(DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME);
	assert_property_has_item(DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME);
	assert_property_has_item(DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME);
	assert_property_has_items(NEXDOME_REVERSED_PROPERTY_NAME, reversed_items, ARRAY_SIZE(reversed_items));
	assert_property_has_item(NEXDOME_RESET_SHUTTER_COMM_PROPERTY_NAME, NEXDOME_RESET_SHUTTER_COMM_ITEM_NAME);
	assert_property_has_item(NEXDOME_FIND_HOME_PROPERTY_NAME, NEXDOME_FIND_HOME_ITEM_NAME);
	assert_property_has_item(NEXDOME_CALLIBRATE_PROPERTY_NAME, NEXDOME_CALLIBRATE_ITEM_NAME);
	assert_property_has_item(NEXDOME_POWER_PROPERTY_NAME, NEXDOME_POWER_ROTATOR_ITEM_NAME);
	assert_property_has_item(NEXDOME_POWER_PROPERTY_NAME, NEXDOME_POWER_SHUTTER_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome_case.device_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome_case.device_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_SHUTTER_PROPERTY_NAME, INDIGO_OK_STATE));

	double target_az = bounded_dome_number_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 25);
	SERIAL_CHECK_TRUE(!isnan(target_az));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexdome_case.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, target_az));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, target_az, 0.1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome_case.device_name, NEXDOME_REVERSED_PROPERTY_NAME, NEXDOME_REVERSED_YES_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(NEXDOME_REVERSED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome_case.device_name, NEXDOME_REVERSED_PROPERTY_NAME, NEXDOME_REVERSED_NO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(NEXDOME_REVERSED_PROPERTY_NAME, INDIGO_OK_STATE));

	double far_az = bounded_dome_number_value(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, 90);
	SERIAL_CHECK_TRUE(!isnan(far_az));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexdome_case.device_name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, far_az));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexdome_case.device_name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(DOME_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nexdome_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nexdome_passes_serial_compliance_checks", nexdome_passes_serial_compliance_checks },
	};
	return indigo_run_tests("NexDome serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
