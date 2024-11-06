#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

void* read_indigo_raw(const char* filename, indigo_raw_header* header) {
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		perror("Cannot open input file");
		return NULL;
	}
	fread(header, sizeof(indigo_raw_header), 1, file);
	uint32_t bytes_per_pixel = (header->signature == INDIGO_RAW_MONO16 || header->signature == INDIGO_RAW_RGB48) ? 2 : 1;
	uint32_t channels = (header->signature == INDIGO_RAW_RGB24 || header->signature == INDIGO_RAW_RGB48) ? 3 : 1;
	void* data = malloc(header->width * header->height * bytes_per_pixel * channels);
	fread(data, header->width * header->height * bytes_per_pixel * channels, 1, file);
	fclose(file);
	return data;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("Usage: %s input.raw output.raw x,y,width,height\n", argv[0]);
		return 1;
	}

	int x, y, width, height;
	if (sscanf(argv[3], "%d,%d,%d,%d", &x, &y, &width, &height) != 4) {
		printf("Invalid ROI format. Use x,y,width,height\n");
		return 1;
	}

	indigo_raw_header header;
	void* data = read_indigo_raw(argv[1], &header);
	if (data == NULL) {
		return 1;
	}

	if (x < 0 || y < 0 || x + width > header.width || y + height > header.height) {
		printf("Invalid ROI parameters\n");
		free(data);
		return 1;
	}

	uint32_t bytes_per_pixel = (header.signature == INDIGO_RAW_MONO16 || header.signature == INDIGO_RAW_RGB48) ? 2 : 1;
	uint32_t channels = (header.signature == INDIGO_RAW_RGB24 || header.signature == INDIGO_RAW_RGB48) ? 3 : 1;
	void* roi_data = malloc(width * height * bytes_per_pixel * channels);
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			memcpy((uint8_t*)roi_data + (j * width + i) * bytes_per_pixel * channels,
				   (uint8_t*)data + ((y + j) * header.width + (x + i)) * bytes_per_pixel * channels,
				   bytes_per_pixel * channels);
		}
	}

	write_indigo_raw(argv[2], header.signature, width, height, roi_data);

	free(data);
	free(roi_data);
	return 0;
}