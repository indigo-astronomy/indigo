// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/wheel_manual/indigo_wheel_manual.h>
#include "serial_simulator_test_common.h"

static const simulator_driver_case manual = { "Manual filter wheel", "indigo_wheel_manual", "Manual filter wheel", indigo_wheel_manual, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static char last_message[INDIGO_VALUE_SIZE];

static indigo_result capture_message(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device && !strcmp(device->name, manual.device_name)) {
		snprintf(last_message, sizeof(last_message), "%s", message ? message : "");
	}
	return INDIGO_OK;
}

static void metadata(void) {
	assert_simulator_driver_info(&manual);
}

static void slots_limits_and_lifecycle(void) {
	simulator_test_client.send_message = capture_message;
	SERIAL_CHECK_TRUE(start_serial_driver(&manual, NULL));
	assert_device_interface(INDIGO_INTERFACE_WHEEL);
	indigo_property *names = find_cached_property(WHEEL_SLOT_NAME_PROPERTY_NAME);
	indigo_property *offsets = find_cached_property(WHEEL_SLOT_OFFSET_PROPERTY_NAME);
	SERIAL_CHECK_TRUE(names && offsets);
	SERIAL_CHECK_EQ_INT(8, names->count);
	SERIAL_CHECK_EQ_INT(8, offsets->count);
	SERIAL_CHECK_EQ_INT(8, find_cached_item(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME)->number.max);
	SERIAL_CHECK_EQ_INT(INDIGO_BUSY, indigo_wheel_manual(INDIGO_DRIVER_SHUTDOWN, NULL));
	for (int slot = 1; slot <= 8; slot++) {
		SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, manual.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, slot));
		SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, slot, 0));
		SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	}
	indigo_change_text_property_1_raw(&simulator_test_client, manual.device_name, WHEEL_SLOT_NAME_PROPERTY_NAME, WHEEL_SLOT_NAME_3_ITEM_NAME, "Hydrogen alpha");
	indigo_change_number_property_1(&simulator_test_client, manual.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3);
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(!strcmp(last_message, "Select filter 'Hydrogen alpha'"));
	disconnect_serial_device(&manual);
	SERIAL_CHECK_TRUE(connect_serial_device(&manual, NULL));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3, 0));
cleanup:
	stop_serial_driver(&manual);
}

int main(void) {
	const indigo_test_case tests[] = { { "metadata", metadata }, { "eight manual slots, named selection and reconnect", slots_limits_and_lifecycle } };
	return indigo_run_tests("Manual wheel", tests, ARRAY_SIZE(tests));
}
