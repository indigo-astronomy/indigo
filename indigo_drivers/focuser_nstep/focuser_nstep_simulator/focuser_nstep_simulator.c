// Rigel Systems nSTEP focuser simulator
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

static int position = 50;
static char tt_value[5] = "+000";
static char ts_value[4] = "000";
static char ta_value = '0';
static char tb_value[4] = "000";
static char tc_value[3] = "30";
static char cw_value = '0';
static char cs_value[4] = "001";
static char co_value[4] = "003";
static int moving_polls = 0;

static void usage(const char *name) {
	printf("Rigel Systems nSTEP focuser simulator\n");
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
		fprintf(stderr, "<- %s\n", buffer);
	}
	return serial_simulator_write_all(handle, buffer, (size_t)length);
}

static void trace_command(const char *command, int length) {
	if (!options.trace) {
		return;
	}
	fprintf(stderr, "-> ");
	for (int i = 0; i < length; i++) {
		unsigned char c = (unsigned char)command[i];
		if (c >= 32 && c < 127) {
			fputc(c, stderr);
		} else {
			fprintf(stderr, "\\x%02X", c);
		}
	}
	fprintf(stderr, "\n");
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

static int read_nstep_command(int handle, char *buffer, int length) {
	char c = '\0';

	while (running) {
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
		break;
	}

	if (c == 0x06 || c == 'S' || c == '#') {
		buffer[0] = c;
		trace_command(buffer, 1);
		return 1;
	}
	if (c != ':') {
		return 0;
	}

	buffer[0] = c;
	if (read_exact(handle, buffer + 1, 2) != 2) {
		return 0;
	}

	int command_length = 3;
	if (!strncmp(buffer, ":R", 2)) {
		trace_command(buffer, command_length);
		return command_length;
	}
	if (!strncmp(buffer, ":CC", 3) || !strncmp(buffer, ":TA", 3)) {
		if (command_length + 1 > length || read_exact(handle, buffer + command_length, 1) != 1) {
			return 0;
		}
		command_length += 1;
		trace_command(buffer, command_length);
		return command_length;
	}
	if (!strncmp(buffer, ":CW", 3)) {
		if (command_length + 2 > length || read_exact(handle, buffer + command_length, 2) != 2) {
			return 0;
		}
		command_length += 2;
		trace_command(buffer, command_length);
		return command_length;
	}
	if (!strncmp(buffer, ":TC", 3)) {
		if (command_length + 3 > length || read_exact(handle, buffer + command_length, 3) != 3) {
			return 0;
		}
		command_length += 3;
		trace_command(buffer, command_length);
		return command_length;
	}
	if (!strncmp(buffer, ":CS", 3) || !strncmp(buffer, ":CO", 3) || !strncmp(buffer, ":TS", 3) || !strncmp(buffer, ":TB", 3)) {
		if (command_length + 4 > length || read_exact(handle, buffer + command_length, 4) != 4) {
			return 0;
		}
		command_length += 4;
		trace_command(buffer, command_length);
		return command_length;
	}
	if (!strncmp(buffer, ":F", 2) || !strncmp(buffer, ":TT", 3)) {
		if (command_length + 5 > length || read_exact(handle, buffer + command_length, 5) != 5) {
			return 0;
		}
		command_length += 5;
		trace_command(buffer, command_length);
		return command_length;
	}

	trace_command(buffer, command_length);
	return command_length;
}

static void copy_digits(char *target, size_t size, const char *source) {
	memcpy(target, source, size - 1);
	target[size - 1] = '\0';
}

static void dispatch_command(int handle, const char *command, int length) {
	if (length == 1 && command[0] == 0x06) {
		sim_printf(handle, "S");
	} else if (length == 1 && command[0] == 'S') {
		if (moving_polls > 0) {
			moving_polls--;
			sim_printf(handle, "1");
		} else {
			sim_printf(handle, "0");
		}
	} else if (!strncmp(command, ":RT", 3)) {
		sim_printf(handle, "+275");
	} else if (!strncmp(command, ":RA", 3)) {
		serial_simulator_write_all(handle, tt_value, 4);
	} else if (!strncmp(command, ":RB", 3)) {
		serial_simulator_write_all(handle, ts_value, 3);
	} else if (!strncmp(command, ":RG", 3)) {
		serial_simulator_write_all(handle, &ta_value, 1);
	} else if (!strncmp(command, ":RH", 3)) {
		serial_simulator_write_all(handle, tc_value, 2);
	} else if (!strncmp(command, ":RE", 3)) {
		serial_simulator_write_all(handle, tb_value, 3);
	} else if (!strncmp(command, ":RW", 3)) {
		serial_simulator_write_all(handle, &cw_value, 1);
	} else if (!strncmp(command, ":RS", 3)) {
		serial_simulator_write_all(handle, cs_value, 3);
	} else if (!strncmp(command, ":RO", 3)) {
		serial_simulator_write_all(handle, co_value, 3);
	} else if (!strncmp(command, ":RP", 3)) {
		sim_printf(handle, "%+07d", position);
	} else if (!strncmp(command, ":CS", 3) && length >= 7) {
		copy_digits(cs_value, sizeof(cs_value), command + 3);
	} else if (!strncmp(command, ":CO", 3) && length >= 7) {
		copy_digits(co_value, sizeof(co_value), command + 3);
	} else if (!strncmp(command, ":CW", 3) && length >= 4) {
		cw_value = command[3];
	} else if (!strncmp(command, ":TS", 3) && length >= 7) {
		copy_digits(ts_value, sizeof(ts_value), command + 3);
	} else if (!strncmp(command, ":TT", 3) && length >= 8) {
		copy_digits(tt_value, sizeof(tt_value), command + 3);
	} else if (!strncmp(command, ":TA", 3) && length >= 4) {
		ta_value = command[3];
	} else if (!strncmp(command, ":TC", 3) && length >= 6) {
		copy_digits(tc_value, sizeof(tc_value), command + 3);
	} else if (!strncmp(command, ":TB", 3) && length >= 7) {
		copy_digits(tb_value, sizeof(tb_value), command + 3);
	} else if (!strncmp(command, ":F", 2) && length >= 8) {
		if (!strncmp(command, ":F10000#", 8)) {
			moving_polls = 0;
		} else {
			int steps = (command[4] - '0') * 100 + (command[5] - '0') * 10 + command[6] - '0';
			position += command[2] == '0' ? steps : -steps;
			moving_polls = 2;
		}
	}
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[32];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_nstep_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("Rigel Systems nSTEP focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = read_nstep_command(serial_fd, buffer, sizeof(buffer));
		if (bytes < 0) {
			break;
		}
		if (bytes > 0) {
			dispatch_command(serial_fd, buffer, bytes);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
