// Copyright (c) 2019 CloudMakers, s. r. o.
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

#ifndef indigo_guider_utils_h
#define indigo_guider_utils_h

#include <stdio.h>

typedef enum { none = 0, centroid, donuts } indigo_guide_algorithm;

typedef struct {
	indigo_guide_algorithm algorithm;
	int width;
	int height;
	union {
		double (*fft_x)[2];
		double centroid_x;
	};
	union {
		double (*fft_y)[2];
		double centroid_y;
	};
	double snr;
} indigo_frame_digest;

extern indigo_result indigo_selection_psf(indigo_raw_type raw_type, const void *data, double x, double y, const int radius, const int width, const int height, double *fwhm, double *hfd, double *peak);

extern indigo_result indigo_selection_frame_digest(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *c);
extern indigo_result indigo_centroid_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *c);
extern indigo_result indigo_donuts_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *fdigest);
extern indigo_result indigo_calculate_drift(const indigo_frame_digest *ref, const indigo_frame_digest *new, double *drift_x, double *drift_y);
extern indigo_result indigo_delete_frame_digest(indigo_frame_digest *fdigest);

#endif /* indigo_guider_utils_h */
