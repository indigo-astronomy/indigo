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

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL
};

static void usage(const char *name) {
	printf("PegasusAstro DMFC simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static int motor_mode = 0;
static double temperature = 22.4;
static int position = 50;
static int moving_status = 0;
static int led_status = 0;
static int reverse = 0;
static int disabled_encoder = 0;
static int backlash_value = 100;
static int speed = 400;

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
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
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

static void dispatch_command(int handle, const char *command) {
	if (!strcmp(command, "#")) {
		sim_printf(handle, "OK_DMFCN\n");
	} else if (!strcmp(command, "V")) {
		sim_printf(handle, "2.6\n");
	} else if (!strcmp(command, "A")) {
		sim_printf(handle, "OK_DMFCN:2.6:%d:%.1f:%d:%d:%d:%d:%d:%d\n", motor_mode, temperature, position, moving_status, led_status, reverse, disabled_encoder, backlash_value);
	} else if (!strcmp(command, "T")) {
		sim_printf(handle, "%.1f\n", temperature);
	} else if (!strcmp(command, "P")) {
		sim_printf(handle, "%d\n", position);
	} else if (!strcmp(command, "I")) {
		sim_printf(handle, "%d\n", moving_status);
	} else if (!strncmp(command, "G:", 2)) {
		position += atoi(command + 2);
		moving_status = 0;
	} else if (!strncmp(command, "M:", 2)) {
		position = atoi(command + 2);
		moving_status = 0;
	} else if (!strncmp(command, "W:", 2)) {
		position = atoi(command + 2);
	} else if (!strcmp(command, "H")) {
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
	return 0;
}
