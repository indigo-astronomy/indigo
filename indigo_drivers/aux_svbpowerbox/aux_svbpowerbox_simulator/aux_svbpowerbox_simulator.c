// SVBONY PowerBox simulator
//
// Copyright (c) 2026 Rumen G. Bogdanovski
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

// Implements the binary framed protocol used by the SVBONY PowerBox (SV241 Pro).
// See PROTOCOL.md in the parent directory for full protocol description.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	bool no_ds18b20;
	bool no_sht40;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.no_ds18b20 = false,
	.no_sht40 = false
};

static const char *simulator_name = "svbpowerbox";

static void usage(const char *name) {
	printf("SVBONY PowerBox simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable interactive output suitable for terminals\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --model <svbpowerbox>   Select simulated model, default is svbpowerbox\n");
	printf("  -T, --no-ds18b20        Simulate missing DS18B20 (returns -127 C)\n");
	printf("  -H, --no-sht40          Simulate missing SHT40 (temp -2 C, random RH)\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int dc[5] = { 1, 1, 1, 1, 1 };
static int usb[2] = { 1, 1 };
static int reg_pwm = 0xC2;
static int pwm[2] = { 0, 0 };

static double voltage = 12.1;
static double current = 2000.0;
static double power_mw = 0.0;
static double ds18b20 = 19.7;
static double sht40_t = 22.3;
static double sht40_h = 48.5;
static double sht40_h_random = -1.0;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

static void clamp_double(double *value, double min, double max) {
	if (*value < min) {
		*value = min;
	} else if (*value > max) {
		*value = max;
	}
}

static void *background(void *arg) {
	(void)arg;
	while (running) {
		pthread_mutex_lock(&state_mutex);
		voltage += (((double)random() / RAND_MAX) - 0.5) * 0.02;
		clamp_double(&voltage, 11.8, 12.4);

		current += (((double)random() / RAND_MAX) - 0.5) * 20.0;
		clamp_double(&current, 500.0, 5000.0);

		power_mw = voltage * current;

		if (!options.no_ds18b20) {
			ds18b20 += (((double)random() / RAND_MAX) - 0.5) * 0.1;
			clamp_double(&ds18b20, 10.0, 35.0);
		} else {
			ds18b20 = -127.0;
		}

		if (!options.no_sht40) {
			sht40_t += (((double)random() / RAND_MAX) - 0.5) * 0.1;
			clamp_double(&sht40_t, 10.0, 35.0);

			sht40_h += (((double)random() / RAND_MAX) - 0.5) * 0.2;
			clamp_double(&sht40_h, 20.0, 90.0);
		} else {
			sht40_t = -2.0;
			if (sht40_h_random < 0.0) {
				sht40_h_random = (double)(random() % 10000000);
			}
			sht40_h = sht40_h_random;
		}
		pthread_mutex_unlock(&state_mutex);

		usleep(500000);
	}
	return NULL;
}

// ----------------------------------------------------------------- runtime

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
				fprintf(stderr, "--model requires svbpowerbox\n");
				return false;
			}
			if (strcmp(argv[i], "svbpowerbox") && strcmp(argv[i], "sv241") && strcmp(argv[i], "sv241-pro")) {
				fprintf(stderr, "Unknown model '%s'\n", argv[i]);
				return false;
			}
		} else if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "--no-ds18b20")) {
			options.no_ds18b20 = true;
		} else if (!strcmp(argv[i], "-H") || !strcmp(argv[i], "--no-sht40")) {
			options.no_sht40 = true;
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- protocol

static int sim_read_byte(int fd, unsigned char *byte) {
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

static void log_frame(const char *prefix, const unsigned char *frame, int length) {
	if (!options.trace) {
		return;
	}

	char hex[96] = { 0 };
	int pos = 0;
	for (int i = 0; i < length && pos < (int)sizeof(hex) - 4; i++) {
		pos += snprintf(hex + pos, sizeof(hex) - (size_t)pos, "%02X ", frame[i]);
	}
	serial_simulator_trace_line(options.trace, prefix, hex);
}

static int sim_read_frame(int fd, unsigned char *cmd) {
	unsigned char byte;

	do {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
	} while (byte != 0x24);

	unsigned char data_len;
	if (sim_read_byte(fd, &data_len) < 0) {
		return -1;
	}

	int remaining = data_len - 2;
	if (remaining < 1 || remaining > 10) {
		return -1;
	}

	unsigned char tail[10];
	for (int i = 0; i < remaining; i++) {
		if (sim_read_byte(fd, &tail[i]) < 0) {
			return -1;
		}
	}

	unsigned int sum = 0x24 + data_len;
	int cmd_len = remaining - 1;
	for (int i = 0; i < cmd_len; i++) {
		sum += tail[i];
		cmd[i] = tail[i];
	}
	unsigned char expected_checksum = (unsigned char)(sum % 0xFF);
	unsigned char received_checksum = tail[cmd_len];

	unsigned char full_rx[16];
	full_rx[0] = 0x24;
	full_rx[1] = data_len;
	memcpy(full_rx + 2, tail, (size_t)remaining);
	log_frame("->", full_rx, 2 + remaining);

	if (expected_checksum != received_checksum) {
		serial_simulator_trace_line(options.trace, "!!", "checksum error");
		return -1;
	}

	return cmd_len;
}

static bool sim_send_response(int fd, unsigned char cmd_echo, const unsigned char *res, int res_len) {
	unsigned char frame[20];
	int full_len = 3 + res_len + 1;

	frame[0] = 0x24;
	frame[1] = (unsigned char)full_len;
	frame[2] = cmd_echo;
	for (int i = 0; i < res_len; i++) {
		frame[3 + i] = res[i];
	}

	unsigned int sum = 0;
	for (int i = 0; i < full_len - 1; i++) {
		sum += frame[i];
	}
	frame[full_len - 1] = (unsigned char)(sum % 0xFF);

	log_frame("<-", frame, full_len);
	return serial_simulator_write_all(fd, (const char *)frame, (size_t)full_len);
}

// ----------------------------------------------------------------- encoding helpers

static void encode_u32(unsigned char *out, double physical, double scale) {
	uint32_t raw = (uint32_t)(physical * scale);
	out[0] = (raw >> 24) & 0xFF;
	out[1] = (raw >> 16) & 0xFF;
	out[2] = (raw >> 8) & 0xFF;
	out[3] = raw & 0xFF;
}

// ----------------------------------------------------------------- command handlers

static void handle_set_port(int fd, const unsigned char *cmd) {
	uint8_t port_idx = cmd[1];
	uint8_t value = cmd[2];

	pthread_mutex_lock(&state_mutex);
	if (port_idx <= 4) {
		dc[port_idx] = value != 0 ? 1 : 0;
	} else if (port_idx == 5) {
		usb[0] = value != 0 ? 1 : 0;
	} else if (port_idx == 6) {
		usb[1] = value != 0 ? 1 : 0;
	} else if (port_idx == 7) {
		reg_pwm = value;
	} else if (port_idx == 8) {
		pwm[0] = value;
	} else if (port_idx == 9) {
		pwm[1] = value;
	}
	pthread_mutex_unlock(&state_mutex);

	unsigned char res[2] = { 0x00, 0x00 };
	sim_send_response(fd, 0x01, res, 2);
}

static void handle_read_power(int fd) {
	unsigned char res[4];
	pthread_mutex_lock(&state_mutex);
	encode_u32(res, power_mw, 100.0);
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x02, res, 4);
}

static void handle_read_voltage(int fd) {
	unsigned char res[4];
	pthread_mutex_lock(&state_mutex);
	encode_u32(res, voltage, 100.0);
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x03, res, 4);
}

static void handle_read_ds18b20(int fd) {
	unsigned char res[4];
	pthread_mutex_lock(&state_mutex);
	double temperature = options.no_ds18b20 ? -127.0 : ds18b20;
	encode_u32(res, temperature + 255.5, 100.0);
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x04, res, 4);
}

static void handle_read_sht40_temp(int fd) {
	unsigned char res[4];
	pthread_mutex_lock(&state_mutex);
	double temperature = options.no_sht40 ? -2.0 : sht40_t;
	encode_u32(res, temperature + 254.0, 100.0);
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x05, res, 4);
}

static void handle_read_sht40_humi(int fd) {
	unsigned char res[4];
	pthread_mutex_lock(&state_mutex);
	double humidity = options.no_sht40 ? sht40_h_random : sht40_h;
	encode_u32(res, humidity + 254.0, 100.0);
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x06, res, 4);
}

static void handle_read_current(int fd) {
	unsigned char res[4];
	pthread_mutex_lock(&state_mutex);
	encode_u32(res, current, 100.0);
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x07, res, 4);
}

static void handle_read_state(int fd) {
	unsigned char res[10];
	pthread_mutex_lock(&state_mutex);
	res[0] = dc[0] ? 0xFF : 0x00;
	res[1] = dc[1] ? 0xFF : 0x00;
	res[2] = dc[2] ? 0xFF : 0x00;
	res[3] = dc[3] ? 0xFF : 0x00;
	res[4] = dc[4] ? 0xFF : 0x00;
	res[5] = usb[0] ? 0xFF : 0x00;
	res[6] = usb[1] ? 0xFF : 0x00;
	res[7] = (unsigned char)reg_pwm;
	res[8] = (unsigned char)pwm[0];
	res[9] = (unsigned char)pwm[1];
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, 0x08, res, 10);
}

static void dispatch_command(int fd, const unsigned char *cmd, int cmd_len) {
	switch (cmd[0]) {
		case 0x01:
			if (cmd_len == 3) {
				handle_set_port(fd, cmd);
			}
			break;
		case 0x02:
			handle_read_power(fd);
			break;
		case 0x03:
			handle_read_voltage(fd);
			break;
		case 0x04:
			handle_read_ds18b20(fd);
			break;
		case 0x05:
			handle_read_sht40_temp(fd);
			break;
		case 0x06:
			handle_read_sht40_humi(fd);
			break;
		case 0x07:
			handle_read_current(fd);
			break;
		case 0x08:
			handle_read_state(fd);
			break;
		default:
			serial_simulator_trace_line(options.trace, "??", "unknown command");
			sim_send_response(fd, 0xAA, NULL, 0);
			break;
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	unsigned char cmd[10];
	char port[128];

	if (!parse_args(argc, argv)) {
		usage(argv[0]);
		return 1;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	srandom((unsigned int)time(NULL));
	if (options.no_sht40) {
		sht40_h_random = (double)(random() % 10000000);
	}

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
		printf("SVBONY PowerBox simulator is running on %s\n", port);
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, background, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	while (running) {
		int cmd_len = sim_read_frame(serial_fd, cmd);
		if (cmd_len > 0) {
			dispatch_command(serial_fd, cmd, cmd_len);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	pthread_join(thread, NULL);
	return 0;
}
