// Copyright (c) 2021 Rumen G. Bogdanovski
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
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#define FITS_HEADER_SIZE 2880

int indigo_raw_to_fists(char *image, char **fits, int *size) {
	int byte_per_pixel = 0, components = 0;
	int frame_width = 0, frame_height = 0;
	if (!strncmp("RAW1", (const char *)(image), 4)) {
		// 8 bit RAW
		byte_per_pixel = 1;
		components = 1;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else if (!strncmp("RAW2", (const char *)(image), 4)) {
		// 16 bit RAW
		byte_per_pixel = 2;
		components = 1;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else if (!strncmp("RAW3", (const char *)(image), 4)) {
		// 8 bit RGB
		byte_per_pixel = 1;
		components = 3;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else if (!strncmp("RAW6", (const char *)(image), 4)) {
		// 16 bit RGB
		byte_per_pixel = 2;
		components = 3;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else {
		errno = EINVAL;
		return -1;
	}

	int pixel_count = frame_width * frame_height;
	int image_size = pixel_count * byte_per_pixel * components + FITS_HEADER_SIZE;
	if (image_size % FITS_HEADER_SIZE) {
		image_size = (image_size / FITS_HEADER_SIZE + 1) * FITS_HEADER_SIZE;
	}

	*fits = realloc(*fits, image_size);
	char *buffer = *fits;
	if (buffer == NULL) {
		return -1;
	}

	char *p = buffer;
	memset(buffer, ' ', image_size);

	int t = sprintf(p, "SIMPLE  = %20c", 'T'); p[t] = ' ';
	t = sprintf(p += 80, "BITPIX  = %20d", byte_per_pixel * 8); p[t] = ' ';
	if (components > 1) {
		t = sprintf(p += 80, "NAXIS   = %20d", 3); p[t] = ' ';
	} else {
		t = sprintf(p += 80, "NAXIS   = %20d", 2); p[t] = ' ';
	}
	t = sprintf(p += 80, "NAXIS1  = %20d", frame_width); p[t] = ' ';
	t = sprintf(p += 80, "NAXIS2  = %20d", frame_height); p[t] = ' ';
	if (components > 1) {
		t = sprintf(p += 80, "NAXIS3  = %20d", components); p[t] = ' ';
	}
	t = sprintf(p += 80, "EXTEND  = %20c", 'T'); p[t] = ' ';
	if (byte_per_pixel == 2) {
		t = sprintf(p += 80, "BZERO   = %20d", 32768); p[t] = ' ';
		t = sprintf(p += 80, "BSCALE  = %20d", 1); p[t] = ' ';
	}
	t = sprintf(p += 80, "ROWORDER= %20s / Image row order", "'TOP-DOWN'"); p[t] = ' ';
	t = sprintf(p += 80, "END"); p[t] = ' ';
	p = buffer + FITS_HEADER_SIZE;
	if (components == 1) {
		// mono
		if (byte_per_pixel == 2) {
			// 16 bit RAW - swap endianness
			uint16_t *in = (uint16_t *)image;
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
			uint16_t *out_ch0 = (uint16_t *)p;
			uint16_t *out_ch1 = (uint16_t *)p + pixel_count;
			uint16_t *out_ch2 = (uint16_t *)p + 2 * pixel_count;
			// 16 bit RGB - average and swap endianness
			uint16_t *in = (uint16_t *)image;
			for (int i = 0; i < pixel_count * 3; i++) {
				int value =  *in++ - 32768;
				*out_ch0++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				value =  *in++ - 32768;
				*out_ch1++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				value =  *in++ - 32768;
				*out_ch2++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
			}
		} else {
			uint8_t *out_ch0 = (uint8_t *)p;
			uint8_t *out_ch1 = (uint8_t *)p + pixel_count;
			uint8_t *out_ch2 = (uint8_t *)p + 2 * pixel_count;
			uint8_t *in = image;
			for (int i = 0; i < pixel_count; i++) {
				*out_ch0++ = *in++;
				*out_ch1++ = *in++;
				*out_ch2++ = *in++;
			}
		}
	}
	*fits = buffer;
	*size = image_size;
	return 0;
}

int save_file(char *file_name, char *data, int size) {
	int handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (handle < 0) {
		return -1;
	}
	indigo_write(handle, data, size);
	close(handle);
	return 0;
}

int open_file(const char *file_name, char **data, int *size) {
	char msg[PATH_MAX];
	int image_size = 0;
	if (file_name == "") {
		errno = ENOENT;
		return -1;
	}
	FILE *file;
	file = fopen(file_name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		image_size = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);
		if (*data == NULL) {
			*data = (char *)malloc(image_size + 1);
		} else {
			*data = (char *)realloc(*data, image_size + 1);
		}
		*size = fread(*data, image_size, 1, file);
		fclose(file);
		*size = image_size;
		return 0;
	}
	return -1;
}

static void print_help(const char *name) {
	printf("INDIGO RAW to FITS conversion tool v.%d.%d-%s built on %s %s.\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);
	printf("usage: %s [options] file1.raw [file2.raw ...]\n", name);
	printf("output files will have '.fits' suffix.\n");
	printf("options:\n"
	       "       -h  | --help\n"
		   "       -q  | --quiet\n"
	);
}

int main(int argc, char *argv[]) {
	bool not_quiet = true;
	if (argc < 2) {
		print_help(argv[0]);
		return 0;
	}

	int arg_base = 1;
	for (int i = arg_base; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "-quiet"))
			not_quiet = false;
	}


	for (int i = arg_base; i < argc; i++) {
		char *in_data = NULL;
		char *out_data = NULL;
		int size = 0;
		char infile_name[PATH_MAX];

		strncpy(infile_name, argv[i], PATH_MAX);
		int res = open_file(argv[i], &in_data, &size);
		if (res != 0) {
			not_quiet && fprintf(stderr, "Can not open '%s' : %s\n", infile_name, strerror(errno));
			if (in_data) free(in_data);
			return 1;
		}
		res = indigo_raw_to_fists(in_data, &out_data, &size);
		if (res != 0) {
			not_quiet && fprintf(stderr, "Can not convert '%s' to FITS: %s\n", infile_name, strerror(errno));
			if (in_data) free(in_data);
			if (out_data) free(out_data);
			return 1;
		}

		char outfile_name[PATH_MAX];
		strncpy(outfile_name, infile_name, PATH_MAX);
		/* relace replace suffix with .fits */
		char *dot = strrchr(outfile_name, '.');
		if (dot) {
			strcpy(dot, ".fits");
		} else {
			snprintf(outfile_name, PATH_MAX, "%s.fits", infile_name);
		}

		res = save_file(outfile_name, out_data, size);
		if (res != 0) {
			not_quiet && fprintf(stderr, "Can not save '%s': %s\n", outfile_name, strerror(errno));
		} else {
			not_quiet && fprintf(stderr, "Converted '%s' -> '%s'\n", infile_name, outfile_name);
		}
		if (in_data) free(in_data);
		if (out_data) free(out_data);
	}
	return 0;
}
