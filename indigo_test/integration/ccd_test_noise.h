// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// Use under the INDIGO Astronomy open-source license (see LICENSE.md).

#ifndef CCD_TEST_NOISE_H
#define CCD_TEST_NOISE_H

#include <stdint.h>

// Coordinate/channel-addressable noise keeps ROI assertions reproducible.
static uint16_t ccd_test_noise(unsigned pixel, unsigned channel) {
	uint32_t value = pixel ^ (0x9e3779b9u * (channel + 1));
	value ^= value >> 16;
	value *= 0x7feb352du;
	value ^= value >> 15;
	value *= 0x846ca68bu;
	value ^= value >> 16;
	return (uint16_t)value;
}

#endif
