// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_ioptron/indigo_focuser_ioptron.h>

#include "serial_simulator_test_common.h"
#include <indigo/indigo_uni_io.h>
#include <errno.h>
#include <stdatomic.h>
#include <fcntl.h>

#ifndef FOCUSER_IOPTRON_SIMULATOR_EXECUTABLE
#define FOCUSER_IOPTRON_SIMULATOR_EXECUTABLE "build/integration/focuser_ioptron_simulator"
#endif

static const simulator_driver_case ioptron_focuser = {
	"iOptron iEAF Focuser",
	"indigo_focuser_ioptron",
	"iOptron iEAF",
	indigo_focuser_ioptron,
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
static char fixture_dir[] = "/tmp/indigo-ioptron.XXXXXX";
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
	disconnect_serial_device(&ioptron_focuser);
	bool disconnected = !context.connected;
	indigo_result result = indigo_focuser_ioptron(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	ASSERT_TRUE(disconnected);
	ASSERT_EQ_INT(INDIGO_OK, result);
}

static bool driver_start(void) {
	return bring_up_serial_driver(&ioptron_focuser) && connect_serial_device(&ioptron_focuser, fixture.port);
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
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&ioptron_focuser));
	int descriptors = open_descriptors();
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, ioptron_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, fixture.port));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ioptron_focuser.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(!context.connected);
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	if (!strncmp(current_profile, "init_", 5)) {
		SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_focuser, fixture.port));
	}
cleanup:
	driver_stop();
}

typedef struct { const char *name; void (*run)(void); const char *profile; } ioptron_test;

static int run_cases(const ioptron_test *cases, int count) {
	int failures = 0;
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *filter = getenv("IOPTRON_TEST_FILTER");
	if (!mkdtemp(fixture_dir)) {
		return 1;
	}
	snprintf(event_path, sizeof(event_path), "%s/events", fixture_dir);
	snprintf(fault_path, sizeof(fault_path), "%s/fault", fixture_dir);
	setenv("INDIGO_IOPTRON_EVENTS", event_path, 1);
	setenv("INDIGO_IOPTRON_FAULT", fault_path, 1);
	for (int i = 0; i < count; i++) {
		if (filter && !strstr(cases[i].name, filter)) {
			continue;
		}
		current_profile = cases[i].profile;
		unlink(fault_path);
		const char *args[] = { "--profile", current_profile, NULL };
		if (!start_external_serial_simulator_with_args(&fixture, FOCUSER_IOPTRON_SIMULATOR_EXECUTABLE, args)) {
			failures++;
			break;
		}
		if (!strcmp(cases[i].name, "instances")) {
			unsetenv("INDIGO_IOPTRON_EVENTS");
			unsetenv("INDIGO_IOPTRON_FAULT");
			const char *second_args[] = { "--profile", "alternate", NULL };
			bool ready = start_external_serial_simulator_with_args(&second_fixture, FOCUSER_IOPTRON_SIMULATOR_EXECUTABLE, second_args);
			setenv("INDIGO_IOPTRON_EVENTS", event_path, 1);
			setenv("INDIGO_IOPTRON_FAULT", fault_path, 1);
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
			_exit(indigo_run_tests("iOptron", &test, 1));
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
	unsetenv("INDIGO_IOPTRON_EVENTS");
	unsetenv("INDIGO_IOPTRON_FAULT");
	printf("iOptron: %d failing scenarios\n", failures);
	return failures ? 1 : 0;
}

static const char *observed_names[] = { CONNECTION_PROPERTY_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_ABORT_MOTION_PROPERTY_NAME, "X_FOCUSER_ZERO_SYNC", FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_TEMPERATURE_PROPERTY_NAME };
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
	return indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, property, item, value) == INDIGO_OK && new_state(property, before, state);
}

static bool switch_change(const char *property, const char *item, bool value, indigo_property_state state) {
	int index = observed_index(property);
	unsigned before = index >= 0 ? atomic_load(&revisions[index]) : 0;
	if (indigo_change_switch_property_1(&simulator_test_client, ioptron_focuser.device_name, property, item, value) != INDIGO_OK) {
		return false;
	}
	return index >= 0 ? new_state(property, before, state) : wait_for_property_state(property, state);
}

static bool at_position(int position) {
	return wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, .01) && wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE) && wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE);
}

static void instances(void) {
	static const simulator_driver_case second = { "iOptron iEAF Focuser", "indigo_focuser_ioptron", "iOptron iEAF #2", indigo_focuser_ioptron, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1, .01));
	SERIAL_CHECK_TRUE(connect_serial_device(&second, second_fixture.port));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, -5, .01));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, .01));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, second.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000));
	SERIAL_CHECK_TRUE(at_position(1000));
	disconnect_serial_device(&ioptron_focuser);
	reset_simulator_context(&second);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, second.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 700));
	SERIAL_CHECK_TRUE(at_position(700));
	SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_focuser, fixture.port));
	SERIAL_CHECK_TRUE(at_position(1000));
cleanup:
	disconnect_serial_device(&second);
	disconnect_serial_device(&ioptron_focuser);
	indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 0);
	driver_stop();
}

static void capabilities(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&ioptron_focuser));
	SERIAL_CHECK_TRUE(!has_defined_property("X_FOCUSER_ZERO_SYNC"));
	SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_focuser, fixture.port));
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	assert_property_has_item("X_FOCUSER_ZERO_SYNC", "SYNC");
	SERIAL_CHECK_TRUE(!has_defined_property("ZERO_SYNC"));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_SPEED_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_ON_POSITION_SET_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_BACKLASH_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.max == 99999);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, 21.5, .01));
	SERIAL_CHECK_TRUE(at_position(1000));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1250, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(1250));
	SERIAL_CHECK_TRUE(commands(":FM   1250#") == 1);
	SERIAL_CHECK_TRUE(atomic_load(&motion_busy) > 0);
cleanup:
	driver_stop();
}

static void movement(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(commands(":FM") == 0);
	for (int reversed = 0; reversed < 2; reversed++) {
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, reversed ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME : FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME, true, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 200, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(at_position(800));
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 200, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(at_position(1000));
	}
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 0, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(fault("external", "99990"));
	SERIAL_CHECK_TRUE(at_position(99990));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(99999));
	SERIAL_CHECK_TRUE(fault("external", "10"));
	SERIAL_CHECK_TRUE(at_position(10));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(0));
cleanup:
	driver_stop();
}

static void zero_sync(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_ZERO_SYNC", "SYNC", false, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(commands(":FZ#") == 0);
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_ZERO_SYNC", "SYNC", true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(0));
	SERIAL_CHECK_TRUE(!find_cached_item("X_FOCUSER_ZERO_SYNC", "SYNC")->sw.value);
	SERIAL_CHECK_TRUE(commands(":FM") == 0 && commands(":FZ#") == 1);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void abort_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	indigo_item *item = find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	SERIAL_CHECK_TRUE(item->number.value == item->number.target && item->number.value < 50000);
	SERIAL_CHECK_TRUE(!find_cached_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME)->sw.value);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void overlap(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 5000, INDIGO_BUSY_STATE));
	indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 6000);
	indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 500);
	SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_ZERO_SYNC", "SYNC", true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(at_position(5000));
	SERIAL_CHECK_TRUE(commands(":FM") == 1 && commands(":FZ#") == 0 && commands(":FR#") == 0);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 4000, INDIGO_BUSY_STATE));
	indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 6000);
	SERIAL_CHECK_TRUE(at_position(9000));
	SERIAL_CHECK_TRUE(commands(":FM") == 2);
cleanup:
	driver_stop();
}

static void poll_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	unsigned before = atomic_load(&revisions[1]);
	SERIAL_CHECK_TRUE(fault("I", current_profile));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_POSITION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void command_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	const char *property = "X_FOCUSER_ZERO_SYNC", *item = "SYNC", *command = "Z";
	if (!strcmp(current_profile, "reverse_failure")) {
		property = FOCUSER_REVERSE_MOTION_PROPERTY_NAME;
		item = FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME;
		command = "R";
	} else if (!strcmp(current_profile, "stop_failure")) {
		property = FOCUSER_ABORT_MOTION_PROPERTY_NAME;
		item = FOCUSER_ABORT_MOTION_ITEM_NAME;
		command = "Q";
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_BUSY_STATE));
	}
	SERIAL_CHECK_TRUE(fault(command, "ignore"));
	SERIAL_CHECK_TRUE(switch_change(property, item, true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(switch_change(property, item, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void start_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("M", current_profile));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 2000, INDIGO_ALERT_STATE));
	if (strcmp(current_profile, "close")) {
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
	}
cleanup:
	driver_stop();
}

static void temperature(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("temperature", "26815"));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, -5, .01));
	unsigned before = atomic_load(&revisions[7]);
	SERIAL_CHECK_TRUE(fault("temperature", "99999"));
	SERIAL_CHECK_TRUE(new_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME)->number.value == -5);
	SERIAL_CHECK_TRUE(fault("temperature", "29465"));
	SERIAL_CHECK_TRUE(wait_for_property_state(FOCUSER_TEMPERATURE_PROPERTY_NAME, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void reconnect(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&ioptron_focuser));
	indigo_change_text_property_1_raw(&simulator_test_client, ioptron_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, "/dev/indigo-ioptron-nonexistent");
	SERIAL_CHECK_TRUE(switch_change(CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true, INDIGO_ALERT_STATE));
	for (int i = 0; i < 3; i++) {
		SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_focuser, fixture.port));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500 + i * 50, INDIGO_OK_STATE));
		disconnect_serial_device(&ioptron_focuser);
		SERIAL_CHECK_TRUE(!context.connected);
	}
cleanup:
	driver_stop();
}

static void disconnect_pending(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (!strcmp(current_profile, "read")) {
		SERIAL_CHECK_TRUE(fault("I", "silent"));
		indigo_usleep(1100000);
	} else {
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_BUSY_STATE));
	}
	disconnect_serial_device(&ioptron_focuser);
	SERIAL_CHECK_TRUE(!context.connected);
	int count = commands(":");
	indigo_usleep(1200000);
	SERIAL_CHECK_EQ_INT(count, commands(":"));
	SERIAL_CHECK_TRUE(connect_serial_device(&ioptron_focuser, fixture.port));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static bool exchange(indigo_uni_handle *handle, const char *command, const char *expected) {
	if (indigo_uni_write(handle, command, (long)strlen(command)) != (long)strlen(command)) {
		return false;
	}
	if (!expected) {
		return true;
	}
	char response[64];
	return indigo_uni_read_section(handle, response, sizeof(response), "#", "#", INDIGO_DELAY(2)) > 0 && !strcmp(response, expected);
}

static void simulator_protocol(void) {
	indigo_uni_handle *handle = indigo_uni_open_serial_with_speed(fixture.port, 115200, INDIGO_LOG_DEBUG);
	SERIAL_CHECK_TRUE(handle != NULL);
	SERIAL_CHECK_TRUE(exchange(handle, ":DeviceInfo#", !strcmp(current_profile, "iafs") ? "001000030004" : "001000020004"));
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", "00010000294651"));
	SERIAL_CHECK_TRUE(exchange(handle, ":FR#", NULL));
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", "00010000294650"));
	SERIAL_CHECK_TRUE(exchange(handle, ":FM   5000#", NULL));
	indigo_usleep(300000);
	SERIAL_CHECK_TRUE(exchange(handle, ":FQ#", NULL));
	char response[64];
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", NULL));
	SERIAL_CHECK_TRUE(indigo_uni_read_section(handle, response, sizeof(response), "#", "#", INDIGO_DELAY(2)) > 0);
	int position = 0;
	SERIAL_CHECK_TRUE(sscanf(response, "%7d", &position) == 1 && position > 1000 && position < 5000 && response[7] == '0');
	indigo_usleep(200000);
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", response));
	SERIAL_CHECK_TRUE(exchange(handle, ":FM   5000#", NULL));
	SERIAL_CHECK_TRUE(exchange(handle, ":FZ#", NULL));
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", "00000000294650"));
	SERIAL_CHECK_TRUE(exchange(handle, ":FM      0#", NULL));
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", "00000000294650"));
	SERIAL_CHECK_TRUE(exchange(handle, ":FM     10#", NULL));
	indigo_usleep(600000);
	SERIAL_CHECK_TRUE(exchange(handle, ":FI#", "00000100294650"));
cleanup:
	indigo_uni_close(&handle);
}

static void readback_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	const char *command = "Z", *property = "X_FOCUSER_ZERO_SYNC", *item = "SYNC";
	if (!strcmp(current_profile, "reverse")) {
		command = "R";
		property = FOCUSER_REVERSE_MOTION_PROPERTY_NAME;
		item = FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME;
	} else if (!strcmp(current_profile, "abort")) {
		command = "Q";
		property = FOCUSER_ABORT_MOTION_PROPERTY_NAME;
		item = FOCUSER_ABORT_MOTION_ITEM_NAME;
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_BUSY_STATE));
	}
	SERIAL_CHECK_TRUE(fault(command, "readfail"));
	SERIAL_CHECK_TRUE(switch_change(property, item, true, INDIGO_ALERT_STATE));
	if (!strcmp(command, "Z")) {
		SERIAL_CHECK_TRUE(at_position(0));
	}
	SERIAL_CHECK_TRUE(switch_change(property, item, true, INDIGO_OK_STATE));
	if (!strcmp(command, "R")) {
		SERIAL_CHECK_TRUE(commands(":FR#") == 1);
	}
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void motion_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (!strcmp(current_profile, "stall")) {
		SERIAL_CHECK_TRUE(fault("M", "stall"));
	}
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_BUSY_STATE));
	unsigned before = atomic_load(&revisions[1]);
	if (strcmp(current_profile, "stall")) {
		SERIAL_CHECK_TRUE(fault("I", "malformed"));
	} else {
		indigo_usleep(4000000);
	}
	SERIAL_CHECK_TRUE(new_state(FOCUSER_POSITION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands(":FQ#") > 0);
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void pending_control(void) {
	SERIAL_CHECK_TRUE(driver_start());
	// The dispatch sets BUSY before queueing the command; both requests share the actual queue.
	SERIAL_CHECK_TRUE(fault("Z", "readfail"));
	indigo_change_switch_property_1(&simulator_test_client, ioptron_focuser.device_name, "X_FOCUSER_ZERO_SYNC", "SYNC", true);
	indigo_change_number_property_1(&simulator_test_client, ioptron_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 2000);
	SERIAL_CHECK_TRUE(at_position(0));
	SERIAL_CHECK_TRUE(commands(":FM") == 0);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

int main(void) {
	simulator_test_client.update_property = observe_update;
	const ioptron_test tests[] = {
		{ "simulator_ieaf", simulator_protocol, "normal" },
		{ "simulator_iafs", simulator_protocol, "iafs" },
		{ "zero_readback_failure", readback_failure, "zero" },
		{ "reverse_readback_failure", readback_failure, "reverse" },
		{ "abort_readback_failure", readback_failure, "abort" },
		{ "motion_read_failure", motion_failure, "normal" },
		{ "stalled_motion", motion_failure, "stall" },
		{ "pending_control", pending_control, "normal" },
		{ "normal", capabilities, "normal" },
		{ "iafs", capabilities, "iafs" },
		{ "split", capabilities, "split" },
		{ "movement", movement, "normal" },
		{ "zero", zero_sync, "normal" },
		{ "abort", abort_motion, "normal" },
		{ "overlap", overlap, "normal" },
		{ "poll_malformed", poll_failure, "malformed" },
		{ "poll_short", poll_failure, "short" },
		{ "poll_overlong", poll_failure, "overlong" },
		{ "poll_partial", poll_failure, "partial" },
		{ "poll_silent", poll_failure, "silent" },
		{ "poll_badflag", poll_failure, "badflag" },
		{ "poll_baddir", poll_failure, "baddir" },
		{ "poll_badpos", poll_failure, "badpos" },
		{ "poll_trailing", poll_failure, "trailing" },
		{ "zero_failure", command_failure, "zero_failure" },
		{ "reverse_failure", command_failure, "reverse_failure" },
		{ "stop_failure", command_failure, "stop_failure" },
		{ "start_failure", start_failure, "ignore" },
		{ "transport_loss", start_failure, "close" },
		{ "temperature", temperature, "normal" },
		{ "reconnect", reconnect, "normal" },
		{ "disconnect_motion", disconnect_pending, "normal" },
		{ "disconnect_read", disconnect_pending, "read" },
		{ "instances", instances, "normal" },
		{ "unknown_identity", rejected_connection, "unknown" },
		{ "init_identity_short", rejected_connection, "init_D_short" },
		{ "init_identity_overlong", rejected_connection, "init_D_overlong" },
		{ "init_identity_partial", rejected_connection, "init_D_partial" },
		{ "init_identity_silent", rejected_connection, "init_D_silent" },
		{ "init_status", rejected_connection, "init_I_malformed" },
	};
	return run_cases(tests, ARRAY_SIZE(tests));
}
