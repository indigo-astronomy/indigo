// myFocuserPro2 focuser simulator
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

static unsigned position = 1000;
static unsigned target_position = 1000;
static unsigned max_position = 100000;
static unsigned speed = 0;
static unsigned step_mode = 8;
static unsigned coils_mode = 0;
static unsigned settle_time = 100;
static unsigned backlash_in = 0;
static unsigned backlash_out = 0;
static bool backlash_in_enabled = false;
static bool backlash_out_enabled = false;
static bool reversed = false;
static bool moving = false;

static void usage(const char *name) {
	printf("myFocuserPro2 focuser simulator\n");
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
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
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
			continue;
		}
		if (c == '#') {
			break;
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

static unsigned parse_value(const char *text) {
	return (unsigned)strtoul(text, NULL, 10);
}

static void dispatch_command(int handle, const char *command) {
	if (!strcmp(command, "00")) {
		sim_printf(handle, "P%u#", position);
	} else if (!strcmp(command, "01")) {
		if (moving) {
			position = target_position;
			moving = false;
			sim_printf(handle, "I1#");
		} else {
			sim_printf(handle, "I0#");
		}
	} else if (!strcmp(command, "04")) {
		sim_printf(handle, "FmyFP2\n3.0\r#");
	} else if (!strncmp(command, "05", 2)) {
		target_position = parse_value(command + 2);
		if (target_position > max_position) {
			target_position = max_position;
		}
		moving = true;
	} else if (!strcmp(command, "06")) {
		sim_printf(handle, "Z22.5#");
	} else if (!strncmp(command, "07", 2)) {
		max_position = parse_value(command + 2);
		if (position > max_position) {
			position = max_position;
			target_position = position;
		}
	} else if (!strcmp(command, "08")) {
		sim_printf(handle, "M%u#", max_position);
	} else if (!strcmp(command, "11")) {
		sim_printf(handle, "O%u#", coils_mode);
	} else if (!strncmp(command, "12", 2)) {
		coils_mode = parse_value(command + 2) ? 1 : 0;
	} else if (!strcmp(command, "13")) {
		sim_printf(handle, "R%d#", reversed ? 1 : 0);
	} else if (!strncmp(command, "14", 2)) {
		reversed = parse_value(command + 2) != 0;
	} else if (!strncmp(command, "15", 2)) {
		speed = parse_value(command + 2);
	} else if (!strcmp(command, "27")) {
		moving = false;
		target_position = position;
	} else if (!strcmp(command, "29")) {
		sim_printf(handle, "S%u#", step_mode);
	} else if (!strncmp(command, "30", 2)) {
		step_mode = parse_value(command + 2);
	} else if (!strncmp(command, "31", 2)) {
		position = target_position = parse_value(command + 2);
		moving = false;
	} else if (!strcmp(command, "48")) {
	} else if (!strncmp(command, "71", 2)) {
		settle_time = parse_value(command + 2);
	} else if (!strcmp(command, "72")) {
		sim_printf(handle, "3%u#", settle_time);
	} else if (!strncmp(command, "73", 2)) {
		backlash_in_enabled = parse_value(command + 2) != 0;
	} else if (!strcmp(command, "74")) {
		sim_printf(handle, "4%d#", backlash_in_enabled ? 1 : 0);
	} else if (!strncmp(command, "75", 2)) {
		backlash_out_enabled = parse_value(command + 2) != 0;
	} else if (!strcmp(command, "76")) {
		sim_printf(handle, "5%d#", backlash_out_enabled ? 1 : 0);
	} else if (!strncmp(command, "77", 2)) {
		backlash_in = parse_value(command + 2);
	} else if (!strcmp(command, "78")) {
		sim_printf(handle, "6%u#", backlash_in);
	} else if (!strncmp(command, "79", 2)) {
		backlash_out = parse_value(command + 2);
	} else if (!strcmp(command, "80")) {
		sim_printf(handle, "7%u#", backlash_out);
	} else if (!strcmp(command, "03")) {
		sim_printf(handle, "F304#");
	}
	(void)speed;
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

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_mypro2_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("myFocuserPro2 focuser simulator is listening on %s\n", port);
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
