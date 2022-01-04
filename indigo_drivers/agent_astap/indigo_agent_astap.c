// Copyright (c) 2021 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASTAP plate solver agent
 \file indigo_agent_astap.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_astap"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <jpeglib.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_platesolver.h>

#include "indigo_agent_astap.h"

typedef struct _index_data {
	char *name;
	char *url;
	char *size;
	double fov_min, fov_max;
} index_data;

static index_data astap_index[] = {
	{ "W08", "https://downloads.sourceforge.net/project/astap-program/star_databases/w08_star_database_mag08_astap.zip?ts=gAAAAABh1F-iYBVowuuAjE41WBUlDxaRFtbxMstFgQFt66665M9siiqpf1a6nYLk2F1ZfcKBzz0OmbICwgUk97JR4P9IZGtOcw%3D%3D&r=", "327K", 20.0, 180.0 },
	{ "V17", "https://downloads.sourceforge.net/project/hnsky/star_databases/v17_star_database_mag17_colour.zip?ts=gAAAAABh1GNW_x4l5mCyPWgoSe3ActBPsBMuVJ7M3qK9-NC4j1haQeWWVLd24k-1l8CiYqHFqPeXBOvMSEh_BOfgl8OZJcLz9Q%3D%3D&r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fhnsky%2Ffiles%2Fstar_databases%2Fv17_star_database_mag17_colour.zip%2Fdownload", "685M", 1, 20 },
	{ "H17", "TBD", "???M", 1, 10 },
	{ "H18", "TBD", "???M", 0.25, 10 },
	{ NULL }
};
static char base_dir[512];

#define ASTAP_DEVICE_PRIVATE_DATA				((astap_private_data *)device->private_data)
#define ASTAP_CLIENT_PRIVATE_DATA				((astap_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_ASTAP_INDEX_PROPERTY	(ASTAP_DEVICE_PRIVATE_DATA->index_property)
#define AGENT_ASTAP_INDEX_W08_ITEM  (AGENT_ASTAP_INDEX_PROPERTY->items+0)
#define AGENT_ASTAP_INDEX_V17_ITEM  (AGENT_ASTAP_INDEX_PROPERTY->items+1)

typedef struct {
	platesolver_private_data platesolver;
	indigo_property *index_property;
	int frame_width;
	int frame_height;
	pid_t pid;
} astap_private_data;

// --------------------------------------------------------------------------------

extern char **environ;

static bool execute_command(indigo_device *device, char *command, ...) {
	char buffer[8 * 1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);

	char command_buf[8 * 1024];
	sprintf(command_buf, "%s 2>&1", buffer);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
	int pipe_stdout[2];
	if (pipe(pipe_stdout)) {
		return false;
	}
	switch (ASTAP_DEVICE_PRIVATE_DATA->pid = fork()) {
		case -1: {
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			indigo_send_message(device, "Failed to execute %s (%s)", command, strerror(errno));
			return false;
		}
		case 0: {
			setpgid(0, 0);
			close(pipe_stdout[0]);
			dup2(pipe_stdout[1], STDOUT_FILENO);
			close(pipe_stdout[1]);
			execl("/bin/sh", "sh", "-c", buffer, NULL, environ);
			perror("execl");
			_exit(127);
		}
	}
	close(pipe_stdout[1]);
	FILE *output = fdopen(pipe_stdout[0], "r");
	char *line = NULL;
	size_t size = 0;
	bool res = true;
	while (getline(&line, &size, output) >= 0) {
		char *nl = strchr(line, '\n');
		if (nl)
			*nl = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s", line);
	}
	if (line) {
		free(line);
		line = NULL;
	}
	fclose(output);
	ASTAP_DEVICE_PRIVATE_DATA->pid = 0;
	if (AGENT_PLATESOLVER_ABORT_PROPERTY->state == INDIGO_BUSY_STATE) {
		res = false;
		AGENT_PLATESOLVER_ABORT_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_PLATESOLVER_ABORT_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_PLATESOLVER_ABORT_PROPERTY, NULL);
	}
	return res;
}

#define astap_save_config indigo_platesolver_save_config

static void *astap_solve(indigo_platesolver_task *task) {
	indigo_device *device = task->device;
	void *image = task->image;
	unsigned long image_size = task->size;
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		char *ext = "raw";
		bool use_stdin = false;
		if (!strncmp("SIMPLE", (const char *)image, 6)) {
			ext = "fits";
			const char *header = (const char *)image;
			while (strncmp(header, "END", 3) && header < (const char *)image + image_size) {
				if (sscanf(header, "NAXIS1  = %d", &ASTAP_DEVICE_PRIVATE_DATA->frame_width) == 0)
					if (sscanf(header, "NAXIS2  = %d", &ASTAP_DEVICE_PRIVATE_DATA->frame_height) == 1)
						break;
				header += 80;
			}
		} else if (!strncmp("JFIF", (const char *)(image + 6), 4)) {
			ext = "jpeg";
			ASTAP_DEVICE_PRIVATE_DATA->frame_width = ASTAP_DEVICE_PRIVATE_DATA->frame_height = 0;
		} else if (!strncmp("RAW", (const char *)(image), 3)) {
			indigo_raw_header *header = (indigo_raw_header *)image;
			ASTAP_DEVICE_PRIVATE_DATA->frame_width = header->width;
			ASTAP_DEVICE_PRIVATE_DATA->frame_height = header->height;
			use_stdin = true;
		} else {
			image = NULL;
		}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		char base[512], file[512], wcs[512];
		sprintf(base, "%s/%s_%lX", base_dir, "image", time(0));
		sprintf(file, "%s.%s", base, ext);
		sprintf(wcs, "%s.wcs", base);
#pragma clang diagnostic pop
		int handle = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle < 0) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Can't create temporary image file");
			goto cleanup;
		}
		indigo_write(handle, (const char *)image, image_size);
		close(handle);
		// execute astap plate solver
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		char params[512] = "";
		int params_index = 0;
		params_index = sprintf(params, "-z %d", (int)AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value);
		if (AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -r %g", AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -ra %g", AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -spd %g", AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value + 90);
		}
		if (AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value > 0) {
			params_index += sprintf(params + params_index, " -s %d", (int)AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value > 0 && ASTAP_DEVICE_PRIVATE_DATA->frame_height > 0) {
			params_index += sprintf(params + params_index, " -fov %.1f", AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value * ASTAP_DEVICE_PRIVATE_DATA->frame_height);
		}
		for (int k = 0; k < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; k++) {
			indigo_item *item = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + k;
			if (item->sw.value) {
				params_index += sprintf(params + params_index, " -d %s/%s", base_dir, item->name);
				AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = k;
				break;
			}
		}

		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = true;
		if (use_stdin) {
			if (!execute_command(device, "astap_cli %s -o %s -f stdin <%s", params, base, file))
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			if (!execute_command(device, "astap_cli %s -f %s", params, file))
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE) {
			FILE *output = fopen(wcs, "r");
			char *line = NULL;
			size_t size = 0;
			if (output == NULL) {
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "No solution found");
				goto cleanup;
			}
			while (getline(&line, &size, output) >= 0) {
				char *nl = strchr(line, '\n');
				if (nl)
					*nl = 0;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s", line);
				char c;
				double d;
				char *s;
				if (sscanf(line, "PLTSOLVD=                    %c", &c) == 1) {
					INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = c != 'T';
				} else if (sscanf(line, "CRPIX1  = %lg", &d) == 1) {
					ASTAP_DEVICE_PRIVATE_DATA->frame_width = 2 * (int)d;
				} else if (sscanf(line, "CRPIX1  = %lg", &d) == 1) {
					ASTAP_DEVICE_PRIVATE_DATA->frame_height = 2 * (int)d;
				} else if (sscanf(line, "CRPIX1  = %lg", &d) == 1) {
					AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = d / 15.0;
				} else if (sscanf(line, "CRPIX1  = %lg", &d) == 1) {
					AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = d;
				} else if (sscanf(line, "CROTA1 = %lg", &d) == 1) {
					AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = d;
				} else if (sscanf(line, "CROTA2 = %lg", &d) == 1) {
					AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = (AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value + d) / 2.0;
				} else if (sscanf(line, "CD1_1   = %lg", &d) == 1) {
					AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = d;
				} else if (sscanf(line, "CD2_2   = %lg", &d) == 1) {
					AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = (AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value + d) / 2.0;
					AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = ASTAP_DEVICE_PRIVATE_DATA->frame_width * AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value;
					AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = ASTAP_DEVICE_PRIVATE_DATA->frame_height * AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value;
				} else if ((s = strstr(line, "Solved")) && sscanf(s, "Solved in %lg", &d) == 1) {
					indigo_send_message(device, "Solved in %gs", d);
				}
			}
			if (line) {
				free(line);
				line = NULL;
			}
			fclose(output);
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_platesolver_sync(device);
			if (
				AGENT_PLATESOLVER_SYNC_SYNC_ITEM->sw.value ||
				AGENT_PLATESOLVER_SYNC_CENTER_ITEM->sw.value ||
				AGENT_PLATESOLVER_SYNC_SET_PA_REFERENCE_AND_MOVE_ITEM->sw.value ||
				AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value
			) {
				/* continue to be busy while mount is moving or syncing but show the solution */
				indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
				for (int i = 0; i < 300; i++) { // wait 3 s to become BUSY
					if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state == INDIGO_BUSY_STATE) {
						break;
					}
					indigo_usleep(10000);
				}
				for (int i = 0; i < 300; i++) { // wait 30s to become not BUSY
					if (time(NULL) - INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_timestamp > 5) {
						AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
						break;
					}
					if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state != INDIGO_BUSY_STATE) {
						AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state;
						break;
					}
					indigo_usleep(100000);
				}
			} else if (AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE) {
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		if (AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_OK_STATE) {
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		} else {
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Plate solver failed");
		}
	cleanup:
		execute_command(device, "rm -rf \"%s\" \"%s\" \"%s.ini\" \"%s.log\"", file, wcs, base, base);
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solver is busy");
	}
	free(task->image);
	free(task);
	return NULL;
}

static void sync_installed_indexes(indigo_device *device, char *dir, indigo_property *property) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char path[INDIGO_VALUE_SIZE];
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		bool add = false;
		bool remove = false;
		char *name;
		for (int j = 0; (name = astap_index[j].name); j++) {
			if (!strncmp(name, item->name, 4)) {
				snprintf(path, sizeof((path)), "%s/%s", base_dir, astap_index[j].name);
				if (item->sw.value) {
					if (access(path, F_OK) == 0) {
						continue;
					}
					indigo_send_message(device, "Downloading %s...", astap_index[j].name);
					if (!execute_command(device, "curl -L -s -o \"%s.zip\" \"%s\"", path, astap_index[j].url)) {
						item->sw.value = false;
						property->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, property, strerror(errno));
						pthread_mutex_unlock(&mutex);
						return;
					}
					if (!execute_command(device, "unzip \"%s.zip\" -d \"%s\"", path, path)) {
						item->sw.value = false;
						property->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, property, strerror(errno));
						pthread_mutex_unlock(&mutex);
						return;
					}
					if (access(path, F_OK) != 0) {
						item->sw.value = false;
						property->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, property, "Failed to download index %s", astap_index[j].name);
						pthread_mutex_unlock(&mutex);
						return;
					}
					execute_command(device, "rm -f \"%s.zip\"", path);
					indigo_send_message(device, "Done");
					add = true;
					continue;
				} else {
					if (access(path, F_OK) == 0) {
						indigo_send_message(device, "Removing %s...", astap_index[j].name);
						execute_command(device, "rm -rf \"%s\"", path);
						remove = true;
						indigo_send_message(device, "Done");
					}
				}
			}
		}
		if (add) {
			indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, item->name, item->label, true);
		}
		if (remove) {
			for (int j = 0; j < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; j++) {
				if (!strcmp(item->name, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items[j].name)) {
					indigo_item tmp[AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count - j];
					memcpy(tmp, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + (j + 1), (AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count - j) * sizeof(indigo_item));
					memcpy(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + j, tmp, (AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count - j) * sizeof(indigo_item));
					AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count--;
					break;
				}
			}
		}
	}
	indigo_delete_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
	indigo_property_sort_items(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, 0);
	AGENT_PLATESOLVER_USE_INDEX_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
	astap_save_config(device);
	pthread_mutex_unlock(&mutex);
}

static void index_handler(indigo_device *device) {
	static int instances = 0;
	instances++;
	sync_installed_indexes(device, "index", AGENT_ASTAP_INDEX_PROPERTY);
	instances--;
	if (AGENT_ASTAP_INDEX_PROPERTY->state == INDIGO_BUSY_STATE)
		AGENT_ASTAP_INDEX_PROPERTY->state = instances ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_ASTAP_INDEX_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_platesolver_device_attach(device, DRIVER_NAME, DRIVER_VERSION, 0) == INDIGO_OK) {
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
		AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.min = AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.min = AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.max = 0;
		AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.max = 0;
		// -------------------------------------------------------------------------------- AGENT_ASTAP_INDEX_XXXX
		char *name, label[INDIGO_VALUE_SIZE], path[INDIGO_VALUE_SIZE];
		bool present;
		AGENT_ASTAP_INDEX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ASTAP_INDEX_PROPERTY_NAME, "Index managememt", "Installed ASTAP indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 10);
		if (AGENT_ASTAP_INDEX_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_ASTAP_INDEX_PROPERTY->count = 0;
		for (int i = 0; (name = astap_index[i].name); i++) {
			sprintf(label, "Index %s (FOV %g-%g°, size %sB)", name, astap_index[i].fov_min, astap_index[i].fov_max, astap_index[i].size);
			snprintf(path, sizeof((path)), "%s/%s", base_dir, name);
			present = access(path, F_OK) != -1;
			indigo_init_switch_item(AGENT_ASTAP_INDEX_PROPERTY->items + i, name, label, present);
			if (present)
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, label, false);
			AGENT_ASTAP_INDEX_PROPERTY->count++;
		}
		// --------------------------------------------------------------------------------
		ASTAP_DEVICE_PRIVATE_DATA->platesolver.save_config = astap_save_config;
		ASTAP_DEVICE_PRIVATE_DATA->platesolver.solve = astap_solve;
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_ASTAP_INDEX_PROPERTY, property))
		indigo_define_property(device, AGENT_ASTAP_INDEX_PROPERTY, NULL);
	return indigo_platesolver_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_ASTAP_INDEX_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_ASTAP_INDEX
		indigo_property_copy_values(AGENT_ASTAP_INDEX_PROPERTY, property, false);
		AGENT_ASTAP_INDEX_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_ASTAP_INDEX_PROPERTY, NULL);
		indigo_set_timer(device, 0, index_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_ABORT_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_ABORT
		indigo_property_copy_values(AGENT_PLATESOLVER_ABORT_PROPERTY, property, false);
		if (AGENT_PLATESOLVER_ABORT_ITEM && ASTAP_DEVICE_PRIVATE_DATA->pid) {
			AGENT_PLATESOLVER_ABORT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_PLATESOLVER_ABORT_PROPERTY, NULL);
			/* NB: To kill the whole process group with PID you should send kill signal to -PID (-1 * PID) */
			kill(-ASTAP_DEVICE_PRIVATE_DATA->pid, SIGTERM);
		}
	}
	return indigo_platesolver_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_ASTAP_INDEX_PROPERTY);
	return indigo_platesolver_device_detach(device);
}

// -------------------------------------------------------------------------------- Initialization

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_astap(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ASTAP_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ASTAP_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_platesolver_client_attach,
		indigo_platesolver_define_property,
		indigo_platesolver_update_property,
		indigo_platesolver_delete_property,
		NULL,
		indigo_platesolver_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, ASTAP_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			if (!indigo_platesolver_validate_executable("astap_cli")) {
				indigo_error("ASTAP is not available");
				return INDIGO_UNRESOLVED_DEPS;
			}			
			last_action = action;
			char *env = getenv("INDIGO_CACHE_BASE");
			if (env) {
				strcpy(base_dir, env);
			} else {
				snprintf(base_dir, sizeof((base_dir)), "%s/.indigo/astap", getenv("HOME"));
			}
			mkdir(base_dir, 0777);
			void *private_data = indigo_safe_malloc(sizeof(astap_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
