// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo/indigo_aux_driver.h>

#include "../test_runner.h"

static void dewpoint_matches_magnus_reference_values(void) {
	ASSERT_NEAR(9.261, indigo_aux_dewpoint(20, 50), 0.001);
	ASSERT_NEAR(17.964, indigo_aux_dewpoint(25, 65), 0.001);
	ASSERT_NEAR(0, indigo_aux_dewpoint(0, 100), 0.001);
}

static void sky_bortle_thresholds_are_stable(void) {
	ASSERT_NEAR(1, indigo_aux_sky_bortle(22.0), 0.001);
	ASSERT_NEAR(2, indigo_aux_sky_bortle(21.7), 0.001);
	ASSERT_NEAR(4.5, indigo_aux_sky_bortle(20.5), 0.001);
	ASSERT_NEAR(9, indigo_aux_sky_bortle(16.5), 0.001);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "dewpoint_matches_magnus_reference_values", dewpoint_matches_magnus_reference_values },
		{ "sky_bortle_thresholds_are_stable", sky_bortle_thresholds_are_stable }
	};
	return indigo_run_tests("aux math unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
