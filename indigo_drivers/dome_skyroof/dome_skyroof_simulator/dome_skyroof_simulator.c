// Interactive Astronomy SkyRoof serial simulator
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
static const char *simulator_name = "dome_skyroof";

static bool roof_open = false;
static bool heater_on = false;
static int moving_status_polls = 0;

static void usage(const char *name) {
	printf("Interactive Astronomy SkyRoof serial simulator\n");
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

static bool sim_read_command(int fd, char *buffer, size_t length) {
	size_t used = 0;
	while (running && used + 1 < length) {
		char byte;
		ssize_t count = read(fd, &byte, 1);
		if (count == 1) {
			if (byte == '\r' || byte == '\n') {
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
	serial_simulator_write_all(fd, "\r", 1);
}

static void dispatch_command(int fd, const char *command) {
	if (!strcmp(command, "Status#")) {
		if (moving_status_polls > 0) {
			moving_status_polls--;
			sim_write_response(fd, "Safety#");
		} else {
			sim_write_response(fd, roof_open ? "RoofOpen#" : "RoofClosed#");
		}
	} else if (!strcmp(command, "Open#")) {
		roof_open = true;
		moving_status_polls = 1;
		sim_write_response(fd, "0#");
	} else if (!strcmp(command, "Close#")) {
		roof_open = false;
		moving_status_polls = 1;
		sim_write_response(fd, "0#");
	} else if (!strcmp(command, "Stop#")) {
		moving_status_polls = 0;
		sim_write_response(fd, "0#");
	} else if (!strcmp(command, "Parkstatus#")) {
		sim_write_response(fd, "0#");
	} else if (!strcmp(command, "HeaterOn#")) {
		heater_on = true;
	} else if (!strcmp(command, "HeaterOff#")) {
		heater_on = false;
	} else {
		serial_simulator_trace_line(options.trace, "??", command);
		sim_write_response(fd, "Safety#");
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
		printf("Interactive Astronomy SkyRoof serial simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command))) {
			dispatch_command(serial_fd, command);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	(void)heater_on;
	return 0;
}
