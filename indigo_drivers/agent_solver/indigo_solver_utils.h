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
 \file indigo_solver_utils.h
 */

#ifndef indigo_solver_utils_h
#define indigo_solver_utils_h

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define INDIGO_SOLVER_DEFAULT_INDEX_FOV	5.0		// default index FOV in degrees, used when the indexer gets no argument

#define DEG_TO_RAD				(M_PI / 180.0)
#define RAD_TO_DEG				(180.0 / M_PI)
#define HOUR_TO_RAD				(M_PI / 12.0)
#define RAD_TO_HOUR				(12.0 / M_PI)
#define HOUR_TO_DEG				(360.0 / 24.0)
#define DEG_TO_HOUR				(24.0 / 360.0)
#define ARCSEC_TO_RAD			(M_PI / (180.0 * 3600.0))
#define MAS_TO_RAD				(M_PI / (180.0 * 3600.0 * 1000.0))

// Quad code coordinates are quantised to 1/INDIGO_SOLVER_CODE_QUANTUM of the baseline.
#define INDIGO_SOLVER_CODE_QUANTUM				1000
// A canonical ordering decision closer than this margin is ambiguous: the indexer stores every variant.
#define INDIGO_SOLVER_CODE_AMBIGUITY			0.005f
#define INDIGO_SOLVER_MAX_QUAD_VARIANTS		8
// Catalog codes within +- this many quanta of the image code (in every component) are matched.
#define INDIGO_SOLVER_HASH_TOLERANCE			1
// Relative tolerance of the image/catalog baseline ratio around the best voted scale.
#define INDIGO_SOLVER_SCALE_TOLERANCE			0.01f

typedef struct indigo_solver_star {
	uint32_t id;
	float ra;
	float dec;
	float magnitude;
	float x;
	float y;
	uint16_t used;
	uint64_t ipix;
} indigo_solver_star;

typedef struct {
	uint64_t hash;
	float baseline;
	float x3, y3, x4, y4;
	indigo_solver_star *stars[4];
	uint16_t used;
} indigo_solver_quad;

// A catalog star, the star record of the index files
typedef struct {
	float ra;
	float dec;
} indigo_solver_index_star;

// A catalog quad is identified by its position in the catalog arrays; a match keeps its four stars
typedef struct {
	indigo_solver_quad *image_quad;
	const indigo_solver_index_star *catalog_stars[4];
	float scale;
	int votes;
} indigo_solver_quad_match;

typedef struct {
	indigo_solver_star *image_star;
	const indigo_solver_index_star *catalog_star;
	int votes;
} indigo_solver_star_match;

// Catalog quads are bucketed by the quantised (x3, y3) code coordinates, both in [0, INDIGO_SOLVER_CODE_QUANTUM]
#define INDIGO_SOLVER_BUCKET_SIDE					(INDIGO_SOLVER_CODE_QUANTUM + 1)
#define INDIGO_SOLVER_BUCKET_COUNT				(INDIGO_SOLVER_BUCKET_SIDE * INDIGO_SOLVER_BUCKET_SIDE)

// Index file version 2: little-endian, the layout the solver uses in memory, so the file is mapped and used as it
// is. A 64 byte header (magic, version, scale, star and quad counts, the bucket side and the offsets of the sections,
// each 8 byte aligned) is followed by the stars, the INDIGO_SOLVER_BUCKET_COUNT + 1 bucket starts, the quad hashes in
// ascending order, their baselines and their star indices, four per quad. Read by indigo_solver_load_index().

#define INDIGO_SOLVER_INDEX_MAGIC					"ISIX"
#define INDIGO_SOLVER_INDEX_VERSION				2
#define INDIGO_SOLVER_INDEX_HEADER_SIZE		64

// The leading fields of the header
typedef struct {
	char magic[4];
	uint32_t version;
	float fov;
	uint32_t star_count;
	uint32_t quad_count;
} indigo_solve_index_header;

// directory NULL or empty means the current working directory
// The catalogue an index of the given scale is built from: scales below 1 degree come from Gaia DR3, the others
// from Tycho-2; it is the file name prefix (gaia-0.5deg.isix, tyc2-1deg.isix) and the item name prefix of the agent
extern const char *indigo_solver_index_catalog(float fov);
extern void indigo_solver_index_path(char *path, size_t size, const char *directory, float fov);
extern bool indigo_solver_index_read_header(FILE *file, indigo_solve_index_header *header);

extern uint64_t indigo_solver_quad_hash(int hx3, int hy3, int hx4, int hy4);
extern void indigo_solver_quad_hash_components(uint64_t hash, int *hx3, int *hy3, int *hx4, int *hy4);
extern void indigo_solver_make_quad(indigo_solver_quad *quad, indigo_solver_star *star_0, indigo_solver_star *star_1, indigo_solver_star *star_2, indigo_solver_star *star_3);
extern int indigo_solver_make_quad_variants(indigo_solver_quad *quads, int max_count, indigo_solver_star *star_0, indigo_solver_star *star_1, indigo_solver_star *star_2, indigo_solver_star *star_3);

extern float indigo_angular_distance(float ra1, float dec1, float ra2, float dec2);
extern bool indigo_radec_to_tangent(float ra0, float dec0, float ra, float dec, float *xi, float *eta);
extern void indigo_tangent_to_radec(float ra0, float dec0, float xi, float eta, float *ra, float *dec);
extern bool indigo_radec_to_xy(float ra0, float dec0, float angle, float scale, int width, int height, int parity, float ra, float dec, float *x, float *y);

#endif
