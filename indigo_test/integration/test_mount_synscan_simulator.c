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
	"Mount SynScan",
	indigo_mount_synscan,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case synscan_guider = {
	"SynScan EQ8 Mount (guider)",
	"indigo_mount_synscan",
	"Mount SynScan (guider)",
	indigo_mount_synscan,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case synscan_aux = {
	"SynScan EQ8 Mount (aux)",
	"indigo_mount_synscan",
	"Mount SynScan (aux)",
	indigo_mount_synscan,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static bool start_udp_simulator(external_serial_simulator *simulator, int port) {
	char port_text[16];
	const char *arguments[] = { "--udp-port", port_text, NULL };
	snprintf(port_text, sizeof(port_text), "%d", port);
	return start_external_serial_simulator_with_args(simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE, arguments);
}

static bool start_model_simulator(external_serial_simulator *simulator, const char *model_code) {
	const char *arguments[] = { "--model-code", model_code, NULL };
	return start_external_serial_simulator_with_args(simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE, arguments);
}

static bool start_feature_simulator(external_serial_simulator *simulator, const char *ra_features, const char *dec_features) {
	const char *arguments[] = { "--ra-features", ra_features, "--dec-features", dec_features, NULL };
	return start_external_serial_simulator_with_args(simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE, arguments);
}

static bool wait_for_text_item_value(const char *property_name, const char *item_name, const char *value) {
	for (int i = 0; i < 100; i++) {
		indigo_item *item = find_cached_item(property_name, item_name);
		if (item != NULL && !strcmp(item->text.value, value)) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool wait_for_property_state_long(const char *property_name, indigo_property_state state) {
	for (int i = 0; i < 300; i++) {
		indigo_property *property = find_cached_property(property_name);
		if (property != NULL && property->state == state) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool wait_for_switch_item_value(const char *property_name, const char *item_name, bool value) {
	for (int i = 0; i < 100; i++) {
		indigo_item *item = find_cached_item(property_name, item_name);
		if (item != NULL && item->sw.value == value) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool wait_for_light_item_value(const char *property_name, const char *item_name, indigo_property_state value) {
	for (int i = 0; i < 100; i++) {
		indigo_item *item = find_cached_item(property_name, item_name);
		if (item != NULL && item->light.value == value) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool wait_for_coordinate_value_change(double initial_ra, double initial_dec, double tolerance) {
	for (int i = 0; i < 100; i++) {
		indigo_item *ra_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
		indigo_item *dec_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
		if (ra_item != NULL && dec_item != NULL && (fabs(ra_item->number.value - initial_ra) > tolerance || fabs(dec_item->number.value - initial_dec) > tolerance)) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

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
		MOUNT_STATE_PROPERTY_NAME,
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
	static const char *motion_ra_items[] = {
		MOUNT_MOTION_WEST_ITEM_NAME,
		MOUNT_MOTION_EAST_ITEM_NAME
	};
	static const char *motion_dec_items[] = {
		MOUNT_MOTION_NORTH_ITEM_NAME,
		MOUNT_MOTION_SOUTH_ITEM_NAME
	};
	static const char *park_items[] = {
		MOUNT_PARK_PARKED_ITEM_NAME,
		MOUNT_PARK_UNPARKED_ITEM_NAME
	};
	static const char *home_items[] = {
		MOUNT_HOME_ITEM_NAME
	};
	static const char *state_items[] = {
		MOUNT_STATE_SLEW_ITEM_NAME,
		MOUNT_STATE_PARK_ITEM_NAME,
		MOUNT_STATE_HOME_ITEM_NAME,
		MOUNT_STATE_TRACKING_ITEM_NAME
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
	assert_property_has_items(MOUNT_MOTION_RA_PROPERTY_NAME, motion_ra_items, ARRAY_SIZE(motion_ra_items));
	assert_property_has_items(MOUNT_MOTION_DEC_PROPERTY_NAME, motion_dec_items, ARRAY_SIZE(motion_dec_items));
	assert_property_has_items(MOUNT_PARK_PROPERTY_NAME, park_items, ARRAY_SIZE(park_items));
	assert_property_has_items(MOUNT_HOME_PROPERTY_NAME, home_items, ARRAY_SIZE(home_items));
	assert_property_has_items(MOUNT_STATE_PROPERTY_NAME, state_items, ARRAY_SIZE(state_items));
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

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACK_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, false));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_parks_after_axis_status_initialized_reply(void) {
	const char *park_position_items[] = {
		MOUNT_PARK_POSITION_HA_ITEM_NAME,
		MOUNT_PARK_POSITION_DEC_ITEM_NAME
	};
	double park_position_values[] = {
		0,
		0
	};
	external_serial_simulator simulator = { 0 };
	char old_home[PATH_MAX] = { 0 };
	const char *old_home_value = getenv("HOME");
	bool home_changed = false;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	if (old_home_value != NULL) {
		snprintf(old_home, sizeof(old_home), "%s", old_home_value);
	}
	SERIAL_CHECK_TRUE(setenv("HOME", simulator.directory, 1) == 0);
	home_changed = true;
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, synscan_mount.device_name, MOUNT_PARK_POSITION_PROPERTY_NAME, ARRAY_SIZE(park_position_items), park_position_items, park_position_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_PARK_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state_long(MOUNT_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	if (home_changed) {
		if (*old_home) {
			setenv("HOME", old_home, 1);
		} else {
			unsetenv("HOME");
		}
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_hides_autohome_without_both_home_indexers(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_feature_simulator(&simulator, "6007", "6003"));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_not_defined_property(MOUNT_AUTOHOME_PROPERTY_NAME);
	assert_not_defined_property(MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME);

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_uses_aux_encoders_for_coordinates(void) {
	const char *encoder_items[] = {
		MOUNT_USE_RA_ENCODER_ITEM_NAME,
		MOUNT_USE_DEC_ENCODER_ITEM_NAME
	};
	bool encoder_values[] = {
		true,
		true
	};
	external_serial_simulator simulator = { 0 };
	double initial_ra = 0;
	double initial_dec = 0;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	indigo_item *ra_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	indigo_item *dec_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	SERIAL_CHECK_TRUE(ra_item != NULL && dec_item != NULL);
	initial_ra = ra_item->number.value;
	initial_dec = dec_item->number.value;

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property(&simulator_test_client, synscan_mount.device_name, MOUNT_USE_ENCODERS_PROPERTY_NAME, ARRAY_SIZE(encoder_items), encoder_items, encoder_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_USE_ENCODERS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_coordinate_value_change(initial_ra, initial_dec, 0.001));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_tracks_after_coordinate_slew_when_requested(void) {
	const char *coordinate_items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	double coordinate_values[] = {
		1,
		30
	};
	external_serial_simulator simulator = { 0 };
	double initial_ra = 0;
	double initial_dec = 0;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	indigo_item *ra_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	indigo_item *dec_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	SERIAL_CHECK_TRUE(ra_item != NULL && dec_item != NULL);
	initial_ra = ra_item->number.value;
	initial_dec = dec_item->number.value;

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_switch_item_value(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME, true));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, synscan_mount.device_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, ARRAY_SIZE(coordinate_items), coordinate_items, coordinate_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_coordinate_value_change(initial_ra, initial_dec, 0.001));
	SERIAL_CHECK_TRUE(wait_for_property_state_long(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_switch_item_value(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_light_item_value(MOUNT_STATE_PROPERTY_NAME, MOUNT_STATE_TRACKING_ITEM_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_reports_home_state_after_home(void) {
	const char *home_position_items[] = {
		MOUNT_HOME_POSITION_HA_ITEM_NAME,
		MOUNT_HOME_POSITION_DEC_ITEM_NAME
	};
	double home_position_values[] = {
		0,
		0
	};
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, synscan_mount.device_name, MOUNT_HOME_POSITION_PROPERTY_NAME, ARRAY_SIZE(home_position_items), home_position_items, home_position_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_HOME_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_HOME_PROPERTY_NAME, MOUNT_HOME_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state_long(MOUNT_HOME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_light_item_value(MOUNT_STATE_PROPERTY_NAME, MOUNT_STATE_HOME_ITEM_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_stops_tracking_after_home(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_HOME_SET_PROPERTY_NAME, MOUNT_HOME_SET_CURRENT_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_HOME_SET_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_switch_item_value(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_HOME_PROPERTY_NAME, MOUNT_HOME_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state_long(MOUNT_HOME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_light_item_value(MOUNT_STATE_PROPERTY_NAME, MOUNT_STATE_HOME_ITEM_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_switch_item_value(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_light_item_value(MOUNT_STATE_PROPERTY_NAME, MOUNT_STATE_TRACKING_ITEM_NAME, INDIGO_IDLE_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_autohome_finds_home_index(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME, MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME, 0));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME, MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_mount.device_name, MOUNT_AUTOHOME_PROPERTY_NAME, MOUNT_AUTOHOME_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_AUTOHOME_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_light_item_value(MOUNT_STATE_PROPERTY_NAME, MOUNT_STATE_HOME_ITEM_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state_long(MOUNT_AUTOHOME_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_light_item_value(MOUNT_STATE_PROPERTY_NAME, MOUNT_STATE_HOME_ITEM_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));
	indigo_item *dec_item = find_cached_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	indigo_item *raw_dec_item = find_cached_item(MOUNT_RAW_COORDINATES_PROPERTY_NAME, MOUNT_RAW_COORDINATES_DEC_ITEM_NAME);
	SERIAL_CHECK_TRUE(dec_item != NULL);
	SERIAL_CHECK_TRUE(raw_dec_item != NULL);
	SERIAL_CHECK_TRUE(fabs(raw_dec_item->number.value - 90.0) < 0.1);

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_reports_new_model_codes(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_model_simulator(&simulator, "25"));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	SERIAL_CHECK_TRUE(wait_for_text_item_value(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME, "CQ350 Pro"));

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

	double guider_rate = bounded_number_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, 75);
	SERIAL_CHECK_TRUE(!isnan(guider_rate));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, guider_rate));
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, guider_rate, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_RA_PROPERTY_NAME, INDIGO_ALERT_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 200));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 400));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 400));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, 0, 0.001));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_guider);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_aux_passes_shutter_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device(&synscan_aux, synscan_mount.device_name, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX_SHUTTER);
	assert_property_has_item(CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME);
	assert_property_has_item(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_aux.device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 2));
	SERIAL_CHECK_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state_long(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, synscan_aux.device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, synscan_aux.device_name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(CCD_ABORT_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_aux);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_disconnects_after_serial_loss(void) {
	external_serial_simulator simulator = { 0 };
	bool driver_started = false;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_SYNSCAN_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	driver_started = true;
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	stop_external_serial_simulator(&simulator);
	SERIAL_CHECK_TRUE(wait_for_simulator_connection_state(false));

cleanup:
	if (driver_started) {
		if (context.connected) {
			stop_serial_driver(&synscan_mount);
		} else {
			tear_down_serial_driver(&synscan_mount);
		}
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_reports_failed_serial_connection(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&synscan_mount));
	SERIAL_CHECK_TRUE(!connect_serial_device(&synscan_mount, "/dev/indigo-missing-synscan-test"));
	SERIAL_CHECK_TRUE(!context.connected);
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(CONNECTION_PROPERTY_NAME));

cleanup:
	tear_down_serial_driver(&synscan_mount);
}

static void synscan_mount_connects_with_explicit_udp_url(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_udp_simulator(&simulator, 0));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_serial_mount_class_property_completeness();

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void synscan_mount_connects_with_udp_autodetection(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_udp_simulator(&simulator, 11880));
	SERIAL_CHECK_TRUE(start_serial_driver(&synscan_mount, "synscan://"));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_serial_mount_class_property_completeness();

cleanup:
	if (context.connected) {
		stop_serial_driver(&synscan_mount);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "synscan_mount_passes_serial_compliance_checks", synscan_mount_passes_serial_compliance_checks },
		{ "synscan_mount_parks_after_axis_status_initialized_reply", synscan_mount_parks_after_axis_status_initialized_reply },
		{ "synscan_mount_hides_autohome_without_both_home_indexers", synscan_mount_hides_autohome_without_both_home_indexers },
		{ "synscan_mount_uses_aux_encoders_for_coordinates", synscan_mount_uses_aux_encoders_for_coordinates },
		{ "synscan_mount_tracks_after_coordinate_slew_when_requested", synscan_mount_tracks_after_coordinate_slew_when_requested },
		{ "synscan_mount_reports_home_state_after_home", synscan_mount_reports_home_state_after_home },
		{ "synscan_mount_stops_tracking_after_home", synscan_mount_stops_tracking_after_home },
		{ "synscan_mount_autohome_finds_home_index", synscan_mount_autohome_finds_home_index },
		{ "synscan_mount_reports_new_model_codes", synscan_mount_reports_new_model_codes },
		{ "synscan_guider_passes_serial_compliance_checks", synscan_guider_passes_serial_compliance_checks },
		{ "synscan_aux_passes_shutter_compliance_checks", synscan_aux_passes_shutter_compliance_checks },
		{ "synscan_mount_disconnects_after_serial_loss", synscan_mount_disconnects_after_serial_loss },
		{ "synscan_mount_reports_failed_serial_connection", synscan_mount_reports_failed_serial_connection },
		{ "synscan_mount_connects_with_explicit_udp_url", synscan_mount_connects_with_explicit_udp_url },
		{ "synscan_mount_connects_with_udp_autodetection", synscan_mount_connects_with_udp_autodetection }
	};
	return indigo_run_tests("SynScan EQ8 serial/UDP simulator integration tests", tests, ARRAY_SIZE(tests));
}
