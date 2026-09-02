// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/mount_lx200/indigo_mount_lx200.h>

#include "serial_simulator_test_common.h"

#ifndef MOUNT_LX200_SIMULATOR_EXECUTABLE
#define MOUNT_LX200_SIMULATOR_EXECUTABLE "build/integration/mount_lx200_simulator"
#endif

#define MOUNT_TYPE_PROPERTY_NAME         "X_MOUNT_TYPE"
#define MOUNT_TYPE_ON_STEP_ITEM_NAME     "ONSTEP"
#define MOUNT_MODE_PROPERTY_NAME         "X_MOUNT_MODE"

static const simulator_driver_case lx200_mount = {
	"LX200 Mount",
	"indigo_mount_lx200",
	MOUNT_LX200_NAME,
	indigo_mount_lx200,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case lx200_guider = {
	"LX200 Mount (guider)",
	"indigo_mount_lx200",
	MOUNT_LX200_GUIDER_NAME,
	indigo_mount_lx200,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case lx200_focuser = {
	"LX200 Mount (focuser)",
	"indigo_mount_lx200",
	MOUNT_LX200_FOCUSER_NAME,
	indigo_mount_lx200,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static const simulator_driver_case lx200_aux = {
	"LX200 Mount (aux)",
	"indigo_mount_lx200",
	MOUNT_LX200_AUX_NAME,
	indigo_mount_lx200,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static bool start_lx200_simulator(external_serial_simulator *simulator, const char *model) {
	const char *arguments[] = { "--model", model, NULL };
	return start_external_serial_simulator_with_args(simulator, MOUNT_LX200_SIMULATOR_EXECUTABLE, arguments);
}

static void lx200_mount_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_lx200_simulator(&simulator, "meade"));
	SERIAL_CHECK_TRUE(start_serial_driver(&lx200_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_MOUNT);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_VENDOR_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_MODEL_ITEM_NAME);
	assert_property_has_item(MOUNT_INFO_PROPERTY_NAME, MOUNT_INFO_FIRMWARE_ITEM_NAME);
	assert_property_has_item(MOUNT_TYPE_PROPERTY_NAME, "MEADE");
	assert_property_has_item(MOUNT_MODE_PROPERTY_NAME, "EQUATORIAL");
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME);
	assert_property_has_item(MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME);
	assert_property_has_item(MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lx200_mount.device_name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_TRACKING_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lx200_mount.device_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(MOUNT_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&lx200_mount);
	}
	stop_external_serial_simulator(&simulator);
}

static void lx200_guider_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_lx200_simulator(&simulator, "meade"));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&lx200_guider, &lx200_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME);
	assert_property_has_item(GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME);

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lx200_guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 100));
	SERIAL_CHECK_TRUE(wait_for_property_not_busy(GUIDER_GUIDE_DEC_PROPERTY_NAME));

cleanup:
	if (context.connected) {
		stop_serial_driver(&lx200_guider);
	}
	stop_external_serial_simulator(&simulator);
}

static void lx200_focuser_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_lx200_simulator(&simulator, "meade"));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&lx200_focuser, &lx200_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME);
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lx200_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 10));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&lx200_focuser);
	}
	stop_external_serial_simulator(&simulator);
}

static void lx200_aux_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_lx200_simulator(&simulator, "onstep"));
	SERIAL_CHECK_TRUE(start_shared_serial_device_with_master_case(&lx200_aux, &lx200_mount, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER);
	assert_serial_aux_class_property_completeness();
	assert_property_has_item(AUX_POWER_OUTLET_PROPERTY_NAME, "OUTLET_1");
	assert_property_has_item(AUX_HEATER_OUTLET_PROPERTY_NAME, "OUTLET_1");

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lx200_aux.device_name, AUX_POWER_OUTLET_PROPERTY_NAME, "OUTLET_1", false));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_POWER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lx200_aux.device_name, AUX_HEATER_OUTLET_PROPERTY_NAME, "OUTLET_1", 50));
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_HEATER_OUTLET_PROPERTY_NAME, INDIGO_OK_STATE));

cleanup:
	if (context.connected) {
		stop_serial_driver(&lx200_aux);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "lx200_mount_passes_serial_compliance_checks", lx200_mount_passes_serial_compliance_checks },
		{ "lx200_guider_passes_serial_compliance_checks", lx200_guider_passes_serial_compliance_checks },
		{ "lx200_focuser_passes_serial_compliance_checks", lx200_focuser_passes_serial_compliance_checks },
		{ "lx200_aux_passes_serial_compliance_checks", lx200_aux_passes_serial_compliance_checks }
	};
	return indigo_run_tests("LX200 mount serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
