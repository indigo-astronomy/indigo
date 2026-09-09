// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <indigo_drivers/rotator_lunatico/indigo_rotator_lunatico.h>
#include "serial_simulator_test_common.h"
#include "aux_test_isolation.h"
#ifndef LUNATICO_SIMULATOR
#define LUNATICO_SIMULATOR "build/integration/rotator_lunatico_simulator"
#endif
static const simulator_driver_case main_rotator = { "Lunatico Main", "indigo_rotator_lunatico", "Rotator Lunatico (Main)", indigo_rotator_lunatico, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case exp_rotator = { "Lunatico Exp", "indigo_rotator_lunatico", "Rotator Lunatico (Exp)", indigo_rotator_lunatico, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case third_rotator = { "Lunatico Third", "indigo_rotator_lunatico", "Rotator Lunatico (Third)", indigo_rotator_lunatico, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case exp_focuser = { "Lunatico Focuser", "indigo_rotator_lunatico", "Focuser Lunatico (Exp)", indigo_rotator_lunatico, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case third_aux = { "Lunatico Powerbox", "indigo_rotator_lunatico", "Powerbox Lunatico (Third)", indigo_rotator_lunatico, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case *selected = &main_rotator;

static bool start(const simulator_driver_case *device) {
	selected = device;
	if (!bring_up_serial_driver(&main_rotator)) { return false; }
	indigo_change_switch_property_1(&simulator_test_client, main_rotator.device_name, "LUNATICO_MODEL", "PLATYPUS", true);
	const char *port = device == &exp_rotator ? "LUNATICO_PORT_EXP_CONFIG" : "LUNATICO_PORT_THIRD_CONFIG";
	if (device == &exp_rotator || device == &third_rotator || device == &third_aux) {
		indigo_change_switch_property_1(&simulator_test_client, main_rotator.device_name, port, device == &third_aux ? "AUX_POWERBOX" : "ROTATOR", true);
	}
	char url[PATH_MAX];
	if (!strncmp(aux_simulator.port, "udp://", 6)) { snprintf(url, sizeof(url), "lunatico://%s", aux_simulator.port + 6); }
	else { snprintf(url, sizeof(url), "%s", aux_simulator.port); }
	return connect_serial_device(device, url);
}

static void motion_case(const simulator_driver_case *device) {
	SERIAL_CHECK_TRUE(start(device));
	assert_device_interface(INDIGO_INTERFACE_ROTATOR);
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	indigo_change_switch_property_1(&simulator_test_client, device->device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, true);
	indigo_change_number_property_1(&simulator_test_client, device->device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 0);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 0, .11));
	indigo_change_switch_property_1(&simulator_test_client, device->device_name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true);
	indigo_change_number_property_1(&simulator_test_client, device->device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 20);
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(cached_number_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME) < 20);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 20, .11));
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
cleanup:
	aux_stop(device);
}

static void main_motion(void) { motion_case(&main_rotator); }

static void exp_motion(void) { motion_case(&exp_rotator); }

static void third_motion(void) { motion_case(&third_rotator); }

static void focuser(void) {
	SERIAL_CHECK_TRUE(start(&exp_focuser));
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	indigo_change_number_property_1(&simulator_test_client, selected->device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1200, .1));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
cleanup:
	aux_stop(selected);
}

static void powerbox(void) {
	SERIAL_CHECK_TRUE(start(&third_aux));
	assert_device_interface(INDIGO_INTERFACE_AUX);
	SERIAL_CHECK_TRUE(wait_for_property_state(AUX_GPIO_SENSORS_PROPERTY_NAME, INDIGO_OK_STATE));
	indigo_change_switch_property_1(&simulator_test_client, selected->device_name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(aux_wait_switch(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true));
cleanup:
	aux_stop(selected);
}

static void abort_motion(void) {
	SERIAL_CHECK_TRUE(start(&main_rotator));
	indigo_change_number_property_1(&simulator_test_client, selected->device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 90);
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	indigo_change_switch_property_1(&simulator_test_client, selected->device_name, ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_ABORT_MOTION_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(cached_number_value(ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME) < 90);
cleanup:
	aux_stop(selected);
}

static void move_error(void) {
	SERIAL_CHECK_TRUE(start(&main_rotator));
	indigo_change_number_property_1(&simulator_test_client, selected->device_name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, 20);
	SERIAL_CHECK_TRUE(wait_for_property_state(ROTATOR_POSITION_PROPERTY_NAME, INDIGO_ALERT_STATE));
cleanup:
	aux_stop(selected);
}

static void rejected_identity(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&main_rotator));
	char url[PATH_MAX];
	if (!strncmp(aux_simulator.port, "udp://", 6)) { snprintf(url, sizeof(url), "lunatico://%s", aux_simulator.port + 6); }
	else { snprintf(url, sizeof(url), "%s", aux_simulator.port); }
	SERIAL_CHECK_TRUE(aux_reject(&main_rotator, url));
cleanup:
	aux_stop(&main_rotator);
}

int main(void) {
	const aux_simulated_case tests[] = {
		{ "main_rotator", main_motion, "normal" },
		{ "exp_rotator", exp_motion, "normal" },
		{ "third_rotator", third_motion, "normal" },
		{ "exp_focuser", focuser, "normal" },
		{ "third_powerbox", powerbox, "normal" },
		{ "abort", abort_motion, "normal" },
		{ "goto_failure", move_error, "goto-error" },
		{ "read_failure", move_error, "read-error" },
		{ "wrong_model", rejected_identity, "wrong-model" },
		{ "silent", rejected_identity, "silent" },
		{ "oversized", rejected_identity, "oversized" },
	};
	return run_aux_simulated("Lunatico rotator", LUNATICO_SIMULATOR, tests, ARRAY_SIZE(tests));
}
