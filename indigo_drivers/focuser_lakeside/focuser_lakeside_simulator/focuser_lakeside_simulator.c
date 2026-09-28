// LakesideAstro focuser simulator
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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "../../../indigo_test/simulator_common/serial_motion.h"
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static bool headless, trace, moving, stalled, automatic;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1, backlash, reverse, active_slope = 1, last_reported_position;
static int slope[2] = { 10, 20 }, slope_direction[2] = { 0, 1 }, slope_deadband[2] = { 5, 10 }, slope_period[2] = { 6, 12 };
static double temperature = 23.0;
static serial_motion motion;

static void event(const char *kind, const char *value) {
	if (events) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, value);
		fflush(events);
	}
}

static bool reply(const char *format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if (length < 0 || length >= (int)sizeof(buffer)) {
		return false;
	}
	event("TX", buffer);
	serial_simulator_trace_line(trace, "<-", buffer);
	if (!strcmp(profile, "split") && length > 2) {
		int first = length / 2;
		if (!serial_simulator_write_all(serial_fd, buffer, (size_t)first)) {
			return false;
		}
		usleep(10000);
		return serial_simulator_write_all(serial_fd, buffer + first, (size_t)(length - first));
	}
	return serial_simulator_write_all(serial_fd, buffer, (size_t)length);
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

static bool parse_unsigned(const char *text, int minimum, int maximum, int *value) {
	if (!text || !*text) {
		return false;
	}
	long result = 0;
	for (const char *p = text; *p; p++) {
		if (!isdigit((unsigned char)*p)) {
			return false;
		}
		result = result * 10 + *p - '0';
		if (result > maximum) {
			return false;
		}
	}
	if (result < minimum) {
		return false;
	}
	*value = (int)result;
	return true;
}

static bool read_fault(const char *command, char *action) {
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
		int position;
		if (parse_unsigned(action, 0, 65535, &position)) {
			serial_motion_sync(&motion, position);
			moving = false;
			last_reported_position = position;
		}
		unlink(fault_file);
		return false;
	}
	if (!strcmp(key, "temperature")) {
		char *end;
		double value = strtod(action, &end);
		if (*action && !*end && value >= -100 && value <= 100) {
			temperature = value;
		}
		unlink(fault_file);
		return false;
	}
	if (strcmp(key, command)) {
		*action = 0;
	} else {
		unlink(fault_file);
	}
	return *action != 0;
}

static bool inject(const char *command, char expected) {
	char action[64] = { 0 };
	if (!read_fault(command, action)) {
		return false;
	}
	event("FAULT", action);
	if (!strcmp(action, "silent")) {
		return true;
	} else if (!strcmp(action, "close")) {
		running = 0;
		return true;
	} else if (!strcmp(action, "partial")) {
		reply("%c12", expected ? expected : 'X');
	} else if (!strcmp(action, "overlong")) {
		char buffer[180];
		memset(buffer, '7', sizeof(buffer));
		buffer[0] = expected ? expected : 'X';
		buffer[sizeof(buffer) - 2] = '#';
		buffer[sizeof(buffer) - 1] = 0;
		reply("%s", buffer);
	} else if (!strcmp(action, "wrong_prefix")) {
		reply("Z12#");
	} else if (!strcmp(action, "reject")) {
		reply("!#");
	} else if (!strcmp(action, "negative")) {
		reply("%c-1#", expected ? expected : 'X');
	} else if (!strcmp(action, "overflow")) {
		reply("%c999999999999999999999#", expected ? expected : 'X');
	} else if (!strcmp(action, "stall")) {
		stalled = true;
		return false;
	} else {
		reply("%cBAD#", expected ? expected : 'X');
	}
	return true;
}

static void update_motion(void) {
	if (!moving || stalled) {
		return;
	}
	int position = (int)serial_motion_update(&motion);
	if (position != last_reported_position) {
		last_reported_position = position;
		if (!inject("progress", 'P')) {
			reply("P%d#", position);
		}
	}
	if (motion.duration == 0) {
		moving = false;
		if (!inject("DONE", 'D')) {
			reply("DONE#");
		}
		event("DONE", "motion");
	}
}

static void start_move(int target) {
	serial_motion_start(&motion, target, 2000);
	last_reported_position = (int)motion.position;
	moving = motion.duration > 0;
	if (!moving) {
		reply("DONE#");
	}
}

static void value_reply(char prefix, int value) {
	reply("%c%d#", prefix, value);
}

static void dispatch(const char *command) {
	event("RX", command);
	serial_simulator_trace_line(trace, "->", command);
	char expected = 0;
	if (!strcmp(command, "??")) expected = 'O';
	else if (!strcmp(command, "?P")) expected = 'P';
	else if (!strcmp(command, "?B")) expected = 'B';
	else if (!strcmp(command, "?D")) expected = 'D';
	else if (!strcmp(command, "?T")) expected = 'T';
	else if (command[0] == '?' && command[1] && !command[2]) expected = command[1];
	else if (!strncmp(command, "CR", 2)) expected = 'O';
	if (inject(command, expected)) {
		return;
	}
	int value;
	if (!strcmp(command, "??")) {
		reply("OK#");
	} else if (!strcmp(command, "?P")) {
		value_reply('P', (int)serial_motion_update(&motion));
	} else if (!strcmp(command, "?B")) {
		value_reply('B', backlash);
	} else if (!strcmp(command, "?D")) {
		value_reply('D', reverse);
	} else if (!strcmp(command, "?T")) {
		value_reply('T', (int)(temperature * 2));
	} else if (!strcmp(command, "CTF")) {
		automatic = false;
	} else if (!strcmp(command, "CTN")) {
		automatic = true;
	} else if (!strcmp(command, "CH")) {
		serial_motion_stop(&motion);
		moving = stalled = false;
		last_reported_position = (int)motion.position;
	} else if (!strncmp(command, "CI", 2) && parse_unsigned(command + 2, 0, 65535, &value)) {
		int position = (int)serial_motion_update(&motion);
		start_move(position > value ? position - value : 0);
	} else if (!strncmp(command, "CO", 2) && parse_unsigned(command + 2, 0, 65535, &value)) {
		int position = (int)serial_motion_update(&motion);
		start_move(position + value > 65535 ? 65535 : position + value);
	} else if (!strncmp(command, "CRB", 3) && parse_unsigned(command + 3, 0, 65535, &value)) {
		backlash = value;
		reply("OK#");
	} else if (!strncmp(command, "CRD", 3) && parse_unsigned(command + 3, 0, 1, &value)) {
		reverse = value;
		reply("OK#");
	} else if (!strncmp(command, "CRg", 3) && parse_unsigned(command + 3, 1, 2, &value)) {
		active_slope = value;
		reply("OK#");
	} else if (!strcmp(command, "?1") || !strcmp(command, "?2")) {
		int index = command[1] - '1';
		value_reply(command[1], slope[index]);
	} else if (!strcmp(command, "?a") || !strcmp(command, "?b")) {
		int index = command[1] - 'a';
		value_reply(command[1], slope_direction[index]);
	} else if (!strcmp(command, "?c") || !strcmp(command, "?d")) {
		int index = command[1] - 'c';
		value_reply(command[1], slope_deadband[index]);
	} else if (!strcmp(command, "?e") || !strcmp(command, "?f")) {
		int index = command[1] - 'e';
		value_reply(command[1], slope_period[index]);
	} else if ((!strncmp(command, "CR1", 3) || !strncmp(command, "CR2", 3)) && parse_unsigned(command + 3, 0, 127, &value)) {
		int index = command[2] - '1';
		slope[index] = value;
		reply("OK#");
	} else if ((!strncmp(command, "CRa", 3) || !strncmp(command, "CRb", 3)) && parse_unsigned(command + 3, 0, 1, &value)) {
		int index = command[2] - 'a';
		slope_direction[index] = value;
		reply("OK#");
	} else if ((!strncmp(command, "CRc", 3) || !strncmp(command, "CRd", 3)) && parse_unsigned(command + 3, 0, 65535, &value)) {
		int index = command[2] - 'c';
		slope_deadband[index] = value;
		reply("OK#");
	} else if ((!strncmp(command, "CRe", 3) || !strncmp(command, "CRf", 3)) && parse_unsigned(command + 3, 0, 65535, &value)) {
		int index = command[2] - 'e';
		slope_period[index] = value;
		reply("OK#");
	} else {
		reply("!#");
	}
	(void)automatic;
	(void)active_slope;
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
		} else {
			fprintf(stderr, "Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|alternate]\n", argv[0]);
			return 1;
		}
	}
	serial_motion_sync(&motion, !strcmp(profile, "alternate") ? 1000 : 32768);
	temperature = !strcmp(profile, "alternate") ? -5.5 : 23.0;
	last_reported_position = (int)motion.position;
	const char *event_path = getenv("INDIGO_LAKESIDE_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	fault_file = getenv("INDIGO_LAKESIDE_FAULT");
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	char port[128], command[128];
	size_t used = 0;
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_lakeside", port))) {
		return 1;
	}
	if (!headless) {
		printf("LakesideAstro simulator on %s\n", port);
		fflush(stdout);
	}
	while (running) {
		update_motion();
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);
		struct timeval timeout = { 0, 10000 };
		int selected = select(serial_fd + 1, &fds, NULL, NULL, &timeout);
		if (selected <= 0) {
			continue;
		}
		char buffer[64];
		ssize_t count = read(serial_fd, buffer, sizeof(buffer));
		if (count <= 0) {
			if (count < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EIO) {
				break;
			}
			continue;
		}
		for (ssize_t i = 0; i < count; i++) {
			if (buffer[i] == '#') {
				command[used] = 0;
				if (used) {
					dispatch(command);
				}
				used = 0;
			} else if (used + 1 < sizeof(command)) {
				command[used++] = buffer[i];
			} else {
				used = 0;
				reply("!#");
			}
		}
	}
	if (events) {
		fclose(events);
	}
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
