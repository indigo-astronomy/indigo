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
 \file indigo_align.c
 */

#include <indigo/indigo_align.h>

/* convert spherical to cartesian coordinates */
indigo_cartesian_point_t indigo_spherical_to_cartesian(const indigo_spherical_point_t *spoint) {
	indigo_cartesian_point_t cpoint = {0,0,0};
	double cos_d = cos(spoint->d);
	cpoint.x = spoint->r * cos_d * cos(spoint->a);
	cpoint.y = spoint->r * cos_d * sin(spoint->a);
	cpoint.z = spoint->r * sin(spoint->d);
	return cpoint;
}

/* convert spherical (in radians) to cartesian coordinates */
indigo_spherical_point_t indigo_cartesian_to_sphercal(const indigo_cartesian_point_t *cpoint) {
	indigo_spherical_point_t spoint = {0,0,0};
	spoint.r = sqrt(cpoint->x * cpoint->x + cpoint->y * cpoint->y + cpoint->z * cpoint->z);
	spoint.a = atan(cpoint->y / cpoint->x);
	spoint.d = acos(cpoint->z / spoint.r);
	return spoint;
}

/* convert spherical point in radians to ha/ra dec in hours and degrees */
void indigo_spherical_to_ra_dec(const indigo_spherical_point_t *spoint, const double lst, double *ra, double *dec) {
	*ra  = lst + spoint->a / DEG2RAD / 15.0 ;
	if (*ra > 24) {
		*ra -= 24.0;
	}
	*dec = spoint->d / DEG2RAD;
}

/* convert ha/ra dec in hours and degrees to spherical point in radians */
void indigo_ra_dec_to_point(const double ra, const double dec, const double lst, indigo_spherical_point_t *spoint) {
	double ha = lst - ra;
	if (ha < 0) {
		ha += 24;
	}
	spoint->a = ha * 15.0 * DEG2RAD;
	spoint->d = dec * DEG2RAD;
	spoint->r = 1;
}

/* great circle distances */
double indigo_gc_distance_spherical(const indigo_spherical_point_t *sp1, const indigo_spherical_point_t *sp2) {
	double sin_d1 = sin(sp1->d);
	double cos_d1 = cos(sp1->d);
	double sin_d2 = sin(sp2->d);
	double cos_d2 = cos(sp2->d);
	double delta_a = fabs(sp1->a - sp2->a);
	double cos_delta_a = cos(delta_a);

	return acos(sin_d1*sin_d2 + cos_d1*cos_d2*cos_delta_a);
}

double indigo_gc_distance(double ra1, double dec1, double ra2, double dec2) {
	indigo_spherical_point_t sp1 = {ra1 * DEG2RAD * 15.0, dec1 * DEG2RAD, 1};
	indigo_spherical_point_t sp2 = {ra2 * DEG2RAD * 15.0, dec2 * DEG2RAD, 1};

	return indigo_gc_distance_spherical(&sp1, &sp2) * RAD2DEG;
}

double indigo_gc_distance_cartesian(const indigo_cartesian_point_t *cp1, const indigo_cartesian_point_t *cp2) {
	double delta_x = cp1->x - cp2->x;
	double delta_y = cp1->y - cp2->y;
	double delta_z = cp1->z - cp2->z;
	double delta = sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

	return 2 * asin(delta / 2);
}

double indigo_calculate_refraction(const double z) {
		double r = 1.02 / tan(DEG2RAD * (z * RAD2DEG + 10.3 / (z * RAD2DEG + 5.11))); // in arcmin
		return r / 60.0 * DEG2RAD;
}

/* compensate atmospheric refraction */
bool indigo_compensate_refraction(
	const indigo_spherical_point_t *st,
	const double latitude,
	indigo_spherical_point_t *st_aparent
) {
	double sin_lat = sin(latitude);
	double cos_lat = cos(latitude);
	double sin_d = sin(st->d);
	double cos_d = cos(st->d);
	double sin_h = sin(st->a);
	double cos_h = cos(st->a);

	if (cos_d == 0) return false;

	double tan_d = sin_d / cos_d;

	double z = acos(sin_lat * sin_d + cos_lat * cos_d * cos_h);
	double az = atan2(sin_h, cos_lat * tan_d - sin_lat * cos_h);

	double r = indigo_calculate_refraction(z);
	double azd = z - r;

	double tan_azd = tan(azd);
	double cos_az = cos(az);

	st_aparent->a = atan2(sin(az) * tan_azd, cos_lat - sin_lat * cos_az * tan_azd);
	st_aparent->d = asin(sin_lat * cos(azd) + cos_lat * sin(azd) * cos_az);
	st_aparent->r = 1;
	return true;
}

/* calculate polar alignment error */
bool _indigo_polar_alignment_error(
	const indigo_spherical_point_t *st1,
	const indigo_spherical_point_t *st2,
	const indigo_spherical_point_t *st2_observed,
	const double latitude,
	indigo_spherical_point_t *equatorial_error,
	indigo_spherical_point_t *horizontal_error
) {
	if (equatorial_error == NULL || horizontal_error == NULL) {
		return false;
	}

	equatorial_error->a = st2_observed->a - st2->a;
	equatorial_error->d = st2_observed->d - st2->d;
	equatorial_error->r = 1;

	double cos_lat = cos(latitude);
	double tan_d1 = tan(st1->d);
	double tan_d2 = tan(st2->d);
	double sin_h1 = sin(st1->a);
	double sin_h2 = sin(st2->a);
	double cos_h1 = cos(st1->a);
	double cos_h2 = cos(st2->a);

	if (isnan(tan_d1) || isnan(tan_d2)) {
		return false;
	}

	double det = cos_lat * (tan_d1 + tan_d2) * (1 - cos(st1->a - st2->a));

	if (det == 0) {
		return false;
	}

	horizontal_error->d = cos_lat * (equatorial_error->a * (sin_h2 - sin_h1) - equatorial_error->d * (tan_d1 * cos_h1 - tan_d2 * cos_h2)) / det;
	horizontal_error->a = (equatorial_error->a * (cos_h1 - cos_h2) + equatorial_error->d * (tan_d2 * sin_h2 - tan_d1 * sin_h1)) / det;
	horizontal_error->r = 1;
	return true;
}

bool indigo_polar_alignment_error(
	const indigo_spherical_point_t *st1,
	const indigo_spherical_point_t *st2,
	const indigo_spherical_point_t *st2_observed,
	const double latitude,
	const bool compensate_refraction,
	indigo_spherical_point_t *equatorial_error,
	indigo_spherical_point_t *horizontal_error
) {
	if (compensate_refraction) {
		indigo_spherical_point_t st1_a, st2_a, st2_observed_a;
		if (!indigo_compensate_refraction(st1, latitude, &st1_a)) return false;
		if (!indigo_compensate_refraction(st2, latitude, &st2_a)) return false;
		if (!indigo_compensate_refraction(st2_observed, latitude, &st2_observed_a)) return false;
		return _indigo_polar_alignment_error(&st1_a, &st2_a, &st2_observed_a, latitude, equatorial_error, horizontal_error);
	} else {
		return _indigo_polar_alignment_error(st1, st2, st2_observed, latitude, equatorial_error, horizontal_error);
	}
}
