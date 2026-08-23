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

static void dispatch_command(int fd, const char *cmd) {
	if (!strcmp(cmd, "SWHOIS")) {
		if (options.model == MODEL_V1) {
			sim_printf(fd, "UDP\n");
		} else {
			sim_printf(fd, "UDP2(1446)\n");
		}
	} else if (!strcmp(cmd, "SGETAL")) {
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
