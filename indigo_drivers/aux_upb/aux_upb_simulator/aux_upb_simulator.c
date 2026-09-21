// Pegasus ultimate powerbox host simulator
//
// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// Refactored from the Arduino sketch by Codex.
#include <stdarg.h>
#include <signal.h>
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

static struct { bool trace; } options;
static int version = 2;
static const char *bad_response;
static int outlets[8] = { 1, 1, 1, 1, 0, 0, 0, 12 };
static int usb[6] = { 1, 1, 1, 1, 1, 1 };
static int automatic, hub = 1, reverse, backlash = 100, speed = 400;
// Overcurrent flags reported as the next to last field of PA, one character per
// power outlet followed by one per heater outlet.
static char overcurrent[8] = "0000000";
static double voltage = 12.2, current = 0.0, temperature = 23.2, humidity = 59, dewpoint = 14.7;
static int power = 0;
// A box with no environmental probe attached reports zeros for the whole weather
// group, which is what a bare Ultimate Powerbox does.
static bool no_probe = false;
// Raw per outlet current reading, in the units the box uses. The real box reports
// close to zero for an unloaded outlet.
static int outlet_current = 200;
// Fault injection: the named command answers with MODE instead of its reply.
// invalid - an unparsable line, short - a truncated line, silent - no answer at
// all, close - the port is closed, which is how a transport loss looks.
static const char *fault_command, *fault_mode;
static bool fault_once;
// Matches of the faulted command that are answered normally before the fault
// applies, so a fault can be aimed at a poll rather than at the connect.
static int fault_skip;
static serial_motion motion = { .position = 50, .target = 50 };
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

// Returns true when the command was answered by an injected fault instead of by
// the protocol implementation below.
static bool inject_fault(int fd, const char *cmd) {
	if (bad_response && !strcmp(cmd, bad_response)) {
		sim_printf(fd, "invalid\n");
		return true;
	}
	if (fault_command == NULL || strcmp(cmd, fault_command)) {
		return false;
	}
	if (fault_skip > 0) {
		fault_skip--;
		return false;
	}
	if (fault_once) {
		fault_command = NULL;
	}
	if (!strcmp(fault_mode, "invalid")) {
		sim_printf(fd, "invalid\n");
	} else if (!strcmp(fault_mode, "short")) {
		sim_printf(fd, "UPB2:12.2\n");
	} else if (!strcmp(fault_mode, "close")) {
		close(fd);
		serial_fd = -1;
		running = 0;
	}
	// "silent" answers nothing at all.
	return true;
}

static void dispatch_command(int fd, const char *cmd) {
	if (inject_fault(fd, cmd)) {
		return;
	}
	long position = serial_motion_update(&motion);
	bool moving = motion.position != motion.target;
	if (!strcmp(cmd, "P#")) {
		sim_printf(fd, "%s\n", version == 2 ? "UPB2_OK" : "UPB_OK");
	} else if (!strcmp(cmd, "PV")) {
		sim_printf(fd, "1.0\n");
	} else if (!strcmp(cmd, "PA")) {
		char response[256];
		int used = snprintf(response, sizeof(response), "%s:%.1f:%.1f:%d:%.1f:%.0f:%.1f:%d%d%d%d:", version == 2 ? "UPB2" : "UPB", voltage, current, power, temperature, humidity, dewpoint, outlets[0], outlets[1], outlets[2], outlets[3]);
		if (version == 2) {
			used += snprintf(response + used, sizeof(response) - used, "%d%d%d%d%d%d:", usb[0], usb[1], usb[2], usb[3], usb[4], usb[5]);
		} else {
			used += snprintf(response + used, sizeof(response) - used, "%d:", !hub);
		}
		int heaters = version == 2 ? 3 : 2;
		for (int i = 0; i < heaters; i++) {
			used += snprintf(response + used, sizeof(response) - used, "%d:", outlets[4 + i]);
		}
		for (int i = 0; i < 4 + heaters; i++) {
			used += snprintf(response + used, sizeof(response) - used, "%d:", outlets[i] ? outlet_current : 0);
		}
		char flags[8];
		snprintf(flags, version == 2 ? 8 : 7, "%s", overcurrent);
		snprintf(response + used, sizeof(response) - used, "%s:%d\n", flags, automatic);
		serial_simulator_write_all(fd, response, strlen(response));
	} else if (!strcmp(cmd, "PC")) {
		sim_printf(fd, "2.1:12:46\n");
	} else if (!strcmp(cmd, "PS")) {
		sim_printf(fd, "PS:%d%d%d%d:%d\n", outlets[0], outlets[1], outlets[2], outlets[3], outlets[7]);
	} else if (cmd[0] == 'P' && cmd[1] >= '1' && cmd[1] <= '8' && cmd[2] == ':') {
		outlets[cmd[1] - '1'] = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (cmd[0] == 'U' && cmd[1] >= '1' && cmd[1] <= '6' && cmd[2] == ':') {
		usb[cmd[1] - '1'] = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "PE:", 3) && strlen(cmd) >= 7) {
		for (int i = 0; i < 4; i++) {
			outlets[i] = cmd[3 + i] == '1';
		}
		sim_printf(fd, "PE:1\n");
	} else if (!strncmp(cmd, "PD:", 3)) {
		automatic = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "PU:", 3)) {
		hub = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "PZ:", 3)) {
		for (int i = 0; i < 4; i++) {
			outlets[i] = cmd[3] == '1';
		}
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "PL:", 3)) {
		sim_printf(fd, "%s\n", cmd);
	} else if (!strcmp(cmd, "PF")) {
		sim_printf(fd, "RBT\n");
	} else if (!strcmp(cmd, "SA")) {
		sim_printf(fd, "%ld:%d:%d:%d\n", position, moving, reverse, backlash);
	} else if (!strcmp(cmd, "SP")) {
		sim_printf(fd, "%ld\n", position);
	} else if (!strcmp(cmd, "SI")) {
		sim_printf(fd, "%d\n", moving);
	} else if (!strcmp(cmd, "ST")) {
		sim_printf(fd, "22.4\n");
	} else if (!strcmp(cmd, "SH")) {
		serial_motion_stop(&motion);
		sim_printf(fd, "H:1\n");
	} else if (!strcmp(cmd, "SS")) {
		sim_printf(fd, "%d\n", speed);
	} else if (!strncmp(cmd, "SM:", 3) || !strncmp(cmd, "SG:", 3)) {
		serial_motion_start(&motion, atoi(cmd + 3) + (cmd[1] == 'G' ? position : 0), speed);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "SC:", 3)) {
		serial_motion_sync(&motion, atoi(cmd + 3));
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "SB:", 3)) {
		backlash = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "SR:", 3)) {
		reverse = atoi(cmd + 3);
		sim_printf(fd, "%s\n", cmd);
	} else if (!strncmp(cmd, "SS:", 3)) {
		speed = atoi(cmd + 3);
		sim_printf(fd, "%d\n", speed);
	}
}

int main(int argc, char **argv) {
	const char *ready_file = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--ready-file") && i + 1 < argc) {
			ready_file = argv[++i];
		} else if (!strcmp(argv[i], "--model") && i + 1 < argc) {
			const char *model = argv[++i];
			if (strcmp(model, "upb") && strcmp(model, "upb2")) {
				return 1;
			}
			version = !strcmp(model, "upb") ? 1 : 2;
		} else if (!strcmp(argv[i], "--bad-response") && i + 1 < argc) {
			bad_response = argv[++i];
		} else if (!strcmp(argv[i], "--overcurrent") && i + 1 < argc) {
			snprintf(overcurrent, sizeof(overcurrent), "%s", argv[++i]);
		} else if (!strcmp(argv[i], "--weather") && i + 3 < argc) {
			temperature = atof(argv[++i]);
			humidity = atof(argv[++i]);
			dewpoint = atof(argv[++i]);
		} else if (!strcmp(argv[i], "--power") && i + 3 < argc) {
			voltage = atof(argv[++i]);
			current = atof(argv[++i]);
			power = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "--no-probe")) {
			no_probe = true;
			temperature = humidity = dewpoint = 0;
		} else if (!strcmp(argv[i], "--outlet-current") && i + 1 < argc) {
			outlet_current = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "--autodew")) {
			automatic = 1;
		} else if (!strcmp(argv[i], "--fault-after") && i + 3 < argc) {
			fault_skip = atoi(argv[++i]);
			fault_once = true;
			fault_command = argv[++i];
			fault_mode = argv[++i];
			if (strcmp(fault_mode, "invalid") && strcmp(fault_mode, "short") && strcmp(fault_mode, "silent") && strcmp(fault_mode, "close")) {
				return 1;
			}
		} else if ((!strcmp(argv[i], "--fault") || !strcmp(argv[i], "--fault-once")) && i + 2 < argc) {
			fault_once = !strcmp(argv[i], "--fault-once");
			fault_command = argv[++i];
			fault_mode = argv[++i];
			if (strcmp(fault_mode, "invalid") && strcmp(fault_mode, "short") && strcmp(fault_mode, "silent") && strcmp(fault_mode, "close")) {
				return 1;
			}
		} else if (!strcmp(argv[i], "--trace")) {
			options.trace = true;
		} else if (strcmp(argv[i], "--headless")) {
			fprintf(stderr, "Usage: %s [--headless] [--trace] [--ready-file path] [--model upb|upb2] [--overcurrent FLAGS] [--weather T H D] [--power V A W] [--autodew] [--no-probe] [--outlet-current N] [--fault CMD invalid|short|silent|close] [--fault-once CMD MODE] [--fault-after N CMD MODE]\n", argv[0]);
			return 1;
		}
	}
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	char port[128], command[128];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}
	if (ready_file && !serial_simulator_write_ready_file(ready_file, "aux_upb", port)) {
		close(serial_fd);
		return 1;
	}
	while (running) {
		if (sim_read_command(serial_fd, command, sizeof(command)) > 0) {
			dispatch_command(serial_fd, command);
		}
	}
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	return 0;
}
