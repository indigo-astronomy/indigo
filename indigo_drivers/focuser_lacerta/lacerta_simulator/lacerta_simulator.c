// LACERTA Motorfocus focuser simulator
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

int fd;

char port[128];
int position = 0;
int target = 0;
int direction = 0;
int backlash = 3;
bool moving = false;
int max_position = 250000;

char id[] = "MFOC";
char fw[] = "3.1.123";
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
	mvwprintw(top, 0, 2, " LACERTA Motorfocus v%s focuser simulator is running on %s ", fw, port);
	mvwprintw(top, 6, cols - 20, " CTRL + C to exit ");
	wrefresh(top);
	bottom = newwin(rows - 7, cols, 7, 0);
	scrollok(bottom, TRUE);
	wrefresh(bottom);
}

int sim_read_line(char *buffer, int length) {
	char c = '\0';
	long total_bytes = 0;
	while (total_bytes < length) {
		long bytes_read = read(fd, &c, 1);
		if (bytes_read <= 0)
			continue;
		buffer[total_bytes++] = c;
		if (c == '#')
			break;
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
		if (target < position) {
			position--;
		} else if (target > position) {
			position++;
		} else if (moving) {
			sim_printf("p %d\r", position);
			sim_printf("M %d\r", position);
			moving = false;
		}
		pthread_mutex_lock(&curses_mutex);
		mvwprintw(top, 1, 2, "position:  %7d", position);
		mvwprintw(top, 2, 2, "target:    %7d", target);
		mvwprintw(top, 3, 2, "direction: %7d", direction);
		mvwprintw(top, 4, 2, "backlash:  %7d", backlash);
		mvwprintw(top, 5, 2, "max:       %7d", max_position);
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
		usleep(1000);
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
		if (!strcmp(buffer, ": i #")) {
			sim_printf("i %s\r", id);
		} else if (!strcmp(buffer, ": v #")) {
			sim_printf("v%s\r", fw);
		} else if (!strncmp(buffer, ": P ", 4)) {
			target = position = atoi(buffer + 4);
			sim_printf("p %d\r", target);
		} else if (!strncmp(buffer, ": G ", 4)) {
			max_position = atoi(buffer + 4);
			sim_printf("g %d\r", max_position);
		} else if (!strcmp(buffer, ": g #")) {
			sim_printf("g %d\r", max_position);
		} else if (!strncmp(buffer, ": R ", 4)) {
			direction = atoi(buffer + 4);
			sim_printf("r %d\r", direction);
		} else if (!strcmp(buffer, ": r #")) {
			sim_printf("r %d\r", direction);
		} else if (!strncmp(buffer, ": q ", 4)) {
			sim_printf("p %d\r", position);
		} else if (!strncmp(buffer, ": M ", 4)) {
			target = atoi(buffer + 4);
		} else if (!strcmp(buffer, ": H")) {
			target = position;
			moving = 0;
			sim_printf("H 1\r");
		} else if (!strcmp(buffer, ": t #")) {
			sim_printf("t %g\r", temperature);
		} else if (!strncmp(buffer, ": B ", 4)) {
			backlash = atoi(buffer + 4);
			sim_printf("b %d\r", backlash);
		} else if (!strcmp(buffer, ": b #")) {
			sim_printf("b %d\r", backlash);
		}
	}
	return 0;
}
