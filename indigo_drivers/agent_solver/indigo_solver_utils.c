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

/** INDIGO quad based plate solver - geometry, quad codes and index file format
 \file indigo_solver_utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <indigo/indigo_bus.h>

#include "indigo_solver_utils.h"

// -------------------------------------------------------------------------------- Spherical geometry

// Haversine form: acos() of the dot product loses every arcsecond below a few arcminutes in float.
float indigo_angular_distance(float ra1, float dec1, float ra2, float dec2) {
	double sdd = sin((dec2 - dec1) / 2.0);
	double sdr = sin((ra2 - ra1) / 2.0);
	double a = sdd * sdd + cos(dec1) * cos(dec2) * sdr * sdr;
	if (a < 0) {
		a = 0;
	}
	if (a > 1) {
		a = 1;
	}
	return 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

// Gnomonic projection onto the tangent plane at (ra0, dec0).
// xi = "east" (increasing RA locally), eta = "north" (increasing Dec), both in radians.
bool indigo_radec_to_tangent(float ra0, float dec0, float ra, float dec, float *xi, float *eta) {
	float dra = ra - ra0;
	while (dra >  M_PI) {
		dra -= 2.0 * M_PI;
	}
	while (dra < -M_PI) {
		dra += 2.0 * M_PI;
	}
	float sdec  = sin(dec),  cdec  = cos(dec);
	float sdec0 = sin(dec0), cdec0 = cos(dec0);
	float cdra  = cos(dra),  sdra  = sin(dra);
	float cosc = sdec0 * sdec + cdec0 * cdec * cdra;
	if (cosc <= 0.0) {
		return false;
	}
	*xi  = (cdec * sdra) / cosc;
	*eta = (cdec0 * sdec - sdec0 * cdec * cdra) / cosc;
	return isfinite(*xi) && isfinite(*eta);
}

// Inverse gnomonic projection.
void indigo_tangent_to_radec(float ra0, float dec0, float xi, float eta, float *ra, float *dec) {
	float sdec0 = sin(dec0), cdec0 = cos(dec0);
	float denom = cdec0 - eta * sdec0;
	float r = ra0 + atan2(xi, denom);
	while (r < 0) {
		r += 2.0 * M_PI;
	}
	while (r >= 2.0 * M_PI) {
		r -= 2.0 * M_PI;
	}
	*ra = r;
	*dec = atan2(sdec0 + eta * cdec0, sqrt(xi * xi + denom * denom));
}

bool indigo_radec_to_xy(float ra0, float dec0, float angle, float scale, int width, int height, int parity, float ra, float dec, float *x, float *y) {
	float xi, eta;
	if (!indigo_radec_to_tangent(ra0, dec0, ra, dec, &xi, &eta)) {
		return false;
	}
	// The angle and the parity are those of indigo_solver_compute_center() and of the WCS of the astrometry agent
	// (astrometry.net orientation): negative parity is the sky as seen (east to the left with north up), positive
	// parity its mirror image; the angle of the sky as seen is counted in the opposite sense from 180 degrees.
	if (parity < 0) {
		xi = -xi;
		angle = (float)M_PI - angle;
	} else {
		angle -= M_PI;
	}
	// Rotate from (xi, eta) to image axes.
	// rot is CCW: (x_img_axis, y_img_axis) relative to (east,north)
	float cr = cos(angle), sr = sin(angle);
	float u =  xi * cr - eta * sr;
	float v =  xi * sr + eta * cr;
	// Convert radians on tangent plane to pixels.
	// Image pixel coordinates: origin at top-left, x right, y down.
	float cx = 0.5 * (float)width;
	float cy = 0.5 * (float)height;
	*x = cx + (u / scale);
	*y = cy - (v / scale);  // minus because +v (north) should go up (smaller y)
	if (!isfinite(*x) || !isfinite(*y)) {
		return false;
	}
	return true;
}

// -------------------------------------------------------------------------------- Quad codes

static inline int clamp_int(int v, int lo, int hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

// Code coordinates: P1 = (0,0), P2 = (1,0); P3 and P4 lie in the lens of the circles of radius 1 around P1 and P2,
// so x3, x4 in [0,1], y3 in [0, 0.87] after the parity fix and y4 in [-0.87, 0.87]. The hash is the lexicographic
// (x3, y3, x4, y4) key of the quantised coordinates, which also makes a hash-sorted index sorted by (x3, y3).
uint64_t indigo_solver_quad_hash(int hx3, int hy3, int hx4, int hy4) {
	hx3 = clamp_int(hx3, 0, INDIGO_SOLVER_CODE_QUANTUM);
	hy3 = clamp_int(hy3, 0, INDIGO_SOLVER_CODE_QUANTUM);
	hx4 = clamp_int(hx4, 0, INDIGO_SOLVER_CODE_QUANTUM);
	hy4 = clamp_int(hy4, -INDIGO_SOLVER_CODE_QUANTUM, INDIGO_SOLVER_CODE_QUANTUM);
	return ((uint64_t)(uint16_t)hx3 << 48) | ((uint64_t)(uint16_t)hy3 << 32) | ((uint64_t)(uint16_t)hx4 << 16) | (uint64_t)(uint16_t)hy4;
}

void indigo_solver_quad_hash_components(uint64_t hash, int *hx3, int *hy3, int *hx4, int *hy4) {
	*hx3 = (int16_t)(uint16_t)((hash >> 48) & 0xFFFF);
	*hy3 = (int16_t)(uint16_t)((hash >> 32) & 0xFFFF);
	*hx4 = (int16_t)(uint16_t)((hash >> 16) & 0xFFFF);
	*hy4 = (int16_t)(uint16_t)(hash & 0xFFFF);
}

static inline float dist_star_sqr(indigo_solver_star *star_0, indigo_solver_star *star_1) {
	float dx = star_0->x - star_1->x;
	float dy = star_0->y - star_1->y;
	return dx * dx + dy * dy;
}

static inline bool lex_less(float ax, float ay, float bx, float by) {
	const float eps = 1e-12;
	if (fabs(ax - bx) > eps) {
		return ax < bx;
	}
	return ay < by - eps;
}

typedef struct {
	float ax, ay, bx, by;
	float baseline;
} projection;

// Project a and b into the frame with origin at p1, +X along p1->p2 and unit length = |p1 p2|.
static void project_pair(indigo_solver_star *p1, indigo_solver_star *p2, indigo_solver_star *a, indigo_solver_star *b, projection *p) {
	const float ox = p1->x, oy = p1->y;
	const float vx = p2->x - ox, vy = p2->y - oy;
	const float inv = 1.0 / sqrt(vx * vx + vy * vy);
	const float exx = vx * inv, exy = vy * inv;
	const float eyx = -exy,     eyy =  exx;
	p->ax = ((a->x - ox) * exx + (a->y - oy) * exy) * inv;
	p->ay = ((a->x - ox) * eyx + (a->y - oy) * eyy) * inv;
	p->bx = ((b->x - ox) * exx + (b->y - oy) * exy) * inv;
	p->by = ((b->x - ox) * eyx + (b->y - oy) * eyy) * inv;
	p->baseline = 1.0 / inv;
}

static void build_code(indigo_solver_quad *quad, indigo_solver_star *p1, indigo_solver_star *p2, indigo_solver_star *a, indigo_solver_star *b, const projection *p, bool swap, bool flip) {
	quad->stars[0] = p1;
	quad->stars[1] = p2;
	if (swap) {
		quad->stars[2] = b;
		quad->stars[3] = a;
		quad->x3 = p->bx; quad->y3 = p->by;
		quad->x4 = p->ax; quad->y4 = p->ay;
	} else {
		quad->stars[2] = a;
		quad->stars[3] = b;
		quad->x3 = p->ax; quad->y3 = p->ay;
		quad->x4 = p->bx; quad->y4 = p->by;
	}
	if (flip) {
		quad->y3 = -quad->y3;
		quad->y4 = -quad->y4;
	}
	quad->baseline = p->baseline;
	quad->used = 0;
	quad->hash = indigo_solver_quad_hash(rint(quad->x3 * INDIGO_SOLVER_CODE_QUANTUM), rint(quad->y3 * INDIGO_SOLVER_CODE_QUANTUM), rint(quad->x4 * INDIGO_SOLVER_CODE_QUANTUM), rint(quad->y4 * INDIGO_SOLVER_CODE_QUANTUM));
}

// P3 is the lexicographically smaller of the two projected points.
static inline bool canonical_swap(const projection *p) {
	return lex_less(p->bx, p->by, p->ax, p->ay);
}

static inline bool ambiguous_swap(const projection *p) {
	return fabs(p->ax - p->bx) < INDIGO_SOLVER_CODE_AMBIGUITY;
}

// Mirror so that y3 >= 0 (or y4 >= 0 when y3 == 0).
static inline bool canonical_flip(const projection *p, bool swap) {
	const float eps = 1e-12;
	float y3 = swap ? p->by : p->ay;
	float y4 = swap ? p->ay : p->by;
	return y3 < -eps || (fabs(y3) <= eps && y4 < -eps);
}

static inline bool ambiguous_flip(const projection *p, bool swap) {
	float y3 = swap ? p->by : p->ay;
	return fabs(y3) < INDIGO_SOLVER_CODE_AMBIGUITY;
}

static inline bool code_less(const indigo_solver_quad *a, const indigo_solver_quad *b) {
	if (lex_less(a->x3, a->y3, b->x3, b->y3)) {
		return true;
	}
	if (lex_less(b->x3, b->y3, a->x3, a->y3)) {
		return false;
	}
	return lex_less(a->x4, a->y4, b->x4, b->y4);
}

// Builds the canonical code of the quad into quads[0]. When a canonical ordering decision (baseline direction,
// P3/P4 order or parity) is closer than INDIGO_SOLVER_CODE_AMBIGUITY, measurement noise can flip it, so the
// alternative codes are appended as well (up to max_count, duplicates removed). Returns the number of codes built.
int indigo_solver_make_quad_variants(indigo_solver_quad *quads, int max_count, indigo_solver_star *star_0, indigo_solver_star *star_1, indigo_solver_star *star_2, indigo_solver_star *star_3) {
	indigo_solver_star *stars[4] = { star_0, star_1, star_2, star_3 };
	// 1) Find farthest pair among the 4
	float best = -1.0;
	int bi = 0, bj = 1;
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			float d2 = dist_star_sqr(stars[i], stars[j]);
			if (d2 > best) {
				best = d2;
				bi = i;
				bj = j;
			}
		}
	}
	indigo_solver_star *A = stars[bi];
	indigo_solver_star *B = stars[bj];
	indigo_solver_star *R[2];
	int r = 0;
	for (int i = 0; i < 4; i++) {
		if (stars[i] != A && stars[i] != B) {
			R[r++] = stars[i];
		}
	}
	// 2) Canonical code for both baseline directions
	projection proj[2];
	project_pair(A, B, R[0], R[1], proj + 0);
	project_pair(B, A, R[0], R[1], proj + 1);
	indigo_solver_quad canon[2];
	for (int d = 0; d < 2; d++) {
		bool swap = canonical_swap(proj + d);
		build_code(canon + d, d ? B : A, d ? A : B, R[0], R[1], proj + d, swap, canonical_flip(proj + d, swap));
	}
	int first = code_less(canon + 1, canon + 0) ? 1 : 0;
	bool amb_dir = fabs(canon[0].x3 - canon[1].x3) < INDIGO_SOLVER_CODE_AMBIGUITY;
	// 3) Enumerate variants, canonical decisions first
	int count = 0;
	for (int k = 0; k < 2; k++) {
		if (k == 1 && !amb_dir) {
			break;
		}
		int d = k == 0 ? first : 1 - first;
		indigo_solver_star *p1 = d ? B : A, *p2 = d ? A : B;
		bool sc = canonical_swap(proj + d);
		for (int s = 0; s < 2; s++) {
			if (s == 1 && !ambiguous_swap(proj + d)) {
				break;
			}
			bool swap = s == 0 ? sc : !sc;
			bool fc = canonical_flip(proj + d, swap);
			for (int f = 0; f < 2; f++) {
				if (f == 1 && !ambiguous_flip(proj + d, swap)) {
					break;
				}
				bool flip = f == 0 ? fc : !fc;
				if (count == max_count) {
					return count;
				}
				indigo_solver_quad q;
				build_code(&q, p1, p2, R[0], R[1], proj + d, swap, flip);
				bool duplicate = false;
				for (int i = 0; i < count; i++) {
					if (quads[i].hash == q.hash) {
						duplicate = true;
						break;
					}
				}
				if (!duplicate) {
					quads[count++] = q;
				}
			}
		}
	}
	return count;
}

void indigo_solver_make_quad(indigo_solver_quad *quad, indigo_solver_star *star_0, indigo_solver_star *star_1, indigo_solver_star *star_2, indigo_solver_star *star_3) {
	indigo_solver_make_quad_variants(quad, 1, star_0, star_1, star_2, star_3);
}

// -------------------------------------------------------------------------------- Index file I/O (little-endian)

static inline uint32_t get_u32(const uint8_t *p) {
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static inline float get_f32(const uint8_t *p) {
	uint32_t u = get_u32(p);
	float f;
	memcpy(&f, &u, 4);
	return f;
}

const char *indigo_solver_index_catalog(float fov) {
	return fov < 1 ? "gaia" : "tyc2";
}

void indigo_solver_index_path(char *path, size_t size, const char *directory, float fov) {
	const char *catalog = indigo_solver_index_catalog(fov);
	if (directory == NULL || directory[0] == 0) {
		snprintf(path, size, "%s-%gdeg.isix", catalog, fov);
	} else {
		size_t length = strlen(directory);
		snprintf(path, size, "%s%s%s-%gdeg.isix", directory, directory[length - 1] == '/' ? "" : "/", catalog, fov);
	}
}

bool indigo_solver_index_read_header(FILE *file, indigo_solve_index_header *header) {
	uint8_t buf[INDIGO_SOLVER_INDEX_HEADER_SIZE];
	if (fread(buf, INDIGO_SOLVER_INDEX_HEADER_SIZE, 1, file) != 1) {
		fprintf(stderr, "%s: short read\n", __FUNCTION__);
		return false;
	}
	memcpy(header->magic, buf, 4);
	header->version = get_u32(buf + 4);
	header->fov = get_f32(buf + 8);
	header->star_count = get_u32(buf + 12);
	header->quad_count = get_u32(buf + 16);
	if (memcmp(header->magic, INDIGO_SOLVER_INDEX_MAGIC, 4) != 0) {
		fprintf(stderr, "%s: bad magic\n", __FUNCTION__);
		return false;
	}
	if (header->version != INDIGO_SOLVER_INDEX_VERSION) {
		fprintf(stderr, "%s: unsupported index version %u (expected %u)\n", __FUNCTION__, header->version, INDIGO_SOLVER_INDEX_VERSION);
		return false;
	}
	return true;
}
