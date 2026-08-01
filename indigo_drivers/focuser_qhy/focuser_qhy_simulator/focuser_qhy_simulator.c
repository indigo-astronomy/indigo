// QHY Q-Focuser simulator
//
// Copyright (c) 2026 Rumen G. Bogdanovski
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

// Implements the JSON object protocol used by the QHY Q-Focuser.
// Requests and responses are framed by matching braces.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <time.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	bool no_out_temp;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.no_out_temp = false
};

static void usage(const char *name) {
	printf("QHY Q-Focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <qhy-focuser>   Select simulated model, default is qhy-focuser\n");
	printf("  --firmware <version>    Override firmware version\n");
	printf("  --board-version <value> Override board version\n");
	printf("  -T, --no-out-temp       Simulate missing outside temperature sensor\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t position = 50000;
static int32_t target = 50000;
static int speed = 1;
static bool reverse = false;
static double chip_temperature = 30.2;
static double outside_temperature = 18.5;
static char firmware[32] = "1.2.3";
static char board_version[32] = "QFOCUSER-1.0";

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
	int tick = 0;

	while (running) {
		pthread_mutex_lock(&state_mutex);
		if (position != target) {
			int step = 9 - speed;
			if (step < 1) {
				step = 1;
			}
			int32_t delta = target - position;
			if (delta > 0) {
				position += delta < step ? delta : step;
			} else {
				position -= -delta < step ? -delta : step;
			}
		}

		if (++tick % 10 == 0) {
			chip_temperature += (((double)random() / RAND_MAX) - 0.5) * 0.05;
			if (chip_temperature < 20.0) {
				chip_temperature = 20.0;
			} else if (chip_temperature > 50.0) {
				chip_temperature = 50.0;
			}
			if (!options.no_out_temp) {
				outside_temperature += (((double)random() / RAND_MAX) - 0.5) * 0.05;
				if (outside_temperature < 5.0) {
					outside_temperature = 5.0;
				} else if (outside_temperature > 35.0) {
					outside_temperature = 35.0;
				}
			}
		}
		pthread_mutex_unlock(&state_mutex);

		usleep(100000);
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
				fprintf(stderr, "--model requires qhy-focuser\n");
				return false;
			}
			if (strcmp(argv[i], "qhy-focuser") && strcmp(argv[i], "qhy") && strcmp(argv[i], "q-focuser")) {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a value\n");
				return false;
			}
			snprintf(firmware, sizeof(firmware), "%s", argv[i]);
		} else if (!strcmp(argv[i], "--board-version")) {
			if (++i == argc) {
				fprintf(stderr, "--board-version requires a value\n");
				return false;
			}
			snprintf(board_version, sizeof(board_version), "%s", argv[i]);
		} else if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "--no-out-temp")) {
			options.no_out_temp = true;
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- protocol

static int sim_read_request(int fd, char *buffer, int length) {
	int total_bytes = 0;
	int depth = 0;
	bool started = false;

	while (running && total_bytes < length - 1) {
		char c;
		ssize_t bytes_read = read(fd, &c, 1);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (!started) {
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
		if (!started) {
			if (c != '{') {
				continue;
			}
			started = true;
		}
		buffer[total_bytes++] = c;
		if (c == '{') {
			depth++;
		} else if (c == '}') {
			if (--depth <= 0) {
				break;
			}
		}
	}

	buffer[total_bytes] = '\0';
	if (*buffer) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return total_bytes;
}

static bool sim_printf(int fd, const char *format, ...) {
	char buffer[256];
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

	serial_simulator_trace_line(options.trace, "<-", buffer);
	return serial_simulator_write_all(fd, buffer, (size_t)length);
}

static bool json_get_int(const char *json, const char *key, int *value) {
	char pattern[64];
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);

	const char *p = strstr(json, pattern);
	if (p == NULL) {
		return false;
	}
	p += strlen(pattern);
	while (*p == ' ' || *p == ':' || *p == '\t') {
		p++;
	}
	if (*p == '\0') {
		return false;
	}

	char *end = NULL;
	long parsed_value = strtol(p, &end, 10);
	if (end == p) {
		return false;
	}
	*value = (int)parsed_value;
	return true;
}

static void dispatch_command(int fd, const char *command) {
	int cmd_id = -1;
	if (!json_get_int(command, "cmd_id", &cmd_id)) {
		serial_simulator_trace_line(options.trace, "!!", "missing cmd_id");
		return;
	}

	switch (cmd_id) {
		case 1:
			sim_printf(fd, "{\"idx\":1,\"version\":\"%s\",\"bv\":\"%s\"}", firmware, board_version);
			break;
		case 3:
			pthread_mutex_lock(&state_mutex);
			target = position;
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "{\"idx\":3}");
			break;
		case 4: {
			int outside_temp;
			int chip_temp;

			pthread_mutex_lock(&state_mutex);
			outside_temp = (int)((options.no_out_temp ? -127.0 : outside_temperature) * 1000.0);
			chip_temp = (int)(chip_temperature * 1000.0);
			pthread_mutex_unlock(&state_mutex);

			sim_printf(fd, "{\"idx\":4,\"o_t\":%d,\"c_t\":%d,\"c_r\":0}", outside_temp, chip_temp);
			break;
		}
		case 5: {
			int32_t current_position;

			pthread_mutex_lock(&state_mutex);
			current_position = position;
			pthread_mutex_unlock(&state_mutex);

			sim_printf(fd, "{\"idx\":5,\"pos\":%d}", current_position);
			break;
		}
		case 6: {
			int target_position;

			if (json_get_int(command, "tar", &target_position)) {
				pthread_mutex_lock(&state_mutex);
				target = target_position;
				pthread_mutex_unlock(&state_mutex);
			}
			sim_printf(fd, "{\"idx\":6}");
			break;
		}
		case 7: {
			int reverse_state;

			if (json_get_int(command, "rev", &reverse_state)) {
				pthread_mutex_lock(&state_mutex);
				reverse = reverse_state != 0;
				pthread_mutex_unlock(&state_mutex);
			}
			sim_printf(fd, "{\"idx\":7}");
			break;
		}
		case 11: {
			int sync_position;

			if (json_get_int(command, "init_val", &sync_position)) {
				pthread_mutex_lock(&state_mutex);
				position = sync_position;
				target = sync_position;
				pthread_mutex_unlock(&state_mutex);
			}
			sim_printf(fd, "{\"idx\":11}");
			break;
		}
		case 13: {
			int new_speed;

			if (json_get_int(command, "speed", &new_speed)) {
				if (new_speed < 1) {
					new_speed = 1;
				} else if (new_speed > 8) {
					new_speed = 8;
				}
				pthread_mutex_lock(&state_mutex);
				speed = new_speed;
				pthread_mutex_unlock(&state_mutex);
			}
			sim_printf(fd, "{\"idx\":13}");
			break;
		}
		case 16:
			sim_printf(fd, "{\"idx\":16}");
			break;
		default:
			sim_printf(fd, "{\"idx\":-1}");
			break;
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char port[PATH_MAX];
	char request[256];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	srandom((unsigned int)time(NULL));

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "qhy_focuser", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("QHY Q-Focuser simulator is running on %s\n", port);
		printf("Press Ctrl+C to exit\n");
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, background, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		return 1;
	}

	while (running) {
		int length = sim_read_request(serial_fd, request, sizeof(request));
		if (length < 0) {
			break;
		}
		if (length == 0) {
			usleep(1000);
			continue;
		}
		dispatch_command(serial_fd, request);
	}

	running = 0;
	pthread_join(thread, NULL);
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
