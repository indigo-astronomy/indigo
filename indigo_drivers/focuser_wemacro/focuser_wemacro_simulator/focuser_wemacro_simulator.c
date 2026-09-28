// WeMacro Rail serial simulator
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

#define FRAME_SIZE 12
#define STATUS_INITIAL 0xF0
#define STATUS_FORWARD 0xF5
#define STATUS_BACKWARD 0xF6
#define STATUS_BATCH 0xF7

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static bool headless, trace, initial_sent, suppress_status, motion_active, batch_active, batch_back, stalled;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static uint8_t input[FRAME_SIZE];
static size_t input_length;
static double input_time, next_status_time;
static char status_fault[32];
static uint8_t motion_status;
static uint32_t batch_remaining, step_length;
static uint8_t speed, settle_time, shutter_per_step, shutter_interval, config_flags;
static serial_motion motion;

static void event(const char *kind, const char *value) {
	if (events != NULL) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, value);
		fflush(events);
	}
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

static uint16_t crc16(const uint8_t *buffer, size_t length) {
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < length; i++) {
		crc ^= buffer[i];
		for (int bit = 0; bit < 8; bit++) {
			crc = crc & 1 ? (crc >> 1) ^ 0xA001 : crc >> 1;
		}
	}
	return crc;
}

static uint32_t frame_value(const uint8_t *frame) {
	return ((uint32_t)frame[6] << 24) | ((uint32_t)frame[7] << 16) | ((uint32_t)frame[8] << 8) | frame[9];
}

static bool write_status_bytes(const uint8_t *buffer, size_t length, bool split) {
	if (split && length > 1) {
		if (!serial_simulator_write_all(serial_fd, (const char *)buffer, 1)) {
			return false;
		}
		usleep(10000);
		return serial_simulator_write_all(serial_fd, (const char *)buffer + 1, length - 1);
	}
	return serial_simulator_write_all(serial_fd, (const char *)buffer, length);
}

static bool send_status(uint8_t status) {
	char detail[64];
	snprintf(detail, sizeof(detail), "%02X", status);
	if (suppress_status || !strcmp(status_fault, "silent") || !strcmp(status_fault, "stall")) {
		event("DROP", detail);
		status_fault[0] = 0;
		return true;
	}
	uint8_t response[3] = { 0xA5, 0x5A, status };
	if (!strcmp(status_fault, "malformed")) {
		response[0] = 0xA4;
	} else if (!strcmp(status_fault, "wrong")) {
		response[2] = STATUS_INITIAL;
	}
	bool split = !strcmp(profile, "split") || !strcmp(status_fault, "split");
	status_fault[0] = 0;
	snprintf(detail, sizeof(detail), "%02X%02X%02X", response[0], response[1], response[2]);
	event("TX", detail);
	serial_simulator_trace_line(trace, "<-", detail);
	return write_status_bytes(response, sizeof(response), split);
}

static bool read_fault(const char *key, char *action, size_t size) {
	char requested[64] = { 0 };
	FILE *file = fault_file != NULL ? fopen(fault_file, "r") : NULL;
	if (file == NULL) {
		return false;
	}
	if (fscanf(file, "%63s %31s", requested, action) != 2) {
		action[0] = 0;
	}
	fclose(file);
	if (strcmp(requested, key)) {
		action[0] = 0;
		return false;
	}
	unlink(fault_file);
	action[size - 1] = 0;
	event("FAULT", action);
	return action[0] != 0;
}

static bool reserved_zero(const uint8_t *frame) {
	return frame[3] == 0 && frame[4] == 0 && frame[5] == 0;
}

static void reject(const char *reason) {
	event("REJECT", reason);
	serial_simulator_trace_line(trace, "reject:", reason);
}

static void accept(const char *name, const uint8_t *frame) {
	char detail[160];
	snprintf(detail, sizeof(detail), "%s cmd=%02X a=%u b=%u c=%u value=%u", name, frame[2], frame[3], frame[4], frame[5], frame_value(frame));
	event("RX", detail);
	serial_simulator_trace_line(trace, "->", detail);
}

static void arm_fault(const char *key) {
	char action[32] = { 0 };
	if (read_fault(key, action, sizeof(action))) {
		snprintf(status_fault, sizeof(status_fault), "%s", action);
		if (!strcmp(action, "stall")) {
			stalled = true;
		} else if (!strcmp(action, "close")) {
			event("CLOSE", key);
			status_fault[0] = 0;
			running = 0;
		}
	}
}

static void dispatch(const uint8_t *frame) {
	uint16_t expected = crc16(frame, 10);
	uint16_t actual = frame[10] | ((uint16_t)frame[11] << 8);
	if (frame[0] != 0xA5 || frame[1] != 0x5A) {
		reject("header");
		return;
	}
	if (expected != actual) {
		reject("crc");
		return;
	}
	uint8_t command = frame[2];
	uint32_t value = frame_value(frame);
	if (command == 0x04) {
		if (!reserved_zero(frame) || value != 0) {
			reject("shutter_fields");
			return;
		}
		accept("SHUTTER", frame);
		return;
	}
	if (command == 0x20) {
		if (!reserved_zero(frame) || value != 0) {
			reject("stop_fields");
			return;
		}
		accept("STOP", frame);
		serial_motion_stop(&motion);
		motion_active = batch_active = stalled = false;
		next_status_time = serial_motion_time() + 0.02;
		motion_status = STATUS_FORWARD;
		return;
	}
	if (command == 0x40 || command == 0x41) {
		if (!reserved_zero(frame) || value > 0xFFFFFF) {
			reject("move_fields");
			return;
		}
		const char *name = command == 0x40 ? "MOVE_FORWARD" : "MOVE_BACKWARD";
		accept(name, frame);
		arm_fault(name);
		double position = serial_motion_update(&motion);
		serial_motion_start(&motion, position + (command == 0x40 ? value : -(double)value), speed == 0xFF ? 400 : 200);
		motion_status = command == 0x40 ? STATUS_FORWARD : STATUS_BACKWARD;
		motion_active = true;
		batch_active = false;
		return;
	}
	if ((command & ~0x0A) == 0x80) {
		if ((frame[3] != 0 && frame[3] != 0xFF) || frame[4] != 0 || frame[5] != 0 || value > 0xFFFFFF) {
			reject("config_fields");
			return;
		}
		accept("CONFIG", frame);
		arm_fault("CONFIG");
		config_flags = command & 0x0A;
		speed = frame[3];
		step_length = value;
		return;
	}
	if ((command & ~0x0A) == 0x10) {
		if (frame[3] > 99 || frame[4] < 1 || frame[4] > 9 || frame[5] < 1 || frame[5] > 99 || value > 0xFFFFFE) {
			reject("batch_fields");
			return;
		}
		accept("BATCH_EXEC", frame);
		arm_fault("BATCH_EXEC");
		settle_time = frame[3];
		shutter_per_step = frame[4];
		shutter_interval = frame[5];
		batch_remaining = value + 1;
		batch_back = (command & 0x08) != 0;
		batch_active = true;
		motion_active = false;
		next_status_time = serial_motion_time() + 0.15;
		return;
	}
	reject("command");
}

static void update_operations(void) {
	if (motion_active && !stalled) {
		serial_motion_update(&motion);
		if (motion.duration == 0) {
			motion_active = false;
			send_status(motion_status);
		}
	}
	if (batch_active && !stalled && serial_motion_time() >= next_status_time) {
		if (batch_remaining > 0) {
			send_status(STATUS_BATCH);
			batch_remaining--;
			next_status_time = serial_motion_time() + 0.15;
		} else if (batch_back) {
			send_status(STATUS_BACKWARD);
			batch_active = false;
			next_status_time = 0;
		} else {
			batch_active = false;
			next_status_time = 0;
		}
	}
	if (!motion_active && !batch_active && next_status_time > 0 && serial_motion_time() >= next_status_time) {
		next_status_time = 0;
		send_status(motion_status);
	}
}

static void poll_control(void) {
	char action[32] = { 0 };
	if (read_fault("TRANSPORT", action, sizeof(action)) && !strcmp(action, "close")) {
		event("CLOSE", "transport");
		running = 0;
	}
}

static void read_input(void) {
	uint8_t buffer[64];
	ssize_t count = read(serial_fd, buffer, sizeof(buffer));
	if (count < 0) {
		if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EIO) {
			running = 0;
		}
		return;
	}
	for (ssize_t i = 0; i < count; i++) {
		uint8_t byte = buffer[i];
		if (input_length == 0 && byte != 0xA5) {
			reject("sync");
			continue;
		}
		if (input_length == 1 && byte != 0x5A) {
			reject("header");
			input_length = byte == 0xA5 ? 1 : 0;
			continue;
		}
		input[input_length++] = byte;
		input_time = serial_motion_time();
		if (input_length == FRAME_SIZE) {
			dispatch(input);
			input_length = 0;
		}
	}
}

static bool parse_args(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) {
			headless = true;
		} else if (!strcmp(argv[i], "--trace")) {
			trace = true;
		} else if (i + 1 < argc && !strcmp(argv[i], "--ready-file")) {
			ready_file = argv[++i];
		} else if (i + 1 < argc && !strcmp(argv[i], "--profile")) {
			profile = argv[++i];
		} else {
			fprintf(stderr, "Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|fallback|silent]\n", argv[0]);
			return false;
		}
	}
	return !strcmp(profile, "normal") || !strcmp(profile, "split") || !strcmp(profile, "fallback") || !strcmp(profile, "silent");
}

int main(int argc, char **argv) {
	char port[256] = { 0 };
	if (!parse_args(argc, argv)) {
		return 1;
	}
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	serial_motion_sync(&motion, 0);
	speed = 0;
	suppress_status = !strcmp(profile, "silent");
	events = getenv("INDIGO_WEMACRO_EVENTS") != NULL ? fopen(getenv("INDIGO_WEMACRO_EVENTS"), "w") : NULL;
	fault_file = getenv("INDIGO_WEMACRO_FAULT");
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}
	if (ready_file != NULL && !serial_simulator_write_ready_file(ready_file, "focuser_wemacro", port)) {
		close(serial_fd);
		return 1;
	}
	if (!headless) {
		printf("WeMacro Rail simulator ready on %s\n", port);
	}
	event("OPEN", port);
	while (running) {
		if (!initial_sent) {
			uint8_t status = !strcmp(profile, "fallback") ? 0xF1 : STATUS_INITIAL;
			if (send_status(status)) {
				initial_sent = true;
			}
		}
		poll_control();
		update_operations();
		if (input_length > 0 && serial_motion_time() - input_time > 0.2) {
			reject("short");
			input_length = 0;
		}
		fd_set readers;
		FD_ZERO(&readers);
		FD_SET(serial_fd, &readers);
		struct timeval timeout = { .tv_sec = 0, .tv_usec = 10000 };
		int result = select(serial_fd + 1, &readers, NULL, NULL, &timeout);
		if (result > 0 && FD_ISSET(serial_fd, &readers)) {
			read_input();
		} else if (result < 0 && errno != EINTR) {
			break;
		}
	}
	event("CLOSE", "simulator");
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	if (events != NULL) {
		fclose(events);
	}
	(void)step_length;
	(void)settle_time;
	(void)shutter_per_step;
	(void)shutter_interval;
	(void)config_flags;
	return 0;
}
