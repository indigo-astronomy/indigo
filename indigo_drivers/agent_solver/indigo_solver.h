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

#define INDIGO_SOLVER_INDEX_FOV_COUNT	5

/** Field of view of the available index scales in degrees, ascending.
 */
extern const float indigo_solver_index_fovs[INDIGO_SOLVER_INDEX_FOV_COUNT];

/** Averages bin x bin pixels in place; only INDIGO_RAW_MONO16 images are binned, others are left untouched.
 */
extern void indigo_solver_bin_image(indigo_raw_header *image, int bin);
/** Detects stars in the image, returns their count.
 */
extern int indigo_solver_image_stars(indigo_raw_header *image, indigo_solver_star *stars, int max_star_count);
/** Picks the index for the expected image size on the sky (radians) among the given FOVs: the largest FOV not above
 1.5 x the geometric mean of the field size, or the smallest available one. Only existing files in index_directory
 (NULL = current working directory) are considered.
 */
extern bool indigo_solver_select_index(const char *index_directory, const float *fovs, int fov_count, float field_width, float field_height, char *path, size_t size, float *fov);
/** Loads an index file; the arrays are allocated and belong to the caller, see indigo_solver_free_catalog().
 */
extern bool indigo_solver_load_index(const char *path, indigo_solver_star **stars, int *star_count, indigo_solver_quad **quads, int *quad_count);
/** Selects the index among all known FOVs and loads it.
 */
extern bool indigo_solver_catalog(const char *index_directory, float field_width, float field_height, indigo_solver_star **stars, int *star_count, indigo_solver_quad **quads, int *quad_count);
extern void indigo_solver_free_catalog(indigo_solver_star *stars, indigo_solver_quad *quads);
extern int indigo_solver_build_image_quads(indigo_solver_star *stars, int star_count, int width, int height, int cell_size, int *used_stars, indigo_solver_quad *quads, int max_quad_count);
extern int indigo_solver_match_quads(indigo_solver_quad *image_quads, int image_quad_count, indigo_solver_quad *catalog_quads, int catalog_quad_count, indigo_solver_quad_match *matches, int max_match_count);
extern int indigo_solver_match_stars(indigo_solver_quad_match *quad_matches, int quad_match_count, indigo_solver_star_match *star_matches, int max_star_match_count);
/** Fits the image centre; ra, dec, rotation in radians, scale in radians per pixel, parity 1 for a normal
 (north up, east left) image orientation and -1 for a mirrored one.
 */
extern bool indigo_solver_compute_center(indigo_solver_star_match *matches, int match_count, int width, int height, float *ra, float *dec, float *rotation, float *scale, int *parity);

#endif
