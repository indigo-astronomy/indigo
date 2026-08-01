// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_md5.h>

#include "../test_runner.h"

static void known_md5_vectors_match(void) {
	char digest[33] = { 0 };

	indigo_md5(digest, "", 0);
	ASSERT_STREQ("d41d8cd98f00b204e9800998ecf8427e", digest);

	indigo_md5(digest, "abc", 3);
	ASSERT_STREQ("900150983cd24fb0d6963f7d28e17f72", digest);

	indigo_md5(digest, "message digest", 14);
	ASSERT_STREQ("f96b697d7cb7938d525a2f31aaf161d0", digest);
}

static void partial_md5_uses_requested_prefix_length(void) {
	char digest[33] = { 0 };
	const char input[] = "abcdef";

	indigo_md5_partial(digest, input, strlen(input), 3);
	ASSERT_STREQ("900150983cd24fb0d6963f7d28e17f72", digest);

	indigo_md5_partial(digest, input, 3, 99);
	ASSERT_STREQ("900150983cd24fb0d6963f7d28e17f72", digest);
}

static void file_partial_md5_hashes_file_prefix(void) {
	char digest[33] = { 0 };
	FILE *file = tmpfile();
	ASSERT_TRUE(file != NULL);
	ASSERT_EQ_INT(6, fwrite("abcdef", 1, 6, file));
	rewind(file);

	indigo_md5_file_partial(digest, file, 3);

	fclose(file);
	ASSERT_STREQ("900150983cd24fb0d6963f7d28e17f72", digest);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "known_md5_vectors_match", known_md5_vectors_match },
		{ "partial_md5_uses_requested_prefix_length", partial_md5_uses_requested_prefix_length },
		{ "file_partial_md5_hashes_file_prefix", file_partial_md5_hashes_file_prefix }
	};
	return indigo_run_tests("md5 unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}

