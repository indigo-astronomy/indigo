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

#include <indigo/indigo_bus.h>
#include <indigo/indigo_align.h>

indigo_cartesian_point_t indigo_spherical_to_cartesian(const indigo_spherical_point_t *spoint) {
	indigo_cartesian_point_t cpoint = {0,0,0};
	double cos_d = cos(spoint->d);
	cpoint.x = spoint->r * cos_d * cos(spoint->a);
	cpoint.y = spoint->r * cos_d * sin(spoint->a);
	cpoint.z = spoint->r * sin(spoint->d);
	return cpoint;
}

/* convert spherical (in radians) to cartesian coordinates */
indigo_spherical_point_t indigo_cartesian_to_spherical(const indigo_cartesian_point_t *cpoint) {
	indigo_spherical_point_t spoint = {0,0,0};
	spoint.r = sqrt(cpoint->x * cpoint->x + cpoint->y * cpoint->y + cpoint->z * cpoint->z);
	spoint.a = atan2(cpoint->y, cpoint->x);
	if (spoint.a < 0) spoint.a += 2 * M_PI;
	spoint.d = asin(cpoint->z / spoint.r);
	return spoint;
}

/* rotate cartesian coordinates around axes */
indigo_cartesian_point_t indigo_cartesian_rotate_x(const indigo_cartesian_point_t *point, double angle) {
	indigo_cartesian_point_t rpoint = {0,0,0};
	double sin_a = sin(-angle);
	double cos_a = cos(-angle);
	rpoint.x =  point->x;
	rpoint.y =  point->y * cos_a + point->z * sin_a;
	rpoint.z =  -point->y * sin_a + point->z * cos_a;
	return rpoint;
}

indigo_cartesian_point_t indigo_cartesian_rotate_y(const indigo_cartesian_point_t *point, double angle) {
	indigo_cartesian_point_t rpoint = {0,0,0};
	double sin_a = sin(angle);
	double cos_a = cos(angle);
	rpoint.x = point->x * cos_a - point->z * sin_a;
	rpoint.y = point->y;
	rpoint.z = point->x * sin_a + point->z * cos_a;
	return rpoint;
}

indigo_cartesian_point_t indigo_cartesian_rotate_z(const indigo_cartesian_point_t *point, double angle) {
	indigo_cartesian_point_t rpoint = {0,0,0};
	double sin_a = sin(-angle);
	double cos_a = cos(-angle);
	rpoint.x =  point->x * cos_a + point->y * sin_a;
	rpoint.y = -point->x * sin_a + point->y * cos_a;
	rpoint.z =  point->z;
	return rpoint;
}

/* convert spherical point in radians to ha/ra dec in hours and degrees */
void indigo_spherical_to_ra_dec(const indigo_spherical_point_t *spoint, const double lst, double *ra, double *dec) {
	*ra  = lst + spoint->a / DEG2RAD / 15.0 ;
	if (*ra > 24) {
		*ra -= 24.0;
	}
	*dec = spoint->d / DEG2RAD;
}

/* convert ha dec to alt az in radians */
void indigo_equatorial_to_hotizontal(const indigo_spherical_point_t *eq_point, const double latitude, indigo_spherical_point_t *h_point) {
	double sin_ha = sin(eq_point->a);
	double cos_ha = cos(eq_point->a);
	double sin_dec = sin(eq_point->d);
	double cos_dec = cos(eq_point->d);
	double sin_lat = sin(latitude);
	double cos_lat = cos(latitude);

	double sin_alt = sin_dec * sin_lat + cos_dec * cos_lat * cos_ha;
	double alt = asin(sin_alt);

	double cos_az = (sin_dec - sin_alt * sin_lat) / (cos(alt) * cos_lat);
	double az = acos(cos_az);

	if (sin_ha > 0) az = 2 * M_PI - az;
	h_point->a = az;
	h_point->d = alt;
	h_point->r = 1;
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
		double r = (1.02 / tan(DEG2RAD * ((90 - z * RAD2DEG) + 10.3 / ((90 - z * RAD2DEG) + 5.11)))) / 60 * DEG2RAD; // in arcmin
		INDIGO_DEBUG(indigo_debug("Refraction = %.3f', Z = %.4f deg\n", r * RAD2DEG * 60, z * RAD2DEG));
		return r;
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
	if (st_aparent->a < 0) st_aparent->a += 2 * M_PI;
	st_aparent->d = asin(sin_lat * cos(azd) + cos_lat * sin(azd) * cos_az);
	st_aparent->r = 1;
	INDIGO_DEBUG(indigo_debug("Refraction HA (real/aparent) = %f / %f, DEC (real / aparent) = %f / %f\n", st->a * RAD2DEG, st_aparent->a * RAD2DEG, st->d * RAD2DEG, st_aparent->d * RAD2DEG));
	return true;
}

bool indigo_compensate_refraction2(
	const indigo_spherical_point_t *st,
	const double latitude,
	const double refraction,
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

	double azd = z - refraction;

	double tan_azd = tan(azd);
	double cos_az = cos(az);

	st_aparent->a = atan2(sin(az) * tan_azd, cos_lat - sin_lat * cos_az * tan_azd);
	if (st_aparent->a < 0) st_aparent->a += 2 * M_PI;
	st_aparent->d = asin(sin_lat * cos(azd) + cos_lat * sin(azd) * cos_az);
	st_aparent->r = 1;
	INDIGO_DEBUG(indigo_debug("Refraction HA (real/aparent) = %f / %f, DEC (real / aparent) = %f / %f\n", st->a * RAD2DEG, st_aparent->a * RAD2DEG, st->d * RAD2DEG, st_aparent->d * RAD2DEG));
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

/* calculate aparent polar alignment correction for a gven poition */
bool indigo_polar_alignment_corrections_at_position(
	const indigo_spherical_point_t *position,
	const double latitude,
	const indigo_spherical_point_t *horizontal_error,
	indigo_spherical_point_t *target_position,
	indigo_spherical_point_t *horizontal_correction
) {
	if (!horizontal_correction || !target_position) {
		return false;
	}
	indigo_spherical_point_t position_h;
	indigo_equatorial_to_hotizontal(position, latitude, &position_h);

	indigo_cartesian_point_t position_hc = indigo_spherical_to_cartesian(&position_h);
	indigo_cartesian_point_t position_hcr = indigo_cartesian_rotate_y(&position_hc, -horizontal_error->d);
	indigo_spherical_point_t position_hr = indigo_cartesian_to_spherical(&position_hcr);
	// rotation in azumuth is simply adding the angle
	position_hr.a += horizontal_error->a;

	horizontal_correction->a = position_hr.a - position_h.a;
	horizontal_correction->d = position_hr.d - position_h.d;
	horizontal_correction->r = 1;
	*target_position =  position_hr;
	return true;
}
