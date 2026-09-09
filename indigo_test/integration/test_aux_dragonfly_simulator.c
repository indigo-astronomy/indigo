// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <indigo_drivers/aux_dragonfly/indigo_aux_dragonfly.h>
#include "serial_simulator_test_common.h"
#include "aux_test_isolation.h"

static const simulator_driver_case primary = { "Dragonfly Controller", "indigo_aux_dragonfly", "Dragonfly Controller", indigo_aux_dragonfly, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static void normal(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&primary, aux_simulator.port));
	assert_device_interface(INDIGO_INTERFACE_AUX);
	for (int i = 0; i < 8; i++) {
		char item[32];
		snprintf(item, sizeof(item), "GPIO_SENSOR_NAME_%d", i + 1);
		SERIAL_CHECK_TRUE(wait_for_number_item_value(AUX_GPIO_SENSORS_PROPERTY_NAME, item, 11 + i, .01));
		snprintf(item, sizeof(item), "OUTLET_%d", i + 1);
		SERIAL_CHECK_EQ_INT(indigo_change_switch_property_1(&simulator_test_client, primary.device_name, AUX_GPIO_OUTLETS_PROPERTY_NAME, item, true), INDIGO_OK);
		SERIAL_CHECK_TRUE(aux_wait_switch(AUX_GPIO_OUTLETS_PROPERTY_NAME, item, true));
	}
	printf("Data and control assertions passed; checking disconnect/shutdown\n");
cleanup:
	aux_stop(&primary);
}

static void pulse(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&primary, aux_simulator.port));
	SERIAL_CHECK_EQ_INT(indigo_change_number_property_1(&simulator_test_client, primary.device_name, "AUX_OUTLET_PULSE_LENGTHS", "OUTLET_1", 500), INDIGO_OK);
	SERIAL_CHECK_TRUE(wait_for_number_item_value("AUX_OUTLET_PULSE_LENGTHS", "OUTLET_1", 500, .01));
	SERIAL_CHECK_EQ_INT(indigo_change_switch_property_1(&simulator_test_client, primary.device_name, AUX_GPIO_OUTLETS_PROPERTY_NAME, "OUTLET_1", true), INDIGO_OK);
	SERIAL_CHECK_TRUE(aux_wait_switch(AUX_GPIO_OUTLETS_PROPERTY_NAME, "OUTLET_1", true));
	SERIAL_CHECK_TRUE(aux_wait_switch(AUX_GPIO_OUTLETS_PROPERTY_NAME, "OUTLET_1", false));
cleanup:
	aux_stop(&primary);
}

static void rejected_connection(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&primary));
	SERIAL_CHECK_TRUE(aux_reject(&primary, aux_simulator.port));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(!context.connected);
cleanup:
	aux_stop(&primary);
}

static void relay_error(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&primary, aux_simulator.port));
	SERIAL_CHECK_EQ_INT(indigo_change_switch_property_1(&simulator_test_client, primary.device_name, AUX_GPIO_OUTLETS_PROPERTY_NAME, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, true), INDIGO_OK);
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_GPIO_OUTLETS_PROPERTY_NAME, INDIGO_ALERT_STATE));
cleanup:
	aux_stop(&primary);
}

int main(void) {
	const aux_simulated_case tests[] = {
		{ "normal", normal, "normal" },
		{ "pulse", pulse, "normal" },
		{ "wrong_identity", rejected_connection, "wrong-model" },
		{ "relay_error", relay_error, "relay-error" },
		{ "oversized_reply", rejected_connection, "oversized" },
	};
	return run_aux_simulated("dragonfly", "build/integration/aux_dragonfly_simulator", tests, ARRAY_SIZE(tests));
}
