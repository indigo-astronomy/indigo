// NexDome serial simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	.ready_file = NULL,
};

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static const char *simulator_name = "dome_nexdome";

static double azimuth = 0;
static double park_azimuth = 180;
static double target_azimuth = 0;
static int motion_polls_remaining = 0;
static int shutter_state = 3; // closed
static bool reversed = false;

static void usage(const char *name) {
	printf("NexDome serial simulator\n");
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

static bool sim_read_line(int fd, char *buffer, size_t length) {
	size_t used = 0;
	while (running && used + 1 < length) {
		char byte;
		ssize_t count = read(fd, &byte, 1);
		if (count == 1) {
			if (byte == '\n' || byte == '\r') {
				if (used == 0) {
					continue;
				}
				break;
			}
			buffer[used++] = byte;
			continue;
		}
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				usleep(500);
				continue;
			}
			return false;
		}
		usleep(500);
	}
	buffer[used] = '\0';
	if (used > 0) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return used > 0;
}

static void sim_write_response(int fd, const char *response) {
	serial_simulator_trace_line(options.trace, "<-", response);
	serial_simulator_write_all(fd, response, strlen(response));
	serial_simulator_write_all(fd, "\n", 1);
}

static double normalized_azimuth(double value) {
	while (value < 0) {
		value += 360;
	}
	while (value >= 360) {
		value -= 360;
	}
	return value;
}

static void finish_motion_if_ready(void) {
	if (motion_polls_remaining > 0 && --motion_polls_remaining == 0) {
		azimuth = target_azimuth;
	}
}

static void dispatch_command(int fd, const char *command) {
	char response[128];
	char op = command[0];

	if (op == 'v') {
		sim_write_response(fd, "VNexDome V 1.10");
	} else if (op == 'a') {
		motion_polls_remaining = 0;
		target_azimuth = azimuth;
		sim_write_response(fd, "A");
	} else if (op == 'm') {
		if (motion_polls_remaining > 0) {
			finish_motion_if_ready();
			sim_write_response(fd, "M 1");
		} else {
			sim_write_response(fd, "M 0");
		}
	} else if (op == 'q') {
		snprintf(response, sizeof(response), "Q %.1f", azimuth);
		sim_write_response(fd, response);
	} else if (op == 'u') {
		snprintf(response, sizeof(response), "U %d 1", shutter_state);
		sim_write_response(fd, response);
	} else if (op == 'd') {
		shutter_state = 1;
		sim_write_response(fd, "D");
	} else if (op == 'e') {
		shutter_state = 3;
		sim_write_response(fd, "D");
	} else if (op == 's') {
		double value = atof(command + 1);
		azimuth = normalized_azimuth(value);
		target_azimuth = azimuth;
		motion_polls_remaining = 0;
		snprintf(response, sizeof(response), "S %.1f", azimuth);
		sim_write_response(fd, response);
	} else if (op == 'n') {
		snprintf(response, sizeof(response), "N %.1f", park_azimuth);
		sim_write_response(fd, response);
	} else if (op == 'g') {
		double value = atof(command + 1);
		if (value >= 0 && value <= 360) {
			target_azimuth = normalized_azimuth(value);
			motion_polls_remaining = 2;
			sim_write_response(fd, "G");
		} else {
			sim_write_response(fd, "E");
		}
	} else if (op == 'h') {
		target_azimuth = 0;
		motion_polls_remaining = 2;
		sim_write_response(fd, "H");
	} else if (op == 'c') {
		target_azimuth = 0;
		motion_polls_remaining = 2;
		sim_write_response(fd, "C");
	} else if (op == 'y') {
		const char *value = command + 1;
		while (isspace((unsigned char)*value)) {
			value++;
		}
		if (*value != '\0') {
			reversed = atoi(value) != 0;
		}
		snprintf(response, sizeof(response), "Y %d", reversed ? 1 : 0);
		sim_write_response(fd, response);
	} else if (op == 'k') {
		sim_write_response(fd, "K 1320 1320");
	} else if (op == 'w') {
		sim_write_response(fd, "W");
	} else {
		serial_simulator_trace_line(options.trace, "??", command);
		sim_write_response(fd, "E");
	}
}

int main(int argc, char *argv[]) {
	char command[128];
	char port[128];

	if (!parse_args(argc, argv)) {
		usage(argv[0]);
		return 1;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	if (!options.headless) {
		printf("NexDome serial simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_line(serial_fd, command, sizeof(command))) {
			dispatch_command(serial_fd, command);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
