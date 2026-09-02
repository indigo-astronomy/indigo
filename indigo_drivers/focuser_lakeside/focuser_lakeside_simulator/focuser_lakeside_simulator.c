// LakesideAstro focuser simulator
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

static unsigned direction = 0;
static unsigned temperature = 23;
static unsigned current_position = 0x8000;
static unsigned target_position = 0x8000;
static unsigned backlash = 0;
static unsigned max_travel = 0xFFFF;
static unsigned step_size = 1;
static unsigned active_slope = 1;
static unsigned slope[2] = { 10, 20 };
static unsigned slope_direction[2] = { 0, 1 };
static unsigned slope_deadband[2] = { 5, 10 };
static unsigned slope_period[2] = { 1, 10 };

static void usage(const char *name) {
	printf("LakesideAstro focuser simulator\n");
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

static void *movement_thread(void *arg) {
	(void)arg;
	while (running) {
		pthread_mutex_lock(&state_mutex);
		int delta = 0;
		if (target_position > current_position) {
			current_position++;
			delta = 1;
		} else if (target_position < current_position) {
			current_position--;
			delta = -1;
		}
		unsigned position = current_position;
		bool done = delta != 0 && target_position == current_position;
		pthread_mutex_unlock(&state_mutex);

		if (delta != 0 && serial_fd >= 0) {
			sim_printf(serial_fd, "P%u#", position);
			if (done) {
				sim_printf(serial_fd, "DONE#");
			}
		}
		usleep(delta == 0 ? 10000 : 5000);
	}
	return NULL;
}

static void respond_value(int handle, const char *prefix, unsigned value) {
	sim_printf(handle, "%s%u#", prefix, value);
}

static void dispatch_command(int handle, const char *command) {
	pthread_mutex_lock(&state_mutex);
	if (!strcmp(command, "??")) {
		sim_printf(handle, "OK#");
	} else if (!strcmp(command, "?D")) {
		respond_value(handle, "D", direction);
	} else if (!strcmp(command, "?T")) {
		respond_value(handle, "T", temperature * 2);
	} else if (!strcmp(command, "?P")) {
		respond_value(handle, "P", current_position);
	} else if (!strcmp(command, "?B")) {
		respond_value(handle, "B", backlash);
	} else if (!strcmp(command, "?I")) {
		respond_value(handle, "I", max_travel);
	} else if (!strcmp(command, "?S")) {
		respond_value(handle, "S", step_size);
	} else if (!strncmp(command, "CRB", 3)) {
		backlash = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRS", 3)) {
		step_size = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRg", 3)) {
		active_slope = (unsigned)atoi(command + 3);
		if (active_slope < 1 || active_slope > 2) {
			active_slope = 1;
		}
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRD", 3)) {
		direction = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CT", 2)) {
	} else if (!strncmp(command, "CI", 2)) {
		unsigned steps = (unsigned)atoi(command + 2);
		target_position = current_position > steps ? current_position - steps : 0;
	} else if (!strncmp(command, "CO", 2)) {
		target_position = current_position + (unsigned)atoi(command + 2);
		if (target_position > max_travel) {
			target_position = max_travel;
		}
	} else if (!strcmp(command, "CH")) {
		target_position = current_position;
	} else if (!strcmp(command, "?1")) {
		respond_value(handle, "1", slope[0]);
	} else if (!strcmp(command, "?a")) {
		respond_value(handle, "a", slope_direction[0]);
	} else if (!strcmp(command, "?c")) {
		respond_value(handle, "c", slope_deadband[0]);
	} else if (!strcmp(command, "?e")) {
		respond_value(handle, "e", slope_period[0]);
	} else if (!strcmp(command, "?2")) {
		respond_value(handle, "2", slope[1]);
	} else if (!strcmp(command, "?b")) {
		respond_value(handle, "b", slope_direction[1]);
	} else if (!strcmp(command, "?d")) {
		respond_value(handle, "d", slope_deadband[1]);
	} else if (!strcmp(command, "?f")) {
		respond_value(handle, "f", slope_period[1]);
	} else if (!strncmp(command, "CR1", 3)) {
		slope[0] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRa", 3)) {
		slope_direction[0] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRc", 3)) {
		slope_deadband[0] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRe", 3)) {
		slope_period[0] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CR2", 3)) {
		slope[1] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRb", 3)) {
		slope_direction[1] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRd", 3)) {
		slope_deadband[1] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else if (!strncmp(command, "CRf", 3)) {
		slope_period[1] = (unsigned)atoi(command + 3);
		sim_printf(handle, "OK#");
	} else {
		sim_printf(handle, "!#");
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

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_lakeside_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("LakesideAstro focuser simulator is listening on %s\n", port);
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
