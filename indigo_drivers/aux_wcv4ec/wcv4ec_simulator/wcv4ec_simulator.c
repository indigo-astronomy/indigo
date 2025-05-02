// WandererAstro WandererCover V4-EC simulator
//
// Copyright (c) 2025 CloudMakers, s. r. o.
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


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <curses.h>

char port[128];
int fd;

char id[] = "WandererCoverV4";
char fw[] = "20240618";
double open_pos = 270.0;
double closed_pos = 20.0;
double current_pos = 20.0;
double voltage = 12.11;
int heater = 0;
int light = 0;
bool do_open = false;
bool do_close = false;

WINDOW *top, *bottom;
pthread_mutex_t curses_mutex;

void init_curses() {
	int rows, cols;
	pthread_mutex_init(&curses_mutex, NULL);
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, rows, cols);
	top = newwin(3, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, "  WandererAstro WandererCover V4-EC simulator is running on %s ", port);
	wrefresh(top);
	bottom = newwin(rows - 3, cols, 3, 0);
	scrollok(bottom, TRUE);
	wrefresh(bottom);
}

int sim_read_line(char *buffer, int length) {
	char c = '\0';
	long total_bytes = 0;
	while (total_bytes < length) {
		long bytes_read = read(fd, &c, 1);
		if (bytes_read <= 0) {
			continue;
		}
		if (c == '\n') {
			break;
		}
		buffer[total_bytes++] = c;
	}
	buffer[total_bytes] = '\0';
	if (*buffer) {
		pthread_mutex_lock(&curses_mutex);
		wprintw(bottom, " -> %s\n", buffer);
		wrefresh(bottom);
		pthread_mutex_unlock(&curses_mutex);
	}
	return (int)total_bytes;
}

bool sim_printf(const char *format, ...) {
	if (strchr(format, '%')) {
		char *buffer = malloc(80);
		va_list args;
		va_start(args, format);
		int length = vsnprintf(buffer, 80, format, args);
		va_end(args);
		pthread_mutex_lock(&curses_mutex);
		wprintw(bottom, " <- %s", buffer);
		wrefresh(bottom);
		pthread_mutex_unlock(&curses_mutex);
		bool result = write(fd, buffer, length);
		free(buffer);
		return result;
	} else {
		pthread_mutex_lock(&curses_mutex);
		wprintw(bottom, " <- %s", format);
		wrefresh(bottom);
		pthread_mutex_unlock(&curses_mutex);
		return write(fd, format, strlen(format));
	}
}

void* background(void* arg) {
	while (true) {
		pthread_mutex_lock(&curses_mutex);
		mvwprintw(top, 1, 2, "OPEN: %5.2f, CLOSED: %5.2f, CURRENT: %5.2f, VOLTAGE: %4.1fV, HEATER: %3d, LIGHT: %3d", open_pos, closed_pos, current_pos, voltage, heater, light);
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
		sim_printf("%sA%sA%.2fA%.2fA%.2fA%.2fA%dA\n", id, fw, closed_pos, open_pos, current_pos, voltage, light);
		usleep(1000000);
		if (do_open) {
			if (open_pos - current_pos > 15) {
				current_pos += 15;
			} else {
				current_pos = open_pos;
				do_open = false;
			}
		} else if (do_close) {
			if (current_pos - closed_pos > 15) {
				current_pos -= 15;
			} else {
				current_pos = closed_pos;
				do_close = false;
			}
		}
	}
	return NULL;
}


int main() {
	pthread_t thread;
	char buffer[80];

	fd = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
	grantpt(fd);
	unlockpt(fd);
	ptsname_r(fd, port, sizeof(port));

	init_curses();

	pthread_create(&thread, NULL, background, NULL);
	
	while (true) {
		sim_read_line(buffer, sizeof(buffer));
		int command = atoi(buffer);
		switch (command) {
			case 1000:
				do_open = false;
				do_close = true;
				break;
			case 1001:
				do_open = true;
				do_close = false;
				break;
			case 2000:
				heater = 0;
				break;
			case 2050:
				heater = 50;
				break;
			case 2100:
				heater = 100;
				break;
			case 2150:
				heater = 150;
				break;
			case 9999:
				light = 0;
				break;
			default:
				if (40000 <= command && command <= 67000) {
					open_pos = (command - 40000) / 100.0;
				} else if (10000 <= command && command <= 12055) {
					closed_pos = (command - 10000) / 100.0;
				} else if (1 <= command && command <= 255) {
					light = command;
				}
				break;
		}
	}
	return 0;
}
