// USB Dewpoint v1/v2 simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
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
// Derived from protocol.txt and indigo_aux_usbdp.c (all commands are 6 bytes,
// responses are \n-terminated).

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef enum { MODEL_V1, MODEL_V2 } model_type;

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	model_type model;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.model = MODEL_V2,
};

static const char *simulator_name = "aux_usbdp";

static void usage(const char *name) {
	printf("USB Dewpoint simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --model v1|v2           Device version (default: v2)\n");
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --slow-status <ms>      Hold the status reply, so a change can land during a poll\n");
	printf("  --fault <cmd> <mode>    Answer <cmd> with invalid|short|silent|close\n");
	printf("  --fault-once <cmd> <mode>       The same, but only the first time (repeatable)\n");
	printf("  --set <name> <value>    Start with a stored setting or reading other than the default:\n");
	printf("                          output1-3, cal1, cal2, cal_amb, threshold1, threshold2, auto,\n");
	printf("                          linked, aggressivity, temp1, temp2, dewpoint\n");
	printf("  --set-after <n> <name> <value>  Test control: the same setting or reading changes, without a\n");
	printf("                          command, from the status frame after the first <n> status replies\n");
	printf("  -h, --help              Show this help and exit\n");
}

// Delay applied before the status reply, so a test can issue a change request while
// the driver's poll is still waiting for it and exercise that race deterministically.
static int slow_status_ms = 0;
// Fault injection: the named command answers with MODE instead of its reply.
#define MAX_FAULTS 8
static struct {
	const char *command, *mode;
	bool once;
} faults[MAX_FAULTS];
static int fault_count;
// Test control: after this many status replies the named setting or reading takes the
// value, the way a sensor or a setting changed on the controller shows up in the next frame.
static int change_after = -1;
static const char *change_name, *change_value;

static bool set_state(const char *name, const char *value, bool apply);

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
		} else if (!strcmp(argv[i], "--slow-status")) {
			if (++i == argc) {
				fprintf(stderr, "--slow-status requires a delay in milliseconds\n");
				return false;
			}
			slow_status_ms = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--fault") || !strcmp(argv[i], "--fault-once")) {
			if (i + 2 >= argc) {
				fprintf(stderr, "%s requires a command and a mode\n", argv[i]);
				return false;
			}
			if (fault_count == MAX_FAULTS) {
				fprintf(stderr, "At most %d faults can be injected\n", MAX_FAULTS);
				return false;
			}
			faults[fault_count].once = !strcmp(argv[i], "--fault-once");
			faults[fault_count].command = argv[++i];
			faults[fault_count].mode = argv[++i];
			if (strcmp(faults[fault_count].mode, "invalid") && strcmp(faults[fault_count].mode, "short") && strcmp(faults[fault_count].mode, "silent") && strcmp(faults[fault_count].mode, "close")) {
				fprintf(stderr, "Unknown fault mode '%s'\n", faults[fault_count].mode);
				return false;
			}
			fault_count++;
		} else if (!strcmp(argv[i], "--set")) {
			if (i + 2 >= argc) {
				fprintf(stderr, "--set requires a name and a value\n");
				return false;
			}
			if (!set_state(argv[i + 1], argv[i + 2], true)) {
				fprintf(stderr, "Unknown setting '%s'\n", argv[i + 1]);
				return false;
			}
			i += 2;
		} else if (!strcmp(argv[i], "--set-after")) {
			if (i + 3 >= argc) {
				fprintf(stderr, "--set-after requires a count, a name and a value\n");
				return false;
			}
			change_after = atoi(argv[++i]);
			change_name = argv[++i];
			change_value = argv[++i];
			if (change_after < 0 || !set_state(change_name, change_value, false)) {
				fprintf(stderr, "Invalid --set-after '%s' '%s'\n", argv[i - 2], change_name);
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
				fprintf(stderr, "--model requires v1 or v2\n");
				return false;
			}
			if (!strcmp(argv[i], "v1")) {
				options.model = MODEL_V1;
			} else if (!strcmp(argv[i], "v2")) {
				options.model = MODEL_V2;
			} else {
				fprintf(stderr, "Unknown model '%s', expected v1 or v2\n", argv[i]);
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

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

// V2 state
static float temp_ch1 = 23.5f, temp_ch2 = 22.1f, temp_amb = 20.0f;
static float rh = 45.0f, dewpoint = 8.3f;
static int output_ch1 = 0, output_ch2 = 0, output_ch3 = 0;
static int cal_ch1 = 0, cal_ch2 = 0, cal_amb = 0;
static int threshold_ch1 = 2, threshold_ch2 = 2;
static int auto_mode = 0, ch2_3_linked = 0, aggressivity = 1;

// V1 state
static float temp_loc = 23.5f;

// The controller keeps its settings in EEPROM, so it can start with other values than the driver's defaults.
static bool set_state(const char *name, const char *value, bool apply) {
	static const struct {
		const char *name;
		int *value;
	} ints[] = {
		{ "output1", &output_ch1 }, { "output2", &output_ch2 }, { "output3", &output_ch3 },
		{ "cal1", &cal_ch1 }, { "cal2", &cal_ch2 }, { "cal_amb", &cal_amb },
		{ "threshold1", &threshold_ch1 }, { "threshold2", &threshold_ch2 },
		{ "auto", &auto_mode }, { "linked", &ch2_3_linked }, { "aggressivity", &aggressivity }
	};
	static const struct {
		const char *name;
		float *value;
	} floats[] = {
		{ "temp1", &temp_ch1 }, { "temp2", &temp_ch2 }, { "dewpoint", &dewpoint }
	};
	for (size_t i = 0; i < sizeof(ints) / sizeof(ints[0]); i++) {
		if (!strcmp(name, ints[i].name)) {
			if (apply) {
				*ints[i].value = atoi(value);
			}
			return true;
		}
	}
	for (size_t i = 0; i < sizeof(floats) / sizeof(floats[0]); i++) {
		if (!strcmp(name, floats[i].name)) {
			if (apply) {
				*floats[i].value = (float)atof(value);
			}
			return true;
		}
	}
	return false;
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- protocol

static bool sim_printf(int fd, const char *format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0 || length >= (int)sizeof(buffer)) {
		return false;
	}
	serial_simulator_trace_line(options.trace, "<-", buffer);
	return serial_simulator_write_all(fd, buffer, (size_t)length);
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

// All UDP commands are exactly 6 bytes with no terminator.
static int sim_read_command(int fd, char *buffer, size_t length) {
	size_t used = 0;
	while (running && used < 6 && used + 1 < length) {
		char byte;
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		buffer[used++] = byte;
	}
	buffer[used] = '\0';
	if (used > 0) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return (int)used;
}

static bool inject_fault(int fd, const char *cmd) {
	for (int i = 0; i < fault_count; i++) {
		if (faults[i].command == NULL || strcmp(cmd, faults[i].command)) {
			continue;
		}
		const char *mode = faults[i].mode;
		if (faults[i].once) {
			faults[i].command = NULL;
		}
		if (!strcmp(mode, "invalid")) {
			sim_printf(fd, "invalid\n");
		} else if (!strcmp(mode, "short")) {
			sim_printf(fd, "##1.0/2.0**\n");
		} else if (!strcmp(mode, "close")) {
			close(fd);
		}
		// "silent" answers nothing at all.
		return true;
	}
	return false;
}

static void dispatch_command(int fd, const char *cmd) {
	if (inject_fault(fd, cmd)) {
		return;
	}
	if (slow_status_ms > 0 && !strcmp(cmd, "SGETAL")) {
		usleep((useconds_t)slow_status_ms * 1000);
	}
	if (!strcmp(cmd, "SWHOIS")) {
		if (options.model == MODEL_V1) {
			sim_printf(fd, "UDP\n");
		} else {
			sim_printf(fd, "UDP2(1446)\n");
		}
	} else if (!strcmp(cmd, "SGETAL")) {
		if (change_after >= 0 && change_after-- == 0) {
			set_state(change_name, change_value, true);
		}
		if (options.model == MODEL_V1) {
			sim_printf(fd, "Tloc=%.1f-Tamb=%.1f-RH=%.1f-DP=%.1f-TH=2-C=0\n",
				temp_loc, temp_amb, rh, dewpoint);
		} else {
			sim_printf(fd, "##%.1f/%.1f/%.1f/%.1f/%.1f/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u**\n",
				temp_ch1, temp_ch2, temp_amb, rh, dewpoint,
				output_ch1, output_ch2, output_ch3,
				cal_ch1, cal_ch2, cal_amb,
				threshold_ch1, threshold_ch2,
				auto_mode, ch2_3_linked, aggressivity);
		}
	} else if (!strcmp(cmd, "SEERAZ")) {
		sim_printf(fd, "EEPROM RESET\n");
	} else if (cmd[0] == 'S' && cmd[1] >= '1' && cmd[1] <= '3' && cmd[2] == 'O') {
		int channel = cmd[1] - '0';
		int power = (cmd[3] - '0') * 100 + (cmd[4] - '0') * 10 + (cmd[5] - '0');
		if (channel == 1) output_ch1 = power;
		else if (channel == 2) output_ch2 = power;
		else output_ch3 = power;
		sim_printf(fd, "DONE\n");
	} else if (!strncmp(cmd, "SAUTO", 5)) {
		auto_mode = cmd[5] == '1';
		sim_printf(fd, "DONE\n");
	} else if (!strncmp(cmd, "STHR", 4)) {
		threshold_ch1 = cmd[4] - '0';
		threshold_ch2 = cmd[5] - '0';
		sim_printf(fd, "DONE\n");
	} else if (!strncmp(cmd, "SCA", 3)) {
		cal_ch1 = cmd[3] - '0';
		cal_ch2 = cmd[4] - '0';
		cal_amb = cmd[5] - '0';
		sim_printf(fd, "DONE\n");
	} else if (!strncmp(cmd, "SLINK", 5)) {
		ch2_3_linked = cmd[5] == '1';
		sim_printf(fd, "DONE\n");
	} else if (!strncmp(cmd, "SAGGR", 5)) {
		aggressivity = cmd[5] - '0';
		sim_printf(fd, "DONE\n");
	} else {
		serial_simulator_trace_line(options.trace, "??", cmd);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[16];
	char port[128];

	if (!parse_args(argc, argv)) {
		usage(argv[0]);
		return 1;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	if (!options.headless) {
		printf("USB Dewpoint %s simulator is running on %s\n",
			options.model == MODEL_V1 ? "v1" : "v2", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command)) == 6) {
			dispatch_command(serial_fd, command);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
