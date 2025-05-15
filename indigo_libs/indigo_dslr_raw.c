// Copyright (c) 2022-2025 Rumen G. Bogdanovski
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_dslr_raw.h>

static int image_debayered_data(libraw_data_t *raw_data, indigo_dslr_raw_image_s *outout_image) {
	int rc;
	libraw_processed_image_t *processed_image = NULL;
	rc = libraw_raw2image(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_raw2image failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}
	rc = libraw_dcraw_process(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_dcraw_process failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}
	processed_image = libraw_dcraw_make_mem_image(raw_data, &rc);
	if (!processed_image) {
		indigo_error("[rc:%d] libraw_dcraw_make_mem_image failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}
	if (processed_image->type != LIBRAW_IMAGE_BITMAP) {
		indigo_error("input data is not of type LIBRAW_IMAGE_BITMAP");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}
	if (processed_image->colors != 3) {
		indigo_error("debayered data has not 3 colors");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}
	if (processed_image->bits != 16) {
		indigo_error("16 bit is supported only");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}
	outout_image->width = processed_image->width;
	outout_image->height = processed_image->height;
	outout_image->size = processed_image->data_size;
	outout_image->colors = (uint8_t)processed_image->colors;
	if (outout_image->data) {
		free(outout_image->data);
	}
	outout_image->data = malloc(outout_image->size);
	if (!outout_image->data) {
		indigo_error("%s", strerror(errno));
		rc = errno;
		goto cleanup;
	}
	memcpy(outout_image->data, processed_image->data, outout_image->size);
cleanup:
	libraw_dcraw_clear_mem(processed_image);
	return rc;
}

static int image_bayered_data(libraw_data_t *raw_data, indigo_dslr_raw_image_s *outout_image, const bool binning) {
	uint16_t *data;
	uint16_t width, height, raw_width;
	uint32_t npixels;
	uint32_t offset;
	size_t size;
	uint32_t i = 0;
	if (raw_data->sizes.iheight > raw_data->sizes.raw_height) {
		indigo_error("Images with raw_height < image_height are not supported");
		return LIBRAW_UNSPECIFIED_ERROR;
	}
	if (binning) {
		width = raw_data->sizes.iwidth % 2 == 0 ? raw_data->sizes.iwidth : raw_data->sizes.iwidth - 1;
		height = raw_data->sizes.iheight % 2 == 0 ? raw_data->sizes.iheight : raw_data->sizes.iheight - 1;
	} else {
		width = raw_data->sizes.iwidth;
		height = raw_data->sizes.iheight;
	}
	raw_width = raw_data->sizes.raw_width;
	npixels = width * height;
	offset = raw_width * raw_data->rawdata.sizes.top_margin +
		raw_data->rawdata.sizes.left_margin;
	size = binning ? npixels / 4 * sizeof(uint16_t) : npixels * sizeof(uint16_t);
	data = (uint16_t *)calloc(1, size);
	if (!data) {
		indigo_error("%s", strerror(errno));
		return -errno;
	}
	outout_image->width = binning ? width / 2 : width;
	outout_image->height = binning ? height / 2 : height;
	outout_image->size = size;
	int c = binning ? 2 : 1;
#ifdef FIT_FORMAT_AMATEUR_CCD
	for (int row = 0; row < height; row += c) {
#else
	for (int row = height - 1; row >= 0; row -= c) {
#endif
		for (int col = 0; col < width; col += c) {
			if (binning) {
				data[i++] = (
					raw_data->rawdata.raw_image[offset + col + 0 + (raw_width * row)] +     /* R */
					raw_data->rawdata.raw_image[offset + col + 1 + (raw_width * row)] +     /* G */
					raw_data->rawdata.raw_image[offset + col + 2 + (raw_width * row)] +     /* G */
					raw_data->rawdata.raw_image[offset + col + 3 + (raw_width * row)]       /* B */
				) / 4;
			} else {
				data[i++] = raw_data->rawdata.raw_image[offset + col + (raw_width * row)];
			}
		}
	}
	outout_image->data = data;
	outout_image->colors = 1;
	return 0;
}

int indigo_dslr_raw_process_image(void *buffer, size_t buffer_size, indigo_dslr_raw_image_s *outout_image) {
	int rc;
	libraw_data_t *raw_data;
	outout_image->width = 0;
	outout_image->height = 0;
	outout_image->bits = 16;
	outout_image->colors = 0;
	outout_image->colors = false;
	memset(outout_image->bayer_pattern, 0, sizeof(outout_image->bayer_pattern));
	outout_image->size = 0;
	outout_image->data = NULL;
	clock_t start = clock();
	raw_data = libraw_init(0);
	/* These work fine for astro - change with caution */
	/* Linear 16-bit output. */
	raw_data->params.output_bps = 16;
	/* Use simple interpolation (0) to debayer. > 20 will not debyer */
	raw_data->params.user_qual = 21;
	/* Disable four color space. */
	raw_data->params.four_color_rgb = 0;
	/* Disable LibRaw's default histogram transformation. */
	raw_data->params.no_auto_bright = 1;
	/* Disable LibRaw's default gamma curve transformation, */
	raw_data->params.gamm[0] = raw_data->params.gamm[1] = 1.0;
	/* Apply an embedded color profile, enabled by LibRaw by default. */
	raw_data->params.use_camera_matrix = 1;
	/* Disable automatic white balance obtained after averaging over the entire image. */
	raw_data->params.use_auto_wb = 0;
	/* Enable white balance from the camera (if possible). */
	raw_data->params.use_camera_wb = 1;
	/* Output colorspace raw*/
	raw_data->params.output_color = 0;
	rc = libraw_open_buffer(raw_data, buffer, buffer_size);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_open_buffer failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}
	rc = libraw_unpack(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_unpack failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}
	if (raw_data->idata.filters == 9) {
		for (int r = 0; r < 6; r++) {
			for (int c = 0; c < 6; c++) {
				outout_image->bayer_pattern[6 * r + c] = raw_data->idata.cdesc[raw_data->idata.xtrans_abs[r][c]];
			}
		}
		outout_image->bayer_pattern[36] = 0;
	} else {
		outout_image->bayer_pattern[0] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 0, 0)];
		outout_image->bayer_pattern[1] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 0, 1)];
		outout_image->bayer_pattern[2] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 1, 0)];
		outout_image->bayer_pattern[3] = raw_data->idata.cdesc[libraw_COLOR(raw_data, 1, 1)];
		outout_image->bayer_pattern[4] = 0;
	}
	indigo_debug("Maker       : %s, Model      : %s", raw_data->idata.make, raw_data->idata.model);
	indigo_debug("Norm Maker  : %s, Norm Model : %s", raw_data->idata.normalized_make, raw_data->idata.normalized_model);
	indigo_debug("width       = %d, height     = %d", raw_data->sizes.width, raw_data->sizes.height);
	indigo_debug("iwidth      = %d, iheight    = %d", raw_data->sizes.iwidth, raw_data->sizes.iheight);
	indigo_debug("raw_width   = %d, raw_height = %d", raw_data->sizes.raw_width, raw_data->sizes.raw_height);
	indigo_debug("left_margin = %d, top_margin = %d", raw_data->sizes.left_margin, raw_data->sizes.top_margin);
	indigo_debug("bayerpat    : %s", outout_image->bayer_pattern);
	if (raw_data->params.user_qual > 20) {
		rc = image_bayered_data(raw_data, outout_image, false);
		if (rc) goto cleanup;
		outout_image->debayered = false;
	} else {
		rc = image_debayered_data(raw_data, outout_image);
		if (rc) goto cleanup;
		outout_image->debayered = true;
	}
	indigo_debug("libraw conversion in %g sec, input size: %d bytes, unpacked + (de)bayered output size: %d bytes, bayer pattern '%s', dimension: %d x %d, bits: %d, colors: %d", (clock() - start) / (double)CLOCKS_PER_SEC, buffer_size, outout_image->size, outout_image->bayer_pattern, outout_image->width, outout_image->height, outout_image->bits, outout_image->colors);
cleanup:
	libraw_free_image(raw_data);
	libraw_recycle(raw_data);
	libraw_close(raw_data);
	return rc;
}

int indigo_dslr_raw_image_info(void *buffer, size_t buffer_size, indigo_dslr_raw_image_info_s *image_info) {
	int rc;
	libraw_data_t *raw_data;
	if (image_info == NULL) {
		indigo_error("No output data structure provided");
		return LIBRAW_UNSPECIFIED_ERROR;
	}
	clock_t start = clock();
	raw_data = libraw_init(0);
	rc = libraw_open_buffer(raw_data, buffer, buffer_size);
	if (rc != LIBRAW_SUCCESS) {
		indigo_error("[rc:%d] libraw_open_buffer failed: '%s'", rc, libraw_strerror(rc));
		goto cleanup;
	}
	strncpy(image_info->camera_make, raw_data->idata.make, sizeof(image_info->camera_make));
	strncpy(image_info->camera_model, raw_data->idata.model, sizeof(image_info->camera_model));
	strncpy(image_info->normalized_camera_make, raw_data->idata.normalized_make, sizeof(image_info->normalized_camera_make));
	strncpy(image_info->normalized_camera_model, raw_data->idata.normalized_model, sizeof(image_info->normalized_camera_model));
	strncpy(image_info->lens, raw_data->lens.Lens, sizeof(image_info->lens));
	strncpy(image_info->lens_make, raw_data->lens.LensMake, sizeof(image_info->lens_make));
	image_info->raw_height = raw_data->sizes.raw_height;
	image_info->raw_width = raw_data->sizes.raw_width;
	image_info->iheight = raw_data->sizes.iheight;
	image_info->iwidth = raw_data->sizes.iwidth;
	image_info->top_margin = raw_data->sizes.top_margin;
	image_info->left_margin = raw_data->sizes.left_margin;
	image_info->iso_speed = raw_data->other.iso_speed;
	image_info->shutter = raw_data->other.shutter;
	image_info->aperture = raw_data->other.aperture;
	image_info->focal_len = raw_data->other.focal_len;
	image_info->timestamp = raw_data->other.timestamp;
	image_info->temperature = -273.15f;
	if (raw_data->makernotes.common.SensorTemperature > -273.15f) {
		 image_info->temperature = raw_data->makernotes.common.SensorTemperature;
	} else if (raw_data->makernotes.common.CameraTemperature > -273.15f) {
		 image_info->temperature = raw_data->makernotes.common.CameraTemperature;
	}
	strncpy(image_info->desc, raw_data->other.desc, sizeof(image_info->desc));
	strncpy(image_info->artist, raw_data->other.artist, sizeof(image_info->artist));
	indigo_debug("libraw got image info in %g sec", (clock() - start) / (double)CLOCKS_PER_SEC);
cleanup:
	libraw_free_image(raw_data);
	libraw_recycle(raw_data);
	libraw_close(raw_data);
	return rc;
}
