//
//  solver_test.c
//  indigo
//
//  Created by Peter Polakovic on 10/01/2021.
//  Copyright © 2021 CloudMakers, s. r. o. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>

#define max_stars 500
#define top_stars 100
#define raw_image_width 1600
#define raw_image_height 1200
#define raw_type INDIGO_RAW_MONO16
#define centroid_radius 8

extern unsigned short raw_image[];
static int stars_found = 0;
static indigo_star_detection stars[max_stars];
static indigo_frame_digest digest;

typedef struct {
	int index[4];
	double mean_x, mean_y;
	double hash[6];
} indigo_quad;

#define max_quads 500

static int quads_found = 0;
static indigo_quad quads[max_quads];

static int double_comparator(const void *item_1, const void *item_2) {
	if ((double *)item_1 < (double*)item_2)
		return 1;
	if ((double *)item_1 > (double*)item_2)
		return -1;
	return 0;
}


indigo_result indigo_find_quads(indigo_star_detection *stars, int stars_count, indigo_quad *quads, int *quads_found) {
	int last_quad = 0;
	int index_1 = 0, index_2 = 0, index_3 = 0;
	double distance[6] = { DBL_MAX };
	double x, y;
	for (int index_0 = 0; index_0 < stars_count; index_0++) {
		for (int i = 0; i < stars_count; i++) {
			if (index_0 == i)
				continue;
			x = stars[index_0].x - stars[i].x;
			y = stars[index_0].y - stars[i].y;
			double d = sqrt(x * x + y * y);
			if (d < distance[0]) {
				distance[2] = distance[1]; distance[1] = distance[0]; distance[0] = d;
				index_3 = index_2; index_2 = index_1; index_1 = i;
			} else if (d < distance[1]) {
				distance[2] = distance[1]; distance[1] = d;
				index_3 = index_2; index_2 = i;
			} else if (d < distance[2]) {
				distance[2] = d;
				index_3 = i;
			}
		}
		double x1 = stars[index_0].x, y1 = stars[index_0].y;
		double x2 = stars[index_1].x, y2 = stars[index_1].y;
		double x3 = stars[index_2].x, y3 = stars[index_2].y;
		double x4 = stars[index_3].x, y4 = stars[index_3].y;
		double mean_x = (x1 + x2 + x3 + x4) / 4, mean_y = (y1 + y2 + y3 + y4) / 4;
		bool found = false;
		for (int i = 0; i < last_quad; i++) {
			if (quads[last_quad].mean_x == mean_x && quads[last_quad].mean_y == mean_y) {
				found = true;
				break;
			}
		}
		if (found)
			continue;
		x = x2 - x3; y = y2 - y3; distance[3] = sqrt(x * x + y * y);
		x = x2 - x4; y = y2 - y4; distance[4] = sqrt(x * x + y * y);
		x = x3 - x4; y = y3 - y4; distance[5] = sqrt(x * x + y * y);
		qsort(distance, 6, sizeof(double), double_comparator);
		indigo_quad *quad = quads + last_quad++;
		quad->mean_x = mean_x;
		quad->mean_y = mean_y;
		quad->index[0] = index_0;
		quad->index[1] = index_1;
		quad->index[2] = index_2;
		quad->index[3] = index_3;
		quad->hash[0] = distance[0];
		for (int i = 1; i < 6; i++)
			quad->hash[i] = distance[i] / distance[0];
	}
	*quads_found = last_quad;
	return INDIGO_OK;
}

int main(int argc, char **argv) {
	indigo_result result;
	indigo_set_log_level(INDIGO_LOG_INFO);
	indigo_log("solver test started");
	
	result = indigo_find_stars(raw_type, raw_image, raw_image_width, raw_image_height, max_stars, stars, &stars_found);
	indigo_log("indigo_find_stars(-> %d) -> %d", stars_found, result);
	
	//qsort(stars, max_stars, sizeof(indigo_star_detection), luminance_comparator);
	
	for (int i = 0; i < top_stars; i++) {
		indigo_selection_frame_digest(raw_type, raw_image, &stars[i].x, &stars[i].y, centroid_radius, raw_image_width, raw_image_height, &digest);
	}
	indigo_log("%dx indigo_selection_frame_digest() on top stars", top_stars);

	for (int i = 0; i < top_stars; i++) {
		indigo_log("stars[%d] = { %g, %g, %g, %g, %d }", i, stars[i].x, stars[i].y, stars[i].luminance, stars[i].nc_distance, stars[i].oversaturated);
	}
	
	result = indigo_find_quads(stars, top_stars, quads, &quads_found);
	indigo_log("indigo_find_quads(-> %d) -> %d", quads_found, result);

	for (int i = 0; i < quads_found; i++) {
		indigo_log("quads[%d] = { [%d, %d, %d, %d], [%g, %g] }", i, quads[i].index[0], quads[i].index[1], quads[i].index[2], quads[i].index[3], quads[i].mean_x, quads[i].mean_y);
	}
	

	indigo_log("solver test finished");
}
