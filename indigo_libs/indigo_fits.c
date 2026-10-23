// Copyright (c) 2021-2025 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_fits.h>

typedef struct {
	char *data;
	size_t used;
	size_t size;
} raw_fits_header;

static bool raw_fits_append(raw_fits_header *header, const char *format, ...) {
	char card[81];
	va_list args;
	va_start(args, format);
	int length = vsnprintf(card, sizeof(card), format, args);
	va_end(args);
	if (length < 0 || length > 80) {
		errno = EINVAL;
		return false;
	}
	if (header->used == header->size) {
		if (header->size > INT_MAX - FITS_RECORD_SIZE) {
			errno = EOVERFLOW;
			return false;
		}
		char *data = realloc(header->data, header->size + FITS_RECORD_SIZE);
		if (data == NULL) {
			return false;
		}
		header->data = data;
		memset(data + header->size, ' ', FITS_RECORD_SIZE);
		header->size += FITS_RECORD_SIZE;
	}
	memcpy(header->data + header->used, card, length);
	header->used += 80;
	return true;
}

static bool raw_fits_keywords(raw_fits_header *header, const indigo_fits_keyword *keywords) {
	for (; keywords && keywords->type; keywords++) {
		if (keywords->name == NULL || keywords->name[0] == '\0' || strlen(keywords->name) > 8) {
			errno = EINVAL;
			return false;
		}
		const char *comment = keywords->comment ? keywords->comment : "";
		switch (keywords->type) {
			case INDIGO_FITS_NUMBER: {
				char number[81];
				int length = snprintf(number, sizeof(number), "%20f", keywords->number);
				if (length < 0 || length > 70) {
					errno = EINVAL;
					return false;
				}
				indigo_fix_locale(number);
				if (!raw_fits_append(header, "%-8s= %s%s%s", keywords->name, number, comment[0] ? " / " : "", comment)) {
					return false;
				}
				break;
			}
			case INDIGO_FITS_STRING:
				if (keywords->string == NULL || strlen(keywords->string) > 68) {
					errno = EINVAL;
					return false;
				}
				if (!raw_fits_append(header, "%-8s= '%s'%*s%s%s", keywords->name, keywords->string, (int)(strlen(keywords->string) < 18 ? 18 - strlen(keywords->string) : 0), "", comment[0] ? " / " : "", comment)) {
					return false;
				}
				break;
			case INDIGO_FITS_LOGICAL:
				if (!raw_fits_append(header, "%-8s= %20c / %s", keywords->name, keywords->logical ? 'T' : 'F', comment)) {
					return false;
				}
				break;
			default:
				errno = EINVAL;
				return false;
		}
	}
	return true;
}

static bool raw_fits_extension(raw_fits_header *header, const char *extension, size_t length) {
	if (length < 9 || memcmp(extension, "SIMPLE=T;", 9)) {
		return true;
	}
	extension += 9;
	length -= 9;
	while (length && *extension) {
		const char *end = memchr(extension, ';', length);
		if (end == NULL) {
			errno = EINVAL;
			return false;
		}
		size_t field_size = end - extension;
		const char *equals = memchr(extension, '=', field_size);
		if (equals == NULL) {
			errno = EINVAL;
			return false;
		}
		size_t name_size = equals - extension;
		while (name_size && extension[name_size - 1] == ' ') {
			name_size--;
		}
		if (name_size == 0 || name_size > 8) {
			errno = EINVAL;
			return false;
		}
		char name[9];
		memcpy(name, extension, name_size);
		name[name_size] = '\0';
		const char *value = equals + 1;
		while (value < end && *value == ' ') {
			value++;
		}
		const char *value_end = value;
		if (value < end && (*value == '\'' || *value == '(')) {
			char closing = *value == '\'' ? '\'' : ')';
			value_end = memchr(value + 1, closing, end - value - 1);
			if (value_end == NULL) {
				errno = EINVAL;
				return false;
			}
			value_end++;
		} else {
			while (value_end < end && *value_end != ' ' && *value_end != '/') {
				value_end++;
			}
		}
		size_t value_size = value_end - value;
		if (value_size == 0 || value_size > 70 || memchr(value, '\0', value_size)) {
			errno = EINVAL;
			return false;
		}
		if (!raw_fits_append(header, "%-8s= %.*s", name, (int)value_size, value)) {
			return false;
		}
		length -= field_size + 1;
		extension = end + 1;
	}
	return true;
}

indigo_result indigo_raw_to_fits(char *image, int in_size, char **fits, int *fits_size, indigo_fits_keyword *keywords) {
	if (image == NULL || fits == NULL || fits_size == NULL || in_size < (int)sizeof(indigo_raw_header)) {
		errno = EINVAL;
		return INDIGO_FAILED;
	}
	indigo_raw_header raw;
	memcpy(&raw, image, sizeof(raw));
	int byte_per_pixel, components;
	if (!memcmp(image, "RAW1", 4)) {
		byte_per_pixel = 1;
		components = 1;
	} else if (!memcmp(image, "RAW2", 4)) {
		byte_per_pixel = 2;
		components = 1;
	} else if (!memcmp(image, "RAW3", 4)) {
		byte_per_pixel = 1;
		components = 3;
	} else if (!memcmp(image, "RAW6", 4)) {
		byte_per_pixel = 2;
		components = 3;
	} else {
		errno = EINVAL;
		return INDIGO_FAILED;
	}
	size_t available = (size_t)in_size - sizeof(raw);
	if (raw.width == 0 || raw.height == 0 || raw.width > INT_MAX || raw.height > INT_MAX || raw.width > available / raw.height / components / byte_per_pixel) {
		errno = EINVAL;
		return INDIGO_FAILED;
	}
	size_t pixel_count = (size_t)raw.width * raw.height;
	size_t data_size = pixel_count * components * byte_per_pixel;
	image += sizeof(raw);
	raw_fits_header header = { 0 };
	if (!raw_fits_append(&header, "SIMPLE  = %20c", 'T') || !raw_fits_append(&header, "BITPIX  = %20d", byte_per_pixel * 8) || !raw_fits_append(&header, "NAXIS   = %20d / %s", components > 1 ? 3 : 2, components > 1 ? "RGB Image" : "Monochrome or Bayer mosaic color image") || !raw_fits_append(&header, "NAXIS1  = %20u", raw.width) || !raw_fits_append(&header, "NAXIS2  = %20u", raw.height)) {
		goto failure;
	}
	if (components > 1 && !raw_fits_append(&header, "NAXIS3  = %20d", components)) {
		goto failure;
	}
	if (!raw_fits_append(&header, "EXTEND  = %20c", 'T')) {
		goto failure;
	}
	if (byte_per_pixel == 2 && (!raw_fits_append(&header, "BZERO   = %20d", 32768) || !raw_fits_append(&header, "BSCALE  = %20d", 1))) {
		goto failure;
	}
	if (!raw_fits_keywords(&header, keywords) || !raw_fits_extension(&header, image + data_size, available - data_size) || !raw_fits_append(&header, "ROWORDER= %20s / Image row order", "'TOP-DOWN'") || !raw_fits_append(&header, "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy") || !raw_fits_append(&header, "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H") || !raw_fits_append(&header, "COMMENT   Converted from INDIGO RAW format. See www.indigo-astronomy.org") || !raw_fits_append(&header, "END")) {
		goto failure;
	}
	size_t padded_data_size = data_size + (FITS_RECORD_SIZE - data_size % FITS_RECORD_SIZE) % FITS_RECORD_SIZE;
	if (padded_data_size > INT_MAX - header.size) {
		errno = EOVERFLOW;
		goto failure;
	}
	size_t image_size = header.size + padded_data_size;
	char *buffer = realloc(*fits, image_size);
	if (buffer == NULL) {
		goto failure;
	}
	memcpy(buffer, header.data, header.size);
	unsigned char *data = (unsigned char *)buffer + header.size;
	memset(data + data_size, 0, padded_data_size - data_size);
	for (size_t i = 0; i < pixel_count; i++) {
		for (int component = 0; component < components; component++) {
			size_t input_offset = (i * components + component) * byte_per_pixel;
			size_t output_offset = (component * pixel_count + i) * byte_per_pixel;
			if (byte_per_pixel == 2) {
				uint16_t value;
				memcpy(&value, image + input_offset, sizeof(value));
				value ^= 0x8000;
				data[output_offset] = value >> 8;
				data[output_offset + 1] = value & 0xff;
			} else {
				data[output_offset] = (unsigned char)image[input_offset];
			}
		}
	}
	free(header.data);
	*fits = buffer;
	*fits_size = (int)image_size;
	return INDIGO_OK;
failure:
	free(header.data);
	return INDIGO_FAILED;
}
