// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#ifndef serial_simulator_test_common_h
#define serial_simulator_test_common_h

#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "simulator_test_common.h"

#define SERIAL_CHECK_TRUE(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #condition); \
		indigo_test_failures++; \
		goto cleanup; \
	} \
} while (0)

#define SERIAL_CHECK_EQ_INT(expected, actual) do { \
	int expected_value = (expected); \
	int actual_value = (actual); \
	if (expected_value != actual_value) { \
		fprintf(stderr, "%s:%d: expected %d, got %d\n", __FILE__, __LINE__, expected_value, actual_value); \
		indigo_test_failures++; \
		goto cleanup; \
	} \
} while (0)

typedef struct {
	const char *executable;
	pid_t pid;
	char directory[PATH_MAX];
	char ready_file[PATH_MAX];
	char port[PATH_MAX];
} external_serial_simulator;

static void remove_simulator_directory(const char *directory) {
	if (*directory) {
		char ready_file[PATH_MAX];
		snprintf(ready_file, sizeof(ready_file), "%s/ready.env", directory);
		unlink(ready_file);
		rmdir(directory);
	}
}

static bool read_ready_file(external_serial_simulator *simulator) {
	static const char port_prefix[] = "INDIGO_SIMULATOR_PORT=";
	FILE *file = fopen(simulator->ready_file, "r");
	if (file == NULL) {
		return false;
	}

	char line[PATH_MAX + 64];
	while (fgets(line, sizeof(line), file) != NULL) {
		line[strcspn(line, "\r\n")] = '\0';
		if (!strncmp(line, port_prefix, sizeof(port_prefix) - 1)) {
			snprintf(simulator->port, sizeof(simulator->port), "%s", line + sizeof(port_prefix) - 1);
		}
	}
	fclose(file);
	return *simulator->port != '\0';
}

static bool start_external_serial_simulator_with_args(external_serial_simulator *simulator, const char *executable, const char * const *arguments) {
	memset(simulator, 0, sizeof(*simulator));
	simulator->executable = executable;
	snprintf(simulator->directory, sizeof(simulator->directory), "/tmp/indigo-serial-sim.XXXXXX");
	if (mkdtemp(simulator->directory) == NULL) {
		perror("mkdtemp");
		return false;
	}
	snprintf(simulator->ready_file, sizeof(simulator->ready_file), "%s/ready.env", simulator->directory);

	simulator->pid = fork();
	if (simulator->pid < 0) {
		perror("fork");
		remove_simulator_directory(simulator->directory);
		return false;
	}
	if (simulator->pid == 0) {
		const char *argv[32];
		int argc = 0;
		argv[argc++] = executable;
		argv[argc++] = "--headless";
		argv[argc++] = "--ready-file";
		argv[argc++] = simulator->ready_file;
		if (arguments != NULL) {
			while (*arguments != NULL && argc < (int)ARRAY_SIZE(argv) - 1) {
				argv[argc++] = *arguments++;
			}
		}
		argv[argc] = NULL;
		execv(executable, (char * const *)argv);
		perror("execv simulator");
		_exit(127);
	}

	for (int i = 0; i < 200; i++) {
		if (read_ready_file(simulator)) {
			return true;
		}
		indigo_usleep(25000);
	}

	fprintf(stderr, "Timed out waiting for %s ready file\n", executable);
	kill(simulator->pid, SIGTERM);
	waitpid(simulator->pid, NULL, 0);
	remove_simulator_directory(simulator->directory);
	memset(simulator, 0, sizeof(*simulator));
	return false;
}

static bool start_external_serial_simulator(external_serial_simulator *simulator, const char *executable) {
	return start_external_serial_simulator_with_args(simulator, executable, NULL);
}

static void stop_external_serial_simulator(external_serial_simulator *simulator) {
	if (simulator->pid > 0) {
		kill(simulator->pid, SIGTERM);
		waitpid(simulator->pid, NULL, 0);
		simulator->pid = 0;
	}
	remove_simulator_directory(simulator->directory);
	memset(simulator, 0, sizeof(*simulator));
}

// Bring the driver online (bus start, client attach, driver INIT) without
// connecting any device. This is the first step for multi-device drivers, so
// each exposed logical device can then be connected in turn over one
// simulator. Single-device tests use the start_serial_driver() wrapper below.
static bool bring_up_serial_driver(const simulator_driver_case *driver_case) {
	reset_simulator_context(driver_case);

	if (indigo_start() != INDIGO_OK) {
		return false;
	}
	if (indigo_attach_client(&simulator_test_client) != INDIGO_OK) {
		indigo_stop();
		return false;
	}
	if (driver_case->entry(INDIGO_DRIVER_INIT, NULL) != INDIGO_OK) {
		driver_case->entry(INDIGO_DRIVER_SHUTDOWN, NULL);
		indigo_detach_client(&simulator_test_client);
		indigo_stop();
		release_cached_properties();
		return false;
	}
	return true;
}

// Retarget the compliance context to one logical device of an already running
// driver and connect it. For a secondary device that shares its master's
// connection, connect the master/primary device first and pass port as NULL
// when the device does not expose its own DEVICE_PORT.
static bool connect_serial_device(const simulator_driver_case *device_case, const char *port) {
	if (context.driver_case != device_case) {
		reset_simulator_context(device_case);
	}
	enumerate_simulator_device();
	if (!has_defined_property(CONNECTION_PROPERTY_NAME)) {
		return false;
	}
	if (port != NULL && has_defined_property(DEVICE_PORT_PROPERTY_NAME)) {
		if (indigo_change_text_property_1_raw(&simulator_test_client, device_case->device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, port) != INDIGO_OK) {
			return false;
		}
		if (!wait_for_property_state(DEVICE_PORT_PROPERTY_NAME, INDIGO_OK_STATE)) {
			return false;
		}
	}
	if (indigo_change_switch_property_1(&simulator_test_client, device_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) != INDIGO_OK) {
		return false;
	}
	return wait_for_simulator_connection_state(true);
}

// Disconnect one logical device, leaving the driver running so other devices
// can still be exercised or the driver can be torn down afterwards. A
// redundant disconnect is ignored by the driver, so this is safe to call in
// cleanup for devices that may never have connected.
static void disconnect_serial_device(const simulator_driver_case *device_case) {
	if (context.driver_case != device_case) {
		reset_simulator_context(device_case);
	}
	indigo_change_switch_property_1(&simulator_test_client, device_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
	wait_for_simulator_connection_state(false);
}

// Tear the driver down (driver SHUTDOWN, client detach, bus stop). Every device
// exposed by the driver must already be disconnected.
static void tear_down_serial_driver(const simulator_driver_case *driver_case) {
	driver_case->entry(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
}

// Single-device convenience wrappers, used by the majority of simulator tests.
static bool start_serial_driver(const simulator_driver_case *driver_case, const char *port) {
	if (!bring_up_serial_driver(driver_case)) {
		return false;
	}
	if (!connect_serial_device(driver_case, port)) {
		tear_down_serial_driver(driver_case);
		return false;
	}
	return true;
}

static void stop_serial_driver(const simulator_driver_case *driver_case) {
	disconnect_serial_device(driver_case);
	tear_down_serial_driver(driver_case);
}

// Start the driver and connect a secondary device that shares its master's
// connection (its own DEVICE_PORT is hidden, so the master device's DEVICE_PORT
// is set to the simulator port before connecting). Each device of a
// multi-device driver is exercised in its own driver lifecycle, so pair this
// with stop_serial_driver(device_case) exactly like start_serial_driver.
static bool start_shared_serial_device(const simulator_driver_case *device_case, const char *master_device_name, const char *port) {
	if (!bring_up_serial_driver(device_case)) {
		return false;
	}
	if (indigo_change_text_property_1_raw(&simulator_test_client, master_device_name, DEVICE_PORT_PROPERTY_NAME, DEVICE_PORT_ITEM_NAME, port) != INDIGO_OK) {
		tear_down_serial_driver(device_case);
		return false;
	}
	if (!connect_serial_device(device_case, NULL)) {
		tear_down_serial_driver(device_case);
		return false;
	}
	return true;
}

static void assert_serial_focuser_class_property_completeness(void) {
	static const char *properties[] = {
		FOCUSER_DIRECTION_PROPERTY_NAME,
		FOCUSER_STEPS_PROPERTY_NAME,
		FOCUSER_ABORT_MOTION_PROPERTY_NAME,
		FOCUSER_POSITION_PROPERTY_NAME
	};
	assert_defined_properties(properties, ARRAY_SIZE(properties));
}

static void assert_serial_rotator_class_property_completeness(void) {
	static const char *properties[] = {
		ROTATOR_ON_POSITION_SET_PROPERTY_NAME,
		ROTATOR_POSITION_PROPERTY_NAME,
		ROTATOR_ABORT_MOTION_PROPERTY_NAME
	};
	assert_defined_properties(properties, ARRAY_SIZE(properties));
}

static void assert_serial_aux_class_property_completeness(void) {
	/* AUX has no base class properties beyond concrete driver-defined properties. */
}

static double bounded_number_value(const char *property_name, const char *item_name, double preferred_value) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		return NAN;
	}
	if (preferred_value < item->number.min) {
		return item->number.min;
	}
	if (preferred_value > item->number.max) {
		return item->number.max;
	}
	return preferred_value;
}

#endif /* serial_simulator_test_common_h */
