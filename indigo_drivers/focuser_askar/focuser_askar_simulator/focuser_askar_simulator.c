// Askar-WAF USB CDC focuser simulator
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

// Implements the F...# serial command set documented in
// Commands_Focuser_CDC_EN.md.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <time.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

#define ASKAR_FRAME_MAX        31
#define ASKAR_MAX_TRAVEL_MIN   100
#define ASKAR_MAX_TRAVEL_MAX   1000000
#define ASKAR_BACKLASH_MAX     10000

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
	printf("Askar-WAF focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <askar-waf>     Select simulated model, default is askar-waf\n");
	printf("  --firmware <version>    Override firmware version\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t position = 50000;
static int32_t target = 50000;
static int32_t max_step = 100000;
static int backlash = 0;
static int reverse = 0;
static int motor_mode = 0;
static char firmware[32] = "1.1.0";
static char model_name[32] = "Askar-WAF";

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
		if (position != target) {
			int32_t delta = target - position;
			int32_t step = 200;
			if (delta > 0) {
				position += delta < step ? delta : step;
			} else {
				position -= -delta < step ? -delta : step;
			}
		}
		pthread_mutex_unlock(&state_mutex);
		usleep(50000);
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
				fprintf(stderr, "--model requires askar-waf\n");
				return false;
			}
			if (strcmp(argv[i], "askar-waf") && strcmp(argv[i], "askar") && strcmp(argv[i], "waf")) {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a value\n");
				return false;
			}
			snprintf(firmware, sizeof(firmware), "%s", argv[i]);
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- protocol

static bool sim_printf(int fd, const char *format, ...) {
	char buffer[128];
	char frame[160];
	va_list args;

	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0) {
		return false;
	}
	if ((size_t)length >= sizeof(buffer)) {
		length = (int)sizeof(buffer) - 1;
		buffer[length] = '\0';
	}

	snprintf(frame, sizeof(frame), "%s#\r\n", buffer);
	serial_simulator_trace_line(options.trace, "<-", frame);
	return serial_simulator_write_all(fd, frame, strlen(frame));
}

static void sim_send_error(int fd) {
	sim_printf(fd, "FE");
}

static int sim_read_frame(int fd, char *buffer, int length) {
	int total_bytes = 0;
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
			return -3;
		}
		if (bytes_read == 0) {
			return 0;
		}
		if (!started) {
			if (c != 'F') {
				continue;
			}
			started = true;
			buffer[total_bytes++] = c;
			continue;
		}
		if (c == '#') {
			buffer[total_bytes] = '\0';
		serial_simulator_trace_line(options.trace, "->", buffer);
			return total_bytes;
		}
		if (c == '\r' || c == '\n') {
			buffer[total_bytes] = '\0';
			return -2;
		}
		buffer[total_bytes++] = c;
		if (total_bytes > ASKAR_FRAME_MAX) {
			while (read(fd, &c, 1) == 1 && c != '#') {
				;
			}
			return -1;
		}
	}
	return -1;
}

static bool parse_int(const char *s, int32_t *value) {
	if (*s == '\0') {
		return false;
	}

	char *end = NULL;
	long parsed_value = strtol(s, &end, 10);
	if (end == s) {
		return false;
	}
	while (*end) {
		if (!isspace((unsigned char)*end)) {
			return false;
		}
		end++;
	}
	*value = (int32_t)parsed_value;
	return true;
}

static int32_t clamp(int32_t value, int32_t min, int32_t max) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

static void dispatch_command(int fd, const char *frame, int length) {
	if (length < 2) {
		sim_send_error(fd);
		return;
	}

	char cmd = frame[1];
	const char *body = frame + 2;

	switch (cmd) {
		case 'P': {
			int32_t value;
			if (!parse_int(body, &value)) {
				sim_send_error(fd);
				break;
			}
			pthread_mutex_lock(&state_mutex);
			target = clamp(value, 0, max_step);
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "FP%d", value);
			break;
		}
		case 'T': {
			int32_t value;
			if ((*body != '+' && *body != '-') || !parse_int(body, &value)) {
				sim_send_error(fd);
				break;
			}
			pthread_mutex_lock(&state_mutex);
			target = clamp(position + value, 0, max_step);
			pthread_mutex_unlock(&state_mutex);
			if (value >= 0) {
				sim_printf(fd, "FT+%d", value);
			} else {
				sim_printf(fd, "FT%d", value);
			}
			break;
		}
		case 'S':
			pthread_mutex_lock(&state_mutex);
			target = position;
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "FS");
			break;
		case 'Y': {
			int32_t value;
			if (!parse_int(body, &value)) {
				sim_send_error(fd);
				break;
			}
			pthread_mutex_lock(&state_mutex);
			position = clamp(value, 0, max_step);
			target = position;
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "FY%d", position);
			break;
		}
		case 'p': {
			int32_t current_position;

			pthread_mutex_lock(&state_mutex);
			current_position = position;
			pthread_mutex_unlock(&state_mutex);

			sim_printf(fd, "Fp%d", current_position);
			break;
		}
		case 'Q': {
			bool moving;

			pthread_mutex_lock(&state_mutex);
			moving = position != target;
			pthread_mutex_unlock(&state_mutex);

			sim_printf(fd, moving ? "FQ1" : "FQ0");
			break;
		}
		case 'm':
		case 'M':
			if (cmd == 'M' && *body) {
				int32_t value;
				if (!parse_int(body, &value) || value < ASKAR_MAX_TRAVEL_MIN || value > ASKAR_MAX_TRAVEL_MAX) {
					sim_send_error(fd);
					break;
				}
				pthread_mutex_lock(&state_mutex);
				if (position > value) {
					pthread_mutex_unlock(&state_mutex);
					sim_send_error(fd);
					break;
				}
				max_step = value;
				target = position;
				pthread_mutex_unlock(&state_mutex);
				sim_printf(fd, "FM%d", value);
			} else {
				pthread_mutex_lock(&state_mutex);
				int32_t value = max_step;
				pthread_mutex_unlock(&state_mutex);
				sim_printf(fd, "Fm%d", value);
			}
			break;
		case 'X': {
			int32_t value;
			if (!parse_int(body, &value) || value < ASKAR_MAX_TRAVEL_MIN || value > ASKAR_MAX_TRAVEL_MAX) {
				sim_send_error(fd);
				break;
			}
			pthread_mutex_lock(&state_mutex);
			if (position > value) {
				pthread_mutex_unlock(&state_mutex);
				sim_send_error(fd);
				break;
			}
			max_step = value;
			target = position;
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "FX%d", value);
			break;
		}
		case 'V':
			sim_printf(fd, "FV%s", firmware);
			break;
		case 'I':
			sim_printf(fd, "FI%s", model_name);
			break;
		case 'b':
			pthread_mutex_lock(&state_mutex);
			int current_backlash = backlash;
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "Fb%d", current_backlash);
			break;
		case 'B': {
			int32_t value;
			if (!parse_int(body, &value)) {
				sim_send_error(fd);
				break;
			}
			value = clamp(value, 0, ASKAR_BACKLASH_MAX);
			pthread_mutex_lock(&state_mutex);
			backlash = (int)value;
			pthread_mutex_unlock(&state_mutex);
			sim_printf(fd, "FB%d", value);
			break;
		}
		case 'r':
		case 'R':
			if (cmd == 'R' && *body) {
				int32_t value;
				if (!parse_int(body, &value) || (value != 0 && value != 1)) {
					sim_send_error(fd);
					break;
				}
				pthread_mutex_lock(&state_mutex);
				reverse = (int)value;
				target = position;
				pthread_mutex_unlock(&state_mutex);
				sim_printf(fd, "FR%d", value);
			} else {
				pthread_mutex_lock(&state_mutex);
				int value = reverse;
				pthread_mutex_unlock(&state_mutex);
				sim_printf(fd, value ? "Fr1" : "Fr0");
			}
			break;
		case 'o':
		case 'O':
			if (cmd == 'O' && *body) {
				int32_t value;
				if (!parse_int(body, &value) || (value != 0 && value != 1)) {
					sim_send_error(fd);
					break;
				}
				pthread_mutex_lock(&state_mutex);
				motor_mode = (int)value;
				target = position;
				pthread_mutex_unlock(&state_mutex);
				sim_printf(fd, "FO%d", value);
			} else {
				pthread_mutex_lock(&state_mutex);
				int value = motor_mode;
				pthread_mutex_unlock(&state_mutex);
				sim_printf(fd, value ? "Fo1" : "Fo0");
			}
			break;
		default:
			sim_send_error(fd);
			break;
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char port[PATH_MAX];
	char frame[64];

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

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "askar_waf", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("Askar-WAF focuser simulator is running on %s\n", port);
		printf("Press Ctrl+C to exit\n");
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, background, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		return 1;
	}

	while (running) {
		int length = sim_read_frame(serial_fd, frame, sizeof(frame));
		if (length == -1) {
			sim_send_error(serial_fd);
			continue;
		}
		if (length < 0) {
			continue;
		}
		if (length == 0) {
			usleep(1000);
			continue;
		}
		dispatch_command(serial_fd, frame, length);
	}

	running = 0;
	pthread_join(thread, NULL);
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
