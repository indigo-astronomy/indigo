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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

// The mathematics for the three point polar alignment were derived independently, based on the common principles of the spherical geometry.
// The algorythm and the source code were developed based on the math we derived.
// However the idea for the three point polar alignment was borrowed from N.I.N.A. and several mount hand controllers which provide such functionality.
// I had a discussion with Stefan Berg on the final touches and he was kind enough to highlight some "gotchas" he faced during the N.I.N.A. development.
// I also want to thank Dr. Alexander Kurtenkov and Prof. Tanyu Bonev for the discussions we had during the development, and Stoyan Glushkov
// for staying late at night testing the implementation.

/** INDIGO Bus
 \file indigo_align.h
 */

#ifndef indigo_align_h
#define indigo_align_h

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const double TWO_PI;
extern const double DEG2RAD;
extern const double RAD2DEG;

#define PA_MAX_ERROR           10.0
#define PA_MAX_INITIAL_ERROR   3.5

//#define DELTA_T          (34 + 32.184 + 0.477677)
#define JD2000           2451545.0

#ifndef UT2JD
#define DELTA_UT1_UTC (0.0299836 / 86400.0) /* For 2025-05-22 */
#define UT2JD(t)         ((t) / 86400.0 + 2440587.5 + DELTA_UT1_UTC)
#define JDNOW            UT2JD(time(NULL))
#endif /* UT2JD */

#define NUT_APPARENT_TO_MEAN 0
#define NUT_MEAN_TO_APPARENT 1
#define NUT_DELTA_ONLY       2

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

/**
 Precesses c0 from eq0 to eq1

 c0.a - Right Ascension (radians)
 c0.d - Declination (radians)
 eq0 - Old Equinox (year+fraction)
 eq1 - New Equinox (year+fraction)
 */
extern indigo_spherical_point_t indigo_precess(const indigo_spherical_point_t *c0, const double eq0, const double eq1);

/**
 Apply, remove ot get nutation corrections for given coordinates

 coord.a - Right Ascension (radians)
 coord.d - Declination (radians)
 jd - Julian Date
 apply_mode - 0 for apparent -> mean-of-date,  1 for mean-of-date -> apparent (true-of-date), 2 for delta only
 */
extern indigo_spherical_point_t indigo_nutate(const indigo_spherical_point_t *coord, double jd, int apply_mode);

/** Convenience wrapper for indigo_precess(...)

	*ra - Right Ascension (hours)
	*dec - Declination (degrees)
 */
extern void indigo_jnow_to_j2k(double *ra, double *dec);

/** Convenience wrapper for indigo_precess(...)

	eq - 0 for JNow or old equinox
	*ra - Right Ascension (hours)
	*dec - Declination (degrees)
 */
extern void indigo_eq_to_j2k(const double eq, double *ra, double *dec);

/** Convenience wrapper for indigo_precess(...)

	*ra - Right Ascension (hours)
	*dec - Declination (degrees)
 */
extern void indigo_j2k_to_jnow(double *ra, double *dec);

/** Convenience wrapper for indigo_precess(...)

	eq - 0 for JNow or new equinox
	*ra - Right Ascension (hours)
	*dec - Declination (degrees)
 */
extern void indigo_j2k_to_eq(const double eq, double *ra, double *dec);

/** Apply nutation correction: mean-of-date -> apparent (true-of-date) coordinates
 *
 * *ra - Right Ascension (hours), modified in place
 * *dec - Declination (degrees), modified in place
 * jd - Julian Date (if <=0 current date is used)
 */
extern void indigo_nutate_mean_to_apparent(double *ra, double *dec, double jd);

/** Apply inverse nutation correction: apparent (true-of-date) -> mean-of-date coordinates
 *
 * *ra - Right Ascension (hours), modified in place
 * *dec - Declination (degrees), modified in place
 * jd - Julian Date (if <=0 current date is used)
 */
extern void indigo_nutate_apparent_to_mean(double *ra, double *dec, double jd);

/** Convert equinox coordinates to apparent (true-of-date) coordinates
 *
 * eq - Source equinox (e.g., 2000.0 for J2000, 1950.0 for B1950)
 * *ra - Right Ascension (hours), modified in place
 * *dec - Declination (degrees), modified in place
 * jd - Julian Date for target apparent coordinates (if <=0 current date is used)
 */
extern void indigo_eq_to_apparent(const double eq, double *ra, double *dec, double jd);

/** Convert apparent (true-of-date) coordinates to equinox coordinates
 *
 * eq - Target equinox (e.g., 2000.0 for J2000, 1950.0 for B1950)
 * *ra - Right Ascension (hours), modified in place
 * *dec - Declination (degrees), modified in place
 * jd - Julian Date of the apparent coordinates (if <=0 current date is used)
 */
extern void indigo_apparent_to_eq(const double eq, double *ra, double *dec, double jd);

/** calculate time to the next transit

	ra - right ascension of the object in decimal hours
	lmst - local mean sidereal time in decimal degrees
	allow_negative_time - if true it returns time to closest transit in decimal hours,
		if previous transit is closer it returns negative time since transit.

	returns time to the next or closest transit in decimal hours depending on allow_negative_time
 */
extern double indigo_time_to_transit(const double ra, const double lmst, const bool allow_negative_time);

/** Calculate raise transit and set times for the nearest transit

	jd - julian day
	latitude (degrees)
	longitude (degrees)
	ra - Right Ascension (decimal hours)
	dec - Declination (degrees)
	*raise_time - raise time (decimal hours)
	*transit_time - transit time (decinal hours)
	*set_time - set time (decinal hours)
 */
void indigo_raise_set(
	const double jd,
	const double latitude,
	const double longitude,
	const double ra,
	const double dec,
	double *raise_time,
	double *transit_time,
	double *set_time
);

/** Calculate the airmass for a given altitude
 */
extern double indigo_airmass(double altitude);

/**
 * @brief Calculate the derotation rate for an alt-azimuth mounted telescope.
 *
 * This function calculates the derotation rate (also known as field rotation rate)
 * for an alt-azimuth mounted telescope based on the altitude and azimuth of the object
 * and the latitude of the observer. The derotation rate is returned in arcseconds per second.
 *
 * @param alt The altitude of the object, in degrees.
 * @param az The azimuth of the object, in degrees.
 * @param latitude The latitude of the observer, in degrees.
 * @return The derotation rate in arcseconds per second.
 */
extern double indigo_derotation_rate(double alt, double az, double latitude);

/**
 * @brief Calculate the parallactic angle for a celestial object.
 *
 * This function calculates the parallactic angle, which is the angle between the
 * celestial pole and the zenith at the position of a celestial object. The parallactic
 * angle is calculated based on the latitude of the observer, the declination of the object,
 * and the hour angle of the object.
 *
 * @param ha The hour angle of the object, in degrees.
 * @param dec The declination of the object, in degrees.
 * @param latitude The latitude of the observer, in degrees.
 * @return The parallactic angle in degrees.
 */
extern double indigo_parallactic_angle(double ha, double dec, double latitude);

/** Greenwitch mean sidereal time (in degrees)
 */
extern double indigo_mean_gst(const time_t *utc);

/** Local mean sidereal time (in decimal hours)
 */
extern double indigo_lst(const time_t *utc, const double longitude);

/** Convert Julian Date to Greenwich Mean Sidereal Time
 *
 * jd - Julian Date
 * returns GMST in decimal hours
 */
extern double indigo_jd_to_mean_gst(double jd);

/** ra/dec to alt/az (in degrees)
 */
extern void indigo_radec_to_altaz(const double ra, const double dec, const time_t *utc, const double latitude, const double longitude, const double elevation, double *alt, double *az);

/** convert ha dec to az alt in radians
 */
extern void indigo_equatorial_to_hotizontal(const indigo_spherical_point_t *eq_point, const double latitude, indigo_spherical_point_t *h_point);

/** convert spherical to cartesian coordinates
 */
extern indigo_cartesian_point_t indigo_spherical_to_cartesian(const indigo_spherical_point_t *spoint);

/** convert spherical (in radians) to cartesian coordinates
 */
extern indigo_spherical_point_t indigo_cartesian_to_spherical(const indigo_cartesian_point_t *cpoint);

/** rotate cartesian coordinates around axes (angles in radians)
 */
extern indigo_cartesian_point_t indigo_cartesian_rotate_x(const indigo_cartesian_point_t *point, double angle);
extern indigo_cartesian_point_t indigo_cartesian_rotate_y(const indigo_cartesian_point_t *point, double angle);
extern indigo_cartesian_point_t indigo_cartesian_rotate_z(const indigo_cartesian_point_t *point, double angle);

/** rotate coordinates using polar errors
 * possition->a = Hour angle in radians
 * possition->d = Declination in radians
 * possition->r = 1; (should be 1)
 * u = angle in radians, Altitude error.
 * v = angle in radians, Azimuth error.
 */
extern indigo_spherical_point_t indigo_apply_polar_error(const indigo_spherical_point_t *position, double u, double v);

/** derotate coordinates using polar errors
 * possition->a = Hour angle in radians
 * possition->d = Declination in radians
 * possition->r = 1; (should be 1)
 * u = angle in radians, Altitude error.
 * v = angle in radians, Azimuth error.
 */
extern indigo_spherical_point_t indigo_correct_polar_error(const indigo_spherical_point_t *position, double u, double v);


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

/** calculate refraction error from zenith distance
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

/** calculate polar alignment error and Declination drifts
 */
extern bool indigo_polar_alignment_error_3p(
	const indigo_spherical_point_t *p1,
	const indigo_spherical_point_t *p2,
	const indigo_spherical_point_t *p3,
	double *d2,
	double *d3,
	double *u,
	double *v
);

/** recalculates polar error for a given target position (if thelescope is aligned) and current position
 */
extern bool indigo_reestimate_polar_error(
	const indigo_spherical_point_t *position,
	const indigo_spherical_point_t *target_position,
	double *u,
	double *v
);

/** calculates the position where the telescope sould point if properly polar aligned
 *  for the given position and polar errors
 */
bool indigo_polar_alignment_target_position(
	const indigo_spherical_point_t *position,
	const double u,
	const double v,
	indigo_spherical_point_t *target_position
);

#ifdef __cplusplus
}
#endif

#endif /* indigo_align_h */
