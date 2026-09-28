// iOptron iEAF focuser simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/select.h>
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

static const char *profile = "normal", *ready_file;
static int model = 2;
static const char *fault_file;
static FILE *events;
static bool trace, headless, motion_active, injected, failed_read, frozen;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1, direction = 1;
static int temperature = 29465;
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
		serial_simulator_write_all(serial_fd, buffer, (size_t)length / 2);
		usleep(10000);
		return serial_simulator_write_all(serial_fd, buffer + length / 2, (size_t)(length - length / 2));
	}
	return serial_simulator_write_all(serial_fd, buffer, (size_t)length);
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

static void update_motion(void) {
	if (frozen) {
		return;
	}
	int position = (int)serial_motion_update(&motion);
	if (motion_active && motion.duration == 0) {
		motion_active = false;
		(void)position;
		event("DONE", "motion");
	}
}

static bool inject(char command) {
	if (command == 'I' && failed_read) {
		failed_read = false;
		reply("invalid#");
		return true;
	}
	char action[64] = "", key[32] = { 0 };
	FILE *file = fault_file ? fopen(fault_file, "r") : NULL;
	if (file) {
		if (fscanf(file, "%31s %63s", key, action) != 2) {
			*action = 0;
		}
		fclose(file);
		if (!strcmp(key, "external")) {
			serial_motion_sync(&motion, atoi(action));
			motion_active = false;
			unlink(fault_file);
			return false;
		}
		if (!strcmp(key, "temperature")) {
			temperature = atoi(action);
			unlink(fault_file);
			return false;
		}
		if (key[0] != command || key[1]) {
			*action = 0;
		} else {
			unlink(fault_file);
		}
	}
	if (!injected && !strncmp(profile, "init_", 5) && command == profile[5]) {
		injected = true;
		snprintf(action, sizeof(action), "%s", profile + 7);
	}
	if (!*action) {
		return false;
	}
	event("FAULT", action);
	if (!strcmp(action, "readfail")) {
		failed_read = true;
		return false;
	} else if (!strcmp(action, "stall")) {
		frozen = true;
		return true;
	} else if (!strcmp(action, "silent") || !strcmp(action, "ignore")) {
		return true;
	} else if (!strcmp(action, "close")) {
		running = 0;
		return true;
	} else if (!strcmp(action, "overlong")) {
		char buffer[160];
		memset(buffer, '7', sizeof(buffer));
		buffer[158] = '#';
		buffer[159] = 0;
		reply("%s", buffer);
	} else if (!strcmp(action, "short")) {
		reply("1#");
	} else if (!strcmp(action, "partial")) {
		reply("0001000");
	} else if (!strcmp(action, "badflag")) {
		reply("00010002294651#");
	} else if (!strcmp(action, "baddir")) {
		reply("00010000294652#");
	} else if (!strcmp(action, "badpos")) {
		reply("99999990294651#");
	} else if (!strcmp(action, "trailing")) {
		reply("00010000294651x#");
	} else {
		reply("invalid#");
	}
	return true;
}

static void dispatch(const char *text) {
	char command = !strcmp(text, ":DeviceInfo#") ? 'D' : text[2];
	event("RX", text);
	serial_simulator_trace_line(trace, "->", text);
	if (inject(command)) {
		return;
	}
	int position = frozen ? (int)motion.position : (int)serial_motion_update(&motion);
	switch (command) {
		case 'D': reply("%06d%02d%04d#", position, !strcmp(profile, "unknown") ? 9 : model, 4); break;
		case 'I': reply("%07d%d%05d%d#", position, frozen || motion.duration > 0, temperature, direction); break;
		case 'M': {
			int value = atoi(text + 3);
			serial_motion_start(&motion, value < 0 ? 0 : value > 99999 ? 99999 : value, 5000);
			motion_active = true;
			break;
		}
		case 'Q':
			if (frozen) {
				serial_motion_sync(&motion, motion.position);
			} else {
				serial_motion_stop(&motion);
			}
			frozen = motion_active = false;
			break;
		case 'Z': serial_motion_sync(&motion, 0); motion_active = false; break;
		case 'R': direction = !direction; break;
	}
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
			fprintf(stderr, "Unknown/incomplete option: %s\n", argv[i]);
			return 1;
		}
	}
	serial_motion_sync(&motion, 1000);
	if (!strcmp(profile, "iafs") || !strcmp(profile, "alternate")) {
		model = 3;
	}
	if (!strcmp(profile, "alternate")) {
		temperature = 26815;
		serial_motion_sync(&motion, 500);
	}
	const char *event_path = getenv("INDIGO_IOPTRON_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	fault_file = getenv("INDIGO_IOPTRON_FAULT");
	char port[128], command[128];
	size_t used = 0;
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_ioptron", port))) {
		return 1;
	}
	if (!headless) {
		printf("IOPTRON simulator on %s\n", port);
		fflush(stdout);
	}
	while (running) {
		update_motion();
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);
		struct timeval timeout = { 0, 10000 };
		if (select(serial_fd + 1, &fds, NULL, NULL, &timeout) <= 0) {
			continue;
		}
		char byte;
		if (read(serial_fd, &byte, 1) != 1) {
			usleep(10000);
			continue;
		}
		if (byte == ':') {
			used = 0;
		}
		if (used + 1 < sizeof(command)) {
			command[used++] = byte;
		}
		if (byte == '#') {
			command[used] = 0;
			dispatch(command);
			used = 0;
		}
	}
	close(serial_fd);
	if (events) {
		fclose(events);
	}
	return 0;
}
