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
#include <math.h>
#include <stdio.h>
#include <errno.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_fits.h>

static int raw_read_keyword_value(const uint8_t *ptr8, char *keyword, char *value) {
	int i;
	int length = (int)strlen((const char *)ptr8);

	for (i = 0; i < 8 && ptr8[i] != ' '; i++) {
		keyword[i] = ptr8[i];
	}
	keyword[i] = '\0';

	if (ptr8[8] == '=') {
		i++;
		while (i < length && ptr8[i] == ' ') {
			i++;
		}

		if (i < length) {
			*value++ = ptr8[i];
			i++;
			if (ptr8[i-1] == '\'') {
				for (; i < length && ptr8[i] != '\''; i++) {
					*value++ = ptr8[i];
				}
				*value++ = '\'';
			} else if (ptr8[i-1] == '(') {
				for (; i < length && ptr8[i] != ')'; i++) {
					*value++ = ptr8[i];
				}
				*value++ = ')';
			} else {
				for (; i < length && ptr8[i] != ' ' && ptr8[i] != '/'; i++) {
					*value++ = ptr8[i];
				}
			}
		}
	}
	*value = '\0';
	return 0;
}

indigo_result indigo_raw_to_fits(char *image, int in_size, char **fits, int *fits_size, indigo_fits_keyword *keywords) {
	int byte_per_pixel = 0, components = 0;
	int frame_width = 0, frame_height = 0;
	if (!strncmp("RAW1", (const char *)(image), 4)) {
		// 8 bit RAW
		byte_per_pixel = 1;
		components = 1;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else if (!strncmp("RAW2", (const char *)(image), 4)) {
		// 16 bit RAW
		byte_per_pixel = 2;
		components = 1;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else if (!strncmp("RAW3", (const char *)(image), 4)) {
		// 8 bit RGB
		byte_per_pixel = 1;
		components = 3;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else if (!strncmp("RAW6", (const char *)(image), 4)) {
		// 16 bit RGB
		byte_per_pixel = 2;
		components = 3;
		frame_width = ((indigo_raw_header *)image)->width;
		frame_height = ((indigo_raw_header *)image)->height;
		image = image + sizeof(indigo_raw_header);
	} else {
		errno = EINVAL;
		return INDIGO_FAILED;
	}

	int pixel_count = frame_width * frame_height;
	int data_size = pixel_count * byte_per_pixel;
	int image_size = data_size * components + FITS_RECORD_SIZE;
	if (image_size % FITS_RECORD_SIZE) {
		image_size = (image_size / FITS_RECORD_SIZE + 1) * FITS_RECORD_SIZE;
	}

	*fits = realloc(*fits, image_size);
	char *buffer = *fits;
	if (buffer == NULL) {
		return INDIGO_FAILED;
	}

	char *p = buffer;
	memset(buffer, ' ', image_size);

	int t = sprintf(p, "SIMPLE  = %20c", 'T'); p[t] = ' ';
	t = sprintf(p += 80, "BITPIX  = %20d", byte_per_pixel * 8); p[t] = ' ';
	if (components > 1) {
		t = sprintf(p += 80, "NAXIS   = %20d / RGB Image", 3); p[t] = ' ';
	} else {
		t = sprintf(p += 80, "NAXIS   = %20d / Monochrome or Bayer mosaic color image", 2); p[t] = ' ';
	}
	t = sprintf(p += 80, "NAXIS1  = %20d", frame_width); p[t] = ' ';
	t = sprintf(p += 80, "NAXIS2  = %20d", frame_height); p[t] = ' ';
	if (components > 1) {
		t = sprintf(p += 80, "NAXIS3  = %20d", components); p[t] = ' ';
	}
	t = sprintf(p += 80, "EXTEND  = %20c", 'T'); p[t] = ' ';
	if (byte_per_pixel == 2) {
		t = sprintf(p += 80, "BZERO   = %20d", 32768); p[t] = ' ';
		t = sprintf(p += 80, "BSCALE  = %20d", 1); p[t] = ' ';
	}
	if (keywords) {
		while (keywords->type && (p - (char *)buffer) < (FITS_RECORD_SIZE - 80)) {
			switch (keywords->type) {
				case INDIGO_FITS_NUMBER:
					t = sprintf(p += 80, "%7s= %20f / %s", keywords->name, keywords->number, keywords->comment);
					indigo_fix_locale(p - 80);
					break;
				case INDIGO_FITS_STRING:
					t = sprintf(p += 80, "%7s= '%s'%*c / %s", keywords->name, keywords->string, (int)(18 - strlen(keywords->string)), ' ', keywords->comment);
					break;
				case INDIGO_FITS_LOGICAL:
					t = sprintf(p += 80, "%7s=                    %c / %s", keywords->name, keywords->logical ? 'T' : 'F', keywords->comment);
					break;
			}
			p[t] = ' ';
			keywords++;
		}
	}

	/* add embedded keywords to the fits header */
	int extension_length = in_size - data_size - sizeof(indigo_raw_header);
	if (extension_length > 9) {
		char *extension = indigo_safe_malloc(extension_length + 1);
		char *extension_start = extension;
		strncpy(extension, image + data_size, extension_length);
		if (!strncmp(extension_start, "SIMPLE=T;", 9)) {
			extension_start += 9;
			extension_length -= 9;
			char *pos = NULL;
			while ((pos = strchr(extension_start, ';'))) {
				char keyword[80];
				char value[80];
				*pos = '\0';
				raw_read_keyword_value((const unsigned char *)extension_start, keyword, value);
				extension_start = pos+1;
				t = sprintf(p += 80, "%7s= %s", keyword, value);
				p[t] = ' ';
			}
		}
		indigo_safe_free(extension);
	}

	t = sprintf(p += 80, "ROWORDER= %20s / Image row order", "'TOP-DOWN'"); p[t] = ' ';
	t = sprintf(p += 80, "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy"); p[t] = ' ';
	t = sprintf(p += 80, "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H"); p[t] = ' ';
	t = sprintf(p += 80, "COMMENT   Converted from INDIGO RAW format. See www.indigo-astronomy.org"); p[t] = ' ';
	t = sprintf(p += 80, "END"); p[t] = ' ';
	p = buffer + FITS_RECORD_SIZE;
	if (components == 1) {
		// mono
		if (byte_per_pixel == 2) {
			// 16 bit RAW - swap endianness
			uint16_t *in = (uint16_t *)image;
			uint16_t *out = (uint16_t *)p;
			for (int i = 0; i < pixel_count; i++) {
				int value = *in++ - 32768;
				*out++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
			}
		} else {
			// 8 bit RAW
			memcpy(p, image, pixel_count);
		}
	} else {
		// RGB
		if (byte_per_pixel == 2) {
			uint16_t *out_ch0 = (uint16_t *)p;
			uint16_t *out_ch1 = (uint16_t *)p + pixel_count;
			uint16_t *out_ch2 = (uint16_t *)p + 2 * pixel_count;
			// 16 bit RGB - average and swap endianness
			uint16_t *in = (uint16_t *)image;
			for (int i = 0; i < pixel_count * 3; i++) {
				int value =  *in++ - 32768;
				*out_ch0++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				value =  *in++ - 32768;
				*out_ch1++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				value =  *in++ - 32768;
				*out_ch2++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
			}
		} else {
			uint8_t *out_ch0 = (uint8_t *)p;
			uint8_t *out_ch1 = (uint8_t *)p + pixel_count;
			uint8_t *out_ch2 = (uint8_t *)p + 2 * pixel_count;
			uint8_t *in = (uint8_t *)image;
			for (int i = 0; i < pixel_count; i++) {
				*out_ch0++ = *in++;
				*out_ch1++ = *in++;
				*out_ch2++ = *in++;
			}
		}
	}
	*fits = buffer;
	*fits_size = image_size;
	return INDIGO_OK;
}
