#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fitsio.h>
#include "indigo/indigo_bus.h"

static bool write_indigo_raw(const char *filename, indigo_raw_type type, uint32_t width, uint32_t height, const void *data, size_t size) {
	FILE *file = fopen(filename, "wb");
	if (file == NULL) {
		perror("Cannot open output file");
		return false;
	}
	indigo_raw_header header = { type, width, height };
	bool ok = fwrite(&header, sizeof(header), 1, file) == 1 && fwrite(data, 1, size, file) == size && !ferror(file);
	if (fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s input.fits output.raw\n", argv[0]);
		return 1;
	}
	fitsfile *fits = NULL;
	int status = 0, close_status = 0, result = 1;
	void *data = NULL, *plane = NULL;
	if (fits_open_file(&fits, argv[1], READONLY, &status)) {
		fits_report_error(stderr, status);
		return 1;
	}
	int bitpix, naxis;
	long naxes[3] = { 0, 0, 0 };
	fits_get_img_param(fits, 3, &bitpix, &naxis, naxes, &status);
	if (status != 0) {
		goto cleanup;
	}
	if ((bitpix != 8 && bitpix != 16) || (naxis != 2 && naxis != 3) || naxes[0] < 1 || naxes[1] < 1 || (uintmax_t)naxes[0] > UINT32_MAX || (uintmax_t)naxes[1] > UINT32_MAX || (naxis == 3 && naxes[2] != 3)) {
		fprintf(stderr, "Unsupported FITS image dimensions or pixel type\n");
		goto cleanup;
	}
	size_t bytes = bitpix == 16 ? 2 : 1, channels = naxis == 3 ? 3 : 1;
	if ((uintmax_t)naxes[0] > SIZE_MAX / bytes / channels / (uintmax_t)naxes[1] || naxes[0] > LONG_MAX / naxes[1]) {
		fprintf(stderr, "FITS image is too large\n");
		goto cleanup;
	}
	size_t pixels = (size_t)naxes[0] * naxes[1], size = pixels * bytes * channels;
	data = malloc(size);
	if (data == NULL) {
		fprintf(stderr, "Cannot allocate image buffer\n");
		goto cleanup;
	}
	if (naxis == 3) {
		plane = malloc(pixels * bytes);
		if (plane == NULL) {
			fprintf(stderr, "Cannot allocate image plane\n");
			goto cleanup;
		}
		for (int i = 0; i < 3; i++) {
			long fpixel[3] = { 1, 1, i + 1 };
			fits_read_pix(fits, bitpix == 16 ? TUSHORT : TBYTE, fpixel, (long)pixels, NULL, plane, NULL, &status);
			if (status != 0) {
				goto cleanup;
			}
			for (size_t j = 0; j < pixels; j++) {
				memcpy((uint8_t *)data + (j * 3 + i) * bytes, (uint8_t *)plane + j * bytes, bytes);
			}
		}
	} else {
		fits_read_img(fits, bitpix == 16 ? TUSHORT : TBYTE, 1, (long)pixels, NULL, data, NULL, &status);
		if (status != 0) {
			goto cleanup;
		}
	}
	indigo_raw_type type = naxis == 3 ? (bitpix == 8 ? INDIGO_RAW_RGB24 : INDIGO_RAW_RGB48) : (bitpix == 8 ? INDIGO_RAW_MONO8 : INDIGO_RAW_MONO16);
	if (!write_indigo_raw(argv[2], type, naxes[0], naxes[1], data, size)) {
		fprintf(stderr, "Failed to write RAW image\n");
		goto cleanup;
	}
	result = 0;
cleanup:
	free(plane);
	free(data);
	fits_close_file(fits, &close_status);
	if (status != 0 || close_status != 0) {
		fits_report_error(stderr, status != 0 ? status : close_status);
		result = 1;
	}
	return result;
}
