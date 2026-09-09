// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_efa/indigo_focuser_efa.h>

#include "serial_simulator_test_common.h"
#include <indigo/indigo_uni_io.h>
#include <errno.h>
#include <stdatomic.h>
#include <fcntl.h>

#ifndef FOCUSER_EFA_SIMULATOR_EXECUTABLE
#define FOCUSER_EFA_SIMULATOR_EXECUTABLE "build/integration/focuser_efa_simulator"
#endif

static const simulator_driver_case efa_focuser = {
	"Celestron / PlaneWave EFA Focuser",
	"indigo_focuser_efa",
	"EFA Focuser",
	indigo_focuser_efa,
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
static char fixture_dir[] = "/tmp/indigo-efa.XXXXXX";
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
	disconnect_serial_device(&efa_focuser);
	bool disconnected = !context.connected;
	indigo_result result = indigo_focuser_efa(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	ASSERT_TRUE(disconnected);
	ASSERT_EQ_INT(INDIGO_OK, result);
}

static bool driver_start(void) {
	return bring_up_serial_driver(&efa_focuser) && connect_serial_device(&efa_focuser, fixture.port);
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
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&efa_focuser));
	int descriptors = open_descriptors();
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, efa_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, fixture.port));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, efa_focuser.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(!context.connected);
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	if (!strncmp(current_profile, "init_", 5)) {
		SERIAL_CHECK_TRUE(connect_serial_device(&efa_focuser, fixture.port));
	}
cleanup:
	driver_stop();
}

typedef struct { const char *name; void (*run)(void); const char *profile; } efa_test;

static int run_cases(const efa_test *cases, int count) {
	int failures = 0;
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *filter = getenv("EFA_TEST_FILTER");
	if (!mkdtemp(fixture_dir)) {
		return 1;
	}
	snprintf(event_path, sizeof(event_path), "%s/events", fixture_dir);
	snprintf(fault_path, sizeof(fault_path), "%s/fault", fixture_dir);
	setenv("INDIGO_EFA_EVENTS", event_path, 1);
	setenv("INDIGO_EFA_FAULT", fault_path, 1);
	for (int i = 0; i < count; i++) {
		if (filter && !strstr(cases[i].name, filter)) {
			continue;
		}
		current_profile = cases[i].profile;
		unlink(fault_path);
		const char *args[] = { "--profile", current_profile, NULL };
		if (!start_external_serial_simulator_with_args(&fixture, FOCUSER_EFA_SIMULATOR_EXECUTABLE, args)) {
			failures++;
			break;
		}
		if (!strcmp(cases[i].name, "instances")) {
			unsetenv("INDIGO_EFA_EVENTS");
			unsetenv("INDIGO_EFA_FAULT");
			const char *second_args[] = { "--profile", "alternate", NULL };
			bool ready = start_external_serial_simulator_with_args(&second_fixture, FOCUSER_EFA_SIMULATOR_EXECUTABLE, second_args);
			setenv("INDIGO_EFA_EVENTS", event_path, 1);
			setenv("INDIGO_EFA_FAULT", fault_path, 1);
			if (!ready) {
				stop_external_serial_simulator(&fixture);
				failures++;
				break;
			}
		}
		fflush(NULL);
		pid_t child = fork();
		if (child == 0) {
			alarm(!strcmp(cases[i].name, "calibration_timeout") ? 220 : 35);
			indigo_test_case test = { cases[i].name, cases[i].run };
			_exit(indigo_run_tests("EFA", &test, 1));
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
	unsetenv("INDIGO_EFA_EVENTS");
	unsetenv("INDIGO_EFA_FAULT");
	printf("EFA: %d failing scenarios\n", failures);
	return failures ? 1 : 0;
}

static const char *observed_names[] = { CONNECTION_PROPERTY_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_ABORT_MOTION_PROPERTY_NAME, "X_FOCUSER_CALIBRATION", "X_FOCUSER_FANS", FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_TEMPERATURE_PROPERTY_NAME };
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
	return indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, property, item, value) == INDIGO_OK && new_state(property, before, state);
}

static bool switch_change(const char *property, const char *item, bool value, indigo_property_state state) {
	int index = observed_index(property);
	unsigned before = index >= 0 ? atomic_load(&revisions[index]) : 0;
	if (indigo_change_switch_property_1(&simulator_test_client, efa_focuser.device_name, property, item, value) != INDIGO_OK) {
		return false;
	}
	return index >= 0 ? new_state(property, before, state) : wait_for_property_state(property, state);
}

static bool at_position(int position) {
	return wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, .01) && wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE) && wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE);
}

static bool abort_ok(void) {
	return switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE);
}

static bool sync_to(int position) {
	return switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true, INDIGO_OK_STATE) && number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, INDIGO_OK_STATE) && switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true, INDIGO_OK_STATE);
}

static bool is_celestron(void) {
	return !strcmp(current_profile, "celestron") || !strncmp(current_profile, "c_", 2);
}

static void capabilities(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&efa_focuser));
	SERIAL_CHECK_TRUE(!has_defined_property("X_FOCUSER_FANS") && !has_defined_property("X_FOCUSER_CALIBRATION"));
	SERIAL_CHECK_TRUE(connect_serial_device(&efa_focuser, fixture.port));
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	SERIAL_CHECK_TRUE(at_position(10));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_SPEED_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(has_defined_property("X_FOCUSER_CALIBRATION") == is_celestron());
	SERIAL_CHECK_TRUE(has_defined_property("X_FOCUSER_FANS") != is_celestron());
	SERIAL_CHECK_TRUE(has_defined_property(FOCUSER_ON_POSITION_SET_PROPERTY_NAME) != is_celestron());
	SERIAL_CHECK_TRUE(find_cached_property(FOCUSER_LIMITS_PROPERTY_NAME)->perm == (is_celestron() ? INDIGO_RO_PERM : INDIGO_RW_PERM));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.max == (is_celestron() ? 100000 : 3799422));
	SERIAL_CHECK_TRUE(!strcmp(find_cached_item(INFO_PROPERTY_NAME, INFO_DEVICE_MODEL_ITEM_NAME)->text.value, is_celestron() ? "Celestron Focus Motor" : "PlaneWave EFA"));
	if (!is_celestron()) {
		SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, 21.5, .01));
		SERIAL_CHECK_TRUE(sync_to(1000));
		SERIAL_CHECK_TRUE(commands("12 04 1000 3") == 1 && commands("12 17") == 0);
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_FANS", "ON", true, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_FANS", "OFF", true, INDIGO_OK_STATE));
	}
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1500, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(1500));
	SERIAL_CHECK_TRUE(commands(is_celestron() ? "12 02 1500 3" : "12 17 1500 3") == 1);
	SERIAL_CHECK_TRUE(atomic_load(&motion_busy) > 0);
cleanup:
	driver_stop();
}

static void movement(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(commands("12 17") == 0 && commands("12 02") == 0);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(1010));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(910));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 0, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(0));
cleanup:
	driver_stop();
}

static void long_move(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 200000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(200000));
	SERIAL_CHECK_TRUE(commands("12 24 9 1") == 1 && commands("12 24 0 1") == 1 && commands("12 17 200000 3") == 1);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 10, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(10));
	SERIAL_CHECK_TRUE(commands("12 25 9 1") == 1 && commands("12 24 0 1") == 2 && commands("12 17 10 3") == 1);
cleanup:
	driver_stop();
}

static void external_position(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("external", "-80"));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, -80, .01));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.target == -80);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(100));
cleanup:
	driver_stop();
}

static void limits(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 2000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.min == 100);
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.max == 2000);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME, 3000, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME)->number.value == 100);
	SERIAL_CHECK_TRUE(sync_to(1000));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1500, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(2000));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 1900, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(100));
cleanup:
	driver_stop();
}

static void abort_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, is_celestron() ? 90000 : 2000000, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(abort_ok());
	indigo_item *item = find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	SERIAL_CHECK_TRUE(item->number.value == item->number.target);
	SERIAL_CHECK_TRUE(!find_cached_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME)->sw.value);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void overlap(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 200000, INDIGO_BUSY_STATE));
	indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 500);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 1000, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(at_position(200000));
	SERIAL_CHECK_TRUE(commands("12 17") == 1);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 199000, INDIGO_BUSY_STATE));
	indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 3000);
	SERIAL_CHECK_TRUE(at_position(1000));
cleanup:
	driver_stop();
}

static void temperature(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, 21.5, .01));
	SERIAL_CHECK_TRUE(fault("temperature", "-80"));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, -5, .01));
	unsigned before = atomic_load(&revisions[7]);
	SERIAL_CHECK_TRUE(fault("temperature", "32639"));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME)->number.value == -5);
	SERIAL_CHECK_TRUE(fault("temperature", "0"));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, 0, .01));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void poll_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	unsigned before = atomic_load(&revisions[1]);
	SERIAL_CHECK_TRUE(fault("01", current_profile));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_POSITION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.value == 10);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void command_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (!strcmp(current_profile, "fans")) {
		SERIAL_CHECK_TRUE(fault("27", "reject"));
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_FANS", "ON", true, INDIGO_ALERT_STATE));
		SERIAL_CHECK_TRUE(!find_cached_item("X_FOCUSER_FANS", "ON")->sw.value);
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_FANS", "ON", true, INDIGO_OK_STATE));
	} else if (!strcmp(current_profile, "sync")) {
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(fault("04", "reject"));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_ALERT_STATE));
		SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.value == 10);
		SERIAL_CHECK_TRUE(abort_ok());
		SERIAL_CHECK_TRUE(sync_to(1000));
	} else if (!strcmp(current_profile, "stop")) {
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 2000000, INDIGO_BUSY_STATE));
		SERIAL_CHECK_TRUE(fault("24", "reject"));
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_ALERT_STATE));
		SERIAL_CHECK_TRUE(abort_ok());
	} else {
		SERIAL_CHECK_TRUE(fault("17", current_profile));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_ALERT_STATE));
		if (strcmp(current_profile, "close")) {
			SERIAL_CHECK_TRUE(abort_ok());
		}
	}
cleanup:
	driver_stop();
}

static void motion_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("17", "stall"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_BUSY_STATE));
	unsigned before = atomic_load(&revisions[1]);
	if (!strcmp(current_profile, "stall")) {
		indigo_usleep(4000000);
	} else {
		SERIAL_CHECK_TRUE(fault(!strcmp(current_profile, "badstate") ? "13" : "01", current_profile));
	}
	SERIAL_CHECK_TRUE(new_state(FOCUSER_POSITION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands("12 24 0 1") > 0);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void calibration(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (strcmp(current_profile, "celestron")) {
		if (!strcmp(current_profile, "c_readfail")) {
			SERIAL_CHECK_TRUE(fault("2B", "checksum"));
		} else if (!strcmp(current_profile, "c_limitsfail")) {
			SERIAL_CHECK_TRUE(fault("2C", "short"));
		} else {
			SERIAL_CHECK_TRUE(fault("2A", !strcmp(current_profile, "c_startfail") ? "reject" : "calfail"));
		}
	}
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_CALIBRATION", "CALIBRATE", true, strcmp(current_profile, "celestron") ? INDIGO_ALERT_STATE : INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(!find_cached_item("X_FOCUSER_CALIBRATION", "CALIBRATE")->sw.value);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_CALIBRATION", "CALIBRATE", true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME)->number.value == 100000);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void calibration_abort(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("2A", "calstall"));
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_CALIBRATION", "CALIBRATE", true, INDIGO_BUSY_STATE));
	indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(wait_for_property_state("X_FOCUSER_CALIBRATION", INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands("12 02") == 0);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void calibration_timeout(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("2A", "calstall"));
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_CALIBRATION", "CALIBRATE", true, INDIGO_BUSY_STATE));
	bool finished = false;
	for (int i = 0; i < 2000; i++) {
		if (find_cached_property("X_FOCUSER_CALIBRATION")->state == INDIGO_ALERT_STATE) {
			finished = true;
			break;
		}
		indigo_usleep(100000);
	}
	SERIAL_CHECK_TRUE(finished && commands("12 2A 0 1") > 0);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_CALIBRATION", "CALIBRATE", true, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void lifecycle(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (!strcmp(current_profile, "read")) {
		SERIAL_CHECK_TRUE(fault("01", "silent"));
		indigo_usleep(1100000);
	} else if (!strcmp(current_profile, "c_pending")) {
		SERIAL_CHECK_TRUE(fault("2A", "calstall"));
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_CALIBRATION", "CALIBRATE", true, INDIGO_BUSY_STATE));
	} else {
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 2000000, INDIGO_BUSY_STATE));
	}
	disconnect_serial_device(&efa_focuser);
	SERIAL_CHECK_TRUE(!context.connected);
	int count = commands("");
	indigo_usleep(1200000);
	SERIAL_CHECK_EQ_INT(count, commands(""));
	SERIAL_CHECK_TRUE(connect_serial_device(&efa_focuser, fixture.port));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void instances(void) {
	static const simulator_driver_case second = { "Celestron / PlaneWave EFA Focuser", "indigo_focuser_efa", "EFA Focuser #2", indigo_focuser_efa, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
	SERIAL_CHECK_TRUE(driver_start());
	indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1, .01));
	SERIAL_CHECK_TRUE(connect_serial_device(&second, second_fixture.port));
	SERIAL_CHECK_TRUE(at_position(500));
	SERIAL_CHECK_TRUE(has_defined_property("X_FOCUSER_CALIBRATION") && !has_defined_property("X_FOCUSER_FANS"));
	disconnect_serial_device(&efa_focuser);
	reset_simulator_context(&second);
	enumerate_simulator_device();
	indigo_change_number_property_1(&simulator_test_client, second.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000);
	SERIAL_CHECK_TRUE(at_position(1000));
	SERIAL_CHECK_TRUE(connect_serial_device(&efa_focuser, fixture.port));
	SERIAL_CHECK_TRUE(at_position(10));
	SERIAL_CHECK_TRUE(has_defined_property("X_FOCUSER_FANS") && !has_defined_property("X_FOCUSER_CALIBRATION"));
cleanup:
	disconnect_serial_device(&second);
	disconnect_serial_device(&efa_focuser);
	indigo_change_number_property_1(&simulator_test_client, efa_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 0);
	driver_stop();
}
static int exchange(indigo_uni_handle *handle, uint8_t destination, uint8_t command, int value, int bytes, uint8_t *reply) {
	uint8_t out[16] = { 0x3B, (uint8_t)(bytes + 3), 0x20, destination, command };
	for (int i = bytes - 1; i >= 0; i--) {
		out[5 + i] = (uint8_t)value;
		value >>= 8;
	}
	unsigned sum = 0;
	for (int i = 1; i < bytes + 5; i++) {
		sum += out[i];
	}
	out[bytes + 5] = (uint8_t)(0u - sum);
	if (indigo_uni_write(handle, (char *)out, bytes + 6) != bytes + 6) {
		return -1;
	}
	uint8_t in[32];
	for (int i = 0; i < 2; i++) {
		if (indigo_uni_wait_for_data(handle, INDIGO_DELAY(2)) <= 0 || indigo_uni_read_available(handle, in + i, 1) != 1) {
			return -1;
		}
	}
	if (in[0] != 0x3B || in[1] < 3 || in[1] > 29) {
		return -1;
	}
	for (int i = 2; i < in[1] + 3; i++) {
		if (indigo_uni_wait_for_data(handle, INDIGO_DELAY(2)) <= 0 || indigo_uni_read_available(handle, in + i, 1) != 1) {
			return -1;
		}
	}
	sum = 0;
	for (int i = 1; i < in[1] + 3; i++) {
		sum += in[i];
	}
	if ((sum & 255) || in[2] != destination || in[3] != 0x20 || in[4] != command) {
		return -1;
	}
	memcpy(reply, in + 5, (size_t)in[1] - 3);
	return in[1] - 3;
}

static int sim_position(indigo_uni_handle *handle) {
	uint8_t reply[32];
	if (exchange(handle, 0x12, 0x01, 0, 0, reply) != 3) {
		return -1;
	}
	return reply[0] * 65536 + reply[1] * 256 + reply[2];
}

static void simulator_protocol(void) {
	indigo_uni_handle *handle = indigo_uni_open_serial_with_speed(fixture.port, 19200, INDIGO_LOG_DEBUG);
	uint8_t reply[32];
	SERIAL_CHECK_TRUE(handle != NULL);
	SERIAL_CHECK_EQ_INT(-1, indigo_uni_get_cts(NULL));
	// A PTY may not support CTS; probing must leave subsequent data I/O usable.
	indigo_uni_get_cts(handle);
	SERIAL_CHECK_EQ_INT(is_celestron() ? 4 : 2, exchange(handle, 0x12, 0xFE, 0, 0, reply));
	SERIAL_CHECK_EQ_INT(10, sim_position(handle));
	SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x12, is_celestron() ? 0x02 : 0x17, 1000, 3, reply));
	indigo_usleep(600000);
	SERIAL_CHECK_EQ_INT(1000, sim_position(handle));
	SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x12, 0x13, 0, 0, reply));
	SERIAL_CHECK_EQ_INT(255, reply[0]);
	SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x12, 0x24, 9, 1, reply));
	indigo_usleep(200000);
	SERIAL_CHECK_TRUE(sim_position(handle) > 1000);
	SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x12, 0x24, 0, 1, reply));
	int stopped = sim_position(handle);
	indigo_usleep(200000);
	SERIAL_CHECK_EQ_INT(stopped, sim_position(handle));
	if (is_celestron()) {
		SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x12, 0x2A, 1, 1, reply));
		SERIAL_CHECK_EQ_INT(2, exchange(handle, 0x12, 0x2B, 0, 0, reply));
		SERIAL_CHECK_EQ_INT(0, reply[0]);
		indigo_usleep(1100000);
		SERIAL_CHECK_EQ_INT(2, exchange(handle, 0x12, 0x2B, 0, 0, reply));
		SERIAL_CHECK_EQ_INT(1, reply[0]);
		SERIAL_CHECK_EQ_INT(8, exchange(handle, 0x12, 0x2C, 0, 0, reply));
	} else {
		SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x12, 0x04, 100, 3, reply));
		SERIAL_CHECK_EQ_INT(100, sim_position(handle));
		SERIAL_CHECK_EQ_INT(3, exchange(handle, 0x12, 0x26, 0, 1, reply));
		SERIAL_CHECK_TRUE(reply[0] == 0 && reply[1] == 1 && reply[2] == 88);
		SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x13, 0x27, 1, 1, reply));
		SERIAL_CHECK_EQ_INT(1, exchange(handle, 0x13, 0x28, 0, 0, reply));
		SERIAL_CHECK_EQ_INT(0, reply[0]);
	}
cleanup:
	indigo_uni_close(&handle);
}

int main(void) {
	simulator_test_client.update_property = observe_update;
	const efa_test tests[] = {
		{ "simulator_efa", simulator_protocol, "normal" },
		{ "simulator_celestron", simulator_protocol, "celestron" },
		{ "normal", capabilities, "normal" },
		{ "uncalibrated_efa", capabilities, "uncalibrated" },
		{ "uncalibrated_celestron", capabilities, "c_uncalibrated" },
		{ "external_position", external_position, "normal" },
		{ "celestron", capabilities, "celestron" },
		{ "split", capabilities, "split" },
		{ "echo", capabilities, "echo" },
		{ "movement_efa", movement, "normal" },
		{ "movement_celestron", movement, "celestron" },
		{ "long_move", long_move, "normal" },
		{ "limits", limits, "normal" },
		{ "abort_efa", abort_motion, "normal" },
		{ "abort_celestron", abort_motion, "celestron" },
		{ "overlap", overlap, "normal" },
		{ "temperature", temperature, "normal" },
		{ "temperature_legacy", temperature, "legacy_temp" },
		{ "poll_checksum", poll_failure, "checksum" },
		{ "poll_short", poll_failure, "short" },
		{ "poll_overlong", poll_failure, "overlong" },
		{ "poll_partial", poll_failure, "partial" },
		{ "poll_silent", poll_failure, "silent" },
		{ "poll_wrongsrc", poll_failure, "wrongsrc" },
		{ "poll_wrongdst", poll_failure, "wrongdst" },
		{ "poll_wrongcmd", poll_failure, "wrongcmd" },
		{ "fans_failure", command_failure, "fans" },
		{ "sync_failure", command_failure, "sync" },
		{ "stop_failure", command_failure, "stop" },
		{ "start_failure", command_failure, "reject" },
		{ "transport_loss", command_failure, "close" },
		{ "motion_read_failure", motion_failure, "checksum" },
		{ "motion_badstate", motion_failure, "badstate" },
		{ "stalled_motion", motion_failure, "stall" },
		{ "calibration", calibration, "celestron" },
		{ "calibration_start_failure", calibration, "c_startfail" },
		{ "calibration_failed", calibration, "c_failed" },
		{ "calibration_read_failure", calibration, "c_readfail" },
		{ "calibration_limits_failure", calibration, "c_limitsfail" },
		{ "calibration_abort", calibration_abort, "celestron" },
		{ "calibration_timeout", calibration_timeout, "celestron" },
		{ "disconnect_motion", lifecycle, "normal" },
		{ "disconnect_read", lifecycle, "read" },
		{ "disconnect_calibration", lifecycle, "c_pending" },
		{ "instances", instances, "normal" },
		{ "unknown_identity", rejected_connection, "unknown" },
		{ "init_checksum", rejected_connection, "init_FE_checksum" },
		{ "init_overlong", rejected_connection, "init_FE_overlong" },
		{ "init_short", rejected_connection, "init_FE_short" },
		{ "init_silent", rejected_connection, "init_FE_silent" },
		{ "init_position", rejected_connection, "init_01_short" },
		{ "init_fans", rejected_connection, "init_28_short" },
		{ "init_calibration", rejected_connection, "init_30_short" },
		{ "init_stopdetect", rejected_connection, "init_EF_checksum" },
		{ "init_celestron_limits", rejected_connection, "c_init_2C_short" },
	};
	return run_cases(tests, ARRAY_SIZE(tests));
}
