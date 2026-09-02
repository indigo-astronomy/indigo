// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_nexstar/indigo_mount_nexstar.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_NEXSTAR_SIMULATOR_EXECUTABLE
#define MOUNT_NEXSTAR_SIMULATOR_EXECUTABLE "build/integration/mount_nexstar_simulator"
#endif

#define COMMAND_GUIDE_RATE_PROPERTY_NAME "COMMAND_GUIDE_RATE"
#define GUIDE_50_ITEM_NAME               "GUIDE_50"
#define GUIDE_100_ITEM_NAME              "GUIDE_100"

static const simulator_driver_case nexstar_mount = {
	"NexStar Mount",
	"indigo_mount_nexstar",
	MOUNT_NEXSTAR_NAME,
	indigo_mount_nexstar,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case nexstar_guider = {
	"NexStar Mount (guider)",
	"indigo_mount_nexstar",
	MOUNT_NEXSTAR_GUIDER_NAME,
	indigo_mount_nexstar,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static bool start_nexstar_simulator(external_serial_simulator *simulator, const char *dialect) {
	const char *arguments[] = { "--dialect", dialect, NULL };
	return start_external_serial_simulator_with_args(simulator, MOUNT_NEXSTAR_SIMULATOR_EXECUTABLE, arguments);
}

static bool connect_nexstar_mount_with_dialect(const char *dialect, const char *expected_vendor) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;
	bool ok = false;

	SERIAL_CHECK_TRUE(start_nexstar_simulator(&simulator, dialect));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&nexstar_mount));
	driver_started = true;
	SERIAL_CHECK_TRUE(connect_serial_device(&nexstar_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	indigo_item *vendor = find_cached_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	SERIAL_CHECK_TRUE(vendor != NULL && !strcmp(vendor->text.value, expected_vendor));
	ok = true;

cleanup:
	if (context.connected) {
		disconnect_serial_device(&nexstar_mount);
	}
	context.connected = false;
	if (driver_started) {
		tear_down_serial_driver(&nexstar_mount);
	}
	stop_external_serial_simulator(&simulator);
	return ok;
}

static void nexstar_mount_connects_both_protocol_dialects(void) {
	SERIAL_CHECK_TRUE(connect_nexstar_mount_with_dialect("celestron", "Celestron"));
	SERIAL_CHECK_TRUE(connect_nexstar_mount_with_dialect("skywatcher", "Sky-Watcher"));

cleanup:
	return;
}

static void nexstar_celestron_mount_passes_serial_compliance_checks(void) {
	static const char *guide_rate_items[] = {
		MOUNT_GUIDE_RATE_RA_ITEM_NAME,
		MOUNT_GUIDE_RATE_DEC_ITEM_NAME
	};
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_nexstar_simulator(&simulator, "celestron"));
	SERIAL_CHECK_TRUE(start_serial_driver(&nexstar_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_items(MOUNT_GUIDE_RATE_PROPERTY_NAME, guide_rate_items, ARRAY_SIZE(guide_rate_items));
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexstar_mount.device_name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexstar_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexstar_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nexstar_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void nexstar_celestron_guider_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_nexstar_simulator(&simulator, "celestron"));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&nexstar_guider, &nexstar_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);
	assert_property_has_item(COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDE_50_ITEM_NAME);
	assert_property_has_item(COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDE_100_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, nexstar_guider.device_name, COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDE_100_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(COMMAND_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, nexstar_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&nexstar_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "nexstar_mount_connects_both_protocol_dialects", nexstar_mount_connects_both_protocol_dialects },
		{ "nexstar_celestron_mount_passes_serial_compliance_checks", nexstar_celestron_mount_passes_serial_compliance_checks },
		{ "nexstar_celestron_guider_passes_serial_compliance_checks", nexstar_celestron_guider_passes_serial_compliance_checks }
	};
	return indigo_run_tests("NexStar mount serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
