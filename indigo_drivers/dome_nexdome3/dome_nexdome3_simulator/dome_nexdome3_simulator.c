// NexDome 3 serial simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
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

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

#define ROTATOR_RANGE 55080
#define SHUTTER_RANGE 46000

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
};

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static const char *simulator_name = "dome_nexdome3";

static int rotator_position = 0;
static int shutter_position = 0;
static int home_position = 0;
static int park_position = 27540;
static int move_threshold = 300;
static int rotator_acceleration = 1500;
static int shutter_acceleration = 1500;
static int rotator_velocity = 600;
static int shutter_velocity = 800;

static void usage(const char *name) {
	printf("NexDome 3 serial simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
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
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

static bool sim_read_line(int fd, char *buffer, size_t length) {
	size_t used = 0;
	while (running && used + 1 < length) {
		char byte;
		ssize_t count = read(fd, &byte, 1);
		if (count == 1) {
			if (byte == '\n' || byte == '\r') {
				if (used == 0) {
					continue;
				}
				break;
			}
			buffer[used++] = byte;
			continue;
		}
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				usleep(500);
				continue;
			}
			return false;
		}
		usleep(500);
	}
	buffer[used] = '\0';
	if (used > 0) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return used > 0;
}

static void sim_write_message(int fd, const char *message) {
	serial_simulator_trace_line(options.trace, "<-", message);
	serial_simulator_write_all(fd, message, strlen(message));
	serial_simulator_write_all(fd, "\n", 1);
}

static int normalize_steps(int value) {
	value %= ROTATOR_RANGE;
	if (value < 0) {
		value += ROTATOR_RANGE;
	}
	return value;
}

static void send_rotator_status(int fd) {
	char response[128];
	snprintf(response, sizeof(response), ":SER,%d,%d,%d,%d,%d#", rotator_position, rotator_position == home_position ? 1 : 0, ROTATOR_RANGE, home_position, move_threshold);
	sim_write_message(fd, response);
}

static void send_shutter_status(int fd) {
	char response[128];
	snprintf(response, sizeof(response), ":SES,%d,%d,%d,%d#", shutter_position, SHUTTER_RANGE, shutter_position >= SHUTTER_RANGE ? 1 : 0, shutter_position <= 0 ? 1 : 0);
	sim_write_message(fd, response);
}

static void dispatch_command(int fd, const char *raw_command) {
	char command[128];
	snprintf(command, sizeof(command), "%s", raw_command[0] == '@' ? raw_command + 1 : raw_command);

	if (!strcmp(command, "FRR")) {
		sim_write_message(fd, ":FR3.0.0#");
	} else if (!strcmp(command, "SRR")) {
		send_rotator_status(fd);
	} else if (!strcmp(command, "SRS")) {
		send_shutter_status(fd);
	} else if (!strcmp(command, "ARR")) {
		char response[64];
		snprintf(response, sizeof(response), ":ARR%d#", rotator_acceleration);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "ARS")) {
		char response[64];
		snprintf(response, sizeof(response), ":ARS%d#", shutter_acceleration);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "DRR")) {
		char response[64];
		snprintf(response, sizeof(response), ":DRR%d#", move_threshold);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "HRR")) {
		char response[64];
		snprintf(response, sizeof(response), ":HRR%d#", home_position);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "PRR")) {
		char response[64];
		snprintf(response, sizeof(response), ":PRR%d#", rotator_position);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "PRS")) {
		char response[64];
		snprintf(response, sizeof(response), ":PRS%d#", shutter_position);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "VRR")) {
		char response[64];
		snprintf(response, sizeof(response), ":VRR%d#", rotator_velocity);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "VRS")) {
		char response[64];
		snprintf(response, sizeof(response), ":VRS%d#", shutter_velocity);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "RRR")) {
		char response[64];
		snprintf(response, sizeof(response), ":RRR%d#", ROTATOR_RANGE);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "RRS")) {
		char response[64];
		snprintf(response, sizeof(response), ":RRS%d#", SHUTTER_RANGE);
		sim_write_message(fd, response);
	} else if (!strcmp(command, "Open")) {
		sim_write_message(fd, ":open#");
		shutter_position = SHUTTER_RANGE;
		send_shutter_status(fd);
	} else if (!strcmp(command, "Close")) {
		sim_write_message(fd, ":close#");
		shutter_position = 0;
		send_shutter_status(fd);
	} else if (!strcmp(command, "Abort")) {
		send_rotator_status(fd);
		send_shutter_status(fd);
	} else if (!strcmp(command, "Home")) {
		sim_write_message(fd, ":left#");
		rotator_position = home_position;
		send_rotator_status(fd);
	} else if (!strncmp(command, "GAR,", 4)) {
		int position = (int)(atof(command + 4) * ROTATOR_RANGE / 360.0);
		sim_write_message(fd, ":right#");
		rotator_position = normalize_steps(position);
		send_rotator_status(fd);
	} else if (!strncmp(command, "PWR,", 4)) {
		rotator_position = normalize_steps(atoi(command + 4));
		sim_write_message(fd, ":OK#");
		send_rotator_status(fd);
	} else if (!strncmp(command, "AWR,", 4)) {
		rotator_acceleration = atoi(command + 4);
		sim_write_message(fd, ":OK#");
	} else if (!strncmp(command, "AWS,", 4)) {
		shutter_acceleration = atoi(command + 4);
		sim_write_message(fd, ":OK#");
	} else if (!strncmp(command, "DWR,", 4)) {
		move_threshold = atoi(command + 4);
		sim_write_message(fd, ":OK#");
	} else if (!strncmp(command, "HWR,", 4)) {
		home_position = normalize_steps(atoi(command + 4));
		sim_write_message(fd, ":OK#");
	} else if (!strncmp(command, "VWR,", 4)) {
		rotator_velocity = atoi(command + 4);
		sim_write_message(fd, ":OK#");
	} else if (!strncmp(command, "VWS,", 4)) {
		shutter_velocity = atoi(command + 4);
		sim_write_message(fd, ":OK#");
	} else if (!strncmp(command, "RWR,", 4) || !strncmp(command, "RWS,", 4) || !strcmp(command, "EWR") || !strcmp(command, "EER")) {
		sim_write_message(fd, ":OK#");
	} else if (!strcmp(command, "WBR")) {
		sim_write_message(fd, ":BV900#");
	} else {
		serial_simulator_trace_line(options.trace, "??", command);
		sim_write_message(fd, ":OK#");
	}
}

int main(int argc, char *argv[]) {
	char command[128];
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
		printf("NexDome 3 serial simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_line(serial_fd, command, sizeof(command))) {
			dispatch_command(serial_fd, command);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
