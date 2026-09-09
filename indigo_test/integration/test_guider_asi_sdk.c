// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#include <errno.h>
#include <sys/wait.h>
#include <indigo_drivers/guider_asi/indigo_guider_asi.h>
#include "serial_simulator_test_common.h"
#include "guider_asi_fake_sdk.h"

static const simulator_driver_case guider = { "ASI USB-St4 Guider #7", "indigo_guider_asi", "ASI USB-St4 Guider #7", indigo_guider_asi, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static bool wait_count(atomic_int *count, int expected) {
	for (int i = 0; i < 100; i++) {
		if (*count == expected) { return true; }
		indigo_usleep(20000);
	}
	return false;
}

static bool start(void) {
	if (!bring_up_serial_driver(&guider) || !wait_count(&asi_attached, 1)) { return false; }
	return connect_serial_device(&guider, NULL);
}

static void stop(void) {
	if (context.connected) { disconnect_serial_device(&guider); }
	indigo_result result = indigo_guider_asi(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_result stopped = indigo_stop();
	release_cached_properties();
	ASSERT_EQ_INT(INDIGO_OK, result);
	ASSERT_EQ_INT(INDIGO_OK, stopped);
	ASSERT_EQ_INT(0, asi_attached);
	ASSERT_EQ_INT(0, asi_invalid_io);
}

static void pulse(int direction, int duration) {
	bool ra = direction >= USB2ST4_EAST;
	const char *items[] = { ra ? GUIDER_GUIDE_EAST_ITEM_NAME : GUIDER_GUIDE_NORTH_ITEM_NAME, ra ? GUIDER_GUIDE_WEST_ITEM_NAME : GUIDER_GUIDE_SOUTH_ITEM_NAME };
	double values[] = { direction == USB2ST4_EAST || direction == USB2ST4_NORTH ? duration : 0, direction == USB2ST4_WEST || direction == USB2ST4_SOUTH ? duration : 0 };
	indigo_change_number_property(&simulator_test_client, guider.device_name, ra ? GUIDER_GUIDE_RA_PROPERTY_NAME : GUIDER_GUIDE_DEC_PROPERTY_NAME, 2, items, values);
}

static void directions(void) {
	SERIAL_CHECK_TRUE(start());
	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	for (int direction = 0; direction < 4; direction++) {
		pulse(direction, 100);
		SERIAL_CHECK_TRUE(wait_count(&asi_relays, 1 << direction));
		SERIAL_CHECK_TRUE(wait_count(&asi_relays, 0));
		SERIAL_CHECK_TRUE(wait_for_property_state(direction >= USB2ST4_EAST ? GUIDER_GUIDE_RA_PROPERTY_NAME : GUIDER_GUIDE_DEC_PROPERTY_NAME, INDIGO_OK_STATE));
	}
	pulse(USB2ST4_EAST, 300);
	pulse(USB2ST4_NORTH, 300);
	SERIAL_CHECK_TRUE(wait_count(&asi_relays, (1 << USB2ST4_EAST) | (1 << USB2ST4_NORTH)));
	SERIAL_CHECK_TRUE(wait_count(&asi_relays, 0));
cleanup:
	stop();
}

static void reversal(void) {
	SERIAL_CHECK_TRUE(start());
	pulse(USB2ST4_EAST, 500);
	SERIAL_CHECK_TRUE(wait_count(&asi_relays, 1 << USB2ST4_EAST));
	pulse(USB2ST4_WEST, 100);
	SERIAL_CHECK_EQ_INT(1 << USB2ST4_WEST, asi_relays);
cleanup:
	stop();
}

static void zero_stop(void) {
	SERIAL_CHECK_TRUE(start());
	pulse(USB2ST4_EAST, 500);
	SERIAL_CHECK_TRUE(wait_count(&asi_relays, 1 << USB2ST4_EAST));
	pulse(USB2ST4_EAST, 0);
	SERIAL_CHECK_TRUE(wait_count(&asi_relays, 0));
cleanup:
	stop();
}

static void start_failure(void) {
	SERIAL_CHECK_TRUE(start());
	asi_fail_on = 1;
	pulse(USB2ST4_EAST, 20);
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_RA_PROPERTY_NAME, INDIGO_ALERT_STATE));
cleanup:
	stop();
}

static void stop_failure(void) {
	SERIAL_CHECK_TRUE(start());
	asi_fail_off = 1;
	pulse(USB2ST4_EAST, 20);
	SERIAL_CHECK_TRUE(wait_for_property_state(GUIDER_GUIDE_RA_PROPERTY_NAME, INDIGO_ALERT_STATE));
cleanup:
	stop();
}

static void open_failure(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&guider));
	SERIAL_CHECK_TRUE(wait_count(&asi_attached, 1));
	asi_fail_open = 1;
	SERIAL_CHECK_TRUE(!connect_serial_device(&guider, NULL));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(connect_serial_device(&guider, NULL));
cleanup:
	stop();
}

static void failed_registration_retry(void) {
	// Do not use bring_up_serial_driver: its failure cleanup calls SHUTDOWN
	// and would conceal whether retrying INIT itself actually works.
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_start());
	asi_fail_register = 1;
	SERIAL_CHECK_EQ_INT(INDIGO_FAILED, indigo_guider_asi(INDIGO_DRIVER_INIT, NULL));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_guider_asi(INDIGO_DRIVER_INIT, NULL));
	SERIAL_CHECK_TRUE(wait_count(&asi_attached, 1));
cleanup:
	stop();
}

static void shutdown_pending_arrival(void) {
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_start());
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_guider_asi(INDIGO_DRIVER_INIT, NULL));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_guider_asi(INDIGO_DRIVER_SHUTDOWN, NULL));
	indigo_usleep(800000);
	SERIAL_CHECK_EQ_INT(0, asi_attached);
cleanup:
	stop();
}

static void removal(void) {
	SERIAL_CHECK_TRUE(start());
	asi_fake_removal();
	SERIAL_CHECK_TRUE(wait_count(&asi_attached, 0));
cleanup:
	stop();
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "directions_and_cross_axis", directions },
		{ "reversal", reversal },
		{ "zero_stop", zero_stop },
		{ "start_failure", start_failure },
		{ "stop_failure", stop_failure },
		{ "open_failure_retry", open_failure },
		{ "registration_failure_retry", failed_registration_retry },
		{ "shutdown_pending_arrival", shutdown_pending_arrival },
		{ "removal", removal },
	};
	setvbuf(stdout, NULL, _IOLBF, 0);
	int failures = 0;
	const char *filter = getenv("ASI_TEST_FILTER");
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (filter && !strstr(tests[i].name, filter)) { continue; }
		fflush(NULL);
		pid_t child = fork();
		if (child == 0) { alarm(20); _exit(indigo_run_tests("ASI USB-ST4 fake SDK", tests + i, 1)); }
		int status = 0;
		pid_t waited;
		do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
		if (child < 0 || waited < 0 || !WIFEXITED(status) || WEXITSTATUS(status)) { failures++; printf("FAIL %s (status %d)\n", tests[i].name, status); }
	}
	printf("ASI USB-ST4: %d failing scenarios\n", failures);
	return failures ? 1 : 0;
}
