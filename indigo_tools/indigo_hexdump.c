// Copyright (c) 2026 CloudMakers, s. r. o.
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

// Build-time helper that turns a file into the body of a C array initializer, so that it can be embedded
// with #include. Every input byte is written as "0x%02x, " on a single line, without a header and without a
// trailing newline; the output is byte-identical to hexdump -v -e '1/1 "0x%02x, "', which is not available
// on Windows. The tool uses the C standard library only and does not depend on any INDIGO library.
//
// Usage: indigo_hexdump [INPUT [OUTPUT]]
// INPUT defaults to the standard input and OUTPUT to the standard output, so "gzip <file | indigo_hexdump"
// works too. Both are handled in binary mode, so no newline translation changes the bytes on Windows.

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

static const char hex_digits[] = "0123456789abcdef";

static int usage(const char *name) {
	fprintf(stderr, "usage: %s [INPUT [OUTPUT]]\n", name);
	return 2;
}

int main(int argc, char **argv) {
	if (argc > 3 || (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {
		return usage(argv[0]);
	}
	const char *input_name = argc > 1 ? argv[1] : NULL;
	const char *output_name = argc > 2 ? argv[2] : NULL;
#ifdef _WIN32
	if (input_name == NULL) {
		_setmode(_fileno(stdin), _O_BINARY);
	}
	if (output_name == NULL) {
		_setmode(_fileno(stdout), _O_BINARY);
	}
#endif
	FILE *input = input_name ? fopen(input_name, "rb") : stdin;
	if (input == NULL) {
		fprintf(stderr, "%s: can't open '%s' for reading\n", argv[0], input_name);
		return 1;
	}
	FILE *output = output_name ? fopen(output_name, "wb") : stdout;
	if (output == NULL) {
		fprintf(stderr, "%s: can't open '%s' for writing\n", argv[0], output_name);
		if (input != stdin) {
			fclose(input);
		}
		return 1;
	}
	unsigned char in_buffer[4096];
	char out_buffer[sizeof(in_buffer) * 6];
	size_t count;
	int result = 0;
	while ((count = fread(in_buffer, 1, sizeof(in_buffer), input)) > 0) {
		char *out = out_buffer;
		for (size_t i = 0; i < count; i++) {
			*out++ = '0';
			*out++ = 'x';
			*out++ = hex_digits[in_buffer[i] >> 4];
			*out++ = hex_digits[in_buffer[i] & 0x0F];
			*out++ = ',';
			*out++ = ' ';
		}
		if (fwrite(out_buffer, 1, out - out_buffer, output) != (size_t)(out - out_buffer)) {
			fprintf(stderr, "%s: can't write '%s'\n", argv[0], output_name ? output_name : "standard output");
			result = 1;
			break;
		}
	}
	if (result == 0 && ferror(input)) {
		fprintf(stderr, "%s: can't read '%s'\n", argv[0], input_name ? input_name : "standard input");
		result = 1;
	}
	if (input != stdin) {
		fclose(input);
	}
	if (output != stdout ? fclose(output) != 0 : fflush(output) != 0) {
		if (result == 0) {
			fprintf(stderr, "%s: can't write '%s'\n", argv[0], output_name ? output_name : "standard output");
		}
		result = 1;
	}
	// a partial output file would look up to date to make and be compiled in silently
	if (result != 0 && output_name != NULL) {
		remove(output_name);
	}
	return result;
}
