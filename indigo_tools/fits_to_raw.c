#include <stdio.h>
#include <stdint.h>
#include <fitsio.h>
#include "indigo/indigo_bus.h"

void write_indigo_raw(const char* filename, indigo_raw_type type, uint32_t width, uint32_t height, void* data) {
	FILE* file = fopen(filename, "wb");
	if (file == NULL) {
		perror("Cannot open output file");
		return;
	}
	indigo_raw_header header = { type, width, height };
	fwrite(&header, sizeof(header), 1, file);
	switch (type) {
		case INDIGO_RAW_MONO8:
			fwrite(data, width * height, 1, file);
			break;
		case INDIGO_RAW_RGB24:
			fwrite(data, width * height * 3, 1, file);
			break;
		case INDIGO_RAW_MONO16:
			fwrite(data, width * height * 2, 1, file);
			break;
		case INDIGO_RAW_RGB48:
			fwrite(data, width * height * 6, 1, file);
			break;
		default:
			printf("Unsupported type\n");
			break;
	}
	fclose(file);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("Usage: %s input.fits output.raw\n", argv[0]);
		return 1;
	}

	fitsfile* fits;
	int status = 0;
	if (fits_open_file(&fits, argv[1], READONLY, &status)) {
		fits_report_error(stderr, status);
		return 1;
	}

	int bitpix, naxis;
	long naxes[3] = { 0, 0, 0 };
	fits_get_img_param(fits, 3, &bitpix, &naxis, naxes, &status);

	if(naxis != 2 && naxis != 3) {
		printf("Unsupported number of axes\n");
		fits_close_file(fits, &status);
		return 1;
	}

	indigo_raw_type type;
	if (bitpix == 8) {
		type = naxis == 3 ? INDIGO_RAW_RGB24 : INDIGO_RAW_MONO8;
	} else {
		type = naxis == 3 ? INDIGO_RAW_RGB48 : INDIGO_RAW_MONO16;
	}

	int bytes_per_pixel = (type == INDIGO_RAW_MONO16 || type == INDIGO_RAW_RGB48) ? 2 : 1;
	void* data = malloc(naxes[0] * naxes[1] * (type == INDIGO_RAW_MONO16 || type == INDIGO_RAW_RGB48 ? 2 : 1) * (naxis == 3 ? 3 : 1));
	if (naxis == 3) {
		for (int i = 0; i < naxes[2]; i++) {
			long fpixel[3] = { 1, 1, i + 1 };
			void* plane_data = malloc(naxes[0] * naxes[1] * bytes_per_pixel);
			fits_read_pix(fits, bitpix == 16 ? TUSHORT : TBYTE, fpixel, naxes[0] * naxes[1], NULL, plane_data, NULL, &status);
			for (int j = 0; j < naxes[0] * naxes[1]; j++) {
				memcpy((uint8_t*)data + j * bytes_per_pixel * 3 + i * bytes_per_pixel, (uint8_t*)plane_data + j * bytes_per_pixel, bytes_per_pixel);
			}
			free(plane_data);
		}
	} else {
		fits_read_img(fits, bitpix == 16 ? TUSHORT : TBYTE, 1, naxes[0] * naxes[1], NULL, data, NULL, &status);
	}

	write_indigo_raw(argv[2], type, naxes[0], naxes[1], data);

	free(data);
	fits_close_file(fits, &status);

	return 0;
}