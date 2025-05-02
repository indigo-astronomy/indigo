// WandererBox Pro V3 powerbox simulator
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

char id[] = "ZXWBProV3";
char fw[] = "20231004";
int dc3_4 = 0, dc5pwm = 0, dc6pwm = 0, dc7pwm = 0, dc8_9, dc10_11, usb3_1, usb3_2, usb3_3, usb2_1_3, usb2_4_6;
double dc3_4v = 5.0;
double temperature_1 = 23.94;
double temperature_2 = 24.70;
double temperature_3 = 25.70;
double temperature_4 = 26.70;
double humidity = 36.90;
double input_current = 12.32;
double v19_current = 4.08;
double v5_12_current = 3.91;
double input_voltage = 12.11;

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
	top = newwin(5, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, "  WandererBox Pro V3 powerbox simulator is running on %s ", port);
	wrefresh(top);
	bottom = newwin(rows - 5, cols, 5, 0);
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
		char *buffer = malloc(256);
		va_list args;
		va_start(args, format);
		int length = vsnprintf(buffer, 256, format, args);
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
		mvwprintw(top, 1, 2, "DC3-4: %s %4.1fV, DC5: %3d, DC6: %3d, DC7: %3d, DC8_9: %3s, DC10_11: %3s", dc3_4 ? "on" : "off", dc3_4v, dc5pwm, dc6pwm, dc7pwm, dc8_9 ? "on" : "off", dc10_11 ? "on" : "off");
		mvwprintw(top, 2, 2, "TEMP: %5.2fC / %5.2fC  / %5.2fC  / %5.2fC, HUMIDITY: %5.2f%%, INPUT: %5.2fA / %5.2fV, OUTPUT: %5.2fA / %5.2fA", temperature_1, temperature_2, temperature_3, temperature_4, humidity, input_current, input_voltage, v19_current, v5_12_current);
		mvwprintw(top, 3, 2, "USB3.0-1: %3s, USB3.0-2: %3s, USB3.0-3: %3s, USB2.0-1-3: %3s, USB2.0-4-6: %3s", usb3_1 ? "on" : "off", usb3_2 ? "on" : "off", usb3_3 ? "on" : "off", usb2_1_3 ? "on" : "off", usb2_4_6 ? "on" : "off");
		wrefresh(top);
		pthread_mutex_unlock(&curses_mutex);
		sim_printf("%sA%sA%.2fA%.2fA%.2fA%.2fA%.2fA%.2fA%.2fA%.2fA%.2fA%dA%dA%dA%dA%dA%dA%dA%dA%dA%dA%dA%dA\n", id, fw, temperature_1,  temperature_2,  temperature_3, humidity, temperature_4, input_current, v19_current, v5_12_current, input_voltage, usb3_1, usb3_2, usb3_3, usb2_1_3, usb2_4_6, dc3_4, dc5pwm, dc6pwm, dc7pwm, dc8_9, dc10_11, (int)(dc3_4v * 10));
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
			case 101:
				dc3_4 = true;
				break;
			case 100:
				dc3_4 = false;
				break;
			case 201:
				dc8_9 = true;
				break;
			case 200:
				dc8_9 = false;
				break;
			case 211:
				dc10_11 = true;
				break;
			case 210:
				dc10_11 = false;
				break;
			case 111:
				usb3_1 = true;
				break;
			case 110:
				usb3_1 = false;
				break;
			case 121:
				usb3_2 = true;
				break;
			case 120:
				usb3_2 = false;
				break;
			case 131:
				usb3_3 = true;
				break;
			case 130:
				usb3_3 = false;
				break;
			case 141:
				usb2_1_3 = true;
				break;
			case 140:
				usb2_1_3 = false;
				break;
			case 151:
				usb2_4_6 = true;
				break;
			case 150:
				usb2_4_6 = false;
				break;
			default:
				if (20000 <= command && command <= 29132) {
					dc3_4v = (command - 20000) / 10.0;
				} else if (5000 <= command && command <= 5255) {
					dc5pwm = command - 5000;
				} else if (6000 <= command && command <= 6255) {
					dc6pwm = command - 6000;
				} else if (7000 <= command && command <= 7255) {
					dc7pwm = command - 7000;
				}
				break;
		}
	}
	return 0;
}
