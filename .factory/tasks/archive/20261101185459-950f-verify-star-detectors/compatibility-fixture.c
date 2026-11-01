#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include RAW_SOURCE
static uint32_t seed = 12345;
static unsigned random_value(void) {
	seed = seed * 1664525u + 1013904223u;
	return seed;
}
#ifdef CHECK_MEDIAN
static int compare_u16(const void *a, const void *b) {
	return (*(const uint16_t *)a > *(const uint16_t *)b) - (*(const uint16_t *)a < *(const uint16_t *)b);
}
static void check_medians(void) {
	uint16_t values[1024], sorted[1024];
	for (int count = 1; count <= 1024; count++) {
		for (int pattern = 0; pattern < 8; pattern++) {
			for (int i = 0; i < count; i++) {
				unsigned value = pattern == 0 ? 0 : pattern == 1 ? UINT16_MAX : pattern == 2 ? i : pattern == 3 ? count - i : pattern == 4 ? random_value() % 3 : pattern == 5 ? random_value() % 65536 : pattern == 6 ? i % 2 * UINT16_MAX : abs(count / 2 - i);
				values[i] = sorted[i] = value;
			}
			qsort(sorted, count, sizeof(*sorted), compare_u16);
			assert(uint16_median(values, count) == sorted[count / 2]);
		}
	}
}
#endif
int main(void) {
#ifdef CHECK_MEDIAN
	check_medians();
	seed = 12345;
#endif
	const indigo_raw_type formats[] = { INDIGO_RAW_MONO8, INDIGO_RAW_MONO16, INDIGO_RAW_RGB24, INDIGO_RAW_RGBA32, INDIGO_RAW_ABGR32, INDIGO_RAW_RGB48 };
	const int radii[] = { 0, 1, 3, 16 };
	int nonempty = 0;
	for (int trial = 0; trial < 12; trial++) {
		uint16_t plane[96 * 96];
		for (int i = 0; i < 96 * 96; i++) {
			plane[i] = 10 + random_value() % 5;
		}
		for (int star = 0; star < 6; star++) {
			int x = 24 + random_value() % 48, y = 24 + random_value() % 48;
			for (int dy = -2; dy <= 2; dy++) {
				for (int dx = -2; dx <= 2; dx++) {
					plane[(y + dy) * 96 + x + dx] = 240 - 32 * (abs(dx) + abs(dy));
				}
			}
		}
		for (int f = 0; f < 6; f++) {
			uint8_t bytes[96 * 96 * 6];
			uint16_t words[96 * 96 * 3];
			for (int i = 0; i < 96 * 96; i++) {
				if (f == 1 || f == 5) {
					for (int c = 0; c < (f == 5 ? 3 : 1); c++) {
						words[i * (f == 5 ? 3 : 1) + c] = plane[i] * 200;
					}
				} else {
					int channels = f == 0 ? 1 : f == 2 ? 3 : 4;
					for (int c = 0; c < channels; c++) {
						bytes[i * channels + c] = plane[i];
					}
				}
			}
			for (int r = 0; r < 4; r++) {
				indigo_star_detection stars[8] = { 0 };
				int count = -1;
				int result = indigo_find_stars_precise_threshold(formats[f], f == 1 || f == 5 ? (void *)words : (void *)bytes, radii[r], 5.0, 96, 96, 8, stars, &count);
				assert(result == INDIGO_OK && count >= 0 && count <= 8);
				nonempty += count > 0;
				printf("%d %d %d %d %d\n", trial, f, radii[r], result, count);
				for (int i = 0; i < count; i++) {
					printf("%.17g %.17g %.17g %.17g %d %d\n", stars[i].x, stars[i].y, stars[i].luminance, stars[i].nc_distance, stars[i].close_to_other, stars[i].oversaturated);
				}
			}
		}
	}
	assert(nonempty > 0);
	return 0;
}
