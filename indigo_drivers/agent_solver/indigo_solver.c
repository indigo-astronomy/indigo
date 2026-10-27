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

/** INDIGO quad based plate solver - catalog index, image quads, matching and WCS fit
 \file indigo_solver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>
#include <setjmp.h>

#include <jpeglib.h>

#include <indigo/indigo_timer.h>
#include <indigo/indigo_dslr_raw.h>

#include "indigo_solver_utils.h"
#include "indigo_solver.h"

const float indigo_solver_index_fovs[INDIGO_SOLVER_INDEX_FOV_COUNT] = { 1, 2, 5, 10, 20 };

// Hash buckets keyed by the quantised (x3, y3) code coordinates, both in [0, INDIGO_SOLVER_CODE_QUANTUM].
#define BUCKET_SIDE		(INDIGO_SOLVER_CODE_QUANTUM + 1)
#define BUCKET_COUNT	(BUCKET_SIDE * BUCKET_SIDE)

static int compare_quads(const void *a, const void *b) {
	const indigo_solver_quad *qa = (const indigo_solver_quad*)a;
	const indigo_solver_quad *qb = (const indigo_solver_quad*)b;
	int p = (qa->stars[0] > qb->stars[0]) - (qa->stars[0] < qb->stars[0]);
	if (p != 0) {
		return p;
	}
	p = (qa->stars[1] > qb->stars[1]) - (qa->stars[1] < qb->stars[1]);
	if (p != 0) {
		return p;
	}
	p = (qa->stars[2] > qb->stars[2]) - (qa->stars[2] < qb->stars[2]);
	if (p != 0) {
		return p;
	}
	return (qa->stars[3] > qb->stars[3]) - (qa->stars[3] < qb->stars[3]);
}

static int compare_quads_by_hash(const void *a, const void *b) {
	const indigo_solver_quad *qa = (const indigo_solver_quad*)a;
	const indigo_solver_quad *qb = (const indigo_solver_quad*)b;
	return (qa->hash > qb->hash) - (qa->hash < qb->hash);
}

static int compare_star_matches_by_votes(const void *a, const void *b) {
	const indigo_solver_star_match *ma = (const indigo_solver_star_match*)a;
	const indigo_solver_star_match *mb = (const indigo_solver_star_match*)b;
	return (mb->votes > ma->votes) - (mb->votes < ma->votes);
}

static bool fit_2d_line(const float *dx, const float *dy, const float *dz, int n, float *a_out, float *b_out, float *c_out) {
	if (n < 3) {
		return false;
	}
	// the normal equations are evaluated in double, in float the determinant of three points cancels to noise
	double sum1 = (double)n;
	double sum_dx = 0, sum_dy = 0, sum_dz = 0;
	double sum_dx2 = 0, sum_dy2 = 0, sum_dxdy = 0;
	double sum_dx_dz = 0, sum_dy_dz = 0;
	for (int i = 0; i < n; i++) {
		double x = dx[i];
		double y = dy[i];
		double z = dz[i];
		sum_dx += x;
		sum_dy += y;
		sum_dz += z;
		sum_dx2 += x * x;
		sum_dy2 += y * y;
		sum_dxdy += x * y;
		sum_dx_dz += x * z;
		sum_dy_dz += y * z;
	}
	// Compute determinant of the normal matrix
	double det = sum1 * (sum_dx2 * sum_dy2 - sum_dxdy * sum_dxdy) - sum_dx * (sum_dx * sum_dy2 - sum_dxdy * sum_dy) + sum_dy * (sum_dx * sum_dxdy - sum_dx2 * sum_dy);
	if (fabs(det) < 1e-14) {
		return false;
	}
	// Cramer's rule for a, b, c
	double det_a = sum_dz * (sum_dx2 * sum_dy2 - sum_dxdy * sum_dxdy) - sum_dx_dz * (sum_dx * sum_dy2 - sum_dxdy * sum_dy) + sum_dy_dz * (sum_dx * sum_dxdy - sum_dx2 * sum_dy);
	double det_b = sum1 * (sum_dx_dz * sum_dy2 - sum_dxdy * sum_dy_dz) - sum_dx * (sum_dz * sum_dy2 - sum_dy_dz * sum_dy) + sum_dy * (sum_dz * sum_dxdy - sum_dx_dz * sum_dy);
	double det_c = sum1 * (sum_dx2 * sum_dy_dz - sum_dx_dz * sum_dxdy) - sum_dx * (sum_dx * sum_dy_dz - sum_dx_dz * sum_dy) + sum_dz * (sum_dx * sum_dxdy - sum_dx2 * sum_dy);
	*a_out = det_a / det;
	*b_out = det_b / det;
	*c_out = det_c / det;
	return true;
}

void indigo_solver_bin_image(indigo_raw_header *image, int bin) {
	if (bin < 2) {
		return;
	}
	if (image->signature != INDIGO_RAW_MONO16) {
		indigo_debug("%s: only 16 bit mono images are binned, signature = %08x", __FUNCTION__, image->signature);
		return;
	}
	indigo_debug("%s: original image width = %d, height = %d", __FUNCTION__, image->width, image->height);
	uint16_t *src = (uint16_t *)(image + 1), *dst = src;
	int src_width = image->width, dst_width = src_width / bin;
	int src_height = image->height, dst_height = src_height / bin;
	for (int dst_y = 0; dst_y < dst_height; dst_y++) {
		int dst_y_bin = dst_y * bin;
		for (int dst_x = 0; dst_x < dst_width; dst_x++) {
			uint32_t sum = 0;
			int dst_x_bin = dst_x * bin;
			for (int by = 0; by < bin; by++) {
				int src_y = dst_y_bin + by;
				const uint16_t *row = src + src_y * src_width;
				for (int bx = 0; bx < bin; bx++) {
					sum += row[dst_x_bin + bx];
				}
			}
			dst[dst_y * dst_width + dst_x] = (uint16_t)(sum / (bin * bin));
		}
	}
	image->width = dst_width;
	image->height = dst_height;
	indigo_debug("%s: binned image width = %d, height = %d", __FUNCTION__, image->width, image->height);
}

// ---- input images: everything is turned into an INDIGO RAW MONO16 image, which is solved as it is

// The physical values of a FITS primary image (BITPIX 8, 16, 32, -32, -64 with BZERO/BSCALE, NAXIS 2 or 3 with the
// planes averaged), the dimensions and the value range; NULL with the reason when the data is not a usable image
#define FITS_RECORD 2880
#define FITS_CARD 80

static bool fits_card_value(const char *card, const char *keyword, double *value) {
	size_t length = strlen(keyword);
	if (strncmp(card, keyword, length) != 0 || card[length] != ' ' || card[8] != '=') {
		return false;
	}
	char buffer[FITS_CARD + 1];
	memcpy(buffer, card + 10, FITS_CARD - 10);
	buffer[FITS_CARD - 10] = 0;
	char *comment = strchr(buffer, '/');
	if (comment != NULL) {
		*comment = 0;
	}
	char *end;
	*value = strtod(buffer, &end);
	return end != buffer;
}

// Luminance weight of a colour channel of a three channel image, 1 for every channel of any other channel count
static float channel_weight(int channel, int channels) {
	static const float weights[3] = { 0.299f, 0.587f, 0.114f };
	return channels == 3 ? weights[channel] : 1.0f / channels;
}

static float *fits_values(const void *data, long size, int *width, int *height, float *min, float *max, bool *bayered, const char **message) {
	const char *fits = (const char *)data;
	double bitpix = 0, naxis = 0, naxis1 = 0, naxis2 = 0, naxis3 = 1, bzero = 0, bscale = 1;
	long offset = 0;
	bool end = false;
	while (!end) {
		if (offset + FITS_RECORD > size) {
			*message = "Truncated FITS header";
			return NULL;
		}
		for (int i = 0; i < FITS_RECORD / FITS_CARD && !end; i++) {
			const char *card = fits + offset + i * FITS_CARD;
			double value;
			if (strncmp(card, "END     ", 8) == 0) {
				end = true;
			} else if (strncmp(card, "BAYERPAT", 8) == 0) {
				*bayered = true;
			} else if (fits_card_value(card, "BITPIX", &value)) {
				bitpix = value;
			} else if (fits_card_value(card, "NAXIS", &value)) {
				naxis = value;
			} else if (fits_card_value(card, "NAXIS1", &value)) {
				naxis1 = value;
			} else if (fits_card_value(card, "NAXIS2", &value)) {
				naxis2 = value;
			} else if (fits_card_value(card, "NAXIS3", &value)) {
				naxis3 = value;
			} else if (fits_card_value(card, "BZERO", &value)) {
				bzero = value;
			} else if (fits_card_value(card, "BSCALE", &value)) {
				bscale = value;
			}
		}
		offset += FITS_RECORD;
	}
	int bytes = (int)fabs(bitpix) / 8;
	if ((bitpix != 8 && bitpix != 16 && bitpix != 32 && bitpix != -32 && bitpix != -64) || (naxis != 2 && naxis != 3) || naxis1 < 1 || naxis2 < 1 || naxis3 < 1 || naxis1 > 65535 || naxis2 > 65535 || naxis3 > 16) {
		*message = "Unsupported FITS image";
		return NULL;
	}
	int planes = (int)naxis3;
	*width = (int)naxis1;
	*height = (int)naxis2;
	size_t pixels = (size_t)*width * *height;
	if (offset + (long)(pixels * planes * bytes) > size) {
		*message = "Truncated FITS image";
		return NULL;
	}
	float *values = indigo_safe_malloc(pixels * sizeof(float));
	const uint8_t *p = (const uint8_t *)fits + offset;
	for (int plane = 0; plane < planes; plane++) {
		float weight = channel_weight(plane, planes);
		for (size_t i = 0; i < pixels; i++, p += bytes) {
			double value;
			switch ((int)bitpix) {
				case 8:
					value = p[0];
					break;
				case 16:
					value = (int16_t)((p[0] << 8) | p[1]);
					break;
				case 32:
					value = (int32_t)(((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]);
					break;
				case -32: {
					uint32_t bits = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
					float f;
					memcpy(&f, &bits, sizeof(f));
					value = f;
					break;
				}
				default: {
					uint64_t bits = 0;
					for (int k = 0; k < 8; k++) {
						bits = (bits << 8) | p[k];
					}
					double d;
					memcpy(&d, &bits, sizeof(d));
					value = d;
					break;
				}
			}
			value = bzero + bscale * value;
			if (!isfinite(value)) {
				value = 0;
			}
			values[i] = plane == 0 ? (float)value * weight : values[i] + (float)value * weight;
		}
	}
	*min = *max = values[0];
	for (size_t i = 0; i < pixels; i++) {
		*min = values[i] < *min ? values[i] : *min;
		*max = values[i] > *max ? values[i] : *max;
	}
	indigo_debug("%s: %d x %d, BITPIX %d, %d plane(s), values %g to %g", __FUNCTION__, *width, *height, (int)bitpix, planes, *min, *max);
	return values;
}

// A RAW MONO16 image of the given size with the pixels filled by the caller
static indigo_raw_header *mono16_image(int width, int height) {
	indigo_raw_header *image = indigo_safe_malloc(sizeof(indigo_raw_header) + (size_t)width * height * sizeof(uint16_t));
	image->signature = INDIGO_RAW_MONO16;
	image->width = width;
	image->height = height;
	return image;
}

struct solver_jpeg_decompress {
	struct jpeg_decompress_struct pub;
	jmp_buf error;
};

static void jpeg_error_callback(j_common_ptr cinfo) {
	longjmp(((struct solver_jpeg_decompress *)cinfo)->error, 1);
}

// A JPEG image as RAW MONO16, the components averaged and the 8 bit values spread over 16 bits
static indigo_raw_header *jpeg_image(const void *data, long size, const char **message) {
	struct solver_jpeg_decompress cinfo;
	struct jpeg_error_mgr jerr;
	indigo_raw_header *image = NULL;
	uint8_t *row = NULL;
	cinfo.pub.err = jpeg_std_error(&jerr);
	jerr.error_exit = jpeg_error_callback;
	if (setjmp(cinfo.error)) {
		jpeg_destroy_decompress(&cinfo.pub);
		indigo_safe_free(image);
		indigo_safe_free(row);
		*message = "Broken JPEG image";
		return NULL;
	}
	jpeg_create_decompress(&cinfo.pub);
	jpeg_mem_src(&cinfo.pub, (unsigned char *)data, (unsigned long)size);
	if (jpeg_read_header(&cinfo.pub, TRUE) != JPEG_HEADER_OK) {
		jpeg_destroy_decompress(&cinfo.pub);
		*message = "Broken JPEG image";
		return NULL;
	}
	jpeg_start_decompress(&cinfo.pub);
	int components = cinfo.pub.output_components;
	if (components < 1 || cinfo.pub.output_width < 1 || cinfo.pub.output_height < 1 || cinfo.pub.output_width > 65535 || cinfo.pub.output_height > 65535) {
		jpeg_destroy_decompress(&cinfo.pub);
		*message = "Unsupported JPEG image";
		return NULL;
	}
	int width = (int)cinfo.pub.output_width, height = (int)cinfo.pub.output_height;
	image = mono16_image(width, height);
	row = indigo_safe_malloc((size_t)width * components);
	uint16_t *out = (uint16_t *)(image + 1);
	while (cinfo.pub.output_scanline < cinfo.pub.output_height) {
		int y = cinfo.pub.output_scanline;
		JSAMPROW rows[1] = { row };
		jpeg_read_scanlines(&cinfo.pub, rows, 1);
		for (int x = 0; x < width; x++) {
			float sum = 0;
			for (int c = 0; c < components; c++) {
				sum += row[x * components + c] * channel_weight(c, components);
			}
			out[(size_t)y * width + x] = (uint16_t)(sum * 257 + 0.5f);
		}
	}
	jpeg_finish_decompress(&cinfo.pub);
	jpeg_destroy_decompress(&cinfo.pub);
	indigo_safe_free(row);
	indigo_debug("%s: %d x %d, %d component(s)", __FUNCTION__, width, height, components);
	return image;
}

// A camera raw file (anything libraw reads) as RAW MONO16
static indigo_raw_header *dslr_image(const void *data, long size, const char **message) {
	indigo_dslr_raw_image_s output = { 0 };
	if (indigo_dslr_raw_process_image((void *)data, (size_t)size, &output) != LIBRAW_SUCCESS || output.data == NULL || output.width < 1 || output.height < 1) {
		free(output.data);
		*message = "Unsupported image format";
		return NULL;
	}
	indigo_raw_header *image = mono16_image(output.width, output.height);
	size_t pixels = (size_t)output.width * output.height;
	uint16_t *out = (uint16_t *)(image + 1);
	if (output.bits == 16 && output.colors == 1 && output.size >= pixels * sizeof(uint16_t)) {
		memcpy(out, output.data, pixels * sizeof(uint16_t));
	} else if (output.bits == 8 && output.colors == 1 && output.size >= pixels) {
		for (size_t i = 0; i < pixels; i++) {
			out[i] = ((uint8_t *)output.data)[i] * 257;
		}
	} else if (output.bits == 16 && output.colors == 3 && output.size >= pixels * 3 * sizeof(uint16_t)) {
		const uint16_t *in = output.data;
		for (size_t i = 0; i < pixels; i++) {
			out[i] = (uint16_t)(in[3 * i] * channel_weight(0, 3) + in[3 * i + 1] * channel_weight(1, 3) + in[3 * i + 2] * channel_weight(2, 3) + 0.5f);
		}
	} else if (output.bits == 8 && output.colors == 3 && output.size >= pixels * 3) {
		const uint8_t *in = output.data;
		for (size_t i = 0; i < pixels; i++) {
			out[i] = (uint16_t)((in[3 * i] * channel_weight(0, 3) + in[3 * i + 1] * channel_weight(1, 3) + in[3 * i + 2] * channel_weight(2, 3)) * 257 + 0.5f);
		}
	} else {
		free(output.data);
		indigo_safe_free(image);
		*message = "Unsupported camera raw image";
		return NULL;
	}
	free(output.data);
	indigo_debug("%s: %d x %d, %d bit, %d color(s)", __FUNCTION__, output.width, output.height, output.bits, output.colors);
	return image;
}

// A RAW mono frame of a colour camera carries "BAYERPAT='RGGB'" in the text appended after its pixels
static bool raw_bayer_keyword(const char *extension, size_t length) {
	static const char keyword[] = "BAYERPAT";
	for (size_t i = 0; i + sizeof(keyword) - 1 <= length; i++) {
		if (memcmp(extension + i, keyword, sizeof(keyword) - 1) == 0) {
			return true;
		}
	}
	return false;
}

indigo_raw_header *indigo_solver_load_image(const void *data, long size, bool *bayered, const char **message) {
	uint32_t signature = 0;
	*bayered = false;
	if (size >= (long)sizeof(signature)) {
		memcpy(&signature, data, sizeof(signature));
	}
	if (signature == INDIGO_RAW_MONO8 || signature == INDIGO_RAW_MONO16 || signature == INDIGO_RAW_RGB24 || signature == INDIGO_RAW_RGB48) {
		const indigo_raw_header *header = (const indigo_raw_header *)data;
		int bytes = signature == INDIGO_RAW_MONO8 ? 1 : signature == INDIGO_RAW_MONO16 ? 2 : signature == INDIGO_RAW_RGB24 ? 3 : 6;
		if (size < (long)sizeof(indigo_raw_header) || header->width < 1 || header->height < 1 || size - (long)sizeof(indigo_raw_header) < (long)((size_t)header->width * header->height * bytes)) {
			*message = "Truncated RAW image";
			return NULL;
		}
		size_t pixels = (size_t)header->width * header->height;
		size_t data_size = sizeof(indigo_raw_header) + pixels * bytes;
		if (bytes <= 2) {
			*bayered = raw_bayer_keyword((const char *)data + data_size, size - data_size);
		}
		if (signature == INDIGO_RAW_MONO16) {
			return indigo_safe_malloc_copy(data_size, (void *)data);
		}
		indigo_raw_header *image = mono16_image(header->width, header->height);
		uint16_t *out = (uint16_t *)(image + 1);
		if (signature == INDIGO_RAW_MONO8) {
			const uint8_t *in = (const uint8_t *)(header + 1);
			for (size_t i = 0; i < pixels; i++) {
				out[i] = in[i] * 257;
			}
		} else if (signature == INDIGO_RAW_RGB24) {
			const uint8_t *in = (const uint8_t *)(header + 1);
			for (size_t i = 0; i < pixels; i++) {
				out[i] = (uint16_t)((in[3 * i] * channel_weight(0, 3) + in[3 * i + 1] * channel_weight(1, 3) + in[3 * i + 2] * channel_weight(2, 3)) * 257 + 0.5f);
			}
		} else {
			const uint16_t *in = (const uint16_t *)(header + 1);
			for (size_t i = 0; i < pixels; i++) {
				out[i] = (uint16_t)(in[3 * i] * channel_weight(0, 3) + in[3 * i + 1] * channel_weight(1, 3) + in[3 * i + 2] * channel_weight(2, 3) + 0.5f);
			}
		}
		return image;
	}
	if (size >= 9 && strncmp((const char *)data, "SIMPLE  =", 9) == 0) {
		int width, height;
		float min, max;
		float *values = fits_values(data, size, &width, &height, &min, &max, bayered, message);
		if (values == NULL) {
			return NULL;
		}
		// physical values within 16 bits are kept, any other range is mapped onto them
		float scale = 1, shift = 0;
		if (min < 0 || max > 65535) {
			scale = max > min ? 65535 / (max - min) : 1;
			shift = min;
		}
		indigo_raw_header *image = mono16_image(width, height);
		uint16_t *out = (uint16_t *)(image + 1);
		size_t pixels = (size_t)width * height;
		for (size_t i = 0; i < pixels; i++) {
			out[i] = (uint16_t)((values[i] - shift) * scale + 0.5f);
		}
		indigo_safe_free(values);
		return image;
	}
	if (size >= 3 && ((const uint8_t *)data)[0] == 0xFF && ((const uint8_t *)data)[1] == 0xD8 && ((const uint8_t *)data)[2] == 0xFF) {
		return jpeg_image(data, size, message);
	}
	return dslr_image(data, size, message);
}

// Star detection works on a copy of the frame with its defects replaced and a matched filter applied: a
// faint star spread over a few pixels is barely above the single pixel noise, after a gaussian blur of the
// width of the star image its peak stands out while the noise averages down. Hot pixels, columns and rows
// must go before the blur, which would turn them into star like blobs.
#define FILTER_SIGMA 1.0
#define FILTER_RADIUS 3
#define DEFECT_MIN_SIGMAS 10
#define DEFECT_NEIGHBOUR_RATIO 0.1

// Standard deviation of the pixel noise: the median absolute difference of horizontally adjacent pixels,
// which stars and gradients hardly affect, scaled from the median of |N(0, 2 sigma²)| to sigma, and at least
// the quantisation step of the data (the smallest non-zero difference), below which 8 bit or JPEG data has no
// noise to measure and a single step would look like a spike
static float image_noise_sigma(const uint16_t *data, int width, int height) {
	uint32_t *histogram = indigo_safe_malloc(65536 * sizeof(uint32_t));
	uint32_t count = 0;
	for (int y = 0; y < height; y++) {
		const uint16_t *row = data + (size_t)y * width;
		for (int x = 1; x < width; x++) {
			histogram[abs((int)row[x] - (int)row[x - 1])]++;
			count++;
		}
	}
	uint32_t cumulative = 0;
	int median = 0, quantum = 0;
	for (int i = 0; i < 65536; i++) {
		cumulative += histogram[i];
		if (quantum == 0 && i > 0 && histogram[i] > 0) {
			quantum = i;
		}
		if (median == 0 && cumulative * 2 >= count) {
			median = i;
		}
	}
	indigo_safe_free(histogram);
	float sigma = median / (0.6745 * M_SQRT2);
	sigma = sigma < quantum ? quantum : sigma;
	return sigma < 1 ? 1 : sigma;
}

static int compare_uint16(const void *a, const void *b) {
	return (int)*(const uint16_t *)a - (int)*(const uint16_t *)b;
}

// Replaces defective pixels by the median of their 8 neighbours. A pixel is defective when it stands far above
// the lowest of them and its horizontal or its vertical neighbours carry almost nothing of that excess: a hot
// pixel has no such neighbours, a hot column or row pixel none on one axis, a star image has them on both axes,
// on its flanks as well as at its peak. Where a hot column crosses a hot row or a hot pixel touches one, the
// pixel has hot neighbours on both axes and survives the first pass; the passes repeat until nothing is left.
#define DEFECT_PASSES 4

static int remove_defects(const uint16_t *src, uint16_t *dst, int width, int height, float sigma) {
	size_t size = (size_t)width * height;
	uint16_t *work = indigo_safe_malloc(size * sizeof(uint16_t));
	memcpy(dst, src, size * sizeof(uint16_t));
	float min_excess = DEFECT_MIN_SIGMAS * sigma;
	int removed = 0;
	for (int pass = 0; pass < DEFECT_PASSES; pass++) {
		int replaced = 0;
		memcpy(work, dst, size * sizeof(uint16_t));
		for (int y = 1; y < height - 1; y++) {
			const uint16_t *row = work + (size_t)y * width;
			for (int x = 1; x < width - 1; x++) {
				uint16_t neighbours[8] = { row[x - width - 1], row[x - width], row[x - width + 1], row[x - 1], row[x + 1], row[x + width - 1], row[x + width], row[x + width + 1] };
				uint16_t lowest = neighbours[0];
				for (int k = 1; k < 8; k++) {
					lowest = neighbours[k] < lowest ? neighbours[k] : lowest;
				}
				float excess = row[x] - lowest;
				if (excess < min_excess) {
					continue;
				}
				float horizontal = (row[x - 1] > row[x + 1] ? row[x - 1] : row[x + 1]) - lowest;
				float vertical = (row[x - width] > row[x + width] ? row[x - width] : row[x + width]) - lowest;
				if (horizontal < DEFECT_NEIGHBOUR_RATIO * excess || vertical < DEFECT_NEIGHBOUR_RATIO * excess) {
					qsort(neighbours, 8, sizeof(uint16_t), compare_uint16);
					dst[(size_t)y * width + x] = (uint16_t)((neighbours[3] + neighbours[4]) / 2);
					replaced++;
				}
			}
		}
		removed += replaced;
		if (replaced == 0) {
			break;
		}
	}
	indigo_safe_free(work);
	return removed;
}

// Separable gaussian blur, the edges repeat the border pixel
static void blur_image(const uint16_t *src, uint16_t *dst, int width, int height) {
	float kernel[2 * FILTER_RADIUS + 1], sum = 0;
	for (int k = -FILTER_RADIUS; k <= FILTER_RADIUS; k++) {
		kernel[k + FILTER_RADIUS] = expf(-k * k / (2 * FILTER_SIGMA * FILTER_SIGMA));
		sum += kernel[k + FILTER_RADIUS];
	}
	for (int k = 0; k <= 2 * FILTER_RADIUS; k++) {
		kernel[k] /= sum;
	}
	float *tmp = indigo_safe_malloc((size_t)width * height * sizeof(float));
	for (int y = 0; y < height; y++) {
		const uint16_t *row = src + (size_t)y * width;
		float *out = tmp + (size_t)y * width;
		for (int x = 0; x < width; x++) {
			float value = 0;
			for (int k = -FILTER_RADIUS; k <= FILTER_RADIUS; k++) {
				int xx = x + k;
				xx = xx < 0 ? 0 : xx >= width ? width - 1 : xx;
				value += kernel[k + FILTER_RADIUS] * row[xx];
			}
			out[x] = value;
		}
	}
	for (int y = 0; y < height; y++) {
		uint16_t *out = dst + (size_t)y * width;
		for (int x = 0; x < width; x++) {
			float value = 0;
			for (int k = -FILTER_RADIUS; k <= FILTER_RADIUS; k++) {
				int yy = y + k;
				yy = yy < 0 ? 0 : yy >= height ? height - 1 : yy;
				value += kernel[k + FILTER_RADIUS] * tmp[(size_t)yy * width + x];
			}
			out[x] = (uint16_t)(value + 0.5f);
		}
	}
	indigo_safe_free(tmp);
}

// The filtered copy of a mono frame as 16 bit data, NULL for other frame types
static uint16_t *filtered_image(const indigo_raw_header *image) {
	size_t size = (size_t)image->width * image->height;
	if (image->width < 3 || image->height < 3) {
		return NULL;
	}
	uint16_t *mono = indigo_safe_malloc(size * sizeof(uint16_t));
	if (image->signature == INDIGO_RAW_MONO16) {
		memcpy(mono, image + 1, size * sizeof(uint16_t));
	} else if (image->signature == INDIGO_RAW_MONO8) {
		const uint8_t *data = (const uint8_t *)(image + 1);
		for (size_t i = 0; i < size; i++) {
			mono[i] = data[i];
		}
	} else {
		indigo_safe_free(mono);
		return NULL;
	}
	uint16_t *cleaned = indigo_safe_malloc(size * sizeof(uint16_t));
	float sigma = image_noise_sigma(mono, image->width, image->height);
	int removed = remove_defects(mono, cleaned, image->width, image->height, sigma);
	blur_image(cleaned, mono, image->width, image->height);
	indigo_safe_free(cleaned);
	indigo_debug("%s: noise sigma %.1f, %d defective pixels replaced", __FUNCTION__, sigma, removed);
	return mono;
}

int indigo_solver_image_stars(indigo_raw_header *image, indigo_solver_star *stars, int max_star_count) {
	indigo_star_detection *all_stars = indigo_safe_malloc(sizeof(indigo_star_detection) * max_star_count);
	int stars_found, count = 0;
	uint16_t *filtered = filtered_image(image);
	if (filtered != NULL) {
		indigo_find_stars_precise_threshold(INDIGO_RAW_MONO16, filtered, 16, 3.5, image->width, image->height, max_star_count, all_stars, &stars_found);
		indigo_safe_free(filtered);
	} else {
		indigo_find_stars_precise_threshold(image->signature, image + 1, 16, 3.5, image->width, image->height, max_star_count, all_stars, &stars_found);
	}
	for (int i = 0; i < stars_found; i++) {
		indigo_star_detection *as = all_stars + i;
		indigo_solver_star *s = stars + (count++);
		s->magnitude = 1 / as->luminance;
		s->x = as->x;
		s->y = as->y;
		s->ra = s->dec = 0;
		s->used = 0;
	}
	indigo_safe_free(all_stars);
	indigo_debug("%s: detected %d image stars", __FUNCTION__, count);
	return count;
}

// Picks the largest available index whose FOV is at most 1.5 x the geometric mean of the field size
// (enough index stars in the field while quads still fit inside it); falls back to the smallest available index.
bool indigo_solver_select_index(const char *index_directory, const float *fovs, int fov_count, float field_width, float field_height, char *path, size_t size, float *fov_out) {
	float target = 1.5 * sqrt(field_width * field_height) * RAD_TO_DEG;
	float selected = 0;
	char candidate[PATH_MAX];
	for (int i = 0; i < fov_count; i++) {
		float fov = fovs[i];
		indigo_solver_index_path(candidate, sizeof(candidate), index_directory, fov);
		if (access(candidate, R_OK) == 0 && (selected == 0 || fov <= target)) {
			selected = fov;
			strncpy(path, candidate, size);
		}
	}
	if (selected == 0) {
		indigo_error("%s: no tyc2-*deg.isix index file found in %s", __FUNCTION__, index_directory && index_directory[0] ? index_directory : "the working directory");
		return false;
	}
	*fov_out = selected;
	indigo_debug("%s: field %.2f° x %.2f°, using %s", __FUNCTION__, field_width * RAD_TO_DEG, field_height * RAD_TO_DEG, path);
	return true;
}

bool indigo_solver_load_index(const char *path, indigo_solver_star **stars, int *star_count, indigo_solver_quad **quads, int *quad_count) {
	*stars = NULL;
	*star_count = 0;
	*quads = NULL;
	*quad_count = 0;
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		indigo_error("%s: failed to open %s", __FUNCTION__, path);
		return false;
	}
	indigo_solve_index_header header;
	if (!indigo_solver_index_read_header(file, &header)) {
		fclose(file);
		return false;
	}
	indigo_solver_index_star *index_stars = indigo_safe_malloc(sizeof(indigo_solver_index_star) * header.star_count);
	indigo_solver_index_quad *index_quads = indigo_safe_malloc(sizeof(indigo_solver_index_quad) * header.quad_count);
	bool ok = indigo_solver_index_read_stars(file, index_stars, header.star_count) && indigo_solver_index_read_quads(file, index_quads, header.quad_count);
	fclose(file);
	if (!ok) {
		indigo_error("%s: %s is truncated", __FUNCTION__, path);
		indigo_safe_free(index_stars);
		indigo_safe_free(index_quads);
		return false;
	}
	indigo_solver_star *s = indigo_safe_malloc(sizeof(indigo_solver_star) * header.star_count);
	memset(s, 0, sizeof(indigo_solver_star) * header.star_count);
	for (uint32_t i = 0; i < header.star_count; i++) {
		s[i].id = i;
		s[i].ra = index_stars[i].ra;
		s[i].dec = index_stars[i].dec;
	}
	indigo_safe_free(index_stars);
	indigo_solver_quad *q = indigo_safe_malloc(sizeof(indigo_solver_quad) * header.quad_count);
	memset(q, 0, sizeof(indigo_solver_quad) * header.quad_count);
	bool sorted = true;
	for (uint32_t i = 0; i < header.quad_count; i++) {
		indigo_solver_index_quad *iq = index_quads + i;
		for (int k = 0; k < 4; k++) {
			if (iq->stars[k] >= header.star_count) {
				indigo_error("%s: %s is corrupted (quad %u references star %u of %u)", __FUNCTION__, path, i, iq->stars[k], header.star_count);
				indigo_safe_free(index_quads);
				indigo_safe_free(s);
				indigo_safe_free(q);
				return false;
			}
			q[i].stars[k] = s + iq->stars[k];
		}
		q[i].hash = iq->hash;
		q[i].baseline = iq->baseline;
		int hx3, hy3, hx4, hy4;
		indigo_solver_quad_hash_components(iq->hash, &hx3, &hy3, &hx4, &hy4);
		q[i].x3 = (float)hx3 / INDIGO_SOLVER_CODE_QUANTUM;
		q[i].y3 = (float)hy3 / INDIGO_SOLVER_CODE_QUANTUM;
		q[i].x4 = (float)hx4 / INDIGO_SOLVER_CODE_QUANTUM;
		q[i].y4 = (float)hy4 / INDIGO_SOLVER_CODE_QUANTUM;
		if (i > 0 && q[i - 1].hash > q[i].hash) {
			sorted = false;
		}
	}
	indigo_safe_free(index_quads);
	if (!sorted) {
		indigo_debug("%s: %s is not sorted by hash, sorting", __FUNCTION__, path);
		qsort(q, header.quad_count, sizeof(indigo_solver_quad), compare_quads_by_hash);
	}
	*stars = s;
	*star_count = header.star_count;
	*quads = q;
	*quad_count = header.quad_count;
	indigo_debug("%s: using %d catalog stars and %d catalog quads from %s (%g°)", __FUNCTION__, header.star_count, header.quad_count, path, header.fov);
	return true;
}

bool indigo_solver_catalog(const char *index_directory, float field_width, float field_height, indigo_solver_star **stars, int *star_count, indigo_solver_quad **quads, int *quad_count) {
	char path[PATH_MAX];
	float fov;
	if (!indigo_solver_select_index(index_directory, indigo_solver_index_fovs, INDIGO_SOLVER_INDEX_FOV_COUNT, field_width, field_height, path, sizeof(path), &fov)) {
		*stars = NULL;
		*star_count = 0;
		*quads = NULL;
		*quad_count = 0;
		return false;
	}
	return indigo_solver_load_index(path, stars, star_count, quads, quad_count);
}

void indigo_solver_free_catalog(indigo_solver_star *stars, indigo_solver_quad *quads) {
	indigo_safe_free(quads);
	indigo_safe_free(stars);
}

int indigo_solver_build_image_quads(indigo_solver_star *stars, int star_count, int width, int height, int cell_size, int *used_stars, indigo_solver_quad *quads, int max_quad_count) {
	int cell_cols = width / cell_size;
	int cell_rows = height / cell_size;
	struct cell {
		int row, col;
		indigo_solver_star *star;
	} cells[cell_rows][cell_cols];
	for (int j = 0; j < cell_rows; j++) {
		for (int i = 0; i < cell_cols; i++) {
			cells[j][i].row = j;
			cells[j][i].col = i;
			cells[j][i].star = NULL;
		}
	}
	for (int i = 0; i < star_count; i++) {
		indigo_solver_star *star = stars + i;
		int x = star->x / cell_size;
		int y = star->y / cell_size;
		if (x < cell_cols && y < cell_rows && (cells[y][x].star == NULL || cells[y][x].star->magnitude > star->magnitude)) {
			cells[y][x].star = star;
		}
	}
	int count = 0;
	for (int j = 0; j < cell_rows; j++) {
		for (int i = 0; i < cell_cols; i++) {
			indigo_solver_star *star = cells[j][i].star;
			if (star) {
				star->used++;
				count++;
			}
		}
	}
	*used_stars = count;
	count = 0;
	int cell_count = cell_cols * cell_rows;
	struct cell *c = (struct cell *)cells;
	for (int i = 0; i < cell_count; i++) {
		if (c[i].star == NULL) {
			continue;
		}
		for (int j = i + 1; j < cell_count; j++) {
			if (c[j].star == NULL) {
				continue;
			}
			for (int k = j + 1; k < cell_count; k++) {
				if (c[k].star == NULL) {
					continue;
				}
				for (int l = k + 1; l < cell_count; l++) {
					if (c[l].star == NULL) {
						continue;
					}
					if (count == max_quad_count) {
						goto exit;
					}
					indigo_solver_make_quad(quads + count, c[i].star, c[j].star, c[k].star, c[l].star);
					count++;
				}
			}
		}
	}
exit:
	qsort(quads, count, sizeof(indigo_solver_quad), compare_quads);
	indigo_debug("%s: %d stars used to form %d quads", __FUNCTION__, *used_stars, count);
	return count;
}

int indigo_solver_filter_quads(indigo_solver_quad *quads, int quad_count, float ra, float dec, float radius, indigo_solver_quad **filtered) {
	int count = 0;
	*filtered = NULL;
	for (int pass = 0; pass < 2; pass++) {
		count = 0;
		for (int i = 0; i < quad_count; i++) {
			indigo_solver_quad *quad = quads + i;
			bool inside = true;
			for (int k = 0; k < 4 && inside; k++) {
				inside = indigo_angular_distance(ra, dec, quad->stars[k]->ra, quad->stars[k]->dec) <= radius;
			}
			if (inside) {
				if (pass) {
					(*filtered)[count] = *quad;
				}
				count++;
			}
		}
		if (count == 0) {
			break;
		}
		if (pass == 0) {
			*filtered = indigo_safe_malloc(count * sizeof(indigo_solver_quad));
		}
	}
	indigo_debug("%s: %d of %d catalog quads within %.2f° of RA %.3f° Dec %.3f°", __FUNCTION__, count, quad_count, radius * RAD_TO_DEG, ra * RAD_TO_DEG, dec * RAD_TO_DEG);
	return count;
}

// Checked every LIMITS_CHECK_INTERVAL iterations of the matching loops
#define LIMITS_CHECK_INTERVAL 4096

static bool limits_exceeded(indigo_solver_limits *limits) {
	if (limits == NULL) {
		return false;
	}
	if (limits->exceeded) {
		return true;
	}
	if (limits->abort != NULL && *limits->abort) {
		limits->exceeded = true;
		limits->reason = "Aborted";
	} else if (limits->deadline > 0 && indigo_monotonic_time() > limits->deadline) {
		limits->exceeded = true;
		limits->reason = "CPU time limit exceeded";
	}
	return limits->exceeded;
}

static inline int bucket_key(int hx3, int hy3) {
	return hx3 * BUCKET_SIDE + hy3;
}

// starts[key] .. starts[key + 1] is the range of catalog quads with the given (x3, y3) bucket; the catalog is hash-sorted,
// so the quads of one bucket are contiguous.
static uint32_t *build_buckets(indigo_solver_quad *catalog_quads, int catalog_quad_count) {
	uint32_t *starts = indigo_safe_malloc((BUCKET_COUNT + 1) * sizeof(uint32_t));
	memset(starts, 0, (BUCKET_COUNT + 1) * sizeof(uint32_t));
	for (int i = 0; i < catalog_quad_count; i++) {
		int hx3, hy3, hx4, hy4;
		indigo_solver_quad_hash_components(catalog_quads[i].hash, &hx3, &hy3, &hx4, &hy4);
		starts[bucket_key(hx3, hy3) + 1]++;
	}
	for (int k = 1; k <= BUCKET_COUNT; k++) {
		starts[k] += starts[k - 1];
	}
	return starts;
}

int indigo_solver_match_quads(indigo_solver_quad *image_quads, int image_quad_count, indigo_solver_quad *catalog_quads, int catalog_quad_count, indigo_solver_quad_match *matches, int max_match_count, indigo_solver_limits *limits) {
	int count_by_hash = 0, count_by_scale = 0;
	if (limits_exceeded(limits)) {
		return 0;
	}
	for (int i = 1; i < catalog_quad_count; i++) {
		if (catalog_quads[i - 1].hash > catalog_quads[i].hash) {
			indigo_debug("%s: catalog quads are not sorted by hash, sorting", __FUNCTION__);
			qsort(catalog_quads, catalog_quad_count, sizeof(indigo_solver_quad), compare_quads_by_hash);
			break;
		}
	}
	uint32_t *starts = build_buckets(catalog_quads, catalog_quad_count);
	const int t = INDIGO_SOLVER_HASH_TOLERANCE;
	for (int i = 0; i < image_quad_count; i++) {
		if (i % LIMITS_CHECK_INTERVAL == 0 && limits_exceeded(limits)) {
			indigo_safe_free(starts);
			return 0;
		}
		indigo_solver_quad *image_quad = image_quads + i;
		int hx3, hy3, hx4, hy4;
		indigo_solver_quad_hash_components(image_quad->hash, &hx3, &hy3, &hx4, &hy4);
		for (int x3 = hx3 - t; x3 <= hx3 + t; x3++) {
			if (x3 < 0 || x3 >= BUCKET_SIDE) {
				continue;
			}
			for (int y3 = hy3 - t; y3 <= hy3 + t; y3++) {
				if (y3 < 0 || y3 >= BUCKET_SIDE) {
					continue;
				}
				int key = bucket_key(x3, y3);
				for (uint32_t j = starts[key]; j < starts[key + 1]; j++) {
					indigo_solver_quad *catalog_quad = catalog_quads + j;
					int cx3, cy3, cx4, cy4;
					indigo_solver_quad_hash_components(catalog_quad->hash, &cx3, &cy3, &cx4, &cy4);
					if (abs(cx4 - hx4) > t || abs(cy4 - hy4) > t) {
						continue;
					}
					if (count_by_hash == max_match_count) {
						indigo_debug("%s: match limit %d reached", __FUNCTION__, max_match_count);
						goto exit;
					}
					indigo_solver_quad_match *match = matches + count_by_hash;
					match->image_quad = image_quad;
					match->catalog_quad = catalog_quad;
					match->scale = image_quad->baseline / catalog_quad->baseline;
					match->votes = 0;
					count_by_hash++;
				}
			}
		}
	}
exit:
	indigo_safe_free(starts);
	indigo_debug("%s: %d quads matched by hash (tolerance +-%d)", __FUNCTION__, count_by_hash, t);
	// Each pair of matches sharing an (image star, catalog star) correspondence votes for both.
	float best_scale = 0;
	int best_votes = 0;
	for (int i = 0; i < count_by_hash; i++) {
		if (i % (LIMITS_CHECK_INTERVAL / 64) == 0 && limits_exceeded(limits)) {
			return 0;
		}
		indigo_solver_quad_match *qmi = matches + i;
		for (int j = i + 1; j < count_by_hash; j++) {
			indigo_solver_quad_match *qmj = matches + j;
			for (int ii = 0; ii < 4; ii++) {
				indigo_solver_star *isi = qmi->image_quad->stars[ii];
				indigo_solver_star *csi = qmi->catalog_quad->stars[ii];
				for (int jj = 0; jj < 4; jj++) {
					indigo_solver_star *isj = qmj->image_quad->stars[jj];
					indigo_solver_star *csj = qmj->catalog_quad->stars[jj];
					if (isi == isj && csi == csj) {
						qmi->votes++;
						qmj->votes++;
						goto next;
					}
				}
			}
		next:
			;
		}
		if (best_votes < qmi->votes) {
			best_votes = qmi->votes;
			best_scale = qmi->scale;
		}
	}
	indigo_debug("%s: %d votes for scale %g", __FUNCTION__, best_votes, best_scale);
	if (best_scale == 0) {
		return 0;
	}
	for (int i = 0; i < count_by_hash; i++) {
		float scale = matches[i].scale;
		if (fabs(1 - scale / best_scale) < INDIGO_SOLVER_SCALE_TOLERANCE) {
			matches[count_by_scale++] = matches[i];
		}
	}
	indigo_debug("%s: %d quads matched by scale (tolerance %g)", __FUNCTION__, count_by_scale, INDIGO_SOLVER_SCALE_TOLERANCE);
	return count_by_scale;
}

int indigo_solver_match_stars(indigo_solver_quad_match *quad_matches, int quad_match_count, indigo_solver_star_match *star_matches, int max_star_match_count, indigo_solver_limits *limits) {
	int count = 0, accepted = 0;
	// 1) Accumulate votes per (image star, catalog star) correspondence
	for (int i = 0; i < quad_match_count; i++) {
		if (i % LIMITS_CHECK_INTERVAL == 0 && limits_exceeded(limits)) {
			return 0;
		}
		indigo_solver_quad_match *quad_match = quad_matches + i;
		for (int j = 0; j < 4; j++) {
			indigo_solver_star *image_star = quad_match->image_quad->stars[j];
			indigo_solver_star *catalog_star = quad_match->catalog_quad->stars[j];
			bool add = true;
			for (int k = 0; k < count; k++) {
				indigo_solver_star_match *star_match = star_matches + k;
				if (star_match->image_star == image_star && star_match->catalog_star == catalog_star) {
					star_match->votes++;
					add = false;
					break;
				}
			}
			if (add) {
				if (count == max_star_match_count) {
					goto exit;
				}
				indigo_solver_star_match *star_match = star_matches + count;
				star_match->image_star = image_star;
				star_match->catalog_star = catalog_star;
				star_match->votes = 1;
				count++;
			}
		}
	}
exit:
	indigo_debug("%s: %d stars matched", __FUNCTION__, count);
	// 2) Best voted correspondences first, each image star and each catalog star used at most once
	qsort(star_matches, count, sizeof(indigo_solver_star_match), compare_star_matches_by_votes);
	for (int i = 0; i < count; i++) {
		bool conflict = false;
		for (int j = 0; j < accepted; j++) {
			if (star_matches[i].image_star == star_matches[j].image_star || star_matches[i].catalog_star == star_matches[j].catalog_star) {
				conflict = true;
				break;
			}
		}
		if (!conflict) {
			star_matches[accepted++] = star_matches[i];
		}
	}
	indigo_debug("%s: %d stars matched by votes", __FUNCTION__, accepted);
	return accepted;
}

#define FIT_MAX_ITERATIONS	20
#define FIT_MAX_RESIDUAL_PX	3.0
#define FIT_MIN_INLIERS			6
#define FIT_SEED_CANDIDATES	8

typedef struct {
	float a0, a1, a2;	// xi  ≈ a0 + a1 · Δx + a2 · Δy
	float b0, b1, b2;	// eta ≈ b0 + b1 · Δx + b2 · Δy
	float pixel;			// radians per pixel
	float rms;				// residual rms on the tangent plane, radians
	int n;
} affine_fit;

typedef struct {
	int count;
	const float *dx, *dy, *xi, *eta;
	const bool *valid;
	float *fx, *fy, *fxi, *feta;	// scratch
} fit_data;

static bool fit_affine(const fit_data *d, const bool *inlier, affine_fit *fit) {
	int n = 0;
	for (int i = 0; i < d->count; i++) {
		if (inlier[i]) {
			d->fx[n] = d->dx[i];
			d->fy[n] = d->dy[i];
			d->fxi[n] = d->xi[i];
			d->feta[n] = d->eta[i];
			n++;
		}
	}
	if (n < 3) {
		return false;
	}
	if (!fit_2d_line(d->fx, d->fy, d->fxi, n, &fit->a0, &fit->a1, &fit->a2) || !fit_2d_line(d->fx, d->fy, d->feta, n, &fit->b0, &fit->b1, &fit->b2)) {
		return false;
	}
	fit->pixel = (hypot(fit->a1, fit->b1) + hypot(fit->a2, fit->b2)) / 2.0;
	float sum2 = 0;
	for (int i = 0; i < d->count; i++) {
		if (inlier[i]) {
			float rx = d->xi[i] - (fit->a0 + fit->a1 * d->dx[i] + fit->a2 * d->dy[i]);
			float ry = d->eta[i] - (fit->b0 + fit->b1 * d->dx[i] + fit->b2 * d->dy[i]);
			sum2 += rx * rx + ry * ry;
		}
	}
	fit->rms = sqrt(sum2 / n);
	fit->n = n;
	return fit->pixel > 0 && isfinite(fit->rms);
}

// Starting from the seed inliers, refits and re-selects every match within FIT_MAX_RESIDUAL_PX of the fit until stable.
// The tolerance is absolute, a sigma based one would let a wrong seed accept everything. Returns the inlier count,
// 0 when the fit degenerates.
static int grow_fit(const fit_data *d, bool *inlier, affine_fit *fit) {
	for (int iteration = 0; iteration < FIT_MAX_ITERATIONS; iteration++) {
		if (!fit_affine(d, inlier, fit)) {
			return 0;
		}
		float limit = FIT_MAX_RESIDUAL_PX * fit->pixel;
		bool changed = false;
		int n = 0;
		for (int i = 0; i < d->count; i++) {
			float rx = d->xi[i] - (fit->a0 + fit->a1 * d->dx[i] + fit->a2 * d->dy[i]);
			float ry = d->eta[i] - (fit->b0 + fit->b1 * d->dx[i] + fit->b2 * d->dy[i]);
			bool in = d->valid[i] && hypot(rx, ry) <= limit;
			if (in != inlier[i]) {
				changed = true;
			}
			inlier[i] = in;
			n += in;
		}
		if (n < 3) {
			return 0;
		}
		if (!changed) {
			return n;
		}
	}
	return fit_affine(d, inlier, fit) ? fit->n : 0;
}

bool indigo_solver_compute_center(indigo_solver_star_match *matches, int match_count, int width, int height, float *ra_center, float *dec_center, float *rotation, float *scale, int *parity, int *matched, float *rms) {
	if (match_count < 3) {
		return false;
	}
	// 1. Best voted correspondences first; they seed the robust fit
	indigo_solver_star_match *sorted = indigo_safe_malloc(match_count * sizeof(indigo_solver_star_match));
	memcpy(sorted, matches, match_count * sizeof(indigo_solver_star_match));
	qsort(sorted, match_count, sizeof(indigo_solver_star_match), compare_star_matches_by_votes);
	// 2. Image offsets from the image centre
	float cx = width / 2.0, cy = height / 2.0;
	float *dx = indigo_safe_malloc(match_count * sizeof(float));
	float *dy = indigo_safe_malloc(match_count * sizeof(float));
	float *xi = indigo_safe_malloc(match_count * sizeof(float));
	float *eta = indigo_safe_malloc(match_count * sizeof(float));
	bool *valid = indigo_safe_malloc(match_count * sizeof(bool));
	bool *inlier = indigo_safe_malloc(match_count * sizeof(bool));
	bool *best_inlier = indigo_safe_malloc(match_count * sizeof(bool));
	fit_data data = {
		.count = match_count, .dx = dx, .dy = dy, .xi = xi, .eta = eta, .valid = valid,
		.fx = indigo_safe_malloc(match_count * sizeof(float)), .fy = indigo_safe_malloc(match_count * sizeof(float)),
		.fxi = indigo_safe_malloc(match_count * sizeof(float)), .feta = indigo_safe_malloc(match_count * sizeof(float))
	};
	for (int i = 0; i < match_count; i++) {
		dx[i] = sorted[i].image_star->x - cx;
		dy[i] = sorted[i].image_star->y - cy;
	}
	// 3. The tangent point is the catalog star of one of the best voted matches (a wrong match far from the field
	//    would put every true match on the wrong side of the sphere); every triple among the best voted matches seeds
	//    an affine fit that grows over consistent matches. The largest consistent set wins, so wrong correspondences
	//    (even half of them) do not pull the solution.
	int candidates = match_count < FIT_SEED_CANDIDATES ? match_count : FIT_SEED_CANDIDATES;
	affine_fit fit, best_fit;
	int best_n = 0;
	float ref_ra = 0, ref_dec = 0;
	for (int ref = 0; ref < candidates; ref++) {
		float ra0 = sorted[ref].catalog_star->ra;
		float dec0 = sorted[ref].catalog_star->dec;
		for (int i = 0; i < match_count; i++) {
			indigo_solver_star *cat = sorted[i].catalog_star;
			valid[i] = indigo_radec_to_tangent(ra0, dec0, cat->ra, cat->dec, xi + i, eta + i);
		}
		for (int i = 0; i < candidates; i++) {
			for (int j = i + 1; j < candidates; j++) {
				for (int k = j + 1; k < candidates; k++) {
					if (!valid[i] || !valid[j] || !valid[k]) {
						continue;
					}
					memset(inlier, 0, match_count * sizeof(bool));
					inlier[i] = inlier[j] = inlier[k] = true;
					int n = grow_fit(&data, inlier, &fit);
					if (n > best_n || (n == best_n && n > 0 && fit.rms < best_fit.rms)) {
						best_n = n;
						best_fit = fit;
						ref_ra = ra0;
						ref_dec = dec0;
						memcpy(best_inlier, inlier, match_count * sizeof(bool));
					}
				}
			}
		}
	}
	// 4. The rotation of a fit is measured against north at its tangent point, which is up to half a field away from
	//    the image centre; at high declination the meridians converge by a degree over that distance. The centre found
	//    with the reference star becomes the tangent point of a second fit over the same inliers, so the angle, the
	//    scale and the final centre refer to the image centre.
	if (best_n >= FIT_MIN_INLIERS) {
		float ra0, dec0;
		indigo_tangent_to_radec(ref_ra, ref_dec, best_fit.a0, best_fit.b0, &ra0, &dec0);
		for (int i = 0; i < match_count; i++) {
			indigo_solver_star *cat = sorted[i].catalog_star;
			valid[i] = indigo_radec_to_tangent(ra0, dec0, cat->ra, cat->dec, xi + i, eta + i);
		}
		memcpy(inlier, best_inlier, match_count * sizeof(bool));
		int n = grow_fit(&data, inlier, &fit);
		if (n >= FIT_MIN_INLIERS) {
			best_n = n;
			best_fit = fit;
			ref_ra = ra0;
			ref_dec = dec0;
		}
	}
	indigo_safe_free(dx);
	indigo_safe_free(dy);
	indigo_safe_free(xi);
	indigo_safe_free(eta);
	indigo_safe_free(valid);
	indigo_safe_free(inlier);
	indigo_safe_free(best_inlier);
	indigo_safe_free(data.fx);
	indigo_safe_free(data.fy);
	indigo_safe_free(data.fxi);
	indigo_safe_free(data.feta);
	indigo_safe_free(sorted);
	// a single matched quad gives four self-consistent pairs, a solution needs more than that
	if (best_n < FIT_MIN_INLIERS) {
		indigo_error("%s: no consistent set of matches found (%d of %d)", __FUNCTION__, best_n, match_count);
		return false;
	}
	// 5. Rotation, scale and parity in the convention of astrometry.net, so the WCS of this agent and of the
	//    astrometry agent agree: the parity is positive when the image is the sky seen in a mirror (east to the
	//    right with north up, the determinant of the pixel to tangent plane mapping is negative), and for that
	//    parity the angle is turned by 180 degrees; the eta row of the fit gives the angle for both parities.
	float angle = atan2(best_fit.b1, best_fit.b2) + M_PI;
	int mirrored = best_fit.a1 * best_fit.b2 - best_fit.a2 * best_fit.b1 < 0 ? 1 : -1;
	if (mirrored > 0) {
		angle += M_PI;
	}
	*rotation = fmod(angle + 4 * M_PI, 2 * M_PI);
	*scale = best_fit.pixel;
	if (parity != NULL) {
		*parity = mirrored;
	}
	if (matched != NULL) {
		*matched = best_n;
	}
	if (rms != NULL) {
		*rms = best_fit.rms;
	}
	// 6. RA/Dec of the image centre: Δx = Δy = 0 maps to (a0, b0) on the tangent plane
	indigo_tangent_to_radec(ref_ra, ref_dec, best_fit.a0, best_fit.b0, ra_center, dec_center);
	indigo_debug("%s: RA = %.3f°, DEC = %.3f°, rotation = %.2f°, scale = %.02f\"/pix, %d of %d matches, rms = %.2f\"", __FUNCTION__, *ra_center * RAD_TO_DEG, *dec_center * RAD_TO_DEG, *rotation * RAD_TO_DEG, *scale / ARCSEC_TO_RAD, best_n, match_count, best_fit.rms / ARCSEC_TO_RAD);
	return true;
}
