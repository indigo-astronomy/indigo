// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_wbplusv3/indigo_aux_wbplusv3.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_WBPLUSV3_SIMULATOR_EXECUTABLE
#define AUX_WBPLUSV3_SIMULATOR_EXECUTABLE "build/integration/aux_wbplusv3_simulator"
#endif

#define X_AUX_CALIBRATE_PROPERTY_NAME "X_AUX_CALIBRATE"
#define X_AUX_CALIBRATE_ITEM_NAME     "CALIBRATE"

static const simulator_driver_case wbplusv3_aux = {
	"WandererBox Plus V3 Powerbox",
	"indigo_aux_wbplusv3",
	"WandererBox Plus V3",
	indigo_aux_wbplusv3,
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

static void wbplusv3_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_WBPLUSV3_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&wbplusv3_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, AUX_POWER_OUTLET_VOLTAGE_1_ITEM_NAME);
	assert_property_has_item(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_HUMIDITY_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_DEWPOINT_ITEM_NAME);
	assert_property_has_item(AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME);
	assert_property_has_item(AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME);
	assert_property_has_item(AUX_DEW_WARNING_PROPERTY_NAME, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_VOLTAGE_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_CURRENT_ITEM_NAME);
	assert_property_has_item(X_AUX_CALIBRATE_PROPERTY_NAME, X_AUX_CALIBRATE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wbplusv3_aux.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wbplusv3_aux.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wbplusv3_aux.device_name, AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, AUX_POWER_OUTLET_VOLTAGE_1_ITEM_NAME, 12.0));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wbplusv3_aux.device_name, AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_USB_PORT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wbplusv3_aux.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wbplusv3_aux.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wbplusv3_aux.device_name, X_AUX_CALIBRATE_PROPERTY_NAME, X_AUX_CALIBRATE_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(X_AUX_CALIBRATE_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&wbplusv3_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "wbplusv3_aux_passes_serial_compliance_checks", wbplusv3_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("WandererBox Plus V3 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
