// iOptron iEAF focuser simulator
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

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static int position = 1000;
static int target_position = 1000;
static int moving_reports = 0;
static int reversed = 0;
static int temperature = 29465;

static void usage(const char *name) {
	printf("iOptron iEAF focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  -h, --help              Show this help and exit\n");
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

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

static int sim_read_command(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;
	bool in_frame = false;

	while (running && total_bytes < length - 1) {
		ssize_t bytes_read = read(handle, &c, 1);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			}
			if (errno == EIO) {
				return 0;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return 0;
		}
		if (!in_frame) {
			if (c != ':') {
				continue;
			}
			in_frame = true;
		}
		buffer[total_bytes++] = c;
		if (c == '#') {
			break;
		}
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
	if (!strcmp(command, ":DeviceInfo#")) {
		sim_printf(handle, "%06d%02d%04d#", position, 2, 4);
	} else if (!strcmp(command, ":FI#")) {
		int moving = moving_reports > 0 ? 1 : 0;
		if (moving_reports > 0) {
			moving_reports--;
			if (moving_reports == 0) {
				position = target_position;
			}
		}
		sim_printf(handle, "%07d%d%05d%d#", position, moving, temperature, reversed ? 0 : 1);
	} else if (!strncmp(command, ":FM", 3)) {
		target_position = atoi(command + 3);
		if (target_position < 0) {
			target_position = 0;
		} else if (target_position > 99999) {
			target_position = 99999;
		}
		moving_reports = 1;
	} else if (!strcmp(command, ":FR#")) {
		reversed = !reversed;
	} else if (!strcmp(command, ":FZ#")) {
		position = target_position = 0;
		moving_reports = 0;
	} else if (!strcmp(command, ":FQ#")) {
		target_position = position;
		moving_reports = 0;
	}
}

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

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_ioptron_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("iOptron iEAF focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = sim_read_command(serial_fd, buffer, sizeof(buffer));
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
