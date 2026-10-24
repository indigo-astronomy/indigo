#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "indigo/indigo_bus.h"

static size_t pixel_size(indigo_raw_type type) {
	switch (type) {
		case INDIGO_RAW_MONO8:
			return 1;
		case INDIGO_RAW_MONO16:
			return 2;
		case INDIGO_RAW_RGB24:
			return 3;
		case INDIGO_RAW_RGB48:
			return 6;
		default:
			return 0;
	}
}

static bool image_size(uint32_t width, uint32_t height, size_t pixel_bytes, size_t *bytes) {
	if (width == 0 || height == 0 || pixel_bytes == 0 || width > SIZE_MAX / pixel_bytes || height > SIZE_MAX / ((size_t)width * pixel_bytes)) {
		return false;
	}
	*bytes = (size_t)width * height * pixel_bytes;
	return true;
}

static bool write_indigo_raw(const char *filename, indigo_raw_type type, uint32_t width, uint32_t height, const void *data, size_t bytes) {
	FILE *file = fopen(filename, "wb");
	if (file == NULL) {
		perror("Cannot open output file");
		return false;
	}
	indigo_raw_header header = { type, width, height };
	bool success = fwrite(&header, sizeof(header), 1, file) == 1 && fwrite(data, 1, bytes, file) == bytes;
	if (fclose(file) != 0) {
		success = false;
	}
	if (!success) {
		fprintf(stderr, "Cannot write complete output file\n");
	}
	return success;
}

static void *read_indigo_raw(const char *filename, indigo_raw_header *header) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		perror("Cannot open input file");
		return NULL;
	}
	size_t bytes;
	if (fread(header, sizeof(*header), 1, file) != 1 || !image_size(header->width, header->height, pixel_size(header->signature), &bytes)) {
		fprintf(stderr, "Invalid RAW header\n");
		fclose(file);
		return NULL;
	}
	long payload_start = ftell(file);
	if (payload_start < 0 || fseek(file, 0, SEEK_END) != 0) {
		fprintf(stderr, "Cannot determine RAW payload size\n");
		fclose(file);
		return NULL;
	}
	long end = ftell(file);
	if (end < payload_start || (uintmax_t)(end - payload_start) < bytes || fseek(file, payload_start, SEEK_SET) != 0) {
		fprintf(stderr, "Incomplete RAW payload\n");
		fclose(file);
		return NULL;
	}
	void *data = malloc(bytes);
	if (data == NULL) {
		fprintf(stderr, "Cannot allocate RAW image\n");
		fclose(file);
		return NULL;
	}
	bool success = fread(data, 1, bytes, file) == bytes;
	if (fclose(file) != 0) {
		success = false;
	}
	if (!success) {
		fprintf(stderr, "Cannot read complete RAW payload\n");
		free(data);
		return NULL;
	}
	return data;
}

static bool parse_roi(const char *text, uint32_t roi[4]) {
	for (int i = 0; i < 4; i++) {
		while (isspace((unsigned char)*text)) {
			text++;
		}
		if (*text == '-') {
			return false;
		}
		char *end;
		errno = 0;
		unsigned long value = strtoul(text, &end, 10);
		if (errno || end == text || value > UINT32_MAX || (i >= 2 && value == 0) || *end != (i == 3 ? '\0' : ',')) {
			return false;
		}
		roi[i] = (uint32_t)value;
		text = end + (i == 3 ? 0 : 1);
	}
	return true;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s input.raw output.raw x,y,width,height\n", argv[0]);
		return 1;
	}
	uint32_t roi[4];
	if (!parse_roi(argv[3], roi)) {
		fprintf(stderr, "Invalid ROI format. Use x,y,width,height with positive dimensions\n");
		return 1;
	}
	indigo_raw_header header;
	void *data = read_indigo_raw(argv[1], &header);
	if (data == NULL) {
		return 1;
	}
	uint32_t x = roi[0], y = roi[1], width = roi[2], height = roi[3];
	size_t pixel_bytes = pixel_size(header.signature), bytes;
	if (x >= header.width || y >= header.height || width > header.width - x || height > header.height - y || !image_size(width, height, pixel_bytes, &bytes)) {
		fprintf(stderr, "Invalid ROI parameters\n");
		free(data);
		return 1;
	}
	void *roi_data = malloc(bytes);
	if (roi_data == NULL) {
		fprintf(stderr, "Cannot allocate RAW crop\n");
		free(data);
		return 1;
	}
	size_t row_bytes = (size_t)width * pixel_bytes;
	for (uint32_t j = 0; j < height; j++) {
		memcpy((uint8_t *)roi_data + (size_t)j * row_bytes, (uint8_t *)data + ((size_t)(y + j) * header.width + x) * pixel_bytes, row_bytes);
	}
	bool success = write_indigo_raw(argv[2], header.signature, width, height, roi_data, bytes);
	free(data);
	free(roi_data);
	return success ? 0 : 1;
}
