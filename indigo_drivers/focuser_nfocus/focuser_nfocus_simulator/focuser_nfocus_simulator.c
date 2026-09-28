// Rigel Systems nFOCUS focuser simulator
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
	bool temperature_present;
	const char *ready_file;
	const char *event_file;
	const char *fault_file;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.temperature_present = true,
	.ready_file = NULL,
	.event_file = NULL,
	.fault_file = NULL
};

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static char speed_in[4] = "001";
static char speed_out[4] = "005";
static serial_motion motion;

static void usage(const char *name) {
	printf("Rigel Systems nFOCUS focuser simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --event-file <path>     Append received commands and injected faults\n");
	printf("  --fault-file <path>     Read one-shot fault injection commands\n");
	printf("  --temperature absent    Report the optional sensor as absent\n");
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
		} else if (!strcmp(argv[i], "--event-file")) {
			if (++i == argc) {
				fprintf(stderr, "--event-file requires a path\n");
				return false;
			}
			options.event_file = argv[i];
		} else if (!strcmp(argv[i], "--fault-file")) {
			if (++i == argc) {
				fprintf(stderr, "--fault-file requires a path\n");
				return false;
			}
			options.fault_file = argv[i];
		} else if (!strcmp(argv[i], "--temperature")) {
			if (++i == argc) {
				fprintf(stderr, "--temperature requires present or absent\n");
				return false;
			}
			if (!strcmp(argv[i], "present")) {
				options.temperature_present = true;
			} else if (!strcmp(argv[i], "absent")) {
				options.temperature_present = false;
			} else {
				fprintf(stderr, "--temperature requires present or absent\n");
				return false;
			}
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

static void command_text(const char *command, int length, char *text, size_t size) {
	if (length == 1 && command[0] == 0x06) {
		snprintf(text, size, "ID");
		return;
	}
	int used = 0;
	for (int i = 0; i < length && used < (int)size - 1; i++) {
		unsigned char c = (unsigned char)command[i];
		if (c >= 32 && c < 127) {
			text[used++] = (char)c;
		} else if (used < (int)size - 4) {
			used += snprintf(text + used, size - (size_t)used, "\\x%02X", c);
		}
	}
	text[used] = 0;
}

static void append_event(const char *kind, const char *value) {
	if (options.event_file == NULL) {
		return;
	}
	FILE *events = fopen(options.event_file, "a");
	if (events != NULL) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, value);
		fclose(events);
	}
}

static bool command_matches_key(const char *text, const char *key) {
	size_t key_length = strlen(key);
	return key_length > 0 && !strncmp(text, key, key_length);
}

static bool consume_fault(const char *command, int length, char *action, size_t size) {
	if (options.fault_file == NULL) {
		return false;
	}
	FILE *file = fopen(options.fault_file, "r");
	if (file == NULL) {
		return false;
	}
	char key[64] = { 0 };
	char requested[32] = { 0 };
	int fields = fscanf(file, "%63s %31s", key, requested);
	fclose(file);
	if (fields != 2) {
		return false;
	}
	char text[64];
	command_text(command, length, text, sizeof(text));
	if (!command_matches_key(text, key)) {
		return false;
	}
	if (!strncmp(requested, "sticky_", 7)) {
		snprintf(action, size, "%s", requested + 7);
	} else {
		unlink(options.fault_file);
		snprintf(action, size, "%s", requested);
	}
	char event[128];
	snprintf(event, sizeof(event), "%s:%s", key, requested);
	append_event("FAULT", event);
	return true;
}

static bool write_reply(int handle, const char *buffer, size_t length) {
	if (options.trace) {
		fprintf(stderr, "<- ");
		for (size_t i = 0; i < length; i++) {
			unsigned char c = (unsigned char)buffer[i];
			if (c >= 32 && c < 127) {
				fputc(c, stderr);
			} else {
				fprintf(stderr, "\\x%02X", c);
			}
		}
		fprintf(stderr, "\n");
	}
	return serial_simulator_write_all(handle, buffer, length);
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
	return write_reply(handle, buffer, (size_t)length);
}

static void trace_command(const char *command, int length) {
	char text[64];
	command_text(command, length, text, sizeof(text));
	append_event("RX", text);
	if (options.trace) {
		fprintf(stderr, "-> %s\n", text);
	}
}

static int read_exact(int handle, char *buffer, int length) {
	int total_bytes = 0;
	while (running && total_bytes < length) {
		ssize_t bytes_read = read(handle, buffer + total_bytes, (size_t)(length - total_bytes));
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				return total_bytes;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return total_bytes;
		}
		total_bytes += (int)bytes_read;
	}
	return total_bytes;
}

static void discard_until_hash(int handle) {
	char c;
	while (running) {
		int bytes = read_exact(handle, &c, 1);
		if (bytes <= 0 || c == '#') {
			return;
		}
	}
}

static int read_nfocus_command(int handle, char *buffer, int length) {
	char c = '\0';

	while (running) {
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
		break;
	}

	if (c == 0x06 || c == 'S' || c == '#') {
		buffer[0] = c;
		trace_command(buffer, 1);
		return 1;
	}
	if (c != ':') {
		return 0;
	}

	buffer[0] = c;
	if (read_exact(handle, buffer + 1, 2) != 2) {
		return 0;
	}

	int command_length = 3;
	if (!strncmp(buffer, ":RT", 3) || !strncmp(buffer, ":RO", 3) || !strncmp(buffer, ":RS", 3)) {
		trace_command(buffer, command_length);
		return command_length;
	}

	if (!strncmp(buffer, ":CS", 3) || !strncmp(buffer, ":CO", 3) || !strncmp(buffer, ":CF", 3)) {
		if (command_length + 4 > length || read_exact(handle, buffer + command_length, 4) != 4) {
			return 0;
		}
		command_length += 4;
		trace_command(buffer, command_length);
		return command_length;
	}

	if (!strncmp(buffer, ":F", 2)) {
		if (command_length + 5 > length || read_exact(handle, buffer + command_length, 5) != 5) {
			return 0;
		}
		command_length += 5;
		trace_command(buffer, command_length);
		return command_length;
	}

	discard_until_hash(handle);
	trace_command(buffer, command_length);
	return command_length;
}

static bool dispatch_fault(int handle, const char *command, int length) {
	char action[32];
	if (!consume_fault(command, length, action, sizeof(action))) {
		return false;
	}
	if (!strcmp(action, "silent")) {
		return true;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		close(handle);
		serial_fd = -1;
		return true;
	}
	if (!strcmp(action, "partial")) {
		write_reply(handle, "x", 1);
		return true;
	}
	if (!strcmp(action, "overlong")) {
		if (length == 1 && command[0] == 0x06) {
			write_reply(handle, "nX", 2);
		} else if (length == 1 && command[0] == 'S') {
			write_reply(handle, "0X", 2);
		} else if (!strncmp(command, ":RT", 3)) {
			write_reply(handle, "+275X", 5);
		} else if (!strncmp(command, ":RO", 3) || !strncmp(command, ":RS", 3)) {
			write_reply(handle, "005X", 4);
		}
		return true;
	}
	if (!strcmp(action, "malformed")) {
		if (length == 1 && command[0] == 0x06) {
			write_reply(handle, "x", 1);
		} else if (length == 1 && command[0] == 'S') {
			write_reply(handle, "?", 1);
		} else if (!strncmp(command, ":RT", 3)) {
			write_reply(handle, "abcd", 4);
		} else if (!strncmp(command, ":RO", 3) || !strncmp(command, ":RS", 3)) {
			write_reply(handle, "999", 3);
		}
		return true;
	}
	if (!strcmp(action, "absent") && !strncmp(command, ":RT", 3)) {
		write_reply(handle, "-888", 4);
		return true;
	}
	return false;
}

static void dispatch_command(int handle, const char *command, int length) {
	if (dispatch_fault(handle, command, length)) {
		return;
	}
	if (length == 1 && command[0] == 0x06) {
		sim_printf(handle, "n");
	} else if (length == 1 && command[0] == 'S') {
		serial_motion_update(&motion);
		sim_printf(handle, motion.duration > 0 ? "1" : "0");
	} else if (!strncmp(command, ":RT", 3)) {
		sim_printf(handle, options.temperature_present ? "+275" : "-888");
	} else if (!strncmp(command, ":RO", 3)) {
		write_reply(handle, speed_out, 3);
	} else if (!strncmp(command, ":RS", 3)) {
		write_reply(handle, speed_in, 3);
	} else if (!strncmp(command, ":CS", 3) && length >= 7) {
		memcpy(speed_in, command + 3, 3);
		speed_in[3] = '\0';
	} else if ((!strncmp(command, ":CO", 3) || !strncmp(command, ":CF", 3)) && length >= 7) {
		memcpy(speed_out, command + 3, 3);
		speed_out[3] = '\0';
	} else if (!strncmp(command, ":F11000#", 8)) {
		serial_motion_stop(&motion);
	} else if (!strncmp(command, ":F", 2) && length >= 8) {
		char steps_text[4] = { command[4], command[5], command[6], 0 };
		int steps = atoi(steps_text);
		int direction = command[2] == '1' ? -1 : 1;
		serial_motion_start(&motion, motion.position + direction * steps, 80);
	}
}

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[32];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	serial_motion_sync(&motion, 0);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, "focuser_nfocus_simulator", port)) {
		close(serial_fd);
		return 1;
	}

	if (!options.headless) {
		printf("Rigel Systems nFOCUS focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		int bytes = read_nfocus_command(serial_fd, buffer, sizeof(buffer));
		if (bytes < 0) {
			break;
		}
		if (bytes > 0) {
			dispatch_command(serial_fd, buffer, bytes);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
