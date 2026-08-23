// MoonLite focuser simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <pthread.h>
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
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned current_position = 0x8000;
static unsigned target_position = 0x8000;
static unsigned temperature = 0x002E;
static unsigned moving_status = 0x00;
static unsigned speed = 0x02;
static unsigned step_mode = 0x00;
static unsigned temperature_compensation = 0x00;
static bool use_compensation = false;

static void usage(const char *name) {
	printf("MoonLite focuser simulator\n");
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

static unsigned parse_hex_value(const char *text) {
	return (unsigned)strtoul(text, NULL, 16);
}

static void *movement_thread(void *arg) {
	(void)arg;
	while (running) {
		pthread_mutex_lock(&state_mutex);
		if (moving_status == 0x01) {
			if (target_position > current_position) {
				current_position++;
			} else if (target_position < current_position) {
				current_position--;
			} else {
				moving_status = 0x00;
			}
		}
		pthread_mutex_unlock(&state_mutex);
		usleep(1000);
	}
	return NULL;
}

static void dispatch_command(int handle, const char *command) {
	pthread_mutex_lock(&state_mutex);
	if (!strcmp(command, "C")) {
	} else if (!strcmp(command, "FG")) {
		moving_status = 0x01;
	} else if (!strcmp(command, "FQ")) {
		moving_status = 0x00;
		target_position = current_position;
	} else if (!strcmp(command, "GC")) {
		sim_printf(handle, "%02X#", temperature_compensation & 0xFF);
	} else if (!strcmp(command, "GD")) {
		sim_printf(handle, "%02X#", speed & 0xFF);
	} else if (!strcmp(command, "GH")) {
		sim_printf(handle, "%02X#", step_mode & 0xFF);
	} else if (!strcmp(command, "GI")) {
		sim_printf(handle, "%02X#", moving_status & 0xFF);
	} else if (!strcmp(command, "GN")) {
		sim_printf(handle, "%04X#", target_position & 0xFFFF);
	} else if (!strcmp(command, "GP")) {
		sim_printf(handle, "%04X#", current_position & 0xFFFF);
	} else if (!strcmp(command, "GT")) {
		sim_printf(handle, "%04X#", temperature & 0xFFFF);
	} else if (!strcmp(command, "GV")) {
		sim_printf(handle, "12#");
	} else if (!strncmp(command, "SD", 2)) {
		speed = parse_hex_value(command + 2);
	} else if (!strcmp(command, "SF")) {
		step_mode = 0x00;
	} else if (!strcmp(command, "SH")) {
		step_mode = 0xFF;
	} else if (!strncmp(command, "SN", 2)) {
		target_position = parse_hex_value(command + 2);
	} else if (!strncmp(command, "SP", 2)) {
		current_position = parse_hex_value(command + 2);
		target_position = current_position;
	} else if (!strncmp(command, "SC", 2)) {
		temperature_compensation = parse_hex_value(command + 2);
	} else if (!strcmp(command, "+")) {
		use_compensation = true;
	} else if (!strcmp(command, "-")) {
		use_compensation = false;
	} else if (!strncmp(command, "PO", 2)) {
	}
	pthread_mutex_unlock(&state_mutex);
}

int main(int argc, char *argv[]) {
	pthread_t thread;
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

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_moonlite_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("MoonLite focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, movement_thread, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		return 1;
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

	running = 0;
	pthread_join(thread, NULL);
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
