// PlaneWave EFA focuser serial simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// This source file was refactored by a Codex agent.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include <stdint.h>
#include <signal.h>
#include <sys/select.h>
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

static const char *profile = "normal", *ready_file, *fault_file;
static bool headless, trace, celestron, fans, frozen, injected, calibration_failed;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1, raw_temperature = 344;
static double calibration_until;
static serial_motion motion;
static FILE *events;
static uint32_t minimum = 0, maximum = 3799422;

static void event(const char *kind, const char *value) {
	if (events) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, value);
		fflush(events);
	}
}

static void stop_signal(int sig) {
	(void)sig;
	running = 0;
}

static int32_t read24(const uint8_t *p) {
	int32_t value = (int32_t)p[0] * 65536 + p[1] * 256 + p[2];
	return value & 0x800000 ? value - 0x1000000 : value;
}

static void write_number(uint8_t *p, uint32_t value, int bytes) {
	for (int i = bytes - 1; i >= 0; i--) {
		p[i] = (uint8_t)(value & 255);
		value >>= 8;
	}
}

static void checksum(uint8_t *p) {
	unsigned sum = 0;
	for (int i = 1; i < p[1] + 2; i++) {
		sum += p[i];
	}
	p[p[1] + 2] = (uint8_t)(0u - sum);
}

static void send_frame(uint8_t *p, int size) {
	if (trace) {
		fprintf(stderr, "TX cmd=%02X bytes=%d\n", p[4], size);
	}
	if (!strcmp(profile, "split")) {
		serial_simulator_write_all(serial_fd, (char *)p, 2);
		usleep(10000);
		serial_simulator_write_all(serial_fd, (char *)p + 2, (size_t)size - 2);
	} else {
		serial_simulator_write_all(serial_fd, (char *)p, (size_t)size);
	}
}

static void dispatch(uint8_t *request) {
	unsigned sum = 0;
	for (int i = 1; i < request[1] + 3; i++) {
		sum += request[i];
	}
	if ((sum & 255) || request[1] < 3) {
		return;
	}
	char journal[128];
	int value = request[1] == 6 ? read24(request + 5) : request[1] >= 4 ? request[5] : 0;
	snprintf(journal, sizeof(journal), "%02X %02X %d %d", request[3], request[4], value, request[1] - 3);
	event("RX", journal);
	serial_simulator_trace_line(trace, "RX", journal);
	char key[32] = "", action[64] = "";
	FILE *file = fault_file ? fopen(fault_file, "r") : NULL;
	if (file) {
		if (fscanf(file, "%31s %63s", key, action) != 2) {
			*action = 0;
		}
		fclose(file);
		if (!strcmp(key, "external")) {
			serial_motion_sync(&motion, atoi(action));
			unlink(fault_file);
			*action = 0;
		} else if (!strcmp(key, "temperature")) {
			raw_temperature = atoi(action);
			unlink(fault_file);
			*action = 0;
		} else if (strtoul(key, NULL, 16) == request[4]) {
			unlink(fault_file);
		} else {
			*action = 0;
		}
	}
	const char *fault_profile = !strncmp(profile, "c_", 2) ? profile + 2 : profile;
	if (!injected && !strncmp(fault_profile, "init_", 5) && strtoul(fault_profile + 5, NULL, 16) == request[4]) {
		injected = true;
		snprintf(action, sizeof(action), "%s", fault_profile + 8);
	}
	if (*action) {
		event("FAULT", action);
	}
	if (!strcmp(action, "silent")) {
		return;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		return;
	}
	uint8_t response[256] = { 0x3B, 4, request[3], request[2], request[4], 1 };
	int32_t position = frozen ? (int32_t)motion.position : (int32_t)serial_motion_update(&motion);
	bool ignore = !strcmp(action, "reject") || !strcmp(action, "ignore");
	if (!ignore) {
		switch (request[4]) {
			case 0xFE:
				response[1] = celestron ? 7 : 5;
				response[5] = 1; response[6] = 5; response[7] = 0; response[8] = 4;
				if (!strcmp(profile, "unknown")) {
					response[1] = 4;
				}
				break;
			case 0x01: response[1] = 6; write_number(response + 5, (uint32_t)position, 3); break;
			case 0x04: serial_motion_sync(&motion, value); break;
			case 0x02:
			case 0x17:
				serial_motion_start(&motion, value, 100000);
				frozen = !strcmp(action, "stall");
				break;
			case 0x24:
			case 0x25:
				if (value) {
					serial_motion_start(&motion, request[4] == 0x24 ? maximum : minimum, 100000);
					frozen = !strcmp(action, "stall");
				} else {
					if (frozen) {
						serial_motion_sync(&motion, motion.position);
					} else {
						serial_motion_stop(&motion);
					}
					frozen = false;
				}
				break;
			case 0x13: response[5] = frozen || motion.duration > 0 ? 0 : 255; break;
			case 0x26:
				response[1] = 6;
				response[5] = request[5];
				write_number(response + 6, (uint32_t)raw_temperature, 2);
				if (!strcmp(profile, "legacy_temp")) {
					response[1] = 5;
					response[5] = (uint8_t)raw_temperature;
					response[6] = (uint8_t)((unsigned)raw_temperature >> 8);
				}
				break;
			case 0x27: fans = value != 0; break;
			case 0x28: response[5] = fans ? 0 : 3; break;
			case 0x30: response[5] = strcmp(profile, "uncalibrated") != 0; break;
			case 0xEF: response[1] = 3; break;
			case 0x2A:
				calibration_until = value ? serial_motion_time() + 1 : 0;
				calibration_failed = !strcmp(action, "calfail");
				if (!strcmp(action, "calstall")) {
					calibration_until += 1000;
				}
				break;
			case 0x2B:
				response[1] = 5;
				response[5] = calibration_failed || !strcmp(profile, "c_uncalibrated") ? 0 : calibration_until > serial_motion_time() ? 0 : 1;
				response[6] = calibration_failed ? 0 : 1;
				break;
			case 0x2C:
				response[1] = 11;
				write_number(response + 5, minimum, 4);
				write_number(response + 9, maximum, 4);
				break;
		}
	}
	if (!strcmp(action, "reject")) {
		response[5] = 0;
	}
	if (!strcmp(action, "badstate")) {
		response[5] = 7;
	}
	if (!strcmp(action, "short")) {
		response[1] = 3;
	}
	if (!strcmp(action, "wrongsrc")) {
		response[2] = 0x14;
	}
	if (!strcmp(action, "wrongdst")) {
		response[3] = 0x14;
	}
	if (!strcmp(action, "wrongcmd")) {
		response[4] = 0x99;
	}
	if (!strcmp(action, "overlong")) {
		response[1] = 200;
	}
	checksum(response);
	if (!strcmp(action, "checksum")) {
		response[response[1] + 2] ^= 1;
	}
	if (!strcmp(profile, "echo")) {
		send_frame(request, request[1] + 3);
	}
	send_frame(response, !strcmp(action, "partial") ? 4 : response[1] + 3);
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) {
			headless = true;
		} else if (!strcmp(argv[i], "--trace")) {
			trace = true;
		} else if (i + 1 < argc && !strcmp(argv[i], "--ready-file")) {
			ready_file = argv[++i];
		} else if (i + 1 < argc && !strcmp(argv[i], "--profile")) {
			profile = argv[++i];
		} else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			printf("Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile NAME]\n", argv[0]);
			return 0;
		} else {
			return 1;
		}
	}
	celestron = !strcmp(profile, "celestron") || !strcmp(profile, "alternate") || !strncmp(profile, "c_", 2);
	if (celestron) {
		maximum = 100000;
	}
	serial_motion_sync(&motion, 10);
	if (!strcmp(profile, "alternate")) {
		serial_motion_sync(&motion, 500);
	}
	const char *event_path = getenv("INDIGO_EFA_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	fault_file = getenv("INDIGO_EFA_FAULT");
	char port[128];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_efa", port))) {
		return 1;
	}
	if (!headless) {
		printf("EFA simulator on %s\n", port);
	}
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	uint8_t frame[256];
	int used = 0;
	while (running) {
		if (!frozen) {
			serial_motion_update(&motion);
		}
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);
		struct timeval timeout = { 0, 10000 };
		if (select(serial_fd + 1, &fds, NULL, NULL, &timeout) <= 0) {
			continue;
		}
		uint8_t byte;
		if (read(serial_fd, &byte, 1) != 1) {
			usleep(10000);
			continue;
		}
		if (!used && byte != 0x3B) {
			continue;
		}
		frame[used++] = byte;
		if (used == 2 && (frame[1] < 3 || frame[1] > 32)) {
			used = 0;
		} else if (used >= 2 && used == frame[1] + 3) {
			dispatch(frame);
			used = 0;
		}
	}
	close(serial_fd);
	if (events) {
		fclose(events);
	}
	return 0;
}
