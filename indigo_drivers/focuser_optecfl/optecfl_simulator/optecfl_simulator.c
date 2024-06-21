// Optec FocusLynx focuser simulator
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

int position[] = { 0, 0 };
int max_position[] = { 10000, 10000 };
int target[] = { 0, 0 };
double temperature[] = { 21.7, 22.1 };
char focuser_type[][3] = { "OA", "OA" };

void* background(void* arg) {
	while (true) {
		for (int focuser = 0; focuser <= 1; focuser++) {
			if (target[focuser] < position[focuser]) {
				position[focuser]--;
			} else if (target[focuser] > position[focuser]) {
				position[focuser]++;
			}
		}
		usleep(10000);
	}
	return NULL;
}

int sim_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	long total_bytes = 0;
	while (total_bytes < length) {
		long bytes_read = read(handle, &c, 1);
		if (bytes_read > 0) {
			if (c == '<')
				total_bytes = 0;
			buffer[total_bytes++] = c;
			if (c == '>')
				break;
		}
	}
	buffer[total_bytes] = '\0';
	if (total_bytes > 0)
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
	
	printf("Optec FocusLynx focuser simulator is running on %s\n", buffer);
	
	pthread_create(&thread, NULL, background, NULL);
	
	while (true) {
		sim_read_line(fd, buffer, sizeof(buffer));
		if (!strcmp(buffer, "<FHGETHUBINFO>")) {
			sim_printf(fd, "!\n");
			sim_printf(fd, "STATUS\n");
			sim_printf(fd, "Hub FVer = 1.0.0\n");
			sim_printf(fd, "END\n");
		} else if (!strcmp(buffer, "<F1GETCONFIG>") || !strcmp(buffer, "<F2GETCONFIG>")) {
			int focuser = buffer[2] == '1' ? 0 : 1;
			sim_printf(fd, "!\n");
			sim_printf(fd, "CONFIG%d\n", focuser + 1);
			sim_printf(fd, "Max Pos = %d\n", max_position[focuser]);
			sim_printf(fd, "Dev Type = %s\n", focuser_type[focuser]);
			sim_printf(fd, "END\n");
		} else if (!strcmp(buffer, "<F1GETSTATUS>") || !strcmp(buffer, "<F2GETSTATUS>")) {
			int focuser = buffer[2] == '1' ? 0 : 1;
			sim_printf(fd, "!\n");
			sim_printf(fd, "STATUS%d\n", focuser + 1);
			sim_printf(fd, "Temp(C) = %+.1f\n", temperature[focuser]);
			sim_printf(fd, "Curr Pos = %d\n", position[focuser]);
			sim_printf(fd, "Targ Pos = %d\n", target[focuser]);
			sim_printf(fd, "IsMoving = %d\n", position[focuser] == target[focuser] ? 0 : 1);
			sim_printf(fd, "TmpProbe = 1\n");
			sim_printf(fd, "END\n");
		} else if (!strcmp(buffer, "<F1HALT>") || !strcmp(buffer, "<F2HALT>")) {
			int focuser = buffer[2] == '1' ? 0 : 1;
			sim_printf(fd, "!\n");
			target[focuser] = position[focuser];
			sim_printf(fd, "HALTED\n");
		} else if (!strncmp(buffer, "<F1MA", 5) || !strncmp(buffer, "<F2MA", 5)) {
			sim_printf(fd, "!\n");
			int focuser = buffer[2] == '1' ? 0 : 1;
			int value = atoi(buffer + 5);
			if (value <= max_position[focuser])
				target[focuser] = value;
			else
				target[focuser] = max_position[focuser];
			sim_printf(fd, "M\n");
		} else if (!strncmp(buffer, "<F1SCCP", 7) || !strncmp(buffer, "<F2SCCP", 7)) {
			sim_printf(fd, "!\n");
			int focuser = buffer[2] == '1' ? 0 : 1;
			int value = atoi(buffer + 7);
			if (value <= max_position[focuser])
				target[focuser] = position[focuser] = value;
			else
				target[focuser] = position[focuser] = max_position[focuser];
			sim_printf(fd, "SET\n");
		} else if (!strncmp(buffer, "<F1SCDT", 7) || !strncmp(buffer, "<F2SCDT", 7)) {
			sim_printf(fd, "!\n");
			int focuser = buffer[2] == '1' ? 0 : 1;
			focuser_type[focuser][0] = buffer[7];
			focuser_type[focuser][1] = buffer[8];
			sim_printf(fd, "SET\n");
		}
	}
	return 0;
}
