// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/aux_wcv4ec/indigo_aux_wcv4ec.h>

#include "serial_simulator_test_common.h"

#ifndef AUX_WCV4EC_SIMULATOR_EXECUTABLE
#define AUX_WCV4EC_SIMULATOR_EXECUTABLE "build/integration/aux_wcv4ec_simulator"
#endif

#define AUX_DETECT_OPEN_CLOSE_PROPERTY_NAME "X_COVER_DETECT_OPEN_CLOSE"
#define AUX_SET_OPEN_CLOSE_PROPERTY_NAME    "X_COVER_SET_OPEN_CLOSE"
#define AUX_HEATER_PROPERTY_NAME            "X_HEATER"
#define AUX_HEATER_OFF_ITEM_NAME            "OFF"
#define AUX_HEATER_LOW_ITEM_NAME            "LOW"
#define AUX_HEATER_HIGH_ITEM_NAME           "HIGH"
#define AUX_HEATER_MAX_ITEM_NAME            "MAX"

static const simulator_driver_case wcv4ec_aux = {
	"WandererCover V4-EC Cover",
	"indigo_aux_wcv4ec",
	"WandererCover V4-EC",
	indigo_aux_wcv4ec,
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

static void wcv4ec_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AUX_WCV4EC_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&wcv4ec_aux, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_OFF_ITEM_NAME);
	assert_property_has_item(AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME);
	assert_property_has_item(AUX_DETECT_OPEN_CLOSE_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME);
	assert_property_has_item(AUX_DETECT_OPEN_CLOSE_PROPERTY_NAME, AUX_COVER_CLOSE_ITEM_NAME);
	assert_property_has_item(AUX_SET_OPEN_CLOSE_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME);
	assert_property_has_item(AUX_SET_OPEN_CLOSE_PROPERTY_NAME, AUX_COVER_CLOSE_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_PROPERTY_NAME, AUX_HEATER_OFF_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_PROPERTY_NAME, AUX_HEATER_LOW_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_PROPERTY_NAME, AUX_HEATER_HIGH_ITEM_NAME);
	assert_property_has_item(AUX_HEATER_PROPERTY_NAME, AUX_HEATER_MAX_ITEM_NAME);
	assert_property_has_item(AUX_COVER_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME);
	assert_property_has_item(AUX_COVER_PROPERTY_NAME, AUX_COVER_CLOSE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wcv4ec_aux.device_name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_LIGHT_INTENSITY_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_INTENSITY_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wcv4ec_aux.device_name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_LIGHT_SWITCH_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_LIGHT_SWITCH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wcv4ec_aux.device_name, AUX_HEATER_PROPERTY_NAME, AUX_HEATER_LOW_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, wcv4ec_aux.device_name, AUX_SET_OPEN_CLOSE_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME, 80));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_SET_OPEN_CLOSE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, wcv4ec_aux.device_name, AUX_COVER_PROPERTY_NAME, AUX_COVER_OPEN_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_COVER_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&wcv4ec_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "wcv4ec_aux_passes_serial_compliance_checks", wcv4ec_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("WandererCover V4-EC serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
