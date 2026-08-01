// StarlightXpress AO simulator
//
// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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
// This simulator was refactored from the Arduino sketch of the same name by the Claude Code agent (Claude Opus 4.8).
//
// Unlike the line-oriented simulators, the SX AO protocol is character
// framed: each request is a single command letter, optionally followed by a
// fixed six-byte parameter block, and each reply is a fixed number of raw
// bytes with no terminator.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *firmware;
	int limit;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.firmware = "123",
	.limit = 50
};

static const char *simulator_name = "ao_sx";

static void usage(const char *name) {
	printf("StarlightXpress AO simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --firmware <version>    Reported firmware, sent after 'V', default is 123\n");
	printf("  --limit <steps>         Tip/tilt travel limit in steps, default is 50\n");
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
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a version\n");
				return false;
			}
			options.firmware = argv[i];
		} else if (!strcmp(argv[i], "--limit")) {
			if (++i == argc) {
				fprintf(stderr, "--limit requires a step count\n");
				return false;
			}
			options.limit = atoi(argv[i]);
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static int ra = 0;
static int dec = 0;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- protocol

static bool sim_send(int fd, const char *data, size_t length) {
	if (options.trace) {
		char text[32];
		size_t shown = length < sizeof(text) - 1 ? length : sizeof(text) - 1;
		memcpy(text, data, shown);
		text[shown] = '\0';
		serial_simulator_trace_line(true, "<-", text);
	}
	return serial_simulator_write_all(fd, data, length);
}

static int sim_read_byte(int fd, char *byte) {
	while (running) {
		ssize_t count = read(fd, byte, 1);
		if (count == 1) {
			return 0;
		}
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				usleep(500);
				continue;
			}
			return -1;
		}
		usleep(500);
	}
	return -1;
}

static int sim_read_bytes(int fd, char *buffer, size_t length) {
	for (size_t i = 0; i < length; i++) {
		if (sim_read_byte(fd, buffer + i) < 0) {
			return -1;
		}
	}
	return (int)length;
}

// Apply a tip/tilt move to one axis and report 'G' when it stays within the
// travel limit, or 'L' when it hits the limit.
static char move_axis(int *axis, int steps, int sign) {
	*axis += sign * steps;
	if (*axis > options.limit) {
		*axis = options.limit;
		return 'L';
	}
	if (*axis < -options.limit) {
		*axis = -options.limit;
		return 'L';
	}
	return 'G';
}

static void dispatch_command(int fd, char command) {
	char params[7] = { 0 };
	char reply;

	switch (command) {
		case 'K':
		case 'R':
			ra = dec = 0;
			sim_send(fd, "K", 1);
			break;
		case 'G': {
			if (sim_read_bytes(fd, params, 6) < 0) {
				return;
			}
			serial_simulator_trace_line(options.trace, "->", params);
			int steps = atoi(params + 1);
			switch (params[0]) {
				case 'N': reply = move_axis(&dec, steps, +1); break;
				case 'S': reply = move_axis(&dec, steps, -1); break;
				case 'T': reply = move_axis(&ra, steps, +1); break;
				case 'W': reply = move_axis(&ra, steps, -1); break;
				default: return;
			}
			sim_send(fd, &reply, 1);
			break;
		}
		case 'M':
			if (sim_read_bytes(fd, params, 6) < 0) {
				return;
			}
			serial_simulator_trace_line(options.trace, "->", params);
			sim_send(fd, "M", 1);
			break;
		case 'L': {
			char state = 0x30;
			if (ra == options.limit) {
				state |= 0x08;
			} else if (ra == -options.limit) {
				state |= 0x02;
			}
			if (dec == options.limit) {
				state |= 0x04;
			} else if (dec == -options.limit) {
				state |= 0x01;
			}
			sim_send(fd, &state, 1);
			break;
		}
		case 'V': {
			char buffer[8];
			int length = snprintf(buffer, sizeof(buffer), "V%s", options.firmware);
			if (length > 0) {
				sim_send(fd, buffer, (size_t)length);
			}
			break;
		}
		case 'X':
			sim_send(fd, "Y", 1);
			break;
		case 'U':
			sim_send(fd, "Z", 1);
			break;
		default:
			break;
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command = '\0';
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
		printf("StarlightXpress AO simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_byte(serial_fd, &command) < 0) {
			break;
		}
		dispatch_command(serial_fd, command);
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
