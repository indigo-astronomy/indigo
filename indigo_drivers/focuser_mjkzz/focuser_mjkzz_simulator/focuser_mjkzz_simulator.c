// MJKZZ rail focuser simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>

#include "../mjkzz_def.h"
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

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

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint8_t address = 0x01;
static int32_t target = 0;
static int32_t position = 0;
static int32_t speed = 0;

static int32_t sim_get_int(const mjkzz_message *message) {
	return ((((((int32_t)message->ucMSG[0] << 8) + (int32_t)message->ucMSG[1]) << 8) + (int32_t)message->ucMSG[2]) << 8) + (int32_t)message->ucMSG[3];
}

static void sim_set_int(mjkzz_message *message, int32_t value) {
	message->ucMSG[0] = (uint8_t)(value >> 24);
	message->ucMSG[1] = (uint8_t)(value >> 16);
	message->ucMSG[2] = (uint8_t)(value >> 8);
	message->ucMSG[3] = (uint8_t)value;
}

static void usage(const char *name) {
	printf("MJKZZ rail focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  -h, --help              Show this help and exit\n");
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
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

static bool sim_read_message(int handle, mjkzz_message *message) {
	uint8_t *buffer = (uint8_t *)message;
	size_t read_bytes = 0;

	while (running && read_bytes < sizeof(*message)) {
		ssize_t result = read(handle, buffer + read_bytes, sizeof(*message) - read_bytes);
		if (result < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return false;
			}
			if (errno == EIO) {
				return false;
			}
			return false;
		}
		if (result == 0) {
			return false;
		}
		read_bytes += (size_t)result;
	}
	if (read_bytes == sizeof(*message) && options.trace) {
		fprintf(stderr, "-> %02x %02x %02x [%02x %02x %02x %02x] %02x (%d)\n", message->ucADD, message->ucCMD, message->ucIDX, message->ucMSG[0], message->ucMSG[1], message->ucMSG[2], message->ucMSG[3], message->ucSUM, sim_get_int(message));
	}
	return read_bytes == sizeof(*message);
}

static bool sim_write_message(int handle, mjkzz_message *message) {
	message->ucSUM = message->ucADD + message->ucCMD + message->ucIDX + message->ucMSG[0] + message->ucMSG[1] + message->ucMSG[2] + message->ucMSG[3];
	if (options.trace) {
		fprintf(stderr, "<- %02x %02x %02x [%02x %02x %02x %02x] %02x (%d)\n", message->ucADD, message->ucCMD, message->ucIDX, message->ucMSG[0], message->ucMSG[1], message->ucMSG[2], message->ucMSG[3], message->ucSUM, sim_get_int(message));
	}
	return serial_simulator_write_all(handle, (const char *)message, sizeof(*message));
}

static void *movement_thread(void *arg) {
	(void)arg;
	while (running) {
		pthread_mutex_lock(&state_mutex);
		if (target < position) {
			position--;
		} else if (target > position) {
			position++;
		}
		pthread_mutex_unlock(&state_mutex);
		usleep(10000);
	}
	return NULL;
}

static void dispatch_message(int handle, mjkzz_message *message) {
	pthread_mutex_lock(&state_mutex);
	if (message->ucADD == address) {
		message->ucADD |= 0x80;
	}
	switch (message->ucCMD) {
		case CMD_GVER:
			message->ucMSG[0] = 1;
			message->ucMSG[1] = 0;
			message->ucMSG[2] = 0;
			message->ucMSG[3] = 0;
			break;
		case CMD_SPOS:
			target = sim_get_int(message);
			break;
		case CMD_GPOS:
			sim_set_int(message, position);
			break;
		case CMD_SSPD:
			speed = sim_get_int(message);
			break;
		case CMD_GSPD:
			sim_set_int(message, speed);
			break;
		case CMD_STOP:
			target = position;
			sim_set_int(message, position);
			break;
		case CMD_SREG:
			break;
		default:
			break;
	}
	message->ucCMD |= 0x80;
	pthread_mutex_unlock(&state_mutex);
	sim_write_message(handle, message);
}

int main(int argc, char *argv[]) {
	pthread_t thread;
	char port[128];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_mjkzz_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("MJKZZ rail focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, movement_thread, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		return 1;
	}

	while (running) {
		mjkzz_message message = { 0 };
		if (sim_read_message(serial_fd, &message)) {
			dispatch_message(serial_fd, &message);
		} else {
			usleep(1000);
		}
	}

	running = 0;
	pthread_join(thread, NULL);
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
