// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_ioptron/indigo_mount_ioptron.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_IOPTRON_SIMULATOR_EXECUTABLE
#define MOUNT_IOPTRON_SIMULATOR_EXECUTABLE "build/integration/mount_ioptron_simulator"
#endif

#define MOUNT_PROTOCOL_PROPERTY_NAME              "PROTOCOL_VERSION"
#define MOUNT_MERIDIAN_HANDLING_PROPERTY_NAME     "MOUNT_MERIDIAN_HANDLING"
#define MOUNT_MERIDIAN_STOP_ITEM_NAME             "STOP"
#define MOUNT_MERIDIAN_FLIP_ITEM_NAME             "FLIP"
#define MOUNT_MERIDIAN_LIMIT_PROPERTY_NAME        "MOUNT_MERIDIAN_LIMIT"
#define MOUNT_MERIDIAN_LIMIT_ITEM_NAME            "LIMIT"

static const simulator_driver_case ioptron_mount = {
	"iOptron Mount",
	"indigo_mount_ioptron",
	"iOptron Mount",
	indigo_mount_ioptron,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case ioptron_guider = {
	"iOptron Mount (guider)",
	"indigo_mount_ioptron",
	"iOptron Mount (guider)",
	indigo_mount_ioptron,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static bool start_ioptron_simulator(external_serial_simulator *simulator, const char *protocol) {
	const char *arguments[] = { "--protocol", protocol, NULL };
	return start_external_serial_simulator_with_args(simulator, MOUNT_IOPTRON_SIMULATOR_EXECUTABLE, arguments);
}

static bool set_ioptron_port(const char *port) {
	if (indigo_change_text_property_1_raw(&simulator_test_client, ioptron_mount.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, port) != INDIGO_OK) {
		return false;
	}
	for (int i = 0; i < 100; i++) {
		indigo_item *item = find_cached_item(DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME);
		if (item != NULL && !strcmp(item->text.value, port)) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool connect_ioptron_mount_with_protocol(const char *simulator_protocol) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;
	bool ok = false;

	SERIAL_CHECK_TRUE(start_ioptron_simulator(&simulator, simulator_protocol));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&ioptron_mount));
	driver_started = true;
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(has_defined_property(DEVICE_PORT_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(set_ioptron_port(simulator.port));
	SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_mount, NULL));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);
	ok = true;

cleanup:
	if (context.connected) {
		disconnect_serial_device(&ioptron_mount);
	}
	context.connected = false;
	if (driver_started) {
		tear_down_serial_driver(&ioptron_mount);
	}
	stop_external_serial_simulator(&simulator);
	return ok;
}

static void ioptron_mount_connects_all_protocol_dialects(void) {
	SERIAL_CHECK_TRUE(connect_ioptron_mount_with_protocol("8406"));
	SERIAL_CHECK_TRUE(connect_ioptron_mount_with_protocol("8407"));
	SERIAL_CHECK_TRUE(connect_ioptron_mount_with_protocol("0100"));
	SERIAL_CHECK_TRUE(connect_ioptron_mount_with_protocol("0200"));
	SERIAL_CHECK_TRUE(connect_ioptron_mount_with_protocol("0205"));
	SERIAL_CHECK_TRUE(connect_ioptron_mount_with_protocol("0300"));

cleanup:
	return;
}

static void ioptron_protocol_3_mount_passes_serial_compliance_checks(void) {
	static const char *guide_rate_items[] = {
		MOUNT_GUIDE_RATE_RA_ITEM_NAME,
		MOUNT_GUIDE_RATE_DEC_ITEM_NAME
	};
	static const char *meridian_handling_items[] = {
		MOUNT_MERIDIAN_STOP_ITEM_NAME,
		MOUNT_MERIDIAN_FLIP_ITEM_NAME
	};
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;

	SERIAL_CHECK_TRUE(start_ioptron_simulator(&simulator, "0300"));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&ioptron_mount));
	driver_started = true;
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(has_defined_property(DEVICE_PORT_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(set_ioptron_port(simulator.port));
	SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_mount, NULL));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_items(MOUNT_GUIDE_RATE_PROPERTY_NAME, guide_rate_items, ARRAY_SIZE(guide_rate_items));
	assert_property_has_items(MOUNT_MERIDIAN_HANDLING_PROPERTY_NAME, meridian_handling_items, ARRAY_SIZE(meridian_handling_items));
	assert_property_has_item(MOUNT_MERIDIAN_LIMIT_PROPERTY_NAME, MOUNT_MERIDIAN_LIMIT_ITEM_NAME);
	assert_property_has_item(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ioptron_mount.device_name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 50, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ioptron_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ioptron_mount.device_name, MOUNT_MERIDIAN_HANDLING_PROPERTY_NAME, MOUNT_MERIDIAN_FLIP_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MERIDIAN_HANDLING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ioptron_mount.device_name, MOUNT_MERIDIAN_LIMIT_PROPERTY_NAME, MOUNT_MERIDIAN_LIMIT_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MERIDIAN_LIMIT_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ioptron_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		disconnect_serial_device(&ioptron_mount);
	}
	context.connected = false;
	if (driver_started) {
		tear_down_serial_driver(&ioptron_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void ioptron_guider_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_ioptron_simulator(&simulator, "0300"));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&ioptron_guider, &ioptron_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ioptron_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&ioptron_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "ioptron_mount_connects_all_protocol_dialects", ioptron_mount_connects_all_protocol_dialects },
		{ "ioptron_protocol_3_mount_passes_serial_compliance_checks", ioptron_protocol_3_mount_passes_serial_compliance_checks },
		{ "ioptron_guider_passes_serial_compliance_checks", ioptron_guider_passes_serial_compliance_checks }
	};
	return indigo_run_tests("iOptron mount serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
