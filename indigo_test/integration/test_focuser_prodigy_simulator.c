// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/focuser_prodigy/indigo_focuser_prodigy.h>

#include "serial_simulator_test_common.h"
#include <indigo/indigo_uni_io.h>
#include <errno.h>
#include <stdatomic.h>
#include <fcntl.h>

#ifndef FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE
#define FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE "build/integration/focuser_prodigy_simulator"
#endif

static const simulator_driver_case prodigy_focuser = {
	"PegasusAstro Prodigy Microfocuser",
	"indigo_focuser_prodigy",
	"Pegasus Prodigy Focuser",
	indigo_focuser_prodigy,
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

static const simulator_driver_case prodigy_powerbox = { "Pegasus Prodigy Powerbox", "indigo_focuser_prodigy", "Pegasus Prodigy Powerbox", indigo_focuser_prodigy, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };

static external_serial_simulator fixture, second_fixture;
static char fixture_dir[] = "/tmp/indigo-prodigy.XXXXXX";
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
	char line[512];
	int count = 0;
	while (fgets(line, sizeof(line), file)) {
		if (!strncmp(line, prefix, strlen(prefix))) {
			count++;
		}
	}
	fclose(file);
	return count;
}

static void driver_stop(void) {
	disconnect_serial_device(&prodigy_powerbox);
	disconnect_serial_device(&prodigy_focuser);
	bool disconnected = !context.connected;
	indigo_result result = indigo_focuser_prodigy(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	ASSERT_TRUE(disconnected);
	ASSERT_EQ_INT(INDIGO_OK, result);
}

static bool driver_start(void) {
	return bring_up_serial_driver(&prodigy_focuser) && connect_serial_device(&prodigy_focuser, fixture.port);
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
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&prodigy_focuser));
	int descriptors = open_descriptors();
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, prodigy_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, fixture.port));
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	SERIAL_CHECK_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(!context.connected);
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	if (!strncmp(current_profile, "init_", 5)) {
		SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	}
cleanup:
	driver_stop();
}

typedef struct { const char *name; void (*run)(void); const char *profile; } prodigy_test;

static int run_cases(const prodigy_test *cases, int count) {
	int failures = 0;
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *filter = getenv("PRODIGY_TEST_FILTER");
	if (!mkdtemp(fixture_dir)) {
		return 1;
	}
	snprintf(event_path, sizeof(event_path), "%s/events", fixture_dir);
	snprintf(fault_path, sizeof(fault_path), "%s/fault", fixture_dir);
	setenv("INDIGO_PRODIGY_EVENTS", event_path, 1);
	setenv("INDIGO_PRODIGY_FAULT", fault_path, 1);
	for (int i = 0; i < count; i++) {
		if (filter && !strstr(cases[i].name, filter)) {
			continue;
		}
		current_profile = cases[i].profile;
		unlink(fault_path);
		const char *args[] = { "--profile", current_profile, NULL };
		if (!start_external_serial_simulator_with_args(&fixture, FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE, args)) {
			failures++;
			break;
		}
		if (!strcmp(cases[i].name, "instances")) {
			unsetenv("INDIGO_PRODIGY_EVENTS");
			unsetenv("INDIGO_PRODIGY_FAULT");
			const char *second_args[] = { "--profile", "alternate", NULL };
			bool ready = start_external_serial_simulator_with_args(&second_fixture, FOCUSER_PRODIGY_SIMULATOR_EXECUTABLE, second_args);
			setenv("INDIGO_PRODIGY_EVENTS", event_path, 1);
			setenv("INDIGO_PRODIGY_FAULT", fault_path, 1);
			if (!ready) {
				stop_external_serial_simulator(&fixture);
				failures++;
				break;
			}
		}
		fflush(NULL);
		pid_t child = fork();
		if (child == 0) {
			alarm(45);
			indigo_test_case test = { cases[i].name, cases[i].run };
			_exit(indigo_run_tests("PRODIGY", &test, 1));
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
	unsetenv("INDIGO_PRODIGY_EVENTS");
	unsetenv("INDIGO_PRODIGY_FAULT");
	printf("PRODIGY: %d failing scenarios\n", failures);
	return failures ? 1 : 0;
}

static const char *observed_names[] = { CONNECTION_PROPERTY_NAME, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_ABORT_MOTION_PROPERTY_NAME, "X_FOCUSER_PARK", "X_AUX_REBOOT", FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_BACKLASH_PROPERTY_NAME, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_USB_PORT_PROPERTY_NAME, AUX_OUTLET_NAMES_PROPERTY_NAME };
static atomic_uint revisions[13], motion_busy;

static int observed_index(const char *name) {
	for (int i = 0; i < 13; i++) {
		if (!strcmp(name, observed_names[i])) {
			return i;
		}
	}
	return -1;
}

static indigo_result observe_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_result result = simulator_client_update_property(client, device, property, message);
	int i = context.driver_case && !strcmp(property->device, context.driver_case->device_name) ? observed_index(property->name) : -1;
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
	for (int i = 0; i < 400; i++) {
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
	return indigo_change_number_property_1(&simulator_test_client, context.driver_case->device_name, property, item, value) == INDIGO_OK && new_state(property, before, state);
}

static bool switch_change(const char *property, const char *item, bool value, indigo_property_state state) {
	int index = observed_index(property);
	unsigned before = index >= 0 ? atomic_load(&revisions[index]) : 0;
	if (indigo_change_switch_property_1(&simulator_test_client, context.driver_case->device_name, property, item, value) != INDIGO_OK) {
		return false;
	}
	return index >= 0 ? new_state(property, before, state) : wait_for_property_state(property, state);
}

static bool at_position(int position) {
	return wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, .01) && wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE) && wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE);
}

static bool switch_reset(const char *property, const char *item) {
	for (int i = 0; i < 100; i++) {
		indigo_item *value = find_cached_item(property, item);
		if (value && !value->sw.value) {
			return true;
		}
		indigo_usleep(20000);
	}
	return false;
}

static bool abort_ok(void) {
	return switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_OK_STATE);
}

static bool sync_to(int position) {
	return switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true, INDIGO_OK_STATE) && number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position, INDIGO_OK_STATE) && switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true, INDIGO_OK_STATE);
}

static void capabilities(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&prodigy_focuser));
	SERIAL_CHECK_TRUE(!has_defined_property("X_FOCUSER_PARK"));
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_serial_focuser_class_property_completeness();
	SERIAL_CHECK_TRUE(has_defined_property("X_FOCUSER_PARK"));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_REVERSE_MOTION_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(!has_defined_property(FOCUSER_COMPENSATION_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(at_position(50));
	SERIAL_CHECK_EQ_INT(400, (int)find_cached_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME)->number.value);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 500, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 25, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(1, commands("S:500"));
	SERIAL_CHECK_EQ_INT(1, commands("C:25"));
cleanup:
	driver_stop();
}

static void movement(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(sync_to(-100));
	SERIAL_CHECK_EQ_INT(0, commands("M:"));
	SERIAL_CHECK_EQ_INT(1, commands("W:-100"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, -100, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(0, commands("M:"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 0, INDIGO_OK_STATE));
	SERIAL_CHECK_EQ_INT(0, commands("G:"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 1000, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)->number.value != 1000);
	SERIAL_CHECK_TRUE(at_position(1000));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(1100));
	SERIAL_CHECK_EQ_INT(1, commands("G:100"));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 200, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(900));
	SERIAL_CHECK_EQ_INT(1, commands("G:-200"));
cleanup:
	driver_stop();
}

static void limits(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME, -100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, 100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, -100, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(switch_change(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 200, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(100));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 200, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(100));
	SERIAL_CHECK_EQ_INT(1, commands("G:"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME, 100, INDIGO_ALERT_STATE));
	SERIAL_CHECK_EQ_INT(-100, (int)find_cached_item(FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME)->number.value);
cleanup:
	driver_stop();
}

static void motion_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("M:500", current_profile));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(commands("H") > 0);
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 600, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(at_position(600));
cleanup:
	driver_stop();
}

static void motion_read_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault(current_profile[0] == 'I' ? "I" : "P", current_profile + 2));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 600, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void abort_motion(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 900000, INDIGO_BUSY_STATE));
	if (!strcmp(current_profile, "stop_failure")) {
		SERIAL_CHECK_TRUE(fault("H", "reject"));
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true, INDIGO_ALERT_STATE));
	}
	SERIAL_CHECK_TRUE(abort_ok());
	indigo_item *position = find_cached_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	SERIAL_CHECK_TRUE(position->number.value == position->number.target && position->number.value != 900000);
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 600, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void park(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(sync_to(900000));
	if (!strcmp(current_profile, "park_failure")) {
		SERIAL_CHECK_TRUE(fault("Z", "reject"));
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_PARK", "PARK", true, INDIGO_ALERT_STATE));
		SERIAL_CHECK_TRUE(abort_ok());
	} else {
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_PARK", "PARK", true, INDIGO_BUSY_STATE));
		SERIAL_CHECK_TRUE(switch_reset("X_FOCUSER_PARK", "PARK"));
		if (!strcmp(current_profile, "park_abort")) {
			SERIAL_CHECK_TRUE(abort_ok());
			SERIAL_CHECK_TRUE(wait_for_property_state("X_FOCUSER_PARK", INDIGO_ALERT_STATE));
		} else {
			SERIAL_CHECK_TRUE(wait_for_property_state("X_FOCUSER_PARK", INDIGO_OK_STATE));
			SERIAL_CHECK_TRUE(at_position(0));
		}
	}
cleanup:
	driver_stop();
}

static void poll_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	const char *property = current_profile[0] == 'T' ? FOCUSER_TEMPERATURE_PROPERTY_NAME : FOCUSER_POSITION_PROPERTY_NAME;
	char command[2] = { current_profile[0], 0 };
	SERIAL_CHECK_TRUE(fault(command, current_profile + 2));
	unsigned before = atomic_load(&revisions[observed_index(property)]);
	SERIAL_CHECK_TRUE(new_state(property, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50, .01));
	before = atomic_load(&revisions[observed_index(property)]);
	SERIAL_CHECK_TRUE(new_state(property, before, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void settings_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (!strcmp(current_profile, "sync_failure")) {
		SERIAL_CHECK_TRUE(fault("W:100", "ignore"));
		SERIAL_CHECK_TRUE(switch_change(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, true, INDIGO_OK_STATE));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 100, INDIGO_ALERT_STATE));
		SERIAL_CHECK_TRUE(abort_ok());
	} else if (!strcmp(current_profile, "backlash_failure")) {
		SERIAL_CHECK_TRUE(fault("C:25", "reject"));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, 25, INDIGO_ALERT_STATE));
		SERIAL_CHECK_EQ_INT(100, (int)find_cached_item(FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME)->number.value);
	} else {
		SERIAL_CHECK_TRUE(fault(!strcmp(current_profile, "speed_readback") ? "B" : "S:500", "reject"));
		SERIAL_CHECK_TRUE(number_change(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 500, INDIGO_ALERT_STATE));
		SERIAL_CHECK_EQ_INT(400, (int)find_cached_item(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME)->number.value);
		SERIAL_CHECK_TRUE(number_change(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 500, INDIGO_OK_STATE));
	}
cleanup:
	driver_stop();
}

static bool aux_start(void) {
	return start_shared_serial_device(&prodigy_powerbox, prodigy_focuser.device_name, fixture.port);
}

static void powerbox(void) {
	SERIAL_CHECK_TRUE(aux_start());
	assert_device_interface(INDIGO_INTERFACE_AUX_POWERBOX);
	SERIAL_CHECK_TRUE(!has_defined_property(DEVICE_PORT_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(has_defined_property(AUX_OUTLET_NAMES_PROPERTY_NAME));
	SERIAL_CHECK_TRUE(switch_change(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(find_cached_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME)->sw.value);
	SERIAL_CHECK_TRUE(switch_change(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(switch_change(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(switch_change(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_2_ITEM_NAME, true, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(commands("X:1") > 0 && commands("Y:1") > 0 && commands("U:1") > 0 && commands("J:1") > 0);
	SERIAL_CHECK_TRUE(switch_change(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_1_ITEM_NAME, false, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(switch_change(AUX_USB_PORT_PROPERTY_NAME, AUX_USB_PORT_1_ITEM_NAME, false, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void power_failure(void) {
	SERIAL_CHECK_TRUE(aux_start());
	bool usb = current_profile[0] == 'J' || current_profile[0] == 'U';
	const char *property = usb ? AUX_USB_PORT_PROPERTY_NAME : AUX_POWER_OUTLET_PROPERTY_NAME;
	char command[4] = { current_profile[0], ':', '1', 0 };
	if (current_profile[0] == 'D') {
		strcpy(command, "D");
	}
	SERIAL_CHECK_TRUE(fault(command, current_profile + 2));
	const char *names[] = { usb ? AUX_USB_PORT_1_ITEM_NAME : AUX_POWER_OUTLET_1_ITEM_NAME, usb ? AUX_USB_PORT_2_ITEM_NAME : AUX_POWER_OUTLET_2_ITEM_NAME };
	bool values[] = { true, true };
	unsigned before = atomic_load(&revisions[observed_index(property)]);
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_switch_property(&simulator_test_client, prodigy_powerbox.device_name, property, 2, names, values));
	SERIAL_CHECK_TRUE(new_state(property, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_TRUE(switch_change(property, names[1], true, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void labels(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&prodigy_powerbox));
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(has_defined_property(AUX_OUTLET_NAMES_PROPERTY_NAME));
	unsigned before = atomic_load(&revisions[12]);
	SERIAL_CHECK_EQ_INT(INDIGO_OK, indigo_change_text_property_1_raw(&simulator_test_client, prodigy_powerbox.device_name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Camera"));
	SERIAL_CHECK_TRUE(new_state(AUX_OUTLET_NAMES_PROPERTY_NAME, before, INDIGO_OK_STATE));
	indigo_change_text_property_1_raw(&simulator_test_client, prodigy_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, fixture.port);
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_powerbox, NULL));
	SERIAL_CHECK_TRUE(!strcmp(find_cached_item(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME)->label, "Camera"));
cleanup:
	driver_stop();
}

static void shared(void) {
	SERIAL_CHECK_TRUE(aux_start());
	int descriptors = open_descriptors();
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	SERIAL_CHECK_EQ_INT(1, commands("#"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 900000, INDIGO_BUSY_STATE));
	disconnect_serial_device(&prodigy_powerbox);
	reset_simulator_context(&prodigy_focuser);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 600, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_powerbox, NULL));
	disconnect_serial_device(&prodigy_focuser);
	reset_simulator_context(&prodigy_powerbox);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(switch_change(AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWER_OUTLET_2_ITEM_NAME, true, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void reboot_case(void) {
	SERIAL_CHECK_TRUE(aux_start());
	if (!strcmp(current_profile, "reboot_timeout")) {
		SERIAL_CHECK_TRUE(fault("Q", "rebootstall"));
	}
	SERIAL_CHECK_TRUE(switch_change("X_AUX_REBOOT", "REBOOT", true, INDIGO_BUSY_STATE));
	SERIAL_CHECK_TRUE(switch_reset("X_AUX_REBOOT", "REBOOT"));
	unsigned before = atomic_load(&revisions[5]);
	SERIAL_CHECK_TRUE(new_state("X_AUX_REBOOT", before, !strcmp(current_profile, "reboot_timeout") ? INDIGO_ALERT_STATE : INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void external(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("P", "position=-50"));
	SERIAL_CHECK_TRUE(at_position(-50));
	SERIAL_CHECK_TRUE(fault("T", "temp=-12.5"));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, -12.5, .01));
	SERIAL_CHECK_TRUE(fault("T", "temp=0"));
	SERIAL_CHECK_TRUE(wait_for_number_item_value(FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_TEMPERATURE_ITEM_NAME, 0, .01));
cleanup:
	driver_stop();
}

static void lifecycle(void) {
	SERIAL_CHECK_TRUE(driver_start());
	if (!strcmp(current_profile, "disconnect_read")) {
		SERIAL_CHECK_TRUE(fault("T", "silent"));
		indigo_usleep(1100000);
	} else {
		SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 900000, INDIGO_BUSY_STATE));
	}
	disconnect_serial_device(&prodigy_focuser);
	int count = commands("");
	indigo_usleep(1200000);
	SERIAL_CHECK_EQ_INT(count, commands(""));
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 600, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static bool protocol_reply(indigo_uni_handle *handle, char *response, int capacity) {
	long count = indigo_uni_read_section2(handle, response, capacity - 1, "\n", "\r", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count <= 1 || response[count - 1] != '\n' || (long)strlen(response) != count) {
		return false;
	}
	response[count - 1] = 0;
	return true;
}

static void protocol(void) {
	indigo_uni_handle *handle = indigo_uni_open_serial_with_speed(fixture.port, 19200, INDIGO_LOG_DEBUG);
	char response[128];
	SERIAL_CHECK_TRUE(handle != NULL);
	SERIAL_CHECK_TRUE(indigo_uni_write(handle, "M:1000\n", 7) == 7);
	SERIAL_CHECK_TRUE(protocol_reply(handle, response, sizeof(response)) && !strcmp(response, "M:1000"));
	indigo_usleep(600000);
	SERIAL_CHECK_TRUE(indigo_uni_write(handle, "P\n", 2) == 2);
	SERIAL_CHECK_TRUE(protocol_reply(handle, response, sizeof(response)) && !strcmp(response, "1000"));
	SERIAL_CHECK_TRUE(indigo_uni_write(handle, "Y:1\n", 4) == 4);
	SERIAL_CHECK_TRUE(protocol_reply(handle, response, sizeof(response)) && !strcmp(response, "Y:1"));
	SERIAL_CHECK_TRUE(indigo_uni_write(handle, "D\n", 2) == 2);
	SERIAL_CHECK_TRUE(protocol_reply(handle, response, sizeof(response)) && !strcmp(response, "D:0:1:0:0"));
cleanup:
	indigo_uni_close(&handle);
}

static void overlap(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 900000, INDIGO_BUSY_STATE));
	indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 200);
	indigo_change_switch_property_1(&simulator_test_client, prodigy_focuser.device_name, "X_FOCUSER_PARK", "PARK", true);
	indigo_usleep(150000);
	SERIAL_CHECK_EQ_INT(0, commands("G:"));
	SERIAL_CHECK_EQ_INT(0, commands("Z"));
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 800000, INDIGO_BUSY_STATE));
	int before = commands("M:");
	indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 0);
	indigo_usleep(150000);
	SERIAL_CHECK_EQ_INT(before, commands("M:"));
	SERIAL_CHECK_TRUE(abort_ok());
cleanup:
	driver_stop();
}

static void shared_failure(void) {
	SERIAL_CHECK_TRUE(driver_start());
	int descriptors = open_descriptors();
	SERIAL_CHECK_TRUE(fault("D", "reply=D:0:2:0:0"));
	reset_simulator_context(&prodigy_powerbox);
	enumerate_simulator_device();
	unsigned before = atomic_load(&revisions[0]);
	indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(new_state(CONNECTION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	reset_simulator_context(&prodigy_focuser);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 500, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_powerbox, NULL));
	SERIAL_CHECK_EQ_INT(1, commands("#"));
cleanup:
	driver_stop();
}

static void reboot_busy(void) {
	SERIAL_CHECK_TRUE(aux_start());
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 900000, INDIGO_BUSY_STATE));
	reset_simulator_context(&prodigy_powerbox);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(switch_change("X_AUX_REBOOT", "REBOOT", true, INDIGO_ALERT_STATE));
	SERIAL_CHECK_EQ_INT(0, commands("Q"));
	reset_simulator_context(&prodigy_focuser);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(abort_ok());
cleanup:
	driver_stop();
}

static void disconnect_special(void) {
	bool restart = !strcmp(current_profile, "disconnect_reboot");
	if (restart) {
		SERIAL_CHECK_TRUE(aux_start());
		SERIAL_CHECK_TRUE(switch_change("X_AUX_REBOOT", "REBOOT", true, INDIGO_BUSY_STATE));
		disconnect_serial_device(&prodigy_powerbox);
	} else {
		SERIAL_CHECK_TRUE(driver_start());
		SERIAL_CHECK_TRUE(sync_to(900000));
		SERIAL_CHECK_TRUE(switch_change("X_FOCUSER_PARK", "PARK", true, INDIGO_BUSY_STATE));
		disconnect_serial_device(&prodigy_focuser);
	}
	int count = commands("");
	indigo_usleep(1500000);
	SERIAL_CHECK_EQ_INT(count, commands(""));
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	SERIAL_CHECK_TRUE(abort_ok());
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 300, INDIGO_OK_STATE));
cleanup:
	driver_stop();
}

static void instances(void) {
	static const simulator_driver_case second = { "Prodigy", "indigo_focuser_prodigy", "Pegasus Prodigy Focuser #2", indigo_focuser_prodigy, false, NULL, 0, NULL, 0, NULL, 0, NULL, 0 };
	SERIAL_CHECK_TRUE(driver_start());
	indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1);
	SERIAL_CHECK_TRUE(wait_for_number_item_value(ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 1, .01));
	SERIAL_CHECK_TRUE(connect_serial_device(&second, second_fixture.port));
	SERIAL_CHECK_TRUE(at_position(500));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 700, INDIGO_OK_STATE));
	disconnect_serial_device(&prodigy_focuser);
	reset_simulator_context(&second);
	enumerate_simulator_device();
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 800, INDIGO_OK_STATE));
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_focuser, fixture.port));
	SERIAL_CHECK_TRUE(at_position(50));
cleanup:
	disconnect_serial_device(&second);
	disconnect_serial_device(&prodigy_focuser);
	indigo_change_number_property_1(&simulator_test_client, prodigy_focuser.device_name, ADDITIONAL_INSTANCES_PROPERTY_NAME, ADDITIONAL_INSTANCES_COUNT_ITEM_NAME, 0);
	driver_stop();
}

static void rejected_aux(void) {
	SERIAL_CHECK_TRUE(bring_up_serial_driver(&prodigy_powerbox));
	int descriptors = open_descriptors();
	indigo_change_text_property_1_raw(&simulator_test_client, prodigy_focuser.device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, fixture.port);
	enumerate_simulator_device();
	unsigned before = atomic_load(&revisions[0]);
	indigo_change_switch_property_1(&simulator_test_client, prodigy_powerbox.device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
	SERIAL_CHECK_TRUE(new_state(CONNECTION_PROPERTY_NAME, before, INDIGO_ALERT_STATE));
	SERIAL_CHECK_EQ_INT(descriptors, open_descriptors());
	SERIAL_CHECK_TRUE(connect_serial_device(&prodigy_powerbox, NULL));
cleanup:
	driver_stop();
}

static void transport_loss(void) {
	SERIAL_CHECK_TRUE(driver_start());
	SERIAL_CHECK_TRUE(fault("P", "close"));
	SERIAL_CHECK_TRUE(number_change(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 50000, INDIGO_ALERT_STATE));
cleanup:
	driver_stop();
}

int main(void) {
	simulator_test_client.update_property = observe_update;
	const prodigy_test cases[] = {
		{ "protocol", protocol, "normal" },
		{ "init_ports", rejected_aux, "init_D_reply=D:0:2:0:0" },
		{ "transport_loss", transport_loss, "normal" },
		{ "overlap", overlap, "normal" },
		{ "shared_failure", shared_failure, "normal" },
		{ "reboot_busy", reboot_busy, "normal" },
		{ "disconnect_park", disconnect_special, "normal" },
		{ "disconnect_reboot", disconnect_special, "disconnect_reboot" },
		{ "instances", instances, "normal" },
		{ "capabilities", capabilities, "normal" },
		{ "split", capabilities, "split" },
		{ "firmware_minor", capabilities, "firmware_minor" },
		{ "movement", movement, "normal" },
		{ "limits", limits, "normal" },
		{ "abort", abort_motion, "normal" },
		{ "stop_failure", abort_motion, "stop_failure" },
		{ "park", park, "normal" },
		{ "park_abort", park, "park_abort" },
		{ "park_failure", park, "park_failure" },
		{ "start_failure", motion_failure, "reject" },
		{ "ignored_start", motion_failure, "ignore" },
		{ "stalled_motion", motion_failure, "stall" },
		{ "motion_read_failure", motion_read_failure, "P:silent" },
		{ "motion_badstate", motion_read_failure, "I:reply=2" },
		{ "sync_failure", settings_failure, "sync_failure" },
		{ "speed_failure", settings_failure, "speed_failure" },
		{ "speed_readback", settings_failure, "speed_readback" },
		{ "backlash_failure", settings_failure, "backlash_failure" },
		{ "poll_bad_position", poll_failure, "P:reply=garbage" },
		{ "poll_bad_state", poll_failure, "I:reply=2" },
		{ "poll_partial", poll_failure, "P:partial" },
		{ "poll_overlong", poll_failure, "P:overlong" },
		{ "poll_silent", poll_failure, "P:silent" },
		{ "poll_temperature_nan", poll_failure, "T:reply=nan" },
		{ "external", external, "normal" },
		{ "powerbox", powerbox, "normal" },
		{ "power_first_failure", power_failure, "X:reject" },
		{ "power_second_failure", power_failure, "Y:reject" },
		{ "usb_first_failure", power_failure, "U:reject" },
		{ "usb_second_failure", power_failure, "J:reject" },
		{ "ports_read_failure", power_failure, "D:reply=D:0:2:0:0" },
		{ "labels", labels, "normal" },
		{ "shared", shared, "normal" },
		{ "reboot", reboot_case, "normal" },
		{ "reboot_timeout", reboot_case, "reboot_timeout" },
		{ "disconnect_motion", lifecycle, "normal" },
		{ "disconnect_read", lifecycle, "disconnect_read" },
		{ "init_identity", rejected_connection, "init_#_reply=OK_OTHER" },
		{ "init_status", rejected_connection, "init_A_reply=OK_PRDG:1.4:1:22.4:50:0:0:0:0:bad" },
		{ "init_short", rejected_connection, "init_A_reply=OK_PRDG:1.4" },
		{ "init_overlong", rejected_connection, "init_A_overlong" },
		{ "init_silent", rejected_connection, "init_A_silent" },
		{ "init_speed", rejected_connection, "init_B_reply=B:0" }
	};
	return run_cases(cases, ARRAY_SIZE(cases));
}
