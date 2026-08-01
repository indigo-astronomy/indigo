// LACERTA Motorfocus focuser simulator
//
// Copyright (c) 2024-2026 CloudMakers, s. r. o.
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
	.model = "MFOC",
	.firmware = "3.1.123"
};

static const char *simulator_name = "focuser_lacerta";

static void usage(const char *name) {
	printf("LACERTA Motorfocus focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <MFOC|FMC>      Select simulated model, default is MFOC\n");
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
				fprintf(stderr, "--model requires MFOC or FMC\n");
				return false;
			}
			if (strcmp(argv[i], "MFOC") && strcmp(argv[i], "FMC")) {
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

static int position = 0;
static int target = 0;
static int direction = 0;
static int backlash = 3;
static int max_position = 250000;
static double temperature = 23.5;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

static void *background(void *arg) {
	(void)arg;
	while (running) {
		pthread_mutex_lock(&state_mutex);
		if (target < position) {
			position--;
		} else if (target > position) {
			position++;
		}
		pthread_mutex_unlock(&state_mutex);
		usleep(1000);
	}
	return NULL;
}

// ----------------------------------------------------------------- protocol

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
		if (byte == ':') {
			used = 0;
		}
		buffer[used++] = byte;
		if (byte == '#') {
			buffer[used] = '\0';
			serial_simulator_trace_line(options.trace, "->", buffer);
			return (int)used;
		}
	}

	buffer[0] = '\0';
	return -1;
}

static bool sim_printf(int fd, const char *format, ...) {
	char buffer[160];
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

static void handle_set_position(int fd, const char *command) {
	int value = atoi(command + 4);
	pthread_mutex_lock(&state_mutex);
	target = position = value;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "p %d\r", value);
}

static void handle_set_max_position(int fd, const char *command) {
	int value = atoi(command + 4);
	pthread_mutex_lock(&state_mutex);
	max_position = value;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "g %d\r", value);
}

static void handle_get_max_position(int fd) {
	pthread_mutex_lock(&state_mutex);
	int value = max_position;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "g %d\r", value);
}

static void handle_set_reverse(int fd, const char *command) {
	int value = atoi(command + 4) != 0 ? 1 : 0;
	pthread_mutex_lock(&state_mutex);
	direction = value;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "r %d\r", value);
}

static void handle_get_reverse(int fd) {
	pthread_mutex_lock(&state_mutex);
	int value = direction;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "r %d\r", value);
}

static void handle_get_position(int fd) {
	pthread_mutex_lock(&state_mutex);
	int value = position;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "p %d\r", value);
}

static void handle_move_absolute(const char *command) {
	int value = atoi(command + 4);
	pthread_mutex_lock(&state_mutex);
	if (value < 0) {
		target = 0;
	} else if (value > max_position) {
		target = max_position;
	} else {
		target = value;
	}
	pthread_mutex_unlock(&state_mutex);
}

static void handle_halt(int fd) {
	pthread_mutex_lock(&state_mutex);
	target = position;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "H 1\r");
}

static void handle_temperature(int fd) {
	pthread_mutex_lock(&state_mutex);
	double value = temperature;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "t %g\r", value);
}

static void handle_set_backlash(int fd, const char *command) {
	int value = atoi(command + 4);
	pthread_mutex_lock(&state_mutex);
	backlash = value;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "b %d\r", value);
}

static void handle_get_backlash(int fd) {
	pthread_mutex_lock(&state_mutex);
	int value = backlash;
	pthread_mutex_unlock(&state_mutex);
	sim_printf(fd, "b %d\r", value);
}

static void dispatch_command(int fd, const char *command) {
	if (!strcmp(command, ": i #")) {
		sim_printf(fd, "i %s\r", options.model);
	} else if (!strcmp(command, ": v #")) {
		sim_printf(fd, "v%s\r", options.firmware);
	} else if (!strncmp(command, ": P ", 4)) {
		handle_set_position(fd, command);
	} else if (!strncmp(command, ": G ", 4)) {
		handle_set_max_position(fd, command);
	} else if (!strcmp(command, ": g #")) {
		handle_get_max_position(fd);
	} else if (!strncmp(command, ": R ", 4)) {
		handle_set_reverse(fd, command);
	} else if (!strcmp(command, ": r #")) {
		handle_get_reverse(fd);
	} else if (!strcmp(command, ": q #")) {
		handle_get_position(fd);
	} else if (!strncmp(command, ": M ", 4)) {
		handle_move_absolute(command);
	} else if (!strcmp(command, ": H #")) {
		handle_halt(fd);
	} else if (!strcmp(command, ": t #")) {
		handle_temperature(fd);
	} else if (!strncmp(command, ": B ", 4)) {
		handle_set_backlash(fd, command);
	} else if (!strcmp(command, ": b #")) {
		handle_get_backlash(fd);
	} else {
		serial_simulator_trace_line(options.trace, "??", command);
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
		printf("LACERTA Motorfocus focuser simulator is running on %s\n", port);
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
