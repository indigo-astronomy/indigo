// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/wheel_trutek/indigo_wheel_trutek.h>

#include "serial_simulator_test_common.h"

#ifndef WHEEL_TRUTEK_SIMULATOR_EXECUTABLE
#define WHEEL_TRUTEK_SIMULATOR_EXECUTABLE "build/integration/wheel_trutek_simulator"
#endif

static const simulator_driver_case trutek_wheel = {
	"Trutek Filter Wheel",
	"indigo_wheel_trutek",
	"Trutek Filter Wheel",
	indigo_wheel_trutek,
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

static void assert_serial_wheel_class_property_completeness(void) {
	static const char *properties[] = {
		WHEEL_SLOT_PROPERTY_NAME,
		WHEEL_SLOT_NAME_PROPERTY_NAME,
		WHEEL_SLOT_OFFSET_PROPERTY_NAME
	};
	assert_defined_properties(properties, ARRAY_SIZE(properties));
}

static void trutek_wheel_passes_serial_compliance_checks(void) {
	static const char *slot_name_items[] = {
		WHEEL_SLOT_NAME_1_ITEM_NAME,
		WHEEL_SLOT_NAME_2_ITEM_NAME,
		WHEEL_SLOT_NAME_3_ITEM_NAME,
		WHEEL_SLOT_NAME_4_ITEM_NAME,
		WHEEL_SLOT_NAME_5_ITEM_NAME
	};
	static const char *slot_offset_items[] = {
		WHEEL_SLOT_OFFSET_1_ITEM_NAME,
		WHEEL_SLOT_OFFSET_2_ITEM_NAME,
		WHEEL_SLOT_OFFSET_3_ITEM_NAME,
		WHEEL_SLOT_OFFSET_4_ITEM_NAME,
		WHEEL_SLOT_OFFSET_5_ITEM_NAME
	};
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, WHEEL_TRUTEK_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&trutek_wheel, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_WHEEL);
	assert_serial_wheel_class_property_completeness();
	assert_property_has_item(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);
	assert_property_has_items(WHEEL_SLOT_NAME_PROPERTY_NAME, slot_name_items, ARRAY_SIZE(slot_name_items));
	assert_property_has_items(WHEEL_SLOT_OFFSET_PROPERTY_NAME, slot_offset_items, ARRAY_SIZE(slot_offset_items));
	assert_number_item_in_range(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 1, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, trutek_wheel.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3));
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, trutek_wheel.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 5, 0.001));

cleanup:
	if (context.connected) {
		stop_serial_driver(&trutek_wheel);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "trutek_wheel_passes_serial_compliance_checks", trutek_wheel_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Trutek wheel serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
