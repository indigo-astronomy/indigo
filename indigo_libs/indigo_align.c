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

static double eq_of_date(double jd) {
	return 2000.0 + (jd - 2451545.0) / 365.25;
}

static double jnow(void) {
	return eq_of_date(UT2JD(time(NULL)));
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

/* Equinox -> apparent (true-of-date) at jd */
void indigo_eq_to_apparent(const double eq, double *ra, double *dec, double jd) {
	indigo_spherical_point_t p = { *ra * 15.0 * DEG2RAD, *dec * DEG2RAD, 1 };
	p = indigo_precess(&p, eq != 0 ? eq : jnow(), eq_of_date(jd));
	p = indigo_nutate_mean_to_true(&p, jd);
	*ra  = p.a * RAD2DEG / 15.0;
	*dec = p.d * RAD2DEG;
}

void indigo_eq_to_apparent_now(const double eq, double *ra, double *dec) {
	indigo_eq_to_apparent(eq, ra, dec, UT2JD(time(NULL)));
}

/* J2000 (mean) -> apparent (true-of-date) at jd */
void indigo_j2k_to_apparent(double *ra, double *dec, double jd) {
	indigo_eq_to_apparent(2000.0, ra, dec, jd);
}

void indigo_j2k_to_apparent_now(double *ra, double *dec) {
	indigo_eq_to_apparent_now(2000.0, ra, dec);
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

	c1.a = atan2(y1, x1);
	if (c1.a < 0) {
		c1.a += TWO_PI;
	}

	// Clamp z1 to [-1,1] to handle numerical precision issues
	if (z1 > 1.0) z1 = 1.0;
	if (z1 < -1.0) z1 = -1.0;
	c1.d = asin(z1);

	return c1;
}

/* Apply nutation: mean-of-date -> true-of-date (apparent) */
indigo_spherical_point_t indigo_nutate_mean_to_true(const indigo_spherical_point_t *mean, double jd) {
	double T = (jd - 2451545.0) / 36525.0;
	double T2 = T * T;
	double T3 = T2 * T;

	/* Fundamental arguments in arcseconds, then convert to radians */
	/* Mean elongation of the Moon from the Sun */
	double D = fmod(297.85036 + 445267.111480*T - 0.0019142*T2 + T3/189474.0, 360.0) * DEG2RAD;
	/* Mean anomaly of the Sun */
	double M = fmod(357.52772 + 35999.050340*T - 0.0001603*T2 - T3/300000.0, 360.0) * DEG2RAD;
	/* Mean anomaly of the Moon */
	double Mp = fmod(134.96298 + 477198.867398*T + 0.0086972*T2 + T3/56250.0, 360.0) * DEG2RAD;
	/* Moon's argument of latitude */
	double F = fmod(93.27191 + 483202.017538*T - 0.0036825*T2 + T3/327270.0, 360.0) * DEG2RAD;
	/* Longitude of the ascending node of the Moon */
	double Om = fmod(125.04452 - 1934.136261*T + 0.0020708*T2 + T3/450000.0, 360.0) * DEG2RAD;

	/* Nutation series (63 principal terms from IAU 1980) */
	struct nutation_term {
		int D, M, Mp, F, Om;
		double psi_0, psi_1;  /* longitude coefficients */
		double eps_0, eps_1;  /* obliquity coefficients */
	};

	static const struct nutation_term terms[] = {
		/* D   M  Mp   F  Om     psi_0    psi_1     eps_0    eps_1 */
		{  0,  0,  0,  0,  1, -171996.0, -174.2,  92025.0,   8.9 },
		{ -2,  0,  0,  2,  2,  -13187.0,   -1.6,   5736.0,  -3.1 },
		{  0,  0,  0,  2,  2,   -2274.0,   -0.2,    977.0,  -0.5 },
		{  0,  0,  0,  0,  2,    2062.0,    0.2,   -895.0,   0.5 },
		{  0,  1,  0,  0,  0,    1426.0,   -3.4,     54.0,  -0.1 },
		{  0,  0,  1,  0,  0,     712.0,    0.1,     -7.0,   0.0 },
		{ -2,  1,  0,  2,  2,    -517.0,    1.2,    224.0,  -0.6 },
		{  0,  0,  0,  2,  1,    -386.0,   -0.4,    200.0,   0.0 },
		{  0,  0,  1,  2,  2,    -301.0,    0.0,    129.0,  -0.1 },
		{ -2, -1,  0,  2,  2,     217.0,   -0.5,    -95.0,   0.3 },
		{ -2,  0,  1,  0,  0,    -158.0,    0.0,      0.0,   0.0 },
		{ -2,  0,  0,  2,  1,     129.0,    0.1,    -70.0,   0.0 },
		{  0,  0, -1,  2,  2,     123.0,    0.0,    -53.0,   0.0 },
		{  2,  0,  0,  0,  0,      63.0,    0.0,      0.0,   0.0 },
		{  0,  0,  1,  0,  1,      63.0,    0.1,    -33.0,   0.0 },
		{  2,  0, -1,  2,  2,     -59.0,    0.0,     26.0,   0.0 },
		{  0,  0, -1,  0,  1,     -58.0,   -0.1,     32.0,   0.0 },
		{  0,  0,  1,  2,  1,     -51.0,    0.0,     27.0,   0.0 },
		{ -2,  0,  2,  0,  0,      48.0,    0.0,      0.0,   0.0 },
		{  0,  0, -2,  2,  1,      46.0,    0.0,    -24.0,   0.0 },
	};

	double dpsi_0_0001as = 0.0, deps_0_0001as = 0.0;

	for (unsigned i = 0; i < sizeof(terms)/sizeof(terms[0]); i++) {
		double arg = terms[i].D*D + terms[i].M*M + terms[i].Mp*Mp + terms[i].F*F + terms[i].Om*Om;
		double sin_arg = sin(arg);
		double cos_arg = cos(arg);

		dpsi_0_0001as += (terms[i].psi_0 + terms[i].psi_1 * T) * sin_arg;
		deps_0_0001as += (terms[i].eps_0 + terms[i].eps_1 * T) * cos_arg;
	}

	/* Convert from 0.0001 arcseconds to radians */
	const double ASEC2RAD = (M_PI / 180.0) / 3600.0;
	double dpsi = dpsi_0_0001as * 1e-4 * ASEC2RAD;
	double deps = deps_0_0001as * 1e-4 * ASEC2RAD;

	/* Mean obliquity of the ecliptic (Laskar's expression) */
	double eps0_arcsec = 84381.448 - 46.8150*T - 0.00059*T2 + 0.001813*T3;
	double eps0 = eps0_arcsec * ASEC2RAD;
	double eps = eps0 + deps;

	/* Apply nutation corrections using spherical trigonometry */
	double a = mean->a;
	double d = mean->d;
	double sin_a = sin(a), cos_a = cos(a);
	double sin_d = sin(d), cos_d = cos(d);
	double tan_d = sin_d / cos_d;
	double sin_eps = sin(eps), cos_eps = cos(eps);

	/* Corrections to right ascension and declination */
	double da = (cos_eps + sin_eps * sin_a * tan_d) * dpsi - cos_a * tan_d * deps;
	double dd = sin_eps * cos_a * dpsi + sin_a * deps;

	indigo_spherical_point_t true_coord = *mean;
	true_coord.a += da;
	true_coord.d += dd;

	/* Normalize right ascension */
	if (true_coord.a < 0) true_coord.a += TWO_PI;
	if (true_coord.a >= TWO_PI) true_coord.a -= TWO_PI;

	/* Check declination bounds */
	if (true_coord.d > M_PI/2.0) true_coord.d = M_PI/2.0;
	if (true_coord.d < -M_PI/2.0) true_coord.d = -M_PI/2.0;

	return true_coord;
}

double indigo_jd_to_mean_gst(double jd) {
	long double gst;
	long double t;

	t = (jd - 2451545.0) / 36525.0;
	gst = 280.46061837 + (360.98564736629 * (jd - 2451545.0)) + (0.000387933 * t * t) - (t * t * t / 38710000.0);
	gst = fmod(gst + 360.0, 360.0);
	gst *= 24.0 / 360.0;
	return gst;
}

double indigo_mean_gst(const time_t *utc) {
	if (utc) {
		return indigo_jd_to_mean_gst(UT2JD(*utc));
	} else {
		return indigo_jd_to_mean_gst(UT2JD(time(NULL)));
	}
}

double indigo_time_to_transit(const double ra, const double lmst, const bool allow_negative_time) {
	double diff = ra - lmst;
	if (allow_negative_time) {
		// normalize to [-6h, +6h] range to handle both meridian crossings
		while (diff > 6.0) diff -= 12.0;
		while (diff < -6.0) diff += 12.0;
	} else {
		diff = fmod(diff + 24.0, 24.0);
		// normalize to [0h,12h] range to handle both meridian crossings
		if (diff > 12.0) {
			diff -= 12.0;
		}
	}
	// sidereal to solar time
	return diff / 1.0027379093508;
}

void indigo_raise_set(
		const double jd,
		const double latitude,
		const double longitude,
		const double ra,
		const double dec,
		double *raise_time,
		double *transit_time,
		double *set_time
	) {
	// use mathematical horizon + refraction
	const double h0 = -0.5667 * DEG2RAD;

	double dec_rad = dec * DEG2RAD;
	double latitude_rad = latitude * DEG2RAD;

	double gst = indigo_jd_to_mean_gst(jd) * 15;

	// transit base in solar time
	double transit_base = jd + 0.5 - floor(jd + 0.5);

	// transit offset in sidereal time
	double transit_offset = (ra * 15 - longitude - gst);

	//indigo_error("transit_offset = %f", transit_offset);

	// make sure we are working with the nearest transit
	if (transit_offset < -180) {
		transit_offset +=360;
	} else if (transit_offset > 180) {
		transit_offset -=360;
	}
	// indigo_error("normalized transit_offset = %f", transit_offset);

	// offset is in sidereal time -> convert to solar time
	transit_offset /= 360.9856473662880;

	// transit time in solar time
	double transit = fmod(transit_base + transit_offset + 1.0, 1.0);
	if (transit_time) {
		*transit_time = transit * 24;
	}

	double cosH = (sin(h0) - sin(latitude_rad) * sin(dec_rad)) / (cos(latitude_rad) * cos(dec_rad));
	if (cosH < -1) { // target never sets
		if (raise_time) {
			*raise_time = 0;
		}
		if (set_time) {
			*set_time = 24;
		}
		return;
	}
	if (cosH > 1) { // target never rises
		if (raise_time) {
			*raise_time = 0;
		}
		if (set_time) {
			*set_time = 0;
		}
		return;
	}

	// get H0 in solar time
	double H0 = acos(cosH) * RAD2DEG / 360.9856473662880;

	// calculate rise time (solar time)
	double rise = transit - H0;
	if (raise_time) {
		if (rise < 0) {
			rise += 1;
		}
		*raise_time = rise * 24;
	}

	// calculate set time (solar time)
	double set = transit + H0;
	if (set_time) {
		if (set > 1) {
			set -= 1;
		}
		*set_time = set * 24;
	}
}

double indigo_airmass(double altitude) {
	altitude *= DEG2RAD;

	double zenith_angle = M_PI / 2 - altitude;

	double sin_z = sin(zenith_angle);
	double sin_squared_z = sin_z * sin_z;

	// Calculate the airmass using the Kasten and Young formula
	double airmass = pow(1 - 0.96 * sin_squared_z, -0.5);

	return airmass;
}

double indigo_derotation_rate(double alt, double az, double latitude) {
	double derotation_rate = 15.04106858 * cos(latitude * DEG2RAD) * cos(az * DEG2RAD) / cos(alt * DEG2RAD);
	return derotation_rate;
}

double indigo_parallactic_angle(double ha, double dec, double latitude) {
	ha *= DEG2RAD;
	dec *= DEG2RAD;
	latitude *= DEG2RAD;

	double q = atan2(sin(ha), tan(latitude) * cos(dec) - sin(dec) * cos(ha));
	return q * RAD2DEG;
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
	*alt = h_point.d * RAD2DEG;
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
	const double search_radius = PA_MAX_ERROR;
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
