// PegasusAstro UPB3 simulator
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

// https://pegasusastro.com/command-list-for-upbv3/


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
int power[] = { 0, 0, 0, 0, 0, 0 };
int heat[] = { 0, 0, 0 };
int buck = 0;
int boost = 0;
int relay = 0;
int usb[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int autodew[] = { 0, 0, 0 };

int buck_voltage = 3;
int boost_voltage = 12;

int position = 0;
int target = 0;
int direction = 0;
int speed = 400;

char id[] = "AA000000";
char fw[] = "1.4.1";

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
	top = newwin(6, cols, 0, 0);
	box(top, 0, 0);
	mvwprintw(top, 0, 2, " PegasusAstro UPB v%d simulator is running on %s ", version, port);
	mvwprintw(top, 6, cols - 20, " CTRL + C to exit ");
	wrefresh(top);
	bottom = newwin(rows - 6, cols, 6, 0);
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
		mvwprintw(top, 1, 2, "power:    %3d%% %3d%% %3d%% %3d%% %3d%% %3d%% %4s %4s %4s", power[0], power[1], power[2], power[3], power[4], power[5], buck ? "on" : "off", boost ? "on" : "off", relay  ? "on" : "off");
		mvwprintw(top, 2, 2, "dew/auto: %3d%% %4s %3d%% %4s %3d%% %4s", heat[0], autodew[0] ? "on" : "off", heat[1], autodew[1] ? "on" : "off", heat[2],  autodew[2] ? "on" : "off");
			mvwprintw(top, 3, 2, "voltage:  %3dV %3dV", buck_voltage, boost_voltage);
		mvwprintw(top, 4, 2, "usb:      %4s %4s %4s %4s %4s %4s %4s %4s", usb[0] ? "on" : "off", usb[1] ? "on" : "off", usb[2] ? "on" : "off", usb[3] ? "on" : "off", usb[4] ? "on" : "off", usb[5] ? "on" : "off", usb[6] ? "on" : "off", usb[7] ? "on" : "off");

		mvwprintw(top, 1, 61, "position:  %6d", position);
		mvwprintw(top, 2, 61, "target:    %6d", target);
		mvwprintw(top, 3, 61, "direction: %6d", direction);
		mvwprintw(top, 4, 61, "speed:     %6d", speed);
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
			if (!strcmp(buffer, "P#") || !strcmp(buffer, "##")) {
				sim_printf(fd, "UPBv3_%s_A\n", id);
			} else if (!strcmp(buffer, "PV")) {
				sim_printf(fd, "PV:%s\n", fw);
			} else if (!strcmp(buffer, "PA")) {
				sim_printf(fd, "PA:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n", power[0], power[1], power[2], power[3], power[4], power[5], heat[0], heat[1], heat[2], buck, boost, relay);
			} else if (!strcmp(buffer, "UA")) {
				sim_printf(fd, "UA:%d:%d:%d:%d:%d:%d:%d:%d\n", usb[0], usb[1], usb[2], usb[3], usb[4], usb[5], usb[6], usb[7]);
			} else if (!strncmp(buffer, "P1:", 3)) {
				power[0] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "P2:", 3)) {
				power[1] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "P3:", 3)) {
				power[2] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "P4:", 3)) {
				power[3] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "P5:", 3)) {
				power[4] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "P6:", 3)) {
				power[5] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U1:", 3)) {
				usb[0] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U2:", 3)) {
				usb[1] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U3:", 3)) {
				usb[2] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U4:", 3)) {
				usb[3] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U5:", 3)) {
				usb[4] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U6:", 3)) {
				usb[5] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U7:", 3)) {
				usb[6] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "U8:", 3)) {
				usb[7] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "D1:", 3)) {
				heat[0] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "D2:", 3)) {
				heat[1] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "D3:", 3)) {
				heat[2] = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "PS")) {
				sim_printf(fd, "PS:1\n");
			} else if (!strcmp(buffer, "US")) {
				sim_printf(fd, "US:1\n");
			} else if (!strcmp(buffer, "DSTR")) {
				sim_printf(fd, "DSTR:1\n");
			} else if (!strcmp(buffer, "AJ")) {
				sim_printf(fd, "AJ:%d:%d:%d:%d\n", buck_voltage, buck, boost_voltage, boost);
			} else if (!strcmp(buffer, "IS")) {
				sim_printf(fd, "IS:0:0:0:1:0:0\n");
			} else if (!strcmp(buffer, "VR")) {
				sim_printf(fd, "VR:12.3:2.2\n");
			} else if (!strcmp(buffer, "PC")) {
				sim_printf(fd, "PC:2.1:13.2:10.2:3324\n");
			} else if (!strncmp(buffer, "RL:", 3)) {
				relay = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "PJ:", 3)) {
				int value = atoi(buffer + 3);
				if (value == 0)
					buck = 0;
				else if (value == 1)
					buck = 1;
				else
					buck_voltage = value;
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "PB:", 3)) {
				int value = atoi(buffer + 3);
				if (value == 0)
					boost = 0;
				else if (value == 1)
					boost = 1;
				else
					boost_voltage = value;
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "ADW1:", 5)) {
				autodew[0] = atoi(buffer + 5);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "ADW2:", 5)) {
				autodew[1] = atoi(buffer + 5);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "ADW3:", 5)) {
				autodew[2] = atoi(buffer + 5);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "PD")) {
				sim_printf(fd, "PD:%d%d%d\n", autodew[0], autodew[1], autodew[2]);
			} else if (!strncmp(buffer, "DA:", 3)) {
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "PL:", 3)) {
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "ES")) {
				sim_printf(fd, "ES:22.5:50.1:12.2\n");
			} else if (!strcmp(buffer, "SA")) {
				sim_printf(fd, "SA:%d:%d:%d:%d:1:0:1\n", position, target == position ? 0 : 1, direction, speed);
			} else if (!strcmp(buffer, "SP")) {
				sim_printf(fd, "SP:%d\n", position);
			} else if (!strncmp(buffer, "SC:", 3)) {
				target = position = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "SM:", 3)) {
				target = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "SG:", 3)) {
				target += atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "SH")) {
				target = position;
				sim_printf(fd, "SH:1\n");
			} else if (!strcmp(buffer, "SI")) {
				sim_printf(fd, "SI:%d\n", target == position ? 0 : 1);
			} else if (!strncmp(buffer, "SR:", 3)) {
				direction = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strncmp(buffer, "SS:", 3)) {
				speed = atoi(buffer + 3);
				sim_printf(fd, "%s\n", buffer);
			} else if (!strcmp(buffer, "PF")) {
			}
		}
	}
	return 0;
}
