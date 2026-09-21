// Interactive Astronomy SkyAlert simulator
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

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *firmware;
	char control_file[PATH_MAX];
	char event_file[PATH_MAX];
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.firmware = "1.8m"
};

static const char *simulator_name = "aux_skyalert";

static void usage(const char *name) {
	printf("Interactive Astronomy SkyAlert simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --firmware <version>    Reported firmware version, default is 1.8m\n");
	printf("  Faults: drop, header, truncate <n>, garbage <n>, infinite <n>, close\n");
	printf("  Runtime control is read once from <ready-file>.control as ACTION ARGUMENT\n");
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
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a version\n");
				return false;
			}
			options.firmware = argv[i];
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
	char buffer[64];
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

// Commands are terminated by \r (SkyAlert protocol); \n is skipped.
static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;

	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '\n') {
			continue;
		}
		if (byte == '\r') {
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

// One fault is armed at a time in <ready-file>.control as "ACTION ARGUMENT" and consumed by the
// next record. ARGUMENT is the one-based index of the affected record line for the actions that
// take one, and "*" for the rest.
typedef struct {
	char action[16];
	int line;
} simulator_fault;

static simulator_fault take_fault(void) {
	simulator_fault fault = { { 0 }, 0 };
	if (*options.control_file == '\0') {
		return fault;
	}
	FILE *file = fopen(options.control_file, "r");
	if (file == NULL) {
		return fault;
	}
	char argument[16] = { 0 };
	int count = fscanf(file, "%15s %15s", fault.action, argument);
	fclose(file);
	if (count != 2) {
		fault.action[0] = '\0';
		return fault;
	}
	unlink(options.control_file);
	fault.line = atoi(argument);
	return fault;
}

// The record the device answers "send" with, in the order the driver reads it. The temperature and
// the sky brightness advance per record, one in each published property, so a test can prove that a
// later reading is a new one - a reading whose every value repeats is not published at all, because
// indigo_update_property() suppresses an update in which nothing changed.
static void send_record(int fd, const simulator_fault *fault) {
	static int samples = 0;
	char lines[10][32];
	snprintf(lines[0], sizeof(lines[0]), "Data");
	snprintf(lines[1], sizeof(lines[1]), "%.1f", 20.3 + 0.1 * samples);  // temperature [C]
	snprintf(lines[2], sizeof(lines[2]), "1");                           // sky temperature [C]
	snprintf(lines[3], sizeof(lines[3]), "1008");                        // rain / dampness [raw]
	snprintf(lines[4], sizeof(lines[4]), "%d", 751 + samples);           // sky brightness [raw]
	snprintf(lines[5], sizeof(lines[5]), "66.3");                        // humidity [%]
	snprintf(lines[6], sizeof(lines[6]), "415");                         // wind speed [raw]
	snprintf(lines[7], sizeof(lines[7]), "1");                           // power [1=ok, 0=failure]
	snprintf(lines[8], sizeof(lines[8]), "%s", options.firmware);        // firmware version
	snprintf(lines[9], sizeof(lines[9]), "101791.83");                   // pressure [Pa]
	samples++;
	int count = 10;
	if (!strcmp(fault->action, "drop")) {
		return;
	}
	if (!strcmp(fault->action, "close")) {
		running = 0;
		close(serial_fd);
		serial_fd = -1;
		return;
	}
	if (!strcmp(fault->action, "header")) {
		snprintf(lines[0], sizeof(lines[0]), "Busy");
		count = 1;
	} else if (!strcmp(fault->action, "truncate")) {
		count = fault->line > 0 && fault->line < 10 ? fault->line : 1;
	} else if (!strcmp(fault->action, "garbage") && fault->line > 0 && fault->line < 10) {
		snprintf(lines[fault->line], sizeof(lines[fault->line]), "n/a");
	} else if (!strcmp(fault->action, "infinite") && fault->line > 0 && fault->line < 10) {
		snprintf(lines[fault->line], sizeof(lines[fault->line]), "inf");
	}
	for (int i = 0; i < count; i++) {
		sim_printf(fd, "%s\r", lines[i]);
	}
}

static void dispatch_command(int fd, const char *cmd) {
	record_command(cmd);
	if (!strcmp(cmd, "send")) {
		simulator_fault fault = take_fault();
		send_record(fd, &fault);
	} else {
		serial_simulator_trace_line(options.trace, "??", cmd);
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[64];
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
		printf("Interactive Astronomy SkyAlert simulator is running on %s\n", port);
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
