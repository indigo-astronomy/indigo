// Unihedron SQM simulator
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
#include <math.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	char control_file[PATH_MAX];
	char event_file[PATH_MAX];
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
};

static const char *simulator_name = "aux_sqm";

static void usage(const char *name) {
	printf("Unihedron SQM simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  Faults: drop, prefix, short, nan, garbage, close; reading <mpsas> sets the value\n");
	printf("  Runtime control is read once from <ready-file>.control as ACTION SELECTOR [VALUE]\n");
	printf("  and every complete command is recorded in <ready-file>.events.\n");
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
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	if (options.ready_file != NULL) {
		snprintf(options.control_file, sizeof(options.control_file), "%s.control", options.ready_file);
		snprintf(options.event_file, sizeof(options.event_file), "%s.events", options.ready_file);
	}
	return true;
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

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

// SQM commands are terminated by 'x' (e.g. "ix", "rx"); \r and \n are skipped.
static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;

	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '\r' || byte == '\n') {
			continue;
		}
		if (byte == 'x') {
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

static void record_command(const char *command) {
	if (*options.event_file == '\0') {
		return;
	}
	FILE *file = fopen(options.event_file, "a");
	if (file == NULL) {
		return;
	}
	fprintf(file, "%s\n", command);
	fclose(file);
}

// One control line is armed at a time in <ready-file>.control as "ACTION SELECTOR [VALUE]" and is
// consumed by the next command whose first character matches the selector, or by any command when
// the selector is "*".
typedef struct {
	char action[16];
	double value;
	bool has_value;
} simulator_control;

static simulator_control take_control(const char *command) {
	simulator_control control = { { 0 }, 0, false };
	if (*options.control_file == '\0') {
		return control;
	}
	FILE *file = fopen(options.control_file, "r");
	if (file == NULL) {
		return control;
	}
	char selector[16] = { 0 };
	char value[32] = { 0 };
	int count = fscanf(file, "%15s %15s %31s", control.action, selector, value);
	fclose(file);
	if (count < 2 || (strcmp(selector, "*") && (command[0] != selector[0] || selector[1] != '\0'))) {
		control.action[0] = '\0';
		return control;
	}
	unlink(options.control_file);
	if (count == 3) {
		control.value = atof(value);
		control.has_value = true;
	}
	return control;
}

// Unit information response, table 8.6 of the SQM-LU-DL manual: "i," then the protocol, model,
// feature and serial number as eight digits each.
static void send_unit_information(int fd, const simulator_control *control) {
	if (!strcmp(control->action, "drop")) {
		return;
	}
	if (!strcmp(control->action, "close")) {
		running = 0;
		close(serial_fd);
		serial_fd = -1;
		return;
	}
	if (!strcmp(control->action, "prefix")) {
		sim_printf(fd, "x,00000002,00000003,00000001,00000413\n");
		return;
	}
	if (!strcmp(control->action, "short")) {
		sim_printf(fd, "i\n");
		return;
	}
	sim_printf(fd, "i,00000002,00000003,00000001,00000413\n");
}

// Reading response, table 8.3 of the SQM-LU-DL manual: "r," then the reading in mag/arcsec2, the
// sensor frequency, the period in counts, the period in seconds and the sensor temperature. The
// reading falls by 0.01 and the counts rise by one per record, well inside one Bortle class, so
// that every record differs from the one before it and is therefore actually published.
static void send_reading(int fd, const simulator_control *control) {
	static int samples = 0;
	double brightness = control->has_value ? control->value : 20.70 - 0.01 * samples;
	int counts = 20 + samples;
	samples++;
	if (!strcmp(control->action, "drop")) {
		return;
	}
	if (!strcmp(control->action, "close")) {
		running = 0;
		close(serial_fd);
		serial_fd = -1;
		return;
	}
	if (!strcmp(control->action, "prefix")) {
		sim_printf(fd, "x,%c%05.2fm,0000022921Hz,%010dc,0000000.000s, 039.4C\n", brightness < 0 ? '-' : ' ', fabs(brightness), counts);
		return;
	}
	if (!strcmp(control->action, "short")) {
		sim_printf(fd, "r,%c%05.2fm,0000022921Hz,%010dc\n", brightness < 0 ? '-' : ' ', fabs(brightness), counts);
		return;
	}
	if (!strcmp(control->action, "nan")) {
		sim_printf(fd, "r,   NaNm,0000022921Hz,%010dc,0000000.000s, 039.4C\n", counts);
		return;
	}
	if (!strcmp(control->action, "garbage")) {
		sim_printf(fd, "r,%c%05.2fm,  bogusHz,%010dc,0000000.000s, 039.4C\n", brightness < 0 ? '-' : ' ', fabs(brightness), counts);
		return;
	}
	sim_printf(fd, "r,%c%05.2fm,0000022921Hz,%010dc,0000000.000s, 039.4C\n", brightness < 0 ? '-' : ' ', fabs(brightness), counts);
}

static void dispatch_command(int fd, const char *cmd) {
	record_command(cmd);
	simulator_control control = take_control(cmd);
	if (!strcmp(cmd, "i")) {
		send_unit_information(fd, &control);
	} else if (!strcmp(cmd, "r") || !strcmp(cmd, "u")) {
		send_reading(fd, &control);
	} else {
		serial_simulator_trace_line(options.trace, "??", cmd);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[32];
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
		printf("Unihedron SQM simulator is running on %s\n", port);
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
