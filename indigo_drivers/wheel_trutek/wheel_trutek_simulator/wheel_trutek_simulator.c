// Trutek wheel serial simulator
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
// This simulator was refactored from wheel_trutek_simulator.ino by a Codex agent.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
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
	int slots;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.slots = 5
};

static const char *simulator_name = "wheel_trutek";
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static int current_filter = 1;
static int target_filter = 1;

// ----------------------------------------------------------------- runtime

static void usage(const char *name) {
	fprintf(stderr, "Usage: %s [--headless] [--ready-file <path>] [--trace] [--no-trace] [--slots <count>]\n", name);
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

static uint8_t checksum(uint8_t header, uint8_t command, uint8_t value) {
	return (uint8_t)(header + command + value);
}

static void trace_frame(const char *prefix, const uint8_t frame[4]) {
	if (options.trace) {
		fprintf(stderr, "%s %02X %02X %02X %02X\n", prefix, frame[0], frame[1], frame[2], frame[3]);
	}
}

static void advance_filter(void) {
	if (current_filter < target_filter) {
		current_filter++;
	} else if (current_filter > target_filter) {
		current_filter--;
	}
}

static bool write_frame(int fd, uint8_t command, uint8_t value) {
	uint8_t frame[4] = { 0xA5, command, value, 0 };
	frame[3] = checksum(frame[0], frame[1], frame[2]);
	trace_frame("<", frame);
	return serial_simulator_write_all(fd, (const char *)frame, sizeof(frame));
}

static bool dispatch_command(int fd, const uint8_t frame[4]) {
	trace_frame(">", frame);
	if (frame[0] != 0xA5 || frame[3] != checksum(frame[0], frame[1], frame[2])) {
		advance_filter();
		return true;
	}

	switch (frame[1]) {
		case 0x01:
			if (frame[2] >= 1 && frame[2] <= options.slots) {
				target_filter = frame[2];
			}
			if (!write_frame(fd, 0x81, current_filter == target_filter ? (uint8_t)target_filter : 0)) {
				return false;
			}
			break;
		case 0x02:
			if (!write_frame(fd, 0x82, current_filter == target_filter ? (uint8_t)(0x30 + current_filter) : 0x30)) {
				return false;
			}
			break;
		case 0x03:
			current_filter = target_filter = 1;
			if (!write_frame(fd, 0x83, (uint8_t)(0x30 + options.slots))) {
				return false;
			}
			break;
	}
	advance_filter();
	return true;
}

static int read_command_byte(int fd, uint8_t *byte) {
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
	uint8_t command[4];
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
		printf("Trutek wheel simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		uint8_t byte;
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
