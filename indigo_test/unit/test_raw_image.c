// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <string.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>

#include "../test_runner.h"

static void raw_header_type_values_are_stable(void) {
	ASSERT_EQ_INT(0x31574152, INDIGO_RAW_MONO8);
	ASSERT_EQ_INT(0x32574152, INDIGO_RAW_MONO16);
	ASSERT_EQ_INT(0x33574152, INDIGO_RAW_RGB24);
	ASSERT_EQ_INT(0x36424752, INDIGO_RAW_RGBA32);
	ASSERT_EQ_INT(0x36524742, INDIGO_RAW_ABGR32);
	ASSERT_EQ_INT(0x36574152, INDIGO_RAW_RGB48);
}

static void mono_raw_with_bayer_extension_is_detected(void) {
	struct {
		indigo_raw_header header;
		unsigned char pixels[4];
		char extension[32];
	} image;

	memset(&image, 0, sizeof(image));
	image.header.signature = INDIGO_RAW_MONO8;
	image.header.width = 2;
	image.header.height = 2;
	strcpy(image.extension, "BAYERPAT=RGGB");

	ASSERT_TRUE(indigo_is_bayered_image(&image.header, sizeof(image.header) + sizeof(image.pixels) + strlen(image.extension) + 1));
}

static void raw_without_bayer_extension_is_not_bayered(void) {
	struct {
		indigo_raw_header header;
		unsigned char pixels[4];
		char extension[16];
	} image;

	memset(&image, 0, sizeof(image));
	image.header.signature = INDIGO_RAW_MONO8;
	image.header.width = 2;
	image.header.height = 2;
	strcpy(image.extension, "NO_BAYER");

	ASSERT_FALSE(indigo_is_bayered_image(&image.header, sizeof(image.header) + sizeof(image.pixels) + strlen(image.extension) + 1));
}

static void color_raw_is_not_treated_as_bayered(void) {
	struct {
		indigo_raw_header header;
		unsigned char pixels[12];
		char extension[32];
	} image;

	memset(&image, 0, sizeof(image));
	image.header.signature = INDIGO_RAW_RGB24;
	image.header.width = 2;
	image.header.height = 2;
	strcpy(image.extension, "BAYERPAT=RGGB");

	ASSERT_FALSE(indigo_is_bayered_image(&image.header, sizeof(image)));
}

static void bayer_channel_equalization_balances_simple_mono8_frame(void) {
	uint8_t data[] = {
		10, 20,
		30, 40
	};

	ASSERT_EQ_INT(INDIGO_OK, indigo_equalize_bayer_channels(INDIGO_RAW_MONO8, data, 2, 2));
	ASSERT_EQ_INT(25, data[0]);
	ASSERT_EQ_INT(25, data[1]);
	ASSERT_EQ_INT(25, data[2]);
	ASSERT_EQ_INT(25, data[3]);
}

static void saturation_mask_marks_saturated_feature(void) {
	uint8_t data[25];
	uint8_t mask[25];
	memset(data, 10, sizeof(data));
	memset(mask, 0, sizeof(mask));
	data[11] = 255;
	data[12] = 255;
	data[13] = 255;

	ASSERT_EQ_INT(INDIGO_OK, indigo_update_saturation_mask(INDIGO_RAW_MONO8, data, 5, 5, mask));
	ASSERT_EQ_INT(0, mask[12]);
	ASSERT_EQ_INT(1, mask[0]);
}

static void contrast_for_constant_frame_is_zero_and_not_saturated(void) {
	uint8_t data[25];
	bool saturated = true;
	memset(data, 42, sizeof(data));

	ASSERT_NEAR(0, indigo_contrast(INDIGO_RAW_MONO8, data, NULL, 5, 5, &saturated), 1e-12);
	ASSERT_FALSE(saturated);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "raw_header_type_values_are_stable", raw_header_type_values_are_stable },
		{ "mono_raw_with_bayer_extension_is_detected", mono_raw_with_bayer_extension_is_detected },
		{ "raw_without_bayer_extension_is_not_bayered", raw_without_bayer_extension_is_not_bayered },
		{ "color_raw_is_not_treated_as_bayered", color_raw_is_not_treated_as_bayered },
		{ "bayer_channel_equalization_balances_simple_mono8_frame", bayer_channel_equalization_balances_simple_mono8_frame },
		{ "saturation_mask_marks_saturated_feature", saturation_mask_marks_saturated_feature },
		{ "contrast_for_constant_frame_is_zero_and_not_saturated", contrast_for_constant_frame_is_zero_and_not_saturated }
	};
	return indigo_run_tests("raw image unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
