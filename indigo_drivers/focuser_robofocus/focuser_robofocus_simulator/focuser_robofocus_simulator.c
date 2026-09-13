// RoboFocus serial simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// This simulator was refactored by a Codex agent.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "../../../indigo_test/simulator_common/serial_motion.h"
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static volatile sig_atomic_t running = 1;
static bool headless, trace, moving, stalled, stall_injected;
static int serial_fd = -1, maximum = 64000, temperature = 600, last_tick_position;
static uint8_t configuration[6] = { 0, 0, 0, 128, 10, 4 };
static uint8_t power_channels[4] = { '1', '1', '1', '1' };
static uint8_t backlash_direction = '2';
static int backlash = 20;
static serial_motion motion;

static uint8_t checksum(const uint8_t *frame) {
	unsigned sum = 0;
	for (int i = 0; i < 8; i++) {
		sum += frame[i];
	}
	return (uint8_t)sum;
}

static void event(const char *kind, const uint8_t *frame, size_t length) {
	char text[64];
	size_t used = 0;
	for (size_t i = 0; i < length && used + 4 < sizeof(text); i++) {
		used += (size_t)snprintf(text + used, sizeof(text) - used, "%s%02x", i ? " " : "", frame[i]);
	}
	serial_simulator_trace_line(trace, kind, text);
	if (!events) {
		return;
	}
	fprintf(events, "%.6f %s", serial_motion_time(), kind);
	for (size_t i = 0; i < length; i++) {
		fprintf(events, " %02x", frame[i]);
	}
	fputc('\n', events);
	fflush(events);
}

static bool write_bytes(const uint8_t *data, size_t length) {
	event("TX", data, length);
	if (!strcmp(profile, "split") && length > 2) {
		size_t first = length / 2;
		if (!serial_simulator_write_all(serial_fd, (const char *)data, first)) {
			return false;
		}
		usleep(10000);
		return serial_simulator_write_all(serial_fd, (const char *)data + first, length - first);
	}
	return serial_simulator_write_all(serial_fd, (const char *)data, length);
}

static bool reply(const uint8_t payload[8], bool bad_checksum) {
	uint8_t frame[9];
	memcpy(frame, payload, 8);
	frame[8] = checksum(frame) + (bad_checksum ? 1 : 0);
	return write_bytes(frame, sizeof(frame));
}

static void format_value(uint8_t payload[8], char command, int value) {
	payload[0] = 'F';
	payload[1] = (uint8_t)command;
	for (int i = 7; i >= 2; i--) {
		payload[i] = (uint8_t)('0' + value % 10);
		value /= 10;
	}
}

static bool parse_value(const uint8_t *field, int count, int minimum, int maximum_value, int *value) {
	int result = 0;
	for (int i = 0; i < count; i++) {
		if (!isdigit(field[i])) {
			return false;
		}
		result = result * 10 + field[i] - '0';
	}
	if (result < minimum || result > maximum_value) {
		return false;
	}
	*value = result;
	return true;
}

static bool all_zero(const uint8_t *field, int count) {
	for (int i = 0; i < count; i++) {
		if (field[i] != '0' && field[i] != 0) {
			return false;
		}
	}
	return true;
}

static bool fault(const char *key, char *action, size_t size) {
	char found[64] = { 0 };
	FILE *file = fault_file ? fopen(fault_file, "r") : NULL;
	if (!file) {
		return false;
	}
	if (fscanf(file, "%63s %63s", found, action) != 2) {
		action[0] = 0;
	}
	fclose(file);
	if (!strcmp(found, "external")) {
		int value;
		if (parse_value((uint8_t *)action, (int)strlen(action), 1, maximum, &value)) {
			serial_motion_sync(&motion, value);
			moving = false;
			last_tick_position = value;
		}
		unlink(fault_file);
		return false;
	}
	if (!strcmp(found, "temperature")) {
		int value;
		if (parse_value((uint8_t *)action, (int)strlen(action), 0, 1200, &value)) {
			temperature = value;
		}
		unlink(fault_file);
		return false;
	}
	if (strcmp(found, key)) {
		action[0] = 0;
		return false;
	}
	unlink(fault_file);
	action[size - 1] = 0;
	return action[0] != 0;
}

static bool injected(const char *key, const uint8_t normal[8]) {
	char action[64] = { 0 };
	if (!fault(key, action, sizeof(action))) {
		return false;
	}
	if (!strcmp(action, "silent")) {
		return true;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		return true;
	}
	if (!strcmp(action, "partial")) {
		write_bytes(normal, 4);
		return true;
	}
	if (!strcmp(action, "bad_checksum")) {
		reply(normal, true);
		return true;
	}
	uint8_t malformed[8] = { 'F', 'X', 'B', 'A', 'D', '0', '0', '0' };
	reply(malformed, false);
	return true;
}

static void send_value(char command, int value) {
	uint8_t payload[8];
	format_value(payload, command, value);
	char key[3] = { 'F', command, 0 };
	if (!injected(key, payload)) {
		reply(payload, false);
	}
}

static void stop_motion(void) {
	if (moving) {
		serial_motion_stop(&motion);
		moving = stalled = false;
		last_tick_position = (int)motion.position;
		send_value('D', last_tick_position);
	}
}

static void start_motion(int target, char command) {
	int position = (int)serial_motion_update(&motion);
	target = target < 1 ? 1 : target > maximum ? maximum : target;
	serial_motion_start(&motion, target, 1000);
	last_tick_position = position;
	moving = motion.duration > 0;
	char key[3] = { 'F', command, 0 }, action[64] = { 0 };
	if (fault(key, action, sizeof(action)) && !strcmp(action, "stall")) {
		stalled = true;
	}
	if (!strcmp(profile, "stall") && !stall_injected) {
		stalled = true;
		stall_injected = true;
	}
	if (!moving) {
		send_value('D', target);
	}
}

static void update_motion(void) {
	if (!moving || stalled) {
		return;
	}
	int position = (int)serial_motion_update(&motion);
	int direction = position < last_tick_position ? -1 : 1;
	for (int emitted = 0; last_tick_position != position && emitted < 128; emitted++) {
		last_tick_position += direction;
		uint8_t tick = direction < 0 ? 'I' : 'O';
		char action[64] = { 0 };
		if (fault("progress", action, sizeof(action))) {
			if (!strcmp(action, "bad")) {
				tick = 'X';
			} else if (!strcmp(action, "close")) {
				running = 0;
				return;
			}
		}
		write_bytes(&tick, 1);
	}
	if (motion.duration == 0 && last_tick_position == position) {
		moving = false;
		send_value('D', position);
	}
}

static void dispatch(const uint8_t request[9]) {
	event("RX", request, 9);
	if (request[0] != 'F' || request[8] != checksum(request)) {
		return;
	}
	int value;
	switch (request[1]) {
		case 'V':
			if (all_zero(request + 2, 6)) send_value('V', 30200);
			break;
		case 'G':
			if (parse_value(request + 2, 6, 0, 65535, &value)) {
				if (value) start_motion(value, 'G'); else send_value('D', (int)serial_motion_update(&motion));
			}
			break;
		case 'I':
			if (parse_value(request + 2, 6, 0, 65535, &value)) start_motion((int)serial_motion_update(&motion) - value, 'I');
			break;
		case 'O':
			if (parse_value(request + 2, 6, 0, 65535, &value)) start_motion((int)serial_motion_update(&motion) + value, 'O');
			break;
		case 'S':
			if (parse_value(request + 2, 6, 0, 65535, &value)) {
				if (value) serial_motion_sync(&motion, value);
				last_tick_position = (int)motion.position;
				send_value('D', last_tick_position);
			}
			break;
		case 'L':
			if (parse_value(request + 2, 6, 0, 65535, &value)) {
				if (value) maximum = value;
				if (motion.position > maximum) serial_motion_sync(&motion, maximum);
				send_value('L', maximum);
			}
			break;
		case 'P': {
			uint8_t payload[8] = { 'F', 'P', '0', '0', power_channels[0], power_channels[1], power_channels[2], power_channels[3] };
			if (!all_zero(request + 2, 6)) {
				if (request[2] != '0' || request[3] != '0') break;
				for (int i = 0; i < 4; i++) {
					if (request[i + 4] != '1' && request[i + 4] != '2') return;
					power_channels[i] = payload[i + 4] = request[i + 4];
				}
			}
			if (!injected("FP", payload)) reply(payload, false);
			break;
		}
		case 'C': {
			if (!all_zero(request + 2, 6)) {
				if (request[2] || request[3] || request[4] || request[5] > 250 || request[6] < 1 || request[6] > 64 || request[7] < 1 || request[7] > 64) break;
				memcpy(configuration, request + 2, 6);
			}
			uint8_t payload[8] = { 'F', 'C', configuration[0], configuration[1], configuration[2], configuration[3], configuration[4], configuration[5] };
			if (!injected("FC", payload)) reply(payload, false);
			break;
		}
		case 'B': {
			if (!all_zero(request + 2, 6)) {
				if ((request[2] != '1' && request[2] != '2' && request[2] != '3') || !parse_value(request + 3, 5, 0, 255, &value)) break;
				backlash_direction = request[2];
				backlash = request[2] == '1' ? 0 : value;
			}
			uint8_t payload[8] = { 'F', 'B', backlash_direction, '0', '0', '0', '0', '0' };
			for (int i = 7, encoded = backlash; i >= 3; i--) {
				payload[i] = (uint8_t)('0' + encoded % 10);
				encoded /= 10;
			}
			if (!injected("FB", payload)) reply(payload, false);
			break;
		}
		case 'T':
			if (all_zero(request + 2, 6)) send_value('T', temperature);
			break;
		default:
			break;
	}
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) headless = true;
		else if (!strcmp(argv[i], "--trace")) trace = true;
		else if (i + 1 < argc && !strcmp(argv[i], "--ready-file")) ready_file = argv[++i];
		else if (i + 1 < argc && !strcmp(argv[i], "--profile")) profile = argv[++i];
		else {
			fprintf(stderr, "Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|alternate]\n", argv[0]);
			return 1;
		}
	}
	if (strcmp(profile, "normal") && strcmp(profile, "split") && strcmp(profile, "alternate") && strcmp(profile, "stall")) return 1;
	int initial = !strcmp(profile, "alternate") ? 1234 : 32000;
	if (!strcmp(profile, "alternate")) {
		maximum = 50000; temperature = 535;
		configuration[3] = 200; configuration[4] = 20; configuration[5] = 8;
		power_channels[0] = power_channels[2] = '2';
		backlash_direction = '3'; backlash = 35;
	}
	serial_motion_sync(&motion, initial);
	last_tick_position = initial;
	const char *event_path = getenv("INDIGO_ROBOFOCUS_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	fault_file = getenv("INDIGO_ROBOFOCUS_FAULT");
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	char port[128];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_robofocus", port))) return 1;
	if (!headless) {
		printf("RoboFocus simulator on %s\n", port);
		fflush(stdout);
	}
	uint8_t request[9];
	size_t used = 0;
	while (running) {
		update_motion();
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);
		struct timeval timeout = { 0, 1000 };
		if (select(serial_fd + 1, &fds, NULL, NULL, &timeout) <= 0) continue;
		uint8_t buffer[64];
		ssize_t count = read(serial_fd, buffer, sizeof(buffer));
		if (count <= 0) {
			if (count < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EIO) break;
			continue;
		}
		for (ssize_t i = 0; i < count; i++) {
			if (moving) stop_motion();
			if (buffer[i] == '\r') { used = 0; continue; }
			request[used++] = buffer[i];
			if (used == sizeof(request)) { dispatch(request); used = 0; }
		}
	}
	if (events) fclose(events);
	if (serial_fd >= 0) close(serial_fd);
	return 0;
}
