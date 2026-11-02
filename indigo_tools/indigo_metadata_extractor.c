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

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define MAX_DRIVERS		500

typedef struct driver_meta {
	char root[PATH_MAX];
	char name[64];
	char id[64];
	char label[128];
	uint32_t version;
} driver_meta;

driver_meta *drivers[MAX_DRIVERS] = { NULL };
int last_driver = 0;

static bool extraction_failed;

static FILE *preprocess(const char *path, pid_t *child) {
	int descriptors[2];
	if (pipe(descriptors) != 0) {
		return NULL;
	}
	*child = fork();
	if (*child == 0) {
		close(descriptors[0]);
		if (dup2(descriptors[1], STDOUT_FILENO) < 0) {
			_exit(127);
		}
		close(descriptors[1]);
		int errors = open("/dev/null", O_WRONLY);
		if (errors >= 0) {
			dup2(errors, STDERR_FILENO);
			close(errors);
		}
		execlp("clang", "clang", "-E", "--", path, (char *)NULL);
		_exit(127);
	}
	close(descriptors[1]);
	if (*child < 0) {
		close(descriptors[0]);
		return NULL;
	}
	FILE *stream = fdopen(descriptors[0], "r");
	if (stream == NULL) {
		close(descriptors[0]);
		while (waitpid(*child, NULL, 0) < 0 && errno == EINTR) {
		}
	}
	return stream;
}

void process_base_folder(const char *path) {
	struct dirent *entry;
	DIR *dp = opendir(path);
	if (dp == NULL) {
		perror("opendir");
		extraction_failed = true;
		return;
	}
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		if (strncmp(entry->d_name, "agent_", 6) && strncmp(entry->d_name, "ao_", 3) && strncmp(entry->d_name, "aux_", 4) && strncmp(entry->d_name, "ccd_", 4) && strncmp(entry->d_name, "dome_", 5) && strncmp(entry->d_name, "focuser_", 8) && strncmp(entry->d_name, "gps_", 4) && strncmp(entry->d_name, "guider_", 7) && strncmp(entry->d_name, "mount_", 6) && strncmp(entry->d_name, "rotator_", 8) && strncmp(entry->d_name, "system_", 7) && strncmp(entry->d_name, "wheel_", 6)) {
			continue;
		}
		driver_meta *driver = calloc(1, sizeof(driver_meta));
		if (driver == NULL) {
			extraction_failed = true;
			break;
		}
		snprintf(driver->name, sizeof(driver->name), "%s", entry->d_name);
		snprintf(driver->root, sizeof(driver->root), "%s/%s", path, driver->name);
		snprintf(driver->id, sizeof(driver->id), "%s", strchr(driver->name, '_') + 1);
		struct stat statbuf;
		if (stat(driver->root, &statbuf) == -1) {
			free(driver);
			continue;
		}
		if (!S_ISDIR(statbuf.st_mode)) {
			free(driver);
			continue;
		}
		char driver_path[PATH_MAX];
		char line[1024];
		snprintf(driver_path, sizeof(driver_path), "%s/indigo_%s.c", driver->root, driver->name);
		if (access(driver_path, F_OK) != 0) {
			snprintf(driver_path, sizeof(driver_path), "%s/indigo_%s.cpp", driver->root, driver->name);
			if (access(driver_path, F_OK) != 0) {
				snprintf(driver_path, sizeof(driver_path), "%s/indigo_%s.m", driver->root, driver->name);
				if (access(driver_path, F_OK) != 0) {
					free(driver);
					continue;
				}
			}
		}
		pid_t child;
		FILE *pipe = preprocess(driver_path, &child);
		if (pipe == NULL) {
			perror("Cannot start preprocessor");
			free(driver);
			extraction_failed = true;
			continue;
		}
		bool found = false;
		while (fgets(line, sizeof(line), pipe) != NULL) {
			char *driver_info = strstr(line, "SET_DRIVER_INFO");
			if (driver_info) {
				if (sscanf(driver_info, "SET_DRIVER_INFO(info, \"%127[^\"]\", __FUNCTION__, 0x%x", driver->label, &driver->version) == 2) {
					if ((driver->version & 0xFFFF000000) == 0) {
						driver->version |= 0x02000000;
					}
					found = true;
				}
			}
		}
		bool read_ok = !ferror(pipe);
		if (fclose(pipe) != 0) {
			read_ok = false;
		}
		int status;
		pid_t waited;
		do {
			waited = waitpid(child, &status, 0);
		} while (waited < 0 && errno == EINTR);
		if (read_ok && waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0 && found && last_driver < MAX_DRIVERS) {
			drivers[last_driver++] = driver;
		} else {
			free(driver);
			extraction_failed = true;
		}
	}
	closedir(dp);
}

int comparator(const void *a, const void *b) {
	driver_meta *driver_a = *(driver_meta **)a;
	driver_meta *driver_b = *(driver_meta **)b;
	return strcmp(driver_a->name, driver_b->name);
}

int main(void) {
	process_base_folder("indigo_drivers");
	process_base_folder("indigo_mac_drivers");
	process_base_folder("indigo_linux_drivers");
	qsort(drivers, last_driver, sizeof(driver_meta *), comparator);
	for (int i = 0; i < last_driver; i++) {
		driver_meta *driver = drivers[i];
		printf("\"%s\", \"%s\", %08x\n", driver->name, driver->label, driver->version);
		free(driver);
	}
	return extraction_failed ? 1 : 0;
}
