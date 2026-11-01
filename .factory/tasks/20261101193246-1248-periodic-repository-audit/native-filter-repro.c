#include "indigo_test/test_runner.h"

static void must_fail(void) {
	ASSERT_TRUE(false);
}

int main(void) {
	indigo_test_case cases[] = { { "must_fail", must_fail } };
	return indigo_run_tests("Audit filter", cases, 1);
}
