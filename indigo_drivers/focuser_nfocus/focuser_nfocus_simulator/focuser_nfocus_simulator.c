// Rigel Systems nFOCUS focuser simulator
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

static char speed_in[4] = "001";
static char speed_out[4] = "005";
static int moving_polls = 0;

static void usage(const char *name) {
	printf("Rigel Systems nFOCUS focuser simulator\n");
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

static void discard_until_hash(int handle) {
	char c;
	while (running) {
		int bytes = read_exact(handle, &c, 1);
		if (bytes <= 0 || c == '#') {
			return;
		}
	}
}

static int read_nfocus_command(int handle, char *buffer, int length) {
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
	if (!strncmp(buffer, ":RT", 3) || !strncmp(buffer, ":RO", 3) || !strncmp(buffer, ":RS", 3)) {
		trace_command(buffer, command_length);
		return command_length;
	}

	if (!strncmp(buffer, ":CS", 3) || !strncmp(buffer, ":CO", 3) || !strncmp(buffer, ":CF", 3)) {
		if (command_length + 4 > length || read_exact(handle, buffer + command_length, 4) != 4) {
			return 0;
		}
		command_length += 4;
		trace_command(buffer, command_length);
		return command_length;
	}

	if (!strncmp(buffer, ":F", 2)) {
		if (command_length + 5 > length || read_exact(handle, buffer + command_length, 5) != 5) {
			return 0;
		}
		command_length += 5;
		trace_command(buffer, command_length);
		return command_length;
	}

	discard_until_hash(handle);
	trace_command(buffer, command_length);
	return command_length;
}

static void dispatch_command(int handle, const char *command, int length) {
	if (length == 1 && command[0] == 0x06) {
		sim_printf(handle, "n");
	} else if (length == 1 && command[0] == 'S') {
		if (moving_polls > 0) {
			moving_polls--;
			sim_printf(handle, "1");
		} else {
			sim_printf(handle, "0");
		}
	} else if (!strncmp(command, ":RT", 3)) {
		sim_printf(handle, "+275");
	} else if (!strncmp(command, ":RO", 3)) {
		serial_simulator_write_all(handle, speed_out, 3);
		if (options.trace) {
			fprintf(stderr, "<- %.3s\n", speed_out);
		}
	} else if (!strncmp(command, ":RS", 3)) {
		serial_simulator_write_all(handle, speed_in, 3);
		if (options.trace) {
			fprintf(stderr, "<- %.3s\n", speed_in);
		}
	} else if (!strncmp(command, ":CS", 3) && length >= 7) {
		memcpy(speed_in, command + 3, 3);
		speed_in[3] = '\0';
	} else if ((!strncmp(command, ":CO", 3) || !strncmp(command, ":CF", 3)) && length >= 7) {
		memcpy(speed_out, command + 3, 3);
		speed_out[3] = '\0';
	} else if (!strncmp(command, ":F", 2)) {
		moving_polls = strncmp(command, ":F11000#", 8) ? 2 : 0;
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

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_nfocus_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("Rigel Systems nFOCUS focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = read_nfocus_command(serial_fd, buffer, sizeof(buffer));
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
