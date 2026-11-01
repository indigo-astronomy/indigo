#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>
int main(int argc, char **argv) {
	const int width = argc > 1 && strstr(argv[1], "edge") != NULL ? 144 : 64;
	const int height = argc > 1 && strstr(argv[1], "edge") != NULL ? 3 : 64;
	const int radius = argc > 1 && (strcmp(argv[1], "radius") == 0 || strcmp(argv[1], "precise") == 0) ? 32768 : 3;
	uint16_t *pixels = calloc((size_t)width * height, sizeof(*pixels));
	assert(pixels);
	for (int i = 0; i < width * height; i++) {
		pixels[i] = 10;
	}
	if (height == 3) {
		pixels[0] = 6000;
	}
	indigo_star_detection stars[8];
	int count = -1;
	indigo_result result = (argc > 1 && strstr(argv[1], "precise") != NULL ? indigo_find_stars_precise_threshold : indigo_find_stars_fast_threshold)(INDIGO_RAW_MONO16, pixels, radius, 5.0, width, height, 8, stars, &count);
	printf("result=%d count=%d\n", result, count);
	free(pixels);
	return 0;
}
