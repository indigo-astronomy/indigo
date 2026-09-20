// PegasusAstro FocusCube 2/3 simulator
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

// https://pegasusastro.com/command-list-for-focuscube3/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
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
	const char *profile;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.profile = "normal"
};

static void usage(const char *name) {
	printf("PegasusAstro FocusCube 3 simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <focuscube3>    Select simulated model, default is focuscube3\n");
	printf("  --device-id <id>        Override FocusCube 3 device id\n");
	printf("  --firmware <version>    Override firmware version\n");
	printf("  --profile <name>        normal, configured, no-handshake, bad-status or\n");
	printf("                          external-motion, default is normal\n");
	printf("  -h, --help              Show this help and exit\n");
	printf("\n");
	printf("INDIGO_FC3_EVENTS names a file receiving every accepted request, one per line.\n");
	printf("INDIGO_FC3_FAULT names a file holding '<command prefix> <silent|garbage|close>'\n");
	printf("which is applied once to the next matching request and then removed.\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int position = 0;
static int target = 0;
static int direction = 0;
static int backlash = 3;
static int speed = 400;
static char id[32] = "AA000000";
static char fw[32] = "1.4.1";
static double temperature = 23.5;
static FILE *events = NULL;

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
				fprintf(stderr, "--model requires focuscube3\n");
				return false;
			}
			if (strcmp(argv[i], "focuscube3") && strcmp(argv[i], "fc3")) {
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
		} else if (!strcmp(argv[i], "--profile")) {
			if (++i == argc) {
				fprintf(stderr, "--profile requires a name\n");
				return false;
			}
			options.profile = argv[i];
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// The controller state a scenario needs is selected once at startup, so a
// connecting driver reads back exactly the configuration under test.
static void apply_profile(void) {
	if (!strcmp(options.profile, "configured")) {
		position = target = 1234;
		backlash = 42;
		direction = 1;
		speed = 750;
		temperature = -3.25;
	} else if (!strcmp(options.profile, "external-motion")) {
		target = 5000;
	}
}

// One-shot fault injection. The control file names a command prefix and the
// way the next matching request has to misbehave, so a test can fail exactly
// one transaction without disturbing the rest of the session.
static const char *pending_fault(const char *command) {
	static char action[32];
	const char *path = getenv("INDIGO_FC3_FAULT");
	if (path == NULL) {
		return NULL;
	}
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	char prefix[32] = { 0 };
	action[0] = '\0';
	bool matched = fscanf(file, "%31s %31s", prefix, action) == 2 && !strncmp(command, prefix, strlen(prefix));
	fclose(file);
	if (!matched) {
		return NULL;
	}
	unlink(path);
	return action;
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

static void dispatch_command(int handle, const char *buffer) {
	pthread_mutex_lock(&state_mutex);
	if (events != NULL) {
		fprintf(events, "%s\n", buffer);
		fflush(events);
	}
	const char *fault = pending_fault(buffer);
	if (fault != NULL) {
		if (!strcmp(fault, "close")) {
			running = 0;
			if (serial_fd >= 0) {
				close(serial_fd);
				serial_fd = -1;
			}
			pthread_mutex_unlock(&state_mutex);
			return;
		}
		if (!strcmp(fault, "silent")) {
			pthread_mutex_unlock(&state_mutex);
			return;
		}
		if (!strcmp(fault, "garbage")) {
			sim_printf(handle, "ERR\n");
			pthread_mutex_unlock(&state_mutex);
			return;
		}
	}
	if (!strcmp(buffer, "F#") || !strcmp(buffer, "##")) {
		sim_printf(handle, strcmp(options.profile, "no-handshake") ? "FC3_%s_A\n" : "ERR_%s\n", id);
	} else if (!strcmp(buffer, "FA")) {
		if (!strcmp(options.profile, "bad-status")) {
			sim_printf(handle, "ERR\n");
		} else {
			sim_printf(handle, "FC3:%d:%d:%.2f:%d:%d\n", position, target == position ? 0 : 1, temperature, direction, backlash);
		}
	} else if (!strncmp(buffer, "FN:", 3)) {
		target = position = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "FM:", 3)) {
		target = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "FG:", 3)) {
		target += atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "FH")) {
		target = position;
		sim_printf(handle, "FH:1\n");
	} else if (!strcmp(buffer, "FT")) {
		sim_printf(handle, "FT:%.2f\n", temperature);
	} else if (!strcmp(buffer, "FI")) {
		sim_printf(handle, "FI:%d\n", target == position ? 0 : 1);
	} else if (!strcmp(buffer, "FV")) {
		sim_printf(handle, "FV:%s\n", fw);
	} else if (!strncmp(buffer, "FD:", 3)) {
		direction = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "SP")) {
		sim_printf(handle, "SP:%d\n", speed);
	} else if (!strncmp(buffer, "SP:", 3)) {
		speed = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "BL:", 3)) {
		backlash = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	}
	pthread_mutex_unlock(&state_mutex);
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char port[128];
	char buffer[128];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	apply_profile();
	const char *journal = getenv("INDIGO_FC3_EVENTS");
	events = journal == NULL ? NULL : fopen(journal, "w");

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuscube3", port)) {
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	if (!options.headless) {
		printf("PegasusAstro FocusCube v3 simulator is running on %s\n", port);
		fflush(stdout);
	}

	pthread_create(&thread, NULL, background, NULL);

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
	if (events != NULL) {
		fclose(events);
	}
	return 0;
}
