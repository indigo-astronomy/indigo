// Pocket Powerbox (PPB / PPBA / SPB) simulator
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
// This simulator was refactored from the Arduino sketch of the same name by the Claude Code agent (claude-sonnet-4-6).

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef enum { MODEL_PPB, MODEL_PPBA, MODEL_SPB } ppb_model_t;

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	ppb_model_t model;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.model = MODEL_PPB,
};

static const char *simulator_name = "aux_ppb";

static void usage(const char *name) {
	printf("Pocket Powerbox simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <ppb|ppba|spb>  Simulated device model (default: ppb)\n");
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
				fprintf(stderr, "--model requires ppb, ppba, or spb\n");
				return false;
			}
			if (!strcmp(argv[i], "ppba")) {
				options.model = MODEL_PPBA;
			} else if (!strcmp(argv[i], "spb")) {
				options.model = MODEL_SPB;
			} else if (!strcmp(argv[i], "ppb")) {
				options.model = MODEL_PPB;
			} else {
				fprintf(stderr, "Unknown model '%s', use ppb, ppba, or spb\n", argv[i]);
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

static bool power1 = true;
static bool power2 = true;
static int dew1 = 0;
static int dew2 = 0;
static bool autodev = true;
static bool power_alert = false;
static int dslr_adj = 5;

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
	char buffer[128];
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

static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;

	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '\r') {
			continue;
		}
		if (byte == '\n') {
			buffer[used] = '\0';
			if (used > 0) {
				serial_simulator_trace_line(options.trace, "->", buffer);
			}
			return (int)used;
		}
		buffer[used++] = byte;
	}

	buffer[0] = '\0';
	return -1;
}

static void dispatch_command(int fd, const char *cmd) {
	if (!strcmp(cmd, "P#")) {
		if (options.model == MODEL_PPBA)
			sim_printf(fd, "PPBA_OK\n");
		else if (options.model == MODEL_SPB)
			sim_printf(fd, "SPB\n");
		else
			sim_printf(fd, "PPB_OK\n");
	} else if (!strcmp(cmd, "PV")) {
		sim_printf(fd, "1.5\n");
	} else if (!strncmp(cmd, "PL:", 3)) {
		sim_printf(fd, "%s\n", cmd);
	} else if (!strcmp(cmd, "PA")) {
		// Current raw: driver divides by 65 to get amps; send 65 when load is present
		int current_raw = (power1 || power2 || dew1 > 0 || dew2 > 0) ? 65 : 0;
		if (options.model == MODEL_PPBA) {
			sim_printf(fd, "PPBA:12.2:%d.0:23.2:59:14.7:%d:%d:%d:%d:%d:%d:%d\n",
				current_raw, power1, power2, dew1, dew2, autodev, power_alert, dslr_adj);
		} else {
			// SPB and PPB share the PPB: prefix; SPB has no DSLR outlet (report 0 for slot 2)
			int p2 = (options.model == MODEL_SPB) ? 0 : power2;
			sim_printf(fd, "PPB:12.2:%d.0:23.2:59:14.7:%d:%d:%d:%d:%d\n",
				current_raw, power1, p2, dew1, dew2, autodev);
		}
	} else if (!strncmp(cmd, "P1:", 3)) {
		power1 = cmd[3] == '1';
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "P2:", 3)) {
		int val = atoi(cmd + 3);
		if (val == 0) {
			power2 = false;
		} else {
			power2 = true;
			if (val > 1)
				dslr_adj = val;
		}
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "P3:", 3)) {
		dew1 = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "P4:", 3)) {
		dew2 = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "PD:", 3)) {
		autodev = cmd[3] == '1';
		// PPBA responds with current dew aggressiveness; PPB/SPB echo the command
		if (options.model == MODEL_PPBA)
			sim_printf(fd, "PD:210\n");
		else
			sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "PE:", 3)) {
		power1 = cmd[3] == '1';
		if (options.model != MODEL_SPB && cmd[4] != '\0')
			power2 = cmd[4] == '1';
		sim_printf(fd, "PE:1\n");
	} else if (!strcmp(cmd, "PF")) {
		// Reboot: driver sends via indigo_uni_printf directly, no response expected
	} else {
		serial_simulator_trace_line(options.trace, "??", cmd);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[128];
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
		const char *model_name = options.model == MODEL_PPBA ? "PPBA" : (options.model == MODEL_SPB ? "SPB" : "PPB");
		printf("Pocket Powerbox %s simulator is running on %s\n", model_name, port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command)) > 0) {
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
