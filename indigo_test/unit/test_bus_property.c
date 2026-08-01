// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <string.h>

#include <indigo/indigo_bus.h>

#include "../test_runner.h"

typedef enum {
	TEST_TEXT_PROPERTY,
	TEST_NUMBER_PROPERTY,
	TEST_SWITCH_PROPERTY,
	TEST_LIGHT_PROPERTY,
	TEST_BLOB_PROPERTY
} test_property_type;

static const test_property_type all_property_types[] = {
	TEST_TEXT_PROPERTY,
	TEST_NUMBER_PROPERTY,
	TEST_SWITCH_PROPERTY,
	TEST_LIGHT_PROPERTY,
	TEST_BLOB_PROPERTY
};

static const char *property_name_for_type(test_property_type type) {
	switch (type) {
	case TEST_TEXT_PROPERTY:
		return "UNIT_TEXT";
	case TEST_NUMBER_PROPERTY:
		return "UNIT_NUMBER";
	case TEST_SWITCH_PROPERTY:
		return "UNIT_SWITCH";
	case TEST_LIGHT_PROPERTY:
		return "UNIT_LIGHT";
	case TEST_BLOB_PROPERTY:
		return "UNIT_BLOB";
	}
	return "UNIT_UNKNOWN";
}

static indigo_property *init_test_property(test_property_type type, int count) {
	const char *name = property_name_for_type(type);
	switch (type) {
	case TEST_TEXT_PROPERTY:
		return indigo_init_text_property(NULL, "Unit Device", name, "Unit", "Unit text", INDIGO_OK_STATE, INDIGO_RW_PERM, count);
	case TEST_NUMBER_PROPERTY:
		return indigo_init_number_property(NULL, "Unit Device", name, "Unit", "Unit number", INDIGO_BUSY_STATE, INDIGO_RW_PERM, count);
	case TEST_SWITCH_PROPERTY:
		return indigo_init_switch_property(NULL, "Unit Device", name, "Unit", "Unit switch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, count);
	case TEST_LIGHT_PROPERTY:
		return indigo_init_light_property(NULL, "Unit Device", name, "Unit", "Unit light", INDIGO_ALERT_STATE, count);
	case TEST_BLOB_PROPERTY:
		return indigo_init_blob_property_p(NULL, "Unit Device", name, "Unit", "Unit blob", INDIGO_IDLE_STATE, INDIGO_RW_PERM, count);
	}
	return NULL;
}

static void init_test_item(indigo_property *property, int index) {
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		indigo_init_text_item(property->items + index, index == 0 ? "ITEM_1" : "ITEM_2", index == 0 ? "Item 1" : "Item 2", index == 0 ? "alpha" : "beta");
		break;
	case INDIGO_NUMBER_VECTOR:
		indigo_init_number_item(property->items + index, index == 0 ? "VALUE_1" : "VALUE_2", index == 0 ? "Value 1" : "Value 2", -10, 100, 0.5, index == 0 ? 42 : 84);
		break;
	case INDIGO_SWITCH_VECTOR:
		indigo_init_switch_item(property->items + index, index == 0 ? "SWITCH_1" : "SWITCH_2", index == 0 ? "Switch 1" : "Switch 2", index == 0);
		break;
	case INDIGO_LIGHT_VECTOR:
		indigo_init_light_item(property->items + index, index == 0 ? "STATUS_1" : "STATUS_2", index == 0 ? "Status 1" : "Status 2", index == 0 ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
		break;
	case INDIGO_BLOB_VECTOR:
		indigo_init_blob_item(property->items + index, index == 0 ? "IMAGE_1" : "IMAGE_2", index == 0 ? "Image 1" : "Image 2");
		break;
	}
}

static void init_test_items(indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		init_test_item(property, i);
	}
}

static indigo_property_type expected_vector_type(test_property_type type) {
	switch (type) {
	case TEST_TEXT_PROPERTY:
		return INDIGO_TEXT_VECTOR;
	case TEST_NUMBER_PROPERTY:
		return INDIGO_NUMBER_VECTOR;
	case TEST_SWITCH_PROPERTY:
		return INDIGO_SWITCH_VECTOR;
	case TEST_LIGHT_PROPERTY:
		return INDIGO_LIGHT_VECTOR;
	case TEST_BLOB_PROPERTY:
		return INDIGO_BLOB_VECTOR;
	}
	return INDIGO_TEXT_VECTOR;
}

static indigo_property_perm expected_permission(test_property_type type) {
	return type == TEST_LIGHT_PROPERTY || type == TEST_BLOB_PROPERTY ? INDIGO_RO_PERM : INDIGO_RW_PERM;
}

static void assert_initialized_item(indigo_property *property) {
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		ASSERT_STREQ("ITEM_1", property->items[0].name);
		ASSERT_STREQ("Item 1", property->items[0].label);
		ASSERT_STREQ("alpha", property->items[0].text.value);
		break;
	case INDIGO_NUMBER_VECTOR:
		ASSERT_STREQ("VALUE_1", property->items[0].name);
		ASSERT_STREQ("Value 1", property->items[0].label);
		ASSERT_STREQ("%g", property->items[0].number.format);
		ASSERT_NEAR(-10, property->items[0].number.min, 1e-12);
		ASSERT_NEAR(100, property->items[0].number.max, 1e-12);
		ASSERT_NEAR(0.5, property->items[0].number.step, 1e-12);
		ASSERT_NEAR(42, property->items[0].number.value, 1e-12);
		ASSERT_NEAR(42, property->items[0].number.target, 1e-12);
		ASSERT_NEAR(42, property->items[0].number.default_value, 1e-12);
		ASSERT_NEAR(42, property->items[0].number.previous_value, 1e-12);
		ASSERT_NEAR(42, property->items[0].number.previous_target, 1e-12);
		break;
	case INDIGO_SWITCH_VECTOR:
		ASSERT_STREQ("SWITCH_1", property->items[0].name);
		ASSERT_STREQ("Switch 1", property->items[0].label);
		ASSERT_TRUE(property->items[0].sw.value);
		ASSERT_TRUE(property->items[0].sw.default_value);
		ASSERT_TRUE(property->items[0].sw.previous_value);
		if (property->count > 1 && !strcmp(property->items[1].name, "SWITCH_2")) {
			ASSERT_FALSE(property->items[1].sw.value);
			ASSERT_FALSE(property->items[1].sw.default_value);
			ASSERT_FALSE(property->items[1].sw.previous_value);
		}
		break;
	case INDIGO_LIGHT_VECTOR:
		ASSERT_STREQ("STATUS_1", property->items[0].name);
		ASSERT_STREQ("Status 1", property->items[0].label);
		ASSERT_EQ_INT(INDIGO_OK_STATE, property->items[0].light.value);
		ASSERT_EQ_INT(INDIGO_OK_STATE, property->items[0].light.previous_value);
		if (property->count > 1 && !strcmp(property->items[1].name, "STATUS_2")) {
			ASSERT_EQ_INT(INDIGO_ALERT_STATE, property->items[1].light.value);
			ASSERT_EQ_INT(INDIGO_ALERT_STATE, property->items[1].light.previous_value);
		}
		break;
	case INDIGO_BLOB_VECTOR:
		ASSERT_STREQ("IMAGE_1", property->items[0].name);
		ASSERT_STREQ("Image 1", property->items[0].label);
		ASSERT_STREQ("", property->items[0].blob.format);
		ASSERT_STREQ("", property->items[0].blob.url);
		ASSERT_EQ_INT(0, property->items[0].blob.size);
		ASSERT_TRUE(property->items[0].blob.value == NULL);
		break;
	}
}

static void mutate_first_item(indigo_property *property) {
	switch (property->type) {
	case INDIGO_TEXT_VECTOR:
		indigo_set_text_item_value(property->items, "changed");
		break;
	case INDIGO_NUMBER_VECTOR:
		property->items[0].number.value = 7;
		break;
	case INDIGO_SWITCH_VECTOR:
		property->items[0].sw.value = false;
		break;
	case INDIGO_LIGHT_VECTOR:
		property->items[0].light.value = INDIGO_BUSY_STATE;
		break;
	case INDIGO_BLOB_VECTOR:
		strncpy(property->items[0].blob.format, ".fits", INDIGO_NAME_SIZE - 1);
		property->items[0].blob.format[INDIGO_NAME_SIZE - 1] = 0;
		property->items[0].blob.size = 128;
		break;
	}
}

static void assert_copy_kept_original_first_item(indigo_property *copy) {
	switch (copy->type) {
	case INDIGO_TEXT_VECTOR:
		ASSERT_STREQ("alpha", copy->items[0].text.value);
		break;
	case INDIGO_NUMBER_VECTOR:
		ASSERT_NEAR(42, copy->items[0].number.value, 1e-12);
		break;
	case INDIGO_SWITCH_VECTOR:
		ASSERT_TRUE(copy->items[0].sw.value);
		break;
	case INDIGO_LIGHT_VECTOR:
		ASSERT_EQ_INT(INDIGO_OK_STATE, copy->items[0].light.value);
		break;
	case INDIGO_BLOB_VECTOR:
		ASSERT_STREQ("", copy->items[0].blob.format);
		ASSERT_EQ_INT(0, copy->items[0].blob.size);
		break;
	}
}

static void properties_are_initialized_for_all_vector_types(void) {
	for (int i = 0; i < (int)(sizeof(all_property_types) / sizeof(all_property_types[0])); i++) {
		test_property_type type = all_property_types[i];
		indigo_property *property = init_test_property(type, 2);
		ASSERT_TRUE(property != NULL);
		init_test_items(property);

		ASSERT_STREQ("Unit Device", property->device);
		ASSERT_STREQ(property_name_for_type(type), property->name);
		ASSERT_STREQ("Unit", property->group);
		ASSERT_EQ_INT(expected_vector_type(type), property->type);
		ASSERT_EQ_INT(expected_permission(type), property->perm);
		ASSERT_EQ_INT(2, property->count);
		if (type == TEST_SWITCH_PROPERTY) {
			ASSERT_EQ_INT(INDIGO_ONE_OF_MANY_RULE, property->rule);
		}
		assert_initialized_item(property);

		indigo_release_property(property);
	}
}

static void property_matching_works_for_all_vector_types(void) {
	for (int i = 0; i < (int)(sizeof(all_property_types) / sizeof(all_property_types[0])); i++) {
		test_property_type type = all_property_types[i];
		indigo_property *property = init_test_property(type, 1);
		indigo_property *selector = init_test_property(type, 0);
		ASSERT_TRUE(property != NULL);
		ASSERT_TRUE(selector != NULL);
		init_test_items(property);

		ASSERT_TRUE(indigo_property_match(property, selector));
		ASSERT_FALSE(indigo_property_match_defined(property, selector));
		ASSERT_FALSE(indigo_property_match_changeable(property, selector));
		property->defined = true;
		ASSERT_TRUE(indigo_property_match_defined(property, selector));
		if (property->perm == INDIGO_RW_PERM) {
			ASSERT_TRUE(indigo_property_match_changeable(property, selector));
		} else {
			ASSERT_FALSE(indigo_property_match_changeable(property, selector));
		}

		indigo_release_property(selector);
		indigo_release_property(property);
	}
}

static void copied_properties_are_independent_for_all_vector_types(void) {
	for (int i = 0; i < (int)(sizeof(all_property_types) / sizeof(all_property_types[0])); i++) {
		test_property_type type = all_property_types[i];
		indigo_property *property = init_test_property(type, 2);
		ASSERT_TRUE(property != NULL);
		init_test_items(property);

		indigo_property *copy = indigo_copy_property(NULL, property);
		ASSERT_TRUE(copy != NULL);
		ASSERT_TRUE(copy != property);
		ASSERT_STREQ(property->device, copy->device);
		ASSERT_STREQ(property->name, copy->name);
		ASSERT_EQ_INT(property->type, copy->type);
		ASSERT_EQ_INT(property->count, copy->count);

		mutate_first_item(property);
		assert_copy_kept_original_first_item(copy);

		indigo_release_property(copy);
		indigo_release_property(property);
	}
}

static void resized_properties_preserve_items_for_all_vector_types(void) {
	for (int i = 0; i < (int)(sizeof(all_property_types) / sizeof(all_property_types[0])); i++) {
		test_property_type type = all_property_types[i];
		indigo_property *property = init_test_property(type, 1);
		ASSERT_TRUE(property != NULL);
		init_test_items(property);

		property = indigo_resize_property(property, 3);
		ASSERT_TRUE(property != NULL);
		ASSERT_EQ_INT(3, property->count);
		assert_initialized_item(property);

		init_test_item(property, 1);
		init_test_item(property, 2);
		ASSERT_STREQ(property->type == INDIGO_BLOB_VECTOR ? "IMAGE_2" : property->type == INDIGO_LIGHT_VECTOR ? "STATUS_2" : property->type == INDIGO_SWITCH_VECTOR ? "SWITCH_2" : property->type == INDIGO_NUMBER_VECTOR ? "VALUE_2" : "ITEM_2", property->items[1].name);

		indigo_release_property(property);
	}
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "properties_are_initialized_for_all_vector_types", properties_are_initialized_for_all_vector_types },
		{ "property_matching_works_for_all_vector_types", property_matching_works_for_all_vector_types },
		{ "copied_properties_are_independent_for_all_vector_types", copied_properties_are_independent_for_all_vector_types },
		{ "resized_properties_preserve_items_for_all_vector_types", resized_properties_preserve_items_for_all_vector_types }
	};
	return indigo_run_tests("bus property unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
