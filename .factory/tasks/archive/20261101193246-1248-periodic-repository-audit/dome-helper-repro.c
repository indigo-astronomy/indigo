#include "indigo_test/hardware/dome_hw_common.h"

int main(void) {
	return dome_hw_at(0, 90, 0.1) ? 0 : 1;
}
