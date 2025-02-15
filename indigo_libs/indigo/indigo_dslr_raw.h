// Copyright (c) 2022 Rumen G. Bogdanovski
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

#ifndef _INDIGO_DSLR_RAW_H
#define _INDIGO_DSLR_RAW_H

#include <libraw.h>
#include <stdbool.h>

typedef struct {
	uint16_t width;
	uint16_t height;
	size_t size;
	void *data;
	uint8_t bits;
	uint8_t colors;
	bool debayered;
	char bayer_pattern[37];
} indigo_dslr_raw_image_s;

typedef struct {
	char camera_make[64];
	char camera_model[64];
	char normalized_camera_make[64];
	char normalized_camera_model[64];
	char lens_make[64];
	char lens[64];
	ushort raw_height;
	ushort raw_width;
	ushort iheight;
	ushort iwidth;
	ushort top_margin;
	ushort left_margin;
	float iso_speed;
	float shutter;
	float aperture;
	float focal_len;
	float temperature;
	time_t timestamp;
	char desc[512];
	char artist[64];
} indigo_dslr_raw_image_info_s;

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FIT_FORMAT_AMATEUR_CCD

INDIGO_EXTERN int indigo_dslr_raw_process_image(void *buffer, size_t buffer_size, indigo_dslr_raw_image_s *output_image);
INDIGO_EXTERN int indigo_dslr_raw_image_info(void *buffer, size_t buffer_size, indigo_dslr_raw_image_info_s *image_info);

#ifdef __cplusplus
}
#endif

#endif /* _INDIGO_DSLR_RAW_H */
