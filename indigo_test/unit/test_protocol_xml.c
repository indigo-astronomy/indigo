// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_client_xml.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_uni_io.h>

#include "../test_runner.h"

#define TEST_DEVICE_NAME "Protocol Test Device"
#define TEXT_PROPERTY_NAME "PROTOCOL_TEXT"
#define NUMBER_PROPERTY_NAME "PROTOCOL_NUMBER"
#define BLOB_PROPERTY_NAME "PROTOCOL_BLOB"
#define TEXT_ITEM_NAME "VALUE"
#define NUMBER_ITEM_NAME "VALUE"
#define BLOB_ITEM_NAME "IMAGE"

typedef struct {
	int enumerate_count;
	int change_count;
	int define_count;
	int update_count;
	int delete_count;
	int message_count;
	indigo_property_type last_type;
	char last_property[INDIGO_NAME_SIZE];
	char last_item[INDIGO_NAME_SIZE];
	char last_text_value[INDIGO_VALUE_SIZE];
	char last_message[INDIGO_VALUE_SIZE];
	double last_number_value;
	indigo_token last_token;
} protocol_xml_context;

static protocol_xml_context context;

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
	context.last_token = property->access_token;
	if (property->count > 0) {
		INDIGO_COPY_NAME(context.last_item, property->items[0].name);
		if (property->type == INDIGO_TEXT_VECTOR) {
			INDIGO_COPY_VALUE(context.last_text_value, property->items[0].text.value);
		} else if (property->type == INDIGO_NUMBER_VECTOR) {
			context.last_number_value = property->items[0].number.value;
		}
	}
	return INDIGO_OK;
}

static indigo_result test_device_detach(indigo_device *device) {
	return INDIGO_OK;
}

static indigo_result test_client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->name, TEXT_PROPERTY_NAME)) {
		return INDIGO_OK;
	}
	context.define_count++;
	context.last_type = property->type;
	INDIGO_COPY_NAME(context.last_property, property->name);
	if (property->count > 0) {
		INDIGO_COPY_NAME(context.last_item, property->items[0].name);
		if (property->type == INDIGO_TEXT_VECTOR) {
			INDIGO_COPY_VALUE(context.last_text_value, property->items[0].text.value);
		}
	}
	if (message != NULL) {
		INDIGO_COPY_VALUE(context.last_message, message);
	}
	return INDIGO_OK;
}

static indigo_result test_client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->name, TEXT_PROPERTY_NAME)) {
		return INDIGO_OK;
	}
	context.update_count++;
	context.last_type = property->type;
	INDIGO_COPY_NAME(context.last_property, property->name);
	if (property->count > 0) {
		INDIGO_COPY_NAME(context.last_item, property->items[0].name);
		if (property->type == INDIGO_TEXT_VECTOR) {
			INDIGO_COPY_VALUE(context.last_text_value, property->items[0].text.value);
		}
	}
	if (message != NULL) {
		INDIGO_COPY_VALUE(context.last_message, message);
	}
	return INDIGO_OK;
}

static indigo_result test_client_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (strcmp(property->name, TEXT_PROPERTY_NAME)) {
		return INDIGO_OK;
	}
	context.delete_count++;
	INDIGO_COPY_NAME(context.last_property, property->name);
	if (message != NULL) {
		INDIGO_COPY_VALUE(context.last_message, message);
	}
	return INDIGO_OK;
}

static indigo_result test_client_send_message(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (property == NULL || strcmp(property->name, TEXT_PROPERTY_NAME)) {
		return INDIGO_OK;
	}
	context.message_count++;
	INDIGO_COPY_NAME(context.last_property, property->name);
	if (message != NULL) {
		INDIGO_COPY_VALUE(context.last_message, message);
	}
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
	"Protocol Observer",
	false,
	NULL,
	INDIGO_OK,
	INDIGO_VERSION_CURRENT,
	NULL,
	NULL,
	test_client_define_property,
	test_client_update_property,
	test_client_delete_property,
	test_client_send_message,
	NULL,
	false,
	false
};

static void reset_context(void) {
	memset(&context, 0, sizeof(context));
	test_device.version = INDIGO_VERSION_CURRENT;
	test_device.last_result = INDIGO_OK;
	test_client.version = INDIGO_VERSION_CURRENT;
	test_client.last_result = INDIGO_OK;
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
	indigo_client *client = indigo_xml_device_adapter(*handle, *handle);
	if (client == NULL) {
		indigo_uni_close(handle);
		return NULL;
	}
	client->version = INDIGO_VERSION_CURRENT;
	client->force_item_updates = true;
	return client;
}

static void close_output_adapter(indigo_client *client, indigo_uni_handle **handle) {
	indigo_release_xml_device_adapter(client);
	indigo_uni_close(handle);
}

static void xml_escape_handles_special_characters(void) {
	ASSERT_STREQ("a&amp;b&lt;c&gt;&quot;d&apos;e", indigo_xml_escape("a&b<c>\"d'e"));
}

static void xml_adapter_serializes_define_update_delete_and_blob_url(void) {
	char output[8192];
	indigo_uni_handle *handle = NULL;
	indigo_client *client = new_output_adapter("build/unit/protocol_xml_output.tmp", &handle);
	ASSERT_TRUE(client != NULL);
	indigo_device device;
	memset(&device, 0, sizeof(device));
	INDIGO_COPY_NAME(device.name, TEST_DEVICE_NAME);

	indigo_property *text = indigo_init_text_property(NULL, TEST_DEVICE_NAME, TEXT_PROPERTY_NAME, "Protocol", "Text & Label", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	ASSERT_TRUE(text != NULL);
	indigo_init_text_item(text->items, TEXT_ITEM_NAME, "Value", "alpha & beta");
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_device_adapter_define_property(client, &device, text, "defined"));
	indigo_set_text_item_value(text->items, "changed <value>");
	text->items[0].do_update = true;
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_device_adapter_update_property(client, &device, text, "updated"));
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_device_adapter_delete_property(client, &device, text, "deleted"));
	indigo_release_property(text);

	indigo_property *blob = indigo_init_blob_property_p(NULL, TEST_DEVICE_NAME, BLOB_PROPERTY_NAME, "Protocol", "BLOB", INDIGO_OK_STATE, INDIGO_WO_PERM, 1);
	ASSERT_TRUE(blob != NULL);
	indigo_init_blob_item(blob->items, BLOB_ITEM_NAME, "Image");
	INDIGO_COPY_NAME(blob->items[0].blob.format, ".fits");
	INDIGO_COPY_VALUE(blob->items[0].blob.url, "http://example.test/blob.fits");
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_device_adapter_define_property(client, &device, blob, NULL));
	indigo_release_property(blob);

	close_output_adapter(client, &handle);
	ASSERT_TRUE(read_file("build/unit/protocol_xml_output.tmp", output, sizeof(output)));

	assert_contains(output, "<defTextVector device='Protocol Test Device' name='PROTOCOL_TEXT'");
	assert_contains(output, "label='Text &amp; Label'");
	assert_contains(output, "<defText name='VALUE' label='Value'>alpha &amp; beta</defText>");
	assert_contains(output, "<setTextVector device='Protocol Test Device' name='PROTOCOL_TEXT' state='Ok' message='updated'>");
	assert_contains(output, "<oneText name='VALUE'>changed &lt;value&gt;</oneText>");
	assert_contains(output, "<delProperty device='Protocol Test Device' name='PROTOCOL_TEXT' message='deleted'/>");
	assert_contains(output, "<defBLOB name='IMAGE' url='http://example.test/blob.fits' label='Image'/>");
}

static void xml_client_adapter_serializes_requests(void) {
	char output[8192];
	indigo_uni_handle *handle = indigo_uni_create_file("build/unit/protocol_xml_client_output.tmp", INDIGO_LOG_NONE);
	ASSERT_TRUE(handle != NULL);
	indigo_device *adapter = indigo_xml_client_adapter("Protocol Peer", "", handle, handle);
	ASSERT_TRUE(adapter != NULL);
	adapter->version = INDIGO_VERSION_CURRENT;

	indigo_property *request = indigo_init_text_property(NULL, TEST_DEVICE_NAME, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
	ASSERT_TRUE(request != NULL);
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_client_parser_enumerate_properties(adapter, &test_client, request));
	indigo_release_property(request);

	indigo_property *text = indigo_init_text_property(NULL, TEST_DEVICE_NAME, TEXT_PROPERTY_NAME, "Protocol", "Text", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	ASSERT_TRUE(text != NULL);
	text->access_token = 0x42;
	indigo_init_text_item(text->items, TEXT_ITEM_NAME, "Value", "alpha & beta");
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_client_parser_change_property(adapter, &test_client, text));
	indigo_release_property(text);

	indigo_property *number = indigo_init_number_property(NULL, TEST_DEVICE_NAME, NUMBER_PROPERTY_NAME, "Protocol", "Number", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	ASSERT_TRUE(number != NULL);
	indigo_init_number_item(number->items, NUMBER_ITEM_NAME, "Value", -10, 100, 0.5, 12.5);
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_client_parser_change_property(adapter, &test_client, number));
	indigo_release_property(number);

	indigo_property *sw = indigo_init_switch_property(NULL, TEST_DEVICE_NAME, "PROTOCOL_SWITCH", "Protocol", "Switch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
	ASSERT_TRUE(sw != NULL);
	indigo_init_switch_item(sw->items, "ON", "On", true);
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_client_parser_change_property(adapter, &test_client, sw));
	indigo_release_property(sw);

	indigo_property *blob = indigo_init_blob_property_p(NULL, TEST_DEVICE_NAME, BLOB_PROPERTY_NAME, "Protocol", "BLOB", INDIGO_OK_STATE, INDIGO_WO_PERM, 1);
	ASSERT_TRUE(blob != NULL);
	indigo_init_blob_item(blob->items, BLOB_ITEM_NAME, "Image");
	ASSERT_EQ_INT(INDIGO_OK, indigo_xml_client_parser_enable_blob(adapter, &test_client, blob, INDIGO_ENABLE_BLOB_URL));
	indigo_release_property(blob);

	indigo_release_xml_client_adapter(adapter);
	indigo_uni_close(&handle);
	ASSERT_TRUE(read_file("build/unit/protocol_xml_client_output.tmp", output, sizeof(output)));

	assert_contains(output, "<getProperties device='Protocol Test Device' name=''/>");
	assert_contains(output, "<newTextVector device='Protocol Test Device' name='PROTOCOL_TEXT' token='42'>");
	assert_contains(output, "<oneText name='VALUE'>alpha &amp; beta</oneText>");
	assert_contains(output, "<newNumberVector device='Protocol Test Device' name='PROTOCOL_NUMBER'>");
	assert_contains(output, "<oneNumber name='VALUE'>12.5</oneNumber>");
	assert_contains(output, "<newSwitchVector device='Protocol Test Device' name='PROTOCOL_SWITCH'>");
	assert_contains(output, "<oneSwitch name='ON'>On</oneSwitch>");
	assert_contains(output, "<enableBLOB device='Protocol Test Device' name='PROTOCOL_BLOB'>URL</enableBLOB>");
}

static void parse_fixture(const char *path, indigo_client *client) {
	indigo_uni_handle *input = indigo_uni_open_file(path, INDIGO_LOG_NONE);
	ASSERT_TRUE(input != NULL);
	((indigo_adapter_context *)client->client_context)->input = input;
	indigo_xml_parse(NULL, client);
	indigo_uni_close(&input);
	((indigo_adapter_context *)client->client_context)->input = NULL;
}

static void xml_parser_routes_change_and_enable_blob_fixtures(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_device(&test_device));

	indigo_uni_handle *input = indigo_uni_open_file("fixtures/protocol/change_text.xml", INDIGO_LOG_NONE);
	ASSERT_TRUE(input != NULL);
	indigo_client *client = indigo_xml_device_adapter(input, NULL);
	ASSERT_TRUE(client != NULL);
	client->version = INDIGO_VERSION_CURRENT;
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(client));
	indigo_xml_parse(NULL, client);
	indigo_uni_close(&input);
	((indigo_adapter_context *)client->client_context)->input = NULL;

	ASSERT_TRUE(context.change_count > 0);
	ASSERT_EQ_INT(INDIGO_TEXT_VECTOR, context.last_type);
	ASSERT_STREQ(TEXT_PROPERTY_NAME, context.last_property);
	ASSERT_STREQ(TEXT_ITEM_NAME, context.last_item);
	ASSERT_STREQ("changed & escaped", context.last_text_value);
	ASSERT_EQ_TOKEN(0x42, context.last_token);

	parse_fixture("fixtures/protocol/enable_blob_url.xml", client);
	ASSERT_TRUE(client->enable_blob_mode_records != NULL);
	ASSERT_STREQ(TEST_DEVICE_NAME, client->enable_blob_mode_records->device);
	ASSERT_STREQ(BLOB_PROPERTY_NAME, client->enable_blob_mode_records->name);
	ASSERT_EQ_INT(INDIGO_ENABLE_BLOB_URL, client->enable_blob_mode_records->mode);

	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(client));
	indigo_release_xml_device_adapter(client);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_device(&test_device));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

static void xml_client_adapter_parses_remote_property_events(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(&test_client));

	indigo_uni_handle *input = indigo_uni_open_file("fixtures/protocol/remote_text_events.xml", INDIGO_LOG_NONE);
	ASSERT_TRUE(input != NULL);
	indigo_device *adapter = indigo_xml_client_adapter("Protocol Peer", "", input, NULL);
	ASSERT_TRUE(adapter != NULL);
	adapter->version = INDIGO_VERSION_CURRENT;
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_device(adapter));
	reset_context();
	indigo_xml_parse(adapter, NULL);
	indigo_uni_close(&input);
	((indigo_adapter_context *)adapter->device_context)->input = NULL;

	ASSERT_EQ_INT(1, context.define_count);
	ASSERT_EQ_INT(1, context.update_count);
	ASSERT_TRUE(context.message_count >= 1);
	ASSERT_EQ_INT(1, context.delete_count);
	ASSERT_EQ_INT(INDIGO_TEXT_VECTOR, context.last_type);
	ASSERT_STREQ(TEXT_PROPERTY_NAME, context.last_property);
	ASSERT_STREQ("deleted", context.last_message);

	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_device(adapter));
	indigo_release_xml_client_adapter(adapter);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(&test_client));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

static void xml_parser_ignores_malformed_fixture_without_change(void) {
	reset_context();

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_device(&test_device));
	indigo_uni_handle *input = indigo_uni_open_file("fixtures/protocol/malformed.xml", INDIGO_LOG_NONE);
	ASSERT_TRUE(input != NULL);
	indigo_client *client = indigo_xml_device_adapter(input, NULL);
	ASSERT_TRUE(client != NULL);
	client->version = INDIGO_VERSION_CURRENT;
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(client));
	indigo_xml_parse(NULL, client);
	indigo_uni_close(&input);
	((indigo_adapter_context *)client->client_context)->input = NULL;

	ASSERT_TRUE(context.change_count <= 1);

	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(client));
	indigo_release_xml_device_adapter(client);
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_device(&test_device));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "xml_escape_handles_special_characters", xml_escape_handles_special_characters },
		{ "xml_adapter_serializes_define_update_delete_and_blob_url", xml_adapter_serializes_define_update_delete_and_blob_url },
		{ "xml_client_adapter_serializes_requests", xml_client_adapter_serializes_requests },
		{ "xml_parser_routes_change_and_enable_blob_fixtures", xml_parser_routes_change_and_enable_blob_fixtures },
		{ "xml_client_adapter_parses_remote_property_events", xml_client_adapter_parses_remote_property_events },
		{ "xml_parser_ignores_malformed_fixture_without_change", xml_parser_ignores_malformed_fixture_without_change }
	};
	return indigo_run_tests("XML protocol unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
