// PegasusAstro Falcon/Falcon2 rotator simulator
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

// https://pegasusastro.com/command-list-for-falconv2/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
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

static void usage(const char *name) {
	printf("PegasusAstro Falcon/Falcon2 rotator simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <falcon1|falcon2>\n");
	printf("                          Select simulated model, default is falcon2\n");
	printf("  --device-id <id>        Override Falcon2 device id\n");
	printf("  --firmware <version>    Override firmware version\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int version = 2;
static double position = 0.0;
static double target = 0.0;
static int direction = 0;
static char id[32] = "AA000000";
static char fw[32] = "1.5";

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
		if (fabs(target - position) <= 0.01) {
			position = target;
		} else if (target < position) {
			position -= 0.01;
		} else if (target > position) {
			position += 0.01;
		}
		pthread_mutex_unlock(&state_mutex);
		usleep(1000);
	}
	return NULL;
}

// ----------------------------------------------------------------- runtime

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
				fprintf(stderr, "--model requires falcon1 or falcon2\n");
				return false;
			}
			if (!strcmp(argv[i], "falcon1")) {
				version = 1;
			} else if (!strcmp(argv[i], "falcon2")) {
				version = 2;
			} else {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
		} else if (!strcmp(argv[i], "--device-id")) {
			if (++i == argc) {
				fprintf(stderr, "--device-id requires a value\n");
				return false;
			}
			snprintf(id, sizeof(id), "%s", argv[i]);
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a value\n");
				return false;
			}
			snprintf(fw, sizeof(fw), "%s", argv[i]);
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- protocol

static int sim_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;

	while (running && total_bytes < length - 1) {
		ssize_t bytes_read = read(handle, &c, 1);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (total_bytes == 0) {
					return 0;
				}
				usleep(1000);
				continue;
			}
			if (errno == EIO) {
				return 0;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return 0;
		}
		if (c == '\n') {
			break;
		}
		if (c == '\r') {
			continue;
		}
		buffer[total_bytes++] = c;
	}
	buffer[total_bytes] = '\0';
	if (*buffer) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return total_bytes;
}

static bool sim_printf(int handle, const char *format, ...) {
	char buffer[128];
	va_list args;

	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0) {
		return false;
	}
	if ((size_t)length >= sizeof(buffer)) {
		length = (int)sizeof(buffer) - 1;
	}

	if (options.trace) {
		fprintf(stderr, "<- %s", buffer);
	}
	return serial_simulator_write_all(handle, buffer, (size_t)length);
}

static void dispatch_command(int fd, const char *command) {
	double current_position;
	int current_direction;
	int is_moving;

	pthread_mutex_lock(&state_mutex);
	current_position = position;
	current_direction = direction;
	is_moving = target == position ? 0 : 1;
	pthread_mutex_unlock(&state_mutex);

	if (!strcmp(command, "F#")) {
		if (version == 1) {
			sim_printf(fd, "FR_OK\n");
		} else if (version == 2) {
			sim_printf(fd, "F2R_%s_A\n", id);
		}
	} else if (!strcmp(command, "FA")) {
		if (version == 1) {
			sim_printf(fd, "FR_OK:0:%.2f:%d:0:0:%d\n", current_position, is_moving, current_direction);
		} else if (version == 2) {
			sim_printf(fd, "F2R:%.2f:%d:0:0:%d\n", current_position, is_moving, current_direction);
		}
	} else if (!strncmp(command, "SD:", 3)) {
		pthread_mutex_lock(&state_mutex);
		target = position = atof(command + 3);
		pthread_mutex_unlock(&state_mutex);
		sim_printf(fd, "%s\n", command);
	} else if (!strncmp(command, "MD:", 3)) {
		pthread_mutex_lock(&state_mutex);
		target = atof(command + 3);
		pthread_mutex_unlock(&state_mutex);
		sim_printf(fd, "%s\n", command);
	} else if (!strcmp(command, "FH")) {
		pthread_mutex_lock(&state_mutex);
		target = position;
		pthread_mutex_unlock(&state_mutex);
		sim_printf(fd, "FH:1\n");
	} else if (!strcmp(command, "FD")) {
		sim_printf(fd, "FD:%.2f\n", current_position);
	} else if (!strcmp(command, "FR")) {
		sim_printf(fd, "FR:%d\n", is_moving);
	} else if (!strcmp(command, "FV")) {
		sim_printf(fd, "FV:%s\n", fw);
	} else if (!strcmp(command, "DR:0")) {
		sim_printf(fd, "DR:0\n");
	} else if (!strncmp(command, "FN:", 3)) {
		pthread_mutex_lock(&state_mutex);
		direction = atoi(command + 3);
		pthread_mutex_unlock(&state_mutex);
		sim_printf(fd, "%s\n", command);
	}
}

int main(int argc, char *argv[]) {
	pthread_t thread;
	char buffer[128];
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

	char simulator_name[32];
	snprintf(simulator_name, sizeof(simulator_name), "falcon%d", version);
	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	if (!options.headless) {
		printf("PegasusAstro Falcon v%d rotator simulator is running on %s\n", version, port);
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, background, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	while (running) {
		int count = sim_read_line(serial_fd, buffer, sizeof(buffer));
		if (count > 0) {
			dispatch_command(serial_fd, buffer);
		} else if (count == 0) {
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
