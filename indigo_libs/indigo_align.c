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

/** INDIGO Bus
 \file indigo_align.c
 */

#ifdef NO_INDIGO_TEST
#define indigo_log printf
#define indigo_debug printf
#else
#include <indigo/indigo_bus.h>
#endif
#include <indigo/indigo_align.h>

const double TWO_PI = 2 * M_PI;
const double DEG2RAD = M_PI / 180.0;
const double RAD2DEG = 180.0 / M_PI;

/* Convenience wrappers for indigo_precess(...) */

static double jnow(void) {
	return 2000 + ((time(NULL) / 86400.0 + 2440587.5 - 0.477677 / 86400.0) - 2451545.0) / 365.25;
}

void indigo_jnow_to_j2k(double *ra, double *dec) {
	indigo_spherical_point_t coordinates = { *ra * 15 * DEG2RAD, *dec * DEG2RAD, 0 };
	coordinates = indigo_precess(&coordinates, jnow(), 2000.0);
	*ra = coordinates.a * RAD2DEG / 15;
	*dec = coordinates.d * RAD2DEG;
}

void indigo_eq_to_j2k(const double eq, double *ra, double *dec) {
	if (eq != 2000.0) {
		indigo_spherical_point_t coordinates = { *ra * 15 * DEG2RAD, *dec * DEG2RAD, 0 };
		coordinates = indigo_precess(&coordinates, eq != 0 ? eq : jnow(), 2000.0);
		*ra = coordinates.a * RAD2DEG / 15;
		*dec = coordinates.d * RAD2DEG;
	}
}

void indigo_j2k_to_jnow(double *ra, double *dec) {
	indigo_spherical_point_t coordinates = { *ra * 15 * DEG2RAD, *dec * DEG2RAD, 0 };
	coordinates = indigo_precess(&coordinates, 2000.0, jnow());
	*ra = coordinates.a * RAD2DEG / 15;
	*dec = coordinates.d * RAD2DEG;
}

void indigo_j2k_to_eq(const double eq, double *ra, double *dec) {
	if (eq != 2000.0) {
		indigo_spherical_point_t coordinates = { *ra * 15 * DEG2RAD, *dec * DEG2RAD, 0 };
		coordinates = indigo_precess(&coordinates, 2000.0, eq != 0 ? eq : jnow());
		*ra = coordinates.a * RAD2DEG / 15;
		*dec = coordinates.d * RAD2DEG;
	}
}

double indigo_mean_gst(const time_t *utc) {
	long double gst;
	long double t;
	double jd;

	if (utc)
		jd = UT2JD(*utc);
	else
		jd = UT2JD(time(NULL));

	t = (jd - 2451545.0) / 36525.0;
	gst = 280.46061837 + (360.98564736629 * (jd - 2451545.0)) + (0.000387933 * t * t) - (t * t * t / 38710000.0);
	gst = fmod(gst + 360.0, 360.0);
	gst *= 24.0 / 360.0;
	return gst;
}

double indigo_lst(const time_t *utc, const double longitude) {
	double gst = indigo_mean_gst(utc);
	return fmod(gst + longitude/15.0 + 24.0, 24.0);
}

void indigo_radec_to_altaz(const double ra, const double dec, const time_t *utc, const double latitude, const double longitude, const double elevation, double *alt, double *az) {
	indigo_spherical_point_t eq_point;
	indigo_spherical_point_t h_point;
	double lst = indigo_lst(utc, longitude);

	indigo_ra_dec_to_point(ra, dec, lst, &eq_point);
	indigo_equatorial_to_hotizontal(&eq_point, latitude * DEG2RAD, &h_point);
	*az = h_point.a * RAD2DEG;
	*alt = h_point.d *RAD2DEG;
}

/* Precesses c0 from eq0 to eq1 */
indigo_spherical_point_t indigo_precess(const indigo_spherical_point_t *c0, const double eq0, const double eq1) {
	double rot[3][3];
	indigo_spherical_point_t c1 = {0, 0, 1};

	double cosd = cos(c0->d);

	double x0 = cosd * cos(c0->a);
	double y0 = cosd * sin(c0->a);
	double z0 = sin(c0->d);

	double ST = (eq0 - 2000.0) * 0.001;
	double T = (eq1 - eq0) * 0.001;

	const double sec2rad = DEG2RAD/3600.0;
	double A = sec2rad * T * (23062.181 + ST * (139.656 + 0.0139 * ST) + T * (30.188 - 0.344 * ST + 17.998 * T));
	double B = sec2rad * T * T * (79.280 + 0.410 * ST + 0.205 * T) + A;
	double C = sec2rad * T * (20043.109 - ST * (85.33 + 0.217 * ST) + T * (-42.665 - 0.217 * ST - 41.833 * T));

	double sinA = sin(A);
	double sinB = sin(B);
	double sinC = sin(C);
	double cosA = cos(A);
	double cosB = cos(B);
	double cosC = cos(C);

	rot[0][0] = cosA * cosB * cosC - sinA * sinB;
	rot[0][1] = (-1) * sinA * cosB * cosC - cosA * sinB;
	rot[0][2] = (-1) * sinC * cosB;

	rot[1][0] = cosA * cosC * sinB + sinA * cosB;
	rot[1][1] = (-1) * sinA * cosC * sinB + cosA * cosB;
	rot[1][2] = (-1) * sinB * sinC;

	rot[2][0] = cosA * sinC;
	rot[2][1] = (-1) * sinA * sinC;
	rot[2][2] = cosC;

	double x1 = rot[0][0] * x0 + rot[0][1] * y0 + rot[0][2] * z0;
	double y1 = rot[1][0] * x0 + rot[1][1] * y0 + rot[1][2] * z0;
	double z1 = rot[2][0] * x0 + rot[2][1] * y0 + rot[2][2] * z0;

	if (x1 == 0) {
		if (y1 > 0) {
			c1.a = 90.0 * DEG2RAD;
		} else {
			c1.a = 270.0 * DEG2RAD;
		}
	} else {
		c1.a = atan2(y1, x1);
	}
	if (c1.a < 0) {
		c1.a += 360 * DEG2RAD;
	}

	c1.d = atan2( z1 , sqrt(1 - z1 * z1));

	return c1;
}

/* convert spherical to cartesian coordinates */
indigo_cartesian_point_t indigo_spherical_to_cartesian(const indigo_spherical_point_t *spoint) {
	indigo_cartesian_point_t cpoint = {0,0,0};
	double theta = -spoint->a;
	double phi = (M_PI / 2.0) - spoint->d;
	double sin_phi = sin(phi);
	cpoint.x = spoint->r * sin_phi * cos(theta);
	cpoint.y = spoint->r * sin_phi * sin(theta);
	cpoint.z = spoint->r * cos(phi);
	return cpoint;
}

/* convert spherical (in radians) to cartesian coordinates */
indigo_spherical_point_t indigo_cartesian_to_spherical(const indigo_cartesian_point_t *cpoint) {
	indigo_spherical_point_t spoint = {0, M_PI / 2.0 , 1};

	if (cpoint->y == 0.0 && cpoint->x == 0.0) {
		/* Zenith */
		return spoint;
	}

	spoint.a = (cpoint->y == 0) ? 0.0 : -atan2(cpoint->y, cpoint->x);
	if (spoint.a < 0) spoint.a += TWO_PI;
	spoint.d = (M_PI / 2.0) - acos(cpoint->z);
	return spoint;
}

/* rotate cartesian coordinates around axes */
indigo_cartesian_point_t indigo_cartesian_rotate_x(const indigo_cartesian_point_t *point, double angle) {
	indigo_cartesian_point_t rpoint = {0,0,0};
	double sin_a = sin(-angle);
	double cos_a = cos(-angle);
	rpoint.x =  point->x;
	rpoint.y =  point->y * cos_a + point->z * sin_a;
	rpoint.z = -point->y * sin_a + point->z * cos_a;
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

/* rotate coordinates using polar errors */
indigo_spherical_point_t indigo_apply_polar_error(const indigo_spherical_point_t *position, double u, double v) {
	indigo_cartesian_point_t position_h = indigo_spherical_to_cartesian(position);
	indigo_cartesian_point_t position_h_z = indigo_cartesian_rotate_z(&position_h, v);
	indigo_cartesian_point_t position_h_zx = indigo_cartesian_rotate_x(&position_h_z, v);
	indigo_cartesian_point_t position_h_zxy = indigo_cartesian_rotate_y(&position_h_zx, u);
	indigo_spherical_point_t p = indigo_cartesian_to_spherical(&position_h_zxy);
	return p;
}

/* derotate coordinates using polar errors */
indigo_spherical_point_t indigo_correct_polar_error(const indigo_spherical_point_t *position, double u, double v) {
	return indigo_apply_polar_error(position, -u, -v);
}

/* convert spherical point in radians to ha/ra dec in hours and degrees */
void indigo_point_to_ra_dec(const indigo_spherical_point_t *spoint, const double lst, double *ra, double *dec) {
	*ra  = lst - spoint->a * RAD2DEG / 15.0 ;
	if (*ra >= 24) {
		*ra -= 24.0;
	}
	if (*ra < 0) {
		*ra += 24.0;
	}

	*dec = spoint->d * RAD2DEG;
}

/* convert ha dec to alt az in radians */
void indigo_equatorial_to_hotizontal(const indigo_spherical_point_t *eq_point, const double latitude, indigo_spherical_point_t *h_point) {
	double sin_ha = sin(eq_point->a);
	double cos_ha = cos(eq_point->a);
	double sin_dec = sin(eq_point->d);
	double cos_dec = cos(eq_point->d);
	double sin_lat = sin(latitude);
	double cos_lat = cos(latitude);

	// az
	h_point->a = atan2(-cos_dec * sin_ha, cos_lat * sin_dec - sin_lat * cos_dec * cos_ha);
	if (h_point->a < 0) h_point->a += TWO_PI;
	// alt
	h_point->d = asin(sin_dec * sin_lat + cos_dec * cos_lat * cos_ha);
	h_point->r = 1;
}

/* convert ha/ra dec in hours and degrees to spherical point in radians */
void indigo_ra_dec_to_point(const double ra, const double dec, const double lst, indigo_spherical_point_t *spoint) {
	double ha = lst - ra;
	if (ha < 0) {
		ha += 24;
	}
	if (ha >= 24) {
		ha -= 24;
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
	indigo_debug("Refraction = %.3f', Z = %.4f deg\n", r * RAD2DEG * 60, z * RAD2DEG);
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
	if (st_aparent->a < 0) st_aparent->a += TWO_PI;
	st_aparent->d = asin(sin_lat * cos(azd) + cos_lat * sin(azd) * cos_az);
	st_aparent->r = 1;
	indigo_debug("Refraction HA (real/aparent) = %f / %f, DEC (real / aparent) = %f / %f\n", st->a * RAD2DEG, st_aparent->a * RAD2DEG, st->d * RAD2DEG, st_aparent->d * RAD2DEG);
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
	if (st_aparent->a < 0) st_aparent->a += TWO_PI;
	st_aparent->d = asin(sin_lat * cos(azd) + cos_lat * sin(azd) * cos_az);
	st_aparent->r = 1;
	indigo_debug("Refraction HA (real/aparent) = %f / %f, DEC (real / aparent) = %f / %f\n", st->a * RAD2DEG, st_aparent->a * RAD2DEG, st->d * RAD2DEG, st_aparent->d * RAD2DEG);
	return true;
}

bool indigo_polar_alignment_error_3p(
	const indigo_spherical_point_t *p1,
	const indigo_spherical_point_t *p2,
	const indigo_spherical_point_t *p3,
	double *d2,
	double *d3,
	double *u,
	double *v
) {
	if (u == NULL || v == NULL || d2 == NULL || d3 == NULL) {
		return false;
	}

	*d2 = p2->d - p1->d;
	*d3 = p3->d - p1->d;

	double sin_a1 = sin(-p1->a);
	double sin_a2 = sin(-p2->a);
	double sin_a3 = sin(-p3->a);
	double cos_a1 = cos(-p1->a);
	double cos_a2 = cos(-p2->a);
	double cos_a3 = cos(-p3->a);

	double k1 = cos_a2 - cos_a1;
	double k2 = sin_a2 - sin_a1;
	double k3 = cos_a3 - cos_a1;
	double k4 = sin_a3 - sin_a1;

	*v = (*d3 * k1 - *d2 * k3) / (k4 * k1 - k2 * k3);
	*u = (*d2 - *v * k2) / k1;

	return true;
}

bool indigo_polar_alignment_target_position(
	const indigo_spherical_point_t *position,
	const double u,
	const double v,
	indigo_spherical_point_t *target_position
) {
	if (target_position == NULL) {
		return false;
	}
	*target_position = indigo_correct_polar_error(position, u, v);
	return true;
}

static void _reestimate_polar_error(
	const indigo_spherical_point_t *position,
	const indigo_spherical_point_t *target_position,
	double min_az, double max_az, double az_inc,
	double min_alt, double max_alt, double alt_inc,
	double *u, double *v
) {
	double min_distance = 1e9;
	for (double c_az = min_az; c_az < max_az; c_az += az_inc) {
		for (double c_alt = min_alt; c_alt < max_alt; c_alt += alt_inc) {
			indigo_spherical_point_t tp;
			indigo_polar_alignment_target_position(position, c_alt, c_az, &tp);
			double distance = indigo_gc_distance_spherical(&tp, target_position);
			if (distance < min_distance) {
				min_distance = distance;
				*v = c_az;
				*u = c_alt;
			}
		}
	}
}

/* recalculates polar error for a given target position (if thelescope is aligned) and current position */
bool indigo_reestimate_polar_error(
	const indigo_spherical_point_t *position,
	const indigo_spherical_point_t *target_position,
	double *u, double *v
) {
	const double search_radius = 5.0;
	_reestimate_polar_error(
		position,
		target_position,
		-search_radius * DEG2RAD,
		 search_radius * DEG2RAD,
		 0.2 * DEG2RAD,
		-search_radius * DEG2RAD,
		 search_radius * DEG2RAD,
		 0.2 * DEG2RAD,
		 u,
		 v
	 );
	_reestimate_polar_error(
		position,
		target_position,
		*v - .2 * DEG2RAD,
		*v + .2 * DEG2RAD,
		0.02 * DEG2RAD,
		*u - .2 * DEG2RAD,
		*u + .2 * DEG2RAD,
		0.02 * DEG2RAD,
		u,
		v
	);
	_reestimate_polar_error(
		position,
		target_position,
		*v - .02 * DEG2RAD,
		*v + .02 * DEG2RAD,
		0.002 * DEG2RAD,
		*u - .02 * DEG2RAD,
		*u + .02 * DEG2RAD,
		0.002 * DEG2RAD,
		u,
		v
	);
	_reestimate_polar_error(
		position,
		target_position,
		*v - .002 * DEG2RAD,
		*v + .002 * DEG2RAD,
		0.0002 * DEG2RAD,
		*u - .002 * DEG2RAD,
		*u + .002 * DEG2RAD,
		0.0002 * DEG2RAD,
		u,
		v
	);

	if (fabs(*u * RAD2DEG) > search_radius * 0.95 || fabs(*v * RAD2DEG) > search_radius * 0.95) {
		return false;
	}

	return true;
}
