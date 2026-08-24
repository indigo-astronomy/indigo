// Pegasus Indigo filter wheel serial simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// This source file was refactored from wheel_indigo_simulator.ino by a Codex agent.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

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

static const char *simulator_name = "wheel_indigo";
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static int current_filter = 1;
static int target_filter = 1;
static int moving = 0;

static void usage(const char *name) {
	printf("Pegasus Indigo filter wheel serial simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  -h, --help              Show this help and exit\n");
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

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

static bool write_response(const char *response) {
	serial_simulator_trace_line(options.trace, "<-", response);
	return serial_simulator_write_all(serial_fd, response, strlen(response));
}

static void update_state(void) {
	if (current_filter < target_filter) {
		current_filter++;
	} else if (current_filter > target_filter) {
		current_filter--;
	} else {
		moving = 0;
	}
}

static void dispatch_command(const char *command) {
	char response[64];
	serial_simulator_trace_line(options.trace, "->", command);
	update_state();
	if (!strcmp(command, "W#")) {
		write_response("FW_OK\n");
	} else if (!strcmp(command, "WA")) {
		snprintf(response, sizeof(response), "FW_OK:%d:%d\n", current_filter, moving);
		write_response(response);
	} else if (!strcmp(command, "WF")) {
		snprintf(response, sizeof(response), "WF:%d\n", moving == 0 ? current_filter : -1);
		write_response(response);
	} else if (!strcmp(command, "WV")) {
		write_response("WV:1.1\n");
	} else if (!strncmp(command, "WM:", 3)) {
		int slot = atoi(command + 3);
		if (slot >= 1 && slot <= 7) {
			target_filter = slot;
			moving = current_filter != target_filter;
		}
		snprintf(response, sizeof(response), "%s\n", command);
		write_response(response);
	} else if (!strcmp(command, "WR")) {
		snprintf(response, sizeof(response), "WR:%d\n", moving);
		write_response(response);
	} else if (!strcmp(command, "WI")) {
		moving = 0;
		current_filter = target_filter = 1;
		write_response("WI:1\n");
	} else if (!strcmp(command, "WQ")) {
		/* The Arduino sketch intentionally gives no response to WQ. */
	}
}

static void run_loop(void) {
	char command[64];
	size_t used = 0;

	while (running) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(serial_fd, &readfds);
		struct timeval timeout = { .tv_sec = 0, .tv_usec = 100000 };
		int selected = select(serial_fd + 1, &readfds, NULL, NULL, &timeout);
		if (selected < 0) {
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		if (selected == 0) {
			continue;
		}
		char buffer[32];
		ssize_t count = read(serial_fd, buffer, sizeof(buffer));
		if (count <= 0) {
			if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO)) {
				continue;
			}
			break;
		}
		for (ssize_t i = 0; i < count; i++) {
			if (buffer[i] == '\r') {
				continue;
			}
			if (buffer[i] == '\n') {
				command[used] = '\0';
				if (used > 0) {
					dispatch_command(command);
				}
				used = 0;
				continue;
			}
			if (used < sizeof(command) - 1) {
				command[used++] = buffer[i];
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (!parse_args(argc, argv)) {
		return 2;
	}
	char port[PATH_MAX];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}
	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		close(serial_fd);
		return 1;
	}
	if (!options.headless) {
		printf("Pegasus Indigo filter wheel simulator listening on %s\n", port);
		fflush(stdout);
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	run_loop();
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
