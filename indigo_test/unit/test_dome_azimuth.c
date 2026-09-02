// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo/indigo_dome_azimuth.h>

#include "../test_runner.h"

/* map360() is private to the solver, the tests need their own copy */
static double normalize360(double angle) {
	while (angle < 0) {
		angle += 360;
	}
	while (angle >= 360) {
		angle -= 360;
	}
	return angle;
}

static void map24_wraps_hours_into_day_range(void) {
	ASSERT_NEAR(0, indigo_map24(0), 1e-12);
	ASSERT_NEAR(23.5, indigo_map24(-0.5), 1e-12);
	ASSERT_NEAR(1.25, indigo_map24(25.25), 1e-12);
	ASSERT_NEAR(23, indigo_map24(-25), 1e-12);
}

static void azimuth_distance_uses_shortest_arc(void) {
	ASSERT_NEAR(20, indigo_azimuth_distance(10, 350), 1e-12);
	ASSERT_NEAR(180, indigo_azimuth_distance(0, 180), 1e-12);
	ASSERT_NEAR(5, indigo_azimuth_distance(42, 47), 1e-12);
}

static void dome_azimuth_solution_stays_in_valid_range(void) {
	double north = indigo_dome_solve_azimuth(2.0, 45.0, 48.0, 1.75, 0.2, 0.6, 0.1, -0.1, 0);
	double south = indigo_dome_solve_azimuth(22.0, -35.0, -38.0, 1.75, 0.2, 0.6, -0.1, 0.1, 0);

	ASSERT_TRUE(north >= 0);
	ASSERT_TRUE(north < 360);
	ASSERT_TRUE(south >= 0);
	ASSERT_TRUE(south < 360);
}

/* The pivot offsets are given in absolute compass terms, so a site mirrored
   across the equator must give the mirrored azimuth for the same geometry. */
static void dome_azimuth_mirrors_pivot_offsets_across_the_equator(void) {
	for (double ha = -6; ha <= 6; ha += 1.5) {
		double north = indigo_dome_solve_azimuth(ha, 20, 45, 3, 0.3, 0.6, 0.2, -0.3, 0);
		double south = indigo_dome_solve_azimuth(ha, -20, -45, 3, 0.3, 0.6, -0.2, -0.3, 0);
		double mirrored = normalize360(180 - north);

		printf("  ha=%+.1f north=%.4f south=%.4f mirrored=%.4f\n", ha, north, south, mirrored);
		ASSERT_NEAR(mirrored, south, 1e-9);
	}
}

/* An independent reference for the same geometry, derived as vectors in the
   horizontal frame instead of from closed form angles, and the same model the
   ain_imager dome view draws. The declination axis is the cross product of the
   polar axis and the line of sight, the OTA sits at one of its two ends, and
   the dome azimuth is where the line of sight leaves the sphere of the dome.
   Kept deliberately independent of indigo_dome_solve_azimuth() so that the two
   have to agree on the geometry rather than on a shared formula. */
typedef struct { double x, y, z; } reference_vector;

static double reference_dome_azimuth(double ha, double dec, double latitude, double dome_radius, double mount_dec_height, double mount_dec_length, double mount_dec_offset_NS, double mount_dec_offset_EW, int side_of_pier) {
	double hour_angle = ha * 15 * M_PI / 180.0;
	double declination = dec * M_PI / 180.0;
	double phi = latitude * M_PI / 180.0;
	double altitude = asin(sin(phi) * sin(declination) + cos(phi) * cos(declination) * cos(hour_angle));
	double azimuth = atan2(-cos(declination) * sin(hour_angle), sin(declination) * cos(phi) - sin(phi) * cos(declination) * cos(hour_angle));

	/* x east, y north, z up, in both hemispheres */
	reference_vector sight = { cos(altitude) * sin(azimuth), cos(altitude) * cos(azimuth), sin(altitude) };
	reference_vector ota = { mount_dec_offset_EW, mount_dec_offset_NS, mount_dec_height };

	if (mount_dec_length > 0) {
		reference_vector pole = { 0, cos(phi), sin(phi) };
		reference_vector axis = {
			pole.y * sight.z - pole.z * sight.y,
			pole.z * sight.x - pole.x * sight.z,
			pole.x * sight.y - pole.y * sight.x
		};
		double norm = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
		if (norm < 1e-9) {
			axis.x = cos(azimuth);
			axis.y = -sin(azimuth);
			axis.z = 0;
			norm = 1;
		}
		axis.x /= norm;
		axis.y /= norm;
		axis.z /= norm;
		double end;
		if (side_of_pier != 0 && fabs(axis.x) > 1e-6) {
			end = ((axis.x > 0) == (side_of_pier < 0)) ? 1.0 : -1.0;
		} else {
			end = (axis.z < -1e-9) ? -1.0 : 1.0;
		}
		ota.x += end * mount_dec_length * axis.x;
		ota.y += end * mount_dec_length * axis.y;
		ota.z += end * mount_dec_length * axis.z;
	}

	double ota_radius = sqrt(ota.x * ota.x + ota.y * ota.y + ota.z * ota.z);
	if (ota_radius >= dome_radius) {
		return normalize360(azimuth * 180.0 / M_PI);
	}
	double b = ota.x * sight.x + ota.y * sight.y + ota.z * sight.z;
	double distance = -b + sqrt(b * b - (ota_radius * ota_radius - dome_radius * dome_radius));
	return normalize360(atan2(ota.x + distance * sight.x, ota.y + distance * sight.y) * 180.0 / M_PI);
}

static void dome_azimuth_matches_the_vector_model(void) {
	static const int sides[] = { -1, 0, 1 };
	double worst = 0;
	int cases = 0;

	for (int s = 0; s < 3; s++) {
		for (double latitude = -75; latitude <= 75; latitude += 7.5) {
			if (fabs(latitude) < 1e-9) {
				continue;
			}
			for (double ha = -11.9; ha < 12; ha += 0.35) {
				for (double dec = -85; dec <= 85; dec += 17) {
					for (double length = 0; length <= 0.9; length += 0.3) {
						double solved = indigo_dome_solve_azimuth(ha, dec, latitude, 2.5, 0.15, length, 0.12, -0.08, sides[s]);
						double reference = reference_dome_azimuth(ha, dec, latitude, 2.5, 0.15, length, 0.12, -0.08, sides[s]);
						double error = indigo_azimuth_distance(solved, reference);
						if (error > worst) {
							worst = error;
						}
						cases++;
					}
				}
			}
		}
	}
	printf("  %d cases, worst disagreement %.9f deg\n", cases, worst);
	ASSERT_TRUE(worst < 1e-9);
}

/* The OTA is carried at the end of the declination axis, so it is only east of
   the pier while the hour angle is within 6 hours of the meridian. Past that
   the same mechanical state carries it west, and the reported side has to pick
   the other end of the axis to stay on the side the mount reports. */
static void dome_azimuth_keeps_the_ota_on_the_reported_side(void) {
	for (double ha = -11.5; ha < 12; ha += 0.5) {
		double east = indigo_dome_solve_azimuth(ha, 15, 45, 2.5, 0, 0.5, 0, 0, -1);
		double west = indigo_dome_solve_azimuth(ha, 15, 45, 2.5, 0, 0.5, 0, 0, 1);
		double east_reference = reference_dome_azimuth(ha, 15, 45, 2.5, 0, 0.5, 0, 0, -1);
		double west_reference = reference_dome_azimuth(ha, 15, 45, 2.5, 0, 0.5, 0, 0, 1);

		printf("  ha=%+5.1f east=%8.3f west=%8.3f\n", ha, east, west);
		ASSERT_NEAR(east_reference, east, 1e-9);
		ASSERT_NEAR(west_reference, west, 1e-9);
	}
}

/* Before the transit a counterweight down mount has the OTA west of the pier
   and reports WEST, and it keeps reporting WEST while it tracks past the
   meridian without flipping, so holding WEST across the transit must not make
   the dome azimuth jump the way the counterweight down assumption does. */
static void dome_azimuth_follows_reported_side_of_pier_past_the_meridian(void) {
	double before = indigo_dome_solve_azimuth(-0.02, 20, 45, 2.5, 0, 0.4, 0, 0, 1);
	double after = indigo_dome_solve_azimuth(0.02, 20, 45, 2.5, 0, 0.4, 0, 0, 1);
	double guessed = indigo_dome_solve_azimuth(0.02, 20, 45, 2.5, 0, 0.4, 0, 0, 0);

	printf("  before=%.4f after=%.4f guessed=%.4f\n", before, after, guessed);
	ASSERT_TRUE(indigo_azimuth_distance(before, after) < 2);
	ASSERT_TRUE(indigo_azimuth_distance(before, guessed) > 30);
}

/* A counterweight down mount reports the side its OTA is actually on, so the
   reported value must not move the dome away from that assumption. */
static void dome_azimuth_agrees_with_counterweight_down_for_a_normal_mount(void) {
	for (double ha = -11.5; ha < 12; ha += 0.5) {
		double unknown = indigo_dome_solve_azimuth(ha, 20, 45, 2.5, 0, 0.4, 0, 0, 0);
		double east = indigo_dome_solve_azimuth(ha, 20, 45, 2.5, 0, 0.4, 0, 0, -1);
		double west = indigo_dome_solve_azimuth(ha, 20, 45, 2.5, 0, 0.4, 0, 0, 1);
		int matches_east = indigo_azimuth_distance(unknown, east) < 1e-9;
		int matches_west = indigo_azimuth_distance(unknown, west) < 1e-9;

		printf("  ha=%+5.1f unknown=%8.3f matches %s\n", ha, unknown, matches_east ? "EAST" : (matches_west ? "WEST" : "NEITHER"));
		ASSERT_TRUE(matches_east || matches_west);
	}
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "map24_wraps_hours_into_day_range", map24_wraps_hours_into_day_range },
		{ "azimuth_distance_uses_shortest_arc", azimuth_distance_uses_shortest_arc },
		{ "dome_azimuth_solution_stays_in_valid_range", dome_azimuth_solution_stays_in_valid_range },
		{ "dome_azimuth_mirrors_pivot_offsets_across_the_equator", dome_azimuth_mirrors_pivot_offsets_across_the_equator },
		{ "dome_azimuth_matches_the_vector_model", dome_azimuth_matches_the_vector_model },
		{ "dome_azimuth_keeps_the_ota_on_the_reported_side", dome_azimuth_keeps_the_ota_on_the_reported_side },
		{ "dome_azimuth_follows_reported_side_of_pier_past_the_meridian", dome_azimuth_follows_reported_side_of_pier_past_the_meridian },
		{ "dome_azimuth_agrees_with_counterweight_down_for_a_normal_mount", dome_azimuth_agrees_with_counterweight_down_for_a_normal_mount }
	};
	return indigo_run_tests("dome azimuth unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

