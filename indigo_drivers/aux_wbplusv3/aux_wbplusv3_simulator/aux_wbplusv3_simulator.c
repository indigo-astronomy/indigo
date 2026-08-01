// WandererBox Plus V3 powerbox simulator
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
	.model = "ZXWBPlusV3",
	.firmware = "20230926"
};

static const char *simulator_name = "aux_wbplusv3";

static void usage(const char *name) {
	printf("WandererBox Plus V3 powerbox simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <ZXWBPlusV3>    Select simulated model, default is ZXWBPlusV3\n");
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
				fprintf(stderr, "--model requires ZXWBPlusV3\n");
				return false;
			}
			if (strcmp(argv[i], "ZXWBPlusV3")) {
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

static bool dc2 = false;
static bool dc4_6 = false;
static bool usb = false;
static int dc3_pwm = 0;
static double dc2_voltage = 5.0;
static double temperature_1 = 23.94;
static double temperature_2 = 24.70;
static double humidity = 36.90;
static double current = 0.26;
static double voltage = 12.11;

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
	sim_printf(
		fd,
		"%sA%sA%.2fA%.2fA%.2fA%.2fA%.2fA%dA%dA%dA%dA%d\n",
		options.model,
		options.firmware,
		temperature_1,
		humidity,
		temperature_2,
		current,
		voltage,
		usb ? 1 : 0,
		dc2 ? 1 : 0,
		dc3_pwm,
		dc4_6 ? 1 : 0,
		(int)(dc2_voltage * 10)
	);
	pthread_mutex_unlock(&state_mutex);
}

static void *background(void *arg) {
	(void)arg;
	while (running) {
		send_status(serial_fd);
		usleep(1000000);
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

static void dispatch_command(const char *buffer) {
	int command = atoi(buffer);

	pthread_mutex_lock(&state_mutex);
	switch (command) {
		case 66300744:
			break;
		case 121:
			dc2 = true;
			break;
		case 120:
			dc2 = false;
			break;
		case 101:
			dc4_6 = true;
			break;
		case 100:
			dc4_6 = false;
			break;
		case 111:
			usb = true;
			break;
		case 110:
			usb = false;
			break;
		default:
			if (20000 <= command && command <= 29132) {
				dc2_voltage = (command - 20000) / 10.0;
			} else if (3000 <= command && command <= 3255) {
				dc3_pwm = command - 3000;
			} else {
				serial_simulator_trace_line(options.trace, "??", buffer);
			}
			break;
	}
	pthread_mutex_unlock(&state_mutex);
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
		printf("WandererBox Plus V3 powerbox simulator is running on %s\n", port);
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
			dispatch_command(command);
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
