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
// and https://github.com/indigo-astronomy/indigo_imager/blob/master/common_src/stretcher.cpp

#include <indigo/indigo_bus.h>
#include <indigo/indigo_stretch.h>

#include <vector>
#include <thread>
#include <algorithm>
#include <math.h>
#include <unistd.h>

template <typename T> void indigo_compute_stretch_params(T const *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long *histogram, const float B = 0.25, const float C = -2.8) {
	if (sample_by < 1) {
		sample_by = 1;
	}
	const int downsampled_size = size / sample_by;
	const int downsampled_size_2 = downsampled_size / 2;
	const int histo_divider = (sizeof(T) == 1) ? 1 : 256;
	std::vector<T> samples(downsampled_size);
	for (int index = 0, i = 0; i < downsampled_size; ++i, index += sample_by) {
		histogram[(samples[i] = buffer[index]) / histo_divider]++;
	}
	const T min_sample = *std::min_element(samples.begin(), samples.end());
	const T max_sample = *std::max_element(samples.begin(), samples.end());
	std::nth_element(samples.begin(), samples.begin() + downsampled_size_2, samples.end());
	const T median_sample = samples[downsampled_size_2];
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	std::vector<T> deviations(downsampled_size);
	for (int index = 0, i = 0; i < downsampled_size; ++i, index += sample_by) {
		if (median_sample > buffer[index]) {
			deviations[i] = median_sample - buffer[index];
		} else {
			deviations[i] = buffer[index] - median_sample;
		}
	}
	std::nth_element(deviations.begin(), deviations.begin() + downsampled_size / 2, deviations.end());
	// scale to 0 -> 1.0.
	const float input_range = (sizeof(T) == 1) ? 0x100L : 0X10000L;
	const float median_deviation = deviations[downsampled_size / 2];
	const float normalized_median = median_sample / input_range;
	const float MADN = 1.4826 * median_deviation / input_range;
	const bool upperHalf = normalized_median > 0.5;
	*shadows = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalized_median + C * MADN)));
	*highlights = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalized_median - C * MADN)));
	float X, M;
	if (!upperHalf) {
		X = normalized_median - *shadows;
		M = B;
	} else {
		X = B;
		M = *highlights - normalized_median;
	}
	if (X == 0) {
		*midtones = 0.0f;
	} else if (X == M) {
		*midtones = 0.5f;
	} else if (X == 1) {
		*midtones = 1.0f;
	} else {
		*midtones = ((M - 1) * X) / ((2 * M - 1) * X - M);
	}
}

template <typename T> void indigo_stretch(T *input_buffer, int input_sample, int size, uint8_t *output_buffer, int output_sample, double shadows, double midtones, double highlights) {
	const double max_input = (sizeof(T) == 1) ? 0xFF : 0xFFFF;
	const float hs_range_factor = highlights == shadows ? 1.0f : 1.0f / (highlights - shadows);
	const T native_shadows = shadows * max_input;
	const T native_highlights = highlights * max_input;
	const float k1 = (midtones - 1) * hs_range_factor * 0xFF / max_input;
	const float k2 = ((2 * midtones) - 1) * hs_range_factor / max_input;
	const float k1_k2 = k1 / k2;
	const float midtones_k2 = midtones / k2;
	const int max_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
	const int chunk = size / max_threads;
	std::thread threads[max_threads];
	for (int rank = 0; rank < max_threads; rank++) {
		threads[rank] = std::thread([=]() {
			const int start = chunk * rank;
			int end = start + chunk;
			if (end > size) {
				end = size;
			}
			for (int i = start; i < end; i++) {
				const T value = input_buffer[i * input_sample];
				int output_index = i * output_sample;
				if (value < native_shadows) {
					output_buffer[output_index] = 0;
				} else if (value > native_highlights) {
					output_buffer[output_index] = 0xFF;
				} else {
					const T input_floored = (value - native_shadows);
					output_buffer[output_index] = k1_k2 * input_floored / (input_floored - midtones_k2);
				}
			}
		}
		);
	}
	for (int rank = 0; rank < max_threads; rank++) {
		threads[rank].join();
	}
}

extern "C" void indigo_compute_stretch_params_8(const uint8_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram) {
	indigo_compute_stretch_params(buffer + 0, size, sample_by * 3, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256));
}

extern "C" void indigo_compute_stretch_params_16(const uint16_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram) {
	indigo_compute_stretch_params(buffer + 0, size, sample_by * 3, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256));
}

extern "C" void indigo_compute_stretch_params_24(const uint8_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram) {
	for (int i = 0; i < 3; i++) {
		indigo_compute_stretch_params(buffer + i, size, sample_by * 3, &shadows[i], &midtones[i], &highlights[i], histogram[i] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256));
	}
}

extern "C" void indigo_compute_stretch_params_48(const uint16_t *buffer, int size, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram) {
	for (int i = 0; i < 3; i++) {
		indigo_compute_stretch_params(buffer + i, size, sample_by * 3, &shadows[i], &midtones[i], &highlights[i], histogram[i] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256));
	}
}

extern "C" void indigo_stretch_8(const uint8_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_stretch(input_buffer + 0, 1, size, output_buffer + 0, 1, shadows[0], midtones[0], highlights[0]);
}

extern "C" void indigo_stretch_16(const uint16_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_stretch(input_buffer + 0, 1, size, output_buffer + 0, 1, shadows[0], midtones[0], highlights[0]);
}

extern "C" void indigo_stretch_24(const uint8_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	for (int i = 0; i < 3; i++) {
		indigo_stretch(input_buffer + i, 1, size, output_buffer + i, 1, shadows[i], midtones[i], highlights[i]);
	}
}

extern "C" void indigo_stretch_48(const uint16_t *input_buffer, int size, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	for (int i = 0; i < 3; i++) {
		indigo_stretch(input_buffer + i, 1, size, output_buffer + i, 1, shadows[i], midtones[i], highlights[i]);
	}
}
