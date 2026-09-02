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

static int focuser_position = 18075;
static int focuser_target = 18075;
static int rotator_position = 0;
static int rotator_target = 0;
static int backlash = 0;
static int speed = 0;
static int hold_current = 1;
static const char *wifi_status = "on";
static const char *led_status = "on";

static void usage(const char *name) {
	printf("PrimaLuceLab SestoSenso/Esatto/Arco simulator\n");
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

static bool sim_printf(int handle, const char *format, ...) {
	char buffer[4096];
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

static void complete_motion(void) {
	focuser_position = focuser_target;
	rotator_position = rotator_target;
}

static void send_state(int handle) {
	complete_motion();
	sim_printf(handle,
		"{\"res\":{\"get\":{"
		"\"MODNAME\":\"SESTOSENSO2\",\"SN\":\"SESTOSENSO20716\","
		"\"SWVERS\":{\"SWAPP\":\"3.10\",\"SWWEB\":\"3.10\"},"
		"\"WIFIAP\":{\"SSID\":\"SESTOSENSO20716\",\"PWD\":\"primalucelab\",\"STATUS\":\"%s\"},"
		"\"WIFISTA\":{\"SSID\":\"MySSID\",\"PWD\":\"MyPassword\"},"
		"\"EXT_T\":\"22.50\",\"VIN_12V\":\"13.98\",\"VIN_USB\":\"5.20\",\"DIMLEDS\":\"%s\",\"ARCO\":1,\"CALRESTART\":{\"MOT1\":0,\"MOT2\":0},"
		"\"MOT1\":{\"ABS_POS\":%d,\"ABS_POS_STEP\":%d,\"SPEED\":%d,\"BKLASH\":%d,"
		"\"STATUS\":{\"MST\":\"stop\"},\"NTC_T\":\"37.12\",\"ERROR\":\"\",\"CALRESTART\":0,"
		"\"FnRUN_ACC\":1,\"FnRUN_DEC\":1,\"FnRUN_SPD\":2,\"FnRUN_CURR_ACC\":7,\"FnRUN_CURR_DEC\":7,\"FnRUN_CURR_SPD\":7,\"FnRUN_CURR_HOLD\":3,"
		"\"HOLDCURR_STATUS\":%d},"
		"\"RUNPRESET_L\":{\"M1ACC\":10},\"RUNPRESET_M\":{\"M1SPD\":6},\"RUNPRESET_S\":{\"M1DEC\":1},"
		"\"RUNPRESET_1\":{\"M1HOLD\":3},\"RUNPRESET_2\":{\"M1CSPD\":5},\"RUNPRESET_3\":{\"M1CDEC\":7},"
		"\"MOT2\":{\"ABS_POS\":%d,\"ABS_POS_DEG\":%d,\"STATUS\":{\"MST\":\"stop\"},\"ERROR\":\"\",\"CALRESTART\":0,\"CAL_STATUS\":\"stop\"}"
		"}}}\n",
		wifi_status, led_status, focuser_position, focuser_position, speed, backlash, hold_current, rotator_position, rotator_position);
}

static void dispatch_command(int handle, const char *command) {
	if (strstr(command, "\"MODNAME\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"get\":{\"MODNAME\":\"SESTOSENSO2\"}}}\n");
	} else if (strstr(command, "\"SWVERS\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"get\":{\"SWVERS\":{\"SWAPP\":\"3.10\",\"SWWEB\":\"3.10\"}}}}\n");
	} else if (strstr(command, "\"get\"") != NULL) {
		send_state(handle);
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
	} else if (strstr(command, "STA_SET_STATUS") != NULL) {
		wifi_status = "sta";
		sim_printf(handle, "{\"res\":{\"cmd\":{\"STA_SET_STATUS\":\"done\"}}}\n");
	} else if (strstr(command, "DIMLEDS") != NULL) {
		led_status = strstr(command, "\"low\"") != NULL ? "low" : (strstr(command, "\"off\"") != NULL ? "off" : "on");
		sim_printf(handle, "{\"res\":{\"cmd\":{\"DIMLEDS\":\"done\"}}}\n");
	} else if (strstr(command, "RUNPRESET") != NULL) {
		sim_printf(handle, "{\"res\":{\"cmd\":{\"RUNPRESET\":\"done\"},\"get\":{\"MOT1\":{\"FnRUN_ACC\":1,\"FnRUN_DEC\":1,\"FnRUN_SPD\":2,\"FnRUN_CURR_ACC\":7,\"FnRUN_CURR_DEC\":7,\"FnRUN_CURR_SPD\":7,\"FnRUN_CURR_HOLD\":3,\"HOLDCURR_STATUS\":%d}}}}\n", hold_current);
	} else if (strstr(command, "\"MOVE_ABS\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		focuser_target = extract_int_after(command, "\"STEP\":", focuser_target);
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"STEP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"GOTO\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		focuser_target = extract_int_after(command, "\"GOTO\":", focuser_target);
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"GOTO\":\"done\"}}}}\n");
	} else if (strstr(command, "\"MOT_STOP\"") != NULL && strstr(command, "\"MOT1\"") != NULL) {
		focuser_target = focuser_position;
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"ARCO\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"set\":{\"ARCO\":\"done\"}}}\n");
	} else if (strstr(command, "\"MOVE_ABS\"") != NULL && strstr(command, "\"MOT2\"") != NULL) {
		rotator_target = extract_int_after(command, "\"DEG\":", rotator_target);
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT2\":{\"STEP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"MOT_STOP\"") != NULL && strstr(command, "\"MOT2\"") != NULL) {
		rotator_target = rotator_position;
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT2\":{\"MOT_STOP\":\"done\"}}}}\n");
	} else if (strstr(command, "\"CAL_STATUS\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT2\":{\"CAL_STATUS\":\"done\"}}}}\n");
	} else if (strstr(command, "\"CAL_FOCUSER\"") != NULL || strstr(command, "\"CAL_DIR\"") != NULL) {
		sim_printf(handle, "{\"res\":{\"cmd\":{\"MOT1\":{\"CAL_FOCUSER\":\"done\"}},\"set\":{\"MOT1\":{\"CAL_DIR\":\"done\"}}}}\n");
	} else {
		sim_printf(handle, "\"Error: invalid cmd\"\n");
	}
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[4096];

	if (!parse_args(argc, argv)) {
		return 1;
	}

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
		if (bytes > 0) {
			dispatch_command(serial_fd, buffer);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
