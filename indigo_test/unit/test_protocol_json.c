// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver_json.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_uni_io.h>

#include "../test_runner.h"

#define TEST_DEVICE_NAME "Protocol Test Device"
#define TEXT_PROPERTY_NAME "PROTOCOL_TEXT"
#define NUMBER_PROPERTY_NAME "PROTOCOL_NUMBER"
#define SWITCH_PROPERTY_NAME "PROTOCOL_SWITCH"
#define BLOB_PROPERTY_NAME "PROTOCOL_BLOB"
#define TEXT_ITEM_NAME "VALUE"
#define NUMBER_ITEM_NAME "VALUE"
#define SWITCH_ON_ITEM_NAME "ON"
#define BLOB_ITEM_NAME "IMAGE"

typedef struct {
	int enumerate_count;
	int change_count;
	indigo_property_type last_type;
	char last_property[INDIGO_NAME_SIZE];
	char last_item[INDIGO_NAME_SIZE];
	double last_number_value;
	bool last_switch_value;
} protocol_json_context;

static protocol_json_context context;

static indigo_result test_device_attach(indigo_device *device) {
	return INDIGO_OK;
}

static indigo_result test_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	context.enumerate_count++;
	return INDIGO_OK;
}

static indigo_result test_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	context.change_count++;
	context.last_type = property->type;
	INDIGO_COPY_NAME(context.last_property, property->name);
	if (property->count > 0) {
		INDIGO_COPY_NAME(context.last_item, property->items[0].name);
		if (property->type == INDIGO_NUMBER_VECTOR) {
			context.last_number_value = property->items[0].number.value;
		} else if (property->type == INDIGO_SWITCH_VECTOR) {
			context.last_switch_value = property->items[0].sw.value;
		}
	}
	return INDIGO_OK;
}

static indigo_result test_device_detach(indigo_device *device) {
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

static void reset_context(void) {
	memset(&context, 0, sizeof(context));
	test_device.version = INDIGO_VERSION_CURRENT;
	test_device.last_result = INDIGO_OK;
}

static void assert_contains(const char *text, const char *needle) {
	if (strstr(text, needle) == NULL) {
		fprintf(stderr, "Expected output to contain: %s\nActual output:\n%s\n", needle, text);
	}
	ASSERT_TRUE(strstr(text, needle) != NULL);
}

static bool read_file(const char *path, char *buffer, size_t size) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return false;
	}
	size_t bytes = fread(buffer, 1, size - 1, file);
	buffer[bytes] = 0;
	fclose(file);
	return true;
}

static indigo_client *new_output_adapter(const char *path, indigo_uni_handle **handle) {
	*handle = indigo_uni_create_file(path, INDIGO_LOG_NONE);
	if (*handle == NULL) {
		return NULL;
	}
	indigo_client *client = indigo_json_device_adapter(*handle, *handle, false);
	if (client == NULL) {
		indigo_uni_close(handle);
		return NULL;
	}
	client->version = INDIGO_VERSION_CURRENT;
	client->force_item_updates = true;
	return client;
}

static void close_output_adapter(indigo_client *client, indigo_uni_handle **handle) {
	indigo_release_json_device_adapter(client);
	indigo_uni_close(handle);
}

static void json_escape_handles_special_characters(void) {
	ASSERT_STREQ("a\\\"b\\\\c\\nd", indigo_json_escape("a\"b\\c\nd"));
}

static void json_adapter_serializes_define_update_delete_and_blob_url(void) {
	char output[8192];
	indigo_uni_handle *handle = NULL;
	indigo_client *client = new_output_adapter("build/unit/protocol_json_output.tmp", &handle);
	ASSERT_TRUE(client != NULL);
	indigo_device device;
	memset(&device, 0, sizeof(device));
	INDIGO_COPY_NAME(device.name, TEST_DEVICE_NAME);

	indigo_property *number = indigo_init_number_property(NULL, TEST_DEVICE_NAME, NUMBER_PROPERTY_NAME, "Protocol", "Number Label", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	ASSERT_TRUE(number != NULL);
	indigo_init_number_item(number->items, NUMBER_ITEM_NAME, "Value", -10, 100, 0.5, 12.5);
	number->version = INDIGO_VERSION_CURRENT;
	ASSERT_EQ_INT(INDIGO_OK, indigo_json_device_adapter_define_property(client, &device, number, "defined"));
	number->items[0].number.value = 13.5;
	number->items[0].number.target = 14.5;
	number->items[0].do_update = true;
	ASSERT_EQ_INT(INDIGO_OK, indigo_json_device_adapter_update_property(client, &device, number, "updated"));
	ASSERT_EQ_INT(INDIGO_OK, indigo_json_device_adapter_delete_property(client, &device, number, "deleted"));
	indigo_release_property(number);

	indigo_property *blob = indigo_init_blob_property_p(NULL, TEST_DEVICE_NAME, BLOB_PROPERTY_NAME, "Protocol", "BLOB", INDIGO_OK_STATE, INDIGO_WO_PERM, 1);
	ASSERT_TRUE(blob != NULL);
	indigo_init_blob_item(blob->items, BLOB_ITEM_NAME, "Image");
	INDIGO_COPY_NAME(blob->items[0].blob.format, ".fits");
	INDIGO_COPY_VALUE(blob->items[0].blob.url, "http://example.test/blob.fits");
	blob->version = INDIGO_VERSION_CURRENT;
	ASSERT_EQ_INT(INDIGO_OK, indigo_json_device_adapter_define_property(client, &device, blob, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_json_device_adapter_update_property(client, &device, blob, NULL));
	indigo_release_property(blob);

	close_output_adapter(client, &handle);
	ASSERT_TRUE(read_file("build/unit/protocol_json_output.tmp", output, sizeof(output)));

	assert_contains(output, "\"defNumberVector\"");
	assert_contains(output, "\"device\": \"Protocol Test Device\"");
	assert_contains(output, "\"name\": \"PROTOCOL_NUMBER\"");
	assert_contains(output, "\"target\": 12.5, \"value\": 12.5");
	assert_contains(output, "\"setNumberVector\"");
	assert_contains(output, "\"target\": 14.5, \"value\": 13.5");
	assert_contains(output, "\"deleteProperty\"");
	assert_contains(output, "\"message\": \"deleted\"");
	assert_contains(output, "\"defBLOBVector\"");
	assert_contains(output, "\"value\": \"http://example.test/blob.fits\"");
	assert_contains(output, "\"setBLOBVector\"");
}

static indigo_client *new_input_adapter(const char *path, indigo_uni_handle **input) {
	*input = indigo_uni_open_file(path, INDIGO_LOG_NONE);
	if (*input == NULL) {
		return NULL;
	}
	indigo_client *client = indigo_json_device_adapter(*input, NULL, false);
	if (client == NULL) {
		indigo_uni_close(input);
		return NULL;
	}
	client->version = INDIGO_VERSION_CURRENT;
	return client;
}

static void close_input_adapter(indigo_client *client, indigo_uni_handle **input) {
	indigo_release_json_device_adapter(client);
	indigo_uni_close(input);
}

static void json_parser_routes_number_and_switch_fixtures(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_device(&test_device));

	indigo_uni_handle *input = NULL;
	indigo_client *client = new_input_adapter("fixtures/protocol/change_number.json", &input);
	ASSERT_TRUE(client != NULL);
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(client));
	indigo_json_parse(NULL, client);
	ASSERT_TRUE(context.change_count > 0);
	ASSERT_EQ_INT(INDIGO_NUMBER_VECTOR, context.last_type);
	ASSERT_STREQ(NUMBER_PROPERTY_NAME, context.last_property);
	ASSERT_STREQ(NUMBER_ITEM_NAME, context.last_item);
	ASSERT_NEAR(12.5, context.last_number_value, 1e-12);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(client));
	close_input_adapter(client, &input);

	int previous_change_count = context.change_count;
	client = new_input_adapter("fixtures/protocol/change_switch.json", &input);
	ASSERT_TRUE(client != NULL);
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(client));
	indigo_json_parse(NULL, client);
	ASSERT_TRUE(context.change_count > previous_change_count);
	ASSERT_EQ_INT(INDIGO_SWITCH_VECTOR, context.last_type);
	ASSERT_STREQ(SWITCH_PROPERTY_NAME, context.last_property);
	ASSERT_STREQ(SWITCH_ON_ITEM_NAME, context.last_item);
	ASSERT_TRUE(context.last_switch_value);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(client));
	close_input_adapter(client, &input);

	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_device(&test_device));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

static void json_parser_ignores_malformed_fixture_without_change(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_device(&test_device));
	indigo_uni_handle *input = NULL;
	indigo_client *client = new_input_adapter("fixtures/protocol/malformed.json", &input);
	ASSERT_TRUE(client != NULL);
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(client));
	indigo_json_parse(NULL, client);
	ASSERT_TRUE(context.change_count <= 1);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(client));
	close_input_adapter(client, &input);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_device(&test_device));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "json_escape_handles_special_characters", json_escape_handles_special_characters },
		{ "json_adapter_serializes_define_update_delete_and_blob_url", json_adapter_serializes_define_update_delete_and_blob_url },
		{ "json_parser_routes_number_and_switch_fixtures", json_parser_routes_number_and_switch_fixtures },
		{ "json_parser_ignores_malformed_fixture_without_change", json_parser_ignores_malformed_fixture_without_change }
	};
	return indigo_run_tests("JSON protocol unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
