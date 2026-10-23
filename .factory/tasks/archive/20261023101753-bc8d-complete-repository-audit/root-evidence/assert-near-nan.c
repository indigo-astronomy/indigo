#include "indigo_test/test_runner.h"
static void invalid_near(void) { ASSERT_NEAR(1.0, NAN, 0.001); }
int main(void) { invalid_near(); printf("failures=%d\n", indigo_test_failures); return indigo_test_failures ? 1 : 0; }
