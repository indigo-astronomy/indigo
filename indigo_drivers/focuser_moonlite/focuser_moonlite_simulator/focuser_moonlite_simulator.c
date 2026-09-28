// MoonLite focuser simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// This simulator was refactored by a Codex agent.

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

typedef struct {
	serial_motion motion;
	unsigned target, speed, step_mode;
	bool moving, stalled;
} motor_state;

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static bool headless, trace, compensation_enabled, temperature_pending;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static double temperature_started;
static int16_t temperature = 0x002e;
static int8_t coefficient, temperature_offset;
static unsigned backlight[3] = { 0x80, 0x80, 0x80 }, contrast = 0x80, temperature_scale, motor_scale[2] = { 1, 1 };
static motor_state motors[2];

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

static void event(const char *kind, const char *frame) {
	if (events) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, frame);
		fflush(events);
	}
}

static void update_motor(motor_state *motor) {
	if (!motor->stalled) {
		serial_motion_update(&motor->motion);
	}
	if (motor->moving && !motor->stalled && motor->motion.duration == 0) {
		motor->moving = false;
	}
}

static bool exact_hex(const char *text, int digits, unsigned *value) {
	if ((int)strlen(text) != digits) {
		return false;
	}
	unsigned result = 0;
	for (int index = 0; index < digits; index++) {
		unsigned char c = (unsigned char)text[index];
		if (!isxdigit(c)) {
			return false;
		}
		result = result * 16 + (unsigned)(isdigit(c) ? c - '0' : toupper(c) - 'A' + 10);
	}
	*value = result;
	return true;
}

static bool read_fault(const char *command, char *action, size_t size) {
	char key[64] = { 0 };
	FILE *file = fault_file ? fopen(fault_file, "r") : NULL;
	if (!file) {
		return false;
	}
	if (fscanf(file, "%63s %63s", key, action) != 2) {
		*action = 0;
	}
	fclose(file);
	if (!strcmp(key, "external")) {
		serial_motion_sync(&motors[0].motion, strtol(action, NULL, 10));
		motors[0].target = (unsigned)motors[0].motion.position;
		motors[0].moving = motors[0].stalled = false;
		unlink(fault_file);
		return false;
	}
	if (!strcmp(key, "temperature")) {
		temperature = (int16_t)(strtod(action, NULL) * 2);
		unlink(fault_file);
		return false;
	}
	if (strcmp(key, command)) {
		*action = 0;
		return false;
	}
	action[size - 1] = 0;
	unlink(fault_file);
	return true;
}

static bool write_reply(const char *payload, const char *action) {
	char frame[128];
	if (!strcmp(action, "silent")) {
		return true;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		close(serial_fd);
		serial_fd = -1;
		return true;
	}
	if (!strcmp(action, "malformed")) {
		payload = "ZZ";
	} else if (!strcmp(action, "badvalue")) {
		payload = "03";
	}
	snprintf(frame, sizeof(frame), "%s#", payload);
	event("TX", frame);
	if (trace) {
		fprintf(stderr, "<- %s\n", frame);
	}
	size_t length = strlen(frame);
	if (!strcmp(action, "partial") || !strcmp(action, "unterminated")) {
		return serial_simulator_write_all(serial_fd, frame, length > 0 ? length - 1 : 0);
	}
	if (!strcmp(action, "overlong")) {
		return serial_simulator_write_all(serial_fd, "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789#", 74);
	}
	if (!strcmp(profile, "split") && length > 2) {
		if (!serial_simulator_write_all(serial_fd, frame, 1)) {
			return false;
		}
		usleep(10000);
		return serial_simulator_write_all(serial_fd, frame + 1, length - 1);
	}
	return serial_simulator_write_all(serial_fd, frame, length);
}

static unsigned motor_speed(unsigned encoded) {
	switch (encoded) {
		case 0x02: return 250;
		case 0x04: return 125;
		case 0x08: return 63;
		case 0x10: return 32;
		case 0x20: return 16;
		default: return 0;
	}
}

static bool dispatch(const char *command) {
	event("RX", command);
	if (trace) {
		fprintf(stderr, "-> %s\n", command);
	}
	char action[64] = { 0 };
	read_fault(command, action, sizeof(action));
	int port = command[0] == '2' ? 1 : 0;
	const char *body = command + port;
	motor_state *motor = &motors[port];
	update_motor(motor);
	unsigned value = 0;
	char reply[32];
	if (!strcmp(action, "reject")) {
		return true;
	}
	if (!strcmp(body, "C") && port == 0) {
		temperature_started = serial_motion_time();
		temperature_pending = true;
	} else if (!strcmp(body, "FG")) {
		if (!strcmp(action, "stall")) {
			motor->stalled = true;
			action[0] = 0;
		}
		serial_motion_start(&motor->motion, motor->target, motor_speed(motor->speed));
		motor->moving = motor->motion.duration > 0;
	} else if (!strcmp(body, "FQ")) {
		serial_motion_stop(&motor->motion);
		motor->target = (unsigned)motor->motion.position;
		motor->moving = motor->stalled = false;
	} else if (!strcmp(body, "GC") && port == 0) {
		snprintf(reply, sizeof(reply), "%02X", (uint8_t)coefficient);
		return write_reply(reply, action);
	} else if (!strcmp(body, "GD")) {
		snprintf(reply, sizeof(reply), "%02X", motor->speed);
		return write_reply(reply, action);
	} else if (!strcmp(body, "GH")) {
		snprintf(reply, sizeof(reply), "%02X", motor->step_mode);
		return write_reply(reply, action);
	} else if (!strcmp(body, "GI")) {
		snprintf(reply, sizeof(reply), "%02X", motor->moving ? 1 : 0);
		return write_reply(reply, action);
	} else if (!strcmp(body, "GN")) {
		snprintf(reply, sizeof(reply), "%04X", motor->target);
		return write_reply(reply, action);
	} else if (!strcmp(body, "GP")) {
		snprintf(reply, sizeof(reply), "%04X", (unsigned)motor->motion.position & 0xffff);
		return write_reply(reply, action);
	} else if (!strcmp(body, "GT") && port == 0) {
		if (temperature_pending && serial_motion_time() - temperature_started < 0.75) {
			return true;
		}
		temperature_pending = false;
		snprintf(reply, sizeof(reply), "%04X", (uint16_t)(temperature + temperature_offset));
		return write_reply(reply, action);
	} else if (!strcmp(body, "GV")) {
		return write_reply("12", action);
	} else if (!strcmp(body, "+") && port == 0) {
		compensation_enabled = true;
	} else if (!strcmp(body, "-") && port == 0) {
		compensation_enabled = false;
	} else if (!strncmp(body, "SC", 2) && port == 0 && exact_hex(body + 2, 2, &value)) {
		coefficient = (int8_t)value;
	} else if (!strncmp(body, "SD", 2) && exact_hex(body + 2, 2, &value) && motor_speed(value)) {
		motor->speed = value;
	} else if (!strcmp(body, "SF")) {
		motor->step_mode = 0;
	} else if (!strcmp(body, "SH")) {
		motor->step_mode = 0xff;
	} else if (!strncmp(body, "SN", 2) && exact_hex(body + 2, 4, &value)) {
		motor->target = value;
	} else if (!strncmp(body, "SP", 2) && exact_hex(body + 2, 4, &value)) {
		serial_motion_sync(&motor->motion, value);
		motor->target = value;
		motor->moving = motor->stalled = false;
	} else if (!strncmp(body, "PO", 2) && port == 0 && exact_hex(body + 2, 2, &value)) {
		temperature_offset = (int8_t)value;
	} else if (!strncmp(body, "PS", 2) && port == 0 && exact_hex(body + 2, 2, &value) && value <= 1) {
		temperature_scale = value;
	} else if (!strncmp(body, "PR", 2) && port == 0 && exact_hex(body + 2, 2, &value)) {
		backlight[0] = value;
	} else if (!strncmp(body, "PG", 2) && port == 0 && exact_hex(body + 2, 2, &value)) {
		backlight[1] = value;
	} else if (!strncmp(body, "PB", 2) && port == 0 && exact_hex(body + 2, 2, &value)) {
		backlight[2] = value;
	} else if (!strncmp(body, "PC", 2) && port == 0 && exact_hex(body + 2, 2, &value)) {
		contrast = value;
	} else if (body[0] == 'P' && (body[1] == 'X' || body[1] == 'y') && port == 0 && exact_hex(body + 2, 4, &value)) {
		motor_scale[body[1] == 'y'] = value;
	} else if (!strcmp(body, "PH01") || !strcmp(body, "PH02")) {
		motor_state *home = &motors[body[3] - '1'];
		home->target = 0;
		serial_motion_start(&home->motion, 0, motor_speed(home->speed));
		home->moving = home->motion.duration > 0;
	} else {
		return false;
	}
	return true;
}

static bool parse_args(int argc, char **argv) {
	for (int index = 1; index < argc; index++) {
		if (!strcmp(argv[index], "--headless")) {
			headless = true;
		} else if (!strcmp(argv[index], "--trace")) {
			trace = true;
		} else if (index + 1 < argc && !strcmp(argv[index], "--ready-file")) {
			ready_file = argv[++index];
		} else if (index + 1 < argc && !strcmp(argv[index], "--profile")) {
			profile = argv[++index];
		} else if (!strcmp(argv[index], "--help") || !strcmp(argv[index], "-h")) {
			printf("Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|alternate]\n", argv[0]);
			exit(0);
		} else {
			fprintf(stderr, "Unknown/incomplete option: %s\n", argv[index]);
			return false;
		}
	}
	return !strcmp(profile, "normal") || !strcmp(profile, "split") || !strcmp(profile, "alternate");
}

int main(int argc, char **argv) {
	(void)serial_simulator_trace_line;
	if (!parse_args(argc, argv)) {
		return 1;
	}
	unsigned initial = !strcmp(profile, "alternate") ? 0x1234 : 0x8000;
	serial_motion_sync(&motors[0].motion, initial);
	serial_motion_sync(&motors[1].motion, 0x4000);
	motors[0].target = initial;
	motors[1].target = 0x4000;
	motors[0].speed = !strcmp(profile, "alternate") ? 0x10 : 0x02;
	motors[1].speed = 0x04;
	motors[0].step_mode = !strcmp(profile, "alternate") ? 0xff : 0;
	coefficient = !strcmp(profile, "alternate") ? -5 : 0;
	temperature = !strcmp(profile, "alternate") ? -11 : 0x002e;
	fault_file = getenv("INDIGO_MOONLITE_FAULT");
	const char *event_path = getenv("INDIGO_MOONLITE_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	char port[128];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_moonlite_simulator", port))) {
		if (events) {
			fclose(events);
		}
		return 1;
	}
	if (!headless) {
		printf("MoonLite focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}
	char frame[128];
	size_t used = 0;
	bool in_frame = false, overflow = false;
	while (running) {
		update_motor(&motors[0]);
		update_motor(&motors[1]);
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(serial_fd, &reads);
		struct timeval timeout = { 0, 10000 };
		if (select(serial_fd + 1, &reads, NULL, NULL, &timeout) <= 0) {
			continue;
		}
		char bytes[64];
		ssize_t count = read(serial_fd, bytes, sizeof(bytes));
		if (count > 0) {
			for (ssize_t index = 0; index < count; index++) {
				char c = bytes[index];
				if (c == ':') {
					in_frame = true;
					overflow = false;
					used = 0;
				} else if (in_frame && c == '#') {
					if (!overflow && used > 0) {
						frame[used] = 0;
						dispatch(frame);
					}
					in_frame = false;
					used = 0;
				} else if (in_frame && !overflow) {
					if (used + 1 < sizeof(frame)) {
						frame[used++] = c;
					} else {
						overflow = true;
					}
				}
			}
		} else if (count < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EIO) {
			break;
		}
	}
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	if (events) {
		fclose(events);
	}
	return 0;
}
