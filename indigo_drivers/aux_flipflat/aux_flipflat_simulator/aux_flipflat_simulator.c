// Optec/Alnitak Flip-Flat Box simulator
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

// Device ids: 10 = Flat-Man_XL, 15 = Flat-Man_L, 19 = Flat-Man,
// 98 = Flip-Mask/Remote Dust Cover, 99 = Flip-Flat.
typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *firmware;
	int device_id;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.firmware = "123",
	.device_id = 99
};

static const char *simulator_name = "aux_flipflat";

static void usage(const char *name) {
	printf("Optec/Alnitak Flip-Flat Box simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --device-id <id>        Model id (10, 15, 19, 98, 99), default is 99\n");
	printf("  --firmware <version>    Reported firmware version, default is 123\n");
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
		} else if (!strcmp(argv[i], "--device-id")) {
			if (++i == argc) {
				fprintf(stderr, "--device-id requires an id\n");
				return false;
			}
			options.device_id = atoi(argv[i]) % 100;
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a version\n");
				return false;
			}
			options.firmware = argv[i];
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

static int cover = 1;         // 1 = closed, 2 = open
static int light = 0;         // 0 = off, 1 = on
static int brightness = 128;  // 0..255
static long long move_end_ms = 0;

static long long now_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static bool cover_moving(void) {
	return now_ms() < move_end_ms;
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- protocol

static bool sim_printf(int fd, const char *format, ...) {
	char buffer[32];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0 || length >= (int)sizeof(buffer)) {
		return false;
	}
	serial_simulator_trace_line(options.trace, "<-", buffer);
	return serial_simulator_write_all(fd, buffer, (size_t)length);
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

// Flip-Flat frames every request with a trailing carriage return.
static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;

	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '\n') {
			continue;
		}
		if (byte == '\r') {
			buffer[used] = '\0';
			if (used > 0) {
				serial_simulator_trace_line(options.trace, "->", buffer);
			}
			return (int)used;
		}
		buffer[used++] = byte;
	}

	buffer[0] = '\0';
	return -1;
}

static void dispatch_command(int fd, const char *buffer) {
	int id = options.device_id;
	if (!strcmp(buffer, ">POOO")) {
		sim_printf(fd, "*P%02dOOO\n", id);
	} else if (!strcmp(buffer, ">OOOO")) {
		cover = 2;
		move_end_ms = now_ms() + 2000;
		sim_printf(fd, "*O%02dOOO\n", id);
	} else if (!strcmp(buffer, ">COOO")) {
		cover = 1;
		move_end_ms = now_ms() + 2000;
		sim_printf(fd, "*C%02dOOO\n", id);
	} else if (!strcmp(buffer, ">LOOO")) {
		light = 1;
		sim_printf(fd, "*L%02dOOO\n", id);
	} else if (!strcmp(buffer, ">DOOO")) {
		light = 0;
		sim_printf(fd, "*D%02dOOO\n", id);
	} else if (!strncmp(buffer, ">B", 2)) {
		brightness = atoi(buffer + 2) % 1000;
		sim_printf(fd, "*B%02d%03d\n", id, brightness);
	} else if (!strcmp(buffer, ">JOOO")) {
		sim_printf(fd, "*J%02d%03d\n", id, brightness);
	} else if (!strcmp(buffer, ">SOOO")) {
		bool moving = cover_moving();
		sim_printf(fd, "*S%02d%d%d%d\n", id, moving ? 1 : 0, light, moving ? 0 : cover);
	} else if (!strcmp(buffer, ">VOOO")) {
		sim_printf(fd, "*V%02d%s\n", id, options.firmware);
	} else if (*buffer) {
		serial_simulator_trace_line(options.trace, "??", buffer);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[32];
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
		printf("Optec/Alnitak Flip-Flat Box simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command)) > 0) {
			dispatch_command(serial_fd, command);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
