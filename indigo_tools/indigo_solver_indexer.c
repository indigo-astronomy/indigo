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

/** INDIGO quad based plate solver - Tycho-2 index builder
 \file indigo_solver_indexer.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include <indigo/indigo_bus.h>

#include <agent_solver/indigo_solver_utils.h>
#include <agent_solver/indigo_solver_healpix.h>

#define MAX_MAGNITUDE			11.5

#define PATH_TEMPLATE			"I_259/tyc2.dat.%02d"
#define FILE_COUNT				20
#define MAX_STARS					2600000		// Tycho-2 has 2 539 913 records
#define MIN_LINE_LENGTH		130				// VT magnitude ends at byte 129

#define J2000_UNIX				946728000.0	// 2000-01-01 12:00 UTC
#define JULIAN_YEAR				(365.25 * 86400.0)

// Byte offsets (0-based) of the Tycho-2 fields used, see I_259/ReadMe
#define COL_TYC3					11
#define COL_PFLAG					13
#define COL_RA						15
#define COL_DEC						28
#define COL_PMRA					41
#define COL_PMDE					49
#define COL_VT						123

typedef struct {
	int size;
	int count;
	indigo_solver_star **stars;
} bucket;

typedef struct {
	float fov;
	float d_min;
	float d_max;
	float star_separation;
	float star_reuse;
	uint32_t nside;
	uint32_t stars_per_cell;
} index_params;

static int compare_star_pointers_by_magnitude(const void *a, const void *b) {
	const indigo_solver_star *ma = *(const indigo_solver_star**)a;
	const indigo_solver_star *mb = *(const indigo_solver_star**)b;
	return (ma->magnitude > mb->magnitude) - (ma->magnitude < mb->magnitude);
}

static int compare_quads_by_hash(const void *a, const void *b) {
	const indigo_solver_quad *ma = (const indigo_solver_quad*)a;
	const indigo_solver_quad *mb = (const indigo_solver_quad*)b;
	return (ma->hash > mb->hash) - (ma->hash < mb->hash);
}

// Loads Tycho-2 stars brighter than MAX_MAGNITUDE (VT) with proper motion applied to the current epoch.
static uint32_t parse_catalog(indigo_solver_star *stars) {
	char path[128];
	char *line = NULL;
	size_t size = 0;
	uint32_t total_star_count = 0, star_count = 0, no_vt_count = 0;
	double years = (time(NULL) - J2000_UNIX) / JULIAN_YEAR;
	printf("Loading stars, proper motion applied for J%.2f...\n", 2000.0 + years);
	for (int i = 0; i < FILE_COUNT; i++) {
		snprintf(path, sizeof(path), PATH_TEMPLATE, i);
		FILE *file = fopen(path, "r");
		if (file == NULL) {
			fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
			exit(1);
		}
		while (getline(&line, &size, file) != -1) {
			if (strlen(line) < MIN_LINE_LENGTH) {
				continue;
			}
			if (line[COL_PFLAG] == 'X' || line[COL_TYC3] != '1') {
				continue;
			}
			total_star_count++;
			float magnitude = atof(line + COL_VT);
			if (magnitude == 0) {
				no_vt_count++;
				continue;
			}
			if (magnitude >= MAX_MAGNITUDE) {
				continue;
			}
			if (star_count == MAX_STARS) {
				fprintf(stderr, "More than %d stars, increase MAX_STARS\n", MAX_STARS);
				exit(1);
			}
			indigo_solver_star *s = stars + star_count;
			double ra = atof(line + COL_RA) * DEG_TO_RAD;
			double dec = atof(line + COL_DEC) * DEG_TO_RAD;
			double pm_ra = atof(line + COL_PMRA) * MAS_TO_RAD * years;		// along RA, already multiplied by cos(dec)
			double pm_dec = atof(line + COL_PMDE) * MAS_TO_RAD * years;
			double cos_dec = cos(dec);
			if (fabs(cos_dec) > 1e-6) {
				ra += pm_ra / cos_dec;
			}
			dec += pm_dec;
			while (ra < 0) {
				ra += 2 * M_PI;
			}
			while (ra >= 2 * M_PI) {
				ra -= 2 * M_PI;
			}
			memset(s, 0, sizeof(indigo_solver_star));
			s->ra = ra;
			s->dec = dec;
			s->magnitude = magnitude;
			star_count++;
		}
		fclose(file);
	}
	indigo_safe_free(line);
	printf("Loaded %u out of %u stars (%u without VT skipped)\n\n", star_count, total_star_count, no_vt_count);
	return star_count;
}

static bucket *fill_buckets(uint32_t nside, indigo_solver_star *stars, uint32_t star_count) {
	uint32_t npix = 12 * nside * nside;
	bucket *buckets = indigo_safe_malloc(sizeof(bucket) * npix);
	memset(buckets, 0, sizeof(bucket) * npix);
	for (uint32_t i = 0; i < star_count; i++) {
		indigo_solver_star *s = stars + i;
		s->id = 0;
		s->used = 0;
		uint32_t ipix = indigo_healpix_ang2pix_ring(nside, s->ra, s->dec);
		s->ipix = ipix;
		bucket *b = buckets + ipix;
		if (b->size == 0) {
			b->stars = indigo_safe_malloc((b->size = 128) * sizeof(indigo_solver_star *));
		} else if (b->size == b->count) {
			b->stars = indigo_safe_realloc(b->stars, (b->size *= 2) * sizeof(indigo_solver_star *));
		}
		b->stars[b->count++] = s;
	}
	return buckets;
}

static void free_buckets(bucket *buckets, uint32_t npix) {
	for (uint32_t i = 0; i < npix; i++) {
		indigo_safe_free(buckets[i].stars);
	}
	indigo_safe_free(buckets);
}

static inline bool distance_check(const index_params *p, indigo_solver_star *star1, indigo_solver_star *star2) {
	float distance = indigo_angular_distance(star1->ra, star1->dec, star2->ra, star2->dec);
	return distance >= p->d_min && distance <= p->d_max;
}

static uint32_t build_quads(const index_params *p, indigo_solver_star *stars, uint32_t star_count, bucket *buckets, indigo_solver_quad **quads_out) {
	uint32_t nside = p->nside;
	uint32_t npix = 12 * nside * nside;
	uint32_t quad_count = 0, variant_count = 0;
	int32_t neighbours[8];
	uint32_t quads_size = 1024;
	for (uint32_t ipix = 0; ipix < npix; ipix++) {
		bucket *b = buckets + ipix;
		qsort(b->stars, b->count, sizeof(indigo_solver_star *), compare_star_pointers_by_magnitude);
		int b_count = 0;
		for (int i = 0; i < b->count; i++) {
			indigo_solver_star *si = b->stars[i];
			for (int j = 0; j < b_count; j++) {
				indigo_solver_star *sj = b->stars[j];
				if (indigo_angular_distance(si->ra, si->dec, sj->ra, sj->dec) < p->star_separation) {
					goto skip;
				}
			}
			b->stars[b_count++] = si;
		skip:
			;
		}
		b->count = b_count;
		if (b->count > p->stars_per_cell) {
			b->count = p->stars_per_cell;
		}
	}
	indigo_solver_quad *quads = indigo_safe_malloc(sizeof(indigo_solver_quad) * quads_size);
	indigo_solver_star **used_stars = indigo_safe_malloc(star_count * sizeof(indigo_solver_star *));
	printf("Building quads...\n");
	for (uint32_t ipix = 0; ipix < npix; ipix++) {
		bucket *b = buckets + ipix;
		if (ipix > 0 && ipix % 1000 == 0) {
			printf("ipix = %u/%u -> %u quads\n", ipix, npix, quad_count);
		}
		float ra0, dec0;
		indigo_healpix_pix2ang_ring(nside, ipix, &ra0, &dec0);
		// create list of stars in current and neighbour healpix cells, sort by magnitude
		indigo_healpix_neighbours_ring(nside, ipix, neighbours);
		memcpy(used_stars, b->stars, b->count * sizeof(indigo_solver_star *));
		int used_star_count = b->count;
		for (int j = 0; j < 8; j++) {
			int32_t k = neighbours[j];
			if (k != -1) {
				bucket *nb = buckets + k;
				memcpy(used_stars + used_star_count, nb->stars, nb->count * sizeof(indigo_solver_star *));
				used_star_count += nb->count;
			}
		}
		qsort(used_stars, used_star_count, sizeof(indigo_solver_star *), compare_star_pointers_by_magnitude);
		for (int i = 0; i < used_star_count; i++) {
			used_stars[i]->used = 0;
		}
		for (int i = 0; i < used_star_count; i++) {
			indigo_solver_star *si = used_stars[i];
			if (si->used > p->star_reuse) {
				continue;
			}
			indigo_radec_to_xy(ra0, dec0, 0, 16 * M_PI / 648000, 1000, 1000, true, si->ra, si->dec, &si->x, &si->y);
			for (int j = i + 1; j < used_star_count; j++) {
				indigo_solver_star *sj = used_stars[j];
				if (sj->used > p->star_reuse || !distance_check(p, sj, si)) {
					continue;
				}
				indigo_radec_to_xy(ra0, dec0, 0, 16 * M_PI / 648000, 1000, 1000, true, sj->ra, sj->dec, &sj->x, &sj->y);
				for (int k = j + 1; k < used_star_count; k++) {
					indigo_solver_star *sk = used_stars[k];
					if (sk->used > p->star_reuse || !distance_check(p, sk, si) || !distance_check(p, sk, sj)) {
						continue;
					}
					indigo_radec_to_xy(ra0, dec0, 0, 16 * M_PI / 648000, 1000, 1000, true, sk->ra, sk->dec, &sk->x, &sk->y);
					for (int l = k + 1; l < used_star_count; l++) {
						indigo_solver_star *sl = used_stars[l];
						if (sl->used > p->star_reuse || !distance_check(p, sl, si) || !distance_check(p, sl, sj) || !distance_check(p, sl, sk)) {
							continue;
						}
						indigo_solver_star *first_one = si;
						if (first_one > sj) {
							first_one = sj;
						}
						if (first_one > sk) {
							first_one = sk;
						}
						if (first_one > sl) {
							first_one = sl;
						}
						if (first_one->ipix != ipix) { // force ownership rule
							continue;
						}
						indigo_radec_to_xy(ra0, dec0, 0, 16 * M_PI / 648000, 1000, 1000, true, sl->ra, sl->dec, &sl->x, &sl->y);
						if (quad_count + INDIGO_SOLVER_MAX_QUAD_VARIANTS > quads_size) {
							quads_size *= 2;
							quads = indigo_safe_realloc(quads, sizeof(indigo_solver_quad) * quads_size);
						}
						int n = indigo_solver_make_quad_variants(quads + quad_count, INDIGO_SOLVER_MAX_QUAD_VARIANTS, si, sj, sk, sl);
						quad_count += n;
						variant_count += n - 1;
						si->used++;
						sj->used++;
						sk->used++;
						sl->used++;
					}
				}
			}
		}
	}
	indigo_safe_free(used_stars);
	qsort(quads, quad_count, sizeof(indigo_solver_quad), compare_quads_by_hash);
	for (uint32_t i = 0; i < star_count; i++) {
		stars[i].used = 0;
	}
	for (uint32_t i = 0; i < quad_count; i++) {
		indigo_solver_quad *q = quads + i;
		q->stars[0]->used = 1;
		q->stars[1]->used = 1;
		q->stars[2]->used = 1;
		q->stars[3]->used = 1;
	}
	*quads_out = quads;
	printf("Built %u quads (%u extra variants of ambiguous codes), %.1f quads per cell\n\n", quad_count, variant_count, (double)quad_count / npix);
	return quad_count;
}

static bool write_index(float fov, indigo_solver_star *stars, uint32_t star_count, indigo_solver_quad *quads, uint32_t quad_count) {
	char path[128];
	indigo_solver_index_path(path, sizeof(path), NULL, fov);
	FILE *file = fopen(path, "wb");
	if (file == NULL) {
		fprintf(stderr, "Failed to create %s: %s\n", path, strerror(errno));
		return false;
	}
	indigo_solve_index_header h = {
		.magic = INDIGO_SOLVER_INDEX_MAGIC,
		.version = INDIGO_SOLVER_INDEX_VERSION,
		.fov = fov,
		.star_count = 0,
		.quad_count = quad_count
	};
	for (uint32_t i = 0; i < star_count; i++) {
		if (stars[i].used) {
			stars[i].id = h.star_count++;
		}
	}
	bool ok = indigo_solver_index_write_header(file, &h);
	indigo_solver_index_star *index_stars = indigo_safe_malloc(h.star_count * sizeof(indigo_solver_index_star));
	uint32_t count = 0;
	for (uint32_t i = 0; i < star_count; i++) {
		indigo_solver_star *s = stars + i;
		if (s->used) {
			index_stars[count].ra = s->ra;
			index_stars[count].dec = s->dec;
			count++;
		}
	}
	ok = ok && indigo_solver_index_write_stars(file, index_stars, h.star_count);
	indigo_safe_free(index_stars);
	indigo_solver_index_quad *index_quads = indigo_safe_malloc(quad_count * sizeof(indigo_solver_index_quad));
	for (uint32_t i = 0; i < quad_count; i++) {
		indigo_solver_quad *q = quads + i;
		index_quads[i].hash = q->hash;
		index_quads[i].baseline = q->baseline;
		for (int k = 0; k < 4; k++) {
			index_quads[i].stars[k] = q->stars[k]->id;
		}
	}
	ok = ok && indigo_solver_index_write_quads(file, index_quads, quad_count);
	indigo_safe_free(index_quads);
	if (fclose(file) != 0 || !ok) {
		fprintf(stderr, "Failed to write %s\n", path);
		return false;
	}
	printf("Saved %u stars and %u quads to %s\n\n", h.star_count, h.quad_count, path);
	return true;
}

static void select_params(float fov, index_params *p) {
	p->fov = fov;
	p->star_reuse = 512;
	if (fov <= 1.0) {
		p->d_min = 0.1 * DEG_TO_RAD;
		p->d_max = 0.8 * DEG_TO_RAD;
		p->nside = 64;
		p->stars_per_cell = 8;
	} else if (fov <= 2.0) {
		p->d_min = 0.2 * DEG_TO_RAD;
		p->d_max = 1.6 * DEG_TO_RAD;
		p->nside = 32;
		p->stars_per_cell = 10;
	} else if (fov <= 5) {
		p->d_min = 0.4 * DEG_TO_RAD;
		p->d_max = 3.2 * DEG_TO_RAD;
		p->nside = 16;
		p->stars_per_cell = 12;
	} else if (fov <= 10) {
		p->d_min = 0.8 * DEG_TO_RAD;
		p->d_max = 6.4 * DEG_TO_RAD;
		p->nside = 8;
		p->stars_per_cell = 16;
	} else {
		p->d_min = 1.6 * DEG_TO_RAD;
		p->d_max = 12.8 * DEG_TO_RAD;
		p->nside = 4;
		p->stars_per_cell = 20;
	}
	p->star_separation = p->d_min * 0.5;
}

static bool build_index(float fov, indigo_solver_star *stars, uint32_t star_count) {
	index_params p;
	select_params(fov, &p);
	uint32_t npix = 12 * p.nside * p.nside;
	printf("fov               = %g°\n", p.fov);
	printf("d_min             = %.1f°\n", p.d_min * RAD_TO_DEG);
	printf("d_max             = %.1f°\n", p.d_max * RAD_TO_DEG);
	printf("star_separation   = %.2f°\n", p.star_separation * RAD_TO_DEG);
	printf("stars_per_cell    = %u\n", p.stars_per_cell);
	printf("nside/npix        = %u/%u\n", p.nside, npix);
	printf("\n");
	bucket *buckets = fill_buckets(p.nside, stars, star_count);
	indigo_solver_quad *quads;
	uint32_t quad_count = build_quads(&p, stars, star_count, buckets, &quads);
	bool ok = write_index(fov, stars, star_count, quads, quad_count);
	indigo_safe_free(quads);
	free_buckets(buckets, npix);
	return ok;
}

int main(int argc, char **argv) {
	float fovs[16];
	int fov_count = 0;
	for (int i = 1; i < argc && fov_count < 16; i++) {
		float fov = atof(argv[i]);
		if (fov <= 0) {
			fprintf(stderr, "Usage: %s [fov_deg ...]   (default %g)\n", argv[0], INDIGO_SOLVER_DEFAULT_INDEX_FOV);
			return 1;
		}
		fovs[fov_count++] = fov;
	}
	if (fov_count == 0) {
		fovs[fov_count++] = INDIGO_SOLVER_DEFAULT_INDEX_FOV;
	}
	printf("INDIGO Solver - Tycho-2 index builder\n-------------------------------------\n");
	indigo_solver_star *stars = indigo_safe_malloc(MAX_STARS * sizeof(indigo_solver_star));
	uint32_t star_count = parse_catalog(stars);
	int failed = 0;
	for (int i = 0; i < fov_count; i++) {
		if (!build_index(fovs[i], stars, star_count)) {
			failed++;
		}
	}
	indigo_safe_free(stars);
	return failed ? 1 : 0;
}
