// Askar-WAF USB CDC focuser simulator
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

// Implements the F...# serial command set documented in
// Commands_Focuser_CDC_EN.md. Commands start with 'F' and end with '#';
// responses are payload + '#' + "\r\n". Frames longer than 31 chars are
// rejected with "FE#\r\n".

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <curses.h>
#include <getopt.h>
#include <time.h>

// ----------------------------------------------------------------- state

#define ASKAR_FRAME_MAX        31     // protocol guarantee, longer -> FE#
#define ASKAR_MAX_TRAVEL_MIN   100
#define ASKAR_MAX_TRAVEL_MAX   1000000
#define ASKAR_BACKLASH_MAX     10000

static char port[128];

static const char *fw_version = "1.1.0";
static const char *model_name = "Askar-WAF";

static int32_t position        = 50000;
static int32_t target_position = 50000;
static int32_t max_step        = 100000;
static int     backlash        = 0;
static int     reverse         = 0;     // 0 = normal, 1 = reversed motor direction

static WINDOW *top, *bottom;
static pthread_mutex_t state_mutex;
static pthread_mutex_t curses_mutex;

// ----------------------------------------------------------------- curses helpers

static void init_curses(void) {
	int rows, cols;
	pthread_mutex_init(&curses_mutex, NULL);
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, rows, cols);
	top = newwin(8, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, " Askar-WAF focuser simulator is running on %s ", port);
	mvwprintw(top, 7, cols - 20, " CTRL + C to exit ");
	wrefresh(top);
	bottom = newwin(rows - 8, cols, 8, 0);
	scrollok(bottom, TRUE);
	wrefresh(bottom);
}

// Background thread: walk position toward target.
static void *background(void *arg) {
	(void)arg;
	while (true) {
		usleep(50000); // 20 Hz

		pthread_mutex_lock(&state_mutex);
		if (position != target_position) {
			int32_t delta = target_position - position;
			int32_t step = 200; // ~4000 steps/sec — typical for the WAF
			if (delta > 0) {
				position += (delta < step) ? delta : step;
			} else {
				position -= (-delta < step) ? -delta : step;
			}
		}
		int32_t cur_pos    = position;
		int32_t tgt_pos    = target_position;
		int32_t cur_max    = max_step;
		int     cur_back   = backlash;
		int     cur_rev    = reverse;
		pthread_mutex_unlock(&state_mutex);

		pthread_mutex_lock(&curses_mutex);
		mvwprintw(top, 1, 2,
			"Model: %-12s   Firmware: %-12s",
			model_name, fw_version);
		mvwprintw(top, 2, 2,
			"Position: %8d   Target: %8d   %-7s",
			cur_pos, tgt_pos,
			(cur_pos == tgt_pos) ? "IDLE" : "MOVING");
		mvwprintw(top, 3, 2,
			"Max step: %8d   Backlash offset: %5d   Reverse: %s",
			cur_max, cur_back, cur_rev ? "ON " : "OFF");
		mvwprintw(top, 4, 2,
			"Range:    [0 .. %d]   Backlash range: [0 .. %d]",
			cur_max, ASKAR_BACKLASH_MAX);
		mvwprintw(top, 5, 2,
			"Set-max range: [%d .. %d] (FMn#)",
			ASKAR_MAX_TRAVEL_MIN, ASKAR_MAX_TRAVEL_MAX);
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
	}
	return NULL;
}

// ----------------------------------------------------------------- low-level I/O

static void log_line(const char *dir, const char *s) {
	pthread_mutex_lock(&curses_mutex);
	wprintw(bottom, " %s %s\n", dir, s);
	wrefresh(bottom);
	pthread_mutex_unlock(&curses_mutex);
}

// Send "payload#\r\n" and log "<- payload#".
static void sim_send_response(int fd, const char *payload) {
	char frame[128];
	snprintf(frame, sizeof(frame), "%s#\r\n", payload);
	char log_buf[128];
	snprintf(log_buf, sizeof(log_buf), "%s#", payload);
	log_line("<-", log_buf);
	write(fd, frame, strlen(frame));
}

static void sim_send_error(int fd) {
	sim_send_response(fd, "FE");
}

// Read one frame: starts at 'F', ends at '#'. Strips trailing CR/LF before 'F'.
// Returns the payload length (without '#'), or -1 if frame exceeds the protocol
// limit (caller should send FE# and resync).
static int sim_read_frame(int fd, char *buf, int max) {
	int len = 0;
	bool started = false;
	while (len < max - 1) {
		char c;
		ssize_t n = read(fd, &c, 1);
		if (n == 1) {
			if (!started) {
				// Skip CR/LF/garbage until we find an 'F' that opens a frame.
				if (c != 'F') continue;
				started = true;
				buf[len++] = c;
				continue;
			}
			if (c == '#') {
				buf[len] = 0;
				return len;
			}
			if (c == '\r' || c == '\n') {
				// Frame was never terminated; abandon and resync.
				buf[len] = 0;
				return -2;
			}
			buf[len++] = c;
			if (len > ASKAR_FRAME_MAX) {
				// Drain the rest of this oversized frame so the next 'F' is fresh.
				while (read(fd, &c, 1) == 1 && c != '#') {
					if (c == 'F') {
						buf[0] = 'F';
						len = 1;
						break;
					}
				}
				return -1;
			}
		} else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			return -3;
		} else {
			usleep(500);
		}
	}
	return -1;
}

// ----------------------------------------------------------------- payload parsing

// Parse a signed decimal integer from `s` (which must point at the digits or a
// sign). Returns true on success and writes the value to *out; returns false
// for an empty/non-numeric body.
static bool parse_int(const char *s, int32_t *out) {
	if (!*s) return false;
	char *end = NULL;
	long v = strtol(s, &end, 10);
	if (end == s) return false;
	while (*end) {
		// Trailing junk inside the frame -> reject.
		if (!isspace((unsigned char)*end)) return false;
		end++;
	}
	*out = (int32_t)v;
	return true;
}

static int32_t clamp(int32_t v, int32_t lo, int32_t hi) {
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

// ----------------------------------------------------------------- command handlers

// FPn# -> absolute move (clamped to [0, max_step])
static void handle_absolute_move(int fd, const char *body) {
	int32_t n;
	if (!parse_int(body, &n)) {
		sim_send_error(fd);
		return;
	}
	pthread_mutex_lock(&state_mutex);
	target_position = clamp(n, 0, max_step);
	pthread_mutex_unlock(&state_mutex);

	char resp[32];
	snprintf(resp, sizeof(resp), "FP%d", n); // echo what the host sent
	sim_send_response(fd, resp);
}

// FT+n# / FT-n# -> relative move
static void handle_relative_move(int fd, const char *body) {
	if (*body != '+' && *body != '-') {
		sim_send_error(fd);
		return;
	}
	int32_t delta;
	if (!parse_int(body, &delta)) {
		sim_send_error(fd);
		return;
	}
	pthread_mutex_lock(&state_mutex);
	target_position = clamp(position + delta, 0, max_step);
	pthread_mutex_unlock(&state_mutex);

	char resp[32];
	if (delta >= 0) {
		snprintf(resp, sizeof(resp), "FT+%d", delta);
	} else {
		snprintf(resp, sizeof(resp), "FT%d", delta); // already has '-'
	}
	sim_send_response(fd, resp);
}

// FS# -> halt
static void handle_halt(int fd) {
	pthread_mutex_lock(&state_mutex);
	target_position = position;
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, "FS");
}

// FYn# -> sync logical position (no motion)
static void handle_sync(int fd, const char *body) {
	int32_t n;
	if (!parse_int(body, &n)) {
		sim_send_error(fd);
		return;
	}
	pthread_mutex_lock(&state_mutex);
	int32_t clamped = clamp(n, 0, max_step);
	position = clamped;
	target_position = clamped;
	pthread_mutex_unlock(&state_mutex);

	char resp[32];
	snprintf(resp, sizeof(resp), "FY%d", clamped);
	sim_send_response(fd, resp);
}

// Fp# -> read position (note: lowercase p)
static void handle_read_position(int fd) {
	pthread_mutex_lock(&state_mutex);
	int32_t cur = position;
	pthread_mutex_unlock(&state_mutex);

	char resp[32];
	snprintf(resp, sizeof(resp), "Fp%d", cur);
	sim_send_response(fd, resp);
}

// FQ# -> motion state
static void handle_read_motion_state(int fd) {
	pthread_mutex_lock(&state_mutex);
	bool moving = position != target_position;
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, moving ? "FQ1" : "FQ0");
}

// Fm# -> read max travel (lowercase m, canonical). FM# is accepted as an alias.
static void handle_read_max(int fd) {
	pthread_mutex_lock(&state_mutex);
	int32_t m = max_step;
	pthread_mutex_unlock(&state_mutex);
	char resp[32];
	snprintf(resp, sizeof(resp), "Fm%d", m);
	sim_send_response(fd, resp);
}

// FMn# (canonical) / FXn# (legacy alias) -> set max travel (must be in
// [100, 1000000]). `echo` is the command letter to mirror in the reply.
static void handle_set_max(int fd, const char *body, char echo) {
	int32_t n;
	if (!parse_int(body, &n)) {
		sim_send_error(fd);
		return;
	}
	if (n < ASKAR_MAX_TRAVEL_MIN || n > ASKAR_MAX_TRAVEL_MAX) {
		sim_send_error(fd);
		return;
	}
	pthread_mutex_lock(&state_mutex);
	// Firmware rejects the new max when the current logical position would fall
	// outside it; max_step is left unchanged.
	if (position > n) {
		pthread_mutex_unlock(&state_mutex);
		sim_send_error(fd);
		return;
	}
	max_step = n;
	// Stops motion; the logical position is unchanged (no offset or motor
	// register reset).
	target_position = position;
	pthread_mutex_unlock(&state_mutex);

	char resp[32];
	snprintf(resp, sizeof(resp), "F%c%d", echo, n);
	sim_send_response(fd, resp);
}

// FV# -> firmware version
static void handle_read_version(int fd) {
	char resp[64];
	snprintf(resp, sizeof(resp), "FV%s", fw_version);
	sim_send_response(fd, resp);
}

// FI# -> model name
static void handle_read_model(int fd) {
	char resp[64];
	snprintf(resp, sizeof(resp), "FI%s", model_name);
	sim_send_response(fd, resp);
}

// Fb# -> read backlash offset (lowercase b)
static void handle_read_backlash(int fd) {
	pthread_mutex_lock(&state_mutex);
	int b = backlash;
	pthread_mutex_unlock(&state_mutex);
	char resp[32];
	snprintf(resp, sizeof(resp), "Fb%d", b);
	sim_send_response(fd, resp);
}

// FBn# -> set backlash offset (clamped to [0, 10000])
static void handle_set_backlash(int fd, const char *body) {
	int32_t n;
	if (!parse_int(body, &n)) {
		sim_send_error(fd);
		return;
	}
	int clamped = (int)clamp(n, 0, ASKAR_BACKLASH_MAX);
	pthread_mutex_lock(&state_mutex);
	backlash = clamped;
	pthread_mutex_unlock(&state_mutex);
	char resp[32];
	snprintf(resp, sizeof(resp), "FB%d", clamped);
	sim_send_response(fd, resp);
}

// Fr# -> read reverse motion direction (lowercase r)
static void handle_read_reverse(int fd) {
	pthread_mutex_lock(&state_mutex);
	int r = reverse;
	pthread_mutex_unlock(&state_mutex);
	sim_send_response(fd, r ? "Fr1" : "Fr0");
}

// FR0# / FR1# -> set reverse motion direction; FR# is an alias for read.
static void handle_set_reverse(int fd, const char *body) {
	// FR# with no value is accepted as a read alias for Fr#.
	if (!*body) {
		pthread_mutex_lock(&state_mutex);
		int r = reverse;
		pthread_mutex_unlock(&state_mutex);
		sim_send_response(fd, r ? "Fr1" : "Fr0");
		return;
	}
	int32_t n;
	if (!parse_int(body, &n) || (n != 0 && n != 1)) {
		sim_send_error(fd);
		return;
	}
	pthread_mutex_lock(&state_mutex);
	reverse = (int)n;
	// Firmware halts motion when the direction is changed.
	target_position = position;
	pthread_mutex_unlock(&state_mutex);
	char resp[32];
	snprintf(resp, sizeof(resp), "FR%d", (int)n);
	sim_send_response(fd, resp);
}

// ----------------------------------------------------------------- dispatch

static void dispatch(int fd, const char *frame, int len) {
	// frame[0] is 'F'; the second character chooses the command, optionally
	// followed by a numeric body.
	if (len < 2) {
		sim_send_error(fd);
		return;
	}
	char cmd = frame[1];
	const char *body = frame + 2;

	switch (cmd) {
		case 'P': handle_absolute_move(fd, body); break;
		case 'T': handle_relative_move(fd, body); break;
		case 'S': handle_halt(fd); break;
		case 'Y': handle_sync(fd, body); break;
		case 'p': handle_read_position(fd); break;
		case 'Q': handle_read_motion_state(fd); break;
		case 'm': handle_read_max(fd); break;
		// FMn# sets the max travel; bare FM# is a read alias for Fm#.
		case 'M': if (*body) handle_set_max(fd, body, 'M'); else handle_read_max(fd); break;
		case 'X': handle_set_max(fd, body, 'X'); break;
		case 'V': handle_read_version(fd); break;
		case 'I': handle_read_model(fd); break;
		case 'b': handle_read_backlash(fd); break;
		case 'B': handle_set_backlash(fd, body); break;
		case 'r': handle_read_reverse(fd); break;
		case 'R': handle_set_reverse(fd, body); break;
		default: {
			char msg[64];
			snprintf(msg, sizeof(msg), "unknown command F%c (%s)", cmd, frame);
			log_line("??", msg);
			sim_send_error(fd);
			break;
		}
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char frame[64];

	int opt;
	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
			case 'h':
				printf("Askar-WAF focuser simulator\n");
				printf("Usage: askar_focuser_simulator [OPTIONS]\n");
				printf("  -h            Show this help and exit\n");
				return 0;
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
		int len = sim_read_frame(fd, frame, sizeof(frame));
		if (len < 0) {
			if (len == -1) {
				log_line("!!", "frame exceeded 31 bytes");
				sim_send_error(fd);
			}
			continue;
		}
		log_line("->", frame);
		dispatch(fd, frame, len);
	}

	return 0;
}
