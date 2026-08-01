// WandererAstro WandererCover V4-EC simulator
//
// Copyright (c) 2025-2026 CloudMakers, s. r. o.
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
// This simulator was refactored by a Codex agent.

#include <pthread.h>
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
	const char *model;
	const char *firmware;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.model = "WandererCoverV4",
	.firmware = "20240618"
};

static const char *simulator_name = "aux_wcv4ec";

static void usage(const char *name) {
	printf("WandererAstro WandererCover V4-EC simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <WandererCoverV4> Select simulated model, default is WandererCoverV4\n");
	printf("  --firmware <version>    Set reported firmware version\n");
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
		} else if (!strcmp(argv[i], "--model")) {
			if (++i == argc) {
				fprintf(stderr, "--model requires WandererCoverV4\n");
				return false;
			}
			if (strcmp(argv[i], "WandererCoverV4")) {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
			options.model = argv[i];
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
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static double open_position = 270.0;
static double close_position = 20.0;
static double current_position = 20.0;
static double voltage = 12.11;
static int heater = 0;
static int brightness = 0;
static bool do_open = false;
static bool do_close = false;
static bool pending_done = false;

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
	char buffer[256];
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

static void send_status(int fd) {
	pthread_mutex_lock(&state_mutex);
	if (pending_done) {
		sim_printf(fd, "done\n");
		pending_done = false;
	}
	sim_printf(
		fd,
		"%sA%sA%.2fA%.2fA%.2fA%.2fA%dA\n",
		options.model,
		options.firmware,
		close_position,
		open_position,
		current_position,
		voltage,
		brightness
	);
	pthread_mutex_unlock(&state_mutex);
}

static void update_cover_position(void) {
	pthread_mutex_lock(&state_mutex);
	if (do_open) {
		if (open_position - current_position > 15) {
			current_position += 15;
		} else {
			current_position = open_position;
			do_open = false;
			pending_done = true;
		}
	} else if (do_close) {
		if (current_position - close_position > 15) {
			current_position -= 15;
		} else {
			current_position = close_position;
			do_close = false;
			pending_done = true;
		}
	}
	pthread_mutex_unlock(&state_mutex);
}

static void *background(void *arg) {
	(void)arg;
	while (running) {
		send_status(serial_fd);
		usleep(1000000);
		update_cover_position();
	}
	return NULL;
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

static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;

	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '\r') {
			continue;
		}
		if (byte == '\n') {
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
	int command = atoi(buffer);
	const char *response = NULL;

	pthread_mutex_lock(&state_mutex);
	switch (command) {
		case 1000:
			do_open = false;
			do_close = true;
			break;
		case 1001:
			do_open = true;
			do_close = false;
			break;
		case 2000:
			heater = 0;
			break;
		case 2050:
			heater = 50;
			break;
		case 2100:
			heater = 100;
			break;
		case 2150:
			heater = 150;
			break;
		case 9999:
			brightness = 0;
			break;
		case 100000:
			close_position = current_position;
			response = "CloseSet\n";
			break;
		case 100001:
			open_position = current_position;
			response = "OpenSet\n";
			break;
		default:
			if (40000 <= command && command <= 67000) {
				open_position = (command - 40000) / 100.0;
			} else if (10000 <= command && command <= 12055) {
				close_position = (command - 10000) / 100.0;
			} else if (1 <= command && command <= 255) {
				brightness = command;
			} else {
				serial_simulator_trace_line(options.trace, "??", buffer);
			}
			break;
	}
	pthread_mutex_unlock(&state_mutex);

	if (response != NULL) {
		sim_printf(fd, "%s", response);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char command[80];
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
		printf("WandererAstro WandererCover V4-EC simulator is running on %s\n", port);
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, background, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		serial_fd = -1;
		return 1;
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
	pthread_join(thread, NULL);
	return 0;
}
