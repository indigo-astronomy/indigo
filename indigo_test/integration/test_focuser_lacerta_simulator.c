// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_lacerta/indigo_focuser_lacerta.h>

#include "serial_simulator_test_common.h"
#include <indigo/indigo_uni_io.h>
#include <errno.h>
#include <stdatomic.h>
#include <fcntl.h>

#ifndef FOCUSER_LACERTA_SIMULATOR_EXECUTABLE
#define FOCUSER_LACERTA_SIMULATOR_EXECUTABLE "build/integration/focuser_lacerta_simulator"
#endif

static const simulator_driver_case lacerta_focuser = {
	"LACERTA Motorfocus Focuser",
	"indigo_focuser_lacerta",
	"LACERTA Motorfocus",
	indigo_focuser_lacerta,
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

static external_serial_simulator fixture, second_fixture;
static char fixture_dir[] = "/tmp/indigo-lacerta.XXXXXX";
static char event_path[256], fault_path[256];
static const char *current_profile;

static bool fault(const char *command, const char *action) {
	char temporary[280];
	snprintf(temporary, sizeof(temporary), "%s.tmp", fault_path);
	FILE *file = fopen(temporary, "w");
	if (!file) {
		return false;
	}
	fprintf(file, "%s %s\n", command, action);
	fclose(file);
	return rename(temporary, fault_path) == 0;
}

static int commands(const char *prefix) {
	FILE *file = fopen(event_path, "r");
	if (!file) {
		return -1;
	}
	char line[512], kind[16], value[256];
	double timestamp;
	int count = 0;
	while (fgets(line, sizeof(line), file)) {
		if (sscanf(line, "%lf %15s %255[^\r\n]", &timestamp, kind, value) == 3 && !strcmp(kind, "RX") && !strncmp(value, prefix, strlen(prefix))) {
			count++;
		}
	}
	fclose(file);
	return count;
}

static void driver_stop(void) {
	disconnect_serial_device(&lacerta_focuser);
	bool disconnected = !context.connected;
	indigo_result result = indigo_focuser_lacerta(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	ASSERT_TRUE(disconnected);
	ASSERT_EQ_INT(INDIGO_OK, result);
}

static bool driver_start(void) {
	return bring_up_serial_driver(&lacerta_focuser) && connect_serial_device(&lacerta_focuser, fixture.port);
}

static int open_descriptors(void) {
	int count = 0;
	for (int fd = 0; fd < 1024; fd++) {
		if (fcntl(fd, F_GETFD) >= 0) {
			count++;
		}
	}
	return count;
}

static void rejected_connection(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&lacerta_focuser));
	int descriptors = open_descriptors();
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, lacerta_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, fixture.port));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lacerta_focuser.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(!context.connected);
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	if (!strncmp(current_profile, "init_", 5)) {
		SERIAL_CHECK_TRUE(connect_serial_device(&lacerta_focuser, fixture.port));
	}
cleanup:
	driver_stop();
}

static void noop_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 0));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

typedef struct { const char *name; void (*run)(void); const char *profile; } lacerta_test;

static int run_cases(const lacerta_test *cases, int count) {
	int failures = 0;
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *filter = getenv("LACERTA_TEST_FILTER");
	if (!mkdtemp(fixture_dir)) {
		return 1;
	}
	snprintf(event_path, sizeof(event_path), "%s/events", fixture_dir);
	snprintf(fault_path, sizeof(fault_path), "%s/fault", fixture_dir);
	setenv("INDIGO_LACERTA_EVENTS", event_path, 1);
	setenv("INDIGO_LACERTA_FAULT", fault_path, 1);
	for (int i = 0; i < count; i++) {
		if (filter && !strstr(cases[i].name, filter)) {
			continue;
		}
		current_profile = cases[i].profile;
		unlink(fault_path);
		const char *args[] = { "--profile", current_profile, NULL };
		if (!start_external_serial_simulator_with_args(&fixture, FOCUSER_LACERTA_SIMULATOR_EXECUTABLE, args)) {
			failures++;
			break;
		}
		if (!strcmp(cases[i].name, "instances")) {
			unsetenv("INDIGO_LACERTA_EVENTS");
			unsetenv("INDIGO_LACERTA_FAULT");
			const char *second_args[] = { "--profile", "alternate", NULL };
			bool ready = start_external_serial_simulator_with_args(&second_fixture, FOCUSER_LACERTA_SIMULATOR_EXECUTABLE, second_args);
			setenv("INDIGO_LACERTA_EVENTS", event_path, 1);
			setenv("INDIGO_LACERTA_FAULT", fault_path, 1);
			if (!ready) {
				stop_external_serial_simulator(&fixture);
				failures++;
				break;
			}
		}
		fflush(NULL);
		pid_t child = fork();
		if (child == 0) {
			alarm(35);
			indigo_test_case test = { cases[i].name, cases[i].run };
			_exit(indigo_run_tests("Lacerta", &test, 1));
		}
		int status = 0;
		if (child > 0) {
			while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
			}
		}
		stop_external_serial_simulator(&fixture);
		stop_external_serial_simulator(&second_fixture);
		if (child < 0 || !WIFEXITED(status) || WEXITSTATUS(status)) {
			fprintf(stderr, "FAIL %s (status %d)\n", cases[i].name, status);
			failures++;
		}
	}
	unlink(fault_path);
	unlink(event_path);
	rmdir(fixture_dir);
	unsetenv("INDIGO_LACERTA_EVENTS");
	unsetenv("INDIGO_LACERTA_FAULT");
	printf("Lacerta: %d failing scenarios\n", failures);
	return failures ? 1 : 0;
}

static void lacerta_focuser_passes_serial_compliance_checks(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME);
	assert_property_has_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME);
	assert_property_has_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	double sync_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	SERIAL_CHECK_TRUE(!isnan(sync_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position, 1));

	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 5));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_BACKLASH_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));

	double max_position = bounded_number_value(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 200000);
	SERIAL_CHECK_TRUE(!isnan(max_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, max_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_LIMITS_PROPERTY_NAME, INDIGO_OK_STATE));

	double target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, sync_position + 200);
	SERIAL_CHECK_TRUE(!isnan(target_position));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 1));

cleanup:
	driver_stop();
}

static bool lacerta_exchange(indigo_uni_handle *handle, const char *command, const char *expected) {
	char response[128];
	long length = (long)strlen(command);
	char stale[128];
	while (indigo_uni_wait_for_data(handle, 0) > 0) {
		if (indigo_uni_read_available(handle, stale, sizeof(stale)) <= 0) {
			return false;
		}
	}
	if (indigo_uni_write(handle, command, length) != length) {
		return false;
	}
	if (!expected) {
		return true;
	}
	if (indigo_uni_read_section(handle, response, sizeof(response), "\r", "\r\n", INDIGO_DELAY(2)) <= 0) {
		return false;
	}
	if (strcmp(response, expected)) {
		fprintf(stderr, "Lacerta %s: expected '%s', got '%s'\n", command, expected, response);
		return false;
	}
	return true;
}

static int lacerta_position(indigo_uni_handle *handle) {
	char response[128], extra;
	int position;
	if (!lacerta_exchange(handle, ": q #", NULL)) {
		return -1;
	}
	for (int i = 0; i < 8; i++) {
		if (indigo_uni_read_section(handle, response, sizeof(response), "\r", "\r\n", INDIGO_DELAY(2)) <= 0) {
			return -1;
		}
		if (sscanf(response, "p %d %c", &position, &extra) == 1) {
			return position;
		}
	}
	return -1;
}

static bool lacerta_arrived(indigo_uni_handle *handle, int target) {
	for (int i = 0; i < 60; i++) {
		int actual = lacerta_position(handle);
		if (i == 59) {
			fprintf(stderr, "Expected simulator target %d, actual %d\n", target, actual);
		}
		if (actual == target) {
			return true;
		}
		indigo_usleep(50000);
	}
	return false;
}

static void lacerta_simulator_protocol(const char *model, const char *firmware) {
	indigo_uni_handle *handle = NULL;
	char expected[64];
	handle = indigo_uni_open_serial_with_speed(fixture.port, 9600, INDIGO_LOG_DEBUG);
	SERIAL_CHECK_TRUE(handle != NULL);
	snprintf(expected, sizeof(expected), "i %s", model);
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": i #", expected));
	snprintf(expected, sizeof(expected), "v%s", firmware);
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": v #", expected));
	// Workbook Munka1 rows 5-6, 17-18, 26-29 and 32: supported controls.
	const char *commands[][2] = {
		{ ": t #", "t 23.5" }, { ": b #", "b 3" }, { ": B 255#", "b 255" }, { ": b #", "b 255" },
		{ ": B 0#", "b 0" }, { ": b #", "b 0" }, { ": R 1#", "r 1" }, { ": r #", "r 1" },
		{ ": R 0#", "r 0" }, { ": r #", "r 0" }, { ": G 1000#", "g 1000" }, { ": g #", "g 1000" },
		{ ": P 0#", "p 0" }
	};
	for (int i = 0; i < ARRAY_SIZE(commands); i++) {
		SERIAL_CHECK_TRUE(lacerta_exchange(handle, commands[i][0], commands[i][1]));
	}
	// Motion progresses with elapsed time even without position queries.
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": M 1000#", NULL));
	indigo_usleep(300000);
	int progress = lacerta_position(handle);
	SERIAL_CHECK_TRUE(progress >= 200 && progress < 900);
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": H #", "H 1"));
	int stopped = lacerta_position(handle);
	SERIAL_CHECK_TRUE(stopped >= progress && stopped < 1000);
	indigo_usleep(200000);
	SERIAL_CHECK_EQ_INT(stopped, lacerta_position(handle));
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": H #", "H 1"));
	// SYNC during motion cancels the old target, without subsequent movement.
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": M 1000#", NULL));
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": P 400#", "p 400"));
	indigo_usleep(150000);
	SERIAL_CHECK_EQ_INT(400, lacerta_position(handle));
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": M 400#", NULL));
	SERIAL_CHECK_EQ_INT(400, lacerta_position(handle));
	// Preserve the simulator's established travel clipping at both ends.
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": M 1200#", NULL));
	SERIAL_CHECK_TRUE(lacerta_arrived(handle, 1000));
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": M -1#", NULL));
	SERIAL_CHECK_TRUE(lacerta_arrived(handle, 0));
	// serial_motion keeps even a short nonzero move observable for 0.5 s.
	SERIAL_CHECK_TRUE(lacerta_exchange(handle, ": M 10#", NULL));
	indigo_usleep(100000);
	progress = lacerta_position(handle);
	SERIAL_CHECK_TRUE(progress >= 0 && progress < 10);
	SERIAL_CHECK_TRUE(lacerta_arrived(handle, 10));
cleanup:
	if (handle) {
		indigo_uni_close(&handle);
	}
}

static void lacerta_simulator_mfoc_protocol(void) {
	lacerta_simulator_protocol("MFOC", "3.1.123");
}

static void lacerta_simulator_fmc_protocol(void) {
	lacerta_simulator_protocol("FMC", "1.1.123");
}

static const char *observed_names[] = { CONNECTION_PROPERTY_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_TEMPERATURE_PROPERTY_NAME };
static atomic_uint revisions[8], motion_busy;

static int observed_index(const char *name) {
	for (int i = 0; i < 8; i++) {
		if (!strcmp(name, observed_names[i])) {
			return i;
		}
	}
	return -1;
}

static indigo_result observe_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_result result = simulator_client_update_property(client, device, property, message);
	int i = observed_index(property->name);
	if (i >= 0) {
		atomic_fetch_add(&revisions[i], 1);
	}
	if (i == 1 && property->state == INDIGO_BUSY_STATE) {
		atomic_fetch_add(&motion_busy, 1);
	}
	return result;
}

static bool new_state(const char *name, unsigned before, indigo_property_state state) {
	int index = observed_index(name);
	for (int i = 0; i < 160; i++) {
		indigo_property *property = find_cached_property(name);
		if (atomic_load(&revisions[index]) > before && property && property->state == state) {
			return true;
		}
		indigo_usleep(50000);
	}
	fprintf(stderr, "No fresh %s state %d\n", name, state);
	return false;
}

static bool number_change(const char *property, const char *item, double value, indigo_property_state state) {
	unsigned before = atomic_load(&revisions[observed_index(property)]);
	return indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, property, item, value) == INDIGO_OK && new_state(property, before, state);
}

static bool switch_change(const char *property, const char *item, bool value, indigo_property_state state) {
	int index = observed_index(property);
	unsigned before = index >= 0 ? atomic_load(&revisions[index]) : 0;
	if (indigo_change_switch_property_1(&simulator_test_client, lacerta_focuser.device_name, property, item, value) != INDIGO_OK) {
		return false;
	}
	return index >= 0 ? new_state(property, before, state) : wait_for_property_state(property, state);
}

static bool sync_to(int position) {
	return switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true, INDIGO_OK_STATE) && number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, INDIGO_OK_STATE) && switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true, INDIGO_OK_STATE);
}

static bool at_position(int position) {
	return wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, .01) && wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE) && wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE);
}

static void capabilities(void) {
	SERIAL_CHECK_TRUE(driver_start());
	int maximum = !strcmp(current_profile, "fmc") ? 65535 : 250000;
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_SPEED_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_COMPENSATION_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_MODE_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(find_cached_property(INFO_PROPERTY_NAME)->count == 6);
	SERIAL_CHECK_TRUE(find_cached_property(FOCUSER_LIMITS_PROPERTY_NAME)->count == 2);
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.max == maximum);
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME)->number.max == 255);
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME)->number.step == 1);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, 23.5, .01));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 255, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(commands(": B 255#") == 1);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.max == 1000);
	SERIAL_CHECK_TRUE(sync_to(500));
	SERIAL_CHECK_TRUE(commands(": P 500#") == 1 && commands(": M ") == 0);
cleanup:
	driver_stop();
}

static void relative_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(sync_to(500));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	unsigned busy = atomic_load(&motion_busy);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(400) && atomic_load(&motion_busy) > busy);
	SERIAL_CHECK_TRUE(commands(": M 400#") == 1);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(500));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(0) && commands(": M 0#") == 1);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(1000));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 0, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(1000));
cleanup:
	driver_stop();
}

static void abort_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10000, INDIGO_BUSY_STATE));
	for (int i = 0; i < 60 && commands(": M 10000#") == 0; i++) {
		indigo_usleep(50000);
	}
	SERIAL_CHECK_TRUE(commands(": M 10000#") == 1);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	indigo_item *position = find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	SERIAL_CHECK_TRUE(position->number.value == position->number.target && position->number.value < 10000);
	SERIAL_CHECK_TRUE(!find_cached_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME)->sw.value);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(800));
cleanup:
	driver_stop();
}

static void overlap(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 2000, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 3000));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 50));
	SERIAL_CHECK_TRUE(at_position(2000));
	SERIAL_CHECK_TRUE(commands(": M ") == 1);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 2000, INDIGO_BUSY_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 3000));
	SERIAL_CHECK_TRUE(at_position(4000));
	SERIAL_CHECK_TRUE(commands(": M ") == 2);
cleanup:
	driver_stop();
}

static void settings_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	const char *property = FOCUSER_BACKLASH_PROPERTY_NAME, *item = FOCUSER_BACKLASH_ITEM_NAME, *command = "B";
	if (!strcmp(current_profile, "limits_failure")) {
		property = FOCUSER_LIMITS_PROPERTY_NAME;
		item = FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME;
		command = "G";
	}
	SERIAL_CHECK_TRUE(fault(command, "mismatch"));
	SERIAL_CHECK_TRUE(number_change(property, item, command[0] == 'B' ? 20 : 1000, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(property, item)->number.value == (command[0] == 'B' ? 3 : 250000));
	SERIAL_CHECK_TRUE(number_change(property, item, command[0] == 'B' ? 20 : 1000, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void reverse_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("R", "mismatch"));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void sync_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(fault("P", "malformed"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.value == 0);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void poll_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	unsigned before = atomic_load(&revisions[1]);
	SERIAL_CHECK_TRUE(fault("q", current_profile));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_POSITION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 600, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(600));
cleanup:
	driver_stop();
}

static void temperature_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, INDIGO_OK_STATE));
	unsigned before = atomic_load(&revisions[7]);
	SERIAL_CHECK_TRUE(fault("temperature", "99.9"));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME)->number.value == 23.5);
	before = atomic_load(&revisions[7]);
	SERIAL_CHECK_TRUE(fault("temperature", "-5"));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, before, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, -5, .01));
	before = atomic_load(&revisions[7]);
	SERIAL_CHECK_TRUE(fault("t", "malformed"));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, before, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void external_position(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("external", "750"));
	SERIAL_CHECK_TRUE(at_position(750));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.target == 750);
cleanup:
	driver_stop();
}

static void stop_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10000, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(fault("H", "reject"));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void pending_disconnect(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10000, INDIGO_BUSY_STATE));
	if (!strcmp(current_profile, "disconnect_read")) {
		SERIAL_CHECK_TRUE(fault("q", "silent"));
		indigo_usleep(200000);
	}
	disconnect_serial_device(&lacerta_focuser);
	SERIAL_CHECK_TRUE(!context.connected);
	int before = commands(": ");
	indigo_usleep(200000);
	SERIAL_CHECK_TRUE(commands(": ") == before);
	SERIAL_CHECK_TRUE(connect_serial_device(&lacerta_focuser, fixture.port));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void transport_loss(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("M", "close"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10000, INDIGO_ALERT_STATE));
cleanup:
	driver_stop();
}

static void reconnect(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&lacerta_focuser));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, lacerta_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, "/dev/indigo-lacerta-nonexistent"));
	SERIAL_CHECK_TRUE(switch_change(CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true, INDIGO_ALERT_STATE));
	for (int i = 0; i < 3; i++) {
		SERIAL_CHECK_TRUE(connect_serial_device(&lacerta_focuser, fixture.port));
		if (i > 0) {
			SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME)->number.value == 25);
			SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME)->number.value == 5000);
			SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME)->sw.value);
		}
		SERIAL_CHECK_TRUE(sync_to(500 + i * 10));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 25, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 5000, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true, INDIGO_OK_STATE));
		disconnect_serial_device(&lacerta_focuser);
		SERIAL_CHECK_TRUE(!context.connected);
	}
cleanup:
	driver_stop();
}

static void motion_poll_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10000, INDIGO_BUSY_STATE));
	for (int i = 0; i < 60 && commands(": M 10000#") == 0; i++) {
		indigo_usleep(50000);
	}
	unsigned before = atomic_load(&revisions[1]);
	SERIAL_CHECK_TRUE(fault("q", "malformed"));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_POSITION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands(": H #") > 0);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 700, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void stalled_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("M", "silent"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_BUSY_STATE));
	bool stopped = false;
	for (int i = 0; i < 180; i++) {
		if (find_cached_property(FOCUSER_POSITION_PROPERTY_NAME)->state == INDIGO_ALERT_STATE) {
			stopped = true;
			break;
		}
		indigo_usleep(100000);
	}
	SERIAL_CHECK_TRUE(stopped && commands(": H #") > 0);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void sensor_absent(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME)->number.value != 99.9);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 400, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void settings_during_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10000, INDIGO_BUSY_STATE));
	for (int i = 0; i < 60 && commands(": M 10000#") == 0; i++) {
		indigo_usleep(50000);
	}
	SERIAL_CHECK_TRUE(number_change(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 10, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 5000, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands(": B ") == 0 && commands(": G ") == 0 && commands(": R ") == 0);
	SERIAL_CHECK_TRUE(!find_cached_item(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME)->sw.value);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(sync_to(1000));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 500, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands(": G ") == 0);
cleanup:
	driver_stop();
}

static void instances(void) {
	static const simulator_driver_case second = { "LACERTA Motorfocus Focuser", "indigo_focuser_lacerta", "LACERTA Motorfocus #2", indigo_focuser_lacerta, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1, .01));
	SERIAL_CHECK_TRUE(connect_serial_device(&second, second_fixture.port));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, -5, .01));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, .01));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, second.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(at_position(1000));
	disconnect_serial_device(&lacerta_focuser);
	reset_simulator_context(&second);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, second.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 700));
	SERIAL_CHECK_TRUE(at_position(700));
	SERIAL_CHECK_TRUE(connect_serial_device(&lacerta_focuser, fixture.port));
	SERIAL_CHECK_TRUE(at_position(0));
cleanup:
	disconnect_serial_device(&second);
	disconnect_serial_device(&lacerta_focuser);
	indigo_change_number_property_1(&simulator_test_client, lacerta_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 0);
	driver_stop();
}

int main(void) {
	simulator_test_client.update_property = observe_update;
	const lacerta_test tests[] = {
		{ "simulator_mfoc", lacerta_simulator_mfoc_protocol, "normal" },
		{ "simulator_fmc", lacerta_simulator_fmc_protocol, "fmc" },
		{ "normal", lacerta_focuser_passes_serial_compliance_checks, "normal" },
		{ "noop", noop_motion, "normal" },
		{ "capabilities_mfoc", capabilities, "normal" },
		{ "capabilities_fmc", capabilities, "fmc" },
		{ "capabilities_mfoc2", capabilities, "mfoc2" },
		{ "debug", capabilities, "debug" },
		{ "split", capabilities, "split" },
		{ "relative", relative_motion, "normal" },
		{ "abort", abort_motion, "normal" },
		{ "overlap", overlap, "normal" },
		{ "backlash_failure", settings_failure, "normal" },
		{ "limits_failure", settings_failure, "limits_failure" },
		{ "reverse_failure", reverse_failure, "normal" },
		{ "sync_failure", sync_failure, "normal" },
		{ "poll_malformed", poll_failure, "malformed" },
		{ "poll_short", poll_failure, "short" },
		{ "poll_overlong", poll_failure, "overlong" },
		{ "poll_silent", poll_failure, "silent" },
		{ "poll_partial", poll_failure, "partial" },
		{ "poll_flood", poll_failure, "flood" },
		{ "temperature", temperature_failure, "normal" },
		{ "external_position", external_position, "normal" },
		{ "stop_failure", stop_failure, "normal" },
		{ "disconnect_motion", pending_disconnect, "normal" },
		{ "disconnect_read", pending_disconnect, "disconnect_read" },
		{ "transport_loss", transport_loss, "normal" },
		{ "reconnect", reconnect, "normal" },
		{ "motion_poll_failure", motion_poll_failure, "normal" },
		{ "stalled_motion", stalled_motion, "normal" },
		{ "sensor_absent", sensor_absent, "nc" },
		{ "settings_during_motion", settings_during_motion, "normal" },
		{ "instances", instances, "normal" },
		{ "init_version", rejected_connection, "init_v_malformed" },
		{ "init_reverse", rejected_connection, "init_r_malformed" },
		{ "init_limits", rejected_connection, "init_g_malformed" },
		{ "init_position", rejected_connection, "init_q_malformed" },
		{ "init_backlash", rejected_connection, "init_b_malformed" },
		{ "unknown_identity", rejected_connection, "unknown" },
		{ "short_identity", rejected_connection, "init_i_short" },
		{ "overlong_identity", rejected_connection, "init_i_overlong" },
		{ "silent_identity", rejected_connection, "init_i_silent" },
	};
	return run_cases(tests, ARRAY_SIZE(tests));
}
