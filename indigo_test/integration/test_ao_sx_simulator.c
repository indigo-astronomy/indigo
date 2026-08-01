// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/ao_sx/indigo_ao_sx.h>

#include "serial_simulator_test_common.h"

#ifndef AO_SX_SIMULATOR_EXECUTABLE
#define AO_SX_SIMULATOR_EXECUTABLE "build/integration/ao_sx_simulator"
#endif

// The driver exposes an AO device and a guider device that share one
// connection (the guider reuses the AO/master connection). Each logical device
// is exercised in its own driver lifecycle; the guider is connected with the
// master (AO) device's DEVICE_PORT pointed at the simulator.
static const simulator_driver_case sx_ao = {
	"StarlightXpress AO",
	"indigo_ao_sx",
	"SX AO",
	indigo_ao_sx,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case sx_guider = {
	"StarlightXpress AO (guider)",
	"indigo_ao_sx",
	"SX AO (guider)",
	indigo_ao_sx,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void sx_ao_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AO_SX_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&sx_ao, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	// Character-framed X/V handshake, G tip/tilt pulses, K reset.
	assert_device_interface(INDIGO_INTERFACE_AO);
	assert_property_has_item(AO_GUIDE_DEC_PROPERTY_NAME, AO_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(AO_GUIDE_DEC_PROPERTY_NAME, AO_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(AO_GUIDE_RA_PROPERTY_NAME, AO_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(AO_GUIDE_RA_PROPERTY_NAME, AO_GUIDE_WEST_ITEM_NAME);
	assert_property_has_item(AO_RESET_PROPERTY_NAME, AO_CENTER_ITEM_NAME);
	assert_property_has_item(AO_RESET_PROPERTY_NAME, AO_UNJAM_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, sx_ao.device_name, AO_GUIDE_DEC_PROPERTY_NAME, AO_GUIDE_NORTH_ITEM_NAME, 10));
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, sx_ao.device_name, AO_GUIDE_RA_PROPERTY_NAME, AO_GUIDE_EAST_ITEM_NAME, 10));
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, sx_ao.device_name, AO_RESET_PROPERTY_NAME, AO_CENTER_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_RESET_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&sx_ao);
	}
	stop_external_serial_simulator(&simulator);
}

static void sx_guider_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, AO_SX_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&sx_guider, sx_ao.device_name, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);

	// Exercise the DEC pulse ("MN..."); the driver's RA handler reads the AO
	// device's guide items, so only enumeration is asserted for GUIDER_GUIDE_RA.
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, sx_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_DEC_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&sx_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "sx_ao_passes_serial_compliance_checks", sx_ao_passes_serial_compliance_checks },
		{ "sx_guider_passes_serial_compliance_checks", sx_guider_passes_serial_compliance_checks }
	};
	return indigo_run_tests("StarlightXpress AO serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
