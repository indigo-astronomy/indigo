// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo/indigo_dome_azimuth.h>

#include "../test_runner.h"

static void map24_wraps_hours_into_day_range(void) {
	ASSERT_NEAR(0, map24(0), 1e-12);
	ASSERT_NEAR(23.5, map24(-0.5), 1e-12);
	ASSERT_NEAR(1.25, map24(25.25), 1e-12);
	ASSERT_NEAR(23, map24(-25), 1e-12);
}

static void azimuth_distance_uses_shortest_arc(void) {
	ASSERT_NEAR(20, indigo_azimuth_distance(10, 350), 1e-12);
	ASSERT_NEAR(180, indigo_azimuth_distance(0, 180), 1e-12);
	ASSERT_NEAR(5, indigo_azimuth_distance(42, 47), 1e-12);
}

static void dome_azimuth_solution_stays_in_valid_range(void) {
	double north = indigo_dome_solve_azimuth(2.0, 45.0, 48.0, 1.75, 0.2, 0.6, 0.1, -0.1);
	double south = indigo_dome_solve_azimuth(22.0, -35.0, -38.0, 1.75, 0.2, 0.6, -0.1, 0.1);

	ASSERT_TRUE(north >= 0);
	ASSERT_TRUE(north < 360);
	ASSERT_TRUE(south >= 0);
	ASSERT_TRUE(south < 360);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "map24_wraps_hours_into_day_range", map24_wraps_hours_into_day_range },
		{ "azimuth_distance_uses_shortest_arc", azimuth_distance_uses_shortest_arc },
		{ "dome_azimuth_solution_stays_in_valid_range", dome_azimuth_solution_stays_in_valid_range }
	};
	return indigo_run_tests("dome azimuth unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

