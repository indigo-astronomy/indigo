// Optec TCF-S focuser simulator
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

static unsigned position = 5000;
static unsigned target = 5000;
static unsigned slope_a = 86;
static char slope_a_sign = '0';
static bool manual_mode = false;
static bool quiet = false;

static void usage(const char *name) {
	printf("Optec TCF-S focuser simulator\n");
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

static int read_exact(int handle, char *buffer, int length) {
	int total_bytes = 0;
	while (running && total_bytes < length) {
		ssize_t bytes_read = read(handle, buffer + total_bytes, (size_t)(length - total_bytes));
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				return total_bytes;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return total_bytes;
		}
		total_bytes += (int)bytes_read;
	}
	return total_bytes;
}

static int read_optec_command(int handle, char *buffer, int length) {
	if (length < 7) {
		return -1;
	}
	int bytes = read_exact(handle, buffer, 6);
	if (bytes <= 0) {
		return bytes;
	}
	if (bytes != 6) {
		return 0;
	}
	buffer[6] = '\0';
	serial_simulator_trace_line(options.trace, "->", buffer);
	return bytes;
}

static void complete_motion_on_poll(void) {
	if (target != position) {
		position = target;
	}
}

static void dispatch_command(int handle, const char *command) {
	if (!strcmp(command, "FMMODE")) {
		manual_mode = true;
		sim_printf(handle, "!\n");
	} else if (!strcmp(command, "FFMODE")) {
		manual_mode = false;
		sim_printf(handle, "END\n");
	} else if (!strcmp(command, "FAMODE")) {
		manual_mode = false;
	} else if (!strcmp(command, "FBMODE")) {
		manual_mode = false;
	} else if (!strcmp(command, "FQUIT0")) {
		quiet = false;
		sim_printf(handle, "DONE\n");
	} else if (!strcmp(command, "FQUIT1")) {
		quiet = true;
		sim_printf(handle, "DONE\n");
	} else if (!strcmp(command, "FPOSRO")) {
		complete_motion_on_poll();
		sim_printf(handle, "P=%04u\n", position);
	} else if (!strcmp(command, "FTMPRO")) {
		sim_printf(handle, "T=+24.5\n");
	} else if (!strcmp(command, "FREADA")) {
		sim_printf(handle, "A=%04u\n", slope_a);
	} else if (!strcmp(command, "FTxxxA")) {
		sim_printf(handle, "A=%c\n", slope_a_sign);
	} else if (!strncmp(command, "FI", 2)) {
		unsigned steps = (unsigned)atoi(command + 2);
		target = position > steps ? position - steps : 0;
		sim_printf(handle, "*\n");
	} else if (!strncmp(command, "FO", 2)) {
		target = position + (unsigned)atoi(command + 2);
		sim_printf(handle, "*\n");
	} else if (!strncmp(command, "FLA", 3)) {
		slope_a = (unsigned)atoi(command + 3);
		sim_printf(handle, "DONE\n");
	} else if (!strncmp(command, "FZAxx", 5)) {
		slope_a_sign = command[5];
		sim_printf(handle, "DONE\n");
	} else if (!strcmp(command, "FCENTR")) {
		position = target = 5000;
		sim_printf(handle, "CENTER\n");
	} else if (!strcmp(command, "FSLEEP")) {
		sim_printf(handle, "ZZZ\n");
	} else if (!strcmp(command, "FWAKUP")) {
		sim_printf(handle, "WAKE\n");
	}
	(void)manual_mode;
	(void)quiet;
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[16];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_optec_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("Optec TCF-S focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = read_optec_command(serial_fd, buffer, sizeof(buffer));
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
