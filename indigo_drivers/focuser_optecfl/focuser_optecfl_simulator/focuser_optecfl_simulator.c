// Optec FocusLynx focuser simulator
//
// Copyright (c) 2024-2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Protocol model follows FocusLynx_Command_Processing_rev3.pdf.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <ctype.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

// ----------------------------------------------------------------- options

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

static const char *simulator_name = "focuser_optecfl";
static const char *profile = "normal";

static void usage(const char *name) {
	printf("Optec FocusLynx focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <focuslynx>     Select simulated model, default is focuslynx\n");
	printf("  --profile <name>        normal, split, nohome, noprobe or syncable, default is normal\n");
	printf("  -h, --help              Show this help and exit\n");
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
		} else if (!strcmp(argv[i], "--profile") && i + 1 < argc) {
			profile = argv[++i];
			if (strcmp(profile, "normal") && strcmp(profile, "split") && strcmp(profile, "nohome") && strcmp(profile, "noprobe") && strcmp(profile, "syncable")) {
				fprintf(stderr, "Unknown profile '%s'\n", profile);
				return false;
			}
		} else if (!strcmp(argv[i], "--ready-file")) {
			if (++i == argc) {
				fprintf(stderr, "--ready-file requires a path\n");
				return false;
			}
			options.ready_file = argv[i];
		} else if (!strcmp(argv[i], "--model")) {
			if (++i == argc) {
				fprintf(stderr, "--model requires focuslynx\n");
				return false;
			}
			if (strcmp(argv[i], "focuslynx")) {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- state

#define COMMAND_LENGTH 128
#define MOTION_SPEED 2000.0

typedef struct {
	serial_motion motion;
	int max_position;
	char nickname[17];
	char type[3];
	double temperature;
	bool tcomp_on;
	bool tcomp_at_start;
	bool tcomp_suspended;
	bool backlash_on;
	int backlash_steps;
	int coefficient[5];
	char mode;
	bool homing;
	bool homed;
	bool ff_detect;
	bool temperature_probe;
	bool remote_io;
	bool hand_controller;
} focuser_state;

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static FILE *events = NULL;
static focuser_state focusers[2];
static int led_brightness = 75;
static bool sleeping = false;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// Appendix A device types. Optec focusers must home and therefore reject SCCP.
static const char *device_types[] = { "OA", "OB", "OC", "OD", "OE", "OF", "OG", "FA", "FB", "FC", "SA", "SB", "SC", "SD", "SE", "SF", "SG", "SH", "SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SP", "SQ", "TA", "ZZ", NULL };

static bool valid_device_type(const char *type) {
	for (int i = 0; device_types[i]; i++) {
		if (!strncmp(device_types[i], type, 2)) {
			return true;
		}
	}
	return false;
}

// Max Pos is derived from the device type by the controller.
static int max_position_for_type(const char *type) {
	if (!strncmp(type, "ZZ", 2)) {
		return 0;
	}
	if (type[0] == 'O') {
		return 125440;
	}
	if (type[0] == 'F') {
		return 112000;
	}
	if (type[0] == 'T') {
		return 105000;
	}
	return 100000;
}

static bool must_home(const focuser_state *focuser) {
	return focuser->type[0] == 'O';
}

static void reset_focuser(focuser_state *focuser, int index) {
	memset(focuser, 0, sizeof(*focuser));
	snprintf(focuser->type, sizeof(focuser->type), "%s", !strcmp(profile, "syncable") ? "SO" : "OA");
	snprintf(focuser->nickname, sizeof(focuser->nickname), "FocusLynx Foc%d", index + 1);
	focuser->max_position = max_position_for_type(focuser->type);
	focuser->temperature = index == 0 ? 21.7 : 22.1;
	focuser->mode = 'A';
	focuser->coefficient[0] = 86;
	focuser->coefficient[1] = 86;
	focuser->coefficient[2] = 86;
	focuser->temperature_probe = strcmp(profile, "noprobe") != 0;
	serial_motion_sync(&focuser->motion, 0);
}

// ----------------------------------------------------------------- protocol

static void send_line(const char *format, ...) {
	char buffer[COMMAND_LENGTH];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if (length < 0 || length >= (int)sizeof(buffer)) {
		return;
	}
	serial_simulator_trace_line(options.trace, "<-", buffer);
	if (!strcmp(profile, "split") && length > 1) {
		serial_simulator_write_all(serial_fd, buffer, 1);
		usleep(20000);
		serial_simulator_write_all(serial_fd, buffer + 1, (size_t)length - 1);
	} else {
		serial_simulator_write_all(serial_fd, buffer, (size_t)length);
	}
	serial_simulator_write_all(serial_fd, "\n", 1);
}

static void send_error(int code, const char *message) {
	send_line("ER=%d", code);
	send_line("%s", message);
}

static int sim_read_byte(int fd, char *byte) {
	while (running) {
		ssize_t count = read(fd, byte, 1);
		if (count == 1) {
			return 0;
		}
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				usleep(500);
				continue;
			}
			return -1;
		}
		usleep(500);
	}
	return -1;
}

static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;
	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '<') {
			used = 0;
		}
		buffer[used++] = byte;
		if (byte == '>') {
			buffer[used] = '\0';
			serial_simulator_trace_line(options.trace, "->", buffer);
			return (int)used;
		}
	}
	buffer[0] = '\0';
	return -1;
}

static int focuser_index(const char *command) {
	if (command[2] == '1') {
		return 0;
	}
	if (command[2] == '2') {
		return 1;
	}
	return -1;
}

static bool all_digits(const char *text, int count) {
	for (int i = 0; i < count; i++) {
		if (!isdigit((unsigned char)text[i])) {
			return false;
		}
	}
	return true;
}

static void update_focuser(focuser_state *focuser) {
	serial_motion_update(&focuser->motion);
	if (focuser->homing && focuser->motion.duration == 0) {
		focuser->homing = false;
		focuser->homed = true;
	}
}

static void start_motion(focuser_state *focuser, int target) {
	if (target < 0) {
		target = 0;
	}
	if (target > focuser->max_position) {
		target = focuser->max_position;
	}
	serial_motion_start(&focuser->motion, target, MOTION_SPEED);
}

static void handle_get_status(focuser_state *focuser, int index) {
	update_focuser(focuser);
	send_line("!");
	send_line("STATUS%d", index + 1);
	send_line("Temp(C) = %+.1f", focuser->temperature);
	send_line("Curr Pos = %06d", (int)focuser->motion.position);
	send_line("Targ Pos = %06d", (int)focuser->motion.target);
	send_line("IsMoving = %d", focuser->motion.duration > 0 ? 1 : 0);
	send_line("IsHoming = %d", focuser->homing ? 1 : 0);
	send_line("IsHomed = %d", focuser->homed ? 1 : 0);
	send_line("FFDetect = %d", focuser->ff_detect ? 1 : 0);
	send_line("TmpProbe = %d", focuser->temperature_probe ? 1 : 0);
	send_line("RemoteIO = %d", focuser->remote_io ? 1 : 0);
	send_line("Hnd Ctlr = %d", focuser->hand_controller ? 1 : 0);
	send_line("END");
}

static void handle_get_config(focuser_state *focuser, int index) {
	send_line("!");
	send_line("CONFIG%d", index + 1);
	send_line("Nickname = %s", focuser->nickname);
	send_line("Max Pos = %06d", focuser->max_position);
	send_line("Dev Typ = %s", focuser->type);
	send_line("TComp ON = %d", focuser->tcomp_on ? 1 : 0);
	for (int i = 0; i < 5; i++) {
		send_line("TempCo %c = %+05d", 'A' + i, focuser->coefficient[i]);
	}
	send_line("TC Mode = %c", focuser->mode);
	send_line("BLC En = %d", focuser->backlash_on ? 1 : 0);
	send_line("BLC Stps = %+d", focuser->backlash_steps);
	send_line("LED Brt = %03d", led_brightness);
	send_line("TC@Start = %d", focuser->tcomp_at_start ? 1 : 0);
	send_line("END");
}

static void handle_get_hub_info(void) {
	send_line("!");
	send_line("HUB INFO");
	send_line("Hub FVer = 1.0.0");
	send_line("Sleeping = %d", sleeping ? 1 : 0);
	send_line("Wired IP = 169.168.1.10");
	send_line("WF Atchd = 1");
	send_line("WF Conn = 1");
	send_line("WF FVer = 1.0.0");
	send_line("WF FV OK = 1");
	send_line("WF SSID = FocusLynxConfig");
	send_line("WF IP = 192.168.1.11");
	send_line("WF SecMd = A");
	send_line("WF SecKy =");
	send_line("WF WepKI = 0");
	send_line("END");
}

static bool handle_focuser_command(const char *command, const char *body, focuser_state *focuser, int index) {
	update_focuser(focuser);
	if (!strcmp(body, "HELLO")) {
		send_line("!");
		send_line("%s", focuser->nickname);
		return true;
	}
	if (!strcmp(body, "GETSTATUS")) {
		handle_get_status(focuser, index);
		return true;
	}
	if (!strcmp(body, "GETCONFIG")) {
		handle_get_config(focuser, index);
		return true;
	}
	if (!strcmp(body, "HALT")) {
		serial_motion_stop(&focuser->motion);
		focuser->homing = false;
		focuser->tcomp_on = false;
		focuser->tcomp_suspended = false;
		send_line("!");
		send_line("HALTED");
		return true;
	}
	if (!strcmp(body, "HOME")) {
		focuser->homing = true;
		focuser->homed = false;
		start_motion(focuser, 0);
		send_line("!");
		send_line("H");
		return true;
	}
	if (!strcmp(body, "CENTER")) {
		start_motion(focuser, focuser->max_position / 2);
		send_line("!");
		send_line("M");
		return true;
	}
	if (!strcmp(body, "ERM")) {
		serial_motion_stop(&focuser->motion);
		if (focuser->tcomp_suspended) {
			focuser->tcomp_on = true;
			focuser->tcomp_suspended = false;
		}
		send_line("!");
		send_line("STOPPED");
		return true;
	}
	if (!strcmp(body, "RESET")) {
		reset_focuser(focuser, index);
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "MA", 2)) {
		if (strlen(body) != 8 || !all_digits(body + 2, 6)) {
			return false;
		}
		start_motion(focuser, atoi(body + 2));
		send_line("!");
		send_line("M");
		return true;
	}
	if (!strncmp(body, "MIR", 3) || !strncmp(body, "MOR", 3)) {
		if (strlen(body) != 4 || (body[3] != '0' && body[3] != '1')) {
			return false;
		}
		if (focuser->tcomp_on) {
			focuser->tcomp_on = false;
			focuser->tcomp_suspended = true;
		}
		start_motion(focuser, body[1] == 'I' ? 0 : focuser->max_position);
		send_line("!");
		send_line("M");
		return true;
	}
	if (!strncmp(body, "SCCP", 4)) {
		if (strlen(body) != 10 || !all_digits(body + 4, 6)) {
			return false;
		}
		if (must_home(focuser) && strcmp(profile, "nohome")) {
			send_error(9, "SCCP not accepted, focuser must home");
			return true;
		}
		serial_motion_sync(&focuser->motion, atoi(body + 4));
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "SCDT", 4)) {
		if (strlen(body) != 6 || !valid_device_type(body + 4)) {
			return false;
		}
		memcpy(focuser->type, body + 4, 2);
		focuser->type[2] = '\0';
		focuser->max_position = max_position_for_type(focuser->type);
		if (focuser->motion.position > focuser->max_position) {
			serial_motion_sync(&focuser->motion, focuser->max_position);
		}
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "SCNN", 4)) {
		if (strlen(body) < 5 || strlen(body) > 20) {
			return false;
		}
		snprintf(focuser->nickname, sizeof(focuser->nickname), "%s", body + 4);
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "SCTE", 4) || !strncmp(body, "SCTS", 4) || !strncmp(body, "SCBE", 4)) {
		if (strlen(body) != 5 || (body[4] != '0' && body[4] != '1')) {
			return false;
		}
		bool value = body[4] == '1';
		if (body[2] == 'T' && body[3] == 'E') {
			focuser->tcomp_on = value;
			focuser->tcomp_suspended = false;
		} else if (body[2] == 'T' && body[3] == 'S') {
			focuser->tcomp_at_start = value;
		} else {
			focuser->backlash_on = value;
		}
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "SCTM", 4)) {
		if (strlen(body) != 5 || body[4] < 'A' || body[4] > 'E') {
			return false;
		}
		focuser->mode = body[4];
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "SCTC", 4)) {
		if (strlen(body) != 10 || body[4] < 'A' || body[4] > 'E' || (body[5] != '+' && body[5] != '-') || !all_digits(body + 6, 4)) {
			return false;
		}
		int value = atoi(body + 6);
		focuser->coefficient[body[4] - 'A'] = body[5] == '-' ? -value : value;
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strncmp(body, "SCBS", 4)) {
		if (strlen(body) != 6 || !all_digits(body + 4, 2)) {
			return false;
		}
		focuser->backlash_steps = atoi(body + 4);
		send_line("!");
		send_line("SET");
		return true;
	}
	(void)command;
	return false;
}

static bool handle_hub_command(const char *body) {
	if (!strcmp(body, "GETHUBINFO")) {
		handle_get_hub_info();
		return true;
	}
	if (!strncmp(body, "SCLB", 4)) {
		if (strlen(body) != 7 || !all_digits(body + 4, 3) || atoi(body + 4) > 100) {
			return false;
		}
		led_brightness = atoi(body + 4);
		send_line("!");
		send_line("SET");
		return true;
	}
	if (!strcmp(body, "WIFIRESET") || !strcmp(body, "WIFIDEFAULTS") || !strcmp(body, "SWPS") || !strncmp(body, "SWSS", 4) || !strncmp(body, "SWSM", 4) || !strncmp(body, "SWSK", 4) || !strncmp(body, "SWWI", 4)) {
		send_line("!");
		send_line("SET");
		return true;
	}
	return false;
}

static void dispatch_command(const char *command) {
	char body[COMMAND_LENGTH] = { 0 };
	char key[64] = { 0 }, action[256] = { 0 };
	if (events) {
		fprintf(events, "%s\n", command);
		fflush(events);
	}
	const char *fault = getenv("INDIGO_OPTECFL_FAULT");
	FILE *file = fault ? fopen(fault, "r") : NULL;
	if (file) {
		if (fscanf(file, "%63s %255[^\n]", key, action) != 2 || strcmp(key, command)) {
			action[0] = '\0';
		} else {
			unlink(fault);
		}
		fclose(file);
	}
	if (!strcmp(action, "close")) {
		running = 0;
		return;
	}
	if (!strcmp(action, "silent")) {
		return;
	}
	if (!strcmp(action, "partial")) {
		serial_simulator_write_all(serial_fd, "!\nSTAT", 6);
		return;
	}
	if (!strcmp(action, "overlong")) {
		char filler[COMMAND_LENGTH];
		memset(filler, 'X', sizeof(filler) - 1);
		filler[sizeof(filler) - 1] = '\0';
		send_line("!");
		send_line("%s", filler);
		return;
	}
	if (!strcmp(action, "noack")) {
		send_line("SET");
		return;
	}
	if (!strncmp(action, "reply=", 6)) {
		send_line("!");
		send_line("%s", action + 6);
		return;
	}
	if (!strncmp(action, "position=", 9)) {
		int slot = focuser_index(command);
		if (slot >= 0) {
			serial_motion_sync(&focusers[slot].motion, atoi(action + 9));
		}
	}
	if (!strncmp(action, "temp=", 5)) {
		int slot = focuser_index(command);
		if (slot >= 0) {
			focusers[slot].temperature = atof(action + 5);
		}
	}
	if (!strcmp(action, "error")) {
		send_error(1, "Unrecognized command");
		return;
	}
	size_t length = strlen(command);
	if (length < 4 || command[0] != '<' || command[length - 1] != '>' || command[1] != 'F') {
		send_error(1, "Unrecognized command");
		return;
	}
	snprintf(body, sizeof(body), "%.*s", (int)(length - 4), command + 3);
	if (command[2] == 'H') {
		if (!handle_hub_command(body)) {
			send_error(1, "Unrecognized command");
		}
		return;
	}
	int index = focuser_index(command);
	if (index < 0) {
		send_error(1, "Unrecognized command");
		return;
	}
	if (!handle_focuser_command(command, body, focusers + index, index)) {
		send_error(1, "Unrecognized command");
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[COMMAND_LENGTH];
	char port[128];
	if (!parse_args(argc, argv)) {
		usage(argv[0]);
		return 1;
	}
	for (int i = 0; i < 2; i++) {
		reset_focuser(focusers + i, i);
	}
	const char *journal = getenv("INDIGO_OPTECFL_EVENTS");
	events = journal ? fopen(journal, "w") : NULL;
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		if (events) {
			fclose(events);
		}
		return 1;
	}
	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		close(serial_fd);
		serial_fd = -1;
		if (events) {
			fclose(events);
		}
		return 1;
	}
	if (!options.headless) {
		printf("Optec FocusLynx focuser simulator is running on %s\n", port);
		fflush(stdout);
	}
	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command)) > 0) {
			dispatch_command(command);
		} else {
			usleep(1000);
		}
	}
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	if (events) {
		fclose(events);
	}
	return 0;
}
