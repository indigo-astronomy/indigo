// Xagyl wheel serial simulator
//
// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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
//
// This simulator was refactored from wheel_xagyl_simulator.ino by a Codex agent.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *model;
	const char *firmware;
	const char *serial_number;
	int slots;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.model = "Xagyl FW5125VX",
	.firmware = "FW3.1.5",
	.serial_number = "123456",
	.slots = 5
};

static const char *simulator_name = "wheel_xagyl";
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static int current_filter = 1;
static int target_filter = 1;

// ----------------------------------------------------------------- runtime

static void usage(const char *name) {
	fprintf(stderr, "Usage: %s [--headless] [--ready-file <path>] [--trace] [--no-trace] [--model <text>] [--firmware <text>] [--serial <text>] [--slots <count>]\n", name);
}

static bool parse_int(const char *text, int min, int max, int *value) {
	char *end = NULL;
	long parsed = strtol(text, &end, 10);
	if (end == text || *end != '\0' || parsed < min || parsed > max) {
		return false;
	}
	*value = (int)parsed;
	return true;
}

static bool parse_args(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) {
			options.headless = true;
			options.trace = false;
		} else if (!strcmp(argv[i], "--trace")) {
			options.trace = true;
		} else if (!strcmp(argv[i], "--no-trace")) {
			options.trace = false;
		} else if (!strcmp(argv[i], "--ready-file") && i + 1 < argc) {
			options.ready_file = argv[++i];
		} else if (!strcmp(argv[i], "--model") && i + 1 < argc) {
			options.model = argv[++i];
		} else if (!strcmp(argv[i], "--firmware") && i + 1 < argc) {
			options.firmware = argv[++i];
		} else if (!strcmp(argv[i], "--serial") && i + 1 < argc) {
			options.serial_number = argv[++i];
		} else if (!strcmp(argv[i], "--slots") && i + 1 < argc) {
			if (!parse_int(argv[++i], 1, 9, &options.slots)) {
				return false;
			}
		} else {
			return false;
		}
	}
	if (current_filter > options.slots) {
		current_filter = options.slots;
		target_filter = options.slots;
	}
	return true;
}

static void signal_handler(int signal_number) {
	(void)signal_number;
	running = 0;
}

static void advance_filter(void) {
	if (current_filter < target_filter) {
		current_filter++;
	} else if (current_filter > target_filter) {
		current_filter--;
	}
}

static bool write_response(int fd, const char *response) {
	serial_simulator_trace_line(options.trace, "<", response);
	return serial_simulator_write_all(fd, response, strlen(response));
}

static bool dispatch_command(int fd, const char command[2]) {
	char trace[3] = { command[0], command[1], 0 };
	char response[64];
	serial_simulator_trace_line(options.trace, ">", trace);

	switch (command[0]) {
		case 'I':
			switch (command[1]) {
				case '0':
					snprintf(response, sizeof(response), "%s\n", options.model);
					break;
				case '1':
					snprintf(response, sizeof(response), "%s\n", options.firmware);
					break;
				case '2':
					snprintf(response, sizeof(response), "P%d\n", current_filter);
					advance_filter();
					return write_response(fd, response);
				case '3':
					snprintf(response, sizeof(response), "S/N: %s\n", options.serial_number);
					break;
				case '8':
					snprintf(response, sizeof(response), "FilterSlots %d\n", options.slots);
					break;
				default:
					advance_filter();
					return true;
			}
			advance_filter();
			return write_response(fd, response);
		case 'G': {
			int requested_filter = command[1] - '0';
			if (requested_filter >= 1 && requested_filter <= options.slots) {
				target_filter = requested_filter;
			}
			advance_filter();
			return true;
		}
		default:
			advance_filter();
			return true;
	}
}

static int read_command_byte(int fd, char *byte) {
	ssize_t count = read(fd, byte, 1);
	if (count == 1) {
		return 1;
	}
	if (count < 0) {
		if (errno == EINTR) {
			return 0;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
			return 0;
		}
		perror("read");
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	char port[128];
	char command[2];
	size_t command_length = 0;

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
		printf("Xagyl wheel simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		char byte;
		int read_result = read_command_byte(serial_fd, &byte);
		if (read_result < 0) {
			break;
		}
		if (read_result == 0) {
			usleep(1000);
			continue;
		}
		command[command_length++] = byte;
		if (command_length == sizeof(command)) {
			if (!dispatch_command(serial_fd, command)) {
				break;
			}
			command_length = 0;
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
