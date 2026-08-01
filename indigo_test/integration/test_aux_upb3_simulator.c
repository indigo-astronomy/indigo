// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_upb3/indigo_aux_upb3.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_UPB3_SIMULATOR_EXECUTABLE
#define AUX_UPB3_SIMULATOR_EXECUTABLE "build/integration/aux_upb3_simulator"
#endif

// The driver exposes an AUX device and a focuser device that share one
// connection (the focuser reuses the AUX/master connection). Each logical
// device is exercised in its own driver lifecycle; the focuser is connected
// with the master (AUX) device's DEVICE_PORT pointed at the simulator.
static const simulator_driver_case upb3_aux = {
	"PegasusAstro Ultimate Powerbox v3",
	"indigo_aux_upb3",
	"Ultimate Powerbox 3",
	indigo_aux_upb3,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case upb3_focuser = {
	"PegasusAstro Ultimate Powerbox v3 (focuser)",
	"indigo_aux_upb3",
	"Ultimate Powerbox 3 (focuser)",
	indigo_aux_upb3,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void upb3_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_UPB3_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&upb3_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, upb3_aux.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, upb3_aux.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, upb3_aux.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, upb3_aux.device_name, AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_USB_PORT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, upb3_aux.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&upb3_aux);
	}
	stop_external_serial_simulator(&simulator);
}

static void upb3_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_UPB3_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&upb3_focuser, upb3_aux.device_name, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);

	double target = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 5000);
	SERIAL_CHECK_TRUE(!isnan(target));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, upb3_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(FOCUSER_POSITION_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&upb3_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "upb3_aux_passes_serial_compliance_checks", upb3_aux_passes_serial_compliance_checks },
		{ "upb3_focuser_passes_serial_compliance_checks", upb3_focuser_passes_serial_compliance_checks }
	};
	return indigo_run_tests("UPB3 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
