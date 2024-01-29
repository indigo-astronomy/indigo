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

int read_file(char *filename, char **data) {
	FILE *f = fopen(filename, "rb");
	if (f == 0) {
		return -1;
	}
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	*data = malloc(fsize + 1);
	int count = fread(*data, 1, fsize, f);
	fclose(f);
	(*data)[fsize] = 0;
	return count;
}

int main(int argc, char *argv[]) {
	char *data;
	if (argc != 2) {
		printf("Please specify a broken FITS file to be fixed. The orifinal file will be overwriten, make sure you have a backup copy!\n");
		printf("usage: %s filename.fits\n", basename(argv[0]));
		return 1;
	}
	int size = read_file(argv[1], &data);
	if (size < 0) {
		printf("error reading: %s\n", argv[1]);
		return 1;
	}

	if (strncmp(data, "SIMPLE", 6)) {
		printf("does not appear to be a FITS image\n");
		return 1;
	}

	if (strncmp(data + BROKEN_FITS_OFFSET, "SIMPLE", 6)) {
		printf("can not find true start position, maybe fixed?\n");
		return 1;
	}

	FILE *f = fopen(argv[1], "wb");
	if (f == 0) {
		printf("can not overwrite: %s\n", argv[1]);
		return 1;
	}

	fwrite(data + BROKEN_FITS_OFFSET, 1, size - BROKEN_FITS_OFFSET, f);

	printf("fixed: %s\n", argv[1]);
	fclose(f);
	free(data);
	return 0;
}