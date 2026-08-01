// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo/indigo_token.h>

#include "../test_runner.h"

static void string_to_token_accepts_hex_and_rejects_invalid_input(void) {
	ASSERT_EQ_TOKEN(0x1234abcdULL, indigo_string_to_token("1234abcd"));
	ASSERT_EQ_TOKEN(0x1234abcdULL, indigo_string_to_token("0x1234abcd"));
	ASSERT_EQ_TOKEN(0, indigo_string_to_token(NULL));
	ASSERT_EQ_TOKEN(0, indigo_string_to_token("not-a-token"));
}

static void device_tokens_can_be_added_updated_and_removed(void) {
	indigo_clear_device_tokens();
	indigo_set_master_token(0);

	ASSERT_TRUE(indigo_add_device_token("Unit Device", 0x1111));
	ASSERT_EQ_TOKEN(0x1111, indigo_get_device_token("Unit Device"));

	ASSERT_TRUE(indigo_add_device_token("Unit Device", 0x2222));
	ASSERT_EQ_TOKEN(0x2222, indigo_get_device_token("Unit Device"));

	ASSERT_TRUE(indigo_remove_device_token("Unit Device"));
	ASSERT_EQ_TOKEN(0, indigo_get_device_token("Unit Device"));
	ASSERT_FALSE(indigo_remove_device_token("Unit Device"));
}

static void device_or_master_token_falls_back_to_master(void) {
	indigo_clear_device_tokens();
	indigo_set_master_token(0xaaaa);

	ASSERT_EQ_TOKEN(0xaaaa, indigo_get_master_token());
	ASSERT_EQ_TOKEN(0xaaaa, indigo_get_device_or_master_token("Missing Device"));

	ASSERT_TRUE(indigo_add_device_token("Specific Device", 0xbbbb));
	ASSERT_EQ_TOKEN(0xbbbb, indigo_get_device_or_master_token("Specific Device"));

	indigo_clear_device_tokens();
	indigo_set_master_token(0);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "string_to_token_accepts_hex_and_rejects_invalid_input", string_to_token_accepts_hex_and_rejects_invalid_input },
		{ "device_tokens_can_be_added_updated_and_removed", device_tokens_can_be_added_updated_and_removed },
		{ "device_or_master_token_falls_back_to_master", device_or_master_token_falls_back_to_master }
	};
	return indigo_run_tests("token unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

