//
//  solver_test.c
//  indigo
//
//  Created by Peter Polakovic on 10/01/2021.
//  Copyright © 2021 CloudMakers, s. r. o. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

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
	double x;
	double y;
	double luminance;
	
} indigo_solver_star;

indigo_result indigo_raw2xy(indigo_raw_type raw_type, const void *data, const int width, const int height, const int stars_max) {
	indigo_star_detection *stars = malloc(stars_max * sizeof(indigo_star_detection));
	if (stars == NULL)
		return INDIGO_FAILED;
	
	free(stars);
	return INDIGO_OK;
}

int main(int argc, char **argv) {
	indigo_set_log_level(INDIGO_LOG_INFO);
	indigo_log("solver test started");
	
	indigo_find_stars(raw_type, raw_image, raw_image_width, raw_image_height, max_stars, stars, &stars_found);
	indigo_log("indigo_find_stars() -> %d", stars_found);
	
	//qsort(stars, max_stars, sizeof(indigo_star_detection), luminance_comparator);
	
	for (int i = 0; i < top_stars; i++) {
		indigo_selection_frame_digest(raw_type, raw_image, &stars[i].x, &stars[i].y, centroid_radius, raw_image_width, raw_image_height, &digest);
	}
	indigo_log("%dx indigo_selection_frame_digest() on top stars", top_stars);

	for (int i = 0; i < top_stars; i++) {
		indigo_log("stars[%d] = { %lf, %lf, %lf, %lf, %d }", i, stars[i].x, stars[i].y, stars[i].luminance, stars[i].nc_distance, stars[i].oversaturated);
	}
	
	indigo_log("solver test finished");
}
