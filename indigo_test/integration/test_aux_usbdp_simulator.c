// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_usbdp/indigo_aux_usbdp.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_USBDP_SIMULATOR_EXECUTABLE
#define AUX_USBDP_SIMULATOR_EXECUTABLE "build/integration/aux_usbdp_simulator"
#endif

// Driver-local names not exported from the header
#define AUX_CALLIBRATION_PROPERTY_NAME      "AUX_TEMPERATURE_CALLIBRATION"
#define AUX_CALLIBRATION_SENSOR_1_ITEM_NAME "SENSOR_1"
#define AUX_CALLIBRATION_SENSOR_2_ITEM_NAME "SENSOR_2"
#define AUX_CALLIBRATION_SENSOR_3_ITEM_NAME "SENSOR_3"

#define AUX_LINK_CH_2AND3_PROPERTY_NAME        "AUX_LINK_CHANNELS_2AND3"
#define AUX_LINK_CH_2AND3_LINKED_ITEM_NAME     "LINKED"
#define AUX_LINK_CH_2AND3_NOT_LINKED_ITEM_NAME "NOT_LINKED"

#define AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME "AUX_HEATER_AGGRESSIVITY"
#define AUX_HEATER_AGGRESSIVITY_1_ITEM_NAME   "AGGRESSIVITY_1"
#define AUX_HEATER_AGGRESSIVITY_2_ITEM_NAME   "AGGRESSIVITY_2"
#define AUX_HEATER_AGGRESSIVITY_5_ITEM_NAME   "AGGRESSIVITY_5"
#define AUX_HEATER_AGGRESSIVITY_10_ITEM_NAME  "AGGRESSIVITY_10"

static const simulator_driver_case usbdp_case = {
	"USB Dewpoint",
	"indigo_aux_usbdp",
	"USB Dewpoint",
	indigo_aux_usbdp,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void usbdp_v2_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	const char * const arguments[] = { "--model", "v2", NULL };

	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, AUX_USBDP_SIMULATOR_EXECUTABLE, arguments));
	SERIAL_CHECK_TRUE(start_serial_driver(&usbdp_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	// Driver fires aux_timer_callback immediately after connect; wait for first SGETAL poll.
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_WEATHER_PROPERTY_NAME, INDIGO_OK_STATE));

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();

	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_HUMIDITY_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_DEWPOINT_ITEM_NAME);

	assert_property_has_item(AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME);
	assert_property_has_item(AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME);

	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_2_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_3_ITEM_NAME);

	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_MANUAL_ITEM_NAME);
	assert_property_has_item(AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME);

	assert_property_has_item(AUX_CALLIBRATION_PROPERTY_NAME, AUX_CALLIBRATION_SENSOR_1_ITEM_NAME);
	assert_property_has_item(AUX_CALLIBRATION_PROPERTY_NAME, AUX_CALLIBRATION_SENSOR_2_ITEM_NAME);
	assert_property_has_item(AUX_CALLIBRATION_PROPERTY_NAME, AUX_CALLIBRATION_SENSOR_3_ITEM_NAME);

	assert_property_has_item(AUX_DEW_THRESHOLD_PROPERTY_NAME, AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME);
	assert_property_has_item(AUX_DEW_THRESHOLD_PROPERTY_NAME, AUX_DEW_THRESHOLD_SENSOR_2_ITEM_NAME);

	assert_property_has_item(AUX_LINK_CH_2AND3_PROPERTY_NAME, AUX_LINK_CH_2AND3_LINKED_ITEM_NAME);
	assert_property_has_item(AUX_LINK_CH_2AND3_PROPERTY_NAME, AUX_LINK_CH_2AND3_NOT_LINKED_ITEM_NAME);

	assert_property_has_item(AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME, AUX_HEATER_AGGRESSIVITY_1_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME, AUX_HEATER_AGGRESSIVITY_2_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME, AUX_HEATER_AGGRESSIVITY_5_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME, AUX_HEATER_AGGRESSIVITY_10_ITEM_NAME);

	// Exercise: set heater 1 to 50%
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, usbdp_case.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_HEATER_OUTLET_1_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));

	// Exercise: enable automatic dew control
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, usbdp_case.device_name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_DEW_CONTROL_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&usbdp_case);
	}
	stop_external_serial_simulator(&simulator);
}

static void usbdp_v1_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	const char * const arguments[] = { "--model", "v1", NULL };

	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, AUX_USBDP_SIMULATOR_EXECUTABLE, arguments));
	SERIAL_CHECK_TRUE(start_serial_driver(&usbdp_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	// Driver fires aux_timer_callback immediately after connect; wait for first SGETAL poll.
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_WEATHER_PROPERTY_NAME, INDIGO_OK_STATE));

	assert_device_interface(INDIGO_INTERFACE_AUX);

	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_TEMPERATURE_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_HUMIDITY_ITEM_NAME);
	assert_property_has_item(AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_DEWPOINT_ITEM_NAME);

	assert_property_has_item(AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME);

cleanup:
	if (context.connected) {
		stop_serial_driver(&usbdp_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "usbdp_v2_passes_serial_compliance_checks", usbdp_v2_passes_serial_compliance_checks },
		{ "usbdp_v1_passes_serial_compliance_checks", usbdp_v1_passes_serial_compliance_checks },
	};
	return indigo_run_tests("USB Dewpoint serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
