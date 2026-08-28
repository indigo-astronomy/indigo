// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <math.h>

#include <indigo/indigo_align.h>

#include "../test_runner.h"

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

/* Hour angle advance in degrees per second of time. */
#define SIDEREAL_DEG_PER_SEC (360.0 / 86164.0905)

/* INDIGO azimuth, measured from North, the same formula as indigo_equatorial_to_hotizontal(). */
static void horizontal(double ha, double dec, double latitude, double *alt, double *az) {
	double h = ha * DEG2RAD;
	double d = dec * DEG2RAD;
	double p = latitude * DEG2RAD;
	*alt = asin(sin(d) * sin(p) + cos(d) * cos(p) * cos(h)) * RAD2DEG;
	*az = atan2(-cos(d) * sin(h), sin(d) * cos(p) - cos(d) * sin(p) * cos(h)) * RAD2DEG;
	if (*az < 0) {
		*az += 360;
	}
}

/* Rate of change of the parallactic angle over one second of time, in arcseconds per second. */
static double measured_rate(double ha, double dec, double latitude) {
	double before = indigo_parallactic_angle(ha - SIDEREAL_DEG_PER_SEC / 2, dec, latitude);
	double after = indigo_parallactic_angle(ha + SIDEREAL_DEG_PER_SEC / 2, dec, latitude);
	double difference = fmod(after - before + 540, 360) - 180;
	return difference * 3600;
}

/* Reference values cross checked against the IAU SOFA/ERFA routine eraHd2pa(). */
static void parallactic_angle_matches_reference_values(void) {
	ASSERT_NEAR(37.847472, indigo_parallactic_angle(30, 20, 45), 1e-5);
	ASSERT_NEAR(-39.361704, indigo_parallactic_angle(-45, 10, 45), 1e-5);
	ASSERT_NEAR(132.915440, indigo_parallactic_angle(15, 60, 50), 1e-5);
	ASSERT_NEAR(116.893353, indigo_parallactic_angle(60, -20, -33), 1e-5);
	ASSERT_NEAR(41.773513, indigo_parallactic_angle(120, 80, 70), 1e-5);
	ASSERT_NEAR(-114.028845, indigo_parallactic_angle(-100, -60, -30), 1e-5);
}

static void parallactic_angle_is_zero_on_the_meridian(void) {
	/* South of the zenith the object culminates upright, north of it upside down. */
	ASSERT_NEAR(0, indigo_parallactic_angle(0, 20, 45), 1e-9);
	ASSERT_NEAR(180, fabs(indigo_parallactic_angle(0, 70, 45)), 1e-9);
	/* East of the meridian the angle is positive, west of it negative. */
	ASSERT_TRUE(indigo_parallactic_angle(-30, 20, 45) < 0);
	ASSERT_TRUE(indigo_parallactic_angle(30, 20, 45) > 0);
}

/* The derotation rate has to be the rate at which the parallactic angle changes, because the mount
   agent derotates by keeping the rotator at the parallactic angle. A sign or scale error in either
   function breaks this. */
static void derotation_rate_equals_parallactic_angle_rate(void) {
	const double cases[][3] = {
		{ 30, 20, 45 }, { -45, 10, 45 }, { 15, 60, 50 }, { 60, -20, -33 },
		{ 120, 80, 70 }, { -100, -60, -30 }, { -2, 10, 45 }, { 45, -5, 20 }
	};
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double ha = cases[i][0], dec = cases[i][1], latitude = cases[i][2];
		double alt, az;
		horizontal(ha, dec, latitude, &alt, &az);
		ASSERT_NEAR(measured_rate(ha, dec, latitude), indigo_derotation_rate(alt, az, latitude), 1e-3);
	}
}

static void derotation_rate_matches_reference_values(void) {
	double alt, az;
	/* Latitude 45 N, declination +20, two hours east and two hours west of the meridian. */
	horizontal(-30, 20, 45, &alt, &az);
	ASSERT_NEAR(10.685434, indigo_derotation_rate(alt, az, 45), 1e-5);
	horizontal(30, 20, 45, &alt, &az);
	ASSERT_NEAR(10.685434, indigo_derotation_rate(alt, az, 45), 1e-5);
	/* Rotation is fastest on the meridian and slows down towards the horizon. */
	horizontal(-60, 20, 45, &alt, &az);
	double low = indigo_derotation_rate(alt, az, 45);
	horizontal(-15, 20, 45, &alt, &az);
	double high = indigo_derotation_rate(alt, az, 45);
	ASSERT_TRUE(high > low);
	ASSERT_TRUE(low > 0);
}

static void derotation_rate_vanishes_due_east_and_west(void) {
	/* cos(azimuth) is zero there, no field rotation regardless of altitude or latitude. */
	ASSERT_NEAR(0, indigo_derotation_rate(0, 90, 45), 1e-9);
	ASSERT_NEAR(0, indigo_derotation_rate(30, 270, 45), 1e-9);
	/* An observer on the equator sees no field rotation on the meridian either. */
	ASSERT_NEAR(0, indigo_derotation_rate(45, 180, 90), 1e-9);
}

static void derotation_rate_grows_towards_the_zenith(void) {
	/* The rate is unbounded at the zenith, the caller has to cope with large values. */
	double alt, az;
	/* 0.15 degrees of hour angle short of a transit that passes 0.1 degrees from the zenith */
	horizontal(-0.15, 44.9, 45, &alt, &az);
	double near_zenith = fabs(indigo_derotation_rate(alt, az, 45));
	ASSERT_TRUE(alt > 89.5);
	ASSERT_TRUE(near_zenith > 1000);
	ASSERT_NEAR(measured_rate(-0.15, 44.9, 45), indigo_derotation_rate(alt, az, 45), 1);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "parallactic_angle_matches_reference_values", parallactic_angle_matches_reference_values },
		{ "parallactic_angle_is_zero_on_the_meridian", parallactic_angle_is_zero_on_the_meridian },
		{ "derotation_rate_equals_parallactic_angle_rate", derotation_rate_equals_parallactic_angle_rate },
		{ "derotation_rate_matches_reference_values", derotation_rate_matches_reference_values },
		{ "derotation_rate_vanishes_due_east_and_west", derotation_rate_vanishes_due_east_and_west },
		{ "derotation_rate_grows_towards_the_zenith", derotation_rate_grows_towards_the_zenith }
	};
	return indigo_run_tests("align math unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
