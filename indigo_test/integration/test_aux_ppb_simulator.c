// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_ppb/indigo_aux_ppb.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_PPB_SIMULATOR_EXECUTABLE
#define AUX_PPB_SIMULATOR_EXECUTABLE "build/integration/aux_ppb_simulator"
#endif

// Driver-local property name, not exported from the header
#define AUX_DSLR_POWER_PROPERTY_NAME "X_DSLR_POWER"

// All three variants expose the same device name; the model is selected by
// the --model flag passed to the simulator at startup.
static const simulator_driver_case ppb_case = {
	"PegasusAstro Pocket Powerbox",
	"indigo_aux_ppb",
	"Pocket Powerbox",
	indigo_aux_ppb,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case ppba_case = {
	"PegasusAstro Pocket Powerbox Advance",
	"indigo_aux_ppb",
	"Pocket Powerbox",
	indigo_aux_ppb,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case spb_case = {
	"PegasusAstro Smart Powerbox",
	"indigo_aux_ppb",
	"Pocket Powerbox",
	indigo_aux_ppb,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void ppb_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	const char * const model_args[] = { "--model", "ppb", NULL };

	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, AUX_PPB_SIMULATOR_EXECUTABLE, model_args));
	SERIAL_CHECK_TRUE(start_serial_driver(&ppb_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_VOLTAGE_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_CURRENT_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppb_case.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppb_case.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ppb_case.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppb_case.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&ppb_case);
	}
	stop_external_serial_simulator(&simulator);
}

static void ppba_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	const char * const model_args[] = { "--model", "ppba", NULL };

	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, AUX_PPB_SIMULATOR_EXECUTABLE, model_args));
	SERIAL_CHECK_TRUE(start_serial_driver(&ppba_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(AUX_DSLR_POWER_PROPERTY_NAME, "3");
	assert_property_has_item(AUX_DSLR_POWER_PROPERTY_NAME, "5");
	assert_property_has_item(AUX_DSLR_POWER_PROPERTY_NAME, "12");
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_VOLTAGE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppba_case.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppba_case.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppba_case.device_name, AUX_DSLR_POWER_PROPERTY_NAME, "8", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DSLR_POWER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ppba_case.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ppba_case.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&ppba_case);
	}
	stop_external_serial_simulator(&simulator);
}

static void spb_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	const char * const model_args[] = { "--model", "spb", NULL };

	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, AUX_PPB_SIMULATOR_EXECUTABLE, model_args));
	SERIAL_CHECK_TRUE(start_serial_driver(&spb_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_INFO_PROPERTY_NAME, AUX_INFO_VOLTAGE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, spb_case.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, spb_case.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, spb_case.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 25));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, spb_case.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&spb_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "ppb_passes_serial_compliance_checks", ppb_passes_serial_compliance_checks },
		{ "ppba_passes_serial_compliance_checks", ppba_passes_serial_compliance_checks },
		{ "spb_passes_serial_compliance_checks", spb_passes_serial_compliance_checks },
	};
	return indigo_run_tests("PPB serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
