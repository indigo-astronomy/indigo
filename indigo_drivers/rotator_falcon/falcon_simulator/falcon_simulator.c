// PegasusAstro Falcon/Falcon2 rotator simulator
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

int version = 2;
double position = 0.0;
double target = 0.0;
int direction = 0;
char id[] = "AA000000";
char fw[] = "1.5";

void* background(void* arg) {
	while (true) {
		if (fabs(target - position) <= 0.01) {
			position = target;
		} else if (target < position) {
			position -= 0.01;
		} else if (target > position) {
			position += 0.01;
		}
		usleep(1000);
	}
	return NULL;
}

int sim_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	long total_bytes = 0;
	while (total_bytes < length) {
		long bytes_read = read(handle, &c, 1);
		if (c == '\n')
			break;
		buffer[total_bytes++] = c;
	}
	buffer[total_bytes] = '\0';
	if (*buffer)
		printf("-> %s\n", buffer);
	return (int)total_bytes;
}

bool sim_printf(int handle, const char *format, ...) {
	if (strchr(format, '%')) {
		char *buffer = malloc(80);
		va_list args;
		va_start(args, format);
		int length = vsnprintf(buffer, 80, format, args);
		va_end(args);
		printf("<- %s", buffer);
		bool result = write(handle, buffer, length);
		free(buffer);
		return result;
	} else {
		printf("<- %s", format);
		return write(handle, format, strlen(format));
	}
}

int main() {
	pthread_t thread;
	char buffer[80];

	int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
	grantpt(fd);
	unlockpt(fd);
	ptsname_r(fd, buffer, 80);
	
	printf("PegasusAstro Falcon v%d rotator simulator is running on %s\n", version, buffer);
	
	pthread_create(&thread, NULL, background, NULL);
	
	while (true) {
		sim_read_line(fd, buffer, sizeof(buffer));
		if (!strcmp(buffer, "F#")) {
			if (version == 1)
				sim_printf(fd, "FR_OK\n");
			else if (version == 2)
				sim_printf(fd, "F2R_%s_A\n", id);
		} else if (!strcmp(buffer, "FA")) {
			if (version == 1)
				sim_printf(fd, "FR_OK:0:%.2f:%d:0:0:%d\n", position, target == position ? 0 : 1, direction);
			else if (version == 2)
				sim_printf(fd, "F2R:%.2f:%d:0:0:%d\n", position, target == position ? 0 : 1, direction);
		} else if (!strncmp(buffer, "SD:", 2)) {
			target = position = atof(buffer + 3);
			sim_printf(fd, "%s\n", buffer);
		} else if (!strncmp(buffer, "MD:", 2)) {
			target = atof(buffer + 3);
			sim_printf(fd, "%s\n", buffer);
		} else if (!strcmp(buffer, "FH")) {
			target = position;
			sim_printf(fd, "FH:1\n");
		} else if (!strcmp(buffer, "FD")) {
			sim_printf(fd, "FD:%.2f\n", position);
		} else if (!strcmp(buffer, "FR")) {
			sim_printf(fd, "FR:%d\n", target == position ? 0 : 1);
		} else if (!strcmp(buffer, "FV")) {
			sim_printf(fd, "FV:%s\n", fw);
		} else if (!strcmp(buffer, "DR:0")) {
			sim_printf(fd, "DR:0\n");
		} else if (!strncmp(buffer, "FN:", 2)) {
			direction = atoi(buffer + 3);
			sim_printf(fd, "%s\n", buffer);
		}
	}
	return 0;
}
