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

void *memfind(const char *haystack, size_t haystacklen, const char *needle, size_t needlelen) {
	for(int offset = 0; offset < haystacklen-needlelen; offset++) {
   		if(!memcmp(needle, haystack + offset, needlelen)) {
			return haystack + offset;
		}
    }
	return NULL;
}

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
		printf("Please specify a broken XISF file to be fixed. The orifinal file will be overwriten, make sure you have a backup copy!\n");
		printf("usage: %s filename.xisf\n", basename(argv[0]));
		return 1;
	}
	int size = read_file(argv[1], &data);
	if (size < 0) {
		printf("error reading: %s\n", argv[1]);
		return 1;
	}

	char header_end[] = "</xisf>";
	char *ptr = memfind(data, size, header_end, strlen(header_end));
	if (ptr == NULL) {
		printf("does not appear to be a XISF image\n");
		return 1;
	}
	ptr = ptr + strlen(header_end);
	int header_len = ptr - data;

	char to_find[] = "attachment:2880";
	ptr = memfind(data, size, to_find, strlen(to_find));
	if (ptr == NULL) {
		printf("can not find attachment position, maybe fixed?\n");
		return 1;
	}
	ptr[11] = '5';
	ptr[12] = '7';
	ptr[13] = '6';
	ptr[14] = '0';

	FILE *f = fopen(argv[1], "wb");
	if (f == 0) {
		printf("can not overwrite: %s\n", argv[1]);
		return 1;
	}
	fwrite(data, header_len, 1, f);

	for (int i = 0; i < 5760 - header_len; i++) {
		fputc(0, f);
	}

	int data_len = size - header_len;
	fwrite(data + 2880, 1, size - 2880, f);

	printf("fixed: %s\n", argv[1]);
	fclose(f);
	free(data);
	return 0;
}