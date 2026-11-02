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
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define BROKEN_FITS_OFFSET (2880*2)

#include "tool_file_io.h"

int main(int argc, char *argv[]) {
	char *data;
	if (argc != 2) {
		printf("Please specify a broken FITS file to be fixed. The orifinal file will be overwriten, make sure you have a backup copy!\n");
		printf("usage: %s filename.fits\n", basename(argv[0]));
		return 1;
	}
	size_t size;
	if (!tool_read_file(argv[1], &data, &size, SIZE_MAX - 1)) {
		printf("error reading: %s\n", argv[1]);
		return 1;
	}

	if (size < BROKEN_FITS_OFFSET + 6 || memcmp(data, "SIMPLE", 6) || memcmp(data + BROKEN_FITS_OFFSET, "SIMPLE", 6)) {
		fprintf(stderr, "Not a supported broken FITS image\n");
		free(data);
		return 1;
	}
	bool ok = tool_write_file(argv[1], data + BROKEN_FITS_OFFSET, size - BROKEN_FITS_OFFSET);
	free(data);
	if (!ok) {
		fprintf(stderr, "Failed to overwrite %s\n", argv[1]);
		return 1;
	}
	printf("fixed: %s\n", argv[1]);
	return 0;
}
