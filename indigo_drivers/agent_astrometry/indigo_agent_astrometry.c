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

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME	"indigo_agent_astrometry"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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
#include <jpeglib.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_platesolver.h>

#include "indigo_agent_astrometry.h"

static char *index_files[] = {
	"index-4219",
	"index-4218",
	"index-4217",
	"index-4216",
	"index-4215",
	"index-4214",
	"index-4213",
	"index-4212",
	"index-4211",
	"index-4210",
	"index-4209",
	"index-4208",
	"index-4207-00", "index-4207-01", "index-4207-02", "index-4207-03", "index-4207-04", "index-4207-05", "index-4207-06", "index-4207-07", "index-4207-08", "index-4207-09", "index-4207-10", "index-4207-11",
	"index-4206-00", "index-4206-01", "index-4206-02", "index-4206-03", "index-4206-04", "index-4206-05", "index-4206-06", "index-4206-07", "index-4206-08", "index-4206-09", "index-4206-10", "index-4206-11",
	"index-4205-00", "index-4205-01", "index-4205-02", "index-4205-03", "index-4205-04", "index-4205-05", "index-4205-06", "index-4205-07", "index-4205-08", "index-4205-09", "index-4205-10", "index-4205-11",
	"index-4204-00", "index-4204-01", "index-4204-02", "index-4204-03", "index-4204-04", "index-4204-05", "index-4204-06", "index-4204-07", "index-4204-08", "index-4204-09", "index-4204-10", "index-4204-11", "index-4204-12", "index-4204-13", "index-4204-14", "index-4204-15", "index-4204-16", "index-4204-17", "index-4204-18", "index-4204-19", "index-4204-20", "index-4204-21", "index-4204-22", "index-4204-23", "index-4204-24", "index-4204-25", "index-4204-26", "index-4204-27", "index-4204-28", "index-4204-29", "index-4204-30", "index-4204-31", "index-4204-32", "index-4204-33", "index-4204-34", "index-4204-35", "index-4204-36", "index-4204-37", "index-4204-38", "index-4204-39", "index-4204-40", "index-4204-41", "index-4204-42", "index-4204-43", "index-4204-44", "index-4204-45", "index-4204-46", "index-4204-47",
	"index-4203-00", "index-4203-01", "index-4203-02", "index-4203-03", "index-4203-04", "index-4203-05", "index-4203-06", "index-4203-07", "index-4203-08", "index-4203-09", "index-4203-10", "index-4203-11", "index-4203-12", "index-4203-13", "index-4203-14", "index-4203-15", "index-4203-16", "index-4203-17", "index-4203-18", "index-4203-19", "index-4203-20", "index-4203-21", "index-4203-22", "index-4203-23", "index-4203-24", "index-4203-25", "index-4203-26", "index-4203-27", "index-4203-28", "index-4203-29", "index-4203-30", "index-4203-31", "index-4203-32", "index-4203-33", "index-4203-34", "index-4203-35", "index-4203-36", "index-4203-37", "index-4203-38", "index-4203-39", "index-4203-40", "index-4203-41", "index-4203-42", "index-4203-43", "index-4203-44", "index-4203-45", "index-4203-46", "index-4203-47",
	"index-4202-00", "index-4202-01", "index-4202-02", "index-4202-03", "index-4202-04", "index-4202-05", "index-4202-06", "index-4202-07", "index-4202-08", "index-4202-09", "index-4202-10", "index-4202-11", "index-4202-12", "index-4202-13", "index-4202-14", "index-4202-15", "index-4202-16", "index-4202-17", "index-4202-18", "index-4202-19", "index-4202-20", "index-4202-21", "index-4202-22", "index-4202-23", "index-4202-24", "index-4202-25", "index-4202-26", "index-4202-27", "index-4202-28", "index-4202-29", "index-4202-30", "index-4202-31", "index-4202-32", "index-4202-33", "index-4202-34", "index-4202-35", "index-4202-36", "index-4202-37", "index-4202-38", "index-4202-39", "index-4202-40", "index-4202-41", "index-4202-42", "index-4202-43", "index-4202-44", "index-4202-45", "index-4202-46", "index-4202-47",
	"index-4201-00", "index-4201-01", "index-4201-02", "index-4201-03", "index-4201-04", "index-4201-05", "index-4201-06", "index-4201-07", "index-4201-08", "index-4201-09", "index-4201-10", "index-4201-11", "index-4201-12", "index-4201-13", "index-4201-14", "index-4201-15", "index-4201-16", "index-4201-17", "index-4201-18", "index-4201-19", "index-4201-20", "index-4201-21", "index-4201-22", "index-4201-23", "index-4201-24", "index-4201-25", "index-4201-26", "index-4201-27", "index-4201-28", "index-4201-29", "index-4201-30", "index-4201-31", "index-4201-32", "index-4201-33", "index-4201-34", "index-4201-35", "index-4201-36", "index-4201-37", "index-4201-38", "index-4201-39", "index-4201-40", "index-4201-41", "index-4201-42", "index-4201-43", "index-4201-44", "index-4201-45", "index-4201-46", "index-4201-47",
	"index-4200-00", "index-4200-01", "index-4200-02", "index-4200-03", "index-4200-04", "index-4200-05", "index-4200-06", "index-4200-07", "index-4200-08", "index-4200-09", "index-4200-10", "index-4200-11", "index-4200-12", "index-4200-13", "index-4200-14", "index-4200-15", "index-4200-16", "index-4200-17", "index-4200-18", "index-4200-19", "index-4200-20", "index-4200-21", "index-4200-22", "index-4200-23", "index-4200-24", "index-4200-25", "index-4200-26", "index-4200-27", "index-4200-28", "index-4200-29", "index-4200-30", "index-4200-31", "index-4200-32", "index-4200-33", "index-4200-34", "index-4200-35", "index-4200-36", "index-4200-37", "index-4200-38", "index-4200-39", "index-4200-40", "index-4200-41", "index-4200-42", "index-4200-43", "index-4200-44", "index-4200-45", "index-4200-46", "index-4200-47",
	"index-4119",
	"index-4118",
	"index-4117",
	"index-4116",
	"index-4115",
	"index-4114",
	"index-4113",
	"index-4112",
	"index-4111",
	"index-4110",
	"index-4109",
	"index-4108",
	"index-4107",
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
} astrometry_private_data;

// --------------------------------------------------------------------------------

static bool execute_command(indigo_device *device, char *command, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, command);
	vsnprintf(buffer, sizeof(buffer), command, args);
	va_end(args);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %s", buffer);
	FILE *output = popen(buffer, "r");
	if (output) {
		char *line = NULL;
		size_t size = 0;
		while (getline(&line, &size, output) >= 0) {
			char *nl = strchr(line, '\n');
			if (nl)
				*nl = 0;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %s", line);
			double d1, d2;
			char s[16];
			if (sscanf(line, "simplexy: nx=%d, ny=%d", &ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width, &ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height) == 2) {
			} else if (sscanf(line, "Field center: (RA,Dec) = (%lg, %lg)", &d1, &d2) == 2) {
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = d1 / 15;
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = d2;
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = false;
			} else if (sscanf(line, "Field size: %lg x %lg", &d1, &d2) == 2) {
				AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = d1;
				AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = d2;
				AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = (d1 / ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width + d2 / ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height) / 2;
			} else if (sscanf(line, "Field rotation angle: up is %lg", &d1) == 1) {
				AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = d1;
			} else if (sscanf(line, "Field 1: solved with index index-%lg", &d1) == 1) {
				AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = d1;
			} else if (sscanf(line, "Field parity: %3s", s) == 1) {
				AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = !strcmp(s, "pos") ? 1 : -1;
			} else if (strstr(line, "Total CPU time limit reached")) {
				indigo_send_message(device, "CPU time limit reached");
			} else if (strstr(line, "Did not solve")) {
				indigo_send_message(device, "Did not solve");
			} else if (strstr(line, "You must list at least one index")) {
				indigo_send_message(device, "You must select at least one index");
			}
			if (line) {
				free(line);
				line = NULL;
			}
		}
		pclose(output);
		return true;
	}
	return false;
}

#define astrometry_save_config indigo_platesolver_save_config

static void *astrometry_solve(indigo_platesolver_task *task) {
	indigo_device *device = task->device;
	void *image = task->image;
	unsigned long image_size = task->size;
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		void *intermediate_image = NULL;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		char *base = tempnam(NULL, "image");
#pragma clang diagnostic pop
		// convert any input image to FITS file
		int handle = open(base, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle < 0) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Can't create temporary image file");
			goto cleanup;
		}
		if (!strncmp("SIMPLE", (const char *)image, 6)) {
			// FITS - copy only
			indigo_write(handle, (const char *)image, image_size);
			ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = 0;
		} else {
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
			} else if (!strncmp("JFIF", (const char *)(image + 6), 4)) {
				// JPEG
				struct jpeg_decompress_struct cinfo;
				struct jpeg_error_mgr jerr;
				cinfo.err = jpeg_std_error(&jerr);
				jpeg_create_decompress(&cinfo);
				jpeg_mem_src(&cinfo, image, image_size);
				int rc = jpeg_read_header(&cinfo, TRUE);
				if (rc < 0) {
					AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Broken JPEG file");
					goto cleanup;
				}
				jpeg_start_decompress(&cinfo);
				byte_per_pixel = 1;
				components = cinfo.output_components;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width = cinfo.output_width;
				ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height = cinfo.output_height;
				int row_stride = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width * components;
				image = intermediate_image = malloc(image_size = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height * row_stride);
				while (cinfo.output_scanline < cinfo.output_height) {
					unsigned char *buffer_array[1];
					buffer_array[0] = intermediate_image + (cinfo.output_scanline) * row_stride;
					jpeg_read_scanlines(&cinfo, buffer_array, 1);
				}
				jpeg_finish_decompress(&cinfo);
				jpeg_destroy_decompress(&cinfo);
			} else {
				image = NULL;
			}
			if (image == NULL) {
				AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Unsupported image format");
				close(handle);
				goto cleanup;
			}
			int pixel_count = ASTROMETRY_DEVICE_PRIVATE_DATA->frame_width * ASTROMETRY_DEVICE_PRIVATE_DATA->frame_height;
			image_size = pixel_count * byte_per_pixel + FITS_HEADER_SIZE;
			if (image_size % FITS_HEADER_SIZE) {
				image_size = (image_size / FITS_HEADER_SIZE + 1) * FITS_HEADER_SIZE;
			}
			char *buffer = malloc(image_size), *p = buffer;
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
			p = buffer + FITS_HEADER_SIZE;
			if (components == 1) {
				// mono
				if (byte_per_pixel == 2) {
					// 16 bit RAW - swap endian
					short *in = image;
					short *out = (short *)p;
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
					short *in = image;
					short *out = (short *)p;
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
			free(buffer);
			if (intermediate_image)
				free(intermediate_image);
		}
		close(handle);
		// execute astrometry.net plate solver
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = 0;
		AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Running plate solver on \"%s\" ...", base);
		char path[INDIGO_VALUE_SIZE];
		snprintf(path, sizeof((path)), "%s/.indigo/astrometry/astrometry.cfg", getenv("HOME"));
		handle = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle < 0) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Can't create astrometry.cfg");
			goto cleanup;
		}
		char config[INDIGO_VALUE_SIZE];
		snprintf(config, sizeof(config), "add_path %s/.indigo/astrometry\n", getenv("HOME"));
		indigo_write(handle, config, strlen(config));
		for (int k = 0; k < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; k++) {
			indigo_item *item = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + k;
			if (item->sw.value) {
				for (int l = 0; index_files[l]; l++) {
					if (!strncmp(item->name, index_files[l], 10)) {
						snprintf(config, sizeof(config), "index %s\n", index_files[l]);
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
		if (!execute_command(device, "image2xy -O%s -o %s.xy %s", hints, base, base)) {
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "image2xy failed");
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
		if (AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value > 0) {
			hints_index += sprintf(hints + hints_index, " --cpulimit %d", (int)AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value);
		}
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = true;
		if (!execute_command(device, "solve-field --overwrite --no-plots --no-remove-lines --no-verify-uniformize --sort-column FLUX --uniformize 0%s --config %s/.indigo/astrometry/astrometry.cfg --axy %s.axy %s.xy", hints, getenv("HOME"), base, base))
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed)
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		if (AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE)
			AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
	cleanup:
		execute_command(device, "rm -rf %s %s.xy %s.axy %s.wcs %s.corr %s.match %s.rdls %s.solved %s-indx.xyls", base, base, base, base, base, base, base, base, base);
		free(base);
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solver is busy");
	}
	free(task->image);
	free(task);
	return NULL;
}

static void sync_installed_indexes(indigo_device *device, char *dir, indigo_property *property) {
	char path[INDIGO_VALUE_SIZE];
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		bool add = false;
		bool remove = false;
		for (int j = 0; index_files[j]; j++) {
			char *file_name = index_files[j];
			if (!strncmp(file_name, item->name, 10)) {
				snprintf(path, sizeof((path)), "%s/.indigo/astrometry/%s.fits", getenv("HOME"), file_name);
				if (item->sw.value) {
					if (access(path, F_OK) == 0) {
						continue;
					}
					indigo_send_message(device, "Downloading %s...", file_name);
					if (!execute_command(device, "curl -L -s -o %s http://data.astrometry.net/%s/%s.fits", path, dir, file_name)) {
						property->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, property, strerror(errno));
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
							return;
						}
						remove = true;
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
					memcpy(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + j, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + (j + 1), (AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count - j) * sizeof(indigo_item));
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
	property->state = INDIGO_OK_STATE;
	indigo_update_property(device, property, NULL);
}

static void index_41xx_handler(indigo_device *device) {
	sync_installed_indexes(device, "4100", AGENT_ASTROMETRY_INDEX_41XX_PROPERTY);
}

static void index_42xx_handler(indigo_device *device) {
	sync_installed_indexes(device, "4200", AGENT_ASTROMETRY_INDEX_42XX_PROPERTY);
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
		if (AGENT_ASTROMETRY_INDEX_41XX_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 19; i >=7; i--) {
			sprintf(name, "index-41%02d", i);
			if (index_diameters[i][0] > 60)
				sprintf(label, "Index 41%02d (%.0f-%.0fº)", i, index_diameters[i][0] / 60, index_diameters[i][1] / 60);
			else
				sprintf(label, "Index 41%02d (%.0f-%.0f\')", i, index_diameters[i][0], index_diameters[i][1]);
			present = true;
			for (int j = 0; index_files[j]; j++) {
				char *file_name = index_files[j];
				if (!strncmp(file_name, name, 10)) {
					snprintf(path, sizeof((path)), "%s/.indigo/astrometry/%s.fits", getenv("HOME"), file_name);
					if (access(path, F_OK)) {
						present = false;
						break;
					}
				}
			}
			indigo_init_switch_item(AGENT_ASTROMETRY_INDEX_41XX_PROPERTY->items - (i - 19), name, label, present);
			if (present)
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, label, false);
		}
		AGENT_ASTROMETRY_INDEX_42XX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ASTROMETRY_INDEX_42XX_PROPERTY_NAME, "Index managememt", "Installed 2MASS catalog indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 20);
		if (AGENT_ASTROMETRY_INDEX_42XX_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 19; i >=0; i--) {
			sprintf(name, "index-42%02d", i);
			if (index_diameters[i][0] > 60)
				sprintf(label, "Index 42%02d (%.0f-%.0fº)", i, index_diameters[i][0] / 60, index_diameters[i][1] / 60);
			else
				sprintf(label, "Index 42%02d (%.0f-%.0f\')", i, index_diameters[i][0], index_diameters[i][1]);
			present = true;
			for (int j = 0; index_files[j]; j++) {
				char *file_name = index_files[j];
				if (!strncmp(file_name, name, 10)) {
					snprintf(path, sizeof((path)), "%s/.indigo/astrometry/%s.fits", getenv("HOME"), file_name);
					if (access(path, F_OK)) {
						present = false;
						break;
					}
				}
			}
			indigo_init_switch_item(AGENT_ASTROMETRY_INDEX_42XX_PROPERTY->items - (i - 19), name, label, present);
			if (present)
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, label, false);
		}
		// --------------------------------------------------------------------------------
		ASTROMETRY_DEVICE_PRIVATE_DATA->platesolver.save_config = astrometry_save_config;
		ASTROMETRY_DEVICE_PRIVATE_DATA->platesolver.solve = astrometry_solve;
		snprintf(path, sizeof((path)), "%s/.indigo/astrometry/", getenv("HOME"));
		mkdir(path, 0777);
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
			last_action = action;
			void *private_data = malloc(sizeof(astrometry_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(astrometry_private_data));
			agent_device = malloc(sizeof(indigo_device));
			assert(agent_device != NULL);
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
			memcpy(agent_client, &agent_client_template, sizeof(indigo_client));
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
