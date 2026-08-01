// Optec wheel serial simulator
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
// This simulator was refactored from wheel_optec_simulator.ino by a Codex agent.

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
	bool goto_error;
	const char *ready_file;
	int slots;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.goto_error = false,
	.ready_file = NULL,
	.slots = 5
};

static const char *simulator_name = "wheel_optec";
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static bool is_ready = false;
static int current_filter = 1;

// ----------------------------------------------------------------- runtime

static void usage(const char *name) {
	fprintf(stderr, "Usage: %s [--headless] [--ready-file <path>] [--trace] [--no-trace] [--goto-error] [--slots <count>]\n", name);
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
		} else if (!strcmp(argv[i], "--goto-error")) {
			options.goto_error = true;
		} else if (!strcmp(argv[i], "--ready-file") && i + 1 < argc) {
			options.ready_file = argv[++i];
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
	}
	return true;
}

static void signal_handler(int signal_number) {
	(void)signal_number;
	running = 0;
}

static bool write_response(int fd, const char *response) {
	if (options.trace) {
		fprintf(stderr, "< %s", response);
	}
	return serial_simulator_write_all(fd, response, strlen(response));
}

static bool dispatch_command(int fd, const char command[6]) {
	if (options.trace) {
		fprintf(stderr, "> %.*s\n", 6, command);
	}

	if (!memcmp(command, "WSMODE", 6)) {
		is_ready = true;
		return write_response(fd, "!\r\n");
	}
	if (!is_ready) {
		return true;
	}
	if (!memcmp(command, "WEXITS", 6)) {
		is_ready = false;
		return write_response(fd, "END\r\n");
	}
	if (!memcmp(command, "WHOME", 6)) {
		current_filter = 1;
		return write_response(fd, "A\r\n");
	}
	if (!memcmp(command, "WFILTR", 6)) {
		char response[4];
		snprintf(response, sizeof(response), "%d\r\n", current_filter);
		return write_response(fd, response);
	}
	if (!memcmp(command, "WGOTO", 5)) {
		int requested_filter = command[5] - '0';
		if (requested_filter < 1 || requested_filter > options.slots) {
			return write_response(fd, "ERR=5\r\n");
		}
		if (options.goto_error) {
			return write_response(fd, "ERR=4\r\n");
		}
		current_filter = requested_filter;
		return write_response(fd, "*\r\n");
	}
	return true;
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
	char command[6];
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
		printf("Optec wheel simulator is running on %s\n", port);
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
