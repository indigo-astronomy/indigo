// Copyright (c) 2025 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO plate solver agent
 \file indigo_agent_solver.c
 */

#define DRIVER_VERSION 0x03000001
#define DRIVER_NAME	"indigo_agent_solver"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <jpeglib.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigo_platesolver.h>
#include <indigo/indigocat/indigocat_star.h>

#include "indigo_agent_solver.h"


#define SOLVER_DEVICE_PRIVATE_DATA				((solver_private_data *)device->private_data)
#define SOLVER_CLIENT_PRIVATE_DATA				((solver_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define MAX_DETECTION_COUNT								1024

typedef struct {
	platesolver_private_data platesolver;
} solver_private_data;

// --------------------------------------------------------------------------------

/* ============================================================================
	 Constants and Types
============================================================================ */

#define PI 3.14159265358979323846
#define DEG_TO_RAD (PI/180.0)
#define RAD_TO_DEG (180.0/PI)
#define HOUR_TO_RAD (PI/12.0)
#define RAD_TO_HOUR (12.0/PI)
#define HOUR_TO_DEG (360.0/24.0)
#define DEG_TO_HOUR (24.0/360.0)
#define ARCSEC_TO_RAD (PI/(180.0*3600.0))

#define MAX_STARS 5000
#define MAX_QUADS 10000
#define MAX_MATCHES 1000
#define MAX_ITERATIONS 10
#define CONVERGENCE_THRESHOLD_RAD (1e-9)  // ~0.00002 arcsec
#define SIGMA_REJECTION 3.0
#define MIN_MATCHES 4


typedef struct {
	double ra_x;
	double dec_y;
	int id;
	bool used;
	double centroid_distance;
} Star;

typedef struct {
	Star *star[4];
	double max_distance;  			// longest star distance
	double ratios[5];	// d2/max_distance, d3/max_distance, d4/max_distance, d5/max_distance, d6/max_distance
} Quad;

typedef struct {
	Quad *image;
	Quad *catalog;
} QuadMatch;

typedef struct {
	Star *image;
	Star *catalog;
	bool used;
} StarMatch;

typedef struct {
	double x, y, z;
} Vec3;

typedef struct {
		double a11, a12, b1; // xi  = a11*x + a12*y + b1
		double a21, a22, b2; // eta = a21*x + a22*y + b2
} Affine2x2T;

typedef struct {
		double ra;        		// rad
		double dec;       		// rad
		double ra_err;    		// rad (1-sigma)
		double dec_err;   		// rad (1-sigma)
		double pix_scale_x; 	// rad/pixel
		double pix_scale_y; 	// rad/pixel
		double rotation;    	// rad (east of north)
		int stars_used;     	// number of stars used in final solution
		double rms_residual;	// rad
} Solution;

/* ============================================================================
	 Utility Functions
============================================================================ */

static bool gnomonic_forward(double ra0, double dec0, double ra, double dec, double *xi, double *eta);

static void add_marker(indigo_platesolver_task *task, double x, double y) {
	indigo_raw_header *header = (indigo_raw_header *)task->image;
	int pixel_size = 0;
	switch (header->signature) {
		case INDIGO_RAW_MONO8:
			pixel_size = 1;
			break;
		case INDIGO_RAW_MONO16:
			pixel_size = 2;
			break;
		case INDIGO_RAW_RGB24:
			pixel_size = 3;
			break;
		case INDIGO_RAW_RGBA32:
		case INDIGO_RAW_ABGR32:
			pixel_size = 4;
			break;
		case INDIGO_RAW_RGB48:
			pixel_size = 6;
			break;
	}
	unsigned long metadata_offset = sizeof(indigo_raw_header) + pixel_size * header->width * header->height;
	if (task->size == metadata_offset) {
		task->image = realloc(task->image, task->size += 8 * 1024);
		sprintf(task->image + metadata_offset, "SIMPLE=T;MARKER=%.2f:%.2f;", x, y);
	} else {
		unsigned long metadata_len = strlen(task->image + metadata_offset);
		if (task->size - metadata_offset < metadata_len + 128) {
			task->image = realloc(task->image, task->size += 8 * 1024);
		}
		sprintf(task->image + metadata_offset + metadata_len, "MARKER=%.2f:%.2f;", x, y);
	}
}

static double planar_distance(double x1, double y1, double x2, double y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	return sqrt(dx * dx + dy * dy);
}

static double angular_distance(double ra0, double dec0, double ra1, double dec1, double ra2, double dec2) {
//	double dRA = ra2 - ra1;
//	double dDec = dec2 - dec1;
//	double a = sin(dDec/2.0) * sin(dDec/2.0) + cos(dec1) * cos(dec2) * sin(dRA/2.0) * sin(dRA/2.0);
//	return 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
	double x1, y1, x2, y2;
	gnomonic_forward(ra0, dec0, ra1, dec1, &x1, &y1);
	gnomonic_forward(ra0, dec0, ra2, dec2, &x2, &y2);
	return planar_distance(x1, y1, x2, y2);
}

static inline double wrap_pi(double a) {
	// wrap to [-pi, pi]
	a = fmod(a + M_PI, 2.0 * M_PI);
	if (a < 0) {
		a += 2.0 * M_PI;
	}
	return a - M_PI;
}

static inline double wrap_2pi(double a) {
	// wrap to [0, 2pi)
	a = fmod(a, 2.0 * M_PI);
	if (a < 0) {
		a += 2.0 * M_PI;
	}
	return a;
}

static Vec3 vec3_add(Vec3 a, Vec3 b) {
	return (Vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static Vec3 vec3_scale(Vec3 a, double s) {
	return (Vec3) { a.x*s, a.y*s, a.z * s };
}

static double vec3_norm(Vec3 a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

static Vec3 vec3_normalize(Vec3 a) {
	double n = vec3_norm(a);
	if (n <= 1e-12) {
		return (Vec3) { 0, 0, 0 };
	}
	return vec3_scale(a, 1.0 / n);
}

static Vec3 radec_to_unit(double ra, double dec) {
	double c = cos(dec);
	return (Vec3) { c * cos(ra), c * sin(ra), sin(dec) };
}

static void unit_to_radec(Vec3 v, double *ra, double *dec) {
	double n = vec3_norm(v);
	if (n <= 1e-12) {
		*ra = 0;
		*dec = 0;
		return;
	}
	v = vec3_scale(v, 1.0 / n);
	*ra = atan2(v.y, v.x);
	*ra = wrap_2pi(*ra);
	*dec = asin(fmax(-1.0, fmin(1.0, v.z)));
}

static int compare_doubles(const void* a, const void* b) {
	double da = *(const double*)a;
	double db = *(const double*)b;
	return (da > db) - (da < db);
}

static int compare_centroid_distances(const void* a, const void* b) {
	Star *sa = *(Star**)a;
	Star *sb = *(Star**)b;
	return (sa->centroid_distance > sb->centroid_distance) - (sa->centroid_distance < sb->centroid_distance);
}

static void angular_centroid(const Star stars[], double* ra, double* dec) {
	Vec3 sum = radec_to_unit(stars[0].ra_x, stars[0].dec_y);
	sum = vec3_add(sum, radec_to_unit(stars[1].ra_x, stars[1].dec_y));
	sum = vec3_add(sum, radec_to_unit(stars[2].ra_x, stars[2].dec_y));
	sum = vec3_add(sum, radec_to_unit(stars[3].ra_x, stars[3].dec_y));
	Vec3 c = vec3_normalize(sum);
	double ra_c  = atan2(c.y, c.x);
	double dec_c = asin(fmax(-1.0, fmin(1.0, c.z)));
	*ra  = wrap_2pi(ra_c);
	*dec = dec_c;
}

static void planar_centroid(const Star stars[], double* x, double* y) {
	*x = (stars[0].ra_x + stars[1].ra_x + stars[3].ra_x + stars[4].ra_x) / 4.0;
	*y = (stars[0].dec_y + stars[1].dec_y + stars[3].dec_y + stars[4].dec_y) / 4.0;
}

/* ============================================================================
	 Quad functions
============================================================================ */

static bool create_image_quad(Star stars[], int indices[4], double distances[6], Quad* quad) {
	double centroid_x, centroid_y;
	planar_centroid(stars, &centroid_x, &centroid_y);
	for (int i = 0; i < 4; i++) {
		Star *star = stars + indices[i];
		quad->star[i] = star;
		star->centroid_distance = planar_distance(centroid_x, centroid_y, star->ra_x, star->dec_y);
	}
	qsort(quad->star, 4, sizeof(Star *), compare_centroid_distances);
	qsort(distances, 6, sizeof(double), compare_doubles);
	double max_distance = distances[5];
	if (max_distance < 1e-12) {
		return false;
	}
	quad->max_distance = max_distance;
	for (int i = 0; i < 5; i++) {
		quad->ratios[i] = distances[i] / max_distance;
	}
	return true;
}

static int find_image_quads(Star *image_stars, int num_image_stars, Quad image_quads[], int max_quads, double min_distance, double max_distance) {
	int quad_count = 0;
	double distances[6] = { 0 };
	for (int i = 0; i < num_image_stars && quad_count < max_quads; i++) {
		Star *star_i = image_stars + i;
		for (int j = i + 1; j < num_image_stars && quad_count < max_quads; j++) {
			Star *star_j = image_stars + j;
			double d = planar_distance(star_i->ra_x, star_i->dec_y, star_j->ra_x, star_j->dec_y);
			if (d < min_distance || d > max_distance) {
				continue;
			}
			distances[0] = d;
			for (int k = j + 1; k < num_image_stars && quad_count < max_quads; k++) {
				Star *star_k = image_stars + k;
				d = planar_distance(star_i->ra_x, star_i->dec_y, star_k->ra_x, star_k->dec_y);
				if (d < min_distance || d > max_distance) {
					continue;
				}
				distances[1] = d;
				d = planar_distance(star_j->ra_x, star_j->dec_y, star_k->ra_x, star_k->dec_y);
				if (d < min_distance || d > max_distance) {
					continue;
				}
				distances[2] = d;
				for (int l = k + 1; l < num_image_stars && quad_count < max_quads; l++) {
					Star *star_l = image_stars + l;
					d = planar_distance(star_i->ra_x, star_i->dec_y, star_l->ra_x, star_l->dec_y);
					if (d < min_distance || d > max_distance) {
						continue;
					}
					distances[3] = d;
					d = planar_distance(star_j->ra_x, star_j->dec_y, star_l->ra_x, star_l->dec_y);
					if (d < min_distance || d > max_distance) {
						continue;
					}
					distances[4] = d;
					d = planar_distance(star_k->ra_x, star_k->dec_y, star_l->ra_x, star_l->dec_y);
					if (d < min_distance || d > max_distance) {
						continue;
					}
					distances[5] = d;
					int indices[4] = { i, j, k, l };
					if (create_image_quad(image_stars, indices, distances, image_quads + quad_count)) {
						quad_count++;
					}
				}
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%d image quads created", quad_count);
	return quad_count;
}

static bool create_catalog_quad(double ra0, double dec0, Star stars[], int indices[4], double distances[6], Quad* quad) {
	double centroid_ra, centroid_dec;
	angular_centroid(stars, &centroid_ra, &centroid_dec);
	for (int i = 0; i < 4; i++) {
		Star *star = stars + indices[i];
		quad->star[i] = star;
		star->centroid_distance = angular_distance(ra0, dec0, centroid_ra, centroid_dec, star->ra_x, star->dec_y);
	}
	qsort(quad->star, 4, sizeof(Star *), compare_centroid_distances);
	qsort(distances, 6, sizeof(double), compare_doubles);
	double max_distance = distances[5];
	if (max_distance < 1e-12) {
		return false;
	}
	quad->max_distance = max_distance;
	for (int i = 0; i < 5; i++) {
		quad->ratios[i] = distances[i] / max_distance;
	}
	return true;
}

static int find_catalog_quads(double ra0, double dec0, Star *catalog_stars, int num_catalog_stars, Quad catalog_quads[], int max_quads, double search_radius) {
	int quad_count = 0;
	double distances[6] = { 0 };
	int selected_stars[MAX_STARS];
	int num_selected = 0;
	double min_dist = 0.01 * search_radius;
	double max_dist = 0.9 * search_radius;
	for (int i = 0; i < num_catalog_stars; i++) {
		double dist = angular_distance(ra0, dec0, ra0, dec0, catalog_stars[i].ra_x, catalog_stars[i].dec_y);
		if (dist <= search_radius && num_selected < MAX_STARS) {
			selected_stars[num_selected++] = i;
		}
	}
	if (num_selected < 4) {
		return 0;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%d catalog stars selected", num_selected);
	for (int i = 0; i < num_selected && quad_count < max_quads; i++) {
		int idx_i = selected_stars[i];
		Star *star_i = catalog_stars + idx_i;
		for (int j = i + 1; j < num_selected && quad_count < max_quads; j++) {
			int idx_j = selected_stars[j];
			Star *star_j = catalog_stars + idx_j;
			double d = angular_distance(ra0, dec0, star_i->ra_x, star_i->dec_y, star_j->ra_x, star_j->dec_y);
			if (d < min_dist || d > max_dist) {
				continue;
			}
			distances[0] = d;
			for (int k = j + 1; k < num_selected && quad_count < max_quads; k++) {
				int idx_k = selected_stars[k];
				Star *star_k = catalog_stars + idx_k;
				d = angular_distance(ra0, dec0, star_i->ra_x, star_i->dec_y, star_k->ra_x, star_k->dec_y);
				if (d < min_dist || d > max_dist) {
					continue;
				}
				distances[1] = d;
				d = angular_distance(ra0, dec0, star_j->ra_x, star_j->dec_y, star_k->ra_x, star_k->dec_y);
				if (d < min_dist || d > max_dist) {
					continue;
				}
				distances[2] = d;
				for (int l = k + 1; l < num_selected && quad_count < max_quads; l++) {
					int idx_l = selected_stars[l];
					Star *star_l = catalog_stars + idx_l;
					d = angular_distance(ra0, dec0, star_i->ra_x, star_i->dec_y, star_l->ra_x, star_l->dec_y);
					if (d < min_dist || d > max_dist) {
						continue;
					}
					distances[3] = d;
					d = angular_distance(ra0, dec0, star_j->ra_x, star_j->dec_y, star_l->ra_x, star_l->dec_y);
					if (d < min_dist || d > max_dist) {
						continue;
					}
					distances[4] = d;
					d = angular_distance(ra0, dec0, star_k->ra_x, star_k->dec_y, star_l->ra_x, star_l->dec_y);
					if (d < min_dist || d > max_dist) {
						continue;
					}
					distances[5] = d;
					int indices[4] = { idx_i, idx_j, idx_k, idx_l };
					if (create_catalog_quad(ra0, dec0, catalog_stars, indices, distances, catalog_quads + quad_count)) {
						quad_count++;
					}
				}
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%d catalog quads created", quad_count);
	return quad_count;
}

static bool quads_match(Quad* image, Quad* catalog) {
	for (int i = 0; i < 5; i++) {
		if (fabs(image->ratios[i] - catalog->ratios[i]) > 0.02) { // quad matching tolerance
			return false;
		}
	}
	return true;
}

static int find_matching_stars(double ra0, double dec0, Quad image_quads[], int num_image_quads, Quad catalog_quads[], int num_catalog_quads, StarMatch matches[], int max_matches) {
	int matching_quads_count = 0;
	QuadMatch matching_quads[max_matches];
	for (int i = 0; i < num_image_quads && matching_quads_count < max_matches; i++) {
		for (int j = 0; j < num_catalog_quads && matching_quads_count < max_matches; j++) {
			if (quads_match(image_quads + i, catalog_quads + j)) {
				matching_quads[matching_quads_count].image = image_quads + i;
				matching_quads[matching_quads_count].catalog = catalog_quads + j;
				matching_quads_count++;
			}
		}
	}
	if (matching_quads_count < 3) {
		return 0;
	}
	double scales[MAX_MATCHES];
	for (int i = 0; i < matching_quads_count; i++) {
		scales[i] = matching_quads[i].catalog->max_distance / matching_quads[i].image->max_distance;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%d matching quads found", matching_quads_count);
	double sorted_scales[MAX_MATCHES];
	memcpy(sorted_scales, scales, (matching_quads_count) * sizeof(double));
	qsort(sorted_scales, matching_quads_count, sizeof(double), compare_doubles);
	double median_scale = sorted_scales[matching_quads_count / 2];
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Scale: min %.2f\"/px max %.2f\"/px median %.2f\"/px", scales[0] * RAD_TO_DEG * 3600, scales[matching_quads_count - 1] * RAD_TO_DEG * 3600, median_scale * RAD_TO_DEG * 3600);
	int j = 0;
	for (int i = 0; i < matching_quads_count; i++) {
		double scale_ratio = scales[i] / median_scale;
		if (fabs(1 - scale_ratio) < 0.1) { // scale tolerance
			matching_quads[j++] = matching_quads[i];
		}
	}
	matching_quads_count = j;
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%d quads passed scale test", matching_quads_count);
	int matching_stars_count = 0;
	for (int i = 0; i < matching_quads_count && matching_stars_count < max_matches; i++) {
		Quad *catalog_quad = matching_quads[i].catalog;
		Quad *image_quad = matching_quads[i].image;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%d %d %d %d %.2f\" %.0fpx %.2f\"/px\n", catalog_quad->star[0]->id, catalog_quad->star[1]->id, catalog_quad->star[2]->id, catalog_quad->star[3]->id, catalog_quad->max_distance * RAD_TO_DEG * 3600, image_quad->max_distance, scales[i] * RAD_TO_DEG * 3600);
		for (int j = 0; j < 4; j++) {
			bool insert = true;
			for (int k = 0; k < matching_stars_count; k++) {
				if (matches[k].catalog == catalog_quad->star[j]) {
					insert = false;
					break;
				}
				double image_dist = planar_distance(matches[k].image->ra_x, matches[k].image->dec_y, image_quad->star[j]->ra_x, image_quad->star[j]->dec_y);
				double catalog_dist = angular_distance(ra0, dec0, matches[k].catalog->ra_x, matches[k].catalog->dec_y, catalog_quad->star[j]->ra_x, catalog_quad->star[j]->dec_y);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%d %d %.2f\" %.0fpx %.2f\"/px\n", matches[k].catalog->id, catalog_quad->star[j]->id, catalog_dist * RAD_TO_DEG * 3600, image_dist, catalog_dist / image_dist * RAD_TO_DEG * 3600);
			}
			if (insert) {
				StarMatch *match = matches + matching_stars_count;
				//INDIGO_DRIVER_LOG(DRIVER_NAME, "%d\n", catalog_quad->star[j]->id);
				match->image = image_quad->star[j];
				match->catalog = catalog_quad->star[j];
				matching_stars_count++;
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%d matching stars found", matching_stars_count);
	return matching_stars_count;
}

/* ============================================================================
	 Astrometric Solution Calculation
============================================================================ */

static void estimate_tangent_point(StarMatch *matches, int matching_stars_count, double *ra0, double *dec0) {
	Vec3 sum = { 0, 0, 0 };
	for (int i = 0; i < matching_stars_count; i++) {
		sum = vec3_add(sum, radec_to_unit(matches[i].catalog->ra_x, matches[i].catalog->dec_y));
	}
	unit_to_radec(sum, ra0, dec0);
}

static bool gnomonic_forward(double ra0, double dec0, double ra, double dec, double *xi, double *eta) {
	double dra = wrap_pi(ra - ra0);
	double sd0 = sin(dec0), cd0 = cos(dec0);
	double sd  = sin(dec),  cd  = cos(dec);
	double cosc = sd0*sd + cd0 * cd * cos(dra);
	if (cosc <= 1e-12) {
		return false;
	}
	*xi  = (cd * sin(dra)) / cosc;
	*eta = (cd0*sd - sd0 * cd * cos(dra)) / cosc;
	return true;
}

static void gnomonic_inverse(double xi, double eta, double ra0, double dec0, double *ra, double *dec) {
	double rho2 = xi * xi + eta * eta;
	if (rho2 <= 0.0) {
		*ra = wrap_2pi(ra0);
		*dec = dec0;
		return;
	}
	double rho = sqrt(rho2);
	double c = atan(rho);
	double sinc = sin(c), cosc = cos(c);
	double sd0 = sin(dec0), cd0 = cos(dec0);
	double dec_out = asin(cosc * sd0 + (eta * sinc * cd0) / rho);
	double num = xi * sinc;
	double den = rho * cd0 * cosc - eta * sd0 * sinc;
	double ra_out = ra0 + atan2(num, den);
	*ra = wrap_2pi(ra_out);
	*dec = dec_out;
}

static bool solve_3x3(double A[3][3], double b[3], double out[3]) {
	double M[3][4] = {
		{ A[0][0] + 1e-12, A[0][1], A[0][2], b[0] },
		{ A[1][0], A[1][1] + 1e-12, A[1][2], b[1] },
		{ A[2][0], A[2][1], A[2][2] + 1e-12, b[2] },
	};
	// Gaussian elimination with partial pivoting
	for (int col = 0; col < 3; col++) {
		// Find pivot
		int piv = col;
		double best = fabs(M[col][col]);
		for (int r = col + 1; r < 3; r++) {
			double v = fabs(M[r][col]);
			if (v > best) { best = v; piv = r; }
		}
		if (best < 1e-12) {
			return false;
		}
		// Swap rows if needed
		if (piv != col) {
			for (int k = col; k < 4; k++) {
				double tmp = M[col][k];
				M[col][k] = M[piv][k];
				M[piv][k] = tmp;
			}
		}
		// Normalize pivot row
		double div = M[col][col];
		for (int k = col; k < 4; k++) {
			M[col][k] /= div;
		}
		// Eliminate column from other rows
		for (int r = 0; r < 3; r++) {
			if (r == col) continue;
			double factor = M[r][col];
			for (int k = col; k < 4; k++) {
				M[r][k] -= factor * M[col][k];
			}
		}
	}
	out[0] = M[0][3];
	out[1] = M[1][3];
	out[2] = M[2][3];
	return true;
}

static bool fit_affine_with_rejection(StarMatch *matches, int matching_stars_count, double ra0, double dec0, Affine2x2T *T, double *rms_residual) {
	if (matching_stars_count < 3) {
		return false;
	}
	int max_iterations = 10;
	double best_rms = 1e30;
	Affine2x2T best_T = { 0 };
	int best_stars_used = 0;
	// Start with all points
	for (size_t i = 0; i < matching_stars_count; i++) {
		matches[i].used = true;
	}
	for (int iter = 0; iter < max_iterations; iter++) {
		// Build normal equations using only current inliers
		double Sxx = 0, Sxy = 0, Sx1 = 0;
		double Syy = 0, Sy1 = 0;
		double S11 = 0;
		double bx0 = 0, bx1 = 0, bx2 = 0; // for xi
		double by0 = 0, by1 = 0, by2 = 0; // for eta
		int stars_used = 0;
		for (int i = 0; i < matching_stars_count; i++) {
			if (!matches[i].used) {
				continue;
			}
			double xi, eta;
			if (!gnomonic_forward(ra0, dec0, matches[i].catalog->ra_x, matches[i].catalog->dec_y, &xi, &eta)) {
				matches[i].used = false;
				continue;
			}
			double x = matches[i].image->ra_x;
			double y = matches[i].image->dec_y;
			Sxx += x * x;
			Sxy += x * y;
			Sx1 += x;
			Syy += y * y;
			Sy1 += y;
			S11 += 1.0;
			bx0 += x * xi;
			bx1 += y * xi;
			bx2 += xi;
			by0 += x * eta;
			by1 += y * eta;
			by2 += eta;
			stars_used++;
		}
		if (stars_used < 3) {
			return false;
		}
		double A[3][3] = {
			{ Sxx, Sxy, Sx1 },
			{ Sxy, Syy, Sy1 },
			{ Sx1, Sy1, S11 }
		};
		double bxi[3] = { bx0, bx1, bx2 };
		double beta[3] = { by0, by1, by2 };
		double pxi[3], peta[3];
		if (!solve_3x3(A, bxi, pxi) || !solve_3x3(A, beta, peta)) {
			return false;
		}
		// Compute residuals
		double sum_res2 = 0;
		double residuals[matching_stars_count];
		int valid_count = 0;
		for (int i = 0; i < matching_stars_count; i++) {
			if (!matches[i].used) {
				continue;
			}
			double xi_pred = pxi[0] * matches[i].image->ra_x + pxi[1] * matches[i].image->dec_y + pxi[2];
			double eta_pred = peta[0] * matches[i].image->ra_x + peta[1] * matches[i].image->dec_y + peta[2];
			double xi_true, eta_true;
			if (!gnomonic_forward(ra0, dec0, matches[i].catalog->ra_x, matches[i].catalog->dec_y, &xi_true, &eta_true)) {
				residuals[i] = 1e30;
				continue;
			}
			double res = sqrt((xi_pred - xi_true)*(xi_pred - xi_true) + (eta_pred - eta_true)*(eta_pred - eta_true));
			residuals[i] = res;
			sum_res2 += res * res;
			valid_count++;
		}
		if (valid_count < 3) {
			return false;
		}
		double current_rms = sqrt(sum_res2 / valid_count);
		// Keep best solution
		if (current_rms < best_rms) {
			best_rms = current_rms;
			best_T.a11 = pxi[0]; best_T.a12 = pxi[1]; best_T.b1 = pxi[2];
			best_T.a21 = peta[0]; best_T.a22 = peta[1]; best_T.b2 = peta[2];
			best_stars_used = stars_used;
		}
		// Sigma-clipping: reject outliers
		double mean = 0, std = 0;
		for (int i = 0; i < matching_stars_count; i++) {
			if (matches[i].used) {
				mean += residuals[i];
			}
		}
		mean /= stars_used;
		for (int i = 0; i < matching_stars_count; i++) {
			if (matches[i].used) {
				std += (residuals[i] - mean) * (residuals[i] - mean);
			}
		}
		std = sqrt(std / (stars_used - 1));
		int changed = 0;
		for (int i = 0; i < matching_stars_count; i++) {
			if (matches[i].used && residuals[i] > mean + SIGMA_REJECTION * std) {
				matches[i].used = false;
				changed = 1;
			}
		}
		// Stop if no more outliers or too few points remain
		if (!changed || stars_used < MIN_MATCHES) {
			break;
		}
	}
	*T = best_T;
	*rms_residual = best_rms;
	return best_stars_used >= MIN_MATCHES;
}

static void estimate_errors(StarMatch *matches, int matching_stars_count, double ra0, double dec0, Affine2x2T *T, double *ra_err, double *dec_err) {
	// Compute residuals and estimate variance
	double sum_xi_res2 = 0, sum_eta_res2 = 0;
	int stars_used = 0;
	for (int i = 0; i < matching_stars_count; i++) {
		if (!matches[i].used) {
			continue;
		}
		double xi_pred = T->a11 * matches[i].image->ra_x + T->a12 * matches[i].image->dec_y + T->b1;
		double eta_pred = T->a21 * matches[i].image->ra_x + T->a22 * matches[i].image->dec_y + T->b2;
		double xi_true, eta_true;
		if (!gnomonic_forward(ra0, dec0, matches[i].catalog->ra_x, matches[i].catalog->dec_y, &xi_true, &eta_true)) {
			continue;
		}
		sum_xi_res2 += (xi_pred - xi_true) * (xi_pred - xi_true);
		sum_eta_res2 += (eta_pred - eta_true) * (eta_pred - eta_true);
		stars_used++;
	}
	if (stars_used <= 3) {
		*ra_err = *dec_err = 0;
		return;
	}
	// Variance of residuals (xi and eta assumed independent)
	double var_xi = sum_xi_res2 / (stars_used - 3);
	double var_eta = sum_eta_res2 / (stars_used - 3);
	// Convert to RA/Dec errors (rough approximation for small fields)
	// At the tangent point, 1 radian in xi/eta ≈ 1 radian in RA/cos(Dec)
	*ra_err = sqrt(var_xi) / cos(dec0);
	*dec_err = sqrt(var_eta);
}

static bool solve_image_center(StarMatch *matches, int matching_stars_count, double img_w, double img_h, Solution *sol, double center_ra, double center_dec) {
	if (matching_stars_count < MIN_MATCHES) {
		return 0;
	}
	
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Initial verification:");
	double *scales = malloc(sizeof(double) * matching_stars_count * matching_stars_count);
	int k = 0;
	for (int i = 0; i < matching_stars_count; i++) {
		for (int j = i + 1; j < matching_stars_count; j++) {
			double img_dist = planar_distance(matches[i].image->ra_x, matches[i].image->dec_y, matches[j].image->ra_x, matches[j].image->dec_y);
			double cat_dist = angular_distance(center_ra, center_dec, matches[i].catalog->ra_x, matches[i].catalog->dec_y, matches[j].catalog->ra_x, matches[j].catalog->dec_y);
			double scale = cat_dist / img_dist; // radians per pixel
			INDIGO_DRIVER_LOG(DRIVER_NAME, "  Pair %d-%d: %.1f px -> %.6f rad (%.2f\"/px)", i, j, img_dist, cat_dist, scale * RAD_TO_DEG * 3600);
			scales[k++] = scale;
		}
	}
	qsort(scales, k, sizeof(double), compare_doubles);
	double median = scales[k/2];
	INDIGO_DRIVER_LOG(DRIVER_NAME, "  median -> %.2f\"/px", median * RAD_TO_DEG * 3600);

	
	// Initial tangent point estimate
	double ra0, dec0;
	estimate_tangent_point(matches, matching_stars_count, &ra0, &dec0);
	double xc = (img_w - 1.0) * 0.5;
	double yc = (img_h - 1.0) * 0.5;
	Affine2x2T T;
	double ra_c = ra0, dec_c = dec0;
	double rms_residual = 0;
	int converged = 0;
	for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
		// Fit affine with outlier rejection
		if (!fit_affine_with_rejection(matches, matching_stars_count, ra0, dec0, &T, &rms_residual)) {
			return false;
		}
		// Apply affine to image center
		double xi_c = T.a11 * xc + T.a12 * yc + T.b1;
		double eta_c = T.a21 * xc + T.a22 * yc + T.b2;
		// Convert to RA/Dec
		double prev_ra = ra_c, prev_dec = dec_c;
		gnomonic_inverse(xi_c, eta_c, ra0, dec0, &ra_c, &dec_c);
		// Update tangent point
		ra0 = ra_c;
		dec0 = dec_c;
		// Check convergence
		double ra_diff = wrap_pi(ra_c - prev_ra);
		double dec_diff = dec_c - prev_dec;
		if (fabs(ra_diff) < CONVERGENCE_THRESHOLD_RAD && fabs(dec_diff) < CONVERGENCE_THRESHOLD_RAD) {
			converged = 1;
			break;
		}
	}
	if (!converged) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Warning: Solution did not fully converge");
	}
	// Count used matches
	int stars_used = 0;
	for (int i = 0; i < matching_stars_count; i++) {
		if (matches[i].used) {
			stars_used++;
		}
	}
	// Estimate errors
	double ra_err, dec_err;
	estimate_errors(matches, matching_stars_count, ra0, dec0, &T, &ra_err, &dec_err);
	// Compute pixel scale and rotation
	double scale_x = sqrt(T.a11 * T.a11 + T.a21 * T.a21);  // rad/pixel
	double scale_y = sqrt(T.a12 * T.a12 + T.a22 * T.a22);  // rad/pixel
	double rotation = atan2(T.a21, T.a11);  // east of north
	// Fill solution
	sol->ra = ra_c;
	sol->dec = dec_c;
	sol->ra_err = ra_err;
	sol->dec_err = dec_err;
	sol->pix_scale_x = scale_x;
	sol->pix_scale_y = scale_y;
	sol->rotation = rotation;
	sol->stars_used = stars_used;
	sol->rms_residual = rms_residual;
	return true;
}

/* ============================================================================
	 Main Solving Function
============================================================================ */

static bool solve_at_position(double ra0, double dec0, Quad *image_quads, int num_image_quads, Star *catalog_stars, int num_catalog_stars, int width, int height, double search_radius) {
	Quad *catalog_quads = malloc(sizeof(Quad) * MAX_QUADS);
	if (!catalog_quads) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Memory allocation failed for catalog quads!");
		return false;
	}
	int num_catalog_quads = find_catalog_quads(ra0, dec0, catalog_stars, num_catalog_stars, catalog_quads, MAX_QUADS, search_radius);
	if (num_catalog_quads < 5) {
		free(catalog_quads);
		return false;
	}
	StarMatch *matches = malloc(sizeof(StarMatch) * MAX_MATCHES);
	int num_matching_stars = find_matching_stars(ra0, dec0, image_quads, num_image_quads, catalog_quads, num_catalog_quads, matches, MAX_MATCHES);
	Solution solution;
	if (solve_image_center(matches, num_matching_stars, width, height, &solution, ra0, dec0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%RA = %sh +/- %sh", indigo_dtos(solution.ra * RAD_TO_HOUR, NULL), indigo_dtos(solution.ra_err * RAD_TO_HOUR, NULL));
		INDIGO_DRIVER_LOG(DRIVER_NAME, "DEC = %s° +/- %s°", indigo_dtos(solution.dec * RAD_TO_DEG, NULL), indigo_dtos(solution.dec_err * RAD_TO_DEG, NULL));
		INDIGO_DRIVER_LOG(DRIVER_NAME, "SCALE = %g\"/px / %g\"/px", solution.pix_scale_x * RAD_TO_DEG * 3600, solution.pix_scale_y * RAD_TO_DEG * 3600);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "ROTATION = %g°", solution.rotation * RAD_TO_DEG);
	}
	free(matches);
	free(catalog_quads);
	return true;
}

// --------------------------------------------------------------------------------

#define solver_save_config indigo_platesolver_save_config

static void solver_abort(indigo_device *device) {
}

static bool solver_solve(indigo_device *device, indigo_platesolver_task *task) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		void *image = task->image;
		indigo_raw_header *header = (indigo_raw_header *)image;
		indigo_star_detection *detected = malloc(sizeof(indigo_star_detection) * MAX_DETECTION_COUNT);
		indigocat_star_entry *catalogue = indigocat_get_star_data();
		Star *catalogue_stars = malloc(sizeof(Star) * indigo_star_data_count);
		int catalogue_count = 0;
		for (int i = 0; i < indigo_star_data_count; i++) {
			if (catalogue[i].mag < 6.8) {
				catalogue_stars[i].ra_x = catalogue[i].ra * HOUR_TO_RAD;
				catalogue_stars[i].dec_y = catalogue[i].dec * DEG_TO_RAD;
				catalogue_stars[i].id = catalogue[i].hip;
				catalogue_count++;
			}
		}
		bool result = true;
		int detection_count = 0;
		if (indigo_find_stars_precise(header->signature, image + sizeof(indigo_raw_header), 5, header->width, header->height, MAX_DETECTION_COUNT, detected, &detection_count) == INDIGO_OK && detection_count > 20) {
			Star *image_stars = malloc(sizeof(Star) * detection_count);
			int image_count = 0;
			for (int i = 0; i < detection_count; i++) {
				if (!detected[i].oversaturated && !detected[i].close_to_other && detected[i].luminance > 12) {
					image_stars[image_count].ra_x = detected[i].x;
					image_stars[image_count].dec_y = detected[i].y;
					add_marker(task, detected[i].x, detected[i].y);
					image_count++;
				}
			}
			INDIGO_DRIVER_LOG(DRIVER_NAME, "%d stars detected", image_count);
			Quad *image_quads = malloc(sizeof(Quad) * MAX_QUADS);
			int num_image_quads = find_image_quads(image_stars, image_count, image_quads, MAX_QUADS, header->height * 0.01, header->width * 0.9);
			if (solve_at_position(6.5 * HOUR_TO_RAD, 5.0 * DEG_TO_RAD, image_quads, num_image_quads, catalogue_stars, catalogue_count, header->width, header->height, 4 * DEG_TO_RAD)) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Solved");
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Plate solving failed");
				result = false;
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Star detection failed");
			result = false;
		}
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		return result;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solver is busy");
	return false;
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_platesolver_device_attach(device, DRIVER_NAME, DRIVER_VERSION, 0) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		SOLVER_DEVICE_PRIVATE_DATA->platesolver.save_config = solver_save_config;
		SOLVER_DEVICE_PRIVATE_DATA->platesolver.solve = solver_solve;
		SOLVER_DEVICE_PRIVATE_DATA->platesolver.abort = solver_abort;
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client) {
		return INDIGO_OK;
	}
	return indigo_platesolver_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client) {
		return INDIGO_OK;
	}
	return indigo_platesolver_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	return indigo_platesolver_device_detach(device);
}

// -------------------------------------------------------------------------------- Initialization

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_solver(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		SOLVER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		SOLVER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_platesolver_client_attach,
		indigo_platesolver_define_property,
		indigo_platesolver_update_property,
		indigo_platesolver_delete_property,
		NULL,
		indigo_platesolver_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, SOLVER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT: {
			void *private_data = indigo_safe_malloc(sizeof(solver_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;
		}
		case INDIGO_DRIVER_SHUTDOWN: {
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			break;
		}
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
