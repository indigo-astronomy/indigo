// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/wheel_qhy/indigo_wheel_qhy.h>

#include "serial_simulator_test_common.h"

#ifndef WHEEL_QHY_SIMULATOR_EXECUTABLE
#define WHEEL_QHY_SIMULATOR_EXECUTABLE "build/integration/wheel_qhy_simulator"
#endif

#define X_MODEL_PROPERTY_NAME "X_MODEL"
#define X_MODEL_1_ITEM_NAME   "1"
#define X_MODEL_2_ITEM_NAME   "2"
#define X_MODEL_3_ITEM_NAME   "3"

static const simulator_driver_case qhy_wheel = {
	"QHY CFW Filter Wheel",
	"indigo_wheel_qhy",
	"CFW Filter Wheel",
	indigo_wheel_qhy,
	false,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static bool start_qhy_serial_driver(const char *port, const char *model_item_name) {
	reset_simulator_context(&qhy_wheel);

	if (indigo_start() != INDIGO_OK) {
		return false;
	}
	if (indigo_attach_client(&simulator_test_client) != INDIGO_OK) {
		goto cleanup;
	}
	if (qhy_wheel.entry(INDIGO_DRIVER_INIT, NULL) != INDIGO_OK) {
		goto cleanup;
	}
	enumerate_simulator_device();
	if (!has_defined_property(DEVICE_PORT_PROPERTY_NAME) || !has_defined_property(CONNECTION_PROPERTY_NAME) || !has_defined_property(X_MODEL_PROPERTY_NAME)) {
		goto cleanup;
	}
	if (indigo_change_switch_property_1(&simulator_test_client, qhy_wheel.device_name, X_MODEL_PROPERTY_NAME, model_item_name, true) != INDIGO_OK) {
		goto cleanup;
	}
	if (!wait_for_property_state(X_MODEL_PROPERTY_NAME, INDIGO_OK_STATE)) {
		goto cleanup;
	}
	if (indigo_change_text_property_1_raw(&simulator_test_client, qhy_wheel.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, port) != INDIGO_OK) {
		goto cleanup;
	}
	if (!wait_for_property_state(DEVICE_PORT_PROPERTY_NAME, INDIGO_OK_STATE)) {
		goto cleanup;
	}
	if (indigo_change_switch_property_1(&simulator_test_client, qhy_wheel.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) != INDIGO_OK) {
		goto cleanup;
	}
	if (!wait_for_simulator_connection_state(true)) {
		goto cleanup;
	}
	return true;

cleanup:
	qhy_wheel.entry(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	return false;
}

static void assert_serial_wheel_class_property_completeness(void) {
	static const char *properties[] = {
		WHEEL_SLOT_PROPERTY_NAME,
		WHEEL_SLOT_NAME_PROPERTY_NAME,
		WHEEL_SLOT_OFFSET_PROPERTY_NAME,
		X_MODEL_PROPERTY_NAME
	};
	assert_defined_properties(properties, ARRAY_SIZE(properties));
}

static void qhy_wheel_passes_model_checks(const char *simulator_model, const char *model_item_name) {
	static const char *slot_name_items[] = {
		WHEEL_SLOT_NAME_1_ITEM_NAME,
		WHEEL_SLOT_NAME_2_ITEM_NAME,
		WHEEL_SLOT_NAME_3_ITEM_NAME,
		WHEEL_SLOT_NAME_4_ITEM_NAME,
		WHEEL_SLOT_NAME_5_ITEM_NAME,
		WHEEL_SLOT_NAME_6_ITEM_NAME,
		WHEEL_SLOT_NAME_7_ITEM_NAME
	};
	static const char *slot_offset_items[] = {
		WHEEL_SLOT_OFFSET_1_ITEM_NAME,
		WHEEL_SLOT_OFFSET_2_ITEM_NAME,
		WHEEL_SLOT_OFFSET_3_ITEM_NAME,
		WHEEL_SLOT_OFFSET_4_ITEM_NAME,
		WHEEL_SLOT_OFFSET_5_ITEM_NAME,
		WHEEL_SLOT_OFFSET_6_ITEM_NAME,
		WHEEL_SLOT_OFFSET_7_ITEM_NAME
	};
	static const char *model_items[] = {
		X_MODEL_1_ITEM_NAME,
		X_MODEL_2_ITEM_NAME,
		X_MODEL_3_ITEM_NAME
	};
	const char *arguments[] = { "--model", simulator_model, NULL };
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator_with_args(&simulator, WHEEL_QHY_SIMULATOR_EXECUTABLE, arguments));
	SERIAL_CHECK_TRUE(start_qhy_serial_driver(simulator.port, model_item_name));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_WHEEL);
	assert_serial_wheel_class_property_completeness();
	assert_property_has_item(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);
	assert_property_has_items(WHEEL_SLOT_NAME_PROPERTY_NAME, slot_name_items, ARRAY_SIZE(slot_name_items));
	assert_property_has_items(WHEEL_SLOT_OFFSET_PROPERTY_NAME, slot_offset_items, ARRAY_SIZE(slot_offset_items));
	assert_property_has_items(X_MODEL_PROPERTY_NAME, model_items, ARRAY_SIZE(model_items));
	assert_number_item_in_range(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 1, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, qhy_wheel.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 4));
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 4, 0.001));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, qhy_wheel.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 7));
	SERIAL_CHECK_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 7, 0.001));

cleanup:
	if (context.connected) {
		stop_serial_driver(&qhy_wheel);
	}
	stop_external_serial_simulator(&simulator);
}

static void qhy_cfw1_wheel_passes_serial_compliance_checks(void) {
	qhy_wheel_passes_model_checks("cfw1", X_MODEL_1_ITEM_NAME);
}

static void qhy_cfw2_wheel_passes_serial_compliance_checks(void) {
	qhy_wheel_passes_model_checks("cfw2", X_MODEL_2_ITEM_NAME);
}

static void qhy_cfw3_wheel_passes_serial_compliance_checks(void) {
	qhy_wheel_passes_model_checks("cfw3", X_MODEL_3_ITEM_NAME);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "qhy_cfw1_wheel_passes_serial_compliance_checks", qhy_cfw1_wheel_passes_serial_compliance_checks },
		{ "qhy_cfw2_wheel_passes_serial_compliance_checks", qhy_cfw2_wheel_passes_serial_compliance_checks },
		{ "qhy_cfw3_wheel_passes_serial_compliance_checks", qhy_cfw3_wheel_passes_serial_compliance_checks }
	};
	return indigo_run_tests("QHY CFW wheel serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
