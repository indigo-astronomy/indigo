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
 \file indigo_solver.h
 */

#ifndef indigo_solver_h
#define indigo_solver_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigocat/indigocat_star.h>

#include "indigo_solver_utils.h"

#define INDIGO_SOLVER_INDEX_FOV_COUNT	6

/** The index fitting a field best has the quad scale of about this fraction of the geometric mean of the field size:
 on frames of 0.4° to 7° the indexes of 0.37 to 1.0 x the field solved, the ones above 1.2 x or below 0.35 x did not.
 */
#define INDIGO_SOLVER_INDEX_FIELD_RATIO	0.7

/** Quad scales of the available indexes in degrees, ascending; an index of scale n° holds quads of 0.1 n° to 0.8 n°,
 not a field of view.
 */
extern const float indigo_solver_index_fovs[INDIGO_SOLVER_INDEX_FOV_COUNT];

/** Any supported input image as a newly allocated INDIGO RAW MONO16 image: INDIGO RAW (MONO16 as it is, MONO8 spread
 over 16 bits, RGB24/RGB48 averaged to mono), FITS (BITPIX 8, 16, 32, -32, -64 with BZERO/BSCALE, NAXIS 2 or 3 with the
 planes weighted), JPEG (components weighted) and camera raw files read by libraw; colour is reduced to mono with the
 luminance weights 0.299 R + 0.587 G + 0.114 B. bayered tells whether a RAW extension or a FITS card carries BAYERPAT,
 so the caller bins the mosaic. NULL with the reason in message.
 */
extern indigo_raw_header *indigo_solver_load_image(const void *data, long size, bool *bayered, const char **message);
/** Averages bin x bin pixels in place; only INDIGO_RAW_MONO16 images are binned, others are left untouched.
 */
extern void indigo_solver_bin_image(indigo_raw_header *image, int bin);
/** Detects stars in the image, returns their count. Mono frames are detected on a filtered copy: hot pixels,
 columns and rows replaced by their surroundings and a gaussian blur matched to the star images applied, so faint
 stars stand out of the pixel noise; other frame types are detected as they are.
 */
extern int indigo_solver_image_stars(indigo_raw_header *image, indigo_solver_star *stars, int max_star_count);
/** Picks the index for the expected image size on the sky (radians) among the given FOVs: the one nearest, on the log
 scale, to INDIGO_SOLVER_INDEX_FIELD_RATIO x the geometric mean of the field size. Only existing files in
 index_directory (NULL = current working directory) are considered.
 */
extern bool indigo_solver_select_index(const char *index_directory, const float *fovs, int fov_count, float field_width, float field_height, char *path, size_t size, float *fov);
/** Catalog stars and quads in the layout of the index file: the quads, sorted by hash, are the entries i of
 hashes, baselines and quad_stars (four star indices from 4 * i); buckets[k] .. buckets[k + 1] is the range of the
 quads in the (x3, y3) bucket k of their codes. The arrays point into the mapped index file, or to memory the
 catalog owns (a built or a filtered catalog); a filtered catalog shares the stars.
 */
typedef struct {
	float fov;
	uint32_t star_count;
	uint32_t quad_count;
	const indigo_solver_index_star *stars;
	const uint32_t *buckets;
	const uint64_t *hashes;
	const float *baselines;
	const uint32_t *quad_stars;
	const void *mapping;
	size_t mapping_size;
	void *allocated[5];
} indigo_solver_catalog;

/** Maps an index file, see indigo_solver_free_catalog().
 */
extern bool indigo_solver_load_index(const char *path, indigo_solver_catalog *catalog);
/** Selects the index among all known FOVs and loads it.
 */
extern bool indigo_solver_catalog_load(const char *index_directory, float field_width, float field_height, indigo_solver_catalog *catalog);
/** Builds a catalog of the stars and of the quads, whose star pointers point into stars; used by the indexer.
 */
extern bool indigo_solver_catalog_build(const indigo_solver_star *stars, int star_count, const indigo_solver_quad *quads, int quad_count, float fov, indigo_solver_catalog *catalog);
/** Writes the catalog as an index file.
 */
extern bool indigo_solver_write_index(const char *path, const indigo_solver_catalog *catalog);
/** Unmaps or frees the catalog and clears it; a cleared catalog may be freed again.
 */
extern void indigo_solver_free_catalog(indigo_solver_catalog *catalog);
extern int indigo_solver_build_image_quads(indigo_solver_star *stars, int star_count, int width, int height, int cell_size, int *used_stars, indigo_solver_quad *quads, int max_quad_count);
/** Bounds of a matching run: a monotonic deadline (0 for none) and an abort flag (NULL for none). The matching
 functions check them every few thousand iterations and return 0 with exceeded set and reason naming the limit.
 */
typedef struct {
	double deadline;
	volatile bool *abort;
	bool exceeded;
	const char *reason;
} indigo_solver_limits;

/** The catalog of the quads whose four stars lie within radius (radians) of ra/dec (radians), sharing the stars of
 catalog; returns their count, the caller frees filtered with indigo_solver_free_catalog().
 */
extern int indigo_solver_filter_quads(const indigo_solver_catalog *catalog, float ra, float dec, float radius, indigo_solver_catalog *filtered);
extern int indigo_solver_match_quads(indigo_solver_quad *image_quads, int image_quad_count, const indigo_solver_catalog *catalog, indigo_solver_quad_match *matches, int max_match_count, indigo_solver_limits *limits);
extern int indigo_solver_match_stars(indigo_solver_quad_match *quad_matches, int quad_match_count, indigo_solver_star_match *star_matches, int max_star_match_count, indigo_solver_limits *limits);
/** Fits the image centre; ra, dec, rotation in radians, scale in radians per pixel, parity 1 for a normal
 (north up, east left) image orientation and -1 for a mirrored one; matched is the number of consistent matches of
 the final fit and rms its residual in radians (both optional).
 */
extern bool indigo_solver_compute_center(indigo_solver_star_match *matches, int match_count, int width, int height, float *ra, float *dec, float *rotation, float *scale, int *parity, int *matched, float *rms);

#endif
