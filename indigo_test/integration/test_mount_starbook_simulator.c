// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_starbook/indigo_mount_starbook.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_STARBOOK_SIMULATOR_EXECUTABLE
#define MOUNT_STARBOOK_SIMULATOR_EXECUTABLE "build/integration/mount_starbook_simulator"
#endif

#define STARBOOK_TIMEZONE_PROPERTY_NAME "STARBOOK_TIMEZONE"
#define STARBOOK_TIMEZONE_VALUE_ITEM_NAME "VALUE"
#define STARBOOK_RESET_PROPERTY_NAME "STARBOOK_RESET"
#define STARBOOK_RESET_ITEM_NAME "RESET"

static const simulator_driver_case starbook_mount = {
	"Vixen StarBook Mount",
	"indigo_mount_starbook",
	MOUNT_STARBOOK_NAME,
	indigo_mount_starbook,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case starbook_guider = {
	"Vixen StarBook Mount (guider)",
	"indigo_mount_starbook",
	MOUNT_STARBOOK_GUIDER_NAME,
	indigo_mount_starbook,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void starbook_mount_passes_http_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_STARBOOK_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&starbook_mount, simulator.port));
	driver_started = true;
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);
	assert_property_has_item(MOUNT_SIDE_OF_PIER_PROPERTY_NAME, MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME);
	assert_property_has_item(MOUNT_SIDE_OF_PIER_PROPERTY_NAME, MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME);
	assert_property_has_item(MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME);
	assert_property_has_item(STARBOOK_TIMEZONE_PROPERTY_NAME, STARBOOK_TIMEZONE_VALUE_ITEM_NAME);
	assert_property_has_item(STARBOOK_RESET_PROPERTY_NAME, STARBOOK_RESET_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, starbook_mount.device_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_SLEW_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, starbook_mount.device_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, starbook_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, starbook_mount.device_name, STARBOOK_TIMEZONE_PROPERTY_NAME, STARBOOK_TIMEZONE_VALUE_ITEM_NAME, 1));
	SERIAL_CHECK_TRUE(wait_for_property_state(STARBOOK_TIMEZONE_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (driver_started) {
		stop_serial_driver(&starbook_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void starbook_guider_passes_http_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_STARBOOK_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&starbook_guider));
	driver_started = true;
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, starbook_mount.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, simulator.port));
	indigo_usleep(100000);
	SERIAL_CHECK_TRUE(connect_serial_device(&starbook_guider, NULL));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, starbook_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, starbook_guider.device_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_RA_PROPERTY_NAME));

cleanup:
	if (driver_started) {
		disconnect_serial_device(&starbook_guider);
		disconnect_serial_device(&starbook_mount);
		tear_down_serial_driver(&starbook_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "starbook_mount_passes_http_compliance_checks", starbook_mount_passes_http_compliance_checks },
		{ "starbook_guider_passes_http_compliance_checks", starbook_guider_passes_http_compliance_checks }
	};
	return indigo_run_tests("Vixen StarBook mount HTTP simulator integration tests", tests, ARRAY_SIZE(tests));
}
