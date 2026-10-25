// PrimaLuceLab ALTO cover simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

// Models an ALTO1 with application firmware 1.03.11 (indigo_drivers/aux_alto/REFACTOR.md): MOT1 POSITION 0 (closed)
// to 100 (open) over CAL_MAXPOS steps, every member of a get, set or cmd answered in its own field, and no reply to
// malformed input. A full stroke takes 5 s instead of the unit's 34 s. The calibration follows the INDI driver and is
// not verified on hardware.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>

#define JSMN_STATIC
#include "../../shared/primalucelib/jsmn.h"

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

#define PRESET_COUNT     3
#define MAX_TOKENS       256
#define REPLY_SIZE       4096
#define INVALID          "Error: invalid command"
#define NOT_FOUND        "ERROR: Obj's name not found or invalid command"
#define PARAMETER_ERROR  "Error: parameter not found"
// Steps per second; a decelerated stop runs this far on.
#define SPEED            48000.0
#define SLOW_SPEED       12000.0
#define STOP_DISTANCE    2000.0

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
static FILE *events = NULL;

static const char *model = "ALTO1";
static const char *firmware = "1.03.11";
static const char *web_firmware = "1.01";
// The long-firmware profile reports 300 character application and web versions, together longer than an INDIGO
// text item.
static char long_firmware[2][301];
static int calibration_max = 239608;
static serial_motion motion = { .position = 239608, .target = 239608 };
// A stalled arm keeps reporting motion while the position does not change.
static bool stalled = false;
static int calibration_restart = 0;
static const char *motor_error = "";
static bool calibrating = false;
static char leds[16] = "on";
static struct {
	char name[33];
	int position;
} presets[PRESET_COUNT];

static void usage(const char *name) {
	printf("PrimaLuceLab ALTO simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --profile <name>        normal, closed, needs-calibration, uncalibrated, unsupported or long-firmware, default is normal\n");
	printf("  -h, --help              Show this help and exit\n");
	printf("\n");
	printf("INDIGO_ALTO_EVENTS names a file receiving every accepted request.\n");
	printf("INDIGO_ALTO_FAULT names a file holding '<key> <silent|garbage|reject|close|stall>', applied once to the next\n");
	printf("request containing that key and then removed; an 'always_' prefix keeps it armed.\n");
	printf("'external <position> [slow]' moves the cover as its web page does; 'slow' delays the reply to that request by\n");
	printf("400 ms, so that a client request arrives while it is in flight.\n");
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

static void apply_profile(void) {
	if (!strcmp(options.profile, "closed")) {
		serial_motion_sync(&motion, 0);
	} else if (!strcmp(options.profile, "needs-calibration")) {
		calibration_restart = 1;
		motor_error = "CalRestart";
	} else if (!strcmp(options.profile, "uncalibrated")) {
		calibration_max = 0;
		calibration_restart = 1;
		serial_motion_sync(&motion, 0);
	} else if (!strcmp(options.profile, "unsupported")) {
		model = "GIOTTO";
	} else if (!strcmp(options.profile, "long-firmware")) {
		for (int i = 0; i < 2; i++) {
			memset(long_firmware[i], 'x', 300);
			memcpy(long_firmware[i], i ? "1.01" : "1.03.11", i ? 4 : 7);
			long_firmware[i][300] = 0;
		}
		firmware = long_firmware[0];
		web_firmware = long_firmware[1];
	}
}

static int steps(void) {
	return (int)lround(serial_motion_update(&motion));
}

static int position(void) {
	return calibration_max > 0 ? (int)lround(100.0 * steps() / calibration_max) : 0;
}

static bool moving(void) {
	serial_motion_update(&motion);
	return motion.duration > 0 || stalled;
}

static const char *motor_state(void) {
	if (!moving()) {
		return "stop";
	}
	if (stalled) {
		return "CstSpeed";
	}
	double done = (serial_motion_time() - motion.started) / motion.duration;
	return done < 0.1 ? "acc" : done > 0.9 ? "dec" : "CstSpeed";
}

static void move_to_steps(double target, double speed) {
	stalled = false;
	serial_motion_start(&motion, target, speed);
}

// One-shot fault injection, as in the GIOTTO simulator. 'external' applies at once.
static const char *pending_fault(const char *request) {
	static char action[64];
	char key[128];
	const char *path = getenv("INDIGO_ALTO_FAULT");
	if (path == NULL) {
		return NULL;
	}
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	char delay[16] = "";
	bool parsed = fscanf(file, "%127s %63s %15s", key, action, delay) >= 2;
	fclose(file);
	if (parsed && !strcmp(key, "external")) {
		int target = atoi(action);
		if (calibration_max > 0 && target >= 0 && target <= 100) {
			move_to_steps(target * (double)calibration_max / 100.0, SPEED);
		}
		unlink(path);
		return strcmp(delay, "slow") ? NULL : "slow";
	}
	if (!parsed || strstr(request, key) == NULL) {
		return NULL;
	}
	if (!strncmp(action, "always_", 7)) {
		memmove(action, action + 7, strlen(action + 7) + 1);
	} else {
		unlink(path);
	}
	return action;
}

static bool reply(const char *text) {
	if (options.trace) {
		fprintf(stderr, "<- %s", text);
	}
	return serial_simulator_write_all(serial_fd, text, strlen(text));
}

static int read_json_command(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;
	int depth = 0;
	bool in_string = false, escaped = false, in_frame = false;
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
		buffer[total_bytes++] = c;
		if (escaped) {
			escaped = false;
		} else if (c == '\\' && in_string) {
			escaped = true;
		} else if (c == '"') {
			in_string = !in_string;
		} else if (!in_string && c == '{') {
			depth++;
		} else if (!in_string && c == '}') {
			if (--depth == 0) {
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

static bool token_is(const char *json, const jsmntok_t *token, const char *text) {
	return (token->type == JSMN_STRING || token->type == JSMN_PRIMITIVE) && (int)strlen(text) == token->end - token->start && !strncmp(json + token->start, text, token->end - token->start);
}

static int token_int(const char *json, const jsmntok_t *token, bool *ok) {
	char text[32];
	int length = token->end - token->start;
	*ok = token->type == JSMN_PRIMITIVE && length > 0 && length < (int)sizeof(text);
	if (!*ok) {
		return 0;
	}
	memcpy(text, json + token->start, length);
	text[length] = 0;
	char *end;
	long value = strtol(text, &end, 10);
	*ok = *end == 0;
	return (int)value;
}

// The index of the token after the value starting at index (values may be nested).
static int skip(const jsmntok_t *tokens, int index) {
	int end = tokens[index].end;
	index++;
	while (tokens[index].start != 0 && tokens[index].start < end) {
		index++;
	}
	return index;
}

static int preset_index(const char *json, const jsmntok_t *token) {
	int length = token->end - token->start;
	if (token->type != JSMN_STRING || length != 8 || strncmp(json + token->start, "PRESET_", 7)) {
		return -1;
	}
	int index = json[token->start + 7] - '1';
	return index >= 0 && index < PRESET_COUNT ? index : -1;
}

static void append(char *buffer, const char *format, ...) {
	size_t used = strlen(buffer);
	va_list args;
	va_start(args, format);
	vsnprintf(buffer + used, REPLY_SIZE - used, format, args);
	va_end(args);
}

// WiFi as the hardware showed it (ALTO1 firmware 1.03.11, GIOTTO firmware 1.03.09): every LANCFG and credential is
// acknowledged and stored as sent, except the access point SSID in access point mode, which is kept; LANCFG takes effect at the next REBOOT, the access point is off in station mode, and a restarting
// controller answers nothing until it has booted.
#define BOOT_TIME    1.0
#define CREDENTIAL   65
static char lan_config[16] = "ap";
static bool station = false;
static bool access_point = true;
static char ap_ssid[CREDENTIAL] = "ALTO100001", ap_password[CREDENTIAL] = "primalucelab";
static char sta_ssid[CREDENTIAL] = "MySSID", sta_password[CREDENTIAL] = "MyPassword";
static double boot_end = 0;

static double monotonic_seconds(void) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + now.tv_nsec / 1e9;
}

static void copy_token(char *target, size_t size, const char *json, const jsmntok_t *token) {
	int length = token->end - token->start;
	snprintf(target, size, "%.*s", length < (int)size - 1 ? length : (int)size - 1, json + token->start);
}

static void append_lan_config(char *buffer) {
	append(buffer, "\"LANCFG\":\"%s\"", lan_config);
}

static void append_access_point(char *buffer) {
	append(buffer, "\"WIFIAP\":{\"SSID\":\"%s\",\"PWD\":\"%s\",\"IP\":\"192.168.4.1\",\"NM\":\"255.255.255.0\",\"STATUS\":\"%s\",\"CLIENTS\":0}", ap_ssid, ap_password, access_point && !station ? "on" : "off");
}

// In station mode the controller has joined the network and got an address (a documentation address here).
static void append_station(char *buffer) {
	append(buffer, "\"WIFISTA\":{\"SSID\":\"%s\",\"PWD\":\"%s\",\"IP\":\"%s\"}", sta_ssid, sta_password, station ? "192.0.2.23" : "");
}

static void append_lan_status(char *buffer) {
	append(buffer, "\"LANSTATUS\":{\"LANCONN\":\"%s\",\"PINGGW\":0,\"PINGEXTHOST\":0}", station ? "gotip" : "disconnected");
}

// Each member of WIFIAP or WIFISTA is answered on its own.
static void set_credentials(char *buffer, const char *json, const jsmntok_t *tokens, int key, const char *refused) {
	const jsmntok_t *token = tokens + key;
	const jsmntok_t *value = tokens + key + 1;
	bool ap = token_is(json, token, "WIFIAP");
	append(buffer, "\"%.*s\":{", token->end - token->start, json + token->start);
	int member = key + 2;
	for (int i = 0; i < value->size; i++) {
		const jsmntok_t *field = tokens + member;
		const char *result = NOT_FOUND;
		if (token_is(json, field, "SSID") || token_is(json, field, "PWD")) {
			bool kept = ap && !station && token_is(json, field, "SSID");
			if (refused == NULL && !kept) {
				char *target = token_is(json, field, "SSID") ? (ap ? ap_ssid : sta_ssid) : (ap ? ap_password : sta_password);
				copy_token(target, CREDENTIAL, json, tokens + member + 1);
			}
			result = "done";
		}
		append(buffer, "%s\"%.*s\":\"%s\"", i ? "," : "", field->end - field->start, json + field->start, refused ? refused : result);
		member = skip(tokens, member + 1);
	}
	append(buffer, "}");
}

// REBOOT is acknowledged before the ESP32 boot log; the stored LANCFG then takes effect and the access point starts.
static void restart(char *buffer) {
	append(buffer, "{\"res\":{\"cmd\":{\"REBOOT\":\"done\"}}}\nets Jul 29 2019 12:21:46\nrst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\nentry 0x400806a4\n");
	station = !strcmp(lan_config, "sta");
	access_point = true;
	// The cover stops where it is; the position is kept.
	stalled = false;
	serial_motion_stop(&motion);
	boot_end = monotonic_seconds() + BOOT_TIME;
}

static void append_status(char *buffer) {
	append(buffer, "\"STATUS\":{\"HIZ\":0,\"UVLO\":0,\"TH_SD\":0,\"TH_WRN\":0,\"OCD\":0,\"WCMD\":0,\"NOPCMD\":0,\"BUSY\":%d,\"DIR\":\"%s\",\"MST\":\"%s\"}", moving() ? 1 : 0, motion.target < motion.origin ? "In" : "Out", motor_state());
}

static void append_motor(char *buffer) {
	append(buffer, "\"MOT1\":{\"SPEED\":0,\"TVAL_HOLD\":9,\"TVAL_RUN\":18,\"TVAL_ACC\":18,\"TVAL_DEC\":18,");
	append_status(buffer);
	append(buffer, ",\"SUBMODEL\":\"%s\",\"NTC_T\":30.80000,\"CAL_STATUS\":\"stop\",\"CAL_MAXPOS\":%d,\"TVAL_MIN\":3,\"TVAL_MAX\":20,\"CAL_MINPOS\":0,\"CAL_HOMEP\":1000,\"HOLDCURR_STATUS\":1,\"ABS_POS_STEP\":%d,\"POSITION_STEP\":%d,\"POSITION\":%d,\"ERROR\":\"%s\"}", model, calibration_max, steps(), steps(), position(), motor_error);
}

static void append_preset(char *buffer, int index) {
	append(buffer, "\"PRESET_%d\":{\"NAME\":\"%s\",\"M1POS\":%d}", index + 1, presets[index].name, presets[index].position);
}

static void full_state(char *buffer) {
	append(buffer, "{\"res\":{\"get\":{\"SWID\":\"003\",\"MODNAME\":\"%s\",\"SN\":\"ALTO100001\",\"HOSTNAME\":\"ALTO100001\",\"MACADDR\":\"8C:1F:64:00:00:02\",", model);
	append(buffer, "\"SWVERS\":{\"SWAPP\":\"%s\",\"SWWEB\":\"%s\"},\"VIN_12V\":\"13.37\",\"VIN_USB\":\"5.46\",\"CALRESTART\":{\"MOT1\":%d},\"LEDBLINKSTATUS\":\"\",", firmware, web_firmware, calibration_restart);
	append_lan_config(buffer);
	append(buffer, ",");
	append_access_point(buffer);
	append(buffer, ",");
	append_station(buffer);
	append(buffer, ",");
	append_lan_status(buffer);
	append(buffer, ",");
	for (int i = 0; i < PRESET_COUNT; i++) {
		append_preset(buffer, i);
		append(buffer, ",");
	}
	append(buffer, "\"DIMLED_1\":200,\"DIMLED_2\":200,\"DIMLED_3\":200,");
	append_motor(buffer);
	append(buffer, ",\"CUTOFF_TEMP\":67,\"NTC_T_CORR\":-5,\"DIMLEDS\":\"%s\",\"LOGLEVEL\":\"errors\"}}}\n", leds);
}

static void get_motor_member(char *buffer, const char *json, const jsmntok_t *token) {
	if (token_is(json, token, "POSITION")) {
		append(buffer, "\"POSITION\":%d", position());
	} else if (token_is(json, token, "ABS_POS_STEP")) {
		append(buffer, "\"ABS_POS_STEP\":%d", steps());
	} else if (token_is(json, token, "STATUS")) {
		append_status(buffer);
	} else if (token_is(json, token, "NTC_T")) {
		append(buffer, "\"NTC_T\":46.88000");
	} else if (token_is(json, token, "CAL_MAXPOS")) {
		append(buffer, "\"CAL_MAXPOS\":%d", calibration_max);
	} else if (token_is(json, token, "SPEED")) {
		append(buffer, "\"SPEED\":0");
	} else {
		append(buffer, "\"%.*s\":\"" PARAMETER_ERROR "\"", token->end - token->start, json + token->start);
	}
}

static void get_member(char *buffer, const char *json, const jsmntok_t *tokens, int key) {
	const jsmntok_t *token = tokens + key;
	const jsmntok_t *value = tokens + key + 1;
	int index = preset_index(json, token);
	if (index >= 0) {
		append_preset(buffer, index);
	} else if (token_is(json, token, "PRESETS")) {
		for (int i = 0; i < PRESET_COUNT; i++) {
			append_preset(buffer, i);
			if (i < PRESET_COUNT - 1) {
				append(buffer, ",");
			}
		}
	} else if (token_is(json, token, "MOT1") && value->type == JSMN_OBJECT) {
		append(buffer, "\"MOT1\":{");
		int member = key + 2;
		for (int i = 0; i < value->size; i++) {
			if (i) {
				append(buffer, ",");
			}
			get_motor_member(buffer, json, tokens + member);
			member = skip(tokens, member + 1);
		}
		append(buffer, "}");
	} else if (token_is(json, token, "MOT1")) {
		append_motor(buffer);
	} else if (token_is(json, token, "MODNAME")) {
		append(buffer, "\"MODNAME\":\"%s\"", model);
	} else if (token_is(json, token, "SWVERS")) {
		append(buffer, "\"SWVERS\":{\"SWAPP\":\"%s\"}", firmware);
	} else if (token_is(json, token, "VIN_12V")) {
		append(buffer, "\"VIN_12V\":\"13.39\"");
	} else if (token_is(json, token, "VIN_USB")) {
		append(buffer, "\"VIN_USB\":\"5.47\"");
	} else if (token_is(json, token, "DIMLEDS")) {
		append(buffer, "\"DIMLEDS\":\"%s\"", leds);
	} else if (token_is(json, token, "CALRESTART")) {
		append(buffer, "\"CALRESTART\":{\"MOT1\":%d}", calibration_restart);
	} else if (token_is(json, token, "LANCFG")) {
		append_lan_config(buffer);
	} else if (token_is(json, token, "WIFIAP")) {
		append_access_point(buffer);
	} else if (token_is(json, token, "WIFISTA")) {
		append_station(buffer);
	} else if (token_is(json, token, "LANSTATUS")) {
		append_lan_status(buffer);
	} else {
		append(buffer, "\"%.*s\":\"" NOT_FOUND "\"", token->end - token->start, json + token->start);
	}
}

static void set_preset(char *buffer, const char *json, const jsmntok_t *tokens, int key, int index, const char *refused) {
	const jsmntok_t *value = tokens + key + 1;
	append(buffer, "\"PRESET_%d\":{", index + 1);
	int member = key + 2;
	for (int i = 0; i < value->size; i++) {
		const jsmntok_t *field = tokens + member;
		const char *result = INVALID;
		if (token_is(json, field, "NAME") && tokens[member + 1].type == JSMN_STRING && tokens[member + 1].end - tokens[member + 1].start < (int)sizeof(presets[0].name)) {
			snprintf(presets[index].name, sizeof(presets[index].name), "%.*s", tokens[member + 1].end - tokens[member + 1].start, json + tokens[member + 1].start);
			result = "done";
		} else if (token_is(json, field, "M1POS") && presets[index].name[0] != 0) {
			// A preset without a name refuses a position; NAME is applied first.
			bool ok;
			int steps = token_int(json, tokens + member + 1, &ok);
			if (ok && steps >= 0 && steps <= calibration_max) {
				presets[index].position = steps;
				result = "done";
			}
		}
		append(buffer, "%s\"%.*s\":\"%s\"", i ? "," : "", field->end - field->start, json + field->start, refused ? refused : result);
		member = skip(tokens, member + 1);
	}
	append(buffer, "}");
}

static void set_member(char *buffer, const char *json, const jsmntok_t *tokens, int key, const char *refused) {
	const jsmntok_t *token = tokens + key;
	const jsmntok_t *value = tokens + key + 1;
	int index = preset_index(json, token);
	if (index >= 0 && value->type == JSMN_OBJECT) {
		set_preset(buffer, json, tokens, key, index, refused);
	} else if ((token_is(json, token, "WIFIAP") || token_is(json, token, "WIFISTA")) && value->type == JSMN_OBJECT) {
		set_credentials(buffer, json, tokens, key, refused);
	} else if (token_is(json, token, "LANCFG") && value->type == JSMN_STRING) {
		// Any text is stored (ALTO1 firmware 1.03.11 took "xyz").
		if (refused == NULL) {
			copy_token(lan_config, sizeof(lan_config), json, value);
		}
		append(buffer, "\"LANCFG\":\"%s\"", refused ? refused : "done");
	} else if (token_is(json, token, "MOT1") && value->type == JSMN_OBJECT) {
		append(buffer, "\"MOT1\":{");
		int member = key + 2;
		for (int i = 0; i < value->size; i++) {
			const jsmntok_t *field = tokens + member;
			const char *result = PARAMETER_ERROR;
			if (token_is(json, field, "POSITION")) {
				// Any value is acknowledged (ALTO1 firmware 1.03.11); modelled as atof() clamped to 0..100.
				char text[32];
				int length = tokens[member + 1].end - tokens[member + 1].start;
				snprintf(text, sizeof(text), "%.*s", length < 31 ? length : 31, json + tokens[member + 1].start);
				double target = fmax(0, fmin(100, atof(text)));
				if (refused == NULL && calibration_max > 0) {
					move_to_steps((int)target * (double)calibration_max / 100.0, SPEED);
				}
				result = "done";
			} else if (token_is(json, field, "SPEED")) {
				// The current speed, acknowledged and ignored.
				result = "done";
			}
			append(buffer, "%s\"%.*s\":\"%s\"", i ? "," : "", field->end - field->start, json + field->start, refused ? refused : result);
			member = skip(tokens, member + 1);
		}
		append(buffer, "}");
	} else {
		append(buffer, "\"%.*s\":\"" NOT_FOUND "\"", token->end - token->start, json + token->start);
	}
}

static const char *calibration_step(const char *json, const jsmntok_t *value) {
	if (token_is(json, value, "Init")) {
		calibrating = true;
		return "done";
	}
	if (!calibrating) {
		return INVALID;
	}
	// The INDI sequence: drive to each end, stop, store; the stored positions set the travel.
	if (token_is(json, value, "Close_Fast") || token_is(json, value, "Close_Slow")) {
		move_to_steps(motion.position - 1000000, token_is(json, value, "Close_Fast") ? SPEED : SLOW_SPEED);
	} else if (token_is(json, value, "Open_Fast") || token_is(json, value, "Open_Slow")) {
		move_to_steps(motion.position + 1000000, token_is(json, value, "Open_Fast") ? SPEED : SLOW_SPEED);
	} else if (token_is(json, value, "StoreAsClosedPos")) {
		serial_motion_stop(&motion);
		serial_motion_sync(&motion, 0);
	} else if (token_is(json, value, "StoreAsMaxOpenPos")) {
		serial_motion_stop(&motion);
		calibration_max = steps();
		calibration_restart = 0;
		motor_error = "";
		calibrating = false;
	} else {
		return INVALID;
	}
	return "done";
}

static void command(char *buffer, const char *json, const jsmntok_t *tokens, const char *refused) {
	const jsmntok_t *key = tokens + 5;
	const jsmntok_t *value = tokens + 6;
	bool ok;
	if (token_is(json, key, "DIMLEDS") && value->type == JSMN_STRING) {
		snprintf(leds, sizeof(leds), "%.*s", value->end - value->start < 15 ? value->end - value->start : 15, json + value->start);
		append(buffer, refused ? "{\"res\":{\"cmd\":{\"DIMLEDS\":\"" INVALID "\"}}}\n" : "{\"res\":{\"cmd\":{\"cmd\":\"done\"}}}\n");
	} else if (token_is(json, key, "LOGLEVEL")) {
		reply("[mLogs_DefaultSetup] \n");
		append(buffer, "{\"res\":{\"cmd\":{\"LOGLEVEL\":\"done\"}}}\n");
	} else if (token_is(json, key, "PRESET")) {
		int index = token_int(json, value, &ok) - 1;
		const char *result = INVALID;
		if (ok && index >= 0 && index < PRESET_COUNT) {
			if (presets[index].name[0] == 0) {
				result = "Error: empty preset";
			} else {
				if (refused == NULL) {
					move_to_steps(presets[index].position, SPEED);
				}
				result = "done";
			}
		}
		append(buffer, "{\"res\":{\"cmd\":{\"PRESET\":\"%s\"}}}\n", refused ? refused : result);
	} else if (token_is(json, key, "MOT1") && value->type == JSMN_OBJECT && value->size == 1) {
		const jsmntok_t *verb = tokens + 7;
		const char *result = INVALID;
		if (token_is(json, verb, "MOT_STOP")) {
			// A decelerated stop runs on a little; a stalled arm stops where it is.
			if (refused == NULL && moving()) {
				if (stalled) {
					stalled = false;
				} else {
					double direction = motion.target < motion.position ? -1 : 1;
					double target = motion.position + direction * STOP_DISTANCE;
					if (fabs(target - motion.position) < fabs(motion.target - motion.position)) {
						serial_motion_start(&motion, target, SPEED);
					}
				}
			}
			result = "done";
		} else if (token_is(json, verb, "MOT_ABORT")) {
			if (refused == NULL) {
				stalled = false;
				serial_motion_stop(&motion);
			}
			result = "done";
		} else if (token_is(json, verb, "CAL_ALTO")) {
			result = refused ? refused : calibration_step(json, tokens + 8);
		}
		append(buffer, "{\"res\":{\"cmd\":{\"MOT1\":{\"%.*s\":\"%s\"}}}}\n", verb->end - verb->start, json + verb->start, refused ? refused : result);
	} else if (token_is(json, key, "AP_SET_STATUS") && value->type == JSMN_STRING) {
		if (refused == NULL) {
			access_point = token_is(json, value, "on");
		}
		append(buffer, "{\"res\":{\"cmd\":{\"AP_SET_STATUS\":\"%s\"}}}\n", refused ? refused : "done");
	} else if (token_is(json, key, "REBOOT")) {
		if (refused == NULL) {
			restart(buffer);
		} else {
			append(buffer, "{\"res\":{\"cmd\":{\"REBOOT\":\"%s\"}}}\n", refused);
		}
	} else {
		append(buffer, "{\"res\":{\"cmd\":{\"%.*s\":\"" INVALID "\"}}}\n", key->end - key->start, json + key->start);
	}
}

static void dispatch_command(const char *json) {
	jsmn_parser parser;
	jsmntok_t tokens[MAX_TOKENS] = { 0 };
	jsmn_init(&parser);
	int count = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS - 1);
	// Malformed input and anything but {"req":{<verb>:...}} get no reply.
	if (count < 4 || tokens[0].type != JSMN_OBJECT || !token_is(json, tokens + 1, "req") || tokens[2].type != JSMN_OBJECT) {
		return;
	}
	if (events != NULL) {
		fprintf(events, "%s\n", json);
		fflush(events);
	}
	// A booting controller answers nothing.
	if (monotonic_seconds() < boot_end) {
		return;
	}
	const char *fault = pending_fault(json);
	if (fault != NULL && !strcmp(fault, "slow")) {
		usleep(400000);
		fault = NULL;
	}
	if (fault != NULL && !strcmp(fault, "silent")) {
		return;
	}
	if (fault != NULL && !strcmp(fault, "close")) {
		running = 0;
		if (serial_fd >= 0) {
			close(serial_fd);
			serial_fd = -1;
		}
		return;
	}
	if (fault != NULL && !strcmp(fault, "garbage")) {
		reply("{\"res\":{\"get\":{\"MOT1\":\n");
		return;
	}
	const char *refused = fault != NULL && !strcmp(fault, "reject") ? INVALID : NULL;
	const jsmntok_t *verb = tokens + 3;
	const jsmntok_t *body = tokens + 4;
	char buffer[REPLY_SIZE] = "";
	if (token_is(json, verb, "get") && body->type == JSMN_STRING && body->end == body->start) {
		full_state(buffer);
		reply(buffer);
		return;
	}
	if (body->type != JSMN_OBJECT) {
		return;
	}
	if (token_is(json, verb, "cmd")) {
		command(buffer, json, tokens, refused);
		// The arm freezes while still reporting motion; the stall applies to the move just started.
		if (fault != NULL && !strcmp(fault, "stall")) {
			serial_motion_stop(&motion);
			stalled = true;
		}
		reply(buffer);
		return;
	}
	bool set = token_is(json, verb, "set");
	if (!set && !token_is(json, verb, "get")) {
		return;
	}
	append(buffer, "{\"res\":{\"%s\":{", set ? "set" : "get");
	int key = 5;
	for (int i = 0; i < body->size; i++) {
		if (i) {
			append(buffer, ",");
		}
		if (set) {
			set_member(buffer, json, tokens, key, refused);
		} else {
			get_member(buffer, json, tokens, key);
		}
		key = skip(tokens, key + 1);
	}
	append(buffer, "}}}\n");
	if (set && fault != NULL && !strcmp(fault, "stall")) {
		serial_motion_stop(&motion);
		stalled = true;
	}
	reply(buffer);
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[4096];
	if (!parse_args(argc, argv)) {
		return 1;
	}
	apply_profile();
	const char *journal = getenv("INDIGO_ALTO_EVENTS");
	events = journal == NULL ? NULL : fopen(journal, "w");
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}
	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "alto_simulator", port)) {
		close(serial_fd);
		return 1;
	}
	if (!options.headless) {
		printf("PrimaLuceLab ALTO simulator is listening on %s\n", port);
		fflush(stdout);
	}
	while (running) {
		int bytes = read_json_command(serial_fd, buffer, sizeof(buffer));
		if (bytes < 0) {
			break;
		}
		if (bytes > 0) {
			dispatch_command(buffer);
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
