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

void process_base_folder(const char *path) {
	struct dirent *entry;
	DIR *dp = opendir(path);
	if (dp == NULL) {
		perror("opendir");
		return;
	}
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		if (strncmp(entry->d_name, "agent_", 6) && strncmp(entry->d_name, "ao_", 3) && strncmp(entry->d_name, "aux_", 4) && strncmp(entry->d_name, "ccd_", 4) && strncmp(entry->d_name, "dome_", 5) && strncmp(entry->d_name, "focuser_", 8) && strncmp(entry->d_name, "gps_", 4) && strncmp(entry->d_name, "guider_", 7) && strncmp(entry->d_name, "mount_", 6) && strncmp(entry->d_name, "rotator_", 8) && strncmp(entry->d_name, "system_", 7) && strncmp(entry->d_name, "wheel_", 6)) {
			continue;
		}
		driver_meta *driver = malloc(sizeof(driver_meta));
		snprintf(driver->name, sizeof(driver->name), "%s", entry->d_name);
		snprintf(driver->root, sizeof(driver->root), "%s/%s", path, driver->name);
		snprintf(driver->id, sizeof(driver->id), "%s", strchr(driver->name, '_') + 1);
		struct stat statbuf;
		if (stat(driver->root, &statbuf) == -1) {
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
		snprintf(line, sizeof(line), "clang -E %s 2>/dev/null", driver_path);
		FILE *pipe = popen(line, "r");
		while (fgets(line, sizeof(line), pipe) != NULL) {
			char *driver_info = strstr(line, "SET_DRIVER_INFO");
			if (driver_info) {
				if (sscanf(driver_info, "SET_DRIVER_INFO(info, \"%127[^\"]\", __FUNCTION__, 0x%x", driver->label, &driver->version) == 2) {
					if ((driver->version & 0xFFFF000000) == 0) {
						driver->version |= 0x02000000;
					}
					drivers[last_driver++] = driver;
				}
			}
		}
		pclose(pipe);
	}
	closedir(dp);
}

int comparator(const void *a, const void *b) {
	driver_meta *driver_a = *(driver_meta **)a;
	driver_meta *driver_b = *(driver_meta **)b;
	return strcmp(driver_a->name, driver_b->name);
}

int main(int argc, char **argv) {
	process_base_folder("indigo_drivers");
	process_base_folder("indigo_mac_drivers");
	process_base_folder("indigo_linux_drivers");
	qsort(drivers, last_driver, sizeof(driver_meta *), comparator);
	for (int i = 0; i < last_driver; i++) {
		driver_meta *driver = drivers[i];
		printf("\"%s\", \"%s\", %08x\n", driver->name, driver->label, driver->version);

	}
}
