// Copyright (c) 2023 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#include "tool_file_io.h"

static const char *memfind(const char *haystack, size_t haystacklen, const char *needle, size_t needlelen) {
	if (needlelen > haystacklen) {
		return NULL;
	}
	for (size_t offset = 0; offset <= haystacklen - needlelen; offset++) {
		if (!memcmp(needle, haystack + offset, needlelen)) {
			return haystack + offset;
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	char *data;
	if (argc != 2) {
		printf("Please specify a broken XISF file to be fixed. The orifinal file will be overwriten, make sure you have a backup copy!\n");
		printf("usage: %s filename.xisf\n", basename(argv[0]));
		return 1;
	}
	size_t size;
	if (!tool_read_file(argv[1], &data, &size, SIZE_MAX - 1) || size < 2880 || size > SIZE_MAX - 2880) {
		fprintf(stderr, "Invalid or unreadable XISF image\n");
		free(data);
		return 1;
	}
	const char *end = memfind(data, 2880, "</xisf>", 7);
	const char *attachment = memfind(data, end != NULL ? (size_t)(end - data) : 0, "attachment:2880", 15);
	if (end == NULL || attachment == NULL) {
		fprintf(stderr, "Not a supported broken XISF image\n");
		free(data);
		return 1;
	}
	size_t header_len = (size_t)(end - data) + 7, output_size = size + 2880;
	char *output = calloc(1, output_size);
	if (output == NULL) {
		free(data);
		return 1;
	}
	memcpy(output, data, header_len);
	memcpy(output + (attachment - data) + 11, "5760", 4);
	memcpy(output + 5760, data + 2880, size - 2880);
	bool ok = tool_write_file(argv[1], output, output_size);
	free(output);
	free(data);
	if (!ok) {
		fprintf(stderr, "Failed to overwrite %s\n", argv[1]);
		return 1;
	}
	printf("fixed: %s\n", argv[1]);
	return 0;
}
