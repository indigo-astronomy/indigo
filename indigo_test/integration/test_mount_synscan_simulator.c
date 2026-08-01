// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_synscan/indigo_mount_synscan.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE
#define MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE "build/integration/mount_synscan_simulator"
#endif

#define MOUNT_POLARSCOPE_PROPERTY_NAME             "POLARSCOPE"
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME      "BRIGHTNESS"
#define MOUNT_USE_ENCODERS_PROPERTY_NAME           "MOUNT_USE_ENCODERS"
#define MOUNT_USE_RA_ENCODER_ITEM_NAME             "RA"
#define MOUNT_USE_DEC_ENCODER_ITEM_NAME            "DEC"
#define MOUNT_AUTOHOME_PROPERTY_NAME               "MOUNT_AUTOHOME"
#define MOUNT_AUTOHOME_ITEM_NAME                   "AUTOHOME"
#define MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME      "MOUNT_AUTOHOME_SETTINGS"
#define MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME        "DEC_OFFSET"

// The driver exposes a mount device and a guider device that share one
// connection (the guider reuses the mount/master connection). Each logical
// device is exercised in its own driver lifecycle; the guider is connected
// with the master (mount) device's DEVICE_PORT pointed at the simulator.
static const simulator_driver_case synscan_mount = {
	"SynScan EQ8 Mount",
	"indigo_mount_synscan",
	MOUNT_SYNSCAN_NAME,
	indigo_mount_synscan,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case synscan_guider = {
	"SynScan EQ8 Mount (guider)",
	"indigo_mount_synscan",
	MOUNT_SYNSCAN_GUIDER_NAME,
	indigo_mount_synscan,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void assert_serial_mount_class_property_completeness(void) {
	static const char *properties[] = {
		MOUNT_INFO_PROPERTY_NAME,
		GEOGRAPHIC_COORDINATES_PROPERTY_NAME,
		MOUNT_SLEW_RATE_PROPERTY_NAME,
		MOUNT_MOTION_DEC_PROPERTY_NAME,
		MOUNT_MOTION_RA_PROPERTY_NAME,
		MOUNT_TRACK_RATE_PROPERTY_NAME,
		MOUNT_TRACKING_PROPERTY_NAME,
		MOUNT_GUIDE_RATE_PROPERTY_NAME,
		MOUNT_ON_COORDINATES_SET_PROPERTY_NAME,
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME,
		MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME,
		MOUNT_ABORT_MOTION_PROPERTY_NAME,
		MOUNT_PEC_PROPERTY_NAME,
		MOUNT_PEC_TRAINING_PROPERTY_NAME,
		MOUNT_POLARSCOPE_PROPERTY_NAME,
		MOUNT_USE_ENCODERS_PROPERTY_NAME,
		MOUNT_AUTOHOME_PROPERTY_NAME,
		MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME
	};
	assert_defined_properties(properties, ARRAY_SIZE(properties));
}

static void synscan_mount_passes_serial_compliance_checks(void) {
	static const char *mount_info_items[] = {
		MOUNT_INFO_MODEL_ITEM_NAME,
		MOUNT_INFO_VENDOR_ITEM_NAME,
		MOUNT_INFO_FIRMWARE_ITEM_NAME
	};
	static const char *tracking_items[] = {
		MOUNT_TRACKING_ON_ITEM_NAME,
		MOUNT_TRACKING_OFF_ITEM_NAME
	};
	static const char *guide_rate_items[] = {
		MOUNT_GUIDE_RATE_RA_ITEM_NAME,
		MOUNT_GUIDE_RATE_DEC_ITEM_NAME
	};
	static const char *equatorial_items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	static const char *polarscope_items[] = {
		MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME
	};
	static const char *encoder_items[] = {
		MOUNT_USE_RA_ENCODER_ITEM_NAME,
		MOUNT_USE_DEC_ENCODER_ITEM_NAME
	};
	static const char *autohome_items[] = {
		MOUNT_AUTOHOME_ITEM_NAME
	};
	static const char *autohome_settings_items[] = {
		MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME
	};
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_serial_mount_class_property_completeness();
	assert_property_has_items(MOUNT_INFO_PROPERTY_NAME, mount_info_items, ARRAY_SIZE(mount_info_items));
	assert_property_has_items(MOUNT_TRACKING_PROPERTY_NAME, tracking_items, ARRAY_SIZE(tracking_items));
	assert_property_has_items(MOUNT_GUIDE_RATE_PROPERTY_NAME, guide_rate_items, ARRAY_SIZE(guide_rate_items));
	assert_property_has_items(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, equatorial_items, ARRAY_SIZE(equatorial_items));
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);
	assert_property_has_items(MOUNT_POLARSCOPE_PROPERTY_NAME, polarscope_items, ARRAY_SIZE(polarscope_items));
	assert_property_has_items(MOUNT_USE_ENCODERS_PROPERTY_NAME, encoder_items, ARRAY_SIZE(encoder_items));
	assert_property_has_items(MOUNT_AUTOHOME_PROPERTY_NAME, autohome_items, ARRAY_SIZE(autohome_items));
	assert_property_has_items(MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME, autohome_settings_items, ARRAY_SIZE(autohome_settings_items));
	assert_not_defined_property(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME);

	double guide_rate = bounded_number_value(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, 50);
	SERIAL_CHECK_TRUE(!isnan(guide_rate));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, guide_rate));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_GUIDE_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_GUIDE_RATE_RA_ITEM_NAME, guide_rate, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_POLARSCOPE_PROPERTY_NAME, MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME, 64));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_POLARSCOPE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(MOUNT_POLARSCOPE_PROPERTY_NAME, MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME, 64, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_USE_ENCODERS_PROPERTY_NAME, MOUNT_USE_RA_ENCODER_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_USE_ENCODERS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_guider_passes_serial_compliance_checks(void) {
	static const char *guide_pulse_dec_items[] = {
		GUIDER_GUIDE_NORTH_ITEM_NAME,
		GUIDER_GUIDE_SOUTH_ITEM_NAME
	};
	static const char *guide_pulse_ra_items[] = {
		GUIDER_GUIDE_EAST_ITEM_NAME,
		GUIDER_GUIDE_WEST_ITEM_NAME
	};
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&synscan_guider, synscan_mount.device_name, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_items(GUIDER_GUIDE_DEC_PROPERTY_NAME, guide_pulse_dec_items, ARRAY_SIZE(guide_pulse_dec_items));
	assert_property_has_items(GUIDER_GUIDE_RA_PROPERTY_NAME, guide_pulse_ra_items, ARRAY_SIZE(guide_pulse_ra_items));
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "synscan_mount_passes_serial_compliance_checks", synscan_mount_passes_serial_compliance_checks },
		{ "synscan_guider_passes_serial_compliance_checks", synscan_guider_passes_serial_compliance_checks }
	};
	return indigo_run_tests("SynScan EQ8 serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
