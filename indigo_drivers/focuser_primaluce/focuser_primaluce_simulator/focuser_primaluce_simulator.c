// PrimaLuceLab SestoSenso/Esatto/Arco simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *profile;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.profile = "normal"
};

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static serial_motion focus_motion = { .position = 18075, .target = 18075 };
static serial_motion rotate_motion;
static int focuser_position = 18075;
static int focuser_target = 18075;
static int rotator_position = 0;
static int rotator_target = 0;
// POSITION_DEG is the synced angle, ABS_POS_DEG the mechanical one; a sync moves the offset between them.
static int rotator_offset = 0;
// Stored focuser positions PRESET_1 to PRESET_9; the controller refuses a zero position.
static int preset_positions[9];
static char preset_names[9][32];
static double rotator_calibration_end = 0;
static int backlash = 0;
static int speed = 0;
static int hold_current = 1;
static const char *wifi_status = "on";
static const char *lan_cfg = "ap";
static const char *led_status = "on";
static const char *model = "SESTOSENSO2";
static const char *firmware = "3.10";
static bool report_abs_pos = true;
static bool report_speed = true;
static int calibration_restart = 0;
static const char *motor_error = "";
// A unit without an external probe reports EXT_T as -127.00 (observed on a SESTO SENSO 2).
static const char *external_temperature = "22.50";
// A stalled motor keeps reporting MST "move" while the position does not change.
static bool focus_stalled = false;
// The next StoreAsMaxPos leaves the coordinate a SESTO SENSO 3 reports when it lost it (1000000).
static bool lose_position = false;
// The calibrated travel; a controller without one reports CAL_MAXPOS 0.
static int calibration_min = 0;
static int calibration_max = 100000;
static const char *calibration_direction = "normal";
// A SESTO SENSO 3 reports the maximum stored by StoreAsMaxPos only a moment after it acknowledges it.
static int calibration_max_stored = -1;
static double calibration_max_visible = 0;
// The SESTO SENSO 3 calibration run started by GoOutToFindMaxPos ends only with StopMotor or stop_calib.
static bool calibration_run = false;
// A SESTO SENSO 3 restarts after a CAL_DIR write; requests arriving meanwhile are answered late, after the restart.
static double restart_end = 0;
static char restart_queue[16][512];
static int restart_queued = 0;
static FILE *events = NULL;

static void usage(const char *name) {
	printf("PrimaLuceLab SestoSenso/Esatto/Arco simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --profile <name>        normal, esatto, sestosenso3, sestosenso3-uncalibrated,\n");
	printf("                          no-abs-pos, no-speed, unsupported, old-firmware,\n");
	printf("                          needs-calibration, external-motion or no-probe,\n");
	printf("                          default is normal\n");
	printf("  -h, --help              Show this help and exit\n");
	printf("\n");
	printf("INDIGO_PRIMALUCE_EVENTS names a file receiving every accepted request.\n");
	printf("INDIGO_PRIMALUCE_FAULT names a file holding '<key> <silent|garbage|reject|nopower|close|slow|stall|ignore|lostposition>' which is\n");
	printf("applied once to the next request containing that key and then removed; an 'always_' prefix keeps\n");
	printf("it armed. 'handmove <position>' moves the focuser as its hand keypad does.\n");
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
		} else if (!strcmp(argv[i], "--profile")) {
			if (++i == argc) {
				fprintf(stderr, "--profile requires a name\n");
				return false;
			}
			options.profile = argv[i];
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// The controller identity and capabilities a scenario needs are selected once
// at startup, so a connecting driver reads back exactly the profile under test.
static void apply_profile(void) {
	if (!strcmp(options.profile, "esatto")) {
		model = "ESATTO";
	} else if (!strcmp(options.profile, "sestosenso3")) {
		model = "SESTOSENSO3";
	} else if (!strcmp(options.profile, "sestosenso3-uncalibrated")) {
		// The state of a SESTO SENSO 3 as delivered (firmware 1.0.9).
		model = "SESTOSENSO3";
		calibration_max = 0;
		calibration_restart = 1;
		motor_error = "CalRestart; CalRestart_WrongMaxPos; ";
	} else if (!strcmp(options.profile, "unsupported")) {
		model = "ARCO";
	} else if (!strcmp(options.profile, "old-firmware")) {
		firmware = "3.00";
	} else if (!strcmp(options.profile, "no-abs-pos")) {
		report_abs_pos = false;
	} else if (!strcmp(options.profile, "no-speed")) {
		report_speed = false;
	} else if (!strcmp(options.profile, "needs-calibration")) {
		calibration_restart = 1;
		motor_error = "MOT1 needs attention";
	} else if (!strcmp(options.profile, "external-motion")) {
		serial_motion_start(&focus_motion, 20000, 500);
	} else if (!strcmp(options.profile, "no-probe")) {
		external_temperature = "-127.00";
	}
}

// One-shot fault injection. The control file names a fragment of a request and
// the way the next request containing it has to misbehave, so a test can fail
// exactly one transaction without disturbing the rest of the session.
static const char *pending_fault(const char *command) {
	static char action[32];
	const char *path = getenv("INDIGO_PRIMALUCE_FAULT");
	if (path == NULL) {
		return NULL;
	}
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	char key[64] = { 0 };
	action[0] = '\0';
	bool parsed = fscanf(file, "%63s %31s", key, action) == 2;
	fclose(file);
	if (parsed && !strcmp(key, "handmove")) {
		serial_motion_start(&focus_motion, atoi(action), 1000);
		focus_stalled = false;
		unlink(path);
		return NULL;
	}
	if (!parsed || strstr(command, key) == NULL) {
		return NULL;
	}
	if (!strncmp(action, "always_", 7)) {
		memmove(action, action + 7, strlen(action + 7) + 1);
	} else {
		unlink(path);
	}
	return action;
}

static bool sim_printf(int handle, const char *format, ...) {
	char buffer[8192];
	va_list args;

	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0) {
		return false;
	}
	if ((size_t)length >= sizeof(buffer)) {
		length = (int)sizeof(buffer) - 1;
	}
	if (options.trace) {
		fprintf(stderr, "<- %s", buffer);
	}
	return serial_simulator_write_all(handle, buffer, (size_t)length);
}

static int read_json_command(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;
	int depth = 0;
	bool in_string = false;
	bool escaped = false;
	bool in_frame = false;

	while (running && total_bytes < length - 1) {
		ssize_t bytes_read = read(handle, &c, 1);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				return 0;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return 0;
		}
		if (!in_frame) {
			if (c != '{') {
				continue;
			}
			in_frame = true;
		}
		if (c != '\r') {
			buffer[total_bytes++] = c;
		}
		if (escaped) {
			escaped = false;
		} else if (c == '\\' && in_string) {
			escaped = true;
		} else if (c == '"') {
			in_string = !in_string;
		} else if (!in_string && c == '{') {
			depth++;
		} else if (!in_string && c == '}') {
			depth--;
			if (depth == 0) {
				break;
			}
		}
	}
	buffer[total_bytes] = '\0';
	if (*buffer) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return total_bytes;
}

static int extract_int_after(const char *command, const char *needle, int fallback) {
	char *start = strstr(command, needle);
	if (start == NULL) {
		return fallback;
	}
	return atoi(start + strlen(needle));
}

static bool sestosenso3(void) {
	return !strcmp(model, "SESTOSENSO3");
}

static void update_motion(void) {
	focuser_position = (int)serial_motion_update(&focus_motion);
	rotator_position = (int)serial_motion_update(&rotate_motion);
	if (calibration_max_stored >= 0 && serial_motion_time() >= calibration_max_visible) {
		calibration_max = calibration_max_stored;
		calibration_max_stored = -1;
	}
}

static void send_state(int handle) {
	char mot1_abs_pos[64] = { 0 };
	char mot1_speed[32] = { 0 };
	update_motion();
	// A SestoSenso 3 reports ABS_POS, older firmware only ABS_POS_STEP, and a
	// model without an adjustable speed omits SPEED altogether.
	if (report_abs_pos) {
		snprintf(mot1_abs_pos, sizeof(mot1_abs_pos), "\"ABS_POS\":%d,", focuser_position);
	}
	if (report_speed) {
		snprintf(mot1_speed, sizeof(mot1_speed), "\"SPEED\":%d,", speed);
	}
	// A SESTO SENSO 3 reports the ARCO angle with five decimals and the rounding of its step conversion: a move to
	// 15 degrees reads back 14.99994 (firmware 1.0.9).
	char rotator_angle[32];
	if (sestosenso3() && rotator_position > 0) {
		snprintf(rotator_angle, sizeof(rotator_angle), "%.5f", rotator_position - 0.00006);
	} else {
		snprintf(rotator_angle, sizeof(rotator_angle), "%d", rotator_position);
	}
	char presets[1024] = { 0 };
	for (int i = 0; i < 9; i++) {
		size_t used = strlen(presets);
		snprintf(presets + used, sizeof(presets) - used, "\"PRESET_%d\":{\"NAME\":\"%s\",\"M1POS\":%d},", i + 1, preset_names[i], preset_positions[i]);
	}
	sim_printf(handle,
		"{\"res\":{\"get\":{%s"
		"\"MODNAME\":\"%s\",\"SN\":\"SESTOSENSO20716\","
		"\"SWVERS\":{\"SWAPP\":\"%s\",\"SWWEB\":\"3.10\"},\"LANCFG\":\"%s\","
		"\"WIFIAP\":{\"SSID\":\"SESTOSENSO20716\",\"PWD\":\"primalucelab\",\"STATUS\":\"%s\"},"
		"\"WIFISTA\":{\"SSID\":\"MySSID\",\"PWD\":\"MyPassword\"},"
		"\"EXT_T\":\"%s\",\"VIN_12V\":\"13.98\",\"VIN_USB\":\"5.20\",\"DIMLEDS\":\"%s\",\"ARCO\":1,\"CALRESTART\":{\"MOT1\":%d,\"MOT2\":%d},"
		"\"MOT1\":{%s\"ABS_POS_STEP\":%d,%s\"BKLASH\":%d,"
		"\"STATUS\":{\"MST\":\"%s\"},\"NTC_T\":\"37.12\",\"ERROR\":\"%s\",\"CALRESTART\":%d,\"CAL_MINPOS\":%d,\"CAL_MAXPOS\":%d,\"CAL_DIR\":\"%s\","
		"\"FnRUN_ACC\":1,\"FnRUN_DEC\":1,\"FnRUN_SPD\":2,\"FnRUN_CURR_ACC\":7,\"FnRUN_CURR_DEC\":7,\"FnRUN_CURR_SPD\":7,\"FnRUN_CURR_HOLD\":3,"
		"\"HOLDCURR_STATUS\":%d},"
		"\"RUNPRESET_L\":{\"M1ACC\":10},\"RUNPRESET_M\":{\"M1SPD\":6},\"RUNPRESET_S\":{\"M1DEC\":1},"
		"\"RUNPRESET_1\":{\"M1HOLD\":3},\"RUNPRESET_2\":{\"M1CSPD\":5},\"RUNPRESET_3\":{\"M1CDEC\":7},"
		"\"MOT2\":{\"ABS_POS\":%d,\"ABS_POS_DEG\":%d,\"POSITION_DEG\":%s,\"STATUS\":{\"MST\":\"%s\"},\"ERROR\":\"\",\"CALRESTART\":%d,\"CAL_STATUS\":\"%s\"}"
		"}}}\n",
		strncmp(model, "SESTOSENSO", 10) ? "" : presets, model, firmware, lan_cfg, wifi_status, external_temperature, led_status, calibration_restart, calibration_restart, mot1_abs_pos, focuser_position, mot1_speed, backlash, focus_motion.duration > 0 || focus_stalled ? "move" : "stop", motor_error, calibration_restart, calibration_min, calibration_max, calibration_direction, hold_current, rotator_position - rotator_offset, rotator_position - rotator_offset, rotator_angle, rotate_motion.duration > 0 ? "move" : "stop", calibration_restart, serial_motion_time() < rotator_calibration_end ? "exec" : "stop");
}

static void dispatch_command(int handle, const char *command) {
	update_motion();
	if (events != NULL) {
		fprintf(events, "%s\n", command);
		fflush(events);
	}
	const char *fault = pending_fault(command);
	if (fault != NULL && !strcmp(fault, "lostposition")) {
		lose_position = true;
		fault = NULL;
	}
	if (fault != NULL && !strcmp(fault, "slow")) {
		usleep(400000);
		update_motion();
		fault = NULL;
	}
	if (fault != NULL && !strcmp(fault, "stall") && strstr(command, "\"MOVE_ABS\"") != NULL) {
		focus_stalled = true;
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"STEP\":\"done\"}}}}\n");
		return;
	}
	// A move that is acknowledged and never runs, as a SESTO SENSO 3 with its motor bridges off (STATUS.HIZ) does.
	if (fault != NULL && !strcmp(fault, "ignore") && (strstr(command, "\"MOVE_ABS\"") != NULL || strstr(command, "\"GOTO\"") != NULL)) {
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"%s\":\"done\"}}}}\n", strstr(command, "\"GOTO\"") != NULL ? "GOTO" : "STEP");
		return;
	}
	if (fault != NULL) {
		if (!strcmp(fault, "close")) {
			running = 0;
			if (serial_fd >= 0) {
				close(serial_fd);
				serial_fd = -1;
			}
			return;
		}
		if (!strcmp(fault, "silent")) {
			return;
		}
		if (!strcmp(fault, "garbage")) {
			sim_printf(handle, "\"Error: invalid cmd\"\n");
			return;
		}
		// The way a SESTO SENSO 2 refuses to drive the motor without its 12 V supply.
		if (!strcmp(fault, "nopower")) {
			const char *verb = strstr(command, "\"GOTO\"") != NULL ? "GOTO" : (strstr(command, "\"MOT_STOP\"") != NULL ? "MOT_STOP" : "STEP");
			sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"%s\":\"12V_PowerSupply_Error\"}}}}\n", verb);
			return;
		}
		// The way a SESTO SENSO 2 answers a request its firmware does not know.
		if (!strcmp(fault, "reject")) {
			sim_printf(handle, "{\"res\":{\"cmd\":{\"ERROR\":\"SYS: Invalid command\"}}}\n");
			return;
		}
	}
	if (strstr(command, "\"MODNAME\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"get\":{\"MODNAME\":\"%s\"}}}\n", model);
	} else if (strstr(command, "\"SWVERS\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"get\":{\"SWVERS\":{\"SWAPP\":\"%s\",\"SWWEB\":\"3.10\"}}}}\n", firmware);
	} else if (strstr(command, "\"get\"") != NULL) {
		send_state(handle);
	} else if (strstr(command, "\"set\"") != NULL && strstr(command, "\"PRESET_") != NULL) {
		int index = extract_int_after(command, "\"PRESET_", 0) - 1;
		char reply[256] = "";
		if (index >= 0 && index < 9) {
			const char *name = strstr(command, "\"NAME\":\"");
			if (name != NULL) {
				name += 8;
				size_t length = strcspn(name, "\"");
				if (length >= sizeof(preset_names[index])) {
					length = sizeof(preset_names[index]) - 1;
				}
				memcpy(preset_names[index], name, length);
				preset_names[index][length] = 0;
				strcat(reply, "\"NAME\":\"done\"");
			}
			if (strstr(command, "\"M1POS\":") != NULL) {
				int position = extract_int_after(command, "\"M1POS\":", 0);
				if (*reply) {
					strcat(reply, ",");
				}
				if (position > 0) {
					preset_positions[index] = position;
					strcat(reply, "\"M1POS\":\"done\"");
				} else {
					strcat(reply, "\"M1POS\":\"Error: invalid command\"");
				}
			}
		}
		sim_printf(handle, "{\"res\":{\"set\":{\"PRESET_%d\":{%s}}}}\n", index + 1, reply);
	} else if (strstr(command, "\"BKLASH\"") != NULL) {
		backlash = extract_int_after(command, "\"BKLASH\":", backlash);
		sim_printf(handle, "{\"res\":{\"set\":{\"MOT1\":{\"BKLASH\":\"done\"}}}}\n");
	} else if (strstr(command, "\"SPEED\"") != NULL) {
		speed = extract_int_after(command, "\"SPEED\":", speed);
		sim_printf(handle, "{\"res\":{\"set\":{\"MOT1\":{\"SPEED\":\"done\"}}}}\n");
	} else if (strstr(command, "\"HOLDCURR_STATUS\"") != NULL) {
		hold_current = extract_int_after(command, "\"HOLDCURR_STATUS\":", hold_current) ? 1 : 0;
		sim_printf(handle, "{\"res\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":\"done\"}}}}\n");
	} else if (strstr(command, "\"WIFIAP\"") != NULL || strstr(command, "\"WIFISTA\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"set\":{\"WIFIAP\":{\"SSID\":\"done\",\"PWD\":\"done\"},\"WIFISTA\":{\"SSID\":\"done\",\"PWD\":\"done\"}}}}\n");
	} else if (strstr(command, "AP_SET_STATUS") != NULL) {
		wifi_status = strstr(command, "\"off\"") != NULL ? "off" : "on";
		sim_printf(handle, "{\"res\":{\"cmd\":{\"AP_SET_STATUS\":\"done\"}}}\n");
	} else if (strstr(command, "\"LANCFG\"") != NULL) {
		lan_cfg = strstr(command, "\"sta\"") != NULL ? "sta" : "ap";
		sim_printf(handle, "{\"res\":{\"set\":{\"LANCFG\":\"done\"}}}\n");
	} else if (strstr(command, "\"REBOOT\"") != NULL) {
		// A SESTO SENSO 2 acknowledges, then prints its ESP32 boot log while it restarts.
		sim_printf(handle, "{\"res\":{\"cmd\":{\"REBOOT\":\"done\"}}}\n");
		sim_printf(handle, "ets Jun  8 2016 00:22:57\n\nrst:0xc (SW_CPU_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)\nentry 0x400806a4\nUnable to find DS18B20\n________ shell start __\n");
	} else if (strstr(command, "DIMLEDS") != NULL) {
		led_status = strstr(command, "\"low\"") != NULL ? "low" : (strstr(command, "\"middle\"") != NULL ? "middle" : (strstr(command, "\"off\"") != NULL ? "off" : "on"));
		// A SESTO SENSO 3 acknowledges under the key "cmd" (firmware 1.0.9).
		sim_printf(handle, sestosenso3() ? "{\"res\":{\"cmd\":{\"cmd\":\"done\"}}}\n" : "{\"res\":{\"cmd\":{\"DIMLEDS\":\"done\"}}}\n");
	} else if (strstr(command, "RUNPRESET") != NULL) {
		sim_printf(handle, "{\"res\":{\"cmd\":{\"RUNPRESET\":\"done\"},\"get\":{\"MOT1\":{\"FnRUN_ACC\":1,\"FnRUN_DEC\":1,\"FnRUN_SPD\":2,\"FnRUN_CURR_ACC\":7,\"FnRUN_CURR_DEC\":7,\"FnRUN_CURR_SPD\":7,\"FnRUN_CURR_HOLD\":3,\"HOLDCURR_STATUS\":%d}}}}\n", hold_current);
	} else if (strstr(command, "\"MOVE_ABS\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		focuser_target = extract_int_after(command, "\"STEP\":", focuser_target);
		focus_stalled = false;
		serial_motion_start(&focus_motion, focuser_target, 1000);
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"STEP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"GOTO\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		// An uncalibrated SESTO SENSO 3 acknowledges GOTO and does not move.
		if (calibration_max > calibration_min) {
			focuser_target = extract_int_after(command, "\"GOTO\":", focuser_target);
			serial_motion_start(&focus_motion, focuser_target, 1000);
		}
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"GOTO\":\"done\"}}}}\n");
	} else if ((strstr(command, "\"MOT_ABORT\"") != NULL || strstr(command, "\"MOT_STOP\"") != NULL) && strstr(command, "\"MOT1\"") != NULL && calibration_run) {
		// The SESTO SENSO 3 calibration run ignores the ordinary stop.
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"%s\":\"done\"}}}}\n", strstr(command, "\"MOT_ABORT\"") != NULL ? "MOT_ABORT" : "MOT_STOP");
	} else if (strstr(command, "\"MOT_ABORT\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		serial_motion_stop(&focus_motion);
		focus_stalled = false;
		focuser_target = focuser_position;
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"MOT_ABORT\":\"done\"}}}}\n");
	} else if (strstr(command, "\"LOGLEVEL\"") != NULL) {
		// A SESTO SENSO 3 prints a log line of its own before the reply (firmware 1.0.9).
		if (sestosenso3()) {
			sim_printf(handle, "[mLogs_DefaultSetup] \n");
		}
		sim_printf(handle, "{\"res\":{\"cmd\":{\"LOGLEVEL\":\"done\"}}}\n");
	} else if (strstr(command, "\"MOT_STOP\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		serial_motion_stop(&focus_motion);
		focus_stalled = false;
		focuser_target = focuser_position;
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"ARCO\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"set\":{\"ARCO\":\"done\"}}}\n");
	} else if (strstr(command, "\"MOVE_ABS\"") != NULL && strstr(command, "\"MOT2\"") != NULL) {
		rotator_target = extract_int_after(command, "\"DEG\":", rotator_target);
		serial_motion_start(&rotate_motion, rotator_target, 90);
		// A SESTO SENSO 3 (firmware 1.0.9) acknowledges under DEG, a SESTO SENSO 2 under STEP.
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT2\":{\"%s\":\"done\"}}}}\n", sestosenso3() ? "DEG" : "STEP");
	} else if (strstr(command, "\"SYNC_POS\"") != NULL && strstr(command, "\"MOT2\"") != NULL) {
		int synced = extract_int_after(command, "\"DEG\":", rotator_position);
		rotator_offset += synced - rotator_position;
		serial_motion_sync(&rotate_motion, synced);
		rotator_position = rotator_target = synced;
		// A SESTO SENSO 3 (firmware 1.0.9) acknowledges under DEG and may print debug text in front of the reply.
		sim_printf(handle, sestosenso3() ? "mJsonAPI_ParseCmdReq_Motor{\"res\":{\"cmd\":{\"MOT2\":{\"DEG\":\"done\"}}}}\n" : "{\"res\":{\"cmd\":{\"MOT2\":{\"SYNC_POS\":\"done\"}}}}\n");
	} else if (strstr(command, "\"MOT_STOP\"") != NULL && strstr(command, "\"MOT2\"") != NULL) {
		serial_motion_stop(&rotate_motion);
		rotator_target = rotator_position;
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT2\":{\"MOT_STOP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"CAL_STATUS\"") != NULL) {
		// The ARCO calibration runs for a while after it is started, and "stop" ends it at once.
		rotator_calibration_end = strstr(command, "\"stop\"") != NULL ? 0 : serial_motion_time() + 2;
		sim_printf(handle, "{\"res\":{\"set\":{\"MOT2\":{\"CAL_STATUS\":\"done\"}}}}\n");
	} else if (strstr(command, "\"CAL_DIR\"") != NULL) {
		calibration_direction = strstr(command, "\"invert\"") != NULL ? "invert" : "normal";
		sim_printf(handle, "{\"res\":{\"set\":{\"MOT1\":{\"CAL_DIR\":\"done\"}}}}\n");
		// A SESTO SENSO 3 restarts after every CAL_DIR write (firmware 1.0.9, about 9.5 s on the unit, shortened here
		// but long enough to collect more late replies than the library drops per request).
		if (sestosenso3()) {
			serial_motion_stop(&focus_motion);
			calibration_run = false;
			sim_printf(handle, "ets Jul 29 2019 12:21:46\n\nrst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\n[mLogs_DefaultSetup] \n[mLogs_DefaultSetup] \nUnable to find DS18B20\n");
			restart_end = serial_motion_time() + 5;
		}
	} else if (strstr(command, "\"CAL_FOCUSER\"") != NULL && sestosenso3()) {
		// The SESTO SENSO 3 semi-automatic calibration: StoreAsMinPos zeroes the coordinate, the run ends with
		// StopMotor, StoreAsMaxPos stores the position and stop_calib leaves the focuser uncalibrated.
		if (strstr(command, "\"StoreAsMinPos\"") != NULL) {
			serial_motion_sync(&focus_motion, 0);
			focuser_position = focuser_target = 0;
		} else if (strstr(command, "\"GoOutToFindMaxPos\"") != NULL) {
			focuser_target = focuser_position + 1000000;
			serial_motion_start(&focus_motion, focuser_target, 1000);
			calibration_run = true;
		} else if (strstr(command, "\"StopMotor\"") != NULL) {
			serial_motion_stop(&focus_motion);
			focuser_target = focuser_position;
			calibration_run = false;
		} else if (strstr(command, "\"StoreAsMaxPos\"") != NULL) {
			calibration_max_stored = focuser_position;
			calibration_max_visible = serial_motion_time() + 0.6;
			calibration_restart = 0;
			motor_error = "";
			if (lose_position) {
				lose_position = false;
				serial_motion_sync(&focus_motion, 1000000);
				focuser_position = focuser_target = 1000000;
			}
		} else if (strstr(command, "\"stop_calib\"") != NULL) {
			serial_motion_stop(&focus_motion);
			focuser_target = focuser_position;
			calibration_run = false;
			calibration_max = 0;
			calibration_restart = 1;
		}
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"CAL_FOCUSER\":\"done\"}}}}\n");
	} else if (strstr(command, "\"CAL_FOCUSER\"") != NULL) {
		// GoOutToFindMaxPos runs the draw tube outward until StoreAsMaxPos stops it and stores the position.
		if (strstr(command, "\"GoOutToFindMaxPos\"") != NULL) {
			focuser_target = focuser_position + 1000000;
			serial_motion_start(&focus_motion, focuser_target, 1000);
		} else if (strstr(command, "\"StoreAsMaxPos\"") != NULL) {
			serial_motion_stop(&focus_motion);
			focuser_target = focuser_position;
			calibration_max = focuser_position;
		}
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"CAL_FOCUSER\":\"done\"}}}}\n");
	} else {
		sim_printf(handle, "\"Error: invalid cmd\"\n");
	}
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[8192];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	apply_profile();
	const char *journal = getenv("INDIGO_PRIMALUCE_EVENTS");
	events = journal == NULL ? NULL : fopen(journal, "w");

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_primaluce_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("PrimaLuceLab focuser/rotator simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = read_json_command(serial_fd, buffer, sizeof(buffer));
		if (bytes < 0) {
			break;
		}
		// Requests that reach a restarting SESTO SENSO 3 are answered together once it is up again.
		bool restarting = serial_motion_time() < restart_end;
		if (!restarting && restart_queued > 0) {
			for (int i = 0; i < restart_queued; i++) {
				dispatch_command(serial_fd, restart_queue[i]);
			}
			restart_queued = 0;
		}
		if (bytes > 0 && restarting) {
			if (restart_queued < 16) {
				snprintf(restart_queue[restart_queued++], sizeof(restart_queue[0]), "%s", buffer);
			}
		} else if (bytes > 0) {
			dispatch_command(serial_fd, buffer);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
	}
	if (events != NULL) {
		fclose(events);
	}
	return 0;
}
