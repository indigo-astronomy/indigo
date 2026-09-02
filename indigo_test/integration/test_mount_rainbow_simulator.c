// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_rainbow/indigo_mount_rainbow.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_RAINBOW_SIMULATOR_EXECUTABLE
#define MOUNT_RAINBOW_SIMULATOR_EXECUTABLE "build/integration/mount_rainbow_simulator"
#endif

static const simulator_driver_case rainbow_mount = {
	"RainbowAstro Mount",
	"indigo_mount_rainbow",
	MOUNT_RAINBOW_NAME,
	indigo_mount_rainbow,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void rainbow_mount_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_RAINBOW_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&rainbow_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_LUNAR_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, rainbow_mount.device_name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rainbow_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rainbow_mount.device_name, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACK_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, rainbow_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&rainbow_mount);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "rainbow_mount_passes_serial_compliance_checks", rainbow_mount_passes_serial_compliance_checks }
	};
	return indigo_run_tests("RainbowAstro mount serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
