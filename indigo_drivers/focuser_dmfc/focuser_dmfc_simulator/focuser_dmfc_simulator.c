// PegasusAstro DMFC simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *profile;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.profile = "normal"
};

static void usage(const char *name) {
	printf("PegasusAstro DMFC simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --profile <name>        normal, configured, no-handshake, bad-status or\n");
	printf("                          external-motion, default is normal\n");
	printf("  -h, --help              Show this help and exit\n");
	printf("\n");
	printf("INDIGO_DMFC_EVENTS names a file receiving every accepted request, one per line.\n");
	printf("INDIGO_DMFC_FAULT names a file holding '<command prefix> <silent|garbage|close>'\n");
	printf("which is applied once to the next matching request and then removed.\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static int motor_mode = 0;
static double temperature = 22.4;
static int position = 50;
static serial_motion motion = { .position = 50, .target = 50 };
static int moving_status = 0;
static int led_status = 0;
static int reverse = 0;
static int disabled_encoder = 0;
static int backlash_value = 100;
static int speed = 400;
static FILE *events = NULL;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- runtime

static bool parse_args(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage(argv[0]);
			exit(0);
		} else if (!strcmp(argv[i], "--headless")) {
			options.headless = true;
			options.trace = false;
		} else if (!strcmp(argv[i], "--trace")) {
			options.trace = true;
		} else if (!strcmp(argv[i], "--ready-file")) {
			if (++i == argc) {
				fprintf(stderr, "--ready-file requires a path\n");
				return false;
			}
			options.ready_file = argv[i];
		} else if (!strcmp(argv[i], "--profile")) {
			if (++i == argc) {
				fprintf(stderr, "--profile requires a name\n");
				return false;
			}
			options.profile = argv[i];
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// The controller state a scenario needs is selected once at startup, so a
// connecting driver reads back exactly the configuration under test.
static void apply_profile(void) {
	if (!strcmp(options.profile, "configured")) {
		motor_mode = 1;
		led_status = 1;
		reverse = 1;
		disabled_encoder = 1;
		backlash_value = 42;
		temperature = -5.5;
		position = 1234;
		serial_motion_sync(&motion, position);
	} else if (!strcmp(options.profile, "external-motion")) {
		serial_motion_start(&motion, 3000, 500);
	}
}

// ----------------------------------------------------------------- protocol

static int sim_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;

	while (running && total_bytes < length - 1) {
		ssize_t bytes_read = read(handle, &c, 1);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (total_bytes == 0) {
					return 0;
				}
				usleep(1000);
				continue;
			}
			if (errno == EIO) {
				return 0;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return 0;
		}
		if (c == '\n') {
			break;
		}
		if (c == '\r') {
			continue;
		}
		buffer[total_bytes++] = c;
	}
	buffer[total_bytes] = '\0';
	if (*buffer) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return total_bytes;
}

static bool sim_printf(int handle, const char *format, ...) {
	char buffer[128];
	va_list args;

	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0) {
		return false;
	}
	if ((size_t)length >= sizeof(buffer)) {
		length = (int)sizeof(buffer) - 1;
	}

	if (options.trace) {
		fprintf(stderr, "<- %s", buffer);
	}
	return serial_simulator_write_all(handle, buffer, (size_t)length);
}

// One-shot fault injection. The control file names a command prefix and the
// way the next matching request has to misbehave, so a test can fail exactly
// one transaction without disturbing the rest of the session.
static const char *pending_fault(const char *command) {
	static char action[32];
	const char *path = getenv("INDIGO_DMFC_FAULT");
	if (path == NULL) {
		return NULL;
	}
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	char prefix[32] = { 0 };
	action[0] = '\0';
	bool matched = fscanf(file, "%31s %31s", prefix, action) == 2 && !strncmp(command, prefix, strlen(prefix));
	fclose(file);
	if (!matched) {
		return NULL;
	}
	unlink(path);
	return action;
}

static void dispatch_command(int handle, const char *command) {
	position = (int)serial_motion_update(&motion);
	moving_status = motion.duration > 0;
	if (events != NULL) {
		fprintf(events, "%s\n", command);
		fflush(events);
	}
	const char *fault = pending_fault(command);
	if (fault != NULL) {
		if (!strcmp(fault, "close")) {
			running = 0;
			if (serial_fd >= 0) {
				close(serial_fd);
				serial_fd = -1;
			}
			return;
		}
		if (!strcmp(fault, "silent")) {
			return;
		}
		if (!strcmp(fault, "garbage")) {
			sim_printf(handle, "ERR\n");
			return;
		}
	}
	if (!strcmp(command, "#")) {
		sim_printf(handle, strcmp(options.profile, "no-handshake") ? "OK_DMFCN\n" : "ERR\n");
	} else if (!strcmp(command, "V")) {
		sim_printf(handle, "2.6\n");
	} else if (!strcmp(command, "A")) {
		if (!strcmp(options.profile, "bad-status")) {
			sim_printf(handle, "ERR\n");
		} else {
			sim_printf(handle, "OK_DMFCN:2.6:%d:%.1f:%d:%d:%d:%d:%d:%d\n", motor_mode, temperature, position, moving_status, led_status, reverse, disabled_encoder, backlash_value);
		}
	} else if (!strcmp(command, "T")) {
		sim_printf(handle, "%.1f\n", temperature);
	} else if (!strcmp(command, "P")) {
		sim_printf(handle, "%d\n", position);
	} else if (!strcmp(command, "I")) {
		sim_printf(handle, "%d\n", moving_status);
	} else if (!strncmp(command, "G:", 2)) {
		serial_motion_start(&motion, position + atoi(command + 2), 1000);
	} else if (!strncmp(command, "M:", 2)) {
		serial_motion_start(&motion, atoi(command + 2), 1000);
	} else if (!strncmp(command, "W:", 2)) {
		serial_motion_sync(&motion, position = atoi(command + 2));
	} else if (!strcmp(command, "H")) {
		serial_motion_stop(&motion);
		moving_status = 0;
	} else if (!strncmp(command, "S:", 2)) {
		speed = atoi(command + 2);
		(void)speed;
	} else if (!strncmp(command, "C:", 2)) {
		backlash_value = atoi(command + 2);
	} else if (!strncmp(command, "N:", 2)) {
		reverse = atoi(command + 2);
		sim_printf(handle, "N:%d\n", reverse);
	} else if (!strncmp(command, "R:", 2)) {
		motor_mode = atoi(command + 2);
		sim_printf(handle, "%d\n", motor_mode);
	} else if (!strncmp(command, "E:", 2)) {
		disabled_encoder = atoi(command + 2);
		sim_printf(handle, "%s\n", command);
	} else if (!strncmp(command, "L:", 2)) {
		led_status = atoi(command + 2);
		sim_printf(handle, "L:%d\n", led_status);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[128];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	apply_profile();
	const char *journal = getenv("INDIGO_DMFC_EVENTS");
	events = journal == NULL ? NULL : fopen(journal, "w");

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_dmfc_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("PegasusAstro DMFC simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = sim_read_line(serial_fd, buffer, sizeof(buffer));
		if (bytes < 0) {
			break;
		}
		if (bytes > 0) {
			dispatch_command(serial_fd, buffer);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
	}
	if (events != NULL) {
		fclose(events);
	}
	return 0;
}
