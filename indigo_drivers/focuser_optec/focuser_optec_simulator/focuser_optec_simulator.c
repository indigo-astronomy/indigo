// Optec TCF-S/TCF-S3 focuser simulator
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "../../../indigo_test/simulator_common/serial_motion.h"
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

typedef enum { FREE_MODE, MANUAL_MODE, AUTO_A_MODE, AUTO_B_MODE, SLEEP_MODE } controller_mode;

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static bool headless, trace, quiet, stalled;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1, maximum = 9999;
static int slope[2] = { 86, 42 }, sign_value[2], delay_value[2];
static double temperature = 24.5, sleep_temperature, telemetry_time, automatic_temperature;
static serial_motion motion;
static controller_mode mode = FREE_MODE, sleep_previous_mode = AUTO_A_MODE;
static int sleep_position, automatic_position;

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

static void update_motion(void) {
	if (mode == AUTO_A_MODE || mode == AUTO_B_MODE) {
		int slot = mode == AUTO_B_MODE;
		double signed_slope = sign_value[slot] ? -slope[slot] : slope[slot];
		int target = automatic_position + (int)((temperature - automatic_temperature) * signed_slope);
		target = target < 0 ? 0 : target > maximum ? maximum : target;
		if (!stalled && target != (int)motion.target) {
			serial_motion_start(&motion, target, 200);
		}
	}
	if (!stalled) {
		serial_motion_update(&motion);
	}
}

static bool exact_digits(const char *text, int count, int *value) {
	if ((int)strlen(text) != count) {
		return false;
	}
	int result = 0;
	for (int index = 0; index < count; index++) {
		if (!isdigit((unsigned char)text[index])) {
			return false;
		}
		result = result * 10 + text[index] - '0';
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
		int position = atoi(action);
		position = position < 0 ? 0 : position > maximum ? maximum : position;
		serial_motion_sync(&motion, position);
		stalled = false;
		unlink(fault_file);
		return false;
	}
	if (!strcmp(key, "temperature")) {
		temperature = strtod(action, NULL);
		unlink(fault_file);
		return false;
	}
	if (strcmp(key, command)) {
		*action = 0;
		return false;
	}
	bool persistent = !strncmp(action, "always_", 7);
	if (persistent) {
		memmove(action, action + 7, strlen(action + 7) + 1);
	}
	action[size - 1] = 0;
	if (!persistent) {
		unlink(fault_file);
	}
	return true;
}

static bool write_reply(const char *payload, const char *action) {
	if (!strcmp(action, "silent") || !strcmp(action, "reject")) {
		return true;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		close(serial_fd);
		serial_fd = -1;
		return true;
	}
	if (!strcmp(action, "malformed")) {
		payload = "INVALID";
	} else if (!strcmp(action, "badvalue")) {
		payload = "P=ABCD";
	} else if (!strcmp(action, "error1")) {
		payload = "ER=1";
	} else if (!strcmp(action, "error2")) {
		payload = "ER=2";
	} else if (!strcmp(action, "error3")) {
		payload = "ER=3";
	}
	char frame[160];
	if (!strcmp(action, "overlong")) {
		snprintf(frame, sizeof(frame), "0123456789012345678901234567890123456789012345678901234567890123456789\n\r");
	} else {
		snprintf(frame, sizeof(frame), "%s\n\r", payload);
	}
	event("TX", frame);
	if (trace) {
		fprintf(stderr, "<- %s", frame);
	}
	size_t length = strlen(frame);
	if (!strcmp(action, "partial") || !strcmp(action, "unterminated")) {
		return serial_simulator_write_all(serial_fd, frame, length > 2 ? length - 2 : 0);
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

static bool dispatch(const char *command) {
	event("RX", command);
	if (trace) {
		fprintf(stderr, "-> %s\n", command);
	}
	char action[64] = { 0 }, reply[32];
	read_fault(command, action, sizeof(action));
	if (!strcmp(action, "reject")) {
		return true;
	}
	update_motion();
	int value = 0;
	if (!strcmp(command, "FMMODE")) {
		mode = MANUAL_MODE;
		stalled = false;
		return write_reply("!", action);
	}
	if (!strcmp(command, "FWAKUP") && mode == SLEEP_MODE) {
		mode = MANUAL_MODE;
		return write_reply("WAKE", action);
	}
	if (!strcmp(command, "FQUIT0")) {
		quiet = false;
		return write_reply("DONE", action);
	}
	if (!strcmp(command, "FQUIT1")) {
		quiet = true;
		return write_reply("DONE", action);
	}
	if (mode != MANUAL_MODE) {
		return false;
	}
	if (!strcmp(command, "FFMODE")) {
		mode = FREE_MODE;
		return write_reply("END", action);
	}
	if (!strcmp(command, "FAMODE") || !strcmp(command, "FBMODE")) {
		mode = command[1] == 'A' ? AUTO_A_MODE : AUTO_B_MODE;
		sleep_previous_mode = mode;
		automatic_position = (int)motion.position;
		automatic_temperature = temperature;
		telemetry_time = serial_motion_time();
		return true;
	}
	if (!strcmp(command, "FPOSRO")) {
		snprintf(reply, sizeof(reply), "P=%04d", (int)motion.position);
		return write_reply(reply, action);
	}
	if (!strcmp(command, "FTMPRO")) {
		snprintf(reply, sizeof(reply), "T=%+05.1f", temperature);
		return write_reply(reply, action);
	}
	if (!strcmp(command, "FREADA") || !strcmp(command, "FREADB")) {
		int slot = command[5] == 'B';
		snprintf(reply, sizeof(reply), "%c=%04d", slot ? 'B' : 'A', slope[slot]);
		return write_reply(reply, action);
	}
	if (!strcmp(command, "FTxxxA") || !strcmp(command, "FTxxxB")) {
		int slot = command[5] == 'B';
		snprintf(reply, sizeof(reply), "%c=%d", slot ? 'B' : 'A', sign_value[slot]);
		return write_reply(reply, action);
	}
	if ((!strncmp(command, "FI", 2) || !strncmp(command, "FO", 2)) && exact_digits(command + 2, 4, &value) && value <= maximum) {
		int target = (int)motion.position + (command[1] == 'I' ? -value : value);
		target = target < 0 ? 0 : target > maximum ? maximum : target;
		if (!strcmp(action, "stall")) {
			stalled = true;
			action[0] = 0;
		} else {
			stalled = false;
			serial_motion_start(&motion, target, 200);
		}
		return write_reply("*", action);
	}
	if ((!strncmp(command, "FLA", 3) || !strncmp(command, "FLB", 3)) && exact_digits(command + 3, 3, &value)) {
		slope[command[2] == 'B'] = value;
		return write_reply("DONE", action);
	}
	if ((!strncmp(command, "FZAxx", 5) || !strncmp(command, "FZBxx", 5)) && (command[5] == '0' || command[5] == '1')) {
		sign_value[command[2] == 'B'] = command[5] - '0';
		return write_reply("DONE", action);
	}
	if ((!strncmp(command, "FDA", 3) || !strncmp(command, "FDB", 3)) && exact_digits(command + 3, 3, &value)) {
		delay_value[command[2] == 'B'] = value;
		return write_reply("DONE", action);
	}
	if (!strcmp(command, "FCENTR")) {
		serial_motion_start(&motion, maximum == 7000 ? 3500 : 5000, 200);
		return write_reply("CENTER", action);
	}
	if (!strcmp(command, "FSLEEP")) {
		sleep_position = (int)motion.position;
		sleep_temperature = temperature;
		mode = SLEEP_MODE;
		return write_reply("ZZZ", action);
	}
	if (!strcmp(command, "FHOME")) {
		int slot = sleep_previous_mode == AUTO_B_MODE;
		double signed_slope = sign_value[slot] ? -slope[slot] : slope[slot];
		int target = sleep_position + (int)((temperature - sleep_temperature) * signed_slope);
		target = target < 0 ? 0 : target > maximum ? maximum : target;
		serial_motion_start(&motion, target, 200);
		return write_reply("DONE", action);
	}
	return false;
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
			printf("Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|alternate|tcf-s]\n", argv[0]);
			exit(0);
		} else {
			fprintf(stderr, "Unknown/incomplete option: %s\n", argv[index]);
			return false;
		}
	}
	return !strcmp(profile, "normal") || !strcmp(profile, "split") || !strcmp(profile, "alternate") || !strcmp(profile, "tcf-s");
}

int main(int argc, char **argv) {
	(void)serial_simulator_trace_line;
	if (!parse_args(argc, argv)) {
		return 1;
	}
	maximum = !strcmp(profile, "tcf-s") ? 7000 : 9999;
	int initial = !strcmp(profile, "alternate") ? 1234 : 5000;
	serial_motion_sync(&motion, initial);
	if (!strcmp(profile, "alternate")) {
		temperature = -5.5;
		slope[0] = 12;
		sign_value[0] = 1;
	}
	fault_file = getenv("INDIGO_OPTEC_FAULT");
	const char *event_path = getenv("INDIGO_OPTEC_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	char port[128];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_optec_simulator", port))) {
		if (events) {
			fclose(events);
		}
		return 1;
	}
	if (!headless) {
		printf("Optec focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}
	char command[16];
	size_t used = 0;
	while (running) {
		update_motion();
		if ((mode == AUTO_A_MODE || mode == AUTO_B_MODE) && !quiet && serial_motion_time() - telemetry_time >= 1) {
			char reply[32];
			snprintf(reply, sizeof(reply), "P=%04d", (int)motion.position);
			write_reply(reply, "");
			snprintf(reply, sizeof(reply), "T=%+05.1f", temperature);
			write_reply(reply, "");
			telemetry_time = serial_motion_time();
		}
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
				if (used + 1 >= sizeof(command)) {
					used = 0;
				}
				command[used++] = bytes[index];
				command[used] = 0;
				size_t expected = used >= 5 && !strncmp(command, "FHOME", 5) ? 5 : 6;
				if (used == expected) {
					dispatch(command);
					used = 0;
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
