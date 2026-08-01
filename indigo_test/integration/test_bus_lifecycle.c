// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <string.h>

#include <indigo/indigo_bus.h>

#include "../test_runner.h"

#define TEST_DEVICE_NAME "Bus Test Device"
#define TEST_PROPERTY_NAME "BUS_TEST_TEXT"
#define TEST_ITEM_NAME "VALUE"

typedef struct {
	indigo_property *property;
	int device_attach_count;
	int device_enumerate_count;
	int device_change_count;
	int device_detach_count;
	int client_attach_count;
	int client_define_count;
	int client_update_count;
	int client_delete_count;
	int client_detach_count;
	char last_defined_device[INDIGO_NAME_SIZE];
	char last_defined_property[INDIGO_NAME_SIZE];
	char last_updated_value[INDIGO_VALUE_SIZE];
	char last_deleted_property[INDIGO_NAME_SIZE];
} bus_test_context;

static bus_test_context context;

static indigo_result test_device_attach(indigo_device *device) {
	context.device_attach_count++;
	context.property = indigo_init_text_property(NULL, device->name, TEST_PROPERTY_NAME, "Bus", "Bus test text", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (context.property == NULL) {
		return INDIGO_FAILED;
	}
	indigo_init_text_item(context.property->items, TEST_ITEM_NAME, "Value", "initial");
	return INDIGO_OK;
}

static indigo_result test_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	context.device_enumerate_count++;
	if (indigo_property_match(context.property, property)) {
		return indigo_define_property_to_client(device, client, context.property, NULL);
	}
	return INDIGO_OK;
}

static indigo_result test_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(context.property, property)) {
		context.device_change_count++;
		indigo_property_copy_values(context.property, property, true);
		context.property->state = INDIGO_OK_STATE;
		return indigo_update_property(device, context.property, NULL);
	}
	return INDIGO_OK;
}

static indigo_result test_device_detach(indigo_device *device) {
	context.device_detach_count++;
	if (context.property != NULL) {
		indigo_release_property(context.property);
		context.property = NULL;
	}
	return INDIGO_OK;
}

static indigo_result test_client_attach(indigo_client *client) {
	context.client_attach_count++;
	return INDIGO_OK;
}

static indigo_result test_client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	context.client_define_count++;
	INDIGO_COPY_NAME(context.last_defined_device, property->device);
	INDIGO_COPY_NAME(context.last_defined_property, property->name);
	return INDIGO_OK;
}

static indigo_result test_client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	context.client_update_count++;
	INDIGO_COPY_VALUE(context.last_updated_value, property->items[0].text.value);
	return INDIGO_OK;
}

static indigo_result test_client_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	context.client_delete_count++;
	INDIGO_COPY_NAME(context.last_deleted_property, property->name);
	return INDIGO_OK;
}

static indigo_result test_client_detach(indigo_client *client) {
	context.client_detach_count++;
	return INDIGO_OK;
}

static indigo_device test_device = INDIGO_DEVICE_INITIALIZER(
	TEST_DEVICE_NAME,
	test_device_attach,
	test_device_enumerate_properties,
	test_device_change_property,
	NULL,
	test_device_detach
);

static indigo_client test_client = {
	"Bus Test Client",
	false,
	NULL,
	INDIGO_OK,
	INDIGO_VERSION_LEGACY,
	NULL,
	test_client_attach,
	test_client_define_property,
	test_client_update_property,
	test_client_delete_property,
	NULL,
	test_client_detach,
	false,
	false
};

static void reset_context(void) {
	memset(&context, 0, sizeof(context));
	test_device.is_remote = true;
	test_device.last_result = INDIGO_OK;
	test_device.access_token = 0;
	test_client.last_result = INDIGO_OK;
}

static indigo_property *new_property_selector(const char *device_name, const char *property_name) {
	return indigo_init_text_property(NULL, device_name, property_name, "", "", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
}

static void start_attach_enumerate_change_delete_and_stop_bus(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(&test_client));
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_device(&test_device));
	ASSERT_EQ_INT(1, context.client_attach_count);
	ASSERT_EQ_INT(1, context.device_attach_count);
	ASSERT_TRUE(context.property != NULL);

	indigo_property *selector = new_property_selector(TEST_DEVICE_NAME, "");
	ASSERT_TRUE(selector != NULL);
	ASSERT_EQ_INT(INDIGO_OK, indigo_enumerate_properties(&test_client, selector));
	ASSERT_EQ_INT(1, context.device_enumerate_count);
	ASSERT_EQ_INT(1, context.client_define_count);
	ASSERT_STREQ(TEST_DEVICE_NAME, context.last_defined_device);
	ASSERT_STREQ(TEST_PROPERTY_NAME, context.last_defined_property);
	ASSERT_TRUE(context.property->defined);
	indigo_release_property(selector);

	indigo_property *change = indigo_init_text_property(NULL, TEST_DEVICE_NAME, TEST_PROPERTY_NAME, "Bus", "Bus test text", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	ASSERT_TRUE(change != NULL);
	indigo_init_text_item(change->items, TEST_ITEM_NAME, "Value", "changed");
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_property(&test_client, change));
	ASSERT_EQ_INT(1, context.device_change_count);
	ASSERT_EQ_INT(1, context.client_update_count);
	ASSERT_STREQ("changed", context.last_updated_value);
	indigo_release_property(change);

	ASSERT_EQ_INT(INDIGO_OK, indigo_delete_property(&test_device, context.property, NULL));
	ASSERT_EQ_INT(1, context.client_delete_count);
	ASSERT_STREQ(TEST_PROPERTY_NAME, context.last_deleted_property);
	ASSERT_FALSE(context.property->defined);

	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_device(&test_device));
	ASSERT_EQ_INT(1, context.device_detach_count);
	ASSERT_TRUE(context.property == NULL);

	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(&test_client));
	ASSERT_EQ_INT(1, context.client_detach_count);
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

static void bus_reports_failures_for_invalid_lifecycle_calls(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_FAILED, indigo_attach_client(&test_client));
	ASSERT_EQ_INT(INDIGO_FAILED, indigo_attach_device(&test_device));
	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_NOT_FOUND, indigo_detach_client(&test_client));
	ASSERT_EQ_INT(INDIGO_NOT_FOUND, indigo_detach_device(&test_device));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "start_attach_enumerate_change_delete_and_stop_bus", start_attach_enumerate_change_delete_and_stop_bus },
		{ "bus_reports_failures_for_invalid_lifecycle_calls", bus_reports_failures_for_invalid_lifecycle_calls }
	};
	return indigo_run_tests("bus lifecycle integration tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

