//
//  solver_test.c
//  indigo
//
//  Created by Peter Polakovic on 10/01/2021.
//  Copyright © 2021 CloudMakers, s. r. o. All rights reserved.
//

// link with solver_test_image.c, indigo_bus.c, indigo_raw_utils.c,
// indigo_cat_data.c, indigo_token.c, indigo_io.c, indigo_align.c

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <sys/param.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigo_align.h>

#include "indigo_cat_data.h"

#define  DEG2RAD (M_PI / 180.0)
#define  HOUR2RAD (M_PI / 12.0)

// input image specs
int raw_image_width = 1600;
int raw_image_height = 1200;
double pixel_scale = (8.44 / 3600);
indigo_raw_type raw_type = INDIGO_RAW_MONO16;

extern unsigned short raw_image[];

// detection params
int max_image_stars = 500;
int max_image_quads = 500;
int top_stars = 100;
int centroid_radius = 8;
int precision = 0x001F;

// to be moved to indigo_raw_utils

typedef struct {
	int index[4];
	double diameter;
	unsigned short hash[5];
} indigo_quad;

typedef struct {
	int hip;
	double ra, dec;
	double mag;
} indigo_star;

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

indigo_result indigo_find_quads(indigo_star_detection *stars, int star_count, indigo_quad *quads, int *quads_found, double *max_distance) {
	int last_quad = 0;
	double x, y;
	*max_distance = 0;
	for (int index_0 = 0; index_0 < star_count; index_0++) {
		int index_1 = 0, index_2 = 0, index_3 = 0;
		double distance[6] = { DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX };
		for (int i = 0; i < star_count; i++) {
			if (index_0 == i)
				continue;
			// find closest, 2nd closest and 3rd closest star to index_0
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
		x = x2 - x3; y = y2 - y3; distance[3] = sqrt(x * x + y * y);
		x = x2 - x4; y = y2 - y4; distance[4] = sqrt(x * x + y * y);
		x = x3 - x4; y = y3 - y4; distance[5] = sqrt(x * x + y * y);
		qsort(distance, 6, sizeof(double), double_comparator);
		*max_distance = MAX(*max_distance, distance[0]);
		indigo_quad *quad = quads + last_quad++;
		quad->index[0] = index_0;
		quad->index[1] = index_1;
		quad->index[2] = index_2;
		quad->index[3] = index_3;
		quad->diameter = distance[0];
		// compute hash
		for (int i = 1; i < 6; i++)
			quad->hash[i - 1] = (unsigned short)(distance[i] / distance[0] * precision);
	}
	qsort(quads, last_quad, sizeof(indigo_quad), quad_hash_comparator);
	*quads_found = last_quad;
	indigo_log("image star count     = %d", star_count);
	indigo_log("image quad count     = %d", *quads_found);
	return INDIGO_OK;
}

static int hip_mag_comparator(const void *item_1, const void *item_2) {
	indigo_star *star_1 = (indigo_star *)item_1;
	indigo_star *star_2 = (indigo_star *)item_2;
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

static double spherical_distance(indigo_star *star_1, indigo_star *star_2) {
	double dec_1 = star_1->dec * DEG2RAD;
	double dec_2 = star_2->dec * DEG2RAD;
	double sin_d1 = sin(dec_1);
	double cos_d1 = cos(dec_1);
	double sin_d2 = sin(dec_2);
	double cos_d2 = cos(dec_2);
	double cos_delta_a = cos(fabs(star_1->ra - star_2->ra) * HOUR2RAD);
	return acos(sin_d1 * sin_d2 + cos_d1 * cos_d2 * cos_delta_a) / DEG2RAD;
}

indigo_result indigo_match_quads(indigo_quad *image_quads, int image_quads_count, double ra, double dec, double radius, int reference_star_count, double max_size) {
	double min_ra = ra - radius / 15;
	double max_ra = ra + radius / 15;
	double min_dec = dec - radius;
	double max_dec = dec + radius;
	int item_count = 16 * 1024;
	int last_item = 0;
	indigocat_star_entry *star_entry = indigo_star_data;
	indigo_star *star, *reference_stars = indigo_safe_malloc(item_count * sizeof(indigo_star));
	while (star_entry->hip) {
		ra = star_entry->ra;
		dec = star_entry->dec;
		indigo_app_star(star_entry->promora, star_entry->promodec, star_entry->px, star_entry->rv, &ra, &dec);
		if (min_ra < ra && ra < max_ra && min_dec < dec && dec < max_dec) {
			if (last_item == item_count)
				reference_stars = indigo_safe_realloc(reference_stars, (item_count *= 2) * sizeof(indigo_star));
			star = reference_stars + (last_item++);
			star->hip = star_entry->hip;
			star->ra = ra;
			star->dec = dec;
			star->mag = star_entry->mag;
			//printf("%.5f\t%.5f\t%.5f\t%.5f\n", ra, dec, x, y);
		}
		star_entry++;
	}
	qsort(reference_stars, last_item, sizeof(indigo_star), hip_mag_comparator);
	indigo_log("reference star found = %d", last_item);
	reference_star_count = MIN(reference_star_count, last_item);
	indigo_log("reference star count = %d", reference_star_count);

//	for (int i = 0; i < reference_star_count; i++)
//		printf("%.3f\t%.3f\t%.3f\t%.3f\n", reference_stars[i].ra, reference_stars[i].dec, reference_stars[i].x, reference_stars[i].y);
	item_count = 16 * 1024;
	last_item = 0;
	indigo_quad *reference_quads = indigo_safe_malloc(item_count * sizeof(indigo_quad));
	for (int index_0 = 0; index_0 < reference_star_count; index_0++) {
		for (int index_1 = index_0 + 1; index_1 < reference_star_count; index_1++) {
			double distance_0 = spherical_distance(reference_stars + index_0, reference_stars + index_1);
			if (max_size < distance_0)
				continue;
			for (int index_2 = index_1 + 1; index_2 < reference_star_count; index_2++) {
				double distance_1 = spherical_distance(reference_stars + index_0, reference_stars + index_2);
				if (max_size < distance_1)
					continue;
				double distance_2 = spherical_distance(reference_stars + index_1, reference_stars + index_2);
				if (max_size < distance_2)
					continue;
				for (int index_3 = index_2 + 1; index_3 < reference_star_count; index_3++) {
					double distance_3 = spherical_distance(reference_stars + index_0, reference_stars + index_3);
					if (max_size < distance_3)
						continue;
					double distance_4 = spherical_distance(reference_stars + index_1, reference_stars + index_3);
					if (max_size < distance_4)
						continue;
					double distance_5 = spherical_distance(reference_stars + index_2, reference_stars + index_3);
					if (max_size < distance_5)
						continue;
					double distance[6] = { distance_0, distance_1, distance_2, distance_3, distance_4, distance_5 };
					qsort(distance, 6, sizeof(double), double_comparator);
					if (last_item == item_count)
						reference_quads = indigo_safe_realloc(reference_quads, (item_count *= 2) * sizeof(indigo_quad));
					indigo_quad *quad = reference_quads + last_item++;
					quad->index[0] = index_0;
					quad->index[1] = index_1;
					quad->index[2] = index_2;
					quad->index[3] = index_3;
					quad->diameter = distance[0];
					for (int i = 1; i < 6; i++)
						quad->hash[i - 1] = (unsigned short)(distance[i] / distance[0] * precision);
				}
			}
		}
	}
	int reference_quad_count = last_item;
	qsort(reference_quads, reference_quad_count, sizeof(indigo_quad), quad_hash_comparator);
	indigo_log("reference quad count = %d", reference_quad_count);
	
	for (int i = 0; i < image_quads_count; i++) {
		indigo_quad *image_quad = image_quads + i;
		indigo_quad *reference_quad = match_reference_quads(reference_quads, 0, reference_quad_count, image_quad);
		if (reference_quad) {
			indigo_log("image quad           = { %d, %d, %d, %d, %04x:%04x:%04x:%04x:%04x }", image_quad->index[0], image_quad->index[1], image_quad->index[2], image_quad->index[3], image_quad->hash[0], image_quad->hash[1], image_quad->hash[2], image_quad->hash[3], image_quad->hash[4]);
			indigo_log("reference quad       = { %d, %d, %d, %d, %04x:%04x:%04x:%04x:%04x }", reference_quad->index[0], reference_quad->index[1], reference_quad->index[2], reference_quad->index[3], reference_quad->hash[0], reference_quad->hash[1], reference_quad->hash[2], reference_quad->hash[3], reference_quad->hash[4]);
			indigo_log("scale                = %g -> %.0f%%", reference_quad->diameter / image_quad->diameter, reference_quad->diameter / image_quad->diameter / pixel_scale * 100);
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
	indigo_log("image star found     = %d", image_stars_found);

	// find subpixel coordinates for top stars
	indigo_frame_digest digest;
	for (int i = 0; i < top_stars; i++)
		indigo_selection_frame_digest(raw_type, raw_image, &image_stars[i].x, &image_stars[i].y, centroid_radius, raw_image_width, raw_image_height, &digest);
	
	// find quads for top stars + min and max skymark size in pixels
	int image_quads_found = 0;
	indigo_quad *image_quads = indigo_safe_malloc(max_image_quads * sizeof(indigo_quad));
	double max_size = 0;
	result = indigo_find_quads(image_stars, top_stars, image_quads, &image_quads_found, &max_size);
	
	indigo_match_quads(image_quads, image_quads_found, 6.5, 5.0, 5.0, 200, max_size * pixel_scale * 1.1);
	
	indigo_log("solver test finished");
}
