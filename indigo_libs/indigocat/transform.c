/*
 *  based on Liam Girdwood's code
 */

#include <math.h>
#include <transform.h>
#include <nutation.h>

void heliocentric_to_cartesian_coords(heliocentric_coords_s *object, cartesian_coords_s * position) {
	double sin_e, cos_e;
	double cos_B, sin_B, sin_L, cos_L;

	/* ecliptic J2000 */
	sin_e = 0.397777156;
	cos_e = 0.917482062;

	/* calc common values */
	cos_B = cos(DEG2RAD * object->B);
	cos_L = cos(DEG2RAD * object->L);
	sin_B = sin(DEG2RAD * object->B);
	sin_L = sin(DEG2RAD * object->L);

	position->X = object->R * cos_L * cos_B;
	position->Y = object->R * (sin_L * cos_B * cos_e - sin_B * sin_e);
	position->Z = object->R * (sin_L * cos_B * sin_e + sin_B * cos_e);
}

void ecliptical_to_equatorial_coords(lonlat_coords_s * object, double JD, equatorial_coords_s * position)
{
	double ra, declination, longitude, latitude;
	nutation_s nutation;

	get_nutation (JD, &nutation);
	nutation.ecliptic = DEG2RAD * nutation.ecliptic;

	longitude = DEG2RAD * object->lon;
	latitude = DEG2RAD * object->lat;

	ra = atan2 ((sin(longitude) * cos(nutation.ecliptic) - tan(latitude) * sin(nutation.ecliptic)), cos(longitude));
	declination = sin(latitude) * cos(nutation.ecliptic) + cos(latitude) * sin(nutation.ecliptic) * sin(longitude);
	declination = asin(declination);

	position->ra = range_degrees(RAD2DEG * ra);
	position->dec = RAD2DEG * declination;
}

/* puts a large angle in the correct range 0 - 360 degrees */
double range_degrees(double angle)
{
	double temp;

	if (angle >= 0.0 && angle < 360.0)
		return angle;

	temp = (int)(angle / 360);
	if (angle < 0.0)
		temp --;
	temp *= 360;
	return angle - temp;
}

double range_radians2(double angle) {
	double temp;

	if (angle > (-2.0 * M_PI) && angle < (2.0 * M_PI))
		return angle;

	temp = (int)(angle / (M_PI * 2.0));
	temp *= (M_PI * 2.0);
	return angle - temp;
}
