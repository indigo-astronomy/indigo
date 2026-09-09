// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <stdatomic.h>
#include <stdarg.h>
#include <indigo/indigo_uni_io.h>
#include <indigo_drivers/ao_sx/indigo_ao_sx.h>
#include "serial_simulator_test_common.h"

static const simulator_driver_case ao = { "SX AO", "indigo_ao_sx", "SX AO", indigo_ao_sx, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static const simulator_driver_case guider = { "SX AO guider", "indigo_ao_sx", "SX AO (guider)", indigo_ao_sx, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
static indigo_uni_handle handle;
static atomic_int opens, closes, invalid_io, requests, fail_open, failure;
static char command[32];
static char failed_command;
static atomic_bool hold_read, read_entered, release_read;
static pthread_mutex_t transport_mutex = PTHREAD_MUTEX_INITIALIZER;

indigo_uni_handle *ao_test_open(const char *port, int level) {
	if (atomic_exchange(&fail_open, 0)) {
		return NULL;
	}
	atomic_fetch_add(&opens, 1);
	return &handle;
}

void ao_test_close(indigo_uni_handle **port) {
	if (*port) {
		atomic_fetch_add(&closes, 1);
		*port = NULL;
	}
}

long ao_test_discard(indigo_uni_handle *port) {
	if (port != &handle || opens == closes) {
		atomic_fetch_add(&invalid_io, 1);
		return -1;
	}
	return 0;
}

long ao_test_write(indigo_uni_handle *port, const char *format, va_list args) {
	if (ao_test_discard(port) < 0) {
		return -1;
	}
	pthread_mutex_lock(&transport_mutex);
	int size = vsnprintf(command, sizeof(command), format, args);
	atomic_fetch_add(&requests, 1);
	if (failure == 1 && (!failed_command || failed_command == command[0])) {
		failure = 0;
		size = -1;
	}
	pthread_mutex_unlock(&transport_mutex);
	return size;
}

long ao_test_read(indigo_uni_handle *port, char *buffer, long length, const char *terminators, const char *ignore, long timeout) {
	if (ao_test_discard(port) < 0) {
		return -1;
	}
	if (atomic_exchange(&hold_read, false)) {
		atomic_store(&read_entered, true);
		while (!atomic_load(&release_read)) {
			indigo_usleep(1000);
		}
	}
	pthread_mutex_lock(&transport_mutex);
	const char *reply = command[0] == 'X' ? "Y" : command[0] == 'V' ? "V123" : command[0] == 'L' ? "0" : command[0] == 'G' ? "G" : command[0] == 'M' ? "M" : "K";
	int mode = (!failed_command || failed_command == command[0]) ? atomic_exchange(&failure, 0) : 0;
	int size = strlen(reply);
	if (mode == 2) {
		size = -1;
	} else if (mode == 3) {
		size--;
	} else if (mode == 4) {
		reply = "!";
		size = 1;
	} else if (mode == 5) {
		reply = "L";
		size = 1;
	}
	if (size > 0) {
		memcpy(buffer, reply, size);
	}
	pthread_mutex_unlock(&transport_mutex);
	return size;
}

static bool command_is(const char *expected) {
	pthread_mutex_lock(&transport_mutex);
	bool same = !strcmp(command, expected);
	pthread_mutex_unlock(&transport_mutex);
	return same;
}

static void directions_and_units(void) {
	const simulator_driver_case *devices[] = { &ao, &guider };
	for (int d = 0; d < 2; d++) {
		const simulator_driver_case *device = devices[d];
		SERIAL_CHECK_TRUE(d ? start_shared_serial_device(device, ao.device_name, "fake") : start_serial_driver(device, "fake"));
		const char *props[] = { d ? GUIDER_GUIDE_RA_PROPERTY_NAME : AO_GUIDE_RA_PROPERTY_NAME, d ? GUIDER_GUIDE_DEC_PROPERTY_NAME : AO_GUIDE_DEC_PROPERTY_NAME };
		const char *items[] = { "EAST", "WEST", "NORTH", "SOUTH" };
		const char *expected[] = { d ? "MT00012" : "GT00012", d ? "MW00012" : "GW00012", d ? "MN00012" : "GN00012", d ? "MS00012" : "GS00012" };
		for (int i = 0; i < 4; i++) {
			indigo_change_number_property_1(&simulator_test_client, device->device_name, props[i / 2], items[i], d ? 129 : 12);
			SERIAL_CHECK_TRUE(wait_for_property_state(props[i / 2], INDIGO_OK_STATE));
			SERIAL_CHECK_TRUE(command_is(expected[i]));
			SERIAL_CHECK_TRUE(wait_for_number_item_value(props[i / 2], items[i], 0, 0));
		}
		int before = requests;
		indigo_change_number_property_1(&simulator_test_client, device->device_name, props[0], "EAST", 0);
		SERIAL_CHECK_TRUE(wait_for_property_state(props[0], INDIGO_OK_STATE));
		SERIAL_CHECK_EQ_INT(before, requests);
		stop_serial_driver(device);
	}
cleanup:
	stop_serial_driver(context.driver_case ? context.driver_case : &ao);
	ASSERT_EQ_INT(opens, closes);
	ASSERT_EQ_INT(0, invalid_io);
}

static void command_errors_and_recovery(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&ao, "fake"));
	for (int mode = 1; mode <= 5; mode++) {
		failed_command = 'G';
		atomic_store(&failure, mode);
		indigo_change_number_property_1(&simulator_test_client, ao.device_name, AO_GUIDE_RA_PROPERTY_NAME, "EAST", 10);
		SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_RA_PROPERTY_NAME, INDIGO_ALERT_STATE));
		indigo_change_number_property_1(&simulator_test_client, ao.device_name, AO_GUIDE_RA_PROPERTY_NAME, "WEST", 10);
		SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	}
	for (int reset = 0; reset < 2; reset++) {
		failed_command = reset ? 'R' : 'K';
		failure = 2;
		indigo_change_switch_property_1(&simulator_test_client, ao.device_name, AO_RESET_PROPERTY_NAME, reset ? "UNJAM" : "CENTER", true);
		SERIAL_CHECK_TRUE(wait_for_property_state(AO_RESET_PROPERTY_NAME, INDIGO_ALERT_STATE));
		indigo_change_switch_property_1(&simulator_test_client, ao.device_name, AO_RESET_PROPERTY_NAME, reset ? "UNJAM" : "CENTER", true);
		SERIAL_CHECK_TRUE(wait_for_property_state(AO_RESET_PROPERTY_NAME, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(command_is(reset ? "R" : "K"));
	}
cleanup:
	failure = 0;
	failed_command = 0;
	stop_serial_driver(&ao);
	ASSERT_EQ_INT(opens, closes);
}

static void initialization_rollback(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&ao));
	fail_open = 1;
	SERIAL_CHECK_TRUE(!connect_serial_device(&ao, "fake"));
	const char commands[] = { 'X', 'V', 'L' };
	for (int i = 0; i < 3; i++) {
		failed_command = commands[i];
		failure = 3;
		SERIAL_CHECK_TRUE(!connect_serial_device(&ao, "fake"));
		SERIAL_CHECK_EQ_INT(opens, closes);
	}
	failed_command = 0;
	SERIAL_CHECK_TRUE(connect_serial_device(&ao, "fake"));
	SERIAL_CHECK_EQ_INT(INDIGO_BUSY, indigo_ao_sx(INDIGO_DRIVER_SHUTDOWN, NULL));
cleanup:
	failure = 0;
	failed_command = 0;
	stop_serial_driver(&ao);
	ASSERT_EQ_INT(opens, closes);
}

static bool wait_read_gate(void) {
	for (int i = 0; i < 2000; i++) {
		if (read_entered) {
			return true;
		}
		indigo_usleep(1000);
	}
	return false;
}

static void queued_correction_reset_and_disconnect(void) {
	SERIAL_CHECK_TRUE(start_serial_driver(&ao, "fake"));
	read_entered = release_read = false;
	hold_read = true;
	indigo_change_number_property_1(&simulator_test_client, ao.device_name, AO_GUIDE_RA_PROPERTY_NAME, "EAST", 20);
	SERIAL_CHECK_TRUE(wait_read_gate());
	indigo_change_number_property_1(&simulator_test_client, ao.device_name, AO_GUIDE_DEC_PROPERTY_NAME, "SOUTH", 20);
	indigo_change_switch_property_1(&simulator_test_client, ao.device_name, AO_RESET_PROPERTY_NAME, "CENTER", true);
	SERIAL_CHECK_EQ_INT(INDIGO_BUSY_STATE, find_cached_property(AO_RESET_PROPERTY_NAME)->state);
	release_read = true;
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_RESET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(command_is("K"));
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_RA_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	read_entered = release_read = false;
	hold_read = true;
	indigo_change_number_property_1(&simulator_test_client, ao.device_name, AO_GUIDE_RA_PROPERTY_NAME, "WEST", 20);
	SERIAL_CHECK_TRUE(wait_read_gate());
	indigo_change_switch_property_1(&simulator_test_client, ao.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
	SERIAL_CHECK_EQ_INT(1, opens - closes);
	release_read = true;
	SERIAL_CHECK_TRUE(wait_for_simulator_connection_state(false));
	SERIAL_CHECK_EQ_INT(opens, closes);
	int before = requests;
	int updates = context.update_count;
	indigo_usleep(100000);
	SERIAL_CHECK_EQ_INT(before, requests);
	SERIAL_CHECK_EQ_INT(updates, context.update_count);
	SERIAL_CHECK_TRUE(connect_serial_device(&ao, "fake"));
	indigo_change_number_property_1(&simulator_test_client, ao.device_name, AO_GUIDE_RA_PROPERTY_NAME, "EAST", 10);
	SERIAL_CHECK_TRUE(wait_for_property_state(AO_GUIDE_RA_PROPERTY_NAME, INDIGO_OK_STATE));
cleanup:
	release_read = true;
	hold_read = false;
	stop_serial_driver(&ao);
	ASSERT_EQ_INT(opens, closes);
	ASSERT_EQ_INT(0, invalid_io);
}

static void shared_connection_orders(void) {
	for (int order = 0; order < 2; order++) {
		const simulator_driver_case *first = order ? &guider : &ao;
		const simulator_driver_case *second = order ? &ao : &guider;
		int before = opens;
		SERIAL_CHECK_TRUE(order ? start_shared_serial_device(first, ao.device_name, "fake") : start_serial_driver(first, "fake"));
		bool connected = connect_serial_device(second, NULL);
		if (!connected) {
			fprintf(stderr, "order=%d target=%s cached=%d state=%d connected=%d disconnected=%d opens=%d closes=%d invalid=%d\n", order, second->device_name, context.defined_property_count, context.last_connection_state, context.connected, context.disconnected, opens, closes, invalid_io);
		}
		SERIAL_CHECK_TRUE(connected);
		SERIAL_CHECK_EQ_INT(before + 1, opens);
		SERIAL_CHECK_EQ_INT(1, opens - closes);
		disconnect_serial_device(first);
		SERIAL_CHECK_EQ_INT(1, opens - closes);
		reset_simulator_context(second);
		enumerate_simulator_device();
		const char *property = order ? AO_GUIDE_RA_PROPERTY_NAME : GUIDER_GUIDE_RA_PROPERTY_NAME;
		indigo_change_number_property_1(&simulator_test_client, second->device_name, property, "EAST", 20);
		SERIAL_CHECK_TRUE(wait_for_property_state(property, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(command_is(order ? "GT00020" : "MT00002"));
		SERIAL_CHECK_TRUE(connect_serial_device(first, NULL));
		SERIAL_CHECK_EQ_INT(before + 1, opens);
		disconnect_serial_device(second);
		SERIAL_CHECK_EQ_INT(1, opens - closes);
		disconnect_serial_device(first);
		SERIAL_CHECK_EQ_INT(opens, closes);
		tear_down_serial_driver(&ao);
	}
	return;
cleanup:
	disconnect_serial_device(&guider);
	disconnect_serial_device(&ao);
	tear_down_serial_driver(&ao);
}

static void guider_errors_and_quantization(void) {
	SERIAL_CHECK_TRUE(start_shared_serial_device(&guider, ao.device_name, "fake"));
	const int durations[] = { 1, 9, 10, 999, 1000 };
	for (int i = 0; i < ARRAY_SIZE(durations); i++) {
		indigo_change_number_property_1(&simulator_test_client, guider.device_name, GUIDER_GUIDE_RA_PROPERTY_NAME, "EAST", durations[i]);
		SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_RA_PROPERTY_NAME, INDIGO_OK_STATE));
		char expected[32];
		snprintf(expected, sizeof(expected), "MT%05d", durations[i] / 10);
		SERIAL_CHECK_TRUE(command_is(expected));
	}
	for (int mode = 1; mode <= 4; mode++) {
		failed_command = 'M';
		failure = mode;
		indigo_change_number_property_1(&simulator_test_client, guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, "NORTH", 100);
		SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_DEC_PROPERTY_NAME, INDIGO_ALERT_STATE));
		indigo_change_number_property_1(&simulator_test_client, guider.device_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, "SOUTH", 100);
		SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(command_is("MS00010"));
	}
cleanup:
	failure = 0;
	failed_command = 0;
	stop_serial_driver(&guider);
	ASSERT_EQ_INT(opens, closes);
	ASSERT_EQ_INT(0, invalid_io);
}

int main(void) {
	alarm(60);
	setvbuf(stdout, NULL, _IOLBF, 0);
	const indigo_test_case tests[] = {
		{ "all AO and guider directions, units and zero requests", directions_and_units },
		{ "command, acknowledgement, limit and reset failures", command_errors_and_recovery },
		{ "handshake and initial status rollback", initialization_rollback },
		{ "shared handle in both connection orders", shared_connection_orders },
		{ "guider errors and 10 ms quantization", guider_errors_and_quantization },
		{ "queued axes, reset and disconnect", queued_correction_reset_and_disconnect }
	};
	if (getenv("INDIGO_TEST_QUEUE_ONLY")) {
		return indigo_run_tests("SX AO queue", tests + 5, 1);
	}
	if (getenv("INDIGO_TEST_SHARED_ONLY")) {
		return indigo_run_tests("SX AO shared lifetime", tests + 3, 1);
	}
	return indigo_run_tests("SX AO fake transport", tests, ARRAY_SIZE(tests));
}
