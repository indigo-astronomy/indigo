// WandererBox Plus V3 powerbox simulator
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

char id[] = "ZXWBPlusV3";
char fw[] = "20230926";
int dc2 = 0, dc4_6 = 0, usb = 0, dc3pwm = 0;;
double dc2v = 5.0;
double temperature_1 = 23.94;
double temperature_2 = 24.70;
double humidity = 36.90;
double current = 0.26;
double voltage = 12.11;

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
	top = newwin(4, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, "  WandererBox Plus V3 powerbox simulator is running on %s ", port);
	wrefresh(top);
	bottom = newwin(rows - 4, cols, 4, 0);
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
		mvwprintw(top, 1, 2, "DC2: %s %4.1fV, DC3: %3d, DC4-6: %3s, USB: %3s", dc2 ? "on" : "off", dc2v, dc3pwm, dc4_6 ? "on" : "off", usb ? "on" : "off");
		mvwprintw(top, 2, 2, "TEMP: %5.2fC / %5.2fC, HUMIDITY: %5.2f%%, INPUT: %5.2fA / %5.2fV", temperature_1, temperature_2, humidity, current, voltage);
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
		sim_printf("%sA%sA%.2fA%.2fA%.2fA%.2fA%.2fA%dA%dA%dA%dA%dA%dA\n", id, fw, temperature_1, humidity, temperature_2, current, voltage, usb, dc2, dc3pwm, dc4_6, (int)(dc2v * 10));
		usleep(1000000);
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
			case 66300744:
				break;
			case 121:
				dc2 = true;
				break;
			case 120:
				dc2 = false;
				break;
			case 101:
				dc4_6 = true;
				break;
			case 100:
				dc4_6 = false;
				break;
			case 111:
				usb = true;
				break;
			case 110:
				usb = false;
				break;
			default:
				if (20000 <= command && command <= 29132) {
					dc2v = (command - 20000) / 10.0;
				} else if (3000 <= command && command <= 3255) {
					dc3pwm = command - 3000;
				}
				break;
		}
	}
	return 0;
}
