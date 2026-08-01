// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <string.h>

#include <indigo/indigo_base64.h>

#include "../test_runner.h"

static void known_vectors_encode_correctly(void) {
	unsigned char out[64];

	ASSERT_EQ_INT(4, base64_encode(out, (const unsigned char *)"f", 1));
	ASSERT_STREQ("Zg==", (const char *)out);
	ASSERT_EQ_INT(4, base64_encode(out, (const unsigned char *)"fo", 2));
	ASSERT_STREQ("Zm8=", (const char *)out);
	ASSERT_EQ_INT(4, base64_encode(out, (const unsigned char *)"foo", 3));
	ASSERT_STREQ("Zm9v", (const char *)out);
	ASSERT_EQ_INT(16, base64_encode(out, (const unsigned char *)"hello world", 11));
	ASSERT_STREQ("aGVsbG8gd29ybGQ=", (const char *)out);
}

static void encoded_data_decodes_back_to_original(void) {
	const unsigned char input[] = { 0x00, 0x01, 0x02, 0x7f, 0x80, 0xff };
	unsigned char encoded[32];
	unsigned char decoded[sizeof(input)];

	long encoded_length = base64_encode(encoded, input, sizeof(input));
	long decoded_length = base64_decode_fast(decoded, encoded, encoded_length);

	ASSERT_EQ_INT(sizeof(input), decoded_length);
	ASSERT_TRUE(memcmp(input, decoded, sizeof(input)) == 0);
}

static void newline_tolerant_decoder_handles_embedded_newline(void) {
	const unsigned char encoded[] = "Zm9v\nYmFy";
	unsigned char decoded[16] = { 0 };

	long decoded_length = base64_decode_fast_nl(decoded, encoded, strlen((const char *)encoded));

	ASSERT_EQ_INT(6, decoded_length);
	ASSERT_TRUE(memcmp("foobar", decoded, 6) == 0);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "known_vectors_encode_correctly", known_vectors_encode_correctly },
		{ "encoded_data_decodes_back_to_original", encoded_data_decodes_back_to_original },
		{ "newline_tolerant_decoder_handles_embedded_newline", newline_tolerant_decoder_handles_embedded_newline }
	};
	return indigo_run_tests("base64 unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

