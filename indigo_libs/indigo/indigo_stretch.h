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

#ifdef __cplusplus
extern "C" {
#endif

extern void indigo_compute_stretch_params_8(const uint8_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram);
extern void indigo_compute_stretch_params_16(const uint16_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram);
extern void indigo_compute_stretch_params_24(const uint8_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram);
extern void indigo_compute_stretch_params_48(const uint16_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram);

extern void indigo_stretch_8(const uint8_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights);
extern void indigo_stretch_16(const uint16_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights);
extern void indigo_stretch_24(const uint8_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights);
extern void indigo_stretch_48(const uint16_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights);

#ifdef __cplusplus
}
#endif

#endif
