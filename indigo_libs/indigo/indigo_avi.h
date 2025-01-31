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

#ifndef H_GWAVI
#define H_GWAVI

#include <stddef.h>

#include "indigo_uni_io.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gwavi_header_t {
	unsigned int time_delay;	/* dwMicroSecPerFrame */
	unsigned int data_rate;		/* dwMaxBytesPerSec */
	unsigned int reserved;
	unsigned int flags;		/* dwFlags */
	unsigned int number_of_frames;	/* dwTotalFrames */
	unsigned int initial_frames;	/* dwInitialFrames */
	unsigned int data_streams;	/* dwStreams */
	unsigned int buffer_size;	/* dwSuggestedBufferSize */
	unsigned int width;		/* dwWidth */
	unsigned int height;		/* dwHeight */
	unsigned int time_scale;
	unsigned int playback_data_rate;
	unsigned int starting_time;
	unsigned int data_length;
};

struct gwavi_stream_header_t {
	char data_type[5];	    /* fccType */
	char codec[5];		    /* fccHandler */
	unsigned int flags;	    /* dwFlags */
	unsigned int priority;
	unsigned int initial_frames;/* dwInitialFrames */
	unsigned int time_scale;    /* dwScale */
	unsigned int data_rate;	    /* dwRate */
	unsigned int start_time;    /* dwStart */
	unsigned int data_length;   /* dwLength */
	unsigned int buffer_size;   /* dwSuggestedBufferSize */
	unsigned int video_quality; /* dwQuality */
	int audio_quality;
	unsigned int sample_size;   /* dwSampleSize */
};

struct gwavi_stream_format_t {
	unsigned int header_size;
	unsigned int width;
	unsigned int height;
	unsigned short int num_planes;
	unsigned short int bits_per_pixel;
	unsigned int compression_type;
	unsigned int image_size;
	unsigned int x_pels_per_meter;
	unsigned int y_pels_per_meter;
	unsigned int colors_used;
	unsigned int colors_important;
	unsigned int *palette;
	unsigned int palette_count;
};

struct gwavi_t {
	indigo_uni_handle handle;
	struct gwavi_header_t avi_header;
	struct gwavi_stream_header_t stream_header;
	struct gwavi_stream_format_t stream_format;
	long marker;
	int offsets_ptr;
	int offsets_len;
	long offsets_start;
	unsigned int *offsets;
	int offset_count;
};

extern struct gwavi_t *gwavi_open(const char *filename, unsigned int width, unsigned int height, const char *fourcc, unsigned int fps);
extern bool gwavi_add_frame(struct gwavi_t *gwavi, unsigned char *buffer, size_t len);
extern bool gwavi_close(struct gwavi_t *gwavi);

#ifdef __cplusplus
}
#endif

#endif

