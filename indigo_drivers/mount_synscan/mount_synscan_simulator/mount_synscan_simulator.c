// SkyWatcher EQ8 simulator
//
// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

#define HIGHSPEED_STEPS       128
#define WORM_STEPS            (4 * 3600)
#define STEPS_PER_REVOLUTION  (60 * WORM_STEPS)
#define FEATURES              (HAS_ENCODER | HAS_PPEC | HAS_HOME_INDEXER | HAS_COMMON_SLEW_START | HAS_HALF_CURRENT_TRACKING)

#define HEX(c)                (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

// ----------------------------------------------------------------- protocol constants

enum MOTOR_STATUS {
	RUNNING     = 0x0001,
	BLOCKED     = 0x0002,
	TRACKING    = 0X0010,
	BACKWARD    = 0x0020,
	HIGHSPEED   = 0x0040,
	INITIALIZED = 0x0100,
	LEVEL_ON    = 0x0300
};

enum EXT_INQUIRY_CMD {
	GET_INDEXER_CMD   = 0x0000,
	GET_FEATURES_CMD  = 0x0001
};

enum FEATURE {
	HAS_ENCODER               = 0x0001,
	HAS_PPEC                  = 0x0002,
	HAS_HOME_INDEXER          = 0x0004,
	IS_AZEQ                   = 0x0008,
	IN_PPEC_TRAINING          = 0x0010,
	IN_PPEC                   = 0x0020,
	HAS_POLAR_LED             = 0x1000,
	HAS_COMMON_SLEW_START     = 0x2000,
	HAS_HALF_CURRENT_TRACKING = 0x4000
};

enum EXT_SETTING_CMD {
	START_PPEC_TRAINING_CMD            = 0x0000,
	STOP_PPEC_TRAINING_CMD             = 0x0001,
	TURN_PPEC_ON_CMD                   = 0x0002,
	TURN_PPEC_OFF_CMD                  = 0X0003,
	ENCODER_ON_CMD                     = 0x0004,
	ENCODER_OFF_CMD                    = 0x0005,
	DISABLE_FULL_CURRENT_LOW_SPEED_CMD = 0x0006,
	ENABLE_FULL_CURRENT_LOW_SPEED_CMD  = 0x0106,
	RESET_HOME_INDEXER_CMD             = 0x0008
};

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	bool pcdirect;
	const char *ready_file;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.pcdirect = false,
	.ready_file = NULL
};

static const char *simulator_name = "mount_synscan";

static void usage(const char *name) {
	printf("SkyWatcher EQ8 SynScan mount simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --pcdirect              Start axes as initialized for PC Direct style probing\n");
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
		} else if (!strcmp(argv[i], "--pcdirect")) {
			options.pcdirect = true;
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

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static char hexa[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static uint32_t axis_timer[2] = { 0, 0 };
static uint32_t axis_t1[2] = { 25, 25 };
static uint16_t axis_status[2] = { 0, 0 };
static uint32_t axis_position[2] = { 0x800000, 0x800000 + STEPS_PER_REVOLUTION / 4 };
static uint32_t axis_increment[2] = { 0, 0 };
static uint32_t axis_target[2] = { 0, 0 };
static bool axis_increment_set[2] = { false, false };
static bool axis_target_set[2] = { false, false };
static uint32_t axis_brake[2] = { 0, 0 };
static uint32_t axis_features[2] = { FEATURES, FEATURES };
static int32_t axis_abs_position[2] = { 0, 0 };
static int32_t axis_home_index[2] = { 0, 0 };
static bool axis_home_index_hit[2] = { false, false };

static void initialize_state(void) {
	if (options.pcdirect) {
		axis_status[0] = INITIALIZED;
		axis_status[1] = INITIALIZED;
	}
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- protocol helpers

static char *reply_8(uint8_t n) {
	static char buffer[8] = "=00";
	buffer[1] = hexa[(n & 0xF0) >> 4];
	buffer[2] = hexa[(n & 0x0F)];
	return buffer;
}

static char *reply_12(uint16_t n) {
	static char buffer[8] = "=000";
	buffer[1] = hexa[(n & 0xF0) >> 4];
	buffer[2] = hexa[(n & 0x0F)];
	buffer[3] = hexa[(n & 0xF00) >> 8];
	return buffer;
}

static char *reply_24(uint32_t n) {
	static char buffer[8] = "=000000";
	buffer[1] = hexa[(n & 0xF0) >> 4];
	buffer[2] = hexa[(n & 0x0F)];
	buffer[3] = hexa[(n & 0xF000) >> 12];
	buffer[4] = hexa[(n & 0x0F00) >> 8];
	buffer[5] = hexa[(n & 0xF00000) >> 20];
	buffer[6] = hexa[(n & 0x0F0000) >> 16];
	return buffer;
}

static uint32_t parse_24(const char *buffer) {
	uint32_t result = HEX(buffer[4]);
	result = (result << 4) | HEX(buffer[5]);
	result = (result << 4) | HEX(buffer[2]);
	result = (result << 4) | HEX(buffer[3]);
	result = (result << 4) | HEX(buffer[0]);
	result = (result << 4) | HEX(buffer[1]);
	return result;
}

static uint8_t parse_8(const char *buffer) {
	return (HEX(buffer[0]) << 4) | HEX(buffer[1]);
}

static char *process_command(char *buffer) {
	if (buffer[0] != ':') {
		return "!3";
	}
	if (buffer[2] != '1' && buffer[2] != '2') {
		return "!0";
	}
	int axis = buffer[2] - '1';
	switch (buffer[1]) {
		case 'B':
			axis_status[axis] = 0;
			return "=";
		case 'D':
			return reply_24(100);
		case 'E':
			axis_position[axis] = parse_24(buffer + 3);
			return "=";
		case 'F':
			axis_status[axis] |= INITIALIZED;
			return "=";
		case 'G': {
			if (axis_status[axis] & RUNNING) {
				return "!2";
			}
			uint8_t mode = parse_8(buffer + 3);
			if (mode & 0x01) {
				axis_status[axis] |= BACKWARD;
			} else {
				axis_status[axis] &= ~BACKWARD;
			}
			switch (mode >> 4) {
				case 0:
					axis_status[axis] &= ~TRACKING;
					axis_status[axis] |= HIGHSPEED;
					break;
				case 1:
					axis_status[axis] |= TRACKING;
					axis_status[axis] &= ~HIGHSPEED;
					break;
				case 2:
					axis_status[axis] &= ~TRACKING;
					axis_status[axis] &= ~HIGHSPEED;
					break;
				case 3:
					axis_status[axis] |= TRACKING;
					axis_status[axis] |= HIGHSPEED;
					break;
			}
			return "=";
		}
		case 'H':
			axis_t1[axis] = 1;
			axis_increment[axis] = parse_24(buffer + 3);
			axis_increment_set[axis] = true;
			axis_target_set[axis] = false;
			return "=";
		case 'I':
			axis_t1[axis] = parse_24(buffer + 3);
			return "=";
		case 'J':
			if (!(axis_status[axis] & TRACKING)) {
				if (axis_increment_set[axis]) {
					if (axis_status[axis] & BACKWARD) {
						axis_target[axis] = axis_position[axis] - axis_increment[axis];
					} else {
						axis_target[axis] = axis_position[axis] + axis_increment[axis];
					}
					axis_increment_set[axis] = false;
					axis_status[axis] |= RUNNING;
				} else if (axis_target_set[axis]) {
					axis_target_set[axis] = false;
					axis_status[axis] |= RUNNING;
				}
			} else {
				axis_status[axis] |= RUNNING;
			}
			return "=";
		case 'K':
		case 'L':
			axis_status[axis] &= ~RUNNING;
			return "=";
		case 'M':
			if (axis_status[axis] & BACKWARD) {
				axis_brake[axis] = axis_position[axis] - parse_24(buffer + 3);
			} else {
				axis_brake[axis] = axis_position[axis] + parse_24(buffer + 3);
			}
			return "=";
		case 'O':
			return "=";
		case 'P':
			return "=";
		case 'S':
			if (axis_status[axis] & RUNNING) {
				return "!2";
			}
			axis_t1[axis] = 1;
			axis_target[axis] = parse_24(buffer + 3);
			axis_increment_set[axis] = false;
			axis_target_set[axis] = true;
			return "=";
		case 'T':
			return "=";
		case 'U':
			axis_brake[axis] = parse_24(buffer + 3);
			return "=";
		case 'V':
			return "=";
		case 'W': {
			switch (parse_24(buffer + 3)) {
				case START_PPEC_TRAINING_CMD:
					axis_features[axis] |= IN_PPEC_TRAINING;
					return "=";
				case STOP_PPEC_TRAINING_CMD:
					axis_features[axis] &= ~IN_PPEC_TRAINING;
					return "=";
				case TURN_PPEC_ON_CMD:
					axis_features[axis] |= IN_PPEC;
					return "=";
				case TURN_PPEC_OFF_CMD:
					axis_features[axis] &= ~IN_PPEC;
					return "=";
				case ENCODER_ON_CMD:
					return "=";
				case ENCODER_OFF_CMD:
					return "=";
				case DISABLE_FULL_CURRENT_LOW_SPEED_CMD:
					return "=";
				case ENABLE_FULL_CURRENT_LOW_SPEED_CMD:
					return "=";
				case RESET_HOME_INDEXER_CMD:
					axis_home_index[axis] = axis_abs_position[axis] >= 0 ? -1 : 0;
					axis_home_index_hit[axis] = false;
					return "=";
			}
			return "!0";
		}
		case 'a':
			return reply_24(STEPS_PER_REVOLUTION);
		case 'b':
			return reply_24(1000);
		case 'c':
			return reply_24(axis_brake[axis]);
		case 'e':
			return "=020304";
		case 'f':
			return reply_12(axis_status[axis]);
		case 'g':
			return reply_8(HIGHSPEED_STEPS);
		case 'h':
			return reply_24(axis_target[axis]);
		case 'i':
			return reply_24(axis_t1[axis]);
		case 'j':
			return reply_24(axis_position[axis]);
		case 'k':
			if (buffer[3] == '1') {
				axis_increment[axis] = 0;
			}
			return "=";
		case 'm':
			return reply_24(axis_brake[axis]);
		case 'q': {
			uint32_t id = parse_24(buffer + 3);
			switch (id) {
				case GET_INDEXER_CMD:
					return reply_24(axis_home_index[axis]);
				case GET_FEATURES_CMD:
					return reply_24(axis_features[axis]);
			}
			return "!0";
		}
		case 's':
			return reply_24(WORM_STEPS);
	}
	return "!0";
}

static void process_home_index(uint8_t axis, int32_t steps) {
	if (axis_abs_position[axis] < 0 && axis_abs_position[axis] + steps >= 0) {
		axis_home_index_hit[axis] = true;
		axis_home_index[axis] = axis_position[axis] - axis_abs_position[axis];
	} else if (axis_abs_position[axis] >= 0 && axis_abs_position[axis] + steps < 0) {
		axis_home_index_hit[axis] = true;
		axis_home_index[axis] = axis_position[axis] - axis_abs_position[axis];
	}
	axis_abs_position[axis] += steps;
}

static void process_axis_timer(uint8_t axis) {
	if (++axis_timer[axis] < axis_t1[axis]) {
		return;
	}
	axis_timer[axis] = 0;
	uint16_t status = axis_status[axis];
	if (status & RUNNING) {
		uint32_t steps = status & HIGHSPEED ? HIGHSPEED_STEPS : 1;
		if (status & TRACKING) {
			if (status & BACKWARD) {
				process_home_index(axis, -(int32_t)steps);
				axis_position[axis] -= steps;
			} else {
				process_home_index(axis, (int32_t)steps);
				axis_position[axis] += steps;
			}
		} else {
			if (axis_position[axis] > axis_target[axis]) {
				if (axis_position[axis] - HIGHSPEED_STEPS <= axis_target[axis]) {
					int32_t diff = (int32_t)(axis_position[axis] - axis_target[axis]);
					process_home_index(axis, -diff);
					axis_position[axis] -= (uint32_t)diff;
				} else {
					process_home_index(axis, -HIGHSPEED_STEPS);
					axis_position[axis] -= HIGHSPEED_STEPS;
				}
			} else if (axis_position[axis] < axis_target[axis]) {
				if (axis_position[axis] + HIGHSPEED_STEPS >= axis_target[axis]) {
					int32_t diff = (int32_t)(axis_target[axis] - axis_position[axis]);
					process_home_index(axis, diff);
					axis_position[axis] += (uint32_t)diff;
				} else {
					process_home_index(axis, HIGHSPEED_STEPS);
					axis_position[axis] += HIGHSPEED_STEPS;
				}
			} else {
				axis_status[axis] &= ~RUNNING;
			}
		}
	}
}

static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;

	while (running && used + 1 < length) {
		ssize_t count = read(fd, &byte, 1);
		if (count == 1) {
			if (byte == '\r') {
				buffer[used] = '\0';
				if (used > 0) {
					serial_simulator_trace_line(options.trace, "->", buffer);
				}
				return (int)used;
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
			return -1;
		}
		usleep(500);
	}

	buffer[0] = '\0';
	return -1;
}

static bool sim_write_reply(int fd, const char *reply) {
	char buffer[16];
	int length = snprintf(buffer, sizeof(buffer), "%s\r", reply);
	if (length < 0 || length >= (int)sizeof(buffer)) {
		return false;
	}
	serial_simulator_trace_line(options.trace, "<-", reply);
	return serial_simulator_write_all(fd, buffer, (size_t)length);
}

static void dispatch_command(int fd, char *command) {
	char reply[16];

	pthread_mutex_lock(&state_mutex);
	snprintf(reply, sizeof(reply), "%s", process_command(command));
	pthread_mutex_unlock(&state_mutex);

	sim_write_reply(fd, reply);
}

static void *background(void *arg) {
	(void)arg;
	while (running) {
		pthread_mutex_lock(&state_mutex);
		process_axis_timer(0);
		process_axis_timer(1);
		pthread_mutex_unlock(&state_mutex);
		usleep(5000);
	}
	return NULL;
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
	initialize_state();

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
		printf("SkyWatcher EQ8 SynScan simulator is running on %s\n", port);
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
