// PegasusAstro UPB3 simulator
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

// https://pegasusastro.com/command-list-for-upbv3/

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
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL
};

static void usage(const char *name) {
	printf("PegasusAstro UPB v3 simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <upb3>          Select simulated model, default is upb3\n");
	printf("  --device-id <id>        Override UPB v3 device id\n");
	printf("  --firmware <version>    Override firmware version\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int version = 3;
static int power[] = { 0, 0, 0, 0, 0, 0 };
static int heat[] = { 0, 0, 0 };
static int buck = 0;
static int boost = 0;
static int relay = 0;
static int usb[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static int autodew[] = { 0, 0, 0 };
static int buck_voltage = 3;
static int boost_voltage = 12;
static int position = 0;
static int target = 0;
static int direction = 0;
static int speed = 400;
static char id[32] = "AA000000";
static char fw[32] = "1.4.1";

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
				fprintf(stderr, "--model requires upb3\n");
				return false;
			}
			if (!strcmp(argv[i], "upb3")) {
				version = 3;
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

static void dispatch_command(int handle, const char *buffer) {
	pthread_mutex_lock(&state_mutex);
	if (!strcmp(buffer, "P#") || !strcmp(buffer, "##")) {
		sim_printf(handle, "UPBv3_%s_A\n", id);
	} else if (!strcmp(buffer, "PV")) {
		sim_printf(handle, "PV:%s\n", fw);
	} else if (!strcmp(buffer, "PA")) {
		sim_printf(handle, "PA:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", power[0], power[1], power[2], power[3], power[4], power[5], heat[0], heat[1], heat[2], buck, boost, relay);
	} else if (!strcmp(buffer, "UA")) {
		sim_printf(handle, "UA:%d:%d:%d:%d:%d:%d:%d:%d\n", usb[0], usb[1], usb[2], usb[3], usb[4], usb[5], usb[6], usb[7]);
	} else if (!strncmp(buffer, "P1:", 3)) {
		power[0] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "P2:", 3)) {
		power[1] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "P3:", 3)) {
		power[2] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "P4:", 3)) {
		power[3] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "P5:", 3)) {
		power[4] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "P6:", 3)) {
		power[5] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U1:", 3)) {
		usb[0] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U2:", 3)) {
		usb[1] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U3:", 3)) {
		usb[2] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U4:", 3)) {
		usb[3] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U5:", 3)) {
		usb[4] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U6:", 3)) {
		usb[5] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U7:", 3)) {
		usb[6] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "U8:", 3)) {
		usb[7] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "D1:", 3)) {
		heat[0] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "D2:", 3)) {
		heat[1] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "D3:", 3)) {
		heat[2] = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "PS")) {
		sim_printf(handle, "PS:1\n");
	} else if (!strcmp(buffer, "US")) {
		sim_printf(handle, "US:1\n");
	} else if (!strcmp(buffer, "DSTR")) {
		sim_printf(handle, "DSTR:1\n");
	} else if (!strcmp(buffer, "AJ")) {
		sim_printf(handle, "AJ:%d:%d:%d:%d\n", buck_voltage, buck, boost_voltage, boost);
	} else if (!strcmp(buffer, "IS")) {
		sim_printf(handle, "IS:0:0:0:1.2:0:0:000100\n");
	} else if (!strcmp(buffer, "VR")) {
		sim_printf(handle, "VR:12.3:2.2\n");
	} else if (!strcmp(buffer, "PC")) {
		sim_printf(handle, "PC:2.1:13.2:10.2:3324\n");
	} else if (!strncmp(buffer, "RL:", 3)) {
		relay = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "PJ:", 3)) {
		int value = atoi(buffer + 3);
		if (value == 0) {
			buck = 0;
		} else if (value == 1) {
			buck = 1;
		} else {
			buck_voltage = value;
		}
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "PB:", 3)) {
		int value = atoi(buffer + 3);
		if (value == 0) {
			boost = 0;
		} else if (value == 1) {
			boost = 1;
		} else {
			boost_voltage = value;
		}
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "ADW1:", 5)) {
		autodew[0] = atoi(buffer + 5);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "ADW2:", 5)) {
		autodew[1] = atoi(buffer + 5);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "ADW3:", 5)) {
		autodew[2] = atoi(buffer + 5);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "PD")) {
		sim_printf(handle, "PD:%d%d%d\n", autodew[0], autodew[1], autodew[2]);
	} else if (!strncmp(buffer, "DA:", 3)) {
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "PL:", 3)) {
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "ES")) {
		sim_printf(handle, "ES:22.5:50.1:12.2\n");
	} else if (!strcmp(buffer, "SA")) {
		sim_printf(handle, "SA:%d:%d:%d:%d:1:0:1\n", position, target == position ? 0 : 1, direction, speed);
	} else if (!strcmp(buffer, "SP")) {
		sim_printf(handle, "SP:%d\n", position);
	} else if (!strncmp(buffer, "SC:", 3)) {
		target = position = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "SM:", 3)) {
		target = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "SG:", 3)) {
		target += atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "SH")) {
		target = position;
		sim_printf(handle, "SH:1\n");
	} else if (!strcmp(buffer, "SI")) {
		sim_printf(handle, "SI:%d\n", target == position ? 0 : 1);
	} else if (!strncmp(buffer, "SR:", 3)) {
		direction = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strncmp(buffer, "SS:", 3)) {
		speed = atoi(buffer + 3);
		sim_printf(handle, "%s\n", buffer);
	} else if (!strcmp(buffer, "PF")) {
	} else if (!strncmp(buffer, "BL:", 3)) {
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

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "upb3", port)) {
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	if (!options.headless) {
		printf("PegasusAstro UPB v%d simulator is running on %s\n", version, port);
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
	return 0;
}
