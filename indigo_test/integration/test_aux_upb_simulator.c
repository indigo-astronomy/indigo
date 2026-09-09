// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <indigo_drivers/aux_upb/indigo_aux_upb.h>
#include "serial_simulator_test_common.h"

static const simulator_driver_case aux = { "Ultimate Powerbox", "indigo_aux_upb", "Ultimate Powerbox", indigo_aux_upb, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case focuser = { "Ultimate Powerbox focuser", "indigo_aux_upb", "Ultimate Powerbox (focuser)", indigo_aux_upb, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static void powerbox_controls(void) {
	external_serial_simulator simulator = { 0 };
	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, "build/integration/aux_upb_simulator"));
	SERIAL_CHECK_TRUE(start_serial_driver(&aux, simulator.port));
	assert_device_interface(INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER);
	SERIAL_CHECK_EQ_INT(4, find_cached_property(AUX_POWER_OUTLET_PROPERTY_NAME)->count);
	SERIAL_CHECK_EQ_INT(3, find_cached_property(AUX_HEATER_OUTLET_PROPERTY_NAME)->count);
	SERIAL_CHECK_EQ_INT(6, find_cached_property(AUX_USB_PORT_PROPERTY_NAME)->count);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME, 23.2, 0.01));
	for (int i = 1; i <= 3; i++) {
		char item[32];
		snprintf(item, sizeof(item), "OUTLET_%d", i);
		indigo_change_number_property_1(&simulator_test_client, aux.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, item, 25 * i);
		SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(wait_for_number_item_value(AUX_HEATER_OUTLET_PROPERTY_NAME, item, 25 * i, 1));
	}
	indigo_change_switch_property_1(&simulator_test_client, aux.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_BUSY, indigo_aux_upb(INDIGO_DRIVER_SHUTDOWN, NULL));
	disconnect_serial_device(&aux);
	SERIAL_CHECK_TRUE(connect_serial_device(&aux, NULL));
cleanup:
	stop_serial_driver(&aux);
	stop_external_serial_simulator(&simulator);
}

static void focuser_motion(void) {
	external_serial_simulator simulator = { 0 };
	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, "build/integration/aux_upb_simulator"));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&focuser, aux.device_name, simulator.port));
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50, 0));
	indigo_change_number_property_1(&simulator_test_client, focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.value < 500);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, 0));
	indigo_change_number_property_1(&simulator_test_client, focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 2000);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	indigo_change_switch_property_1(&simulator_test_client, focuser.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(FOCUSER_POSITION_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.value < 2000);
	disconnect_serial_device(&focuser);
	SERIAL_CHECK_TRUE(connect_serial_device(&focuser, NULL));
cleanup:
	stop_serial_driver(&focuser);
	stop_external_serial_simulator(&simulator);
}

static void failed_identity(void) {
	external_serial_simulator simulator = { 0 };
	const char *arguments[] = { "--bad-response", "P#", NULL };
	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, "build/integration/aux_upb_simulator", arguments));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&aux));
	SERIAL_CHECK_TRUE(!connect_serial_device(&aux, simulator.port));
	SERIAL_CHECK_EQ_INT(INDIGO_ALERT_STATE, context.last_connection_state);
cleanup:
	stop_serial_driver(&aux);
	stop_external_serial_simulator(&simulator);
}

static void failed_focuser_status(void) {
	external_serial_simulator simulator = { 0 };
	const char *arguments[] = { "--bad-response", "SA", NULL };
	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, "build/integration/aux_upb_simulator", arguments));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&focuser));
	indigo_change_text_property_1_raw(&simulator_test_client, aux.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, simulator.port);
	SERIAL_CHECK_TRUE(!connect_serial_device(&focuser, NULL));
	SERIAL_CHECK_EQ_INT(INDIGO_ALERT_STATE, context.last_connection_state);
cleanup:
	stop_serial_driver(&focuser);
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	alarm(40);
	const indigo_test_case tests[] = { { "UPB2 power, weather and heater controls", powerbox_controls }, { "Shared focuser measured movement, abort and reconnect", focuser_motion }, { "Unknown identity rollback", failed_identity }, { "Malformed shared focuser initialization", failed_focuser_status } };
	return indigo_run_tests("Ultimate Powerbox protocol", tests, ARRAY_SIZE(tests));
}
