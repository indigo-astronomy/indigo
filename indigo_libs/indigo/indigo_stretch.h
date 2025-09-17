// Copyright (C) 2023 Rumen G. Bogdanovski
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

// based on https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html

#ifndef indigo_stretch_h
#define indigo_stretch_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

INDIGO_EXTERN void indigo_compute_stretch_params_8(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, double B, double C); // use default values B = 0.25, C = -2.8
INDIGO_EXTERN void indigo_compute_stretch_params_16(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, double B, double);
INDIGO_EXTERN void indigo_compute_stretch_params_24(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_48(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);

INDIGO_EXTERN void indigo_compute_stretch_params_8_rggb(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_8_gbrg(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_8_grbg(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_8_bggr(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_16_rggb(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_16_gbrg(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_16_grbg(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);
INDIGO_EXTERN void indigo_compute_stretch_params_16_bggr(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C);

INDIGO_EXTERN void indigo_stretch_8(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights);
INDIGO_EXTERN void indigo_stretch_16(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights);
INDIGO_EXTERN void indigo_stretch_24(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_48(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);

INDIGO_EXTERN void indigo_stretch_8_rggb(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_8_gbrg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_8_grbg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_8_bggr(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_16_rggb(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_16_gbrg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_16_grbg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);
INDIGO_EXTERN void indigo_stretch_16_bggr(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals);

INDIGO_EXTERN void indigo_debayer_8_rggb(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_8_gbrg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_8_grbg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_8_bggr(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_16_rggb(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_16_gbrg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_16_grbg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer);
INDIGO_EXTERN void indigo_debayer_16_bggr(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer);

#ifdef __cplusplus
}
#endif

#endif
