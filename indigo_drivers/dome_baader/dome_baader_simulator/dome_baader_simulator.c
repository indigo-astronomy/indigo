// Baader Classic Dome simulator
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
// Protocol: fixed-length 9-byte frames with no terminator in either direction.
// All commands are 9 bytes; all responses are 9 bytes.
// State transitions are instantaneous (no background motion thread).

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

#define BAADER_FRAME_LEN 9

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
};

static const char *simulator_name = "dome_baader";

static void usage(const char *name) {
	printf("Baader Classic Dome simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
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

// Azimuth in 0.1° steps, 0-3599 (i.e. 0.0-359.9°)
static int az_position = 0;

// Shutter: 0 = fully closed, 100 = fully open
static int shutter_pos = 0;

// Flap: false = closed, true = open
static bool flap_open = false;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- I/O

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

// All Baader commands are exactly BAADER_FRAME_LEN bytes with no terminator.
static int sim_read_command(int fd, char *buffer, size_t length) {
	size_t used = 0;
	while (running && used < BAADER_FRAME_LEN && used + 1 < length) {
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

// All Baader responses are exactly BAADER_FRAME_LEN bytes with no terminator.
static void sim_write_response(int fd, const char *response) {
	serial_simulator_trace_line(options.trace, "<-", response);
	serial_simulator_write_all(fd, response, BAADER_FRAME_LEN);
}

// ----------------------------------------------------------------- protocol

static void dispatch_command(int fd, const char *cmd) {
	if (!strcmp(cmd, "d#ser_num")) {
		sim_write_response(fd, "d#3141592");
	} else if (!strcmp(cmd, "d#getazim")) {
		char response[BAADER_FRAME_LEN + 1];
		// Format: d#azX#### where X is a single character and #### is az_position
		snprintf(response, sizeof(response), "d#azi%04d", az_position);
		sim_write_response(fd, response);
	} else if (!strncmp(cmd, "d#azi", 5)) {
		az_position = atoi(cmd + 5) % 3600;
		if (az_position < 0) az_position += 3600;
		sim_write_response(fd, "d#gotmess");
	} else if (!strcmp(cmd, "d#getshut")) {
		if (shutter_pos == 100) {
			sim_write_response(fd, "d#shutope");
		} else if (shutter_pos == 0) {
			sim_write_response(fd, "d#shutclo");
		} else {
			char response[BAADER_FRAME_LEN + 1];
			snprintf(response, sizeof(response), "d#shut_%02d", shutter_pos);
			sim_write_response(fd, response);
		}
	} else if (!strcmp(cmd, "d#opeshut")) {
		shutter_pos = 100;
		sim_write_response(fd, "d#gotmess");
	} else if (!strcmp(cmd, "d#closhut")) {
		shutter_pos = 0;
		sim_write_response(fd, "d#gotmess");
	} else if (!strcmp(cmd, "d#getflap")) {
		if (flap_open) {
			sim_write_response(fd, "d#flapope");
		} else {
			sim_write_response(fd, "d#flapclo");
		}
	} else if (!strcmp(cmd, "d#opeflap")) {
		if (shutter_pos < 5) {
			sim_write_response(fd, "d#err_sht");
		} else {
			flap_open = true;
			sim_write_response(fd, "d#gotmess");
		}
	} else if (!strcmp(cmd, "d#cloflap")) {
		if (shutter_pos < 5) {
			sim_write_response(fd, "d#err_sht");
		} else {
			flap_open = false;
			sim_write_response(fd, "d#gotmess");
		}
	} else if (!strcmp(cmd, "d#opefull")) {
		shutter_pos = 100;
		flap_open = true;
		sim_write_response(fd, "d#gotmess");
	} else if (!strcmp(cmd, "d#stopdom")) {
		sim_write_response(fd, "d#gotmess");
	} else if (!strcmp(cmd, "d#get_eme")) {
		sim_write_response(fd, "d#eme0000");
	} else if (!strcmp(cmd, "d#chk_aon")) {
		sim_write_response(fd, "d#automod");
	} else {
		serial_simulator_trace_line(options.trace, "??", cmd);
		sim_write_response(fd, "d#comerro");
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char command[BAADER_FRAME_LEN + 2];
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
		printf("Baader Classic Dome simulator is running on %s\n", port);
		fflush(stdout);
	}

	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command)) == BAADER_FRAME_LEN) {
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
