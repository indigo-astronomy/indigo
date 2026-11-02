// Copyright (c) 2021-2025 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <glob.h>
#include <sys/stat.h>
#include <limits.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_fits.h>

#define MAX_EXTRA_KEYWORDS 3

#include "tool_file_io.h"

int save_file(char *file_name, char *data, int size) {
	if (size < 0) {
		errno = EINVAL;
		return -1;
	}
#if defined(INDIGO_WINDOWS)
	int handle = open(file_name, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0);
#else
	int handle = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
	if (handle < 0) {
		return -1;
	}
	int offset = 0;
	while (offset < size) {
		int written = (int)write(handle, data + offset, (size_t)(size - offset));
		if (written < 0 && errno == EINTR) {
			continue;
		}
		if (written <= 0) {
			close(handle);
			return -1;
		}
		offset += (int)written;
	}
	return close(handle) == 0 ? 0 : -1;
}

int open_file(const char *file_name, char **data, int *size) {
	char *replacement = NULL;
	size_t length;
	*size = 0;
	if (!tool_read_file(file_name, &replacement, &length, INT_MAX)) {
		return -1;
	}
	free(*data);
	*data = replacement;
	*size = (int)length;
	return 0;
}

static void print_help(const char *name) {
	printf("INDIGO RAW to FITS conversion tool v.%d.%d-%s built on %s %s.\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);
	printf("usage: %s [options] file1.raw [file2.raw ...]\n", name);
	printf("output files will have '.fits' suffix.\n");
	printf("options:\n"
	       "       -e  | --exposure-time seconds  : add EXPTIME keyword with the given value to the header\n"
	       "       -t  | --ccd-temperature degC   : add CCD-TEMP keyword with the given value to the header\n"
	       "       -b  | --bayer-pattern pattern  : add BAYERPAT keyword (pattern = BGGR | GBRG | GRBG | RGGB)\n"
	       "       -h  | --help                   : print this help\n"
	       "       -q  | --quiet                  : print only errors and statistics\n"
	);
}

int main(int argc, char *argv[]) {
	indigo_fits_keyword extra_keywords[MAX_EXTRA_KEYWORDS + 1] = {0};
	int extra_keywords_count = 0;

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
		} else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet")) {
			not_quiet = false;
		} else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--exposure-time")) {
			if (argc > i+1) {
				i++;
				arg_base++;
				indigo_fits_keyword keyword = {INDIGO_FITS_NUMBER, "EXPTIME", .number = atof(argv[i]), "exposure time [s]"};
				extra_keywords[extra_keywords_count++] = keyword;
			} else {
				fprintf(stderr, "No exposure time specified\n");
				return 1;
			}
		} else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bayer-pattern")) {
			if (argc > i+1) {
				i++;
				arg_base++;
				if (!strcmp(argv[i], "BGGR") || !strcmp(argv[i], "GBRG") || !strcmp(argv[i], "GRBG") || !strcmp(argv[i], "RGGB")) {
					indigo_fits_keyword keyword = {INDIGO_FITS_STRING, "BAYERPAT", .string = argv[i], "Bayer color pattern" };
					extra_keywords[extra_keywords_count++] = keyword;
				} else {
					fprintf(stderr, "No valid Bayer pattern specidied\n");
					return 1;
				}
			} else {
				fprintf(stderr, "No Bayer pattern specidied\n");
				return 1;
			}

		} else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--ccd-temperture")) {
			if (argc > i+1) {
				i++;
				arg_base++;
				indigo_fits_keyword keyword = {INDIGO_FITS_NUMBER, "CCD-TEMP", .number = atof(argv[i]), "CCD temperature [C]"};
				extra_keywords[extra_keywords_count++] = keyword;
			} else {
				fprintf(stderr, "No CCD temperture specified\n");
				return 1;
			}
		} else if (argv[i][0] != '-') {
			break;
		}
		arg_base++;

		if (extra_keywords_count > MAX_EXTRA_KEYWORDS) {
			fprintf(stderr, "Too many extra keywords\n");
			return 1;
		}
	}

	int succeeded = 0;
	int failed = 0;
	for (int i = arg_base; i < argc; i++) {
		glob_t globlist = { 0 };
		int glob_result = glob(argv[i], 0, NULL, &globlist);
		if (glob_result != 0) {
			fprintf(stderr, "Cannot expand pattern '%s'\n", argv[i]);
			globfree(&globlist);
			failed++;
			continue;
		}

		int k = 0;
		while (globlist.gl_pathv[k]) {
			char *in_data = NULL;
			char *out_data = NULL;
			int out_data_size = 0, in_data_size = 0;
			char infile_name[PATH_MAX];

			if (snprintf(infile_name, sizeof(infile_name), "%s", globlist.gl_pathv[k]) >= sizeof(infile_name)) {
				fprintf(stderr, "Input path is too long\n");
				failed++;
				k++;
				continue;
			}
			int res = open_file(globlist.gl_pathv[k], &in_data, &in_data_size);
			if (res != 0) {
				fprintf(stderr, "Can not open '%s' : %s\n", infile_name, strerror(errno));
				if (in_data) free(in_data);
				failed++;
				k++;
				continue;
			}
			res = indigo_raw_to_fits(in_data, in_data_size, &out_data, &out_data_size, extra_keywords);
			if (res != INDIGO_OK) {
				fprintf(stderr, "Can not convert '%s' to FITS: %s\n", infile_name, strerror(errno));
				if (in_data) free(in_data);
				if (out_data) free(out_data);
				failed++;
				k++;
				continue;
			}

			char outfile_name[PATH_MAX];
			char *dot = strrchr(infile_name, '.');
			char *slash = strrchr(infile_name, '/');
			if (dot != NULL && slash != NULL && dot < slash) {
				dot = NULL;
			}
			size_t stem = dot != NULL ? (size_t)(dot - infile_name) : strlen(infile_name);
			if (stem > sizeof(outfile_name) - sizeof(".fits")) {
				fprintf(stderr, "Output path is too long\n");
				free(in_data);
				free(out_data);
				failed++;
				k++;
				continue;
			}
			memcpy(outfile_name, infile_name, stem);
			memcpy(outfile_name + stem, ".fits", sizeof(".fits"));

			res = save_file(outfile_name, out_data, out_data_size);
			if (res != 0) {
				fprintf(stderr, "Can not save '%s': %s\n", outfile_name, strerror(errno));
				failed++;
			} else {
				not_quiet && printf("Converted '%s' -> '%s'\n", infile_name, outfile_name);
				succeeded++;
			}
			if (in_data) free(in_data);
			if (out_data) free(out_data);
			k++;
		}
		globfree(&globlist);
	}
	fprintf(stderr, "%3d raw files sucessfully converted\n", succeeded);
	if (failed) {
		fprintf(stderr, "%3d files failed.\n", failed);
		return 1;
	}
	return 0;
}
