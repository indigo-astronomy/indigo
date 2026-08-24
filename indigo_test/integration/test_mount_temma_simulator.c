// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_temma/indigo_mount_temma.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_TEMMA_SIMULATOR_EXECUTABLE
#define MOUNT_TEMMA_SIMULATOR_EXECUTABLE "build/integration/mount_temma_simulator"
#endif

#define TEMMA_CORRECTION_SPEED_PROPERTY_NAME "TEMMA_CORRECTION_SPEED"
#define TEMMA_CORRECTION_SPEED_RA_ITEM_NAME "RA"
#define TEMMA_CORRECTION_SPEED_DEC_ITEM_NAME "DEC"
#define TEMMA_HIGH_SPEED_PROPERTY_NAME "TEMMA_HIGH_SPEED"
#define TEMMA_HIGH_SPEED_LOW_ITEM_NAME "LOW"
#define TEMMA_HIGH_SPEED_HIGH_ITEM_NAME "HIGH"
#define TEMMA_ZENITH_PROPERTY_NAME "TEMMA_ZENITH"
#define TEMMA_ZENITH_EAST_ITEM_NAME "EAST"
#define TEMMA_ZENITH_WEST_ITEM_NAME "WEST"

static const simulator_driver_case temma_mount = {
	"Takahashi Temma Mount",
	"indigo_mount_temma",
	MOUNT_TEMMA_NAME,
	indigo_mount_temma,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case temma_guider = {
	"Takahashi Temma Mount (guider)",
	"indigo_mount_temma",
	MOUNT_TEMMA_GUIDER_NAME,
	indigo_mount_temma,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void temma_mount_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_TEMMA_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&temma_mount, simulator.port));
	driver_started = true;
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(MOUNT_SIDE_OF_PIER_PROPERTY_NAME, MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME);
	assert_property_has_item(MOUNT_SIDE_OF_PIER_PROPERTY_NAME, MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME);
	assert_property_has_item(TEMMA_CORRECTION_SPEED_PROPERTY_NAME, TEMMA_CORRECTION_SPEED_RA_ITEM_NAME);
	assert_property_has_item(TEMMA_CORRECTION_SPEED_PROPERTY_NAME, TEMMA_CORRECTION_SPEED_DEC_ITEM_NAME);
	assert_property_has_item(TEMMA_HIGH_SPEED_PROPERTY_NAME, TEMMA_HIGH_SPEED_LOW_ITEM_NAME);
	assert_property_has_item(TEMMA_HIGH_SPEED_PROPERTY_NAME, TEMMA_HIGH_SPEED_HIGH_ITEM_NAME);
	assert_property_has_item(TEMMA_ZENITH_PROPERTY_NAME, TEMMA_ZENITH_EAST_ITEM_NAME);
	assert_property_has_item(TEMMA_ZENITH_PROPERTY_NAME, TEMMA_ZENITH_WEST_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, temma_mount.device_name, TEMMA_CORRECTION_SPEED_PROPERTY_NAME, TEMMA_CORRECTION_SPEED_RA_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(TEMMA_CORRECTION_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, temma_mount.device_name, TEMMA_HIGH_SPEED_PROPERTY_NAME, TEMMA_HIGH_SPEED_HIGH_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(TEMMA_HIGH_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, temma_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, temma_mount.device_name, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACK_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, temma_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (driver_started) {
		stop_serial_driver(&temma_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void temma_guider_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_TEMMA_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&temma_guider, &temma_mount, simulator.port));
	driver_started = true;
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, temma_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, temma_guider.device_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_RA_PROPERTY_NAME));

cleanup:
	if (driver_started) {
		stop_serial_driver(&temma_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "temma_mount_passes_serial_compliance_checks", temma_mount_passes_serial_compliance_checks },
		{ "temma_guider_passes_serial_compliance_checks", temma_guider_passes_serial_compliance_checks }
	};
	return indigo_run_tests("Takahashi Temma mount serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
