// LACERTA Motorfocus focuser simulator
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
// This simulator was refactored by a Codex agent.

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/select.h>
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

static const char *profile = "normal", *model = "MFOC", *firmware = "3.1.123", *ready_file;
static const char *fault_file;
static FILE *events;
static bool trace, headless, motion_active, injected;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1, backlash = 3, direction, maximum = 250000;
static double temperature = 23.5;
static serial_motion motion;

static void event(const char *kind, const char *value) {
	if (events) {
		fprintf(events, "%.6f %s %s\n", serial_motion_time(), kind, value);
		fflush(events);
	}
}

static bool reply(const char *format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if (length < 0 || length >= (int)sizeof(buffer)) {
		return false;
	}
	event("TX", buffer);
	serial_simulator_trace_line(trace, "<-", buffer);
	if (!strcmp(profile, "split") && length > 2) {
		serial_simulator_write_all(serial_fd, buffer, (size_t)length / 2);
		usleep(10000);
		return serial_simulator_write_all(serial_fd, buffer + length / 2, (size_t)(length - length / 2));
	}
	return serial_simulator_write_all(serial_fd, buffer, (size_t)length);
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

static void update_motion(void) {
	int position = (int)serial_motion_update(&motion);
	if (motion_active && motion.duration == 0) {
		motion_active = false;
		reply("M %d\r", position);
		reply("p %d\r", position);
		event("DONE", "motion");
	}
}

static bool inject(char command) {
	char action[64] = "", key[32] = { 0 };
	FILE *file = fault_file ? fopen(fault_file, "r") : NULL;
	if (file) {
		if (fscanf(file, "%31s %63s", key, action) != 2) {
			*action = 0;
		}
		fclose(file);
		if (!strcmp(key, "external")) {
			serial_motion_sync(&motion, atoi(action));
			motion_active = false;
			unlink(fault_file);
			return false;
		}
		if (!strcmp(key, "temperature")) {
			temperature = atof(action);
			unlink(fault_file);
			return false;
		}
		if (key[0] != command || key[1]) {
			*action = 0;
		} else {
			unlink(fault_file);
		}
	}
	if (!injected && !strncmp(profile, "init_", 5) && command == profile[5]) {
		injected = true;
		snprintf(action, sizeof(action), "%s", profile + 7);
	}
	if (!*action) {
		return false;
	}
	event("FAULT", action);
	char response = command == 'q' || command == 'P' ? 'p' : command == 'H' ? 'H' : (char)tolower((unsigned char)command);
	if (!strcmp(action, "silent")) {
		return true;
	} else if (!strcmp(action, "close")) {
		running = 0;
		return true;
	} else if (!strcmp(action, "overlong")) {
		char buffer[160];
		memset(buffer, '7', sizeof(buffer));
		buffer[0] = response;
		buffer[1] = ' ';
		buffer[158] = '\r';
		buffer[159] = 0;
		reply("%s", buffer);
	} else if (!strcmp(action, "short")) {
		reply("%c\r", response);
	} else if (!strcmp(action, "mismatch")) {
		reply("%c 2\r", response);
	} else if (!strcmp(action, "reject")) {
		reply("%c 0\r", command);
	} else if (!strcmp(action, "partial")) {
		reply("%c 12", response);
	} else if (!strcmp(action, "flood")) {
		for (int i = 0; i < 150; i++) {
			reply("D ignored\r");
		}
	} else {
		reply("%c invalid\r", response);
	}
	return true;
}

static void dispatch(const char *text) {
	char command;
	int value = 0;
	if (sscanf(text, ": %c %d", &command, &value) < 1) {
		return;
	}
	event("RX", text);
	serial_simulator_trace_line(trace, "->", text);
	if (inject(command)) {
		return;
	}
	if (!strcmp(profile, "debug")) {
		reply("D : %c command received\r", command);
		reply("D : %c command execution\r", command);
	}
	switch (command) {
		case 'i': reply("i %s\r", !strcmp(profile, "unknown") ? "OTHER" : model); break;
		case 'v': reply("v%s\r", firmware); break;
		case 'q': reply("p %d\r", (int)serial_motion_update(&motion)); break;
		case 't': reply("t %g\r", temperature); break;
		case 'P':
			if (value >= 0 && value <= maximum) {
				serial_motion_sync(&motion, value);
				motion_active = false;
			}
			reply("p %d\r", (int)serial_motion_update(&motion));
			break;
		case 'M':
			serial_motion_start(&motion, value < 0 ? 0 : value > maximum ? maximum : value, 1000);
			motion_active = true;
			break;
		case 'H':
			serial_motion_stop(&motion);
			motion_active = false;
			reply("H 1\r");
			break;
		case 'B':
			if (value >= 0 && value <= 255) {
				backlash = value;
			}
			// Fall through to the readback.
		case 'b': reply("b %d\r", backlash); break;
		case 'R':
			if (value == 0 || value == 1) {
				direction = value;
			}
		case 'r': reply("r %d\r", direction); break;
		case 'G':
			if (value >= 300 && value <= (*firmware == '1' ? 65535 : 250000)) {
				maximum = value;
			}
		case 'g': reply("g %d\r", maximum); break;
		default: reply("D unknown command\r%c 0\r", command); break;
	}
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) {
			headless = true;
		} else if (!strcmp(argv[i], "--trace")) {
			trace = true;
		} else if (i + 1 < argc && !strcmp(argv[i], "--ready-file")) {
			ready_file = argv[++i];
		} else if (i + 1 < argc && !strcmp(argv[i], "--profile")) {
			profile = argv[++i];
		} else if (i + 1 < argc && !strcmp(argv[i], "--model")) {
			model = argv[++i];
		} else if (i + 1 < argc && !strcmp(argv[i], "--firmware")) {
			firmware = argv[++i];
		} else {
			fprintf(stderr, "Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile NAME] [--model MFOC|FMC] [--firmware VERSION]\n", argv[0]);
			return 1;
		}
	}
	if (!strcmp(profile, "fmc")) {
		model = "FMC";
		firmware = "1.1.123";
	} else if (!strcmp(profile, "mfoc2")) {
		firmware = "2.1.123";
	}
	maximum = *firmware == '1' ? 65535 : 250000;
	if (!strcmp(profile, "nc")) {
		temperature = 99.9;
	} else if (!strcmp(profile, "alternate")) {
		temperature = -5;
		serial_motion_sync(&motion, 500);
	}
	const char *event_path = getenv("INDIGO_LACERTA_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	fault_file = getenv("INDIGO_LACERTA_FAULT");
	char port[128], command[128];
	size_t used = 0;
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_lacerta", port))) {
		return 1;
	}
	if (!headless) {
		printf("LACERTA simulator on %s\n", port);
		fflush(stdout);
	}
	while (running) {
		update_motion();
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);
		struct timeval timeout = { 0, 10000 };
		if (select(serial_fd + 1, &fds, NULL, NULL, &timeout) <= 0) {
			continue;
		}
		char byte;
		if (read(serial_fd, &byte, 1) != 1) {
			usleep(10000);
			continue;
		}
		if (byte == ':') {
			used = 0;
		}
		if (used + 1 < sizeof(command)) {
			command[used++] = byte;
		}
		if (byte == '#') {
			command[used] = 0;
			dispatch(command);
			used = 0;
		}
	}
	close(serial_fd);
	if (events) {
		fclose(events);
	}
	return 0;
}
