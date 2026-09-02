// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_nexstaraux/indigo_mount_nexstaraux.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_NEXSTARAUX_SIMULATOR_EXECUTABLE
#define MOUNT_NEXSTARAUX_SIMULATOR_EXECUTABLE "build/integration/mount_nexstaraux_simulator"
#endif

static const simulator_driver_case nexstaraux_mount = {
	"NexStar AUX Mount",
	"indigo_mount_nexstaraux",
	"Mount Nexstar AUX",
	indigo_mount_nexstaraux,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case nexstaraux_guider = {
	"NexStar AUX Mount (guider)",
	"indigo_mount_nexstaraux",
	"Mount Nexstar AUX (guider)",
	indigo_mount_nexstaraux,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void nexstaraux_mount_passes_tcp_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_NEXSTARAUX_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&nexstaraux_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME);
	assert_property_has_item(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_DEC_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexstaraux_mount.device_name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexstaraux_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexstaraux_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nexstaraux_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void nexstaraux_guider_passes_tcp_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_NEXSTARAUX_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&nexstaraux_guider, &nexstaraux_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_DEC_RATE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexstaraux_guider.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexstaraux_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nexstaraux_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nexstaraux_mount_passes_tcp_compliance_checks", nexstaraux_mount_passes_tcp_compliance_checks },
		{ "nexstaraux_guider_passes_tcp_compliance_checks", nexstaraux_guider_passes_tcp_compliance_checks }
	};
	return indigo_run_tests("NexStar AUX mount TCP simulator integration tests", tests, ARRAY_SIZE(tests));
}
