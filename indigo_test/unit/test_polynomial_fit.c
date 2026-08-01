// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <string.h>

#include <indigo/indigo_polynomial_fit.h>

#include "../test_runner.h"

static void polynomial_value_and_derivative_are_correct(void) {
	double coefficients[] = { 2, 3, 4 };
	double derivative[2] = { 0 };

	ASSERT_NEAR(24, indigo_polynomial_value(2, 3, coefficients), 1e-12);
	indigo_polynomial_derivative(3, coefficients, derivative);
	ASSERT_NEAR(3, derivative[0], 1e-12);
	ASSERT_NEAR(8, derivative[1], 1e-12);
}

static void polynomial_extremum_and_minimum_are_correct_for_quadratic(void) {
	double coefficients[] = { 9, -6, 1 };
	double extremums[2] = { 0 };

	ASSERT_EQ_INT(0, indigo_polynomial_extremums(3, coefficients, extremums));
	ASSERT_NEAR(3, extremums[0], 1e-12);
	ASSERT_NEAR(3, indigo_polynomial_min_x(3, coefficients, -10, 10, 1e-6), 1e-5);
}

static void polynomial_fit_recovers_exact_line(void) {
	double x[] = { 0, 1, 2, 3 };
	double y[] = { 2, 5, 8, 11 };
	double coefficients[2] = { 0 };

	ASSERT_EQ_INT(0, indigo_polynomial_fit(4, x, y, 2, coefficients));
	ASSERT_NEAR(2, coefficients[0], 1e-9);
	ASSERT_NEAR(3, coefficients[1], 1e-9);
}

static void polynomial_string_contains_terms(void) {
	double coefficients[] = { 2, -3, 4 };
	char buffer[256] = { 0 };

	indigo_polynomial_string(3, coefficients, buffer);

	ASSERT_TRUE(strstr(buffer, "y =") != NULL);
	ASSERT_TRUE(strstr(buffer, "*x^2") != NULL);
	ASSERT_TRUE(strstr(buffer, "*x") != NULL);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "polynomial_value_and_derivative_are_correct", polynomial_value_and_derivative_are_correct },
		{ "polynomial_extremum_and_minimum_are_correct_for_quadratic", polynomial_extremum_and_minimum_are_correct_for_quadratic },
		{ "polynomial_fit_recovers_exact_line", polynomial_fit_recovers_exact_line },
		{ "polynomial_string_contains_terms", polynomial_string_contains_terms }
	};
	return indigo_run_tests("polynomial fit unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

