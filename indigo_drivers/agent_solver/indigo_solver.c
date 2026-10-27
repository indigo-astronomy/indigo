// Copyright (c) 2026 CloudMakers, s. r. o.
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

/** INDIGO quad based plate solver - catalog index, image quads, matching and WCS fit
 \file indigo_solver.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>

#include "indigo_solver_utils.h"
#include "indigo_solver.h"

const float indigo_solver_index_fovs[INDIGO_SOLVER_INDEX_FOV_COUNT] = { 1, 2, 5, 10, 20 };

// Hash buckets keyed by the quantised (x3, y3) code coordinates, both in [0, INDIGO_SOLVER_CODE_QUANTUM].
#define BUCKET_SIDE		(INDIGO_SOLVER_CODE_QUANTUM + 1)
#define BUCKET_COUNT	(BUCKET_SIDE * BUCKET_SIDE)

static int compare_quads(const void *a, const void *b) {
	const indigo_solver_quad *qa = (const indigo_solver_quad*)a;
	const indigo_solver_quad *qb = (const indigo_solver_quad*)b;
	int p = (qa->stars[0] > qb->stars[0]) - (qa->stars[0] < qb->stars[0]);
	if (p != 0) {
		return p;
	}
	p = (qa->stars[1] > qb->stars[1]) - (qa->stars[1] < qb->stars[1]);
	if (p != 0) {
		return p;
	}
	p = (qa->stars[2] > qb->stars[2]) - (qa->stars[2] < qb->stars[2]);
	if (p != 0) {
		return p;
	}
	return (qa->stars[3] > qb->stars[3]) - (qa->stars[3] < qb->stars[3]);
}

static int compare_quads_by_hash(const void *a, const void *b) {
	const indigo_solver_quad *qa = (const indigo_solver_quad*)a;
	const indigo_solver_quad *qb = (const indigo_solver_quad*)b;
	return (qa->hash > qb->hash) - (qa->hash < qb->hash);
}

static int compare_star_matches_by_votes(const void *a, const void *b) {
	const indigo_solver_star_match *ma = (const indigo_solver_star_match*)a;
	const indigo_solver_star_match *mb = (const indigo_solver_star_match*)b;
	return (mb->votes > ma->votes) - (mb->votes < ma->votes);
}

static bool fit_2d_line(const float *dx, const float *dy, const float *dz, int n, float *a_out, float *b_out, float *c_out) {
	if (n < 3) {
		return false;
	}
	// the normal equations are evaluated in double, in float the determinant of three points cancels to noise
	double sum1 = (double)n;
	double sum_dx = 0, sum_dy = 0, sum_dz = 0;
	double sum_dx2 = 0, sum_dy2 = 0, sum_dxdy = 0;
	double sum_dx_dz = 0, sum_dy_dz = 0;
	for (int i = 0; i < n; i++) {
		double x = dx[i];
		double y = dy[i];
		double z = dz[i];
		sum_dx += x;
		sum_dy += y;
		sum_dz += z;
		sum_dx2 += x * x;
		sum_dy2 += y * y;
		sum_dxdy += x * y;
		sum_dx_dz += x * z;
		sum_dy_dz += y * z;
	}
	// Compute determinant of the normal matrix
	double det = sum1 * (sum_dx2 * sum_dy2 - sum_dxdy * sum_dxdy) - sum_dx * (sum_dx * sum_dy2 - sum_dxdy * sum_dy) + sum_dy * (sum_dx * sum_dxdy - sum_dx2 * sum_dy);
	if (fabs(det) < 1e-14) {
		return false;
	}
	// Cramer's rule for a, b, c
	double det_a = sum_dz * (sum_dx2 * sum_dy2 - sum_dxdy * sum_dxdy) - sum_dx_dz * (sum_dx * sum_dy2 - sum_dxdy * sum_dy) + sum_dy_dz * (sum_dx * sum_dxdy - sum_dx2 * sum_dy);
	double det_b = sum1 * (sum_dx_dz * sum_dy2 - sum_dxdy * sum_dy_dz) - sum_dx * (sum_dz * sum_dy2 - sum_dy_dz * sum_dy) + sum_dy * (sum_dz * sum_dxdy - sum_dx_dz * sum_dy);
	double det_c = sum1 * (sum_dx2 * sum_dy_dz - sum_dx_dz * sum_dxdy) - sum_dx * (sum_dx * sum_dy_dz - sum_dx_dz * sum_dy) + sum_dz * (sum_dx * sum_dxdy - sum_dx2 * sum_dy);
	*a_out = det_a / det;
	*b_out = det_b / det;
	*c_out = det_c / det;
	return true;
}

void indigo_solver_bin_image(indigo_raw_header *image, int bin) {
	if (bin < 2) {
		return;
	}
	if (image->signature != INDIGO_RAW_MONO16) {
		indigo_debug("%s: only 16 bit mono images are binned, signature = %08x", __FUNCTION__, image->signature);
		return;
	}
	indigo_debug("%s: original image width = %d, height = %d", __FUNCTION__, image->width, image->height);
	uint16_t *src = (uint16_t *)(image + 1), *dst = src;
	int src_width = image->width, dst_width = src_width / bin;
	int src_height = image->height, dst_height = src_height / bin;
	for (int dst_y = 0; dst_y < dst_height; dst_y++) {
		int dst_y_bin = dst_y * bin;
		for (int dst_x = 0; dst_x < dst_width; dst_x++) {
			uint32_t sum = 0;
			int dst_x_bin = dst_x * bin;
			for (int by = 0; by < bin; by++) {
				int src_y = dst_y_bin + by;
				const uint16_t *row = src + src_y * src_width;
				for (int bx = 0; bx < bin; bx++) {
					sum += row[dst_x_bin + bx];
				}
			}
			dst[dst_y * dst_width + dst_x] = (uint16_t)(sum / (bin * bin));
		}
	}
	image->width = dst_width;
	image->height = dst_height;
	indigo_debug("%s: binned image width = %d, height = %d", __FUNCTION__, image->width, image->height);
}

int indigo_solver_image_stars(indigo_raw_header *image, indigo_solver_star *stars, int max_star_count) {
	indigo_star_detection *all_stars = indigo_safe_malloc(sizeof(indigo_star_detection) * max_star_count);
	int stars_found, count = 0;
	indigo_find_stars_precise_threshold(image->signature, image + 1, 16, 3.5, image->width, image->height, max_star_count, all_stars, &stars_found);
	for (int i = 0; i < stars_found; i++) {
		indigo_star_detection *as = all_stars + i;
		indigo_solver_star *s = stars + (count++);
		s->magnitude = 1 / as->luminance;
		s->x = as->x;
		s->y = as->y;
		s->ra = s->dec = 0;
		s->used = 0;
	}
	indigo_safe_free(all_stars);
	indigo_debug("%s: detected %d image stars", __FUNCTION__, count);
	return count;
}

// Picks the largest available index whose FOV is at most 1.5 x the geometric mean of the field size
// (enough index stars in the field while quads still fit inside it); falls back to the smallest available index.
bool indigo_solver_select_index(const char *index_directory, const float *fovs, int fov_count, float field_width, float field_height, char *path, size_t size, float *fov_out) {
	float target = 1.5 * sqrt(field_width * field_height) * RAD_TO_DEG;
	float selected = 0;
	char candidate[PATH_MAX];
	for (int i = 0; i < fov_count; i++) {
		float fov = fovs[i];
		indigo_solver_index_path(candidate, sizeof(candidate), index_directory, fov);
		if (access(candidate, R_OK) == 0 && (selected == 0 || fov <= target)) {
			selected = fov;
			strncpy(path, candidate, size);
		}
	}
	if (selected == 0) {
		indigo_error("%s: no tyc2-*deg.isix index file found in %s", __FUNCTION__, index_directory && index_directory[0] ? index_directory : "the working directory");
		return false;
	}
	*fov_out = selected;
	indigo_debug("%s: field %.2f° x %.2f°, using %s", __FUNCTION__, field_width * RAD_TO_DEG, field_height * RAD_TO_DEG, path);
	return true;
}

bool indigo_solver_load_index(const char *path, indigo_solver_star **stars, int *star_count, indigo_solver_quad **quads, int *quad_count) {
	*stars = NULL;
	*star_count = 0;
	*quads = NULL;
	*quad_count = 0;
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		indigo_error("%s: failed to open %s", __FUNCTION__, path);
		return false;
	}
	indigo_solve_index_header header;
	if (!indigo_solver_index_read_header(file, &header)) {
		fclose(file);
		return false;
	}
	indigo_solver_index_star *index_stars = indigo_safe_malloc(sizeof(indigo_solver_index_star) * header.star_count);
	indigo_solver_index_quad *index_quads = indigo_safe_malloc(sizeof(indigo_solver_index_quad) * header.quad_count);
	bool ok = indigo_solver_index_read_stars(file, index_stars, header.star_count) && indigo_solver_index_read_quads(file, index_quads, header.quad_count);
	fclose(file);
	if (!ok) {
		indigo_error("%s: %s is truncated", __FUNCTION__, path);
		indigo_safe_free(index_stars);
		indigo_safe_free(index_quads);
		return false;
	}
	indigo_solver_star *s = indigo_safe_malloc(sizeof(indigo_solver_star) * header.star_count);
	memset(s, 0, sizeof(indigo_solver_star) * header.star_count);
	for (uint32_t i = 0; i < header.star_count; i++) {
		s[i].id = i;
		s[i].ra = index_stars[i].ra;
		s[i].dec = index_stars[i].dec;
	}
	indigo_safe_free(index_stars);
	indigo_solver_quad *q = indigo_safe_malloc(sizeof(indigo_solver_quad) * header.quad_count);
	memset(q, 0, sizeof(indigo_solver_quad) * header.quad_count);
	bool sorted = true;
	for (uint32_t i = 0; i < header.quad_count; i++) {
		indigo_solver_index_quad *iq = index_quads + i;
		for (int k = 0; k < 4; k++) {
			if (iq->stars[k] >= header.star_count) {
				indigo_error("%s: %s is corrupted (quad %u references star %u of %u)", __FUNCTION__, path, i, iq->stars[k], header.star_count);
				indigo_safe_free(index_quads);
				indigo_safe_free(s);
				indigo_safe_free(q);
				return false;
			}
			q[i].stars[k] = s + iq->stars[k];
		}
		q[i].hash = iq->hash;
		q[i].baseline = iq->baseline;
		int hx3, hy3, hx4, hy4;
		indigo_solver_quad_hash_components(iq->hash, &hx3, &hy3, &hx4, &hy4);
		q[i].x3 = (float)hx3 / INDIGO_SOLVER_CODE_QUANTUM;
		q[i].y3 = (float)hy3 / INDIGO_SOLVER_CODE_QUANTUM;
		q[i].x4 = (float)hx4 / INDIGO_SOLVER_CODE_QUANTUM;
		q[i].y4 = (float)hy4 / INDIGO_SOLVER_CODE_QUANTUM;
		if (i > 0 && q[i - 1].hash > q[i].hash) {
			sorted = false;
		}
	}
	indigo_safe_free(index_quads);
	if (!sorted) {
		indigo_debug("%s: %s is not sorted by hash, sorting", __FUNCTION__, path);
		qsort(q, header.quad_count, sizeof(indigo_solver_quad), compare_quads_by_hash);
	}
	*stars = s;
	*star_count = header.star_count;
	*quads = q;
	*quad_count = header.quad_count;
	indigo_debug("%s: using %d catalog stars and %d catalog quads from %s (%g°)", __FUNCTION__, header.star_count, header.quad_count, path, header.fov);
	return true;
}

bool indigo_solver_catalog(const char *index_directory, float field_width, float field_height, indigo_solver_star **stars, int *star_count, indigo_solver_quad **quads, int *quad_count) {
	char path[PATH_MAX];
	float fov;
	if (!indigo_solver_select_index(index_directory, indigo_solver_index_fovs, INDIGO_SOLVER_INDEX_FOV_COUNT, field_width, field_height, path, sizeof(path), &fov)) {
		*stars = NULL;
		*star_count = 0;
		*quads = NULL;
		*quad_count = 0;
		return false;
	}
	return indigo_solver_load_index(path, stars, star_count, quads, quad_count);
}

void indigo_solver_free_catalog(indigo_solver_star *stars, indigo_solver_quad *quads) {
	indigo_safe_free(quads);
	indigo_safe_free(stars);
}

int indigo_solver_build_image_quads(indigo_solver_star *stars, int star_count, int width, int height, int cell_size, int *used_stars, indigo_solver_quad *quads, int max_quad_count) {
	int cell_cols = width / cell_size;
	int cell_rows = height / cell_size;
	struct cell {
		int row, col;
		indigo_solver_star *star;
	} cells[cell_rows][cell_cols];
	for (int j = 0; j < cell_rows; j++) {
		for (int i = 0; i < cell_cols; i++) {
			cells[j][i].row = j;
			cells[j][i].col = i;
			cells[j][i].star = NULL;
		}
	}
	for (int i = 0; i < star_count; i++) {
		indigo_solver_star *star = stars + i;
		int x = star->x / cell_size;
		int y = star->y / cell_size;
		if (x < cell_cols && y < cell_rows && (cells[y][x].star == NULL || cells[y][x].star->magnitude > star->magnitude)) {
			cells[y][x].star = star;
		}
	}
	int count = 0;
	for (int j = 0; j < cell_rows; j++) {
		for (int i = 0; i < cell_cols; i++) {
			indigo_solver_star *star = cells[j][i].star;
			if (star) {
				star->used++;
				count++;
			}
		}
	}
	*used_stars = count;
	count = 0;
	int cell_count = cell_cols * cell_rows;
	struct cell *c = (struct cell *)cells;
	for (int i = 0; i < cell_count; i++) {
		if (c[i].star == NULL) {
			continue;
		}
		for (int j = i + 1; j < cell_count; j++) {
			if (c[j].star == NULL) {
				continue;
			}
			for (int k = j + 1; k < cell_count; k++) {
				if (c[k].star == NULL) {
					continue;
				}
				for (int l = k + 1; l < cell_count; l++) {
					if (c[l].star == NULL) {
						continue;
					}
					if (count == max_quad_count) {
						goto exit;
					}
					indigo_solver_make_quad(quads + count, c[i].star, c[j].star, c[k].star, c[l].star);
					count++;
				}
			}
		}
	}
exit:
	qsort(quads, count, sizeof(indigo_solver_quad), compare_quads);
	indigo_debug("%s: %d stars used to form %d quads", __FUNCTION__, *used_stars, count);
	return count;
}

static inline int bucket_key(int hx3, int hy3) {
	return hx3 * BUCKET_SIDE + hy3;
}

// starts[key] .. starts[key + 1] is the range of catalog quads with the given (x3, y3) bucket; the catalog is hash-sorted,
// so the quads of one bucket are contiguous.
static uint32_t *build_buckets(indigo_solver_quad *catalog_quads, int catalog_quad_count) {
	uint32_t *starts = indigo_safe_malloc((BUCKET_COUNT + 1) * sizeof(uint32_t));
	memset(starts, 0, (BUCKET_COUNT + 1) * sizeof(uint32_t));
	for (int i = 0; i < catalog_quad_count; i++) {
		int hx3, hy3, hx4, hy4;
		indigo_solver_quad_hash_components(catalog_quads[i].hash, &hx3, &hy3, &hx4, &hy4);
		starts[bucket_key(hx3, hy3) + 1]++;
	}
	for (int k = 1; k <= BUCKET_COUNT; k++) {
		starts[k] += starts[k - 1];
	}
	return starts;
}

int indigo_solver_match_quads(indigo_solver_quad *image_quads, int image_quad_count, indigo_solver_quad *catalog_quads, int catalog_quad_count, indigo_solver_quad_match *matches, int max_match_count) {
	int count_by_hash = 0, count_by_scale = 0;
	for (int i = 1; i < catalog_quad_count; i++) {
		if (catalog_quads[i - 1].hash > catalog_quads[i].hash) {
			indigo_debug("%s: catalog quads are not sorted by hash, sorting", __FUNCTION__);
			qsort(catalog_quads, catalog_quad_count, sizeof(indigo_solver_quad), compare_quads_by_hash);
			break;
		}
	}
	uint32_t *starts = build_buckets(catalog_quads, catalog_quad_count);
	const int t = INDIGO_SOLVER_HASH_TOLERANCE;
	for (int i = 0; i < image_quad_count; i++) {
		indigo_solver_quad *image_quad = image_quads + i;
		int hx3, hy3, hx4, hy4;
		indigo_solver_quad_hash_components(image_quad->hash, &hx3, &hy3, &hx4, &hy4);
		for (int x3 = hx3 - t; x3 <= hx3 + t; x3++) {
			if (x3 < 0 || x3 >= BUCKET_SIDE) {
				continue;
			}
			for (int y3 = hy3 - t; y3 <= hy3 + t; y3++) {
				if (y3 < 0 || y3 >= BUCKET_SIDE) {
					continue;
				}
				int key = bucket_key(x3, y3);
				for (uint32_t j = starts[key]; j < starts[key + 1]; j++) {
					indigo_solver_quad *catalog_quad = catalog_quads + j;
					int cx3, cy3, cx4, cy4;
					indigo_solver_quad_hash_components(catalog_quad->hash, &cx3, &cy3, &cx4, &cy4);
					if (abs(cx4 - hx4) > t || abs(cy4 - hy4) > t) {
						continue;
					}
					if (count_by_hash == max_match_count) {
						indigo_debug("%s: match limit %d reached", __FUNCTION__, max_match_count);
						goto exit;
					}
					indigo_solver_quad_match *match = matches + count_by_hash;
					match->image_quad = image_quad;
					match->catalog_quad = catalog_quad;
					match->scale = image_quad->baseline / catalog_quad->baseline;
					match->votes = 0;
					count_by_hash++;
				}
			}
		}
	}
exit:
	indigo_safe_free(starts);
	indigo_debug("%s: %d quads matched by hash (tolerance +-%d)", __FUNCTION__, count_by_hash, t);
	// Each pair of matches sharing an (image star, catalog star) correspondence votes for both.
	float best_scale = 0;
	int best_votes = 0;
	for (int i = 0; i < count_by_hash; i++) {
		indigo_solver_quad_match *qmi = matches + i;
		for (int j = i + 1; j < count_by_hash; j++) {
			indigo_solver_quad_match *qmj = matches + j;
			for (int ii = 0; ii < 4; ii++) {
				indigo_solver_star *isi = qmi->image_quad->stars[ii];
				indigo_solver_star *csi = qmi->catalog_quad->stars[ii];
				for (int jj = 0; jj < 4; jj++) {
					indigo_solver_star *isj = qmj->image_quad->stars[jj];
					indigo_solver_star *csj = qmj->catalog_quad->stars[jj];
					if (isi == isj && csi == csj) {
						qmi->votes++;
						qmj->votes++;
						goto next;
					}
				}
			}
		next:
			;
		}
		if (best_votes < qmi->votes) {
			best_votes = qmi->votes;
			best_scale = qmi->scale;
		}
	}
	indigo_debug("%s: %d votes for scale %g", __FUNCTION__, best_votes, best_scale);
	if (best_scale == 0) {
		return 0;
	}
	for (int i = 0; i < count_by_hash; i++) {
		float scale = matches[i].scale;
		if (fabs(1 - scale / best_scale) < INDIGO_SOLVER_SCALE_TOLERANCE) {
			matches[count_by_scale++] = matches[i];
		}
	}
	indigo_debug("%s: %d quads matched by scale (tolerance %g)", __FUNCTION__, count_by_scale, INDIGO_SOLVER_SCALE_TOLERANCE);
	return count_by_scale;
}

int indigo_solver_match_stars(indigo_solver_quad_match *quad_matches, int quad_match_count, indigo_solver_star_match *star_matches, int max_star_match_count) {
	int count = 0, accepted = 0;
	// 1) Accumulate votes per (image star, catalog star) correspondence
	for (int i = 0; i < quad_match_count; i++) {
		indigo_solver_quad_match *quad_match = quad_matches + i;
		for (int j = 0; j < 4; j++) {
			indigo_solver_star *image_star = quad_match->image_quad->stars[j];
			indigo_solver_star *catalog_star = quad_match->catalog_quad->stars[j];
			bool add = true;
			for (int k = 0; k < count; k++) {
				indigo_solver_star_match *star_match = star_matches + k;
				if (star_match->image_star == image_star && star_match->catalog_star == catalog_star) {
					star_match->votes++;
					add = false;
					break;
				}
			}
			if (add) {
				if (count == max_star_match_count) {
					goto exit;
				}
				indigo_solver_star_match *star_match = star_matches + count;
				star_match->image_star = image_star;
				star_match->catalog_star = catalog_star;
				star_match->votes = 1;
				count++;
			}
		}
	}
exit:
	indigo_debug("%s: %d stars matched", __FUNCTION__, count);
	// 2) Best voted correspondences first, each image star and each catalog star used at most once
	qsort(star_matches, count, sizeof(indigo_solver_star_match), compare_star_matches_by_votes);
	for (int i = 0; i < count; i++) {
		bool conflict = false;
		for (int j = 0; j < accepted; j++) {
			if (star_matches[i].image_star == star_matches[j].image_star || star_matches[i].catalog_star == star_matches[j].catalog_star) {
				conflict = true;
				break;
			}
		}
		if (!conflict) {
			star_matches[accepted++] = star_matches[i];
		}
	}
	indigo_debug("%s: %d stars matched by votes", __FUNCTION__, accepted);
	return accepted;
}

#define FIT_MAX_ITERATIONS	20
#define FIT_MAX_RESIDUAL_PX	3.0
#define FIT_MIN_INLIERS			6
#define FIT_SEED_CANDIDATES	8

typedef struct {
	float a0, a1, a2;	// xi  ≈ a0 + a1 · Δx + a2 · Δy
	float b0, b1, b2;	// eta ≈ b0 + b1 · Δx + b2 · Δy
	float pixel;			// radians per pixel
	float rms;				// residual rms on the tangent plane, radians
	int n;
} affine_fit;

typedef struct {
	int count;
	const float *dx, *dy, *xi, *eta;
	const bool *valid;
	float *fx, *fy, *fxi, *feta;	// scratch
} fit_data;

static bool fit_affine(const fit_data *d, const bool *inlier, affine_fit *fit) {
	int n = 0;
	for (int i = 0; i < d->count; i++) {
		if (inlier[i]) {
			d->fx[n] = d->dx[i];
			d->fy[n] = d->dy[i];
			d->fxi[n] = d->xi[i];
			d->feta[n] = d->eta[i];
			n++;
		}
	}
	if (n < 3) {
		return false;
	}
	if (!fit_2d_line(d->fx, d->fy, d->fxi, n, &fit->a0, &fit->a1, &fit->a2) || !fit_2d_line(d->fx, d->fy, d->feta, n, &fit->b0, &fit->b1, &fit->b2)) {
		return false;
	}
	fit->pixel = (hypot(fit->a1, fit->b1) + hypot(fit->a2, fit->b2)) / 2.0;
	float sum2 = 0;
	for (int i = 0; i < d->count; i++) {
		if (inlier[i]) {
			float rx = d->xi[i] - (fit->a0 + fit->a1 * d->dx[i] + fit->a2 * d->dy[i]);
			float ry = d->eta[i] - (fit->b0 + fit->b1 * d->dx[i] + fit->b2 * d->dy[i]);
			sum2 += rx * rx + ry * ry;
		}
	}
	fit->rms = sqrt(sum2 / n);
	fit->n = n;
	return fit->pixel > 0 && isfinite(fit->rms);
}

// Starting from the seed inliers, refits and re-selects every match within FIT_MAX_RESIDUAL_PX of the fit until stable.
// The tolerance is absolute, a sigma based one would let a wrong seed accept everything. Returns the inlier count,
// 0 when the fit degenerates.
static int grow_fit(const fit_data *d, bool *inlier, affine_fit *fit) {
	for (int iteration = 0; iteration < FIT_MAX_ITERATIONS; iteration++) {
		if (!fit_affine(d, inlier, fit)) {
			return 0;
		}
		float limit = FIT_MAX_RESIDUAL_PX * fit->pixel;
		bool changed = false;
		int n = 0;
		for (int i = 0; i < d->count; i++) {
			float rx = d->xi[i] - (fit->a0 + fit->a1 * d->dx[i] + fit->a2 * d->dy[i]);
			float ry = d->eta[i] - (fit->b0 + fit->b1 * d->dx[i] + fit->b2 * d->dy[i]);
			bool in = d->valid[i] && hypot(rx, ry) <= limit;
			if (in != inlier[i]) {
				changed = true;
			}
			inlier[i] = in;
			n += in;
		}
		if (n < 3) {
			return 0;
		}
		if (!changed) {
			return n;
		}
	}
	return fit_affine(d, inlier, fit) ? fit->n : 0;
}

bool indigo_solver_compute_center(indigo_solver_star_match *matches, int match_count, int width, int height, float *ra_center, float *dec_center, float *rotation, float *scale, int *parity) {
	if (match_count < 3) {
		return false;
	}
	// 1. Best voted correspondences first; they seed the robust fit
	indigo_solver_star_match *sorted = indigo_safe_malloc(match_count * sizeof(indigo_solver_star_match));
	memcpy(sorted, matches, match_count * sizeof(indigo_solver_star_match));
	qsort(sorted, match_count, sizeof(indigo_solver_star_match), compare_star_matches_by_votes);
	// 2. Image offsets from the image centre
	float cx = width / 2.0, cy = height / 2.0;
	float *dx = indigo_safe_malloc(match_count * sizeof(float));
	float *dy = indigo_safe_malloc(match_count * sizeof(float));
	float *xi = indigo_safe_malloc(match_count * sizeof(float));
	float *eta = indigo_safe_malloc(match_count * sizeof(float));
	bool *valid = indigo_safe_malloc(match_count * sizeof(bool));
	bool *inlier = indigo_safe_malloc(match_count * sizeof(bool));
	bool *best_inlier = indigo_safe_malloc(match_count * sizeof(bool));
	fit_data data = {
		.count = match_count, .dx = dx, .dy = dy, .xi = xi, .eta = eta, .valid = valid,
		.fx = indigo_safe_malloc(match_count * sizeof(float)), .fy = indigo_safe_malloc(match_count * sizeof(float)),
		.fxi = indigo_safe_malloc(match_count * sizeof(float)), .feta = indigo_safe_malloc(match_count * sizeof(float))
	};
	for (int i = 0; i < match_count; i++) {
		dx[i] = sorted[i].image_star->x - cx;
		dy[i] = sorted[i].image_star->y - cy;
	}
	// 3. The tangent point is the catalog star of one of the best voted matches (a wrong match far from the field
	//    would put every true match on the wrong side of the sphere); every triple among the best voted matches seeds
	//    an affine fit that grows over consistent matches. The largest consistent set wins, so wrong correspondences
	//    (even half of them) do not pull the solution.
	int candidates = match_count < FIT_SEED_CANDIDATES ? match_count : FIT_SEED_CANDIDATES;
	affine_fit fit, best_fit;
	int best_n = 0;
	float ref_ra = 0, ref_dec = 0;
	for (int ref = 0; ref < candidates; ref++) {
		float ra0 = sorted[ref].catalog_star->ra;
		float dec0 = sorted[ref].catalog_star->dec;
		for (int i = 0; i < match_count; i++) {
			indigo_solver_star *cat = sorted[i].catalog_star;
			valid[i] = indigo_radec_to_tangent(ra0, dec0, cat->ra, cat->dec, xi + i, eta + i);
		}
		for (int i = 0; i < candidates; i++) {
			for (int j = i + 1; j < candidates; j++) {
				for (int k = j + 1; k < candidates; k++) {
					if (!valid[i] || !valid[j] || !valid[k]) {
						continue;
					}
					memset(inlier, 0, match_count * sizeof(bool));
					inlier[i] = inlier[j] = inlier[k] = true;
					int n = grow_fit(&data, inlier, &fit);
					if (n > best_n || (n == best_n && n > 0 && fit.rms < best_fit.rms)) {
						best_n = n;
						best_fit = fit;
						ref_ra = ra0;
						ref_dec = dec0;
						memcpy(best_inlier, inlier, match_count * sizeof(bool));
					}
				}
			}
		}
	}
	// 4. The rotation of a fit is measured against north at its tangent point, which is up to half a field away from
	//    the image centre; at high declination the meridians converge by a degree over that distance. The centre found
	//    with the reference star becomes the tangent point of a second fit over the same inliers, so the angle, the
	//    scale and the final centre refer to the image centre.
	if (best_n >= FIT_MIN_INLIERS) {
		float ra0, dec0;
		indigo_tangent_to_radec(ref_ra, ref_dec, best_fit.a0, best_fit.b0, &ra0, &dec0);
		for (int i = 0; i < match_count; i++) {
			indigo_solver_star *cat = sorted[i].catalog_star;
			valid[i] = indigo_radec_to_tangent(ra0, dec0, cat->ra, cat->dec, xi + i, eta + i);
		}
		memcpy(inlier, best_inlier, match_count * sizeof(bool));
		int n = grow_fit(&data, inlier, &fit);
		if (n >= FIT_MIN_INLIERS) {
			best_n = n;
			best_fit = fit;
			ref_ra = ra0;
			ref_dec = dec0;
		}
	}
	indigo_safe_free(dx);
	indigo_safe_free(dy);
	indigo_safe_free(xi);
	indigo_safe_free(eta);
	indigo_safe_free(valid);
	indigo_safe_free(inlier);
	indigo_safe_free(best_inlier);
	indigo_safe_free(data.fx);
	indigo_safe_free(data.fy);
	indigo_safe_free(data.fxi);
	indigo_safe_free(data.feta);
	indigo_safe_free(sorted);
	// a single matched quad gives four self-consistent pairs, a solution needs more than that
	if (best_n < FIT_MIN_INLIERS) {
		indigo_error("%s: no consistent set of matches found (%d of %d)", __FUNCTION__, best_n, match_count);
		return false;
	}
	// 5. Rotation, scale and parity in the convention of astrometry.net, so the WCS of this agent and of the
	//    astrometry agent agree: the parity is positive when the image is the sky seen in a mirror (east to the
	//    right with north up, the determinant of the pixel to tangent plane mapping is negative), and for that
	//    parity the angle is turned by 180 degrees; the eta row of the fit gives the angle for both parities.
	float angle = atan2(best_fit.b1, best_fit.b2) + M_PI;
	int mirrored = best_fit.a1 * best_fit.b2 - best_fit.a2 * best_fit.b1 < 0 ? 1 : -1;
	if (mirrored > 0) {
		angle += M_PI;
	}
	*rotation = fmod(angle + 4 * M_PI, 2 * M_PI);
	*scale = best_fit.pixel;
	if (parity != NULL) {
		*parity = mirrored;
	}
	// 6. RA/Dec of the image centre: Δx = Δy = 0 maps to (a0, b0) on the tangent plane
	indigo_tangent_to_radec(ref_ra, ref_dec, best_fit.a0, best_fit.b0, ra_center, dec_center);
	indigo_debug("%s: RA = %.3f°, DEC = %.3f°, rotation = %.2f°, scale = %.02f\"/pix, %d of %d matches, rms = %.2f\"", __FUNCTION__, *ra_center * RAD_TO_DEG, *dec_center * RAD_TO_DEG, *rotation * RAD_TO_DEG, *scale / ARCSEC_TO_RAD, best_n, match_count, best_fit.rms / ARCSEC_TO_RAD);
	return true;
}
