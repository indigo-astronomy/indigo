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

// Implements the binary framed protocol used by the SVBONY PowerBox (SV241 Pro).
// See PROTOCOL.md in the parent directory for full protocol description.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <curses.h>
#include <getopt.h>
#include <time.h>

// ----------------------------------------------------------------- state

char port[128];

// DC output states (ON=1 / OFF=0)
int dc[5]    = { 1, 1, 1, 1, 1 };

// USB group states (ON=1 / OFF=0): [0]=USB-C/1/2  [1]=USB-3/4/5
int usb[2]   = { 1, 1 };

// Regulated output PWM (0–255; 0 = off)
int reg_pwm  = 0xC2;  // ≈ 11.4 V at startup

// PWM dew heater duty (0–255)
int pwm[2]   = { 0, 0 };

// Simulated sensor values (drift slightly in the background thread)
double voltage   = 12.1;   // V
double current   = 2000.0; // mA  (INA219 raw unit)
double power_mw  = 0.0;    // mW  (computed from voltage × current)
double ds18b20   = 19.7;   // °C
double sht40_t   = 22.3;   // °C
double sht40_h   = 48.5;   // %RH
// Simulator flags (can be set from command line)
bool sim_no_ds18b20 = false; // -T : simulate missing DS18B20 (return -127 C)
bool sim_no_sht40   = false; // -H : simulate missing SHT40 (temp -2 C, random RH)
// Persistent simulated SHT40 humidity when -H is used (generated once)
double sim_sht40_h_random = -1.0;

WINDOW *top, *bottom;
pthread_mutex_t curses_mutex;

// ----------------------------------------------------------------- curses helpers

void init_curses(void) {
	int rows, cols;
	pthread_mutex_init(&curses_mutex, NULL);
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, rows, cols);
	top = newwin(9, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, " SVBONY PowerBox simulator is running on %s ", port);
	mvwprintw(top, 8, cols - 20, " CTRL + C to exit ");
	wrefresh(top);
	bottom = newwin(rows - 9, cols, 9, 0);
	scrollok(bottom, TRUE);
	wrefresh(bottom);
}

// Background thread: slowly drift sensor values and refresh the display panel
void *background(void *arg) {
	(void)arg;
	while (true) {
		usleep(500000); // 0.5 s

		// Gentle sensor drift
		voltage  += (((double)random() / RAND_MAX) - 0.5) * 0.02;
		if (voltage < 11.8) voltage = 11.8;
		if (voltage > 12.4) voltage = 12.4;

		current  += (((double)random() / RAND_MAX) - 0.5) * 20.0;
		if (current < 500.0)  current = 500.0;
		if (current > 5000.0) current = 5000.0;

		power_mw = voltage * current; // mW

		if (!sim_no_ds18b20) {
			ds18b20 += (((double)random() / RAND_MAX) - 0.5) * 0.1;
			if (ds18b20 < 10.0) ds18b20 = 10.0;
			if (ds18b20 > 35.0) ds18b20 = 35.0;
		} else {
			ds18b20 = -127.0;
		}

		if (!sim_no_sht40) {
			sht40_t += (((double)random() / RAND_MAX) - 0.5) * 0.1;
			if (sht40_t < 10.0) sht40_t = 10.0;
			if (sht40_t > 35.0) sht40_t = 35.0;

			sht40_h += (((double)random() / RAND_MAX) - 0.5) * 0.2;
			if (sht40_h < 20.0) sht40_h = 20.0;
			if (sht40_h > 90.0) sht40_h = 90.0;
		} else {
			sht40_t = -2.0;
			/* when simulating missing SHT40, use the persistent random humidity */
			if (sim_sht40_h_random < 0.0)
				sim_sht40_h_random = (double)(random() % 10000000);
			sht40_h = sim_sht40_h_random;
		}

		pthread_mutex_lock(&curses_mutex);
		mvwprintw(top, 1, 2,
			"DC1: %-3s  DC2: %-3s  DC3: %-3s  DC4: %-3s  DC5: %-3s",
			dc[0] ? "ON" : "OFF",
			dc[1] ? "ON" : "OFF",
			dc[2] ? "ON" : "OFF",
			dc[3] ? "ON" : "OFF",
			dc[4] ? "ON" : "OFF");
		mvwprintw(top, 2, 2,
			"Regulated: %-3s  pwm=%3d  %.1f V",
			reg_pwm ? "ON" : "OFF", reg_pwm, reg_pwm * 15.3 / 255.0);
		mvwprintw(top, 3, 2,
			"USB-A (C/1/2): %-3s    USB-B (3/4/5): %-3s",
			usb[0] ? "ON" : "OFF",
			usb[1] ? "ON" : "OFF");
		mvwprintw(top, 4, 2,
			"Heater PWM-A: %-3s  pwm=%3d  %5.1f%%    Heater PWM-B: %-3s  pwm=%3d  %5.1f%%",
			pwm[0] ? "ON" : "OFF", pwm[0], pwm[0] * 100.0 / 255.0,
			pwm[1] ? "ON" : "OFF", pwm[1], pwm[1] * 100.0 / 255.0);
		mvwprintw(top, 5, 2,
			"Voltage: %6.2f V     Current: %7.1f mA     Power: %8.1f mW",
			voltage, current, power_mw);
		mvwprintw(top, 6, 2,
			"DS18B20: %6.2f C     SHT40 temp: %5.1f C     SHT40 humidity: %5.1f %%",
			ds18b20, sht40_t, sht40_h);
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
	}
	return NULL;
}

// ----------------------------------------------------------------- low-level I/O

// Read a single byte from a (non-blocking) fd, spinning until one is available
static int sim_read_byte(int fd, unsigned char *b) {
	while (true) {
		ssize_t n = read(fd, b, 1);
		if (n == 1) return 0;
		if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return -1;
		usleep(500);
	}
}

// Log a hex dump of a frame to the scrolling window
static void log_frame(const char *dir, const unsigned char *frame, int len) {
	char hex[64] = {0};
	int pos = 0;
	for (int i = 0; i < len && pos < (int)sizeof(hex) - 4; i++) {
		pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", frame[i]);
	}
	pthread_mutex_lock(&curses_mutex);
	wprintw(bottom, " %s [%s]\n", dir, hex);
	wrefresh(bottom);
	pthread_mutex_unlock(&curses_mutex);
}

// Read a complete binary request frame; fills cmd[] and returns cmd_len, or -1 on error
static int sim_read_frame(int fd, unsigned char *cmd) {
	unsigned char b;

	// Sync to frame header
	do {
		if (sim_read_byte(fd, &b) < 0) return -1;
	} while (b != 0x24);

	// data_len: total bytes in frame (header + data_len + cmd_bytes + checksum)
	unsigned char data_len;
	if (sim_read_byte(fd, &data_len) < 0) return -1;

	// remaining = cmd bytes + checksum  (data_len - 2 bytes already consumed)
	int remaining = data_len - 2;
	if (remaining < 0 || remaining > 10) return -1;

	unsigned char tail[10];
	for (int i = 0; i < remaining; i++) {
		if (sim_read_byte(fd, &tail[i]) < 0) return -1;
	}

	// Verify checksum: sum of (0x24 + data_len + cmd_bytes) % 0xFF
	unsigned int sum = 0x24 + data_len;
	int cmd_len = remaining - 1; // last byte is checksum
	for (int i = 0; i < cmd_len; i++) {
		sum += tail[i];
		cmd[i] = tail[i];
	}
	unsigned char expected_chk = (unsigned char)(sum % 0xFF);
	unsigned char recv_chk = tail[cmd_len];

	// Log the received frame
	unsigned char full_rx[16];
	full_rx[0] = 0x24;
	full_rx[1] = data_len;
	memcpy(full_rx + 2, tail, remaining);
	log_frame("->", full_rx, 2 + remaining);

	if (expected_chk != recv_chk) {
		pthread_mutex_lock(&curses_mutex);
		wprintw(bottom, " !! checksum error (got %02X expected %02X)\n", recv_chk, expected_chk);
		wrefresh(bottom);
		pthread_mutex_unlock(&curses_mutex);
		return -1;
	}

	return cmd_len;
}

// Build and send a response frame
static void sim_send_response(int fd, unsigned char cmd_echo, const unsigned char *res, int res_len) {
	unsigned char frame[20];
	int full_len = 3 + res_len + 1; // header + data_len + echo + res + checksum

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
	write(fd, frame, full_len);
}

// ----------------------------------------------------------------- encoding helpers

// Encode a 32-bit big-endian value (raw = physical_value * scale) into 4 bytes
static void encode_u32(unsigned char *out, double physical, double scale) {
	uint32_t raw = (uint32_t)(physical * scale);
	out[0] = (raw >> 24) & 0xFF;
	out[1] = (raw >> 16) & 0xFF;
	out[2] = (raw >>  8) & 0xFF;
	out[3] =  raw        & 0xFF;
}

// ----------------------------------------------------------------- command handlers

// 0x01  Set port state / duty cycle
static void handle_set_port(int fd, const unsigned char *cmd) {
	uint8_t port_idx = cmd[1];
	uint8_t value    = cmd[2];

	if (port_idx <= 4) {
		dc[port_idx] = (value != 0) ? 1 : 0;
	} else if (port_idx == 5) {
		usb[0] = (value != 0) ? 1 : 0;
	} else if (port_idx == 6) {
		usb[1] = (value != 0) ? 1 : 0;
	} else if (port_idx == 7) {
		reg_pwm = value;
	} else if (port_idx == 8) {
		pwm[0] = value;
	} else if (port_idx == 9) {
		pwm[1] = value;
	}

	unsigned char res[2] = { 0x00, 0x00 };
	sim_send_response(fd, 0x01, res, 2);
}

// 0x02  Read INA219 power (mW × 100 as uint32)
static void handle_read_power(int fd) {
	unsigned char res[4];
	encode_u32(res, power_mw, 100.0);
	sim_send_response(fd, 0x02, res, 4);
}

// 0x03  Read INA219 load voltage (V × 100 as uint32)
static void handle_read_voltage(int fd) {
	unsigned char res[4];
	encode_u32(res, voltage, 100.0);
	sim_send_response(fd, 0x03, res, 4);
}

// 0x04  Read DS18B20 temperature
//   raw/100 - 255.5 = temp_C  =>  raw = (temp_C + 255.5) * 100
static void handle_read_ds18b20(int fd) {
	unsigned char res[4];
	double temp = sim_no_ds18b20 ? -127.0 : ds18b20;
	encode_u32(res, temp + 255.5, 100.0);
	sim_send_response(fd, 0x04, res, 4);
}

// 0x05  Read SHT40 temperature
//   raw/100 - 254 = temp_C  =>  raw = (temp_C + 254) * 100
static void handle_read_sht40_temp(int fd) {
	unsigned char res[4];
	double temp = sim_no_sht40 ? -2.0 : sht40_t;
	encode_u32(res, temp + 254.0, 100.0);
	sim_send_response(fd, 0x05, res, 4);
}

// 0x06  Read SHT40 humidity
//   same encoding as temperature: raw = (humi + 254) * 100
static void handle_read_sht40_humi(int fd) {
	unsigned char res[4];
	double hum = sim_no_sht40 ? sim_sht40_h_random : sht40_h;
	encode_u32(res, hum + 254.0, 100.0);
	sim_send_response(fd, 0x06, res, 4);
}

// 0x07  Read INA219 current (mA × 100 as uint32)
static void handle_read_current(int fd) {
	unsigned char res[4];
	encode_u32(res, current, 100.0);
	sim_send_response(fd, 0x07, res, 4);
}

// 0x08  Read all port states (10 bytes)
//   [dc1, dc2, dc3, dc4, dc5, usb_a, usb_b, reg_pwm, pwm_a, pwm_b]
static void handle_read_state(int fd) {
	unsigned char res[10];
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
	sim_send_response(fd, 0x08, res, 10);
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	unsigned char cmd[10];

	// parse command-line options: -T (no DS18B20), -H (no SHT40), -h help
	int opt;
	while ((opt = getopt(argc, argv, "hHT")) != -1) {
		switch (opt) {
			case 'h':
				printf("SVB PowerBox simulator\n");
				printf("Usage: svbpowerbox_simulator [OPTIONS]\n");
				printf("  -h            Show this help and exit\n");
				printf("  -T            Simulate missing DS18B20 (returns -127 C)\n");
				printf("  -H            Simulate missing SHT40 (temp -2 C, random RH)\n");
				return 0;
			case 'T':
				sim_no_ds18b20 = true;
				break;
			case 'H':
				sim_no_sht40 = true;
				break;
			default:
				break;
		}
	}

	// seed random generator
	srandom((unsigned int)time(NULL));

	// if simulating missing SHT40, generate one persistent random RH value
	if (sim_no_sht40) {
		sim_sht40_h_random = (double)(random() % 10000000);
	}

	int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("open /dev/ptmx");
		return 1;
	}
	grantpt(fd);
	unlockpt(fd);
	ptsname_r(fd, port, sizeof(port));

	init_curses();
	pthread_create(&thread, NULL, background, NULL);

	while (true) {
		int cmd_len = sim_read_frame(fd, cmd);
		if (cmd_len < 1) continue;

		switch (cmd[0]) {
			case 0x01:
				if (cmd_len == 3) handle_set_port(fd, cmd);
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
			default: {
				// Unknown command — respond with 0xAA (failure echo)
				pthread_mutex_lock(&curses_mutex);
				wprintw(bottom, " ?? unknown cmd %02X\n", cmd[0]);
				wrefresh(bottom);
				pthread_mutex_unlock(&curses_mutex);
				sim_send_response(fd, 0xAA, NULL, 0);
				break;
			}
		}
	}

	return 0;
}
