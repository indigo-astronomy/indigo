// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#ifndef indigo_test_runner_h
#define indigo_test_runner_h

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef void (*indigo_test_function)(void);

typedef struct {
	const char *name;
	indigo_test_function function;
} indigo_test_case;

static int indigo_test_failures = 0;

#define ASSERT_TRUE(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #condition); \
		indigo_test_failures++; \
		return; \
	} \
} while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ_INT(expected, actual) do { \
	int expected_value = (expected); \
	int actual_value = (actual); \
	if (expected_value != actual_value) { \
		fprintf(stderr, "%s:%d: expected %d, got %d\n", __FILE__, __LINE__, expected_value, actual_value); \
		indigo_test_failures++; \
		return; \
	} \
} while (0)

#define ASSERT_EQ_TOKEN(expected, actual) do { \
	indigo_token expected_value = (expected); \
	indigo_token actual_value = (actual); \
	if (expected_value != actual_value) { \
		fprintf(stderr, "%s:%d: expected 0x%llx, got 0x%llx\n", __FILE__, __LINE__, expected_value, actual_value); \
		indigo_test_failures++; \
		return; \
	} \
} while (0)

#define ASSERT_NEAR(expected, actual, tolerance) do { \
	double expected_value = (expected); \
	double actual_value = (actual); \
	double tolerance_value = (tolerance); \
	if (fabs(expected_value - actual_value) > tolerance_value) { \
		fprintf(stderr, "%s:%d: expected %.12g, got %.12g, tolerance %.12g\n", __FILE__, __LINE__, expected_value, actual_value, tolerance_value); \
		indigo_test_failures++; \
		return; \
	} \
} while (0)

#define ASSERT_STREQ(expected, actual) do { \
	const char *expected_value = (expected); \
	const char *actual_value = (actual); \
	if (strcmp(expected_value, actual_value)) { \
		fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, expected_value, actual_value); \
		indigo_test_failures++; \
		return; \
	} \
} while (0)

static int indigo_run_tests(const char *suite_name, const indigo_test_case *tests, int count) {
	int initial_failures = indigo_test_failures;
	printf("Running %s\n", suite_name);
	for (int i = 0; i < count; i++) {
		int before = indigo_test_failures;
		tests[i].function();
		if (indigo_test_failures == before) {
			printf("  PASS %s\n", tests[i].name);
		} else {
			printf("  FAIL %s\n", tests[i].name);
		}
	}
	if (indigo_test_failures == initial_failures) {
		printf("%s: all tests passed\n", suite_name);
		return 0;
	}
	printf("%s: %d test(s) failed\n", suite_name, indigo_test_failures - initial_failures);
	return 1;
}

#endif /* indigo_test_runner_h */
