// PegasusAstro FocusCube simulator
//
// Copyright (c) 2024 CloudMakers, s. r. o.
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
int version = 3;
int position = 0;
int target = 0;
int direction = 0;
int backlash = 3;
int speed = 400;

char id[] = "AA000000";
char fw[] = "1.4.1";
double temperature = 23.5;

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
	top = newwin(7, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, " PegasusAstro FocusCube v%d rotator simulator is running on %s ", version, port);
	mvwprintw(top, 6, cols - 20, " CTRL + C to exit ", version, port);
	wrefresh(top);
	bottom = newwin(rows - 7, cols, 7, 0);
	scrollok(bottom, TRUE);
	wrefresh(bottom);
}

void* background(void* arg) {
	while (true) {
		if (target < position) {
			position--;
		} else if (target > position) {
			position++;
		}
		pthread_mutex_lock(&curses_mutex);
		mvwprintw(top, 1, 2, "position:  %6d", position);
		mvwprintw(top, 2, 2, "target:    %6d", target);
		mvwprintw(top, 3, 2, "direction: %6d", direction);
		mvwprintw(top, 4, 2, "backlash:  %6d", backlash);
		mvwprintw(top, 5, 2, "speed:     %6d", speed);
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
		usleep(1000);
	}
	return NULL;
}

int sim_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	long total_bytes = 0;
	while (total_bytes < length) {
		long bytes_read = read(handle, &c, 1);
		if (bytes_read <= 0)
			continue;
		if (c == '\n')
			break;
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

bool sim_printf(int handle, const char *format, ...) {
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
		bool result = write(handle, buffer, length);
		free(buffer);
		return result;
	} else {
		pthread_mutex_lock(&curses_mutex);
		wprintw(bottom, " <- %s", format);
		wrefresh(bottom);
		pthread_mutex_unlock(&curses_mutex);
		return write(handle, format, strlen(format));
	}
}

int main() {
	pthread_t thread;
	char buffer[80];

	int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
	grantpt(fd);
	unlockpt(fd);
	ptsname_r(fd, port, sizeof(port));

	init_curses();
	
	pthread_create(&thread, NULL, background, NULL);
	
	while (true) {
		sim_read_line(fd, buffer, sizeof(buffer));
		if (version == 3) {
			if (!strcmp(buffer, "F#") || !strcmp(buffer, "##")) {
				sim_printf(fd, "F3C_%s_A\n", id);
			} else if (!strcmp(buffer, "FA")) {
				sim_printf(fd, "FC3:%d:%d:%.2f:%d:%d\n", position, target == position ? 0 : 1, temperature, direction, backlash);
			} else if (!strncmp(buffer, "FN:", 3)) {
				target = position = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "FM:", 3)) {
				target = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "FG:", 3)) {
				target += atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "FH")) {
				target = position;
				sim_printf(fd, "FH:1\n");
			} else if (!strcmp(buffer, "FT")) {
				sim_printf(fd, "FT:%.2f\n", temperature);
			} else if (!strcmp(buffer, "FI")) {
				sim_printf(fd, "FI:%d\n", target == position ? 0 : 1);
			} else if (!strcmp(buffer, "FV")) {
				sim_printf(fd, "FV:%s\n", fw);
			} else if (!strncmp(buffer, "FD:", 3)) {
				direction = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "SP")) {
				sim_printf(fd, "SP:%d\n", speed);
			} else if (!strncmp(buffer, "SP:", 3)) {
				speed = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "BL:", 3)) {
				backlash = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			}
		}
	}
	return 0;
}
