// Copyright (c) 2021 Rumen G.Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumen@skyarchive.org>

/** INDIGO Bus
 \file indigo_align.h
 */

#ifndef indigo_align_h
#define indigo_align_h

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

/** Cartesian point
 */
typedef struct {
	double x;
	double y;
	double z;
} indigo_cartesian_point_t;

/** Spherical point
 */
typedef struct {
	double a;   /* longitude, az, ra, ha in radians*/
	double d;   /* latitude, alt, dec in radians */
	double r;   /* radius (1 for celestial coordinates) */
} indigo_spherical_point_t;

/** convert ha dec to az alt in radians
 */
extern void indigo_equatorial_to_hotizontal(const indigo_spherical_point_t *eq_point, const double latitude, indigo_spherical_point_t *h_point);

/** convert spherical to cartesian coordinates
 */
extern indigo_cartesian_point_t indigo_spherical_to_cartesian(const indigo_spherical_point_t *spoint);

/** convert spherical (in radians) to cartesian coordinates
 */
extern indigo_spherical_point_t indigo_cartesian_to_sphercal(const indigo_cartesian_point_t *cpoint);

/** rotate cartesian coordinates around axes (angles in radians)
 */
extern indigo_cartesian_point_t indigo_cartesian_rotate_x(const indigo_cartesian_point_t *point, double angle);
extern indigo_cartesian_point_t indigo_cartesian_rotate_y(const indigo_cartesian_point_t *point, double angle);
extern indigo_cartesian_point_t indigo_cartesian_rotate_z(const indigo_cartesian_point_t *point, double angle);

/** rotate coordinates using polar errors
 * possition->a = Hour angle in radians
 * possition->d = Declination in radians
 * possition->r = 1; (should be 1)
 * u = angle in radians, rotate around Y (East - West) axis. Eastward is poritive.
 * v = angle in radians, rotate around X (North - South) axis, Southward is positive.
 */
extern indigo_spherical_point_t indigo_apply_polar_error(const indigo_spherical_point_t *position, double u, double v);
#define indigo_correct_polar_error(position, u, v) (indigo_apply_polar_error(position, -u, -v))

/** convert spherical point in radians to ha/ra dec in hours and degrees
 */
extern void indigo_point_to_ra_dec(const indigo_spherical_point_t *spoint, const double lst, double *ra, double *dec);

/** convert ha/ra dec in hours and degrees to spherical point in radians
 */
extern void indigo_ra_dec_to_point(const double ra, const double dec, const double lst, indigo_spherical_point_t *spoint);

/** great circle distances of sphericaal coordinate poits
 */
extern double indigo_gc_distance_spherical(const indigo_spherical_point_t *sp1, const indigo_spherical_point_t *sp2);

/** great circle distance of ra/ha dec in degrees
 */
extern double indigo_gc_distance(double ra1, double dec1, double ra2, double dec2);

/** great circle distance of points in cartesian coordinates
 */
extern double indigo_gc_distance_cartesian(const indigo_cartesian_point_t *cp1, const indigo_cartesian_point_t *cp2);

/** calculate refraction error from altitude
 */
extern double indigo_calculate_refraction(const double z);

/** compensate atmospheric refraction
 */
extern bool indigo_compensate_refraction(
	const indigo_spherical_point_t *st,
	const double latitude,
	indigo_spherical_point_t *st_corrected
);

/** compensate atmospheric refraction
 */
extern bool indigo_compensate_refraction2(
	const indigo_spherical_point_t *st,
	const double latitude,
	const double refraction,
	indigo_spherical_point_t *st_corrected
);

/** calculate polar alignment error
 */
extern bool indigo_polar_alignment_error(
	const indigo_spherical_point_t *st1,
	const indigo_spherical_point_t *st2,
	const indigo_spherical_point_t *st2_observed,
	const double latitude,
	const bool compensate_refraction,
	indigo_spherical_point_t *equatorial_error,
	indigo_spherical_point_t *horizontal_error
);

#ifdef __cplusplus
}
#endif

#endif /* indigo_align_h */
