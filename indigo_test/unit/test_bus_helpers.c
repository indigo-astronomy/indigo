// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <string.h>

#include <indigo/indigo_bus.h>

#include "../test_runner.h"

static void numeric_string_helpers_parse_and_format_values(void) {
	char buffer[64] = { 0 };

	ASSERT_NEAR(123.45, indigo_atod("123.45"), 1e-12);
	ASSERT_NEAR(123.45, indigo_atod("123,45"), 1e-12);
	ASSERT_NEAR(-1200, indigo_atod("-1.2e3"), 1e-12);
	ASSERT_STREQ("123.456", indigo_dtoa(123.456, buffer));
}

static void sexagesimal_helpers_parse_and_format_values(void) {
	ASSERT_NEAR(12.5, indigo_stod("12:30:00"), 1e-12);
	ASSERT_NEAR(-0.5, indigo_stod("-0:30:00"), 1e-12);
	ASSERT_STREQ("12:30:00.00", indigo_dtos(12.5, "%d:%02d:%05.2f"));
	ASSERT_STREQ("-12:30:00.00", indigo_dtos(-12.5, "%d:%02d:%05.2f"));
}

static void pixel_scale_and_local_service_helpers_are_deterministic(void) {
	char name[INDIGO_NAME_SIZE] = "CCD Simulator @ Local Service";
	INDIGO_COPY_NAME(indigo_local_service_name, "Local Service");

	ASSERT_NEAR(2.06265, indigo_pixel_scale(100, 10), 1e-12);
	ASSERT_NEAR(0, indigo_pixel_scale(0, 10), 1e-12);

	indigo_trim_local_service(name);
	ASSERT_STREQ("CCD Simulator", name);
}

static void switch_helpers_find_and_update_items(void) {
	indigo_property *property = indigo_init_switch_property(NULL, "Unit Device", "UNIT_SWITCH", "Unit", "Unit switch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	ASSERT_TRUE(property != NULL);
	indigo_init_switch_item(property->items + 0, "A", "A", true);
	indigo_init_switch_item(property->items + 1, "B", "B", false);

	ASSERT_TRUE(indigo_get_item(property, "B") == property->items + 1);
	ASSERT_TRUE(indigo_get_switch(property, "A"));

	indigo_set_switch(property, property->items + 1, true);
	ASSERT_FALSE(property->items[0].sw.value);
	ASSERT_TRUE(property->items[1].sw.value);

	indigo_release_property(property);
}

static void copy_values_and_targets_clamp_numbers(void) {
	indigo_property *target = indigo_init_number_property(NULL, "Unit Device", "UNIT_NUMBER", "Unit", "Unit number", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_property *source = indigo_init_number_property(NULL, "Unit Device", "UNIT_NUMBER", "Unit", "Unit number", INDIGO_ALERT_STATE, INDIGO_RW_PERM, 1);
	ASSERT_TRUE(target != NULL);
	ASSERT_TRUE(source != NULL);
	indigo_init_number_item(target->items, "VALUE", "Value", 0, 10, 1, 5);
	indigo_init_number_item(source->items, "VALUE", "Value", -100, 100, 1, 15);

	indigo_property_copy_values(target, source, true);
	ASSERT_EQ_INT(INDIGO_ALERT_STATE, target->state);
	ASSERT_NEAR(10, target->items[0].number.value, 1e-12);
	ASSERT_NEAR(10, target->items[0].number.target, 1e-12);

	source->items[0].number.value = -5;
	indigo_property_copy_targets(target, source, false);
	ASSERT_NEAR(10, target->items[0].number.value, 1e-12);
	ASSERT_NEAR(0, target->items[0].number.target, 1e-12);

	indigo_release_property(source);
	indigo_release_property(target);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "numeric_string_helpers_parse_and_format_values", numeric_string_helpers_parse_and_format_values },
		{ "sexagesimal_helpers_parse_and_format_values", sexagesimal_helpers_parse_and_format_values },
		{ "pixel_scale_and_local_service_helpers_are_deterministic", pixel_scale_and_local_service_helpers_are_deterministic },
		{ "switch_helpers_find_and_update_items", switch_helpers_find_and_update_items },
		{ "copy_values_and_targets_clamp_numbers", copy_values_and_targets_clamp_numbers }
	};
	return indigo_run_tests("bus helper unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

