// SteelDriveII serial simulator
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
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "../../../indigo_test/simulator_common/serial_motion.h"
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

#define MAX_POSITION INT_MAX

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static volatile sig_atomic_t running = 1;
static bool headless, trace, use_crc, zeroing, zeroed, stall_injected;
static int serial_fd = -1, limit = 2000, focus = 1234, jogsteps = 50, singlesteps = 1, backlight = 50;
static int tcomp, tcomp_period = 1000, tcomp_sensor = 2, use_endstop, pwm = 50, pid_ctrl, pid_sensor, ambient_sensor = 1, auto_dew;
static double temp0 = 22.45, temp1 = 21.78, temp0_offset, temp1_offset, tcomp_factor = 2.5, tcomp_delta = 0.5, pid_target, pid_dew_offset;
static char device_name[20] = "BP_SD_01";
static serial_motion motion;

static void reset_controller(void) {
	limit = 2000;
	focus = 1234;
	jogsteps = 50;
	singlesteps = 1;
	backlight = 50;
	tcomp = 0;
	tcomp_period = 1000;
	tcomp_sensor = 2;
	use_endstop = 0;
	pwm = 50;
	pid_ctrl = 0;
	pid_sensor = 0;
	ambient_sensor = 1;
	auto_dew = 0;
	temp0_offset = temp1_offset = 0;
	tcomp_factor = 2.5;
	tcomp_delta = 0.5;
	pid_target = pid_dew_offset = 0;
	zeroing = zeroed = false;
	snprintf(device_name, sizeof(device_name), "BP_SD_01");
	serial_motion_sync(&motion, 1000);
}

static const uint8_t crc_array[256] = {
	0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
	0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
	0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
	0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
	0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
	0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
	0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
	0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
	0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
	0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
	0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
	0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
	0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
	0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
	0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
	0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};

static uint8_t crc8(const char *text, size_t length) {
	uint8_t crc = 0;
	for (size_t i = 0; i < length; i++) {
		crc = crc_array[(uint8_t)text[i] ^ crc];
	}
	return crc;
}

static int hex_value(char value) {
	if (value >= '0' && value <= '9') {
		return value - '0';
	}
	if (value >= 'A' && value <= 'F') {
		return value - 'A' + 10;
	}
	if (value >= 'a' && value <= 'f') {
		return value - 'a' + 10;
	}
	return -1;
}

static void record_event(const char *kind, const char *line) {
	serial_simulator_trace_line(trace, kind, line);
	if (events != NULL) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, line);
		fflush(events);
	}
}

static bool write_text(const char *text, size_t length, bool split) {
	if ((split || !strcmp(profile, "split")) && length > 2) {
		size_t first = length / 2;
		if (!serial_simulator_write_all(serial_fd, text, first)) {
			return false;
		}
		usleep(10000);
		return serial_simulator_write_all(serial_fd, text + first, length - first);
	}
	return serial_simulator_write_all(serial_fd, text, length);
}

static bool send_echo(const char *line) {
	char wire[600];
	int length = snprintf(wire, sizeof(wire), "%s\r\n", line);
	record_event("ECHO", line);
	return length > 0 && length < (int)sizeof(wire) && write_text(wire, (size_t)length, false);
}

static bool send_line(const char *line, bool bad_crc, bool split) {
	char wire[600];
	int length;
	if (use_crc && !strcmp(line, "$BS Hello World!")) {
		length = snprintf(wire, sizeof(wire), "%s\r\n", line);
	} else if (use_crc) {
		length = snprintf(wire, sizeof(wire), "%s*%02X\r\n", line, (uint8_t)(crc8(line, strlen(line)) + (bad_crc ? 1 : 0)));
	} else {
		length = snprintf(wire, sizeof(wire), "%s\r\n", line);
	}
	record_event("TX", line);
	return length > 0 && length < (int)sizeof(wire) && write_text(wire, (size_t)length, split);
}

static bool read_fault(const char *key, char *action, size_t size) {
	char found[80] = { 0 };
	FILE *file = fault_file == NULL ? NULL : fopen(fault_file, "r");
	if (file == NULL) {
		return false;
	}
	if (fscanf(file, "%79s %79s", found, action) != 2) {
		action[0] = 0;
	}
	fclose(file);
	if (strcmp(found, key)) {
		action[0] = 0;
		return false;
	}
	unlink(fault_file);
	action[size - 1] = 0;
	return action[0] != 0;
}

static void apply_external_fault(void) {
	char key[80] = { 0 }, value[80] = { 0 };
	FILE *file = fault_file == NULL ? NULL : fopen(fault_file, "r");
	if (file == NULL) {
		return;
	}
	if (fscanf(file, "%79s %79s", key, value) != 2 || (strcmp(key, "external_position") && strcmp(key, "external_temperature") && strcmp(key, "external_pwm"))) {
		fclose(file);
		return;
	}
	fclose(file);
	unlink(fault_file);
	if (!strcmp(key, "external_position")) {
		char *end;
		long parsed = strtol(value, &end, 10);
		if (!*end && parsed >= 0 && parsed <= limit) {
			serial_motion_sync(&motion, parsed);
			zeroed = false;
		}
	} else if (!strcmp(key, "external_temperature")) {
		char *end;
		double parsed = strtod(value, &end);
		if (!*end && isfinite(parsed) && parsed >= -128 && parsed <= 150) {
			temp0 = parsed;
			temp1 = parsed + 1;
		}
	} else {
		char *end;
		long parsed = strtol(value, &end, 10);
		if (!*end && parsed >= 0 && parsed <= 100) {
			pwm = (int)parsed;
		}
	}
}

static bool injected_reply(const char *key, const char *normal) {
	char action[80] = { 0 };
	if (!read_fault(key, action, sizeof(action))) {
		return false;
	}
	if (!strcmp(action, "silent")) {
		return true;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		return true;
	}
	if (!strcmp(action, "error")) {
		send_line("$BS ERROR: Injected failure!", false, false);
		return true;
	}
	if (!strcmp(action, "bad_crc")) {
		send_line(normal, true, false);
		return true;
	}
	if (!strcmp(action, "split")) {
		send_line(normal, false, true);
		return true;
	}
	send_line("$BS STATUS BROKEN", false, false);
	return true;
}

static bool parse_integer(const char *text, long minimum, long maximum, long *value) {
	if (*text == 0 || isspace((unsigned char)*text)) {
		return false;
	}
	char *end;
	errno = 0;
	long parsed = strtol(text, &end, 10);
	if (errno || *end || parsed < minimum || parsed > maximum) {
		return false;
	}
	*value = parsed;
	return true;
}

static bool parse_float(const char *text, double minimum, double maximum, double *value) {
	if (*text == 0 || isspace((unsigned char)*text) || strchr(text, '.') == NULL) {
		return false;
	}
	char *end;
	errno = 0;
	double parsed = strtod(text, &end);
	if (errno || *end || !isfinite(parsed) || parsed < minimum || parsed > maximum) {
		return false;
	}
	*value = parsed;
	return true;
}

static bool set_integer(const char *command, const char *prefix, long minimum, long maximum, int *value) {
	size_t length = strlen(prefix);
	long parsed;
	if (strncmp(command, prefix, length) || !parse_integer(command + length, minimum, maximum, &parsed)) {
		return false;
	}
	*value = (int)parsed;
	return true;
}

static bool set_float(const char *command, const char *prefix, double minimum, double maximum, double *value) {
	size_t length = strlen(prefix);
	return !strncmp(command, prefix, length) && parse_float(command + length, minimum, maximum, value);
}

static const char *motion_state(void) {
	serial_motion_update(&motion);
	if (motion.duration > 0) {
		return motion.target > motion.origin ? "GOING_UP" : "GOING_DOWN";
	}
	if (zeroing) {
		zeroing = false;
		zeroed = true;
	}
	return zeroed ? "ZEROED" : "STOPPED";
}

static void status_reply(const char *key, const char *format, ...) {
	char line[512], value[400];
	va_list args;
	va_start(args, format);
	vsnprintf(value, sizeof(value), format, args);
	va_end(args);
	snprintf(line, sizeof(line), "$BS STATUS %s:%s", key, value);
	if (!injected_reply(key, line)) {
		send_line(line, false, false);
	}
}

static void ok_reply(const char *key) {
	if (!injected_reply(key, "$BS OK")) {
		send_line("$BS OK", false, false);
	}
}

static bool validate_crc(char *line, bool required) {
	char *star = strrchr(line, '*');
	if (star == NULL) {
		return !required;
	}
	if (star[1] == 0 || star[2] == 0 || star[3] != 0) {
		return false;
	}
	int high = hex_value(star[1]);
	int low = hex_value(star[2]);
	if (high < 0 || low < 0 || crc8(line, (size_t)(star - line)) != (uint8_t)((high << 4) | low)) {
		return false;
	}
	*star = 0;
	return true;
}

static void dispatch(char *command) {
	apply_external_fault();
	serial_motion_update(&motion);
	if (!strcmp(command, "$BS CRC_ENABLE")) {
		use_crc = true;
		ok_reply("CRC_ENABLE");
		return;
	}
	if (!strcmp(command, "$BS CRC_DISABLE")) {
		use_crc = false;
		ok_reply("CRC_DISABLE");
		return;
	}
	if (!strcmp(command, "$BS GET VERSION")) {
		status_reply("VERSION", "0.770");
	} else if (!strcmp(command, "$BS GET NAME")) {
		status_reply("NAME", "%s", device_name);
	} else if (!strncmp(command, "$BS SET NAME:", 13) && command[13] && strlen(command + 13) <= 19 && strpbrk(command + 13, ";:*\r\n") == NULL) {
		snprintf(device_name, sizeof(device_name), "%s", command + 13);
		ok_reply("SET_NAME");
	} else if (!strcmp(command, "$BS GET FOCUS")) {
		status_reply("FOCUS", "%d", focus);
	} else if (set_integer(command, "$BS SET FOCUS:", 0, MAX_POSITION, &focus)) {
		ok_reply("SET_FOCUS");
	} else if (!strcmp(command, "$BS GET JOGSTEPS")) {
		status_reply("JOGSTEPS", "%d", jogsteps);
	} else if (set_integer(command, "$BS SET JOGSTEPS:", 1, MAX_POSITION, &jogsteps)) {
		if (singlesteps > jogsteps) {
			singlesteps = jogsteps;
		}
		ok_reply("SET_JOGSTEPS");
	} else if (!strcmp(command, "$BS GET SINGLESTEPS")) {
		status_reply("SINGLESTEPS", "%d", singlesteps);
	} else if (set_integer(command, "$BS SET SINGLESTEPS:", 1, jogsteps, &singlesteps)) {
		ok_reply("SET_SINGLESTEPS");
	} else if (!strcmp(command, "$BS GET BKLGT")) {
		status_reply("BKLGT", "%d", backlight);
	} else if (set_integer(command, "$BS SET BKLGT:", 0, 100, &backlight)) {
		ok_reply("SET_BKLGT");
	} else if (!strcmp(command, "$BS GET TEMP0_OFS")) {
		status_reply("TEMP0_OFS", "%.2f", temp0_offset);
	} else if (set_float(command, "$BS SET TEMP0_OFS:", -50, 50, &temp0_offset)) {
		ok_reply("SET_TEMP0_OFS");
	} else if (!strcmp(command, "$BS GET TEMP1_OFS")) {
		status_reply("TEMP1_OFS", "%.2f", temp1_offset);
	} else if (set_float(command, "$BS SET TEMP1_OFS:", -50, 50, &temp1_offset)) {
		ok_reply("SET_TEMP1_OFS");
	} else if (!strcmp(command, "$BS GET TCOMP")) {
		status_reply("TCOMP", "%d", tcomp);
	} else if (set_integer(command, "$BS SET TCOMP:", 0, 1, &tcomp)) {
		ok_reply("SET_TCOMP");
	} else if (!strcmp(command, "$BS GET TCOMP_FACTOR")) {
		status_reply("TCOMP_FACTOR", "%.2f", tcomp_factor);
	} else if (set_float(command, "$BS SET TCOMP_FACTOR:", -100000, 100000, &tcomp_factor)) {
		ok_reply("SET_TCOMP_FACTOR");
	} else if (!strcmp(command, "$BS GET TCOMP_PERIOD")) {
		status_reply("TCOMP_PERIOD", "%d", tcomp_period);
	} else if (set_integer(command, "$BS SET TCOMP_PERIOD:", 0, INT_MAX, &tcomp_period)) {
		ok_reply("SET_TCOMP_PERIOD");
	} else if (!strcmp(command, "$BS GET TCOMP_DELTA")) {
		status_reply("TCOMP_DELTA", "%.2f", tcomp_delta);
	} else if (set_float(command, "$BS SET TCOMP_DELTA:", 0, 100, &tcomp_delta)) {
		ok_reply("SET_TCOMP_DELTA");
	} else if (!strcmp(command, "$BS GET TCOMP_SENSOR")) {
		status_reply("TCOMP_SENSOR", "%d", tcomp_sensor);
	} else if (set_integer(command, "$BS SET TCOMP_SENSOR:", 0, 2, &tcomp_sensor)) {
		ok_reply("SET_TCOMP_SENSOR");
	} else if (!strcmp(command, "$BS GET USE_ENDSTOP")) {
		status_reply("USE_ENDSTOP", "%d", use_endstop);
	} else if (set_integer(command, "$BS SET USE_ENDSTOP:", 0, 1, &use_endstop)) {
		ok_reply("SET_USE_ENDSTOP");
	} else if (!strcmp(command, "$BS GET POS")) {
		status_reply("POS", "%d", (int)lround(serial_motion_update(&motion)));
	} else if (!strncmp(command, "$BS SET POS:", 12)) {
		long value;
		if (parse_integer(command + 12, 0, MAX_POSITION, &value)) {
			serial_motion_sync(&motion, value);
			zeroed = value == 0;
			ok_reply("SET_POS");
		} else {
			send_line("$BS ERROR: Unknown command!", false, false);
		}
	} else if (!strcmp(command, "$BS GET LIMIT")) {
		status_reply("LIMIT", "%d", limit);
	} else if (set_integer(command, "$BS SET LIMIT:", 0, MAX_POSITION, &limit)) {
		if (motion.position > limit) {
			serial_motion_sync(&motion, limit);
		}
		ok_reply("SET_LIMIT");
	} else if (!strncmp(command, "$BS GO ", 7)) {
		long value;
		if (parse_integer(command + 7, LONG_MIN, LONG_MAX, &value)) {
			int target = value < 0 ? 0 : value > limit ? limit : (int)value;
			char action[80] = { 0 };
			serial_motion_start(&motion, target, 500);
			zeroed = false;
			if ((read_fault("GO_STALL", action, sizeof(action)) && !strcmp(action, "stall")) || (!strcmp(profile, "stall") && !stall_injected)) {
				motion.duration = 3600000;
				stall_injected = true;
			}
			ok_reply("GO");
		} else {
			send_line("$BS ERROR: Unknown command!", false, false);
		}
	} else if (!strcmp(command, "$BS STOP")) {
		serial_motion_stop(&motion);
		zeroing = false;
		zeroed = false;
		ok_reply("STOP");
	} else if (!strcmp(command, "$BS ZEROING")) {
		if (use_endstop) {
			char action[80] = { 0 };
			serial_motion_start(&motion, 0, 500);
			zeroing = true;
			zeroed = false;
			if (read_fault("ZEROING_STALL", action, sizeof(action)) && !strcmp(action, "stall")) {
				motion.duration = 3600000;
			}
		} else {
			serial_motion_sync(&motion, 0);
			zeroed = true;
		}
		ok_reply("ZEROING");
	} else if (!strcmp(command, "$BS SUMMARY")) {
		int position = (int)lround(serial_motion_update(&motion));
		double average = (temp0 + temp0_offset + temp1 + temp1_offset) / 2;
		char line[512];
		snprintf(line, sizeof(line), "$BS STATUS NAME:%s;POS:%d;STATE:%s;LIMIT:%d;FOCUS:%d;TEMP0:%.2f;TEMP1:%.2f;TEMP_AVG:%.2f;TCOMP:%d;PWM:%d", device_name, position, motion_state(), limit, focus, temp0 + temp0_offset, temp1 + temp1_offset, average, tcomp, pwm);
		if (!injected_reply("SUMMARY", line)) {
			send_line(line, false, false);
		}
	} else if (!strcmp(command, "$BS GET TEMP0")) {
		status_reply("TEMP0", "%.2f", temp0 + temp0_offset);
	} else if (!strcmp(command, "$BS GET TEMP1")) {
		status_reply("TEMP1", "%.2f", temp1 + temp1_offset);
	} else if (!strcmp(command, "$BS GET PID_CTRL")) {
		status_reply("PID_CTRL", "%d", pid_ctrl);
	} else if (set_integer(command, "$BS SET PID_CTRL:", 0, 1, &pid_ctrl)) {
		ok_reply("SET_PID_CTRL");
	} else if (!strcmp(command, "$BS GET PWM")) {
		status_reply("PWM", "%d", pwm);
	} else if (set_integer(command, "$BS SET PWM:", 0, 100, &pwm)) {
		pid_ctrl = 0;
		ok_reply("SET_PWM");
	} else if (!strcmp(command, "$BS GET AUTO_DEW")) {
		status_reply("AUTO_DEW", "%d", auto_dew);
	} else if (set_integer(command, "$BS SET AUTO_DEW:", 0, 1, &auto_dew)) {
		ok_reply("SET_AUTO_DEW");
	} else if (!strcmp(command, "$BS GET PID_SENSOR")) {
		status_reply("PID_SENSOR", "%d", pid_sensor);
	} else if (set_integer(command, "$BS SET PID_SENSOR:", 0, 2, &pid_sensor)) {
		ok_reply("SET_PID_SENSOR");
	} else if (!strcmp(command, "$BS GET PID_TARGET")) {
		status_reply("PID_TARGET", "%.2f", pid_target);
	} else if (set_float(command, "$BS SET PID_TARGET:", -50, 50, &pid_target)) {
		ok_reply("SET_PID_TARGET");
	} else if (!strcmp(command, "$BS GET PID_DEW_OFS")) {
		status_reply("PID_DEW_OFS", "%.2f", pid_dew_offset);
	} else if (set_float(command, "$BS SET PID_DEW_OFS:", -50, 50, &pid_dew_offset)) {
		ok_reply("SET_PID_DEW_OFS");
	} else if (!strcmp(command, "$BS GET AMBIENT_SENSOR")) {
		status_reply("AMBIENT_SENSOR", "%d", ambient_sensor);
	} else if (set_integer(command, "$BS SET AMBIENT_SENSOR:", 0, 1, &ambient_sensor)) {
		ok_reply("SET_AMBIENT_SENSOR");
	} else if (!strcmp(command, "$BS RESET")) {
		ok_reply("RESET");
		send_line("$BS DEBUG:FACTORY RESET...", false, false);
		send_line("$BS DEBUG: LOADING DEFAULTS...", false, false);
		use_crc = false;
		reset_controller();
		send_line("$BS Hello World!", false, false);
	} else if (!strcmp(command, "$BS REBOOT")) {
		use_crc = false;
		send_line("$BS Hello World!", false, false);
	} else {
		send_line("$BS ERROR: Unknown command!", false, false);
	}
}

static void process_line(char *line) {
	record_event("RX", line);
	send_echo(line);
	bool exception = !strncmp(line, "$BS RESET", 9) || !strncmp(line, "$BS REBOOT", 10) || !strncmp(line, "$BS CRC_DISABLE", 15);
	if (strncmp(line, "$BS", 3) || !validate_crc(line, use_crc && !exception)) {
		return;
	}
	dispatch(line);
}

static bool parse_args(int argc, char *argv[]) {
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
			fprintf(stderr, "Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|alternate|missing_sensor|stall]\n", argv[0]);
			return false;
		}
	}
	return !strcmp(profile, "normal") || !strcmp(profile, "split") || !strcmp(profile, "alternate") || !strcmp(profile, "missing_sensor") || !strcmp(profile, "stall");
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

int main(int argc, char *argv[]) {
	if (!parse_args(argc, argv)) {
		return 1;
	}
	if (!strcmp(profile, "alternate")) {
		limit = 5000;
		focus = 777;
		jogsteps = 80;
		singlesteps = 4;
		backlight = 75;
		temp0_offset = -0.25;
		temp1_offset = 0.5;
		tcomp = 1;
		tcomp_factor = -3.25;
		tcomp_period = 2500;
		tcomp_delta = 0.75;
		tcomp_sensor = 1;
		use_endstop = 1;
		pwm = 35;
		pid_ctrl = 1;
		pid_target = 5.5;
		pid_sensor = 2;
		ambient_sensor = 0;
		pid_dew_offset = 2.25;
		auto_dew = 1;
		snprintf(device_name, sizeof(device_name), "SD2_ALT");
	}
	if (!strcmp(profile, "missing_sensor")) {
		temp0 = temp1 = -128;
	}
	serial_motion_sync(&motion, !strcmp(profile, "alternate") ? 1500 : 1000);
	fault_file = getenv("INDIGO_STEELDRIVE2_FAULT");
	const char *events_path = getenv("INDIGO_STEELDRIVE2_EVENTS");
	events = events_path == NULL ? NULL : fopen(events_path, "w");
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	char port[256];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file != NULL && !serial_simulator_write_ready_file(ready_file, "focuser_steeldrive2", port))) {
		return 1;
	}
	if (!headless) {
		printf("SteelDriveII simulator ready on %s\n", port);
		fflush(stdout);
	}
	char line[600] = { 0 };
	size_t used = 0;
	bool boot = true;
	int idle_ticks = 0;
	while (running) {
		fd_set descriptors;
		FD_ZERO(&descriptors);
		FD_SET(serial_fd, &descriptors);
		struct timeval timeout = { 0, 10000 };
		int selected = select(serial_fd + 1, &descriptors, NULL, NULL, &timeout);
		if (selected <= 0) {
			if (selected < 0 && errno != EINTR) {
				break;
			}
			if (boot && ++idle_ticks >= 10) {
				send_line("$BS Hello World!", false, false);
				idle_ticks = 0;
			}
			continue;
		}
		char buffer[128];
		ssize_t count = read(serial_fd, buffer, sizeof(buffer));
		if (count < 0) {
			if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EIO) {
				break;
			}
			continue;
		}
		for (ssize_t i = 0; i < count; i++) {
			char value = buffer[i];
			if (value == '\r') {
				continue;
			}
			if (value == '\n') {
				if (used > 0) {
					line[used] = 0;
					boot = false;
					process_line(line);
					used = 0;
				}
			} else if (used < sizeof(line) - 1) {
				line[used++] = value;
			} else {
				used = 0;
			}
		}
	}
	if (events != NULL) {
		fclose(events);
	}
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
