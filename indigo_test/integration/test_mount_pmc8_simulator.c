// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_pmc8/indigo_mount_pmc8.h>

#include "serial_simulator_test_common.h"
#include "abort_queue_test_common.h"

#ifndef MOUNT_PMC8_SIMULATOR_EXECUTABLE
#define MOUNT_PMC8_SIMULATOR_EXECUTABLE "build/integration/mount_pmc8_simulator"
#endif

#define PMC8_CONNECTION_MODE_PROPERTY_NAME "CONNECTION_MODE"
#define PMC8_MOUNT_TYPE_PROPERTY_NAME "MOUNT_TYPE"
#define PMC8_MOUNT_DEVICE_NAME "Mount PMC Eight"
#define PMC8_GUIDER_DEVICE_NAME "Mount PMC Eight (guider)"

static void assert_text_item_value(const char *property_name, const char *item_name, const char *expected_value) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		fprintf(stderr, "Missing text item %s.%s on %s\n", property_name, item_name, context.driver_case->device_name);
	}
	ASSERT_TRUE(item != NULL);
	ASSERT_STREQ(expected_value, indigo_get_text_item_value(item));
}

static bool cached_switch_item_value(const char *property_name, const char *item_name, bool *value) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		fprintf(stderr, "Missing switch item %s.%s on %s\n", property_name, item_name, context.driver_case->device_name);
		return false;
	}
	*value = item->sw.value;
	return true;
}

static const simulator_driver_case pmc8_mount = {
	"PMC-Eight Mount",
	"indigo_mount_pmc8",
	PMC8_MOUNT_DEVICE_NAME,
	indigo_mount_pmc8,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case pmc8_guider = {
	"PMC-Eight Mount (guider)",
	"indigo_mount_pmc8",
	PMC8_GUIDER_DEVICE_NAME,
	indigo_mount_pmc8,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static bool start_pmc8_mount_in_serial_mode(external_serial_simulator *simulator) {
	if (!start_external_serial_simulator(simulator, MOUNT_PMC8_SIMULATOR_EXECUTABLE)) {
		return false;
	}
	if (!bring_up_serial_driver(&pmc8_mount)) {
		stop_external_serial_simulator(simulator);
		return false;
	}
	enumerate_simulator_device();
	if (indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_CONNECTION_MODE_PROPERTY_NAME, "SERIAL", true) != INDIGO_OK) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!wait_for_property_state(PMC8_CONNECTION_MODE_PROPERTY_NAME, INDIGO_OK_STATE)) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (indigo_change_text_property_1_raw(&simulator_test_client, pmc8_mount.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, simulator->port) != INDIGO_OK) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!wait_for_property_state(DEVICE_PORT_PROPERTY_NAME, INDIGO_OK_STATE)) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) != INDIGO_OK) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!wait_for_simulator_connection_state(true)) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	return true;
}

static bool start_pmc8_mount_with_network_mode(external_serial_simulator *simulator, const char *connection_mode) {
	const char *arguments[] = { "--network", NULL };
	if (!start_external_serial_simulator_with_args(simulator, MOUNT_PMC8_SIMULATOR_EXECUTABLE, arguments)) {
		return false;
	}
	const char *url = !strcmp(connection_mode, "TCP") ? simulator->tcp_url : simulator->udp_url;
	if (*url == '\0') {
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!bring_up_serial_driver(&pmc8_mount)) {
		stop_external_serial_simulator(simulator);
		return false;
	}
	enumerate_simulator_device();
	if (indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_CONNECTION_MODE_PROPERTY_NAME, connection_mode, true) != INDIGO_OK) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!wait_for_property_state(PMC8_CONNECTION_MODE_PROPERTY_NAME, INDIGO_OK_STATE)) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (indigo_change_text_property_1_raw(&simulator_test_client, pmc8_mount.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, url) != INDIGO_OK) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!wait_for_property_state(DEVICE_PORT_PROPERTY_NAME, INDIGO_OK_STATE)) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) != INDIGO_OK) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	if (!wait_for_simulator_connection_state(true)) {
		tear_down_serial_driver(&pmc8_mount);
		stop_external_serial_simulator(simulator);
		return false;
	}
	return true;
}

static void pmc8_mount_defines_custom_properties_while_disconnected(void) {
	bool driver_up = false;
	bool auto_selected = false;

	SERIAL_CHECK_TRUE(bring_up_serial_driver(&pmc8_mount));
	driver_up = true;
	enumerate_simulator_device();

	assert_property_has_item(PMC8_CONNECTION_MODE_PROPERTY_NAME, "UDP");
	assert_property_has_item(PMC8_CONNECTION_MODE_PROPERTY_NAME, "TCP");
	assert_property_has_item(PMC8_CONNECTION_MODE_PROPERTY_NAME, "SERIAL");
	assert_property_has_item(PMC8_CONNECTION_MODE_PROPERTY_NAME, "SERIAL_DTR");
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "AUTO");
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "G11");
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "TITAN");
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "EXOS-2");
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "iEXOS-100");
	SERIAL_CHECK_TRUE(cached_switch_item_value(PMC8_MOUNT_TYPE_PROPERTY_NAME, "AUTO", &auto_selected));
	SERIAL_CHECK_TRUE(auto_selected);

cleanup:
	if (driver_up) {
		tear_down_serial_driver(&pmc8_mount);
	}
}

static void pmc8_mount_accepts_disconnected_connection_mode_changes(void) {
	bool driver_up = false;

	SERIAL_CHECK_TRUE(bring_up_serial_driver(&pmc8_mount));
	driver_up = true;
	enumerate_simulator_device();

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_CONNECTION_MODE_PROPERTY_NAME, "TCP", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(PMC8_CONNECTION_MODE_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_CONNECTION_MODE_PROPERTY_NAME, "UDP", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(PMC8_CONNECTION_MODE_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (driver_up) {
		tear_down_serial_driver(&pmc8_mount);
	}
}

static void pmc8_mount_rejects_direct_serial_to_udp_change_while_connected(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_pmc8_mount_in_serial_mode(&simulator));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_CONNECTION_MODE_PROPERTY_NAME, "UDP", true));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(PMC8_CONNECTION_MODE_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(context.connected);

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_connects_over_tcp(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_pmc8_mount_with_network_mode(&simulator, "TCP"));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "EXOS-2");
	assert_text_item_value(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME, "Explore Scientific EXOS II");

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_connects_over_udp(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_pmc8_mount_with_network_mode(&simulator, "UDP"));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "EXOS-2");
	assert_text_item_value(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME, "Explore Scientific EXOS II");

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_auto_mount_type_detects_model_on_connect(void) {
	external_serial_simulator simulator = { 0 };
	bool auto_selected = false;
	bool exos2_selected = true;

	SERIAL_CHECK_TRUE(start_pmc8_mount_in_serial_mode(&simulator));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_text_item_value(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME, "Explore Scientific EXOS II");
	SERIAL_CHECK_TRUE(cached_switch_item_value(PMC8_MOUNT_TYPE_PROPERTY_NAME, "AUTO", &auto_selected));
	SERIAL_CHECK_TRUE(auto_selected);
	SERIAL_CHECK_TRUE(cached_switch_item_value(PMC8_MOUNT_TYPE_PROPERTY_NAME, "EXOS-2", &exos2_selected));
	SERIAL_CHECK_TRUE(!exos2_selected);

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_uses_manual_mount_type_without_autodetection(void) {
	external_serial_simulator simulator = { 0 };
	bool driver_up = false;
	bool auto_selected = true;
	bool g11_selected = false;
	bool exos2_selected = true;

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_PMC8_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&pmc8_mount));
	driver_up = true;
	enumerate_simulator_device();

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_CONNECTION_MODE_PROPERTY_NAME, "SERIAL", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(PMC8_CONNECTION_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, PMC8_MOUNT_TYPE_PROPERTY_NAME, "G11", true));
	SERIAL_CHECK_TRUE(wait_for_property_state(PMC8_MOUNT_TYPE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, pmc8_mount.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, simulator.port));
	SERIAL_CHECK_TRUE(wait_for_property_state(DEVICE_PORT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_simulator_connection_state(true));
	assert_text_item_value(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME, "Losmandy G-11");
	SERIAL_CHECK_TRUE(cached_switch_item_value(PMC8_MOUNT_TYPE_PROPERTY_NAME, "AUTO", &auto_selected));
	SERIAL_CHECK_TRUE(!auto_selected);
	SERIAL_CHECK_TRUE(cached_switch_item_value(PMC8_MOUNT_TYPE_PROPERTY_NAME, "G11", &g11_selected));
	SERIAL_CHECK_TRUE(g11_selected);
	SERIAL_CHECK_TRUE(cached_switch_item_value(PMC8_MOUNT_TYPE_PROPERTY_NAME, "EXOS-2", &exos2_selected));
	SERIAL_CHECK_TRUE(!exos2_selected);

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	} else if (driver_up) {
		tear_down_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_performs_basic_mount_operations(void) {
	external_serial_simulator simulator = { 0 };
	const char *coordinate_items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	double sync_values[] = { 1, 45 };
	double track_values[] = { 2, 40 };

	SERIAL_CHECK_TRUE(start_pmc8_mount_in_serial_mode(&simulator));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, pmc8_mount.device_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, ARRAY_SIZE(coordinate_items), coordinate_items, sync_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, pmc8_mount.device_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, ARRAY_SIZE(coordinate_items), coordinate_items, track_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_PARK_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_aborts_during_coordinate_track(void) {
	external_serial_simulator simulator = { 0 };
	const char *coordinate_items[] = {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME,
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME
	};
	double track_values[] = { 3, 35 };

	SERIAL_CHECK_TRUE(start_pmc8_mount_in_serial_mode(&simulator));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property(&simulator_test_client, pmc8_mount.device_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, ARRAY_SIZE(coordinate_items), coordinate_items, track_values));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, INDIGO_BUSY_STATE));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_MOTION_DEC_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_mount_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_PMC8_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&pmc8_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(PMC8_MOUNT_TYPE_PROPERTY_NAME, "EXOS-2");
	assert_text_item_value(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME, "Explore Scientific EXOS II");
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME);
	assert_property_has_item(MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, pmc8_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

	SERIAL_CHECK_TRUE(check_queued_abort(pmc8_mount.device_name, "MOUNT_PARK", "PARKED", 0, true, "MOUNT_ABORT_MOTION", "ABORT_MOTION", true));

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void pmc8_guider_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, MOUNT_PMC8_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&pmc8_guider, &pmc8_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, pmc8_guider.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, pmc8_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, pmc8_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, pmc8_guider.device_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_RA_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 0, 0.001));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, pmc8_guider.device_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_RA_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 0, 0.001));

cleanup:
	if (context.connected) {
		stop_serial_driver(&pmc8_guider);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "pmc8_mount_defines_custom_properties_while_disconnected", pmc8_mount_defines_custom_properties_while_disconnected },
		{ "pmc8_mount_accepts_disconnected_connection_mode_changes", pmc8_mount_accepts_disconnected_connection_mode_changes },
		{ "pmc8_mount_rejects_direct_serial_to_udp_change_while_connected", pmc8_mount_rejects_direct_serial_to_udp_change_while_connected },
		{ "pmc8_mount_connects_over_tcp", pmc8_mount_connects_over_tcp },
		{ "pmc8_mount_connects_over_udp", pmc8_mount_connects_over_udp },
		{ "pmc8_mount_auto_mount_type_detects_model_on_connect", pmc8_mount_auto_mount_type_detects_model_on_connect },
		{ "pmc8_mount_uses_manual_mount_type_without_autodetection", pmc8_mount_uses_manual_mount_type_without_autodetection },
		{ "pmc8_mount_performs_basic_mount_operations", pmc8_mount_performs_basic_mount_operations },
		{ "pmc8_mount_aborts_during_coordinate_track", pmc8_mount_aborts_during_coordinate_track },
		{ "pmc8_mount_passes_serial_compliance_checks", pmc8_mount_passes_serial_compliance_checks },
		{ "pmc8_guider_passes_serial_compliance_checks", pmc8_guider_passes_serial_compliance_checks }
	};
	return indigo_run_tests("PMC-Eight mount serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
