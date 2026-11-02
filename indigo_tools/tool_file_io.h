// Copyright (c) 2026 INDIGO initiative
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

/* Checked whole-file I/O for the standalone image repair tools. */
#ifndef INDIGO_TOOL_FILE_IO_H
#define INDIGO_TOOL_FILE_IO_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
static inline bool tool_read_file(const char *name, char **data, size_t *size, size_t limit) {
	*data = NULL;
	*size = 0;
	FILE *file = fopen(name, "rb");
	if (file == NULL) {
		return false;
	}
	bool ok = false;
	long length;
	if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) >= 0 && (uintmax_t)length <= limit && (uintmax_t)length < SIZE_MAX && fseek(file, 0, SEEK_SET) == 0) {
		*data = malloc((size_t)length + 1);
		if (*data != NULL && fread(*data, 1, (size_t)length, file) == (size_t)length && !ferror(file)) {
			(*data)[length] = 0;
			*size = (size_t)length;
			ok = true;
		}
	}
	if (fclose(file) != 0) {
		ok = false;
	}
	if (!ok) {
		free(*data);
		*data = NULL;
		*size = 0;
	}
	return ok;
}

static inline bool tool_write_file(const char *name, const void *data, size_t size) {
	FILE *file = fopen(name, "wb");
	if (file == NULL) {
		return false;
	}
	bool ok = fwrite(data, 1, size, file) == size && !ferror(file);
	if (fclose(file) != 0) {
		ok = false;
	}
	return ok;
}
#endif
