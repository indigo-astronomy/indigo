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

#include <indigo/indigo_stretch.h>

#include <vector>
#include <thread>
#include <algorithm>
#include <math.h>

#define INDIGO_DEFAULT_THREADS 4
#define MIN_SIZE_TO_PARALLELIZE 0x3FFFF

const int max_threads = (std::thread::hardware_concurrency() > 0) ? std::thread::hardware_concurrency() : INDIGO_DEFAULT_THREADS;

// raw - raw pixels, any unsigned int
// index - offset of the current pixel in raw
// row, column - row, column of the current pixel (to skip line + to detect edges of the frame)
// width, height - width, height of the frame
// offsets - 0x00 = RGGB, 0x01 = GBRG, 0x10 = GRBG, 0x11 = BGGR
// red, gree, blue - debayered pixel as an mean value of 1 to 4 pixels

template <typename T> static inline void debayer(T *raw, int index, int row, int column, int width, int height, int offsets, double &red, double &green, double &blue) {
	switch (offsets ^ ((column & 1) << 4 | (row & 1))) {
		case 0x00:
			red = raw[index];
			if (column == 0) {
				if (row == 0) {
					green = (raw[index + 1] + raw[index + width]) / 2.0;
					blue = raw[index + width + 1];
				} else if (row == height - 1) {
					green = (raw[index + 1] + raw[index - width]) / 2.0;
					blue = raw[index - width + 1];
				} else {
					green = (raw[index + 1] + raw[index + width] + raw[index - width]) / 3.0;
					blue = (raw[index - width + 1] + raw[index + width + 1]) / 2.0;
				}
			} else if (column == width - 1) {
				if (row == 0) {
					green = (raw[index - 1] + raw[index + width]) / 2.0;
					blue = (raw[index + width - 1] + raw[index + width + 1]) / 2.0;
				} else if (row == height - 1) {
					green = (raw[index - 1] + raw[index - width]) / 2.0;
					blue = (raw[index - width - 1] + raw[index - width + 1]) / 2.0;
				} else {
					green = (raw[index - 1] + raw[index + width] + raw[index - width]) / 3.0;
					blue = (raw[index - width - 1] + raw[index + width - 1]) / 2.0;
				}
			} else {
				if (row == 0) {
					green = (raw[index + 1] + raw[index - 1] + raw[index + width]) / 3.0;
					blue = (raw[index + width - 1] + raw[index + width + 1]) / 2.0;
				} else if (row == height - 1) {
					green = (raw[index + 1] + raw[index - 1] + raw[index - width]) / 3.0;
					blue = (raw[index - width - 1] + raw[index - width + 1]) / 2.0;
				} else {
					green = (raw[index + 1] + raw[index - 1] + raw[index + width] + raw[index - width]) / 4.0;
					blue = (raw[index - width - 1] + raw[index - width + 1] + raw[index + width - 1] + raw[index + width + 1]) / 4.0;
				}
			}
			break;
		case 0x10:
			if (column == 0) {
				red = raw[index + 1];
			} else if (column == width - 1) {
				red = raw[index - 1];
			} else {
				red = (raw[index - 1] + raw[index + 1]) / 2.0;
			}
			green = raw[index];
			if (row == 0) {
				blue = raw[index + width];
			} else if (row == height - 1) {
				blue = raw[index - width];
			} else {
				blue = (raw[index - width] + raw[index + width]) / 2.0;
			}
			break;
		case 0x01:
			if (row == 0) {
				red = raw[index + width];
			} else if (row == height - 1) {
				red = raw[index - width];
			} else {
				red = (raw[index - width] + raw[index + width]) / 2.0;
			}
			green = raw[index];
			if (column == 0) {
				blue = raw[index + 1];
			} else if (column == width - 1) {
				blue = raw[index - 1];
			} else {
				blue = (raw[index - 1] + raw[index + 1]) / 2.0;
			}
			break;
		case 0x11:
			if (column == 0) {
				if (row == 0) {
					red = raw[index + width + 1];
					green = (raw[index + 1] + raw[index + width]) / 2.0;
				} else if (row == height - 1) {
					red = raw[index - width + 1];
					green = (raw[index + 1] + raw[index - width]) / 2.0;
				} else {
					red = raw[index - width + 1];
					green = (raw[index + 1] + raw[index + width] + raw[index - width]) / 3.0;
				}
			} else if (column == width - 1) {
				if (row == 0) {
					red = raw[index + width - 1];
					green = (raw[index - 1] + raw[index + width]) / 2.0;
				} else if (row == height - 1) {
					red = raw[index - width - 1];
					green = (raw[index - 1] + raw[index - width]) / 2.0;
				} else {
					red = (raw[index - width - 1] + raw[index + width - 1]) / 2.0;
					green = (raw[index - 1] + raw[index + width] + raw[index - width]) / 3.0;
				}
			} else {
				if (row == 0) {
					red = (raw[index + width - 1] + raw[index + width + 1]) / 2.0;
					green = (raw[index + 1] + raw[index - 1] + raw[index + width]) / 3.0;
				} else if (row == height - 1) {
					red = (raw[index - width - 1] + raw[index - width + 1]) / 2.0;
					green = (raw[index + 1] + raw[index - 1] + raw[index - width]) / 3.0;
				} else {
					red = (raw[index - width - 1] + raw[index - width + 1] + raw[index + width - 1] + raw[index + width + 1]) / 4.0;
					green = (raw[index + 1] + raw[index - 1] + raw[index + width] + raw[index - width]) / 4.0;
				}
			}
			blue = raw[index];
			break;
	}
}

// buffer - pixels, 8 or 16 bit unsigned int
// width, height - width, height of the frame
// sample_columns_by, sample_rows_by - to subsample buffer
// shadows, midtones, highlights - stretch thresholds
// histogram - 256x values
// totals - sum of all pixels in subsample
// B, C - background, contrast params

template <typename T> void indigo_compute_stretch_params(const T *buffer, int width, int height, int sample_columns_by, int sample_rows_by, double *shadows, double *midtones, double *highlights, unsigned long *histogram, unsigned long *totals, double B = 0.25, double C = -2.8) {
	const int sample_size = (int)((ceil((double)width / sample_columns_by) * ceil((double)height / sample_rows_by)));
	const int sample_size_2 = sample_size / 2;
	const int histo_divider = (sizeof(T) == 1) ? 1 : 256; // TBD for 32 bits
	unsigned long total = 0;
	std::vector<T> samples(sample_size);
	if (sample_rows_by == 1) {
		int i = 0;
		int size = width * height;
		for (int index = 0; index < size; index += sample_columns_by) {
			T value = buffer[index];
			histogram[(samples[i++] = value) / histo_divider]++;
			total += value;
		}
	} else {
		int i = 0;
		const T *line = buffer;
		for (int line_index = 0; line_index < height; line_index += sample_rows_by) {
			for (int column_index = 0; column_index < width; column_index += sample_columns_by) {
				T value = line[column_index];
				histogram[(samples[i++] = value) / histo_divider]++;
				total += value;
			}
			line += width * sample_rows_by;
		}
	}
	if (totals) {
		*totals = total;
	}
	std::nth_element(samples.begin(), samples.begin() + sample_size_2, samples.end());
	const double median_sample = samples[sample_size_2];
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	std::vector<T> deviations(sample_size);
	for (int i = 0; i < sample_size; i++) {
		deviations[i] = (T)abs(median_sample - samples[i]);
	}
	std::nth_element(deviations.begin(), deviations.begin() + sample_size / 2, deviations.end());
	// scale to 0 -> 1.0.
	const double input_range = (sizeof(T) == 1) ? 0xFFL : 0XFFFFL; // TBD for 32 bits
	const double median_deviation = deviations[sample_size_2];
	const double normalized_median = median_sample / input_range;
	const double MADN = 1.4826 * median_deviation / input_range;
	const bool upperHalf = normalized_median > 0.5;
	*shadows = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalized_median + C * MADN)));
	*highlights = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalized_median - C * MADN)));
	double X, M;
	if (upperHalf) {
		X = B;
		M = *highlights - normalized_median;
	} else {
		X = normalized_median - *shadows;
		M = B;
	}
	if (X == 0) {
		*midtones = 0.0;
	} else if (X == M) {
		*midtones = 0.5;
	} else if (X == 1) {
		*midtones = 1.0;
	} else {
		*midtones = ((M - 1) * X) / ((2 * M - 1) * X - M);
	}
}


// value - pixel value
// native_shadows, native_highlights - shadows and highlights thresholds in input range
// k1_k2 = k1 * k2
// midtones_k2 = midtones / k2

inline static uint8_t stretch(double value, int native_shadows, int native_highlights, double k1_k2, double midtones_k2) {
	if (value < native_shadows) {
		return 0;
	} else if (value > native_highlights) {
		return 0xFF;
	} else {
		const double input_floored = (value - native_shadows);
		return (uint8_t)(k1_k2 * input_floored / (input_floored - midtones_k2));
	}
}

// input_buffer - source data as 8 or 16 bit integer
// step - 1 for mono or single channel, 3 for rgb
// width, height - height, width of frame
// output_buffer - JPEG conversion source buffer
// shadows, midtones, highlights - stretch thresholds
// coef - each pixel value is divided by this value before stretching (e.g. for AWB)

template <typename T> void indigo_stretch(T *input_buffer, int step, int width, int height, uint8_t *output_buffer, double shadows, double midtones, double highlights, double coef) {
	const int size = width * height;
	const double max_input = (sizeof(T) == 1) ? 0xFF : 0xFFFF; // TBD for 32 bits
	const double hs_range_factor = highlights == shadows ? 1.0f : 1.0f / (highlights - shadows);
	const int native_shadows = (int)(shadows * max_input);
	const int native_highlights = (int)(highlights * max_input);
	const double k1 = (midtones - 1) * hs_range_factor * 0xFF / max_input;
	const double k2 = ((2 * midtones) - 1) * hs_range_factor / max_input;
	const double k1_k2 = k1 / k2;
	const double midtones_k2 = midtones / k2;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		for (int i = 0; i < size; i++) {
			output_buffer[i * step] = stretch(input_buffer[i * step] / coef, native_shadows, native_highlights, k1_k2, midtones_k2);
		}
	} else {
		std::vector<std::thread> threads(max_threads);
		for (int rank = 0; rank < max_threads; rank++) {
			const int chunk = (int)ceil((double)size / max_threads);
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				end = (end > size) ? size : end;
				for (int i = start; i < end; i++) {
					output_buffer[i * step] = stretch(input_buffer[i * step] / coef, native_shadows, native_highlights, k1_k2, midtones_k2);
				}
			});
		}
		for (int rank = 0; rank < max_threads; rank++) {
			threads[rank].join();
		}
	}
}

#ifdef HISTOGRAM_AWB

// input_buffer - source data as 8 or 16 bit integer
// width, height - height, width of frame
// offsets - 0x00 = RGGB, 0x01 = GBRG, 0x10 = GRBG, 0x11 = BGGR
// output_buffer - JPEG conversion source buffer
// shadows, midtones, highlights - stretch thresholds
// totals - sum of all pixels in subsample to compute coeficients for AWB

template <typename T> void indigo_debayer_stretch(T *input_buffer, int width, int height, int offsets, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	const int size = width * height;
	const double max_input = (sizeof(T) == 1) ? 0xFF : 0xFFFF;
	int reference = 0;
	double redCoef = 1, greenCoef = 1, blueCoef = 1;
	if (totals) {
		if (totals[0] > totals[1] && totals[0] > totals[2]) {
			reference = 0;
			greenCoef = totals[1] / totals[0];
			blueCoef = totals[2] / totals[0];
		} else if (totals[1] > totals[0] && totals[1] > totals[2]) {
			reference = 1;
			redCoef = totals[0] / totals[1];
			blueCoef = totals[2] / totals[1];
		} else {
			reference = 2;
			redCoef = totals[0] / totals[2];
			greenCoef = totals[1] / totals[2];
		}
	}
	const double hs_range_factor = highlights[reference] == shadows[reference] ? 1.0f : 1.0f / (highlights[reference] - shadows[reference]);
	const int native_shadows = shadows[reference] * max_input;
	const int native_highlights = highlights[reference] * max_input;
	const double k1 = (midtones[reference] - 1) * hs_range_factor * 0xFF / max_input;
	const double k2 = ((2 * midtones[reference]) - 1) * hs_range_factor / max_input;
	const double k1_k2 = k1 / k2;
	const double midtones_k2 = midtones[1] / k2;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		int input_index = 0;
		for (int row_index = 0; row_index < height; row_index++) {
			for (int column_index = 0; column_index < width; column_index++) {
				double red = 0, green = 0, blue = 0;
				int output_index = input_index * 3;
				debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
				output_buffer[output_index] = stretch(red / redCoef, native_shadows, native_highlights, k1_k2, midtones_k2);
				output_buffer[output_index + 1] = stretch(green / greenCoef, native_shadows, native_highlights, k1_k2, midtones_k2);
				output_buffer[output_index + 2] = stretch(blue / blueCoef, native_shadows, native_highlights, k1_k2, midtones_k2);
				input_index++;
			}
		}
	} else {
		std::vector<std::thread> threads(max_threads);
		for (int rank = 0; rank < max_threads; rank++) {
			const int chunk = ceil((double)height / max_threads);
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				end = (end > height) ? height : end;
				int input_index = start * width;
				for (int row_index = start; row_index < end; row_index++) {
					for (int column_index = 0; column_index < width; column_index++) {
						double red = 0, green = 0, blue = 0;
						int output_index = input_index * 3;
						debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
						output_buffer[output_index] = stretch(red / redCoef, native_shadows, native_highlights, k1_k2, midtones_k2);
						output_buffer[output_index + 1] = stretch(green / greenCoef, native_shadows, native_highlights, k1_k2, midtones_k2);
						output_buffer[output_index + 2] = stretch(blue / blueCoef, native_shadows, native_highlights, k1_k2, midtones_k2);
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

#else  // unlincked stretch AWB

// input_buffer - source data as 8 or 16 bit integer
// width, height - height, width of frame
// offsets - 0x00 = RGGB, 0x01 = GBRG, 0x10 = GRBG, 0x11 = BGGR
// output_buffer - JPEG conversion source buffer
// shadows, midtones, highlights - stretch thresholds
// totals - unused

template <typename T> void indigo_debayer_stretch(T *input_buffer, int width, int height, int offsets, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	const int size = width * height;
	const double max_input = (sizeof(T) == 1) ? 0xFF : 0xFFFF;
	const double red_hs_range_factor = highlights[0] == shadows[0] ? 1.0f : 1.0f / (highlights[0] - shadows[0]);
	const int red_native_shadows = (int)(shadows[0] * max_input);
	const int red_native_highlights = (int)(highlights[0] * max_input);
	const double red_k1 = (midtones[0] - 1) * red_hs_range_factor * 0xFF / max_input;
	const double red_k2 = ((2 * midtones[0]) - 1) * red_hs_range_factor / max_input;
	const double red_k1_k2 = red_k1 / red_k2;
	const double red_midtones_k2 = midtones[0] / red_k2;
	const double green_hs_range_factor = highlights[1] == shadows[1] ? 1.0f : 1.0f / (highlights[1] - shadows[1]);
	const int green_native_shadows = (int)(shadows[1] * max_input);
	const int green_native_highlights = (int)(highlights[1] * max_input);
	const double green_k1 = (midtones[1] - 1) * green_hs_range_factor * 0xFF / max_input;
	const double green_k2 = ((2 * midtones[1]) - 1) * green_hs_range_factor / max_input;
	const double green_k1_k2 = green_k1 / green_k2;
	const double green_midtones_k2 = midtones[1] / green_k2;
	const double blue_hs_range_factor = highlights[2] == shadows[2] ? 1.0f : 1.0f / (highlights[2] - shadows[2]);
	const int blue_native_shadows = (int)(shadows[2] * max_input);
	const int blue_native_highlights = (int)(highlights[2] * max_input);
	const double blue_k1 = (midtones[2] - 1) * blue_hs_range_factor * 0xFF / max_input;
	const double blue_k2 = ((2 * midtones[2]) - 1) * blue_hs_range_factor / max_input;
	const double blue_k1_k2 = blue_k1 / blue_k2;
	const double blue_midtones_k2 = midtones[2] / blue_k2;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		int input_index = 0;
		for (int row_index = 0; row_index < height; row_index++) {
			for (int column_index = 0; column_index < width; column_index++) {
				double red = 0, green = 0, blue = 0;
				int output_index = input_index * 3;
				debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
				output_buffer[output_index] = stretch(red, red_native_shadows, red_native_highlights, red_k1_k2, red_midtones_k2);
				output_buffer[output_index + 1] = stretch(green, green_native_shadows, green_native_highlights, green_k1_k2, green_midtones_k2);
				output_buffer[output_index + 2] = stretch(blue, blue_native_shadows, blue_native_highlights, blue_k1_k2, blue_midtones_k2);
				input_index++;
			}
		}
	} else {
		std::vector<std::thread> threads(max_threads);
		for (int rank = 0; rank < max_threads; rank++) {
			const int chunk = (int)ceil((double)height / max_threads);
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				end = (end > height) ? height : end;
				int input_index = start * width;
				for (int row_index = start; row_index < end; row_index++) {
					for (int column_index = 0; column_index < width; column_index++) {
						double red = 0, green = 0, blue = 0;
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

#endif // HISTOGRAM_AWB

// uint8_t input
void indigo_debayer(const uint8_t *input_buffer, int width, int height, int offsets, uint8_t *output_buffer) {
	const int size = width * height;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		int input_index = 0;
		for (int row_index = 0; row_index < height; row_index++) {
			for (int column_index = 0; column_index < width; column_index++) {
				double red = 0, green = 0, blue = 0;
				int output_index = input_index * 3;
				debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
				output_buffer[output_index] = (uint8_t)red;
				output_buffer[output_index + 1] = (uint8_t)green;
				output_buffer[output_index + 2] = (uint8_t)blue;
				input_index++;
			}
		}
	} else {
		std::vector<std::thread> threads(max_threads);
		for (int rank = 0; rank < max_threads; rank++) {
			const int chunk = (int)ceil((double)height / max_threads);
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				end = (end > height) ? height : end;
				int input_index = start * width;
				for (int row_index = start; row_index < end; row_index++) {
					for (int column_index = 0; column_index < width; column_index++) {
						double red = 0, green = 0, blue = 0;
						int output_index = input_index * 3;
						debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
						output_buffer[output_index] = (uint8_t)red;
						output_buffer[output_index + 1] = (uint8_t)green;
						output_buffer[output_index + 2] = (uint8_t)blue;
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

// uint16_t input with scaling
void indigo_debayer(const uint16_t *input_buffer, int width, int height, int offsets, uint8_t *output_buffer) {
	const int size = width * height;
	if (size < MIN_SIZE_TO_PARALLELIZE) {
		int input_index = 0;
		for (int row_index = 0; row_index < height; row_index++) {
			for (int column_index = 0; column_index < width; column_index++) {
				double red = 0, green = 0, blue = 0;
				int output_index = input_index * 3;
				debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
				output_buffer[output_index] = (uint8_t)((uint16_t)red >> 8);
				output_buffer[output_index + 1] = (uint8_t)((uint16_t)green >> 8);
				output_buffer[output_index + 2] = (uint8_t)((uint16_t)blue >> 8);
				input_index++;
			}
		}
	} else {
		std::vector<std::thread> threads(max_threads);
		for (int rank = 0; rank < max_threads; rank++) {
			const int chunk = (int)ceil((double)height / max_threads);
			threads[rank] = std::thread([=]() {
				const int start = chunk * rank;
				int end = start + chunk;
				end = (end > height) ? height : end;
				int input_index = start * width;
				for (int row_index = start; row_index < end; row_index++) {
					for (int column_index = 0; column_index < width; column_index++) {
						double red = 0, green = 0, blue = 0;
						int output_index = input_index * 3;
						debayer(input_buffer, input_index, row_index, column_index, width, height, offsets, red, green, blue);
						output_buffer[output_index] = (uint8_t)((uint16_t)red >> 8);
						output_buffer[output_index + 1] = (uint8_t)((uint16_t)green >> 8);
						output_buffer[output_index + 2] = (uint8_t)((uint16_t)blue >> 8);
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

void indigo_compute_stretch_params_8(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, double B, double C) {
	indigo_compute_stretch_params(buffer + 0, width, height, sample_by, 1, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), NULL, B, C);
}

void indigo_compute_stretch_params_16(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, double B, double C) {
	indigo_compute_stretch_params(buffer + 0, width, height,  sample_by, 1, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), NULL, B, C);
}

void indigo_compute_stretch_params_24(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	for (int i = 0; i < 3; i++) {
		indigo_compute_stretch_params(buffer + i, 3 * width, height, sample_by * 3, 1, &shadows[i], &midtones[i], &highlights[i], histogram[i] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[i], B, C);
	}
}

void indigo_compute_stretch_params_48(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	for (int i = 0; i < 3; i++) {
		indigo_compute_stretch_params(buffer + i, 3 * width, height, sample_by * 3, 1, &shadows[i], &midtones[i], &highlights[i], histogram[i] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[i], B, C);
	}
}

void indigo_compute_stretch_params_8_rggb(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_8_gbrg(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_8_grbg(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_8_bggr(const uint8_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_16_rggb(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_16_gbrg(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_16_grbg(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer + width, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_compute_stretch_params_16_bggr(const uint16_t *buffer, int width, int height, int sample_by, double *shadows, double *midtones, double *highlights, unsigned long **histogram, unsigned long *totals, double B, double C) {
	indigo_compute_stretch_params(buffer + width + 1, width, height, sample_by * 2, 2, &shadows[0], &midtones[0], &highlights[0], histogram[0] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[0], B, C);
	indigo_compute_stretch_params(buffer + 1, width, height, sample_by * 2, 2, &shadows[1], &midtones[1], &highlights[1], histogram[1] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[1], B, C);
	indigo_compute_stretch_params(buffer, width, height, sample_by * 2, 2, &shadows[2], &midtones[2], &highlights[2], histogram[2] = (unsigned long *)malloc(sizeof(unsigned long) * 256), &totals[2], B, C);
}

void indigo_stretch_8(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_stretch(input_buffer + 0, 1, width, height, output_buffer + 0, shadows[0], midtones[0], highlights[0], 1);
}

void indigo_stretch_16(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights) {
	indigo_stretch(input_buffer + 0, 1, width, height, output_buffer + 0, shadows[0], midtones[0], highlights[0], 1);
}

void indigo_stretch_24(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	int reference = 0;
	double coef[3] = { 1, 1, 1 };
	if (totals[0] > totals[1] && totals[0] > totals[2]) {
		reference = 0;
		coef[1] = totals[1] / totals[0];
		coef[2] = totals[2] / totals[0];
	} else if (totals[1] > totals[0] && totals[1] > totals[2]) {
		reference = 1;
		coef[0] = totals[0] / totals[1];
		coef[2] = totals[2] / totals[1];
	} else {
		reference = 2;
		coef[0] = totals[0] / totals[2];
		coef[1] = totals[1] / totals[2];
	}
	for (int i = 0; i < 3; i++) {
		indigo_stretch(input_buffer + i, 3, width, height, output_buffer + i, shadows[reference], midtones[reference], highlights[reference], coef[i]);
	}
}

void indigo_stretch_48(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	int reference = 0;
	double coef[3] = { 1, 1, 1 };
	if (totals[0] > totals[1] && totals[0] > totals[2]) {
		reference = 0;
		coef[1] = totals[1] / totals[0];
		coef[2] = totals[2] / totals[0];
	} else if (totals[1] > totals[0] && totals[1] > totals[2]) {
		reference = 1;
		coef[0] = totals[0] / totals[1];
		coef[2] = totals[2] / totals[1];
	} else {
		reference = 2;
		coef[0] = totals[0] / totals[2];
		coef[1] = totals[1] / totals[2];
	}
	for (int i = 0; i < 3; i++) {
		indigo_stretch(input_buffer + i, 3, width, height, output_buffer + i, shadows[reference], midtones[reference], highlights[reference], coef[i]);
	}
}

void indigo_stretch_8_rggb(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x00, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_8_gbrg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x01, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_8_grbg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x10, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_8_bggr(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x11, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_16_rggb(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x00, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_16_gbrg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x01, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_16_grbg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x10, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_stretch_16_bggr(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer, double *shadows, double *midtones, double *highlights, unsigned long *totals) {
	indigo_debayer_stretch(input_buffer, width, height, 0x11, output_buffer, shadows, midtones, highlights, totals);
}

void indigo_debayer_8_rggb(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x00, output_buffer);
}

void indigo_debayer_8_gbrg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x01, output_buffer);
}

void indigo_debayer_8_grbg(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x10, output_buffer);
}

void indigo_debayer_8_bggr(const uint8_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x11, output_buffer);
}

void indigo_debayer_16_rggb(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x00, output_buffer);
}

void indigo_debayer_16_gbrg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x01, output_buffer);
}

void indigo_debayer_16_grbg(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x10, output_buffer);
}

void indigo_debayer_16_bggr(const uint16_t *input_buffer, int width, int height, uint8_t *output_buffer) {
	indigo_debayer(input_buffer, width, height, 0x11, output_buffer);
}
