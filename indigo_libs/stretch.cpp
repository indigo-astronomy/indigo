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

#define MIN_SIZE_TO_PARALLELIZE 0x3FFFF

template <typename T> void indigo_compute_stretch_params(const T *buffer, int width, int height, int sample_columns_by, int sample_rows_by, double *shadows, double *midtones, double *highlights, unsigned long *histogram, float B = 0.25, float C = -2.8) {
	const int sample_size = width * height / sample_columns_by / sample_rows_by;
	const int sample_size_2 = sample_size / 2;
	const int histo_divider = (sizeof(T) == 1) ? 1 : 256;
	std::vector<T> samples(sample_size);
	if (sample_rows_by == 1) {
		int i = 0;
		for (int index = 0; i < sample_size; index += sample_columns_by) {
			histogram[(samples[i++] = buffer[index]) / histo_divider]++;
		}
	} else {
		int i = 0;
		const T *line = buffer;
		for (int line_index = 0; line_index < height; line_index += sample_rows_by) {
			for (int column_index = 0; column_index < width; column_index += sample_columns_by) {
				histogram[(samples[i++] = line[column_index]) / histo_divider]++;
			}
			line += width;
		}
	}
	std::nth_element(samples.begin(), samples.begin() + sample_size_2, samples.end());
	const T median_sample = samples[sample_size_2];
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	std::vector<T> deviations(sample_size);
	if (sample_rows_by == 1) {
		int i = 0;
		for (int index = 0; i < sample_size; index += sample_columns_by) {
			deviations[i++] = abs(median_sample - buffer[index]);
		}
	} else {
		int i = 0;
		const T *line = buffer;
		for (int line_index = 0; line_index < height; line_index += sample_rows_by) {
			for (int column_index = 0; column_index < width; column_index += sample_columns_by) {
				deviations[i++] = abs(median_sample - line[column_index]);
			}
			line += width;
		}
	}
	std::nth_element(deviations.begin(), deviations.begin() + sample_size / 2, deviations.end());
	// scale to 0 -> 1.0.
	const float input_range = (sizeof(T) == 1) ? 0x100L : 0X10000L;
	const float median_deviation = deviations[sample_size / 2];
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

template <typename T> inline static uint8_t stretch(T value, int native_shadows, int native_highlights, float k1_k2, float midtones_k2) {
	if (value < native_shadows) {
		return 0;
	} else if (value > native_highlights) {
		return 0xFF;
	} else {
		const T input_floored = (value - native_shadows);
		return k1_k2 * input_floored / (input_floored - midtones_k2);
	}
}

template <typename T> static inline void debayer(T *raw, int index, int row, int column, int width, int height, int offsets, int &red, int &green, int &blue) {
	switch (offsets ^ ((column & 1) << 4 | (row & 1))) {
		case 0x00:
			red = raw[index];
			if (column == 0) {
				if (row == 0) {
					green = (raw[index + 1] + raw[index + width]) / 2;
					blue = raw[index + width + 1];
				} else if (row == height - 1) {
					green = (raw[index + 1] + raw[index - width]) / 2;
					blue = raw[index - width + 1];
				} else {
					green = (raw[index + 1] + raw[index + width] + raw[index - width]) / 3;
					blue = (raw[index - width + 1] + raw[index + width + 1]) / 2;
				}
			} else if (column == width - 1) {
				if (row == 0) {
					green = (raw[index - 1] + raw[index + width]) / 2;
					blue = (raw[index + width - 1] + raw[index + width + 1]) / 2;
				} else if (row == height - 1) {
					green = (raw[index - 1] + raw[index - width]) / 2;
					blue = (raw[index - width - 1] + raw[index - width + 1]) / 2;
				} else {
					green = (raw[index - 1] + raw[index + width] + raw[index - width]) / 3;
					blue = (raw[index - width - 1] + raw[index + width - 1]) / 2;
				}
			} else {
				if (row == 0) {
					green = (raw[index + 1] + raw[index - 1] + raw[index + width]) / 3;
					blue = (raw[index + width - 1] + raw[index + width + 1]) / 2;
				} else if (row == height - 1) {
					green = (raw[index + 1] + raw[index - 1] + raw[index - width]) / 3;
					blue = (raw[index - width - 1] + raw[index - width + 1]) / 2;
				} else {
					green = (raw[index + 1] + raw[index - 1] + raw[index + width] + raw[index - width]) / 4;
					blue = (raw[index - width - 1] + raw[index - width + 1] + raw[index + width - 1] + raw[index + width + 1]) / 4;
				}
			}
			break;
		case 0x10:
			if (column == 0) {
				red = raw[index + 1];
			} else if (column == width - 1) {
				red = raw[index - 1];
			} else {
				red = (raw[index - 1] + raw[index + 1]) / 2;
			}
			green = raw[index];
			if (row == 0) {
				blue = raw[index + width];
			} else if (row == height - 1) {
				blue = raw[index - width];
			} else {
				blue = (raw[index - width] + raw[index + width]) / 2;
			}
			break;
		case 0x01:
			if (row == 0) {
				red = raw[index + width];
			} else if (row == height - 1) {
				red = raw[index - width];
			} else {
				red = (raw[index - width] + raw[index + width]) / 2;
			}
			green = raw[index];
			if (column == 0) {
				blue = raw[index + 1];
			} else if (column == width - 1) {
				blue = raw[index - 1];
			} else {
				blue = (raw[index - 1] + raw[index + 1]) / 2;
			}
			break;
		case 0x11:
			if (column == 0) {
				if (row == 0) {
					red = raw[index + width + 1];
					green = (raw[index + 1] + raw[index + width]) / 2;
				} else if (row == height - 1) {
					red = raw[index - width + 1];
					green = (raw[index + 1] + raw[index - width]) / 2;
				} else {
					red = raw[index - width + 1];
					green = (raw[index + 1] + raw[index + width] + raw[index - width]) / 3;
				}
			} else if (column == width - 1) {
				if (row == 0) {
					red = raw[index + width - 1];
					green = (raw[index - 1] + raw[index + width]) / 2;
				} else if (row == height - 1) {
					red = raw[index - width - 1];
					green = (raw[index - 1] + raw[index - width]) / 2;
				} else {
					red = (raw[index - width - 1] + raw[index + width - 1]) / 2;
					green = (raw[index - 1] + raw[index + width] + raw[index - width]) / 3;
				}
			} else {
				if (row == 0) {
					red = (raw[index + width - 1] + raw[index + width + 1]) / 2;
					green = (raw[index + 1] + raw[index - 1] + raw[index + width]) / 3;
				} else if (row == height - 1) {
					red = (raw[index - width - 1] + raw[index - width + 1]) / 2;
					green = (raw[index + 1] + raw[index - 1] + raw[index - width]) / 3;
				} else {
					red = (raw[index - width - 1] + raw[index - width + 1] + raw[index + width - 1] + raw[index + width + 1]) / 4;
					green = (raw[index + 1] + raw[index - 1] + raw[index + width] + raw[index - width]) / 4;
				}
			}
			blue = raw[index];
			break;
	}
}

template <typename T> void indigo_stretch(T *input_buffer, int input_sample, int width, int height, uint8_t *output_buffer, int output_sample, double shadows, double midtones, double highlights) {
	const int size = width * height;
	const double max_input = (sizeof(T) == 1) ? 0xFF : 0xFFFF;
	const float hs_range_factor = highlights == shadows ? 1.0f : 1.0f / (highlights - shadows);
	const int native_shadows = shadows * max_input;
	const int native_highlights = highlights * max_input;
	const float k1 = (midtones - 1) * hs_range_factor * 0xFF / max_input;
	const float k2 = ((2 * midtones) - 1) * hs_range_factor / max_input;
	const float k1_k2 = k1 / k2;
	const float midtones_k2 = midtones / k2;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		for (int i = 0; i < size; i++) {
			output_buffer[i * output_sample] = stretch(input_buffer[i * input_sample], native_shadows, native_highlights, k1_k2, midtones_k2);
		}
	} else {
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
					output_buffer[i * output_sample] = stretch(input_buffer[i * input_sample], native_shadows, native_highlights, k1_k2, midtones_k2);
				}
			});
		}
		for (int rank = 0; rank < max_threads; rank++) {
			threads[rank].join();
		}
	}
}

template <typename T> void indigo_debayer_stretch(T *input_buffer, int width, int height, int offsets, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	const int size = width * height;
	const double max_input = (sizeof(T) == 1) ? 0xFF : 0xFFFF;
	const float red_hs_range_factor = highlights[0] == shadows[0] ? 1.0f : 1.0f / (highlights[0] - shadows[0]);
	const int red_native_shadows = shadows[0] * max_input;
	const int red_native_highlights = highlights[0] * max_input;
	const float red_k1 = (midtones[0] - 1) * red_hs_range_factor * 0xFF / max_input;
	const float red_k2 = ((2 * midtones[0]) - 1) * red_hs_range_factor / max_input;
	const float red_k1_k2 = red_k1 / red_k2;
	const float red_midtones_k2 = midtones[0] / red_k2;
	const float green_hs_range_factor = highlights[1] == shadows[1] ? 1.0f : 1.0f / (highlights[1] - shadows[1]);
	const int green_native_shadows = shadows[1] * max_input;
	const int green_native_highlights = highlights[1] * max_input;
	const float green_k1 = (midtones[1] - 1) * green_hs_range_factor * 0xFF / max_input;
	const float green_k2 = ((2 * midtones[1]) - 1) * green_hs_range_factor / max_input;
	const float green_k1_k2 = green_k1 / green_k2;
	const float green_midtones_k2 = midtones[1] / green_k2;
	const float blue_hs_range_factor = highlights[2] == shadows[2] ? 1.0f : 1.0f / (highlights[2] - shadows[2]);
	const int blue_native_shadows = shadows[2] * max_input;
	const int blue_native_highlights = highlights[2] * max_input;
	const float blue_k1 = (midtones[2] - 1) * blue_hs_range_factor * 0xFF / max_input;
	const float blue_k2 = ((2 * midtones[2]) - 1) * blue_hs_range_factor / max_input;
	const float blue_k1_k2 = blue_k1 / blue_k2;
	const float blue_midtones_k2 = midtones[2] / blue_k2;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		int input_index = 0;
		for (int row_index = 0; row_index < height; row_index++) {
			for (int column_index = 0; column_index < width; column_index++) {
				int red = 0, green = 0, blue = 0;
				int output_index = input_index * 3;
				debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
				output_buffer[output_index] = stretch(red, red_native_shadows, red_native_highlights, red_k1_k2, red_midtones_k2);
				output_buffer[output_index + 1] = stretch(green, green_native_shadows, green_native_highlights, green_k1_k2, green_midtones_k2);
				output_buffer[output_index + 2] = stretch(blue, blue_native_shadows, blue_native_highlights, blue_k1_k2, blue_midtones_k2);
				input_index++;
			}
		}
	} else {
		const int max_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
		const int chunk = height / max_threads;
		std::thread threads[max_threads];
		for (int rank = 0; rank < max_threads; rank++) {
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				if (end > height) {
					end = height;
				}
				int input_index = start * width;
				for (int row_index = start; row_index < end; row_index++) {
					for (int column_index = 0; column_index < width; column_index++) {
						int red = 0, green = 0, blue = 0;
						int output_index = input_index * 3;
						debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
						output_buffer[output_index] = stretch(red, red_native_shadows, red_native_highlights, red_k1_k2, red_midtones_k2);
						output_buffer[output_index + 1] = stretch(green, green_native_shadows, green_native_highlights, green_k1_k2, green_midtones_k2);
						output_buffer[output_index + 2] = stretch(blue, blue_native_shadows, blue_native_highlights, blue_k1_k2, blue_midtones_k2);
						input_index++;
					}
				}
			});
		}
		for (int rank = 0; rank < max_threads; rank++) {
			threads[rank].join();
		}
	}
}

extern "C" void indigo_compute_stretch_params_8(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + 0, width, height, sample_by, 1, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_16(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + 0, width, height,  sample_by, 1, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_24(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	for (int i = 0; i < 3; i++) {
		indigo_compute_stretch_params(buffer + i, 3 * width, height, sample_by * 3, 1, &shadows[i], &midtones[i], &highlights[i], histogram[i] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	}
}

extern "C" void indigo_compute_stretch_params_48(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	for (int i = 0; i < 3; i++) {
		indigo_compute_stretch_params(buffer + i, 3 * width, height, sample_by * 3, 1, &shadows[i], &midtones[i], &highlights[i], histogram[i] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	}
}

extern "C" void indigo_compute_stretch_params_8_rggb(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_8_gbrg(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_8_grbg(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_8_bggr(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_16_rggb(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_16_gbrg(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_16_grbg(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + width, width, height * 2, sample_by, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_compute_stretch_params_16_bggr(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, float B, float C) {
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)indigo_safe_malloc(sizeof(unsigned long) * 256), B, C);
}

extern "C" void indigo_stretch_8(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_stretch(input_buffer + 0, 1, width, height, output_buffer + 0, 1, shadows[0], midtones[0], highlights[0]);
}

extern "C" void indigo_stretch_16(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_stretch(input_buffer + 0, 1, width, height, output_buffer + 0, 1, shadows[0], midtones[0], highlights[0]);
}

extern "C" void indigo_stretch_24(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	for (int i = 0; i < 3; i++) {
		indigo_stretch(input_buffer + i, 1, 3 * width, height, output_buffer + i, 1, shadows[i], midtones[i], highlights[i]);
	}
}

extern "C" void indigo_stretch_48(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	for (int i = 0; i < 3; i++) {
		indigo_stretch(input_buffer + i, 1, 3 * width, height, output_buffer + i, 1, shadows[i], midtones[i], highlights[i]);
	}
}

extern "C" void indigo_stretch_8_rggb(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x00, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_8_gbrg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x01, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_8_grbg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x10, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_8_bggr(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x11, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_16_rggb(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x00, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_16_gbrg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x01, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_16_grbg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x10, output_buffer, shadows, midtones, highlights);
}

extern "C" void indigo_stretch_16_bggr(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_debayer_stretch(input_buffer, width, height, 0x11, output_buffer, shadows, midtones, highlights);
}

