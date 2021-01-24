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
#include <sys/param.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigo_novas.h>

#include "indigo_cat_data.h"

// input image specs
#define raw_image_width 1600
#define raw_image_height 1200
#define pixel_scale (8.44 / 3600)
#define raw_type INDIGO_RAW_MONO16

extern unsigned short raw_image[];

// detection params
#define max_image_stars 500
#define max_image_quads 500
#define top_stars 100
#define centroid_radius 8
#define precision 	0x003F

// to be moved to indigo_raw_utils

typedef struct {
	int index[4];
	double scale;
	unsigned short hash[5];
} indigo_quad;

static int double_comparator(const void *item_1, const void *item_2) {
	double double_1 = *(double *)item_1;
	double double_2 = *(double *)item_2;
	if (double_1 < double_2)
		return 1;
	if (double_1 > double_2)
		return -1;
	return 0;
}

static int quad_hash_comparator(const void *item_1, const void *item_2) {
	indigo_quad *quad_1 = (indigo_quad *)item_1;
	indigo_quad *quad_2 = (indigo_quad *)item_2;
	for (int i = 0; i < 5; i++) {
		if (quad_1->hash[i] < quad_2->hash[i])
			return 1;
		if (quad_1->hash[i] > quad_2->hash[i])
			return -1;
	}
	return 0;
}

indigo_result indigo_find_quads(indigo_star_detection *stars, int star_count, indigo_quad *quads, int *quads_found, double *min_distance, double *max_distance) {
	int last_quad = 0;
	double x, y;
	double min_distance_2 = DBL_MAX, max_distance_2 = 0;
	for (int index_0 = 0; index_0 < star_count; index_0++) {
		int index_1 = 0, index_2 = 0, index_3 = 0;
		double distance[6] = { DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX };
		for (int i = 0; i < star_count; i++) {
			if (index_0 == i)
				continue;
			// find closest, 2nd closest and 3rd closest star to index_0
			x = stars[index_0].x - stars[i].x;
			y = stars[index_0].y - stars[i].y;
			double d = x * x + y * y;
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
		// check for duplicate quads
		bool found = false;
		for (int i = 0; i < last_quad; i++) {
			indigo_quad *quad = quads + i;
			if ((quad->index[0] == index_0 || quad->index[0] == index_1 || quad->index[0] == index_2 || quad->index[0] == index_3) && (quad->index[1] == index_0 || quad->index[1] == index_1 || quad->index[1] == index_2 || quad->index[1] == index_3) && (quad->index[2] == index_0 || quad->index[2] == index_1 || quad->index[2] == index_2 || quad->index[2] == index_3) && (quad->index[3] == index_0 || quad->index[3] == index_1 || quad->index[3] == index_2 || quad->index[3] == index_3)) {
				found = true;
				break;
			}
		}
		if (found)
			continue;
		// compute other distances, min and max skymark size
		double x2 = stars[index_1].x, y2 = stars[index_1].y;
		double x3 = stars[index_2].x, y3 = stars[index_2].y;
		double x4 = stars[index_3].x, y4 = stars[index_3].y;
		x = x2 - x3; y = y2 - y3; distance[3] = x * x + y * y;
		x = x2 - x4; y = y2 - y4; distance[4] = x * x + y * y;
		x = x3 - x4; y = y3 - y4; distance[5] = x * x + y * y;
		qsort(distance, 6, sizeof(double), double_comparator);
		max_distance_2 = MAX(max_distance_2, distance[0]);
		min_distance_2 = MIN(min_distance_2, distance[5]);
		indigo_quad *quad = quads + last_quad++;
		quad->index[0] = index_0;
		quad->index[1] = index_1;
		quad->index[2] = index_2;
		quad->index[3] = index_3;
		quad->scale = distance[0];
		// compute hash
		for (int i = 1; i < 6; i++)
			quad->hash[i - 1] = (unsigned short)(distance[i] / distance[0] * precision);
	}
	qsort(quads, last_quad, sizeof(indigo_quad), quad_hash_comparator);
	*min_distance = sqrt(min_distance_2);
	*max_distance = sqrt(max_distance_2);
	*quads_found = last_quad;
	return INDIGO_OK;
}

static int hip_mag_comparator(const void *item_1, const void *item_2) {
	indigo_star_entry *star_1 = *((indigo_star_entry **)item_1);
	indigo_star_entry *star_2 = *((indigo_star_entry **)item_2);
	if (star_1->mag < star_2->mag)
		return 1;
	if (star_1->mag > star_2->mag)
		return -1;
	return 0;
}

static indigo_quad *match_reference_quads(indigo_quad *reference_quads, int from, int to, indigo_quad *image_quad) {
	if (to - from < 10) {
		for (int i = from; i < to; i++) {
			if (quad_hash_comparator(reference_quads + i, image_quad) == 0)
				return reference_quads + i;
		}
		return NULL;
	}
	int middle = from + (to - from) / 2;
	int cmp = quad_hash_comparator(reference_quads + middle, image_quad);
	if (cmp > 0)
		return match_reference_quads(reference_quads, from, middle, image_quad);
	else if (cmp < 0)
		return match_reference_quads(reference_quads, middle + 1, to, image_quad);
	return reference_quads + middle;
}

indigo_result indigo_match_quads(indigo_quad *image_quads, int image_quads_count, double min_ra, double max_ra, double min_dec, double max_dec, int reference_star_count, double min_size, double max_size) {
	int item_count = 16 * 1024;
	int last_item = 0;
	indigo_star_entry **star_entries = indigo_safe_malloc(item_count * sizeof(void *));
	indigo_star_entry *star_entry = indigo_star_data;
	while (star_entry->hip) {
		if (min_ra <= star_entry->ra && star_entry->ra <= max_ra && min_dec <= star_entry->dec && star_entry->dec <= max_dec) {
			if (last_item == item_count)
				star_entries = indigo_safe_realloc(star_entries, (item_count *= 2) * sizeof(void *));
			star_entries[last_item++] = star_entry;
		}
		star_entry++;
	}
	reference_star_count = MIN(reference_star_count, last_item);
	qsort(star_entries, last_item, sizeof(void *), hip_mag_comparator);
	struct coords {
		double ra, dec;
	} *star, *reference_stars = indigo_safe_malloc(reference_star_count * sizeof(struct coords));
	for (int i = 0; i <reference_star_count; i++) {
		star_entry = indigo_star_data + i;
		star = reference_stars + i;
		star->ra = star_entry->ra;
		star->dec = star_entry->dec;
		indigo_app_star(star_entry->promora, star_entry->promodec, star_entry->px, star_entry->rv, &star->ra, &star->dec);
		star->ra *= 15; // convert hours to degrees
	}
	free(star_entries);
	item_count = 16 * 1024;
	last_item = 0;
	indigo_quad *reference_quads = indigo_safe_malloc(item_count * sizeof(indigo_quad));
	min_size = min_size * min_size;
	max_size = max_size * max_size;
	double ra, dec, distance[6];
	for (int index_0 = 0; index_0 < reference_star_count; index_0++) {
		for (int index_1 = index_0 + 1; index_1 < reference_star_count; index_1++) {
			ra = reference_stars[index_0].ra - reference_stars[index_1].ra;
			dec = reference_stars[index_0].ra - reference_stars[index_1].ra;
			distance[0] = ra *ra + dec * dec;
			if (distance[0] < min_size || max_size < distance[0])
				continue;
			for (int index_2 = index_1 + 1; index_2 < reference_star_count; index_2++) {
				ra = reference_stars[index_0].ra - reference_stars[index_2].ra;
				dec = reference_stars[index_0].ra - reference_stars[index_2].ra;
				distance[1] = ra *ra + dec * dec;
				if (distance[1] < min_size || max_size < distance[1])
					continue;
				ra = reference_stars[index_1].ra - reference_stars[index_2].ra;
				dec = reference_stars[index_1].ra - reference_stars[index_2].ra;
				distance[2] = ra *ra + dec * dec;
				if (distance[2] < min_size || max_size < distance[1])
					continue;
				for (int index_3 = index_2 + 1; index_3 < reference_star_count; index_3++) {
					ra = reference_stars[index_0].ra - reference_stars[index_3].ra;
					dec = reference_stars[index_0].ra - reference_stars[index_3].ra;
					distance[3] = ra *ra + dec * dec;
					if (distance[3] < min_size || max_size < distance[3])
						continue;
					ra = reference_stars[index_1].ra - reference_stars[index_3].ra;
					dec = reference_stars[index_1].ra - reference_stars[index_3].ra;
					distance[4] = ra *ra + dec * dec;
					if (distance[4] < min_size || max_size < distance[4])
						continue;
					ra = reference_stars[index_2].ra - reference_stars[index_3].ra;
					dec = reference_stars[index_2].ra - reference_stars[index_3].ra;
					distance[5] = ra *ra + dec * dec;
					if (distance[5] < min_size || max_size < distance[5])
						continue;
					qsort(distance, 6, sizeof(double), double_comparator);
					if (last_item == item_count)
						reference_quads = indigo_safe_realloc(reference_quads, (item_count *= 2) * sizeof(indigo_quad));
					indigo_quad *quad = reference_quads + last_item++;
					quad->index[0] = index_0;
					quad->index[1] = index_1;
					quad->index[2] = index_2;
					quad->index[3] = index_3;
					quad->scale = distance[0];
					for (int i = 1; i < 6; i++)
						quad->hash[i - 1] = (unsigned short)(distance[i] / distance[0] * precision);
				}
			}
		}
	}
	int reference_quad_count = last_item;
	qsort(reference_quads, reference_quad_count, sizeof(indigo_quad), quad_hash_comparator);

	for (int i = 0; i < image_quads_count; i++) {
		indigo_quad *image_quad = image_quads + i;
		indigo_quad *reference_quad = match_reference_quads(reference_quads, 0, reference_quad_count, image_quad);
		if (reference_quad) {
			indigo_log("image quad(%3d) = { %03d, %03d, %03d, %03d, %04x%04x%04x%04x%04x }", i, image_quad->index[0], image_quad->index[1], image_quad->index[2], image_quad->index[3], image_quad->hash[0], image_quad->hash[1], image_quad->hash[2], image_quad->hash[3], image_quad->hash[4]);
			indigo_log("reference quad = { %03d, %03d, %03d, %03d, %04x%04x%04x%04x%04x }", reference_quad->index[0], reference_quad->index[1], reference_quad->index[2], reference_quad->index[3], reference_quad->hash[0], reference_quad->hash[1], reference_quad->hash[2], reference_quad->hash[3], reference_quad->hash[4]);
		}
	}
	
	free(reference_stars);
	free(reference_quads);
	return INDIGO_OK;
}


int main(int argc, char **argv) {
	indigo_result result;
	indigo_set_log_level(INDIGO_LOG_INFO);
	indigo_log("solver test started");
	
	// find stars
	int image_stars_found = 0;
	indigo_star_detection *image_stars = indigo_safe_malloc(max_image_stars * sizeof(indigo_star_detection));
	result = indigo_find_stars(raw_type, raw_image, raw_image_width, raw_image_height, max_image_stars, image_stars, &image_stars_found);
	indigo_log("indigo_find_stars(-> %d) -> %d", image_stars_found, result);

	// find subpixel coordinates for top stars
	indigo_frame_digest digest;
	for (int i = 0; i < top_stars; i++)
		indigo_selection_frame_digest(raw_type, raw_image, &image_stars[i].x, &image_stars[i].y, centroid_radius, raw_image_width, raw_image_height, &digest);
	indigo_log("%dx indigo_selection_frame_digest() on top stars", top_stars);
	for (int i = 0; i < top_stars; i++)
		indigo_log("stars[%d] = { %g, %g, %g, %g, %d }", i, image_stars[i].x, image_stars[i].y, image_stars[i].luminance, image_stars[i].nc_distance, image_stars[i].oversaturated);
	
	// find quads for top stars + min and max skymark size in pixels
	int image_quads_found = 0;
	indigo_quad *image_quads = indigo_safe_malloc(max_image_quads * sizeof(indigo_quad));
	double min_size = DBL_MAX, max_size = 0;
	result = indigo_find_quads(image_stars, top_stars, image_quads, &image_quads_found, &min_size, &max_size);
	indigo_log("indigo_find_quads(-> %d, -> %g, -> %g) -> %d", image_quads_found, min_size, max_size, result);
	for (int i = 0; i < image_quads_found; i++)
		indigo_log("quads[%d] = { %03d, %03d, %03d, %03d, %04x%04x%04x%04x%04x }", i, image_quads[i].index[0], image_quads[i].index[1], image_quads[i].index[2], image_quads[i].index[3], image_quads[i].hash[0], image_quads[i].hash[1], image_quads[i].hash[2], image_quads[i].hash[3], image_quads[i].hash[4]);
	
	indigo_match_quads(image_quads, image_quads_found, 6.3, 6.7, 0, 20, 100, min_size * pixel_scale, max_size * pixel_scale);
	
	
	indigo_log("solver test finished");
}
