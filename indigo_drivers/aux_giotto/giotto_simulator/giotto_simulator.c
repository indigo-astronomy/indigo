// PrimaLuceLab GIOTTO flat panel simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

// Models the GIOTTO protocol v2.1 and the behaviour of a GIOTTO with application firmware 1.03.09
// (indigo_drivers/aux_giotto/REFACTOR.md): every member of a get, set or cmd is answered in its own
// field, a refused member does not stop the others, and malformed input gets no reply.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>

#define JSMN_STATIC
#include "../../shared/primalucelib/jsmn.h"

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

#define PRESET_COUNT 9
#define MAX_TOKENS   256
#define REPLY_SIZE   4096
#define INVALID      "Error: invalid command"
#define NOT_FOUND    "Error: Obj's name not found or invalid command"

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

static const char *model = "GIOTTO";
static const char *firmware = "1.03.09";
static const char *web_firmware = "1.1";
// The long-firmware profile reports 300 character application and web versions, together longer than an INDIGO
// text item.
static char long_firmware[2][301];
static int max_brightness = 114;
static int brightness = 0;
static int light = 0;
static int fading = 1;
static int fading_time = 10;
static int light_at_power_on = 0;
static char leds[16] = "on";
static int calibration_restart[2] = { 0, 0 };
static struct {
	char name[33];
	int brightness;
} presets[PRESET_COUNT];

static void usage(const char *name) {
	printf("PrimaLuceLab GIOTTO simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --profile <name>        normal, needs-calibration, unsupported, no-range or long-firmware, default is normal\n");
	printf("  -h, --help              Show this help and exit\n");
	printf("\n");
	printf("INDIGO_GIOTTO_EVENTS names a file receiving every accepted request.\n");
	printf("INDIGO_GIOTTO_FAULT names a file holding '<key> <silent|garbage|reject|close>', applied once to the next\n");
	printf("request containing that key and then removed; an 'always_' prefix keeps it armed.\n");
	printf("'external <light> <brightness> [slow]' changes the panel as its web page does; 'slow' delays the reply to that\n");
	printf("request by 400 ms, so that a client request arrives while it is in flight.\n");
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
	if (!strcmp(options.profile, "needs-calibration")) {
		calibration_restart[0] = 1;
	} else if (!strcmp(options.profile, "unsupported")) {
		model = "ALTO";
	} else if (!strcmp(options.profile, "no-range")) {
		max_brightness = 0;
	} else if (!strcmp(options.profile, "long-firmware")) {
		for (int i = 0; i < 2; i++) {
			memset(long_firmware[i], 'x', 300);
			memcpy(long_firmware[i], i ? "1.1" : "1.03.09", i ? 3 : 7);
			long_firmware[i][300] = 0;
		}
		firmware = long_firmware[0];
		web_firmware = long_firmware[1];
	}
}

// One-shot fault injection, as in the PrimaLuce focuser simulator. 'external' applies at once.
static const char *pending_fault(const char *request) {
	static char action[64];
	char key[128];
	const char *path = getenv("INDIGO_GIOTTO_FAULT");
	if (path == NULL) {
		return NULL;
	}
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	int external_light, external_brightness;
	char delay[16] = "";
	bool parsed = fscanf(file, "%127s %63s", key, action) == 2;
	fclose(file);
	if (parsed && !strcmp(key, "external") && sscanf(action, "%d", &external_light) == 1) {
		file = fopen(path, "r");
		if (file != NULL && fscanf(file, "%*s %d %d %15s", &external_light, &external_brightness, delay) >= 2) {
			light = external_light == 1;
			brightness = external_brightness;
		}
		if (file != NULL) {
			fclose(file);
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

static void append_preset(char *buffer, int index) {
	append(buffer, "\"PRESET_%d\":{\"NAME\":\"%s\",\"BRIGHTNESS\":%d}", index + 1, presets[index].name, presets[index].brightness);
}

// WiFi as the hardware showed it (ALTO1 firmware 1.03.11, GIOTTO firmware 1.03.09): every LANCFG and credential is
// acknowledged and stored as sent, except the access point SSID in access point mode, which is kept; LANCFG takes effect at the next REBOOT, the access point is off in station mode, and a restarting
// controller answers nothing until it has booted.
#define BOOT_TIME    1.0
#define CREDENTIAL   65
static char lan_config[16] = "ap";
static bool station = false;
static bool access_point = true;
static char ap_ssid[CREDENTIAL] = "GIOTTO12000001", ap_password[CREDENTIAL] = "primalucelab";
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
	// The light comes back as LIGHT_ATPWON selects; the brightness is kept.
	light = light_at_power_on;
	boot_end = monotonic_seconds() + BOOT_TIME;
}

static const char *light_status(void) {
	return !light ? "off" : brightness == 0 ? "on" : "lighting";
}

static void full_state(char *buffer) {
	append(buffer, "{\"res\":{\"get\":{\"SWID\":\"002\",\"MODNAME\":\"%s\",\"SN\":\"GIOTTO12000001\",\"HOSTNAME\":\"GIOTTO12000001\",\"MACADDR\":\"8C:1F:64:00:00:01\",", model);
	append(buffer, "\"SWVERS\":{\"SWAPP\":\"%s\",\"SWWEB\":\"%s\"},\"NTC_T\":\"29.74\",\"VIN_12V\":\"13.13\",\"VIN_USB\":\"5.33\",\"MOTOR\":0,", firmware, web_firmware);
	append_lan_config(buffer);
	append(buffer, ",");
	append_access_point(buffer);
	append(buffer, ",");
	append_station(buffer);
	append(buffer, ",");
	append_lan_status(buffer);
	append(buffer, ",");
	append(buffer, "\"LED3_LEDBLINKSTATUS\":\"\",\"LEDBLINKSTATUS\":\"\",\"CALRESTART\":{\"CH1\":%d,\"CH2\":%d},\"NUM_STRIPLIGHTS\":1,", calibration_restart[0], calibration_restart[1]);
	if (max_brightness > 0) {
		append(buffer, "\"MAX_BRIGHTNESS\":%d,", max_brightness);
	}
	append(buffer, "\"BRIGHTNESS\":%d,\"LIGHT_ATPWON\":%d,\"LIGHT_FADING\":%d,\"LIGHT_FADING_TIME\":%d,\"LIGHT\":%d,\"LIGHT_STATUS\":\"%s\",", brightness, light_at_power_on, fading, fading_time, light, light_status());
	for (int i = 0; i < PRESET_COUNT; i++) {
		append_preset(buffer, i);
		append(buffer, ",");
	}
	append(buffer, "\"DIMLED_1\":200,\"DIMLED_2\":200,\"DIMLED_3\":200,\"CUTOFF_TEMP\":67,\"NTC_T_CORR\":-5,\"DIMLEDS\":\"%s\",\"LOGLEVEL\":\"errors\"}}}\n", leds);
}

static void get_member(char *buffer, const char *json, const jsmntok_t *tokens, int key) {
	const jsmntok_t *token = tokens + key;
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
	} else if (token_is(json, token, "MODNAME")) {
		append(buffer, "\"MODNAME\":\"%s\"", model);
	} else if (token_is(json, token, "SN")) {
		append(buffer, "\"SN\":\"GIOTTO12000001\"");
	} else if (token_is(json, token, "SWVERS")) {
		append(buffer, "\"SWVERS\":{\"SWAPP\":\"%s\"}", firmware);
	} else if (token_is(json, token, "LIGHT")) {
		append(buffer, "\"LIGHT\":%d", light);
	} else if (token_is(json, token, "BRIGHTNESS")) {
		append(buffer, "\"BRIGHTNESS\":%d", brightness);
	} else if (token_is(json, token, "MAX_BRIGHTNESS") && max_brightness > 0) {
		append(buffer, "\"MAX_BRIGHTNESS\":%d", max_brightness);
	} else if (token_is(json, token, "LIGHT_STATUS")) {
		append(buffer, "\"LIGHT_STATUS\":\"%s\"", light_status());
	} else if (token_is(json, token, "LIGHT_FADING")) {
		append(buffer, "\"LIGHT_FADING\":%d", fading);
	} else if (token_is(json, token, "LIGHT_FADING_TIME")) {
		append(buffer, "\"LIGHT_FADING_TIME\":%d", fading_time);
	} else if (token_is(json, token, "LIGHT_ATPWON")) {
		append(buffer, "\"LIGHT_ATPWON\":%d", light_at_power_on);
	} else if (token_is(json, token, "DIMLEDS")) {
		append(buffer, "\"DIMLEDS\":\"%s\"", leds);
	} else if (token_is(json, token, "NTC_T")) {
		append(buffer, "\"NTC_T\":\"31.96\"");
	} else if (token_is(json, token, "VIN_12V")) {
		append(buffer, "\"VIN_12V\":\"13.12\"");
	} else if (token_is(json, token, "VIN_USB")) {
		append(buffer, "\"VIN_USB\":\"5.33\"");
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

static const char *set_number(int *target, int value, bool ok, int min, int max) {
	if (!ok || value < min || value > max) {
		return INVALID;
	}
	*target = value;
	return "done";
}

static void set_member(char *buffer, const char *json, const jsmntok_t *tokens, int key, const char *refused) {
	const jsmntok_t *token = tokens + key;
	const jsmntok_t *value = tokens + key + 1;
	bool ok;
	int number = token_int(json, value, &ok);
	int index = preset_index(json, token);
	if (index >= 0 && value->type == JSMN_OBJECT) {
		// Each member of a preset is answered on its own.
		append(buffer, "\"PRESET_%d\":{", index + 1);
		int member = key + 2;
		for (int i = 0; i < value->size; i++) {
			const jsmntok_t *field = tokens + member;
			const char *result = INVALID;
			if (token_is(json, field, "NAME") && tokens[member + 1].type == JSMN_STRING && tokens[member + 1].end - tokens[member + 1].start < (int)sizeof(presets[0].name)) {
				snprintf(presets[index].name, sizeof(presets[index].name), "%.*s", tokens[member + 1].end - tokens[member + 1].start, json + tokens[member + 1].start);
				result = "done";
			} else if (token_is(json, field, "BRIGHTNESS") && presets[index].name[0] != 0) {
				// The panel refuses the brightness of a preset without a name; NAME is applied first.
				bool field_ok;
				int field_value = token_int(json, tokens + member + 1, &field_ok);
				result = set_number(&presets[index].brightness, field_value, field_ok, 0, max_brightness);
			}
			append(buffer, "%s\"%.*s\":\"%s\"", i ? "," : "", field->end - field->start, json + field->start, refused ? refused : result);
			member = skip(tokens, member + 1);
		}
		append(buffer, "}");
		return;
	}
	if ((token_is(json, token, "WIFIAP") || token_is(json, token, "WIFISTA")) && value->type == JSMN_OBJECT) {
		set_credentials(buffer, json, tokens, key, refused);
		return;
	}
	const char *result = NOT_FOUND;
	if (token_is(json, token, "LANCFG") && value->type == JSMN_STRING) {
		// Any text is stored (ALTO1 firmware 1.03.11 took "xyz").
		if (refused == NULL) {
			copy_token(lan_config, sizeof(lan_config), json, value);
		}
		result = "done";
	} else if (token_is(json, token, "LIGHT")) {
		// Any number is acknowledged; only 1 switches the light on.
		result = ok ? "done" : INVALID;
		if (ok) {
			light = number == 1;
		}
	} else if (token_is(json, token, "BRIGHTNESS")) {
		result = set_number(&brightness, number, ok, 0, max_brightness);
	} else if (token_is(json, token, "LIGHT_FADING")) {
		result = set_number(&fading, number, ok, 0, 1);
	} else if (token_is(json, token, "LIGHT_FADING_TIME")) {
		result = set_number(&fading_time, number, ok, 0, 100);
	} else if (token_is(json, token, "LIGHT_ATPWON")) {
		result = set_number(&light_at_power_on, number, ok, 0, 1);
	}
	append(buffer, "\"%.*s\":\"%s\"", token->end - token->start, json + token->start, refused ? refused : result);
}

static void dispatch_command(const char *json) {
	jsmn_parser parser;
	jsmntok_t tokens[MAX_TOKENS] = { 0 };
	jsmn_init(&parser);
	int count = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS - 1);
	// Malformed input and anything but {"req":{<verb>:...}} get no reply, as on the panel.
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
		// The USB cable is pulled: the port disappears and nothing answers again.
		running = 0;
		if (serial_fd >= 0) {
			close(serial_fd);
			serial_fd = -1;
		}
		return;
	}
	if (fault != NULL && !strcmp(fault, "garbage")) {
		reply("{\"res\":{\"get\":{\"LIGHT\":\n");
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
		const jsmntok_t *key = tokens + 5;
		const jsmntok_t *value = tokens + 6;
		bool ok;
		if (token_is(json, key, "DIMLEDS") && value->type == JSMN_STRING) {
			// The panel acknowledges DIMLEDS under the key "cmd" (firmware 1.03.09).
			snprintf(leds, sizeof(leds), "%.*s", value->end - value->start < 15 ? value->end - value->start : 15, json + value->start);
			reply(refused ? "{\"res\":{\"cmd\":{\"DIMLEDS\":\"" INVALID "\"}}}\n" : "{\"res\":{\"cmd\":{\"cmd\":\"done\"}}}\n");
		} else if (token_is(json, key, "LOGLEVEL")) {
			reply("[mLogs_DefaultSetup] \n");
			reply("{\"res\":{\"cmd\":{\"LOGLEVEL\":\"done\"}}}\n");
		} else if (token_is(json, key, "PRESET")) {
			int index = token_int(json, value, &ok) - 1;
			const char *result = INVALID;
			if (ok && index >= 0 && index < PRESET_COUNT) {
				if (presets[index].name[0] == 0) {
					result = "Error: empty preset";
				} else {
					// A preset sets the brightness only; the light keeps its state.
					brightness = presets[index].brightness;
					result = "done";
				}
			}
			append(buffer, "{\"res\":{\"cmd\":{\"PRESET\":\"%s\"}}}\n", refused ? refused : result);
			reply(buffer);
		} else if (token_is(json, key, "AP_SET_STATUS") && value->type == JSMN_STRING) {
			if (refused == NULL) {
				access_point = token_is(json, value, "on");
			}
			append(buffer, "{\"res\":{\"cmd\":{\"AP_SET_STATUS\":\"%s\"}}}\n", refused ? refused : "done");
			reply(buffer);
		} else if (token_is(json, key, "REBOOT")) {
			if (refused == NULL) {
				restart(buffer);
			} else {
				append(buffer, "{\"res\":{\"cmd\":{\"REBOOT\":\"%s\"}}}\n", refused);
			}
			reply(buffer);
		} else {
			append(buffer, "{\"res\":{\"cmd\":{\"%.*s\":\"" INVALID "\"}}}\n", key->end - key->start, json + key->start);
			reply(buffer);
		}
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
	reply(buffer);
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[4096];
	if (!parse_args(argc, argv)) {
		return 1;
	}
	apply_profile();
	const char *journal = getenv("INDIGO_GIOTTO_EVENTS");
	events = journal == NULL ? NULL : fopen(journal, "w");
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}
	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "giotto_simulator", port)) {
		close(serial_fd);
		return 1;
	}
	if (!options.headless) {
		printf("PrimaLuceLab GIOTTO simulator is listening on %s\n", port);
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
