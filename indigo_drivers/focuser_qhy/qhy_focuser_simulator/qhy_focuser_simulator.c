// QHY Q-Focuser simulator
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

// Implements the JSON line protocol used by the QHY Q-Focuser.
// Commands are JSON objects terminated by '}', e.g. {"cmd_id":5}.
// Responses are JSON objects also terminated by '}' and carry an
// "idx" field matching the issued cmd_id.

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

// Focuser identity
const char *fw_version    = "1.2.3";
const char *board_version = "QFOCUSER-1.0";

// Motion state
int32_t position        = 50000;   // current step position
int32_t target_position = 50000;   // requested step position
int speed               = 1;       // 1 = fastest, 8 = slowest
bool reverse            = false;   // direction reversed

// Simulated environment (slowly drifts in the background thread)
double chip_temp  = 30.2;          // C
double out_temp   = 18.5;          // C (NO_TEMP_READING in driver is -50)

// The Q-Focuser is USB-powered, so c_r is always reported as 0. The driver
// reacts to that by sending a "set hold" command (cmd_id 16) at connect time.

// Simulator flags (set from command line)
bool sim_no_out_temp = false;      // -T : simulate missing outside sensor (return < -50 C)

WINDOW *top, *bottom;
pthread_mutex_t state_mutex;
pthread_mutex_t curses_mutex;

// ----------------------------------------------------------------- curses helpers

static void init_curses(void) {
	int rows, cols;
	pthread_mutex_init(&curses_mutex, NULL);
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, rows, cols);
	top = newwin(9, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, " QHY Q-Focuser simulator is running on %s ", port);
	mvwprintw(top, 8, cols - 20, " CTRL + C to exit ");
	wrefresh(top);
	bottom = newwin(rows - 9, cols, 9, 0);
	scrollok(bottom, TRUE);
	wrefresh(bottom);
}

// Background thread: simulate motion and drift sensor values
static void *background(void *arg) {
	(void)arg;
	int tick = 0;
	while (true) {
		usleep(100000); // 100 ms
		tick++;

		pthread_mutex_lock(&state_mutex);

		// Motion: take a step toward target each tick; step size shrinks with speed (1 fastest)
		if (position != target_position) {
			int step = 9 - speed; // speed 1 -> 8 steps/tick, speed 8 -> 1 step/tick
			if (step < 1) step = 1;
			int32_t delta = target_position - position;
			if (delta > 0) {
				position += (delta < step) ? delta : step;
			} else {
				position -= (-delta < step) ? -delta : step;
			}
		}

		// Sensor drift -- temperatures change ~once per second
		if (tick % 10 == 0) {
			chip_temp += (((double)random() / RAND_MAX) - 0.5) * 0.05;
			if (chip_temp < 20.0) chip_temp = 20.0;
			if (chip_temp > 50.0) chip_temp = 50.0;

			if (!sim_no_out_temp) {
				out_temp += (((double)random() / RAND_MAX) - 0.5) * 0.05;
				if (out_temp < 5.0)  out_temp = 5.0;
				if (out_temp > 35.0) out_temp = 35.0;
			}
		}

		int32_t cur_pos    = position;
		int32_t tgt_pos    = target_position;
		int     cur_speed  = speed;
		bool    cur_rev    = reverse;
		double  cur_ot     = sim_no_out_temp ? -127.0 : out_temp;
		double  cur_ct     = chip_temp;

		pthread_mutex_unlock(&state_mutex);

		pthread_mutex_lock(&curses_mutex);
		mvwprintw(top, 1, 2,
			"Firmware: %-12s   Board: %-16s",
			fw_version, board_version);
		mvwprintw(top, 2, 2,
			"Position: %8d         Target: %8d         %-7s",
			cur_pos, tgt_pos,
			(cur_pos == tgt_pos) ? "IDLE" : "MOVING");
		mvwprintw(top, 3, 2,
			"Speed:    %d (1 fastest .. 8 slowest)     Reverse: %-3s",
			cur_speed, cur_rev ? "ON" : "OFF");
		mvwprintw(top, 4, 2,
			"Chip temp:    %6.2f C",
			cur_ct);
		mvwprintw(top, 5, 2,
			"Outside temp: %6.2f C   %s",
			cur_ot, sim_no_out_temp ? "(no sensor)" : "           ");
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
	}
	return NULL;
}

// ----------------------------------------------------------------- low-level I/O

// Read a complete JSON request, framed by matching braces.
// Returns number of bytes read into buf (NUL-terminated), or -1 on error.
static int sim_read_request(int fd, char *buf, int max) {
	int len = 0;
	int depth = 0;
	bool started = false;
	while (len < max - 1) {
		char c;
		ssize_t n = read(fd, &c, 1);
		if (n == 1) {
			if (!started) {
				// Skip everything until the opening brace
				if (c != '{') continue;
				started = true;
			}
			buf[len++] = c;
			if (c == '{') depth++;
			else if (c == '}') {
				depth--;
				if (depth <= 0) break;
			}
		} else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			return -1;
		} else {
			usleep(500);
		}
	}
	buf[len] = 0;
	return len;
}

static void log_line(const char *dir, const char *s) {
	pthread_mutex_lock(&curses_mutex);
	wprintw(bottom, " %s %s\n", dir, s);
	wrefresh(bottom);
	pthread_mutex_unlock(&curses_mutex);
}

static void sim_send_response(int fd, const char *response) {
	log_line("<-", response);
	write(fd, response, strlen(response));
}

// ----------------------------------------------------------------- request parsing

// Extract an integer value for a JSON key like "cmd_id":42 from the request.
// Returns true on success and stores the parsed value in *out.
static bool json_get_int(const char *json, const char *key, int *out) {
	char pattern[64];
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	const char *p = strstr(json, pattern);
	if (!p) return false;
	p += strlen(pattern);
	while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
	if (!*p) return false;
	char *end = NULL;
	long v = strtol(p, &end, 10);
	if (end == p) return false;
	*out = (int)v;
	return true;
}

// ----------------------------------------------------------------- command handlers

static void handle_get_version(int fd) {
	char resp[160];
	snprintf(resp, sizeof(resp),
		"{\"idx\":1,\"version\":\"%s\",\"bv\":\"%s\"}",
		fw_version, board_version);
	sim_send_response(fd, resp);
}

static void handle_abort(int fd) {
	pthread_mutex_lock(&state_mutex);
	target_position = position;
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, "{\"idx\":3}");
}

static void handle_temperature_voltage(int fd) {
	pthread_mutex_lock(&state_mutex);
	// Driver divides o_t and c_t by 1000.0. c_r/10 is the voltage on a
	// power jack; this device has none, so always report 0.
	int o_t = (int)((sim_no_out_temp ? -127.0 : out_temp) * 1000.0);
	int c_t = (int)(chip_temp * 1000.0);
	pthread_mutex_unlock(&state_mutex);

	char resp[128];
	snprintf(resp, sizeof(resp),
		"{\"idx\":4,\"o_t\":%d,\"c_t\":%d,\"c_r\":0}",
		o_t, c_t);
	sim_send_response(fd, resp);
}

static void handle_get_position(int fd) {
	pthread_mutex_lock(&state_mutex);
	int32_t pos = position;
	pthread_mutex_unlock(&state_mutex);

	char resp[64];
	snprintf(resp, sizeof(resp), "{\"idx\":5,\"pos\":%d}", pos);
	sim_send_response(fd, resp);
}

static void handle_absolute_move(int fd, const char *req) {
	int tar = 0;
	if (json_get_int(req, "tar", &tar)) {
		pthread_mutex_lock(&state_mutex);
		target_position = tar;
		pthread_mutex_unlock(&state_mutex);
	}
	sim_send_response(fd, "{\"idx\":6}");
}

static void handle_set_reverse(int fd, const char *req) {
	int rev = 0;
	if (json_get_int(req, "rev", &rev)) {
		pthread_mutex_lock(&state_mutex);
		reverse = (rev != 0);
		pthread_mutex_unlock(&state_mutex);
	}
	sim_send_response(fd, "{\"idx\":7}");
}

static void handle_sync_position(int fd, const char *req) {
	int init_val = 0;
	if (json_get_int(req, "init_val", &init_val)) {
		pthread_mutex_lock(&state_mutex);
		position = init_val;
		target_position = init_val;
		pthread_mutex_unlock(&state_mutex);
	}
	sim_send_response(fd, "{\"idx\":11}");
}

static void handle_set_speed(int fd, const char *req) {
	int s = 1;
	if (json_get_int(req, "speed", &s)) {
		if (s < 1) s = 1;
		if (s > 8) s = 8;
		pthread_mutex_lock(&state_mutex);
		speed = s;
		pthread_mutex_unlock(&state_mutex);
	}
	sim_send_response(fd, "{\"idx\":13}");
}

static void handle_set_hold(int fd) {
	// ihold/irun are accepted but not simulated
	sim_send_response(fd, "{\"idx\":16}");
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char req[256];

	int opt;
	while ((opt = getopt(argc, argv, "hT")) != -1) {
		switch (opt) {
			case 'h':
				printf("QHY Q-Focuser simulator\n");
				printf("Usage: qhy_focuser_simulator [OPTIONS]\n");
				printf("  -h            Show this help and exit\n");
				printf("  -T            Simulate missing outside temperature sensor (returns -127 C)\n");
				return 0;
			case 'T':
				sim_no_out_temp = true;
				break;
			default:
				break;
		}
	}

	srandom((unsigned int)time(NULL));
	pthread_mutex_init(&state_mutex, NULL);

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
		int n = sim_read_request(fd, req, sizeof(req));
		if (n < 1) continue;

		log_line("->", req);

		int cmd_id = -1;
		if (!json_get_int(req, "cmd_id", &cmd_id)) {
			log_line("!!", "no cmd_id in request");
			continue;
		}

		switch (cmd_id) {
			case 1:  handle_get_version(fd); break;
			case 3:  handle_abort(fd); break;
			case 4:  handle_temperature_voltage(fd); break;
			case 5:  handle_get_position(fd); break;
			case 6:  handle_absolute_move(fd, req); break;
			case 7:  handle_set_reverse(fd, req); break;
			case 11: handle_sync_position(fd, req); break;
			case 13: handle_set_speed(fd, req); break;
			case 16: handle_set_hold(fd); break;
			default: {
				char msg[64];
				snprintf(msg, sizeof(msg), "unknown cmd_id %d", cmd_id);
				log_line("??", msg);
				// Respond with idx=-1 which the driver explicitly handles
				sim_send_response(fd, "{\"idx\":-1}");
				break;
			}
		}
	}

	return 0;
}
