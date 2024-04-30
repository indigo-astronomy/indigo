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

/** INDIGO Astrometry.net plate solver agent
 \file indigo_agent_astrometry.c
 */

#define DRIVER_VERSION 0x0010
#define DRIVER_NAME	"indigo_agent_astrometry"

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
#include <indigo/indigo_align.h>
#include <indigo/indigo_platesolver.h>
#include <indigo/indigo_dslr_raw.h>

#include "indigo_agent_astrometry.h"

static char *index_files[] = {
	"4219",
	"4218",
	"4217",
	"4216",
	"4215",
	"4214",
	"4213",
	"4212",
	"4211",
	"4210",
	"4209",
	"4208",
	"4207-00", "4207-01", "4207-02", "4207-03", "4207-04", "4207-05", "4207-06", "4207-07", "4207-08", "4207-09", "4207-10", "4207-11",
	"4206-00", "4206-01", "4206-02", "4206-03", "4206-04", "4206-05", "4206-06", "4206-07", "4206-08", "4206-09", "4206-10", "4206-11",
	"4205-00", "4205-01", "4205-02", "4205-03", "4205-04", "4205-05", "4205-06", "4205-07", "4205-08", "4205-09", "4205-10", "4205-11",
	"4204-00", "4204-01", "4204-02", "4204-03", "4204-04", "4204-05", "4204-06", "4204-07", "4204-08", "4204-09", "4204-10", "4204-11", "4204-12", "4204-13", "4204-14", "4204-15", "4204-16", "4204-17", "4204-18", "4204-19", "4204-20", "4204-21", "4204-22", "4204-23", "4204-24", "4204-25", "4204-26", "4204-27", "4204-28", "4204-29", "4204-30", "4204-31", "4204-32", "4204-33", "4204-34", "4204-35", "4204-36", "4204-37", "4204-38", "4204-39", "4204-40", "4204-41", "4204-42", "4204-43", "4204-44", "4204-45", "4204-46", "4204-47",
	"4203-00", "4203-01", "4203-02", "4203-03", "4203-04", "4203-05", "4203-06", "4203-07", "4203-08", "4203-09", "4203-10", "4203-11", "4203-12", "4203-13", "4203-14", "4203-15", "4203-16", "4203-17", "4203-18", "4203-19", "4203-20", "4203-21", "4203-22", "4203-23", "4203-24", "4203-25", "4203-26", "4203-27", "4203-28", "4203-29", "4203-30", "4203-31", "4203-32", "4203-33", "4203-34", "4203-35", "4203-36", "4203-37", "4203-38", "4203-39", "4203-40", "4203-41", "4203-42", "4203-43", "4203-44", "4203-45", "4203-46", "4203-47",
	"4202-00", "4202-01", "4202-02", "4202-03", "4202-04", "4202-05", "4202-06", "4202-07", "4202-08", "4202-09", "4202-10", "4202-11", "4202-12", "4202-13", "4202-14", "4202-15", "4202-16", "4202-17", "4202-18", "4202-19", "4202-20", "4202-21", "4202-22", "4202-23", "4202-24", "4202-25", "4202-26", "4202-27", "4202-28", "4202-29", "4202-30", "4202-31", "4202-32", "4202-33", "4202-34", "4202-35", "4202-36", "4202-37", "4202-38", "4202-39", "4202-40", "4202-41", "4202-42", "4202-43", "4202-44", "4202-45", "4202-46", "4202-47",
	"4201-00", "4201-01", "4201-02", "4201-03", "4201-04", "4201-05", "4201-06", "4201-07", "4201-08", "4201-09", "4201-10", "4201-11", "4201-12", "4201-13", "4201-14", "4201-15", "4201-16", "4201-17", "4201-18", "4201-19", "4201-20", "4201-21", "4201-22", "4201-23", "4201-24", "4201-25", "4201-26", "4201-27", "4201-28", "4201-29", "4201-30", "4201-31", "4201-32", "4201-33", "4201-34", "4201-35", "4201-36", "4201-37", "4201-38", "4201-39", "4201-40", "4201-41", "4201-42", "4201-43", "4201-44", "4201-45", "4201-46", "4201-47",
	"4200-00", "4200-01", "4200-02", "4200-03", "4200-04", "4200-05", "4200-06", "4200-07", "4200-08", "4200-09", "4200-10", "4200-11", "4200-12", "4200-13", "4200-14", "4200-15", "4200-16", "4200-17", "4200-18", "4200-19", "4200-20", "4200-21", "4200-22", "4200-23", "4200-24", "4200-25", "4200-26", "4200-27", "4200-28", "4200-29", "4200-30", "4200-31", "4200-32", "4200-33", "4200-34", "4200-35", "4200-36", "4200-37", "4200-38", "4200-39", "4200-40", "4200-41", "4200-42", "4200-43", "4200-44", "4200-45", "4200-46", "4200-47",
	"4119",
	"4118",
	"4117",
	"4116",
	"4115",
	"4114",
	"4113",
	"4112",
	"4111",
	"4110",
	"4109",
	"4108",
	"4107",
	NULL
};

static double index_diameters[][2] = {
	{ 2.0, 2.8 },
	{ 2.8, 4.0 },
	{ 4.0, 5.6 },
	{ 5.6, 8.0 },
	{ 8, 11 },
	{ 11, 16 },
	{ 16, 22 },
	{ 22, 30 },
	{ 30, 42 },
	{ 42, 60 },
	{ 60, 85 },
	{ 85, 120 },
	{ 120, 170 },
	{ 170, 240 },
	{ 240, 340 },
	{ 340, 480 },
	{ 480, 680 },
	{ 680, 1000 },
	{ 1000, 1400 },
	{ 1400, 2000 }
};

static char *index_size[][2] = {
	{ NULL, "14.2G" },
	{ NULL, "9.2G" },
	{ NULL, "5.0G" },
	{ NULL, "2.6G" },
	{ NULL, "1.3G" },
	{ NULL, "659M" },
	{ NULL, "328M" },
	{ "164M", "165M" },
	{ "94M", "82M" },
	{ "49M", "41M" },
	{ "25M", "21M" },
	{ "10M", "8M" },
	{ "5M", "4M" },
	{ "3M", "2M" },
	{ "1M", "1M" },
	{ "740K", "596K" },
	{ "409K", "340K" },
	{ "248K", "213K" },
	{ "187K", "164K" },
	{ "144K", "132K" }
};

static char base_dir[512];

#define ASTROMETRY_DEVICE_PRIVATE_DATA				((astrometry_private_data *)device->private_data)
#define ASTROMETRY_CLIENT_PRIVATE_DATA				((astrometry_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_ASTROMETRY_INDEX_41XX_PROPERTY	(ASTROMETRY_DEVICE_PRIVATE_DATA->index_41xx_property)
#define AGENT_ASTROMETRY_INDEX_4119_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+0)
#define AGENT_ASTROMETRY_INDEX_4118_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+1)
#define AGENT_ASTROMETRY_INDEX_4117_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+2)
#define AGENT_ASTROMETRY_INDEX_4116_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+3)
#define AGENT_ASTROMETRY_INDEX_4115_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+4)
#define AGENT_ASTROMETRY_INDEX_4114_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+5)
#define AGENT_ASTROMETRY_INDEX_4113_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+6)
#define AGENT_ASTROMETRY_INDEX_4112_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+7)
#define AGENT_ASTROMETRY_INDEX_4111_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+8)
#define AGENT_ASTROMETRY_INDEX_4110_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+9)
#define AGENT_ASTROMETRY_INDEX_4109_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+10)
#define AGENT_ASTROMETRY_INDEX_4108_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+11)
#define AGENT_ASTROMETRY_INDEX_4107_ITEM    	(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items+12)

#define AGENT_ASTROMETRY_INDEX_42XX_PROPERTY	(ASTROMETRY_DEVICE_PRIVATE_DATA->index_42xx_property)
#define AGENT_ASTROMETRY_INDEX_4219_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+0)
#define AGENT_ASTROMETRY_INDEX_4218_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+1)
#define AGENT_ASTROMETRY_INDEX_4217_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+2)
#define AGENT_ASTROMETRY_INDEX_4216_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+3)
#define AGENT_ASTROMETRY_INDEX_4215_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+4)
#define AGENT_ASTROMETRY_INDEX_4214_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+5)
#define AGENT_ASTROMETRY_INDEX_4213_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+6)
#define AGENT_ASTROMETRY_INDEX_4212_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+7)
#define AGENT_ASTROMETRY_INDEX_4211_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+8)
#define AGENT_ASTROMETRY_INDEX_4210_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+9)
#define AGENT_ASTROMETRY_INDEX_4209_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+10)
#define AGENT_ASTROMETRY_INDEX_4208_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+11)
#define AGENT_ASTROMETRY_INDEX_4207_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+12)
#define AGENT_ASTROMETRY_INDEX_4206_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+13)
#define AGENT_ASTROMETRY_INDEX_4205_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+14)
#define AGENT_ASTROMETRY_INDEX_4204_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+15)
#define AGENT_ASTROMETRY_INDEX_4203_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+16)
#define AGENT_ASTROMETRY_INDEX_4202_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+17)
#define AGENT_ASTROMETRY_INDEX_4201_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+18)
#define AGENT_ASTROMETRY_INDEX_4200_ITEM    	(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items+19)

typedef struct {
	platesolver_private_data platesolver;
	indigo_property *index_41xx_property;
	indigo_property *index_42xx_property;
	int frame_width;
	int frame_height;
	pid_t pid;
	bool abort_requested;
} astrometry_private_data;

// --------------------------------------------------------------------------------

extern char **environ;

static bool execute_command(indigo_device *device, char *command, ...) {
	char buffer[8 * 1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);

	ASTROMETRY_DEVICE_PRIVATE_DATA->abort_requested = false;
	char command_buf[8 * 1024];
	sprintf(command_buf, "%s 2>&1", buffer);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
	int pipe_stdout[2];
	if (pipe(pipe_stdout)) {
		return false;
	}
	switch (ASTROMETRY_DEVICE_PRIVATE_DATA->pid = fork()) {
		case -1: {
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to execute %s (%s)", command_buf, strerror(errno));
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
		double d1, d2;
		char s[16];
		if (strstr(line, "message:")) {
			indigo_send_message(device, line + 9);
		} else if (sscanf(line, "simplexy: nx=%d, ny=%d", &ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width, &ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height) == 2) {
			ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width *= AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value;
			ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height *= AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value;
		} else if (sscanf(line, "Field center: (RA,Dec) = (%lg, %lg)", &d1, &d2) == 2) {
			AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = d1 / 15;
			AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = d2;
			if (AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.target == 0) {
				indigo_j2k_to_jnow(&AGENT_PLATESOLVER_WCS_RA_ITEM->number.value, &AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value);
				AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value = 0;
			} else {
				AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value = 2000;
			}
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = false;
		} else if (sscanf(line, "Field size: %lg x %lg %s", &d1, &d2, s) == 3) {
			if (!strcmp(s, "degrees")) {
				AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = d1;
				AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = d2;
			} else if (!strcmp(s, "arcminutes")) {
				AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = d1 / 60.0;
				AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = d2 / 60.0;
			} else if (!strcmp(s, "arcseconds")) {
				AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = d1 / 3600.0;
				AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = d2 / 3600.0;
			}
			AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = (
				AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value / ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width +
				AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value / ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height
			) / 2;
		} else if (sscanf(line, "Field rotation angle: up is %lg", &d1) == 1) {
			AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = d1;
		} else if (sscanf(line, "Field 1: solved with index index-%lg", &d1) == 1) {
			indigo_send_message(device, "Solved");
			AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = d1;
		} else if (sscanf(line, "Field parity: %3s", s) == 1) {
			AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = !strcmp(s, "pos") ? 1 : -1;
		} else if (strstr(line, "Total CPU time limit reached")) {
			indigo_send_message(device, "CPU time limit reached");
		} else if (strstr(line, "Did not solve")) {
			indigo_send_message(device, "No solution found");
		} else if (strstr(line, "You must list at least one index")) {
			indigo_send_message(device, "You must select at least one index");
		} else if (strstr(line, ": not found")) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", line);
			res = false;
		}
		if (line) {
			free(line);
			line = NULL;
		}
	}
	fclose(output);
	ASTROMETRY_DEVICE_PRIVATE_DATA->pid = 0;
	if (ASTROMETRY_DEVICE_PRIVATE_DATA->abort_requested) {
		res = false;
		ASTROMETRY_DEVICE_PRIVATE_DATA->abort_requested = false;
		indigo_send_message(device, "Aborted");
	}
	return res;
}

struct indigo_jpeg_decompress_struct {
	struct jpeg_decompress_struct pub;
	jmp_buf jpeg_error;
};

static void jpeg_decompress_error_callback(j_common_ptr cinfo) {
	longjmp(((struct indigo_jpeg_decompress_struct *)cinfo)->jpeg_error, 1);
}

#define astrometry_save_config indigo_platesolver_save_config

static void astrometry_abort(indigo_device *device) {
	if (ASTROMETRY_DEVICE_PRIVATE_DATA->pid) {
		ASTROMETRY_DEVICE_PRIVATE_DATA->abort_requested = true;
		/* NB: To kill the whole process group with PID you should send kill signal to -PID (-1 * PID) */
		kill(-ASTROMETRY_DEVICE_PRIVATE_DATA->pid, SIGTERM);
	}
}

static bool astrometry_solve(indigo_device *device, void *image, unsigned long image_size) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = true;
		char *message = "";
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_SOLVING;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		char base[512];
		sprintf(base, "%s/%s_%lX", base_dir, "image", time(0));
#pragma clang diagnostic pop
		// convert any input image to FITS file
		int handle = open(base, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle < 0) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			message = "Can't create temporary image file";
			goto cleanup;
		}
		if (!strncmp("SIMPLE", (const char *)image, 6)) {
			// FITS - copy only
			indigo_write(handle, (const char *)image, image_size);
			ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = 0;
		} else {
			void *intermediate_image = NULL;
			int byte_per_pixel = 0, components = 0;
			if (!strncmp("RAW1", (const char *)(image), 4)) {
				// 8 bit RAW
				byte_per_pixel = 1;
				components = 1;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = ((indigo_raw_header *)image)->width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = ((indigo_raw_header *)image)->height;
				image = image + sizeof(indigo_raw_header);
			} else if (!strncmp("RAW2", (const char *)(image), 4)) {
				// 16 bit RAW
				byte_per_pixel = 2;
				components = 1;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = ((indigo_raw_header *)image)->width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = ((indigo_raw_header *)image)->height;
				image = image + sizeof(indigo_raw_header);
			} else if (!strncmp("RAW3", (const char *)(image), 4)) {
				// 8 bit RGB
				byte_per_pixel = 1;
				components = 3;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = ((indigo_raw_header *)image)->width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = ((indigo_raw_header *)image)->height;
				image = image + sizeof(indigo_raw_header);
			} else if (!strncmp("RAW6", (const char *)(image), 4)) {
				// 16 bit RGB
				byte_per_pixel = 2;
				components = 3;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = ((indigo_raw_header *)image)->width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = ((indigo_raw_header *)image)->height;
				image = image + sizeof(indigo_raw_header);
			} else if (((uint8_t *)image)[0] == 0xFF && ((uint8_t *)image)[1] == 0xD8 && ((uint8_t *)image)[2] == 0xFF) {
				// JPEG
				struct indigo_jpeg_decompress_struct cinfo;
				struct jpeg_error_mgr jerr;
				cinfo.pub.err = jpeg_std_error(&jerr);
				jerr.error_exit = jpeg_decompress_error_callback;
				if (setjmp(cinfo.jpeg_error)) {
					jpeg_destroy_decompress(&cinfo.pub);
					AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
					message = "Broken JPEG file";
					goto cleanup;
				}
				jpeg_create_decompress(&cinfo.pub);
				jpeg_mem_src(&cinfo.pub, image, image_size);
				int rc = jpeg_read_header(&cinfo.pub, TRUE);
				if (rc < 0) {
					jpeg_destroy_decompress(&cinfo.pub);
					AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
					message = "Broken JPEG file";
					goto cleanup;
				}
				jpeg_start_decompress(&cinfo.pub);
				byte_per_pixel = 1;
				components = cinfo.pub.output_components;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = cinfo.pub.output_width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = cinfo.pub.output_height;
				int row_stride = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width * components;
				image = intermediate_image = indigo_safe_malloc(image_size = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height * row_stride);
				while (cinfo.pub.output_scanline < cinfo.pub.output_height) {
					unsigned char *buffer_array[1];
					buffer_array[0] = intermediate_image + (cinfo.pub.output_scanline) * row_stride;
					jpeg_read_scanlines(&cinfo.pub, buffer_array, 1);
				}
				jpeg_finish_decompress(&cinfo.pub);
				jpeg_destroy_decompress(&cinfo.pub);
			} else {
				indigo_dslr_raw_image_s output_image = {0};
				int rc = indigo_dslr_raw_process_image((void *)image, image_size, &output_image);
				if (rc != LIBRAW_SUCCESS) {
					if (output_image.data != NULL) free(output_image.data);
					image = NULL;
				}
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = output_image.width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = output_image.height;
				byte_per_pixel = 2;
				components = 1;
				image = intermediate_image = output_image.data;
			}
			if (image == NULL) {
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
				message = "Unsupported image format";
				close(handle);
				goto cleanup;
			}
			int pixel_count = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width * ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height;
			image_size = pixel_count * byte_per_pixel + FITS_LOGICAL_RECORD_LENGTH;
			if (image_size % FITS_LOGICAL_RECORD_LENGTH) {
				image_size = (image_size / FITS_LOGICAL_RECORD_LENGTH + 1) * FITS_LOGICAL_RECORD_LENGTH;
			}
			char *buffer = indigo_safe_malloc(image_size), *p = buffer;
			memset(buffer, ' ', image_size);
			int t = sprintf(p, "SIMPLE  = %20c", 'T'); p[t] = ' ';
			t = sprintf(p += 80, "BITPIX  = %20d", byte_per_pixel * 8); p[t] = ' ';
			t = sprintf(p += 80, "NAXIS   = %20d", 2); p[t] = ' ';
			t = sprintf(p += 80, "NAXIS1  = %20d", ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width); p[t] = ' ';
			t = sprintf(p += 80, "NAXIS2  = %20d", ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height); p[t] = ' ';
			t = sprintf(p += 80, "EXTEND  = %20c", 'T'); p[t] = ' ';
			if (byte_per_pixel == 2) {
				t = sprintf(p += 80, "BZERO   = %20d", 32768); p[t] = ' ';
				t = sprintf(p += 80, "BSCALE  = %20d", 1); p[t] = ' ';
			}
			t = sprintf(p += 80, "END"); p[t] = ' ';
			p = buffer + FITS_LOGICAL_RECORD_LENGTH;
			if (components == 1) {
				// mono
				if (byte_per_pixel == 2) {
					// 16 bit RAW - swap endian
					uint16_t *in = image;
					uint16_t *out = (uint16_t *)p;
					for (int i = 0; i < pixel_count; i++) {
						int value = *in++ - 32768;
						*out++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				} else {
					// 8 bit RAW
					memcpy(p, image, pixel_count);
				}
			} else {
				// RGB
				if (byte_per_pixel == 2) {
					// 16 bit RGB - average and swap endian
					uint16_t *in = image;
					uint16_t *out = (uint16_t *)p;
					for (int i = 0; i < pixel_count; i++) {
						int value = (in[0] + in[1] + in[2]) / 3 - 32768;
						in += 3;
						*out++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				} else {
					// 8 bit RGB - average
					char *in = image;
					char *out = p;
					for (int i = 0; i < pixel_count; i++) {
						int value = (in[0] + in[1] + in[2]) / 3;
						in += 3;
						*out++ = value;
					}
				}
			}
			indigo_write(handle, buffer, image_size);
			indigo_safe_free(buffer);
			indigo_safe_free(intermediate_image);
		}
		close(handle);
		// execute astrometry.net plate solver
		char path[INDIGO_VALUE_SIZE];
		snprintf(path, sizeof((path)), "%s/astrometry.cfg", base_dir);
		handle = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle < 0) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			message = "Can't create astrometry.cfg";
			goto cleanup;
		}
		char config[INDIGO_VALUE_SIZE];
		snprintf(config, sizeof(config), "add_path %s\n", base_dir);
		indigo_write(handle, config, strlen(config));
		for (int k = 0; k < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; k++) {
			indigo_item *item = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + k;
			if (item->sw.value) {
				for (int l = 0; index_files[l]; l++) {
					if (!strncmp(item->name, index_files[l], 4)) {
						snprintf(config, sizeof(config), "index index-%s\n", index_files[l]);
						indigo_write(handle, config, strlen(config));
					}
				}
			}
		}
		close(handle);
		char hints[512] = "";
		int hints_index = 0;
		if (ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width == 0 || ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height == 0) {
			hints_index += sprintf(hints + hints_index, " -v");
		}
		if (AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value > 1) {
			hints_index += sprintf(hints + hints_index, " -d %d", (int)AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value);
		}
		if (!execute_command(device, "image2xy -O%s -o \"%s.xy\" \"%s\"", hints, base, base)) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			message = "Execution of image2xy failed";
			goto cleanup;
		}
		*hints = 0;
		hints_index = 0;
		if (AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value > 0) {
			hints_index += sprintf(hints + hints_index, " --ra %g --dec %g --radius %g", AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value * 15, AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value, AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.value != 0) {
			hints_index += sprintf(hints + hints_index, " --parity %s", AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.value > 0 ? "pos" : "neg");
		}
		if (AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value > 0) {
			hints_index += sprintf(hints + hints_index, " --depth %d", (int)AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value);
		}
		if (AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value > 0) {
			hints_index += sprintf(hints + hints_index, " --scale-units arcsecperpix --scale-low %.3f --scale-high %.3f", AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value * 0.9 * 3600, AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value * 1.1 * 3600);
		} else if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale > 0 && AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value < 0) {
			hints_index += sprintf(hints + hints_index, " --scale-units arcsecperpix --scale-low %.3f --scale-high %.3f", INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale * 0.9 * 3600, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale * 1.1 * 3600);
		}
		if (AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value > 0) {
			hints_index += sprintf(hints + hints_index, " --cpulimit %d", (int)AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value);
		}
		if (!execute_command(device, "solve-field --overwrite --no-plots --no-remove-lines --no-verify-uniformize --sort-column FLUX --uniformize 0%s --config \"%s/astrometry.cfg\" --axy \"%s.axy\" \"%s.xy\"", hints, base_dir, base, base)) {
			message = "Execution of solve-field failed";
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	cleanup:
		/* globs do not work in quotes */
		execute_command(device, "rm -rf \"%s/image_\"*", base_dir);
		if (message[0] == '\0')
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		else
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, message);
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		return !INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solver is busy");
	return false;
}

static void sync_installed_indexes(indigo_device *device, char *dir, indigo_property *property) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char path[INDIGO_VALUE_SIZE];
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		bool add = false;
		bool remove = false;
		for (int j = 0; index_files[j]; j++) {
			char *file_name = index_files[j];
			if (!strncmp(file_name, item->name, 4)) {
				snprintf(path, sizeof((path)), "%s/index-%s.fits", base_dir, file_name);
				if (item->sw.value) {
					if (access(path, F_OK) == 0) {
						continue;
					}
					indigo_send_message(device, "Downloading %s...", file_name);
					if (!execute_command(device, "curl -L -s -o \"%s\" http://data.astrometry.net/%s/index-%s.fits", path, dir, file_name)) {
						item->sw.value = false;
						property->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, property, strerror(errno));
						pthread_mutex_unlock(&mutex);
						return;
					}

					/* basic index file integritiy check as curl saves HTTP
					   errors like "404: not found" in the output file
					*/
					bool failed = false;
					char signature[7]={0};
					FILE *fp=fopen(path,"rb");
					if (fp) {
						fread(signature, 6, 1, fp);
						fclose(fp);
						if (strncmp(signature, "SIMPLE", 6)) {
							failed = true;
						}
					} else {
						failed = true;
					}
					if (failed) {
						unlink(path);
						item->sw.value = false;
						property->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, property, "Index download failed: '%s'", path);
						pthread_mutex_unlock(&mutex);
						return;
					}

					indigo_send_message(device, "Done", file_name);
					add = true;
					continue;
				} else {
					if (access(path, F_OK) == 0) {
						if (unlink(path)) {
							property->state = INDIGO_ALERT_STATE;
							indigo_update_property(device, property, strerror(errno));
							pthread_mutex_unlock(&mutex);
							return;
						}
						remove = true;
					}
				}
			}
		}
		if (add) {
			char long_label[INDIGO_VALUE_SIZE];
			if (!strcmp(property->name, AGENT_ASTROMETRY_INDEX_41XX_PROPERTY_NAME)) {
				snprintf(long_label, INDIGO_VALUE_SIZE, "Tycho-2 %s", item->label);
			} else {
				snprintf(long_label, INDIGO_VALUE_SIZE, "2MASS %s", item->label);
			}
			indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, item->name, long_label, true);
		}
		if (remove) {
			for (int j = 0; j < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; j++) {
				if (!strcmp(item->name, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items[j].name)) {
					memmove(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + j, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + (j + 1), (AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count - j) * sizeof(indigo_item));
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
	astrometry_save_config(device);
	pthread_mutex_unlock(&mutex);
}

static void index_41xx_handler(indigo_device *device) {
	static int instances = 0;
	instances++;
	sync_installed_indexes(device, "4100", AGENT_ASTROMETRY_INDEX_41XX_PROPERTY);
	instances--;
	if (AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->state == INDIGO_BUSY_STATE)
		AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->state = instances ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_ASTROMETRY_INDEX_41XX_PROPERTY, NULL);
}

static void index_42xx_handler(indigo_device *device) {
	static int instances = 0;
	instances++;
	sync_installed_indexes(device, "4200", AGENT_ASTROMETRY_INDEX_42XX_PROPERTY);
	instances--;
	if (AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->state == INDIGO_BUSY_STATE)
		AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->state = instances ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_ASTROMETRY_INDEX_42XX_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_platesolver_device_attach(device, DRIVER_NAME, DRIVER_VERSION, 0) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- AGENT_ASTROMETRY_INDEX_XXXX
		char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE], path[INDIGO_VALUE_SIZE];
		bool present;
		AGENT_ASTROMETRY_INDEX_41XX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ASTROMETRY_INDEX_41XX_PROPERTY_NAME, "Index managememt", "Installed Tycho-2 catalog indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 13);
		strcpy(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->hints,"warn_on_clear:\"Delete Tycho-2 index file?\";");
		if (AGENT_ASTROMETRY_INDEX_41XX_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 19; i >=7; i--) {
			sprintf(name, "41%02d", i);
			if (index_diameters[i][0] > 60)
				sprintf(label, "Index 41%02d (%.0f-%.0f°, %sB)", i, index_diameters[i][0] / 60, index_diameters[i][1] / 60, index_size[i][0]);
			else
				sprintf(label, "Index 41%02d (%.0f-%.0f\', %sB)", i, index_diameters[i][0], index_diameters[i][1], index_size[i][0]);
			present = true;
			for (int j = 0; index_files[j]; j++) {
				char *file_name = index_files[j];
				if (!strncmp(file_name, name, 4)) {
					snprintf(path, sizeof((path)), "%s/index-%s.fits", base_dir, file_name);
					if (access(path, F_OK)) {
						present = false;
						break;
					}
				}
			}
			indigo_init_switch_item(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items - (i - 19), name, label, present);
			sprintf((AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items - (i - 19))->hints, "warn_on_clear:\"Delete Tycho-2 index 41%02d?\";", i);
			if (present) {
				char long_label[INDIGO_VALUE_SIZE];
				snprintf(long_label, INDIGO_VALUE_SIZE, "Tycho-2 %s", label);
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, long_label, false);
			}
		}
		AGENT_ASTROMETRY_INDEX_42XX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ASTROMETRY_INDEX_42XX_PROPERTY_NAME, "Index managememt", "Installed 2MASS catalog indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 20);
		strcpy(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->hints, "warn_on_clear:\"Delete 2MASS index file?\";");
		if (AGENT_ASTROMETRY_INDEX_42XX_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 19; i >=0; i--) {
			sprintf(name, "42%02d", i);
			if (index_diameters[i][0] > 60)
				sprintf(label, "Index 42%02d (%.0f-%.0f°, %sB)", i, index_diameters[i][0] / 60, index_diameters[i][1] / 60, index_size[i][1]);
			else
				sprintf(label, "Index 42%02d (%.0f-%.0f\', %sB)", i, index_diameters[i][0], index_diameters[i][1], index_size[i][1]);
			present = true;
			for (int j = 0; index_files[j]; j++) {
				char *file_name = index_files[j];
				if (!strncmp(file_name, name, 4)) {
					snprintf(path, sizeof((path)), "%s/index-%s.fits", base_dir, file_name);
					if (access(path, F_OK)) {
						present = false;
						break;
					}
				}
			}
			indigo_init_switch_item(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items - (i - 19), name, label, present);
			sprintf((AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items - (i - 19))->hints, "warn_on_clear:\"Delete 2MASS index 42%02d?\";", i);
			if (present) {
				char long_label[INDIGO_VALUE_SIZE];
				snprintf(long_label, INDIGO_VALUE_SIZE, "2MASS %s", label);
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, long_label, false);
			}
		}
		indigo_property_sort_items(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, 0);
		// --------------------------------------------------------------------------------
		ASTROMETRY_DEVICE_PRIVATE_DATA->platesolver.save_config = astrometry_save_config;
		ASTROMETRY_DEVICE_PRIVATE_DATA->platesolver.solve = astrometry_solve;
		ASTROMETRY_DEVICE_PRIVATE_DATA->platesolver.abort = astrometry_abort;
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY, property))
		indigo_define_property(device, AGENT_ASTROMETRY_INDEX_41XX_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY, property))
		indigo_define_property(device, AGENT_ASTROMETRY_INDEX_42XX_PROPERTY, NULL);
	return indigo_platesolver_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_ASTROMETRY_INDEX_41XX
		indigo_property_copy_values(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY, property, false);
		AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_ASTROMETRY_INDEX_41XX_PROPERTY, NULL);
		indigo_set_timer(device, 0, index_41xx_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_ASTROMETRY_INDEX_42XX
		indigo_property_copy_values(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY, property, false);
		AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_ASTROMETRY_INDEX_42XX_PROPERTY, NULL);
		indigo_set_timer(device, 0, index_42xx_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_platesolver_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY);
	indigo_release_property(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY);
	return indigo_platesolver_device_detach(device);
}

// -------------------------------------------------------------------------------- Initialization

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

static void kill_children() {
	indigo_device *device = agent_device;
	if (device && device->private_data) {
		if (ASTROMETRY_DEVICE_PRIVATE_DATA->pid)
			kill(-ASTROMETRY_DEVICE_PRIVATE_DATA->pid, SIGKILL);
		indigo_device **additional_devices = DEVICE_CONTEXT->additional_device_instances;
		if (additional_devices) {
			for (int i = 0; i < MAX_ADDITIONAL_INSTANCES; i++) {
				device = additional_devices[i];
				if (device && device->private_data && ASTROMETRY_DEVICE_PRIVATE_DATA->pid)
					kill(-ASTROMETRY_DEVICE_PRIVATE_DATA->pid, SIGKILL);
			}
		}
	}
}

indigo_result indigo_agent_astrometry(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		ASTROMETRY_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		ASTROMETRY_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_platesolver_client_attach,
		indigo_platesolver_define_property,
		indigo_platesolver_update_property,
		indigo_platesolver_delete_property,
		NULL,
		indigo_platesolver_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, ASTROMETRY_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			if (!indigo_platesolver_validate_executable("solve-field") || !indigo_platesolver_validate_executable("image2xy") || !indigo_platesolver_validate_executable("curl")) {
				indigo_error("Astrometry.net or curl is not available");
				return INDIGO_UNRESOLVED_DEPS;
			}
			last_action = action;
			char *env = getenv("INDIGO_CACHE_BASE");
			if (env) {
				snprintf(base_dir, sizeof((base_dir)), "%s/astrometry", env);
			} else {
				snprintf(base_dir, sizeof((base_dir)), "%s/.indigo/astrometry", getenv("HOME"));
			}
			mkdir(base_dir, 0777);
			void *private_data = indigo_safe_malloc(sizeof(astrometry_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			static bool first_time = true;
			if (first_time) {
				first_time = false;
				atexit(kill_children);
			}
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
