// Optec FocusLynx focuser simulator
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
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL
};

static const char *simulator_name = "focuser_optecfl";

static void usage(const char *name) {
	printf("Optec FocusLynx focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <focuslynx>     Select simulated model, default is focuslynx\n");
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
				fprintf(stderr, "--model requires focuslynx\n");
				return false;
			}
			if (strcmp(argv[i], "focuslynx")) {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
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

static int position[2] = { 0, 0 };
static int max_position[2] = { 10000, 10000 };
static int target[2] = { 0, 0 };
static double temperature[2] = { 21.7, 22.1 };
static char focuser_type[2][3] = { "OA", "OA" };

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
		for (int focuser = 0; focuser < 2; focuser++) {
			if (target[focuser] < position[focuser]) {
				position[focuser]--;
			} else if (target[focuser] > position[focuser]) {
				position[focuser]++;
			}
		}
		pthread_mutex_unlock(&state_mutex);
		usleep(10000);
	}
	return NULL;
}

// ----------------------------------------------------------------- protocol

static int focuser_index(const char *command) {
	if (command[2] == '1') {
		return 0;
	}
	if (command[2] == '2') {
		return 1;
	}
	return -1;
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
		if (byte == '<') {
			used = 0;
		}
		buffer[used++] = byte;
		if (byte == '>') {
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

static void handle_get_hub_info(int fd) {
	sim_printf(fd, "!\n");
	sim_printf(fd, "STATUS\n");
	sim_printf(fd, "Hub FVer = 1.0.0\n");
	sim_printf(fd, "END\n");
}

static void handle_get_config(int fd, const char *command) {
	int focuser = focuser_index(command);
	if (focuser < 0) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	int max = max_position[focuser];
	char type[3];
	snprintf(type, sizeof(type), "%s", focuser_type[focuser]);
	pthread_mutex_unlock(&state_mutex);

	sim_printf(fd, "!\n");
	sim_printf(fd, "CONFIG%d\n", focuser + 1);
	sim_printf(fd, "Max Pos = %d\n", max);
	sim_printf(fd, "Dev Type = %s\n", type);
	sim_printf(fd, "END\n");
}

static void handle_get_status(int fd, const char *command) {
	int focuser = focuser_index(command);
	if (focuser < 0) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	double temp = temperature[focuser];
	int current = position[focuser];
	int goal = target[focuser];
	int moving = current == goal ? 0 : 1;
	pthread_mutex_unlock(&state_mutex);

	sim_printf(fd, "!\n");
	sim_printf(fd, "STATUS%d\n", focuser + 1);
	sim_printf(fd, "Temp(C) = %+.1f\n", temp);
	sim_printf(fd, "Curr Pos = %d\n", current);
	sim_printf(fd, "Targ Pos = %d\n", goal);
	sim_printf(fd, "IsMoving = %d\n", moving);
	sim_printf(fd, "TmpProbe = 1\n");
	sim_printf(fd, "END\n");
}

static void handle_halt(int fd, const char *command) {
	int focuser = focuser_index(command);
	if (focuser < 0) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	target[focuser] = position[focuser];
	pthread_mutex_unlock(&state_mutex);

	sim_printf(fd, "!\n");
	sim_printf(fd, "HALTED\n");
}

static void handle_move_absolute(int fd, const char *command) {
	int focuser = focuser_index(command);
	if (focuser < 0) {
		return;
	}

	int value = atoi(command + 5);
	pthread_mutex_lock(&state_mutex);
	if (value <= max_position[focuser]) {
		target[focuser] = value;
	} else {
		target[focuser] = max_position[focuser];
	}
	pthread_mutex_unlock(&state_mutex);

	sim_printf(fd, "!\n");
	sim_printf(fd, "M\n");
}

static void handle_sync_position(int fd, const char *command) {
	int focuser = focuser_index(command);
	if (focuser < 0) {
		return;
	}

	int value = atoi(command + 7);
	pthread_mutex_lock(&state_mutex);
	if (value <= max_position[focuser]) {
		target[focuser] = position[focuser] = value;
	} else {
		target[focuser] = position[focuser] = max_position[focuser];
	}
	pthread_mutex_unlock(&state_mutex);

	sim_printf(fd, "!\n");
	sim_printf(fd, "SET\n");
}

static void handle_set_device_type(int fd, const char *command) {
	int focuser = focuser_index(command);
	if (focuser < 0) {
		return;
	}

	pthread_mutex_lock(&state_mutex);
	focuser_type[focuser][0] = command[7];
	focuser_type[focuser][1] = command[8];
	focuser_type[focuser][2] = '\0';
	pthread_mutex_unlock(&state_mutex);

	sim_printf(fd, "!\n");
	sim_printf(fd, "SET\n");
}

static void dispatch_command(int fd, const char *command) {
	if (!strcmp(command, "<FHGETHUBINFO>")) {
		handle_get_hub_info(fd);
	} else if (!strcmp(command, "<F1GETCONFIG>") || !strcmp(command, "<F2GETCONFIG>")) {
		handle_get_config(fd, command);
	} else if (!strcmp(command, "<F1GETSTATUS>") || !strcmp(command, "<F2GETSTATUS>")) {
		handle_get_status(fd, command);
	} else if (!strcmp(command, "<F1HALT>") || !strcmp(command, "<F2HALT>")) {
		handle_halt(fd, command);
	} else if (!strncmp(command, "<F1MA", 5) || !strncmp(command, "<F2MA", 5)) {
		handle_move_absolute(fd, command);
	} else if (!strncmp(command, "<F1SCCP", 7) || !strncmp(command, "<F2SCCP", 7)) {
		handle_sync_position(fd, command);
	} else if (!strncmp(command, "<F1SCDT", 7) || !strncmp(command, "<F2SCDT", 7)) {
		handle_set_device_type(fd, command);
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
		printf("Optec FocusLynx focuser simulator is running on %s\n", port);
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
