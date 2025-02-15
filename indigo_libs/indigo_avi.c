/*
 * This code is based on libgwavi library.
 *
 * Copyright (c) 2008-2011, Michael Kohn
 * Copyright (c) 2013, Robin Hahling
 * All rights reserved.
 *
 * Refactored for INDIGO.
 *
 * Copyright (c) 2020, CloudMakers, s. r. o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the author nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is the file containing gwavi library functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_avi.h>

static bool write_int(indigo_uni_handle *handle, uint32_t n) {
	unsigned char buffer[4];
	buffer[0] = n;
	buffer[1] = n >> 8;
	buffer[2] = n >> 16;
	buffer[3] = n >> 24;
	return indigo_uni_write(handle, (const char *)buffer, 4);
}

static bool write_short(indigo_uni_handle *handle, uint16_t n) {
	unsigned char buffer[2];
	buffer[0] = (uint8_t)n;
	buffer[1] = (uint8_t)(n >> 8);
	return indigo_uni_write(handle, (const char *)buffer, 2);
}

static bool write_chars(indigo_uni_handle *handle, const char *s) {
	int count = (int)strlen(s);
	return indigo_uni_write(handle, s, count);
}

static bool write_chars_bin(indigo_uni_handle *handle, const char *s, int count) {
	return indigo_uni_write(handle, s, count);
}

static bool tell(indigo_uni_handle *handle, long *offset) {
	return (*offset = indigo_uni_seek(handle, 0, SEEK_CUR)) != -1;
}

static bool seek(indigo_uni_handle *handle, long offset) {
	return indigo_uni_seek(handle, offset, SEEK_SET);
}

static bool write_avi_header(indigo_uni_handle *handle, struct gwavi_header_t *avi_header) {
	long marker, t;
	return
		write_chars_bin(handle, "avih", 4) &&
		tell(handle, &marker) &&
		write_int(handle, 0) &&
		write_int(handle, avi_header->time_delay) &&
		write_int(handle, avi_header->data_rate) &&
		write_int(handle, avi_header->reserved) &&
		write_int(handle, avi_header->flags) &&
		write_int(handle, avi_header->number_of_frames) &&
		write_int(handle, avi_header->initial_frames) &&
		write_int(handle, avi_header->data_streams) &&
		write_int(handle, avi_header->buffer_size) &&
		write_int(handle, avi_header->width) &&
		write_int(handle, avi_header->height) &&
		write_int(handle, avi_header->time_scale) &&
		write_int(handle, avi_header->playback_data_rate) &&
		write_int(handle, avi_header->starting_time) &&
		write_int(handle, avi_header->data_length) &&
		tell(handle, &t) &&
		seek(handle, marker) &&
		write_int(handle, (unsigned int)(t - marker - 4)) &&
		seek(handle, t);
}

static bool write_stream_header(indigo_uni_handle *handle, struct gwavi_stream_header_t *stream_header) {
	long marker, t;
	return
		write_chars_bin(handle, "strh", 4) &&
		tell(handle, &marker) &&
		write_int(handle, 0) &&
		write_chars_bin(handle, stream_header->data_type, 4) &&
		write_chars_bin(handle, stream_header->codec, 4) &&
		write_int(handle, stream_header->flags) &&
		write_int(handle, stream_header->priority) &&
		write_int(handle, stream_header->initial_frames) &&
		write_int(handle, stream_header->time_scale) &&
		write_int(handle, stream_header->data_rate) &&
		write_int(handle, stream_header->start_time) &&
		write_int(handle, stream_header->data_length) &&
		write_int(handle, stream_header->buffer_size) &&
		write_int(handle, stream_header->video_quality) &&
		write_int(handle, stream_header->sample_size) &&
		write_int(handle, 0) &&
		write_int(handle, 0) &&
		tell(handle, &t) &&
		seek(handle, marker) &&
		write_int(handle, (unsigned int)(t - marker - 4)) &&
		seek(handle, t);
}

static bool write_stream_format(indigo_uni_handle *handle, struct gwavi_stream_format_t *stream_format) {
	long marker, t;
	return
		write_chars_bin(handle, "strf", 4) &&
		tell(handle, &marker) &&
		write_int(handle, 0) &&
		write_int(handle, stream_format->header_size) &&
		write_int(handle, stream_format->width) &&
		write_int(handle, stream_format->height) &&
		write_short(handle, stream_format->num_planes) &&
		write_short(handle, stream_format->bits_per_pixel) &&
		write_int(handle, stream_format->compression_type) &&
		write_int(handle, stream_format->image_size) &&
		write_int(handle, stream_format->x_pels_per_meter) &&
		write_int(handle, stream_format->y_pels_per_meter) &&
		write_int(handle, stream_format->colors_used) &&
		write_int(handle, stream_format->colors_important) &&
		tell(handle, &t) &&
		seek(handle, marker) &&
		write_int(handle, (unsigned int)(t - marker - 4)) &&
		seek(handle, t);
}

static bool write_avi_header_chunk(struct gwavi_t *gwavi) {
	long marker, sub_marker, t;
	indigo_uni_handle *handle = gwavi->handle;
	return
		write_chars_bin(handle, "LIST", 4) &&
		tell(handle, &marker) &&
		write_int(handle, 0) &&
		write_chars_bin(handle, "hdrl", 4) &&
		write_avi_header(handle, &gwavi->avi_header) &&
		write_chars_bin(handle, "LIST", 4) &&
		tell(handle, &sub_marker) &&
		write_int(handle, 0) &&
		write_chars_bin(handle, "strl", 4) &&
		write_stream_header(handle, &gwavi->stream_header) &&
		write_stream_format(handle, &gwavi->stream_format) &&
		tell(handle, &t) &&
		seek(handle, sub_marker) &&
		write_int(handle, (unsigned int)(t - sub_marker - 4)) &&
		seek(handle, t) &&
		tell(handle, &t) &&
		seek(handle, marker) &&
		write_int(handle, (unsigned int)(t - marker - 4)) &&
		seek(handle, t);
}

static bool write_index(indigo_uni_handle *handle, int count, unsigned int *offsets) {
	long marker = 0, t;
	unsigned int offset = 4;
	bool result = (offsets != 0) &&
		write_chars_bin(handle, "idx1", 4) &&
		tell(handle, &marker) &&
		write_int(handle, 0);
	if (!result)
		return false;
	for (t = 0; t < count; t++) {
		if ((offsets[t] & 0x80000000) == 0) {
			result = write_chars(handle, "00dc");
		} else {
			result = write_chars(handle, "01wb");
			offsets[t] &= 0x7fffffff;
		}
		result = result && write_int(handle, 0x10) && write_int(handle, offset) && write_int(handle, offsets[t]);
		if (!result)
			return false;
		offset = offset + offsets[t] + 8;
	}
	result = tell(handle, &t) && seek(handle, marker) && write_int(handle, (unsigned int)(t - marker - 4)) && seek(handle, t);
	return result;
}

/**
 * This is the first function you should call when using gwavi library.
 * It allocates memory for a gwavi_t structure and returns it and takes care of
 * initializing the AVI header with the provided information.
 *
 * When you're done creating your AVI file, you should call gwavi_close()
 * function to free memory allocated for the gwavi_t structure and properly
 * close the output file.
 *
 * @param filename This is the name of the AVI file which will be generated by
 * this library.
 * @param width Width of a frame.
 * @param height Height of a frame.
 * @param fourcc FourCC representing the codec of the video encoded stream. a
 * FourCC is a sequence of four chars used to uniquely identify data formats.
 * For more information, you can visit www.fourcc.org.
 * @param fps Number of frames per second of your video. It needs to be > 0.
 * @param audio This parameter is optionnal. It is used for the audio track. If
 * you do not want to add an audio track to your AVI file, simply pass NULL for
 * this argument.
 *
 * @return Structure containing required information in order to create the AVI
 * file. If an error occured, NULL is returned.
 */
struct gwavi_t *gwavi_open(const char *filename, unsigned int width, unsigned int height, const char *fourcc, unsigned int fps) {
	struct gwavi_t *gwavi = NULL;
	indigo_uni_handle *handle = indigo_uni_create_file(filename, -INDIGO_LOG_TRACE);
	if (handle == NULL) {
		INDIGO_ERROR(indigo_error("gwavi_open: failed to open file for writing"));
		goto failure;
	}
	if ((gwavi = (struct gwavi_t *)malloc(sizeof(struct gwavi_t))) == NULL) {
		INDIGO_ERROR(indigo_error("gwavi_open: could not allocate memory for gwavi structure"));
		goto failure;
	}
	memset(gwavi, 0, sizeof(struct gwavi_t));
	gwavi->handle = handle;
	/* set avi header */
	gwavi->avi_header.time_delay= 1000000 / fps;
	gwavi->avi_header.data_rate = width * height * 3;
	gwavi->avi_header.flags = 0x10;
	gwavi->avi_header.data_streams = 1;
	/* this field gets updated when calling gwavi_close() */
	gwavi->avi_header.number_of_frames = 0;
	gwavi->avi_header.width = width;
	gwavi->avi_header.height = height;
	gwavi->avi_header.buffer_size = (width * height * 3);
	/* set stream header */
	(void)strcpy(gwavi->stream_header.data_type, "vids");
	(void)memcpy(gwavi->stream_header.codec, fourcc, 4);
	gwavi->stream_header.time_scale = 1;
	gwavi->stream_header.data_rate = fps;
	gwavi->stream_header.buffer_size = (width * height * 3);
	gwavi->stream_header.data_length = 0;
	/* set stream format */
	gwavi->stream_format.header_size = 40;
	gwavi->stream_format.width = width;
	gwavi->stream_format.height = height;
	gwavi->stream_format.num_planes = 1;
	gwavi->stream_format.bits_per_pixel = 24;
	gwavi->stream_format.compression_type =
	((unsigned int)fourcc[3] << 24) +
	((unsigned int)fourcc[2] << 16) +
	((unsigned int)fourcc[1] << 8) +
	((unsigned int)fourcc[0]);
	gwavi->stream_format.image_size = width * height * 3;
	gwavi->stream_format.colors_used = 0;
	gwavi->stream_format.colors_important = 0;
	gwavi->stream_format.palette = 0;
	gwavi->stream_format.palette_count = 0;
	if (!write_chars_bin(handle, "RIFF", 4) || !write_int(handle, 0))
		goto failure;
	if (!write_chars_bin(handle, "AVI ", 4) ||  !write_avi_header_chunk(gwavi))
		goto failure;
	if (!write_chars_bin(handle, "LIST", 4) || !tell(handle, &gwavi->marker) || !write_int(handle, 0) || !write_chars_bin(handle, "movi", 4))
		goto failure;
	gwavi->offsets_len = 1024;
	if ((gwavi->offsets = (unsigned int *)malloc((size_t)gwavi->offsets_len * sizeof(unsigned int))) == NULL) {
		INDIGO_ERROR(indigo_error("gwavi_open: could not allocate memory for gwavi offsets table"));
		goto failure;
	}
	gwavi->offsets_ptr = 0;
	return gwavi;
failure:
	indigo_uni_close(&handle);
	if (gwavi) {
		if (gwavi->offsets) {
			free(gwavi->offsets);
		}
		free(gwavi);
	}
	return NULL;
}

/**
 * This function allows you to add an encoded video frame to the AVI file.
 *
 * @param gwavi Main gwavi structure initialized with gwavi_open()-
 * @param buffer Video buffer size.
 * @param len Video buffer length.
 *
 * @return true on success, false on error.
 */
bool gwavi_add_frame(struct gwavi_t *gwavi, unsigned char *buffer, size_t len) {
	size_t maxi_pad;  /* if your frame is raggin, give it some paddin' */
	size_t t;
	char zero = 0;
	if (!gwavi || !buffer || len < 256)
		return false;
	gwavi->offset_count++;
	gwavi->stream_header.data_length++;
	maxi_pad = len % 4;
	if (maxi_pad > 0)
		maxi_pad = 4 - maxi_pad;
	if (gwavi->offset_count >= gwavi->offsets_len) {
		gwavi->offsets_len += 1024;
		gwavi->offsets = (unsigned int *)realloc(gwavi->offsets, (size_t)gwavi->offsets_len * sizeof(unsigned int));
	}
	gwavi->offsets[gwavi->offsets_ptr++] = (unsigned int)(len + maxi_pad);
	if (!write_chars_bin(gwavi->handle, "00dc", 4) || !write_int(gwavi->handle, (unsigned int)(len + maxi_pad)) || !indigo_uni_write(gwavi->handle, (const char *)buffer, (long)len))
		return false;
	for (t = 0; t < maxi_pad; t++) {
		if (!indigo_uni_write(gwavi->handle, &zero, 1))
			return false;
	}
	return true;
}

/**
 * This function should be called when the program is done adding video and/or
 * audio frames to the AVI file. It frees memory allocated for gwavi_open() for
 * the main gwavi_t structure. It also properly closes the output file.
 *
 * @param gwavi Main gwavi structure initialized with gwavi_open()-
 *
 * @return 0 on success, -1 on error.
 */
bool gwavi_close(struct gwavi_t *gwavi) {
	long t;
	if (!gwavi)
		return false;
	indigo_uni_handle *handle = gwavi->handle;
	if (!tell(handle, &t) || !seek(handle, gwavi->marker) || !write_int(handle, (unsigned int)(t - gwavi->marker - 4)))
		goto failure;
	if (!seek(handle, t) || !write_index(handle, gwavi->offset_count, gwavi->offsets))
		goto failure;
	free(gwavi->offsets);
	gwavi->offsets = NULL;
	gwavi->avi_header.number_of_frames = gwavi->stream_header.data_length;
	if (!tell(handle, &t) || !seek(handle, 12) || !write_avi_header_chunk(gwavi) || !seek(handle, t))
		goto failure;
	if (!seek(handle, 4) || !write_int(handle, (unsigned int)(t - 8)))
		goto failure;
	indigo_uni_close(&handle);
	free(gwavi);
	return true;
failure:
	if (gwavi->offsets) {
		free(gwavi->offsets);
	}
	indigo_uni_close(&handle);
	free(gwavi);
	return false;
}
