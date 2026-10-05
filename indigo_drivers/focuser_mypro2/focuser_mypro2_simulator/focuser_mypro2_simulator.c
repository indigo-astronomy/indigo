// myFocuserPro2 focuser simulator
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
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL
};

// normal            - myFP2 board with a temperature probe
// gemini            - Gemini board, which supports full and half step only and runs in half step
// no-sensor         - the DS18B20 probe is absent, :06# answers -127
// silent            - the controller never answers, every transaction times out
// temperature-drift - the probe cools by 2 degrees after the third reading
// custom            - every setting differs from the driver defaults
// moving            - the controller is already running a move when the driver connects
//
// INDIGO_MYPRO2_EVENTS names a journal that receives every request, and
// INDIGO_MYPRO2_FAULT a one-shot control file "<command prefix> <action> [argument]"
// that makes the next matching request misbehave, see pending_fault().
static const char *profile = "normal";
static FILE *events = NULL;
// a stalled motor reports a move in progress while its position stays put
static bool stalled = false;
static int temperature_readings;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static serial_motion motion;
static unsigned max_position = 100000;
// Motor speed 0, 1 and 2 are slow, medium and fast.
static unsigned speed = 0;
static unsigned step_mode = 8;
static unsigned coils_mode = 0;
static unsigned settle_time = 100;
static unsigned backlash_in = 0;
static unsigned backlash_out = 0;
static bool backlash_in_enabled = false;
static bool backlash_out_enabled = false;
static bool reversed = false;

// The controller runs faster in a finer step mode and with a higher motor
// speed setting, which is what makes an abort observable in the tests.
static double steps_per_second(void) {
	return 400.0 * (1 << speed);
}

static void usage(const char *name) {
	printf("myFocuserPro2 focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --profile <name>        normal, gemini, no-sensor, silent, temperature-drift,\n");
	printf("                          custom or moving\n");
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
		} else if (!strcmp(argv[i], "--profile")) {
			if (++i == argc) {
				fprintf(stderr, "--profile requires a name\n");
				return false;
			}
			profile = argv[i];
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

static int sim_read_command(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;
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
			if (c != ':') {
				continue;
			}
			in_frame = true;
			continue;
		}
		if (c == '#') {
			break;
		}
		buffer[total_bytes++] = c;
	}
	buffer[total_bytes] = '\0';
	if (*buffer) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return total_bytes;
}

static bool sim_printf(int handle, const char *format, ...) {
	char buffer[128];
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
	serial_simulator_trace_line(options.trace, "<-", buffer);
	return serial_simulator_write_all(handle, buffer, (size_t)length);
}

// One-shot fault injection. The control file names a command prefix, the way
// the next matching request has to misbehave and an optional argument, so a
// test can fail exactly one transaction without disturbing the rest.
static const char *pending_fault(const char *command, char *argument, size_t size) {
	static char action[32];
	const char *path = getenv("INDIGO_MYPRO2_FAULT");
	if (path == NULL) {
		return NULL;
	}
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	char prefix[32] = { 0 }, value[64] = { 0 };
	action[0] = '\0';
	int fields = fscanf(file, "%31s %31s %63s", prefix, action, value);
	fclose(file);
	if (fields < 2 || strncmp(command, prefix, strlen(prefix))) {
		return NULL;
	}
	snprintf(argument, size, "%s", fields == 3 ? value : "");
	unlink(path);
	return action;
}

static unsigned parse_value(const char *text) {
	return (unsigned)strtoul(text, NULL, 10);
}

static bool is_moving(void) {
	serial_motion_update(&motion);
	return stalled || motion.duration > 0;
}

static void dispatch_command(int handle, const char *command) {
	if (!strcmp(profile, "silent")) {
		return;
	}
	if (events != NULL) {
		fprintf(events, "%s\n", command);
		fflush(events);
	}
	char argument[64] = { 0 };
	const char *fault = pending_fault(command, argument, sizeof(argument));
	if (fault != NULL) {
		if (!strcmp(fault, "close")) {
			// the cable is pulled: the port disappears under the driver
			running = 0;
			if (serial_fd >= 0) {
				close(serial_fd);
				serial_fd = -1;
			}
			return;
		}
		if (!strcmp(fault, "silent") || !strcmp(fault, "ignore")) {
			// silent: a query is never answered; ignore: a setting is not applied
			return;
		}
		if (!strcmp(fault, "garbage")) {
			sim_printf(handle, "ERR#");
			return;
		}
		if (!strcmp(fault, "value")) {
			// the reply is replaced, e.g. by an implausible or malformed one
			sim_printf(handle, "%s", argument);
			return;
		}
		if (!strcmp(fault, "overlong")) {
			sim_printf(handle, "P%s#", "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");
			return;
		}
		if (!strcmp(fault, "truncated")) {
			sim_printf(handle, "P12");
			return;
		}
		if (!strcmp(fault, "split")) {
			// the reply arrives in two pieces, the second one 50 ms later
			char reply[32];
			snprintf(reply, sizeof(reply), "P%u#", (unsigned)serial_motion_update(&motion));
			size_t half = strlen(reply) / 2;
			serial_simulator_write_all(handle, reply, half);
			usleep(50000);
			sim_printf(handle, "%s", reply + half);
			return;
		}
		if (!strcmp(fault, "slow")) {
			// the reply still arrives within the driver's timeout, but late
			// enough for a request to be accepted while it is outstanding
			usleep(200000);
		}
		if (!strcmp(fault, "external")) {
			// a hand controller starts a move the driver did not command
			serial_motion_start(&motion, parse_value(argument), steps_per_second());
		}
		if (!strcmp(fault, "stall")) {
			// the move is accepted, but the motor never turns
			stalled = true;
			return;
		}
		if (!strcmp(fault, "model")) {
			// another board answers the identification this time
			sim_printf(handle, "F%s\n2.71\r#", argument);
			return;
		}
	}
	if (!strcmp(command, "00")) {
		sim_printf(handle, "P%u#", (unsigned)serial_motion_update(&motion));
	} else if (!strcmp(command, "01")) {
		sim_printf(handle, is_moving() ? "I1#" : "I0#");
	} else if (!strcmp(command, "04")) {
		if (!strcmp(profile, "custom")) {
			sim_printf(handle, "FmyFP2ESP32\n2.71\r#");
		} else {
			sim_printf(handle, !strcmp(profile, "gemini") ? "FmyFP2Gemini\n3.0\r#" : "FmyFP2\n3.0\r#");
		}
	} else if (!strncmp(command, "05", 2)) {
		unsigned target = parse_value(command + 2);
		serial_motion_start(&motion, target > max_position ? max_position : target, steps_per_second());
	} else if (!strcmp(command, "06")) {
		if (!strcmp(profile, "custom")) {
			sim_printf(handle, "Z-3.25#");
		} else if (!strcmp(profile, "no-sensor")) {
			sim_printf(handle, "Z-127.00#");
		} else if (!strcmp(profile, "temperature-drift")) {
			sim_printf(handle, ++temperature_readings > 3 ? "Z20.5#" : "Z22.5#");
		} else {
			sim_printf(handle, "Z22.5#");
		}
	} else if (!strncmp(command, "07", 2)) {
		max_position = parse_value(command + 2);
		if (serial_motion_update(&motion) > max_position) {
			serial_motion_sync(&motion, max_position);
		}
	} else if (!strcmp(command, "08")) {
		sim_printf(handle, "M%u#", max_position);
	} else if (!strcmp(command, "11")) {
		sim_printf(handle, "O%u#", coils_mode);
	} else if (!strncmp(command, "12", 2)) {
		coils_mode = parse_value(command + 2) ? 1 : 0;
	} else if (!strcmp(command, "13")) {
		sim_printf(handle, "R%d#", reversed ? 1 : 0);
	} else if (!strncmp(command, "14", 2)) {
		reversed = parse_value(command + 2) != 0;
	} else if (!strncmp(command, "15", 2)) {
		speed = parse_value(command + 2);
	} else if (!strcmp(command, "27")) {
		stalled = false;
		serial_motion_stop(&motion);
	} else if (!strcmp(command, "29")) {
		sim_printf(handle, "S%u#", step_mode);
	} else if (!strncmp(command, "30", 2)) {
		step_mode = parse_value(command + 2);
	} else if (!strncmp(command, "31", 2)) {
		serial_motion_sync(&motion, parse_value(command + 2));
	} else if (!strcmp(command, "48")) {
	} else if (!strncmp(command, "71", 2)) {
		settle_time = parse_value(command + 2);
	} else if (!strcmp(command, "72")) {
		sim_printf(handle, "3%u#", settle_time);
	} else if (!strncmp(command, "73", 2)) {
		backlash_in_enabled = parse_value(command + 2) != 0;
	} else if (!strcmp(command, "74")) {
		sim_printf(handle, "4%d#", backlash_in_enabled ? 1 : 0);
	} else if (!strncmp(command, "75", 2)) {
		backlash_out_enabled = parse_value(command + 2) != 0;
	} else if (!strcmp(command, "76")) {
		sim_printf(handle, "5%d#", backlash_out_enabled ? 1 : 0);
	} else if (!strncmp(command, "77", 2)) {
		backlash_in = parse_value(command + 2);
	} else if (!strcmp(command, "78")) {
		sim_printf(handle, "6%u#", backlash_in);
	} else if (!strncmp(command, "79", 2)) {
		backlash_out = parse_value(command + 2);
	} else if (!strcmp(command, "80")) {
		sim_printf(handle, "7%u#", backlash_out);
	} else if (!strcmp(command, "03")) {
		sim_printf(handle, "F304#");
	}
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[128];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_motion_sync(&motion, 1000);
	if (!strcmp(profile, "custom")) {
		serial_motion_sync(&motion, 23456);
		max_position = 60000;
		step_mode = 4;
		coils_mode = 1;
		settle_time = 30;
		backlash_in = backlash_out = 17;
		backlash_in_enabled = backlash_out_enabled = true;
		reversed = true;
	} else if (!strcmp(profile, "gemini")) {
		step_mode = 2;
	} else if (!strcmp(profile, "moving")) {
		// a move to 5000 at the slowest speed takes 10 seconds
		serial_motion_start(&motion, 5000, steps_per_second());
	}
	const char *journal = getenv("INDIGO_MYPRO2_EVENTS");
	events = journal == NULL ? NULL : fopen(journal, "w");
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_mypro2_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("myFocuserPro2 focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = sim_read_command(serial_fd, buffer, sizeof(buffer));
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
	if (events != NULL) {
		fclose(events);
	}
	return 0;
}
