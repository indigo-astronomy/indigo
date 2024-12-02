//
//  indigo_guider_utils.c
//  indigo
//
//  Created by Peter Polakovic on 14/04/2019.
//  Copyright © 2019 CloudMakers, s. r. o. All rights reserved.
//
//  Based on libguider by Rumen Bogdanovski
//  Copyright © 2015 Rumen Bogdanovski All rights reserved.
//

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_raw_utils.h>

// Above this value the pixel is considered saturated
// Derived from different camera
#define SATURATION_8 247
#define SATURATION_16 65407

#define RE (0)
#define IM (1)
#define PI_2 (6.2831853071795864769252867665590057683943L)

static int median3(int a, int b, int c) {
	if (a > b) {
		if (b > c) return b;
		else if (a > c) return c;
		else return a;
	} else {
		if (a > c) return a;
		else if (b > c) return c;
		else return b;
	}
}

#define SAMPLES 50000

bool indigo_is_bayered_image(indigo_raw_header *header, size_t data_length) {
	if (header->signature == INDIGO_RAW_MONO8 || header->signature == INDIGO_RAW_MONO16) {
		size_t expected_length = sizeof(indigo_raw_header) + header->width * header->height * ((header->signature == INDIGO_RAW_MONO8) ? 1 : 2);
		int extension_length = (int)(data_length - expected_length);
		if (extension_length > 0) {
			char *extension = (char *)header + expected_length;;
			extension[extension_length - 1] = '\0'; /* Make sure it is null terminated */
			if (strstr(extension, "BAYERPAT")) {
				return true;
			}
		}
	}
	return false;
}

indigo_result indigo_equalize_bayer_channels(indigo_raw_type raw_type, void *data, const int width, const int height) {
	long long ch1_sum = 0, ch2_sum = 0, ch3_sum = 0, ch4_sum = 0;
	int ch1_count = 0, ch2_count = 0, ch3_count = 0, ch4_count = 0;

	if (raw_type != INDIGO_RAW_MONO8 && raw_type != INDIGO_RAW_MONO16) {
		return INDIGO_FAILED;
	}

	int x_step = (int)(width / sqrt(SAMPLES)) & ~1;  // Ensure it's even
	int y_step = (int)(height / sqrt(SAMPLES)) & ~1; // Ensure it's even

	if (raw_type == INDIGO_RAW_MONO16) {
		uint16_t* data16 = (uint16_t*)data;
		for (int y = 0; y < height - 1; y += y_step) {
			for (int x = 0; x < width - 1; x += x_step) {
				// Calculate averages using pixels at (x, y), (x+1, y), (x, y+1), and (x+1, y+1)
				int index = y * width + x;
				int index_right = index + 1;
				int index_down = (y + 1) * width + x;
				int index_diag = index_down + 1;

				ch1_sum += data16[index];
				ch1_count++;

				ch3_sum += data16[index_right];
				ch3_count++;

				ch2_sum += data16[index_down];
				ch2_count++;

				ch4_sum += data16[index_diag];
				ch4_count++;
			}
		}
	} else if (raw_type == INDIGO_RAW_MONO8) {
		uint8_t* data8 = (uint8_t*)data;
		for (int y = 0; y < height - 1; y += y_step) {
			for (int x = 0; x < width - 1; x += x_step) {
				// Calculate averages using pixels at (x, y), (x+1, y), (x, y+1), and (x+1, y+1)
				int index = y * width + x;
				int index_right = index + 1;
				int index_down = (y + 1) * width + x;
				int index_diag = index_down + 1;

				ch1_sum += data8[index];
				ch1_count++;

				ch3_sum += data8[index_right];
				ch3_count++;

				ch2_sum += data8[index_down];
				ch2_count++;

				ch4_sum += data8[index_diag];
				ch4_count++;
			}
		}
	}

	double overall_average = (ch1_sum + ch2_sum + ch3_sum + ch4_sum) / (double)(ch1_count + ch2_count + ch3_count + ch4_count);
	double ch1_scale_factor = overall_average / (ch1_sum / (double)ch1_count);
	double ch2_scale_factor = overall_average / (ch2_sum / (double)ch2_count);
	double ch3_scale_factor = overall_average /(ch3_sum / (double)ch3_count);
	double ch4_scale_factor = overall_average / (ch4_sum / (double)ch4_count);

	if (raw_type == INDIGO_RAW_MONO16) {
		uint16_t* data16 = (uint16_t*)data;
		for (int y = 0; y < height - 1; y += 2) {
			for (int x = 0; x < width - 1; x += 2) {
				// Scale pixels at (x, y), (x+1, y), (x, y+1), and (x+1, y+1)
				int index = y * width + x;
				int index_right = index + 1;
				int index_down = (y + 1) * width + x;
				int index_diag = index_down + 1;

				data16[index] = (uint16_t)(data16[index] * ch1_scale_factor);
				data16[index_right] = (uint16_t)(data16[index_right] * ch3_scale_factor);
				data16[index_down] = (uint16_t)(data16[index_down] * ch2_scale_factor);
				data16[index_diag] = (uint16_t)(data16[index_diag] * ch4_scale_factor);
			}
		}
	} else if (raw_type == INDIGO_RAW_MONO8) {
		uint8_t* data8 = (uint8_t*)data;
		for (int y = 0; y < height - 1; y += 2) {
			for (int x = 0; x < width - 1; x += 2) {
				// Scale pixels at (x, y), (x+1, y), (x, y+1), and (x+1, y+1)
				int index = y * width + x;
				int index_right = index + 1;
				int index_down = (y + 1) * width + x;
				int index_diag = index_down + 1;

				data8[index] = (data8[index] * ch1_scale_factor);
				data8[index_right] = (data8[index_right] * ch3_scale_factor);
				data8[index_down] = (data8[index_down] * ch2_scale_factor);
				data8[index_diag] = (data8[index_diag] * ch4_scale_factor);
			}
		}
	}
	return INDIGO_OK;
}

double indigo_rmse(double set[], const int count) {
	double sum = 0;

	if (count < 1) return 0;

	for (int i = 0; i < count; i++) {
		sum += set[i] * set[i];
	}

	return sqrt(sum / count);
}

/*
Corrects pixel P2 [x, y] if it is a hot pixel or part of a hot
line or column with the median of P0-P4. If P2 is Max value
and P2 > 2 * second_max => P2 is considered a hot pixel.

3x3 diagonal window is used to make algorithm work with hotlines and hot rows.
P0 00 P1
00 P2 00
P3 00 P4
*/
static int clear_hot_pixel_16(uint16_t* image, int x, int y, int width, int height) {
	int j, k, max, value;
	k = 0;
	int window[5];

	if (x < 1) x = 1;
	if (x >= width - 1) x = width - 2;
	if (y < 1) y = 1;
	if (y >= height - 1) y = height - 2;

	window[0] = image[(y - 1) * width + x - 1];
	window[1] = image[(y - 1) * width + x + 1];
	window[2] = value = image[y * width + x];
	window[3] = image[(y + 1) * width + x - 1];
	window[4] = image[(y + 1) * width + x + 1];

	//if ((x == 100 && y == 100) || (x == 900 && y == 500))
	//	indigo_error("x = %d y = %d => %d %d [%d] %d %d", x, y, window[0], window[1], window[2], window[3], window[4]);

	for (j = 0; j < 3; j++) {
		max = j;
		for (k = j + 1; k < 5; k++) if (window[k] > window[max]) max = k;
		int temp = window[j];
		window[j] = window[max];
		window[max] = temp;
	}
	/* window[0] = max;
	   window[1] = second_max;
	   window[2] = median
	*/
	if ((value == window[0]) && (value > (window[1] * 2))) value = window[2];
	//if ((x == 100 && y == 100) || (x == 900 && y == 500))
	//	indigo_error("x = %d y = %d => [%d]", x, y, value);
	return value;
}

static int clear_hot_pixel_8(uint8_t* image, int x, int y, int width, int height) {
	int i, k, max, value;
	k = 0;
	int window[5];

	if (x < 1) x = 1;
	if (x >= width - 1) x = width - 2;
	if (y < 1) y = 1;
	if (y >= height - 1) y = height - 2;

	window[0] = image[(y - 1) * width + x - 1];
	window[1] = image[(y - 1) * width + x + 1];
	window[2] = value = image[y * width + x];
	window[3] = image[(y + 1) * width + x - 1];
	window[4] = image[(y + 1) * width + x + 1];

	//if ((x == 100 && y == 100) || (x == 900 && y == 500))
	//	indigo_error("x = %d y = %d => %d %d [%d] %d %d", x, y, window[0], window[1], window[2], window[3], window[4]);

	for (i = 0; i < 3; i++) {
		max = i;
		for (k = i + 1; k < 5; k++) if (window[k] > window[max]) max = k;
		int temp = window[i];
		window[i] = window[max];
		window[max] = temp;
	}
	/* window[0] = max;
	   window[1] = second_max;
	   window[2] = median
	*/
	if ((value == window[0]) && (value > (window[1] * 2))) value = window[2];
	//if ((x == 100 && y == 100) || (x == 900 && y == 500))
	//	indigo_error("x = %d y = %d => [%d]", x, y, value);
	return value;
}

/*
static void hann_window(double (*data)[2], int len) {
	for (int n = 0; n < len; n++) {
		double sin_value = sin(3.14159265358979 * n / len);
		data[n][RE] = data[n][RE] * sin_value * sin_value;
		//indigo_error("HANN %d -> %.4f -> %.4f ", n, data[n][RE], sin_value * sin_value);
	}
}

static void tuckey_window(double (*data)[2], int len, double alpha) {
	const int N = (int)(alpha * len / 2.0) - 1;
	for (int n = 0; n <= N; n++) {
		double sin_value = sin(3.14159265358979 * n / (2 * N));
		double sin_value_sqr = sin_value * sin_value;
		data[n][RE] = data[n][RE] * sin_value_sqr;
		data[len-n-1][RE] = data[len-n-1][RE] * sin_value_sqr;
		//indigo_error("Tuckey %d %d -> %.4f -> %.4f ", N, n, data[n][RE], sin_value_sqr);
	}
}
*/

static void _fft(const int n, const int offset, const int delta, const double (*x)[2], double (*X)[2], double (*_X)[2]);

static void fft(const int n, const double (*x)[2], double (*X)[2]) {
	double (*_X)[2] = indigo_safe_malloc(2 * n * sizeof(double));
	_fft(n, 0, 1, x, X, _X);
	free(_X);
}

static void _fft(const int n, const int offset, const int delta, const double (*x)[2], double (*X)[2], double (*_X)[2]) {
	int n2 = n / 2;
	int k;
	double ccos, csin;
	int k00, k01, k10, k11;
	double tmp0, tmp1;
	if (n != 2) {
		_fft(n2, offset, 2 * delta, x, _X, X);
		_fft(n2, offset + delta, 2 * delta, x, _X, X);
		for (k = 0; k < n2; k++) {
			k00 = offset + k * delta;
			k01 = k00 + n2 * delta;
			k10 = offset + 2 * k * delta;
			k11 = k10 + delta;
			ccos = cos(PI_2 * k / (double)n);
			csin = sin(PI_2 * k / (double)n);
			tmp0 = ccos * _X[k11][RE] + csin * _X[k11][IM];
			tmp1 = ccos * _X[k11][IM] - csin * _X[k11][RE];
			X[k01][RE] = _X[k10][RE] - tmp0;
			X[k01][IM] = _X[k10][IM] - tmp1;
			X[k00][RE] = _X[k10][RE] + tmp0;
			X[k00][IM] = _X[k10][IM] + tmp1;
		}
	} else {
		k00 = offset;
		k01 = k00 + delta;
		X[k01][RE] = x[k00][RE] - x[k01][RE];
		X[k01][IM] = x[k00][IM] - x[k01][IM];
		X[k00][RE] = x[k00][RE] + x[k01][RE];
		X[k00][IM] = x[k00][IM] + x[k01][IM];
	}
}

static void ifft(const int n, const double (*X)[2], double (*x)[2]) {
	int n2 = n / 2;
	int i;
	double tmp0, tmp1;
	fft(n, X, x);
	x[0][RE] = x[0][RE] / n;
	x[0][IM] = x[0][IM] / n;
	x[n2][RE] = x[n2][RE] / n;
	x[n2][IM] = x[n2][IM] / n;
	for (i = 1; i < n2; i++) {
		tmp0 = x[i][RE] / n;
		tmp1 = x[i][IM] / n;
		x[i][RE] = x[n-i][RE] / n;
		x[i][IM] = x[n-i][IM] / n;
		x[n-i][RE] = tmp0;
		x[n-i][IM] = tmp1;
	}
}

static void corellate_fft(const int n, const double (*X1)[2], const double (*X2)[2], double (*c)[2]) {
	int i;
	double (*C)[2] = indigo_safe_malloc(2 * n * sizeof(double));
	/* pointwise multiply X1 conjugate with X2 here, store in X1 */
	for (i = 0; i < n; i++) {
		C[i][RE] = X1[i][RE] * X2[i][RE] + X1[i][IM] * X2[i][IM];
		C[i][IM] = X1[i][IM] * X2[i][RE] - X1[i][RE] * X2[i][IM];
	}
	ifft(n, C, c);
	free(C);
}

static double find_distance(const int n, const double (*c)[2]) {
	int i;
	const int n2 = n / 2;
	int max=0;
	int prev, next;
	for (i = 0; i < n; i++) {
		max = (c[i][0] > c[max][0]) ? i : max;
	}
	/* find previous and next positions to calculate quadratic interpolation */
	if ((max == 0) || (max == n2)) {
		prev = n - 1;
		next = 1;
	} else if (max == (n-1)) {
		prev = n - 2;
		next = 0;
	} else {
		prev = max - 1;
		next = max + 1;
	}
	/* find subpixel offset of the maximum position using quadratic interpolation */
	double max_subp = (c[next][RE] - c[prev][RE]) / (2 * (2 * c[max][RE] - c[next][RE] - c[prev][RE]));
	//INDIGO_DEBUG(indigo_debug("max_subp = %5.2f max: %d -> %5.2f %5.2f %5.2f\n", max_subp, max, c[prev][0], c[max][0], c[next][0]));
	if (max == n2) {
		return max_subp;
	} else if (max > n2) {
		return (double)((max - n) + max_subp);
	} else if (max < n2) {
		return (double)(max + max_subp);
	} else { /* should not be reached */
		return 0;
	}
}

static int next_power_2(const int n) {
	int k = 1;
	while (k < n)
		k <<= 1;
	return k;
}

indigo_result indigo_selection_psf(indigo_raw_type raw_type, const void *data, double x, double y, const int radius, const int width, const int height, double *fwhm, double *hfd, double *peak) {
	if ((width <= 2 * radius) || (height <= 2 * radius))
		return INDIGO_FAILED;
	int xx = (int)round(x);
	int yy = (int)round(y);
	if (xx < radius || width - radius < xx)
		return INDIGO_FAILED;
	if (yy < radius || height - radius < yy)
		return INDIGO_FAILED;
	if ((data == NULL) || (hfd == NULL) || (peak == NULL))
		return INDIGO_FAILED;

	double background = 0, max = 0, value = 0;
	int background_count = 0;

	int *values = (int*)malloc(8 * radius * sizeof(int));
	if (values == NULL)
		return INDIGO_FAILED;

	int ce = xx + radius, le = yy + radius;
	int cb = xx - radius, lb = yy - radius;
	for (int j = lb; j <= le; j++) {
		int k = j * width;
		for (int i = cb; i <= ce; i++) {
			int kk = k + i;
			switch (raw_type) {
				case INDIGO_RAW_MONO8: {
					value = ((uint8_t *)data)[kk];
					break;
				}
				case INDIGO_RAW_MONO16: {
					value = ((uint16_t *)data)[kk];
					break;
				}
				case INDIGO_RAW_RGB24: {
					kk *= 3;
					value = (((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2]) / 3;
					break;
				}
				case INDIGO_RAW_RGBA32: {
					kk *= 4;
					value = (((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2]) / 3;
					break;
				}
				case INDIGO_RAW_ABGR32: {
					kk *= 4;
					value = (((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2] + ((uint8_t *)data)[kk + 3]) / 3;
					break;
				}
				case INDIGO_RAW_RGB48: {
					kk *= 3;
					value = (((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2]) / 3;
					break;
				}
			}

			/* use border of the selection to calculate the background */
			if (j == lb || j == le || i == cb || i == ce) {
				background += value;
				values[background_count] = value;
				background_count++;
			}
			if (value > max)
				max = value;
		}
	}

	background = background / background_count;
	*peak = max - background;

	/* calculate stddev */
	int sum = 0;
	for (int i = 0; i < background_count; i++) {
		sum += (values[i] - background) * (values[i] - background);
	}
	free(values);
	double stddev = sqrt(sum / background_count);

	/* HFD calculation */
	double threshold = background + 2 * stddev; /* 2 * stddev is a good threshold for HFD */
	indigo_debug("HFD : background = %2f, stddev = %.2f, threshold = %.2f, max = %.2f", background, stddev, threshold, max);

	if (max < threshold) {
		*hfd = 2 * radius + 1;
	} else {
		double prod = 0, total = 0;
		for (int j = yy - radius; j <= le; j++) {
			int k = j * width;
			for (int i = xx - radius; i <= ce; i++) {
				int kk = k + i;
				switch (raw_type) {
					case INDIGO_RAW_MONO8: {
						value = ((uint8_t *)data)[kk];
						break;
					}
					case INDIGO_RAW_MONO16: {
						value = ((uint16_t *)data)[kk];
						break;
					}
					case INDIGO_RAW_RGB24: {
						kk *= 3;
						value = (((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2]) / 3;
						break;
					}
					case INDIGO_RAW_RGBA32: {
						kk *= 4;
						value = (((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2]) / 3;
						break;
					}
					case INDIGO_RAW_ABGR32: {
						kk *= 4;
						value = (((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2] + ((uint8_t *)data)[kk + 3]) / 3;
						break;
					}
					case INDIGO_RAW_RGB48: {
						kk *= 3;
						value = (((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2]) / 3;
						break;
					}
				}
				value -= threshold;
					if (value > 0) {
					double dist = sqrt((x - i) * (x - i) + (y - j) * (y - j));
					prod += dist * value;
					total += value;
				}
			}
		}
		*hfd = 2 * prod / total;
	}

	/* FWHM calculation */
	threshold = background + 6 * stddev; /* 6 * stddev is a good threshold for FWHM*/
	indigo_debug("FWHM: background = %2f, stddev = %.2f, threshold = %.2f, max = %.2f", background, stddev, threshold, max);

	if (max < threshold) {
		*fwhm = 2 * radius + 1;
	} else {
		double half_max = *peak / 2 + background;
		static int d2[][2] = { { -1, 0 }, { 0, -1 }, { 0, 1 }, { 1, 0 } };
		double d3[] = { radius, radius, radius, radius };
		for (int d = 0; d < 4; d++) {
			double previous = max;
			for (int k = 1; k < radius; k++) {
				int i = k * d2[d][0];
				int j = k * d2[d][1];
				int kk = (yy + j) * width + i + xx;
				switch (raw_type) {
					case INDIGO_RAW_MONO8: {
						value = ((uint8_t *)data)[kk];
						break;
					}
					case INDIGO_RAW_MONO16: {
						value = ((uint16_t *)data)[kk];
						break;
					}
					case INDIGO_RAW_RGB24: {
						kk *= 3;
						value = ((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2];
						break;
					}
					case INDIGO_RAW_RGBA32: {
						kk *= 4;
						value = ((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2];
						break;
					}
					case INDIGO_RAW_ABGR32: {
						kk *= 4;
						value = ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2] + ((uint8_t *)data)[kk + 3];
						break;
					}
					case INDIGO_RAW_RGB48: {
						kk *= 3;
						value = ((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2];
						break;
					}
				}
				if (value <= half_max) {
					if (value == previous)
						d3[d] = k;
					else
						d3[d] = k - 1 + (previous - half_max) / (previous - value);
					break;
				}
				if (value < previous)
					previous = value;
			}
		}
		double tmp = (d3[0] + d3[1] + d3[2] + d3[3]) / 2;
		if (tmp < 1 || tmp > 2 * radius)
			tmp = 2 * radius + 1;
		*fwhm = tmp;
	}
	return INDIGO_OK;
}

indigo_result indigo_selection_frame_digest_iterative(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *digest, int converge_iterations) {
	int result = INDIGO_FAILED;
	int ci = converge_iterations;
	while (ci--) {
		result = indigo_selection_frame_digest(raw_type, data, x, y, radius, width, height, digest);
		if (result != INDIGO_OK) {
			break;
		}
	}
	if (result != INDIGO_OK) {
		/* No star found in the selection -> search in wider vicinity then converge again */
		indigo_debug("%s(): No star found around X = %.3f, Y= %3f. Searching in wider vicinity", __FUNCTION__, *x, *y, converge_iterations);
		result = indigo_selection_frame_digest(raw_type, data, x, y, (int)(radius * 2.5), width, height, digest);
		ci = converge_iterations;
		while (ci--) {
			result = indigo_selection_frame_digest(raw_type, data, x, y, radius, width, height, digest);
		}
	}
	return result;
}

#define MAX_RADIUS 128
indigo_result indigo_selection_frame_digest(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *digest) {
	const int xx = (int)round(*x);
	const int yy = (int)round(*y);
	double background[MAX_RADIUS*8+2];
	int background_count = 0;

	if ((width <= 2 * radius + 1) || (height <= 2 * radius + 1) || radius > MAX_RADIUS)
		return INDIGO_FAILED;
	if (xx < radius || width - radius < xx)
		return INDIGO_FAILED;
	if (yy < radius || height - radius < yy)
		return INDIGO_FAILED;
	if ((data == NULL) || (digest == NULL))
		return INDIGO_FAILED;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;

	double m10 = 0, m01 = 0, m00 = 0, max = 0, sum = 0;
	const int ce = xx + radius, le = yy + radius;
	const int cs = xx - radius, ls = yy - radius;
	double value;
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int j = ls; j <= le; j++) {
				for (int i = cs; i <= ce; i++) {
					value = clear_hot_pixel_8(data8, i, j, width, height);
					/* use border for background noise estimation */
					if (j == ls || j == le || i == cs || i == ce) {
						background[background_count++] = value;
					}
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int j = ls; j <= le; j++) {
				for (int i = cs; i <= ce; i++) {
					value = clear_hot_pixel_16(data16, i, j, width, height);
					/* use border for background noise estimation */
					if (j == ls || j == le || i == cs || i == ce) {
						background[background_count++] = value;
					}
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 3 * (k + i);
					value = data8[kk] + data8[kk + 1] + data8[kk + 2];
					/* use border for background noise estimation */
					if (j == ls || j == le || i == cs || i == ce) {
						background[background_count++] = value;
					}
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 4 * (k + i);
					value = data8[kk] + data8[kk + 1] + data8[kk + 2];
					/* use border for background noise estimation */
					if (j == ls || j == le || i == cs || i == ce) {
						background[background_count++] = value;
					}
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 4 * (k + i);
					value = data8[kk + 1] + data8[kk + 2] + data8[kk + 3];
					/* use border for background noise estimation */
					if (j == ls || j == le || i == cs || i == ce) {
						background[background_count++] = value;
					}
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 3 * (k + i);
					value = data16[kk] + data16[kk + 1] + data16[kk + 2];
					/* use border for background noise estimation */
					if (j == ls || j == le || i == cs || i == ce) {
						background[background_count++] = value;
					}
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
	}

	double average = sum / ((2 * radius + 1) * (2 * radius + 1));
	double stddev = indigo_stddev(background, background_count);

	/* Set threshold at 5 * standard deviation */
	double threshold = average + 5 * stddev;

	INDIGO_DEBUG(indigo_debug("Selection: threshold = %.3lf, max = %.3lf, average = %.3lf, stddev = %.3lf", threshold, max, average, stddev));

	/* If max is below the thresold no guiding is possible */
	if (max <= threshold) return INDIGO_GUIDE_ERROR;

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int j = ls; j <= le; j++) {
				for (int i = cs; i <= ce; i++) {
					value = clear_hot_pixel_8(data8, i, j, width, height) - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;
					m10 += (i + 1 - cs) * value;
					m01 += (j + 1 - ls) * value;
					m00 += value;
				}
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int j = ls; j <= le; j++) {
				for (int i = cs; i <= ce; i++) {
					value = clear_hot_pixel_16(data16, i, j, width, height) - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;
					m10 += (i + 1 - cs) * value;
					m01 += (j + 1 - ls) * value;
					m00 += value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 3 * (k + i);
					value = data8[kk] + data8[kk + 1] + data8[kk + 2] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;
					m10 += (i + 1 - cs) * value;
					m01 += (j + 1 - ls) * value;
					m00 += value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 4 * (k + i);
					value = data8[kk] + data8[kk + 1] + data8[kk + 2] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;
					m10 += (i + 1 - cs) * value;
					m01 += (j + 1 - ls) * value;
					m00 += value;
				}
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 4 * (k + i);
					value = data8[kk + 1] + data8[kk + 2] + data8[kk + 3] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;
					m10 += (i + 1 - cs) * value;
					m01 += (j + 1 - ls) * value;
					m00 += value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int j = ls; j <= le; j++) {
				int k = j * width;
				for (int i = cs; i <= ce; i++) {
					int kk = 3 * (k + i);
					value = data16[kk] + data16[kk + 1] + data16[kk + 2] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;
					m10 += (i + 1 - cs) * value;
					m01 += (j + 1 - ls) * value;
					m00 += value;
				}
			}
			break;
		}
	}

	digest->width = width;
	digest->height = height;
	/* Calculate centroid for the selection only, add the offset and subtract 0.5
	   as the centroid of a single pixel is 0.5,0.5 not 1,1.
	*/
	digest->centroid_x = *x = cs + m10 / m00 - 0.5;
	digest->centroid_y = *y = ls + m01 / m00 - 0.5;
	digest->snr = sqrt(m00);
	digest->donuts_snr = 0;
	digest->algorithm = centroid;
	INDIGO_DEBUG(indigo_debug("indigo_selection_frame_digest: centroid = [%5.2f, %5.2f], signal = %.3f, stddev_noise = %.3f, SNR = %3f", digest->centroid_x, digest->centroid_y, m00, stddev, digest->snr));
	return INDIGO_OK;
}

indigo_result indigo_centroid_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *digest) {
	if ((width < 3) || (height < 3))
		return INDIGO_FAILED;
	if ((data == NULL) || (digest == NULL))
		return INDIGO_FAILED;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;

	double m10 = 0, m01 = 0, m00 = 0;
	int ci = 0, li = 0, size = width * height;
	double sum = 0, max = 0;
	double value;
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int i = 0; i < size; i++) {
				value = data8[i];
				sum += value;
				if (value > max) max = value;
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int i = 0; i < size; i++) {
				value = data16[i];
				sum += value;
				if (value > max) max = value;
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int i = 0; i < 3 * size; i++) {
				value = data8[i] + data8[i + 1] + data8[i + 2];
				i += 2;
				sum += value;
				if (value > max) max = value;
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			for (int i = 0; i < 4 * size; i++) {
				value = data8[i] + data8[i + 1] + data8[i + 2];
				i += 3;
				sum += value;
				if (value > max) max = value;
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			for (int i = 0; i < 4 * size; i++) {
				value = data8[i + 1] + data8[i + 2] + data8[i + 3];
				i += 3;
				sum += value;
				if (value > max) max = value;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int i = 0; i < 3 * size; i++) {
				value = data16[i] + data16[i + 1] + data16[i + 2];
				i += 2;
				sum += value;
				if (value > max) max = value;
			}
			break;
		}
	}

	/* Set threshold 20% above average value */
	double threshold = 1.20 * sum / size;

	INDIGO_DEBUG(indigo_debug("Centroid: threshold = %.3f, max = %.3f", threshold, max));

	/* If max is below the thresold no guiding is possible */
	if (max <= threshold) return INDIGO_GUIDE_ERROR;

	ci = 1; li = 1;
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int i = 0; i < size; i++) {
				value = data8[i];
				m10 += ci * value;
				m01 += li * value;
				m00 += value;
				ci++;
				if (ci > width) {
					ci = 1;
					li++;
				}
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int i = 0; i < size; i++) {
				value = data16[i];
				m10 += ci * value;
				m01 += li * value;
				m00 += value;
				ci++;
				if (ci > width) {
					ci = 1;
					li++;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int i = 0; i < 3 * size; i++) {
				value = data8[i] + data8[i + 1] + data8[i + 2];
				i += 2;
				m10 += ci * value;
				m01 += li * value;
				m00 += value;
				ci++;
				if (ci > width) {
					ci = 1;
					li++;
				}
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			for (int i = 0; i < 4 * size; i++) {
				value = data8[i] + data8[i + 1] + data8[i + 2];
				i += 3;
				m10 += ci * value;
				m01 += li * value;
				m00 += value;
				ci++;
				if (ci > width) {
					ci = 1;
					li++;
				}
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			for (int i = 0; i < 4 * size; i++) {
				value = data8[i + 1] + data8[i + 2] + data8[i + 3];
				i += 3;
				m10 += ci * value;
				m01 += li * value;
				m00 += value;
				ci++;
				if (ci > width) {
					ci = 1;
					li++;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int i = 0; i < 3 * size; i++) {
				value = data16[i] + data16[i + 1] + data16[i + 2];
				i += 2;
				m10 += ci * value;
				m01 += li * value;
				m00 += value;
				ci++;
				if (ci > width) {
					ci = 1;
					li++;
				}
			}
			break;
		}
	}

	digest->width = width;
	digest->height = height;
	/* Calculate centroid for the frame and subtract 0.5
	   as the centroid of a single pixel is 0.5,0.5 not 1,1.
	*/
	digest->centroid_x = m10 / m00 - 0.5;
	digest->centroid_y = m01 / m00 - 0.5;
	digest->snr = sqrt(m00);
	digest->donuts_snr = 0;
	digest->algorithm = centroid;
	//INDIGO_DEBUG(indigo_debug("indigo_centroid_frame_digest: centroid = [%5.2f, %5.2f]", digest->centroid_x, digest->centroid_y));
	return INDIGO_OK;
}

#define BG_RADIUS	5

//static void remove_gradient(double (*vector)[2], int size) {
//	double x_mean = size / 2.0;
//	double y_mean = 0;
//	for (int i = 0; i < size; i++) {
//		y_mean += vector[i][RE];
//	}
//	y_mean /= size;
//	double numerator = 0.0, denominator = 0.0;
//	for (int i = 0; i < size; i++) {
//		numerator += (i - x_mean) * (vector[i][RE] - y_mean);
//		denominator += (i - x_mean) * (i - x_mean);
//	}
//	double slope = numerator / denominator;
//	for (int i = 0; i < size; i++) {
//		vector[i][RE] -= i * slope;
//	}
//}

// Function to calculate the signal-to-noise ratio (SNR)
double calculate_snr_8(uint8_t *array, int size) {
	double sum = 0;
	for (int i = 0; i < size; i++) {
		sum += array[i];
	}
	double mean = sum / size;

	sum = 0;
	for (int i = 0; i < size; i++) {
		sum += (array[i] - mean) * (array[i] - mean);
	}
	double stddev = sqrt(sum / size);

	double threshold = mean + stddev;

	double signal = 0;
	double noise = 0;
	int signal_count = 0;
	int noise_count = 0;

	for (int i = 0; i < size; i++) {
		if (array[i] >= threshold) {
			signal += array[i];
			signal_count++;
		} else {
			noise += array[i];
			noise_count++;
		}
	}

	double avg_signal = signal_count > 0 ? signal / signal_count : 0;
	double avg_noise = noise_count > 0 ? noise / noise_count : 0;

	double snr =  avg_noise > 0 ? avg_signal * avg_signal / (avg_noise * avg_noise) : 0;
	return snr;
}

double calculate_snr_16(uint16_t *array, int size) {
	double sum = 0;
	for (int i = 0; i < size; i++) {
		sum += array[i];
	}
	double mean = sum / size;

	sum = 0;
	for (int i = 0; i < size; i++) {
		sum += (array[i] - mean) * (array[i] - mean);
	}
	double stddev = sqrt(sum / size);

	double threshold = mean + stddev;

	double signal = 0;
	double noise = 0;
	int signal_count = 0;
	int noise_count = 0;

	for (int i = 0; i < size; i++) {
		if (array[i] >= threshold) {
			signal += array[i];
			signal_count++;
		} else {
			noise += array[i];
			noise_count++;
		}
	}

	double avg_signal = signal_count > 0 ? signal / signal_count : 0;
	double avg_noise = noise_count > 0 ? noise / noise_count : 0;

	double snr =  avg_noise > 0 ? avg_signal * avg_signal / (avg_noise * avg_noise) : 0;
	return snr;
}

static double calculate_donuts_snr(double (*array)[2], int size) {
	double sum = 0;
	for (int i = 0; i < size; i++) {
		sum += array[i][RE];
	}
	double mean = sum / size;

	sum = 0;
	for (int i = 0; i < size; i++) {
		double amplitude = sqrt(array[i][RE] * array[i][RE] + array[i][IM] * array[i][IM]);
		sum += (amplitude - mean) * (amplitude - mean);
	}
	double stddev = sqrt(sum / size);

	double threshold = mean + stddev;

	double signal = 0;
	double noise = 0;
	int signal_count = 0;
	int noise_count = 0;

	for (int i = 0; i < size; i++) {
		double amplitude = sqrt(array[i][RE] * array[i][RE] + array[i][IM] * array[i][IM]);
		if (amplitude >= threshold) {
			signal += amplitude;
			signal_count++;
		} else {
			noise += amplitude;
			noise_count++;
		}
	}

	double avg_signal = signal_count > 0 ? signal / signal_count : 0;
	double avg_noise = noise_count > 0 ? noise / noise_count : 0;

	double snr =  avg_noise > 0 ? avg_signal * avg_signal / (avg_noise * avg_noise) : 0;
	return snr;
}

static void calibrate_re(double (*vector)[2], int size) {
	int first = BG_RADIUS + 1, last = size - BG_RADIUS - 1;
	double mins[size];
//	remove_gradient(vector, size);
	for (int i = first; i <= last; i++) {
		double min = vector[i - BG_RADIUS][RE];
		for (int j = -BG_RADIUS + 1; j <= BG_RADIUS; j++) {
			double value = vector[i + j][RE];
			if (value < min) {
				min = value;
			}
		}
		mins[i] = min;
	}
	for (int i = 0; i < first; i++) {
		vector[i][RE] = 0;
	}
	for (int i = last + 1; i < size; i++) {
		vector[i][RE] = 0;
	}
	for (int i = first; i <= last; i++) {
		double value = vector[i][RE] - mins[i];
		vector[i][RE] = value;
	}
}

indigo_result indigo_donuts_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, const int edge_clipping, indigo_frame_digest *digest) {
	return indigo_donuts_frame_digest_clipped(raw_type, data, width, height, edge_clipping, edge_clipping, width - 2 * edge_clipping, height - 2 * edge_clipping, digest);
}

indigo_result indigo_donuts_frame_digest_clipped(indigo_raw_type raw_type, const void *data, const int width, const int height, const int include_left, const int include_top, const int include_width, const int include_height, indigo_frame_digest *digest) {
	if (include_width <= 0)
		return INDIGO_FAILED;
	if (include_height <= 0)
		return INDIGO_FAILED;
	if ((data == NULL) || (digest == NULL))
		return INDIGO_FAILED;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;

	double sum = 0, max = 0;
	const int ce = include_left + include_width, le = include_top + include_height;
	const int cs = include_left, ls = include_top;
	double value;
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int j = ls; j < le; j++) {
				for (int i = cs; i < ce; i++) {
					value = clear_hot_pixel_8(data8, i, j, width, height);
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int j = ls; j < le; j++) {
				for (int i = cs; i < ce; i++) {
					value = clear_hot_pixel_16(data16, i, j, width, height);
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int j = ls; j < le; j++) {
				int k = j * width;
				for (int i = cs; i < ce; i++) {
					int kk = 3 * (k + i);
					value = data8[kk] + data8[kk + 1] + data8[kk + 2];
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			for (int j = ls; j < le; j++) {
				int k = j * width;
				for (int i = cs; i < ce; i++) {
					int kk = 4 * (k + i);
					value = data8[kk] + data8[kk + 1] + data8[kk + 2];
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			for (int j = ls; j < le; j++) {
				int k = j * width;
				for (int i = cs; i < ce; i++) {
					int kk = 4 * (k + i);
					value = data8[kk + 1] + data8[kk + 2] + data8[kk + 3];
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int j = ls; j < le; j++) {
				int k = j * width;
				for (int i = cs; i < ce; i++) {
					int kk = 3 * (k + i);
					value = data16[kk] + data16[kk + 1] + data16[kk + 2];
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
	}

	/* Set threshold 15% above average value */
	double threshold = 1.15 * sum / (include_width * include_height);

	INDIGO_DEBUG(indigo_debug("Donuts: threshold = %.3f, max = %.3f", threshold, max));

	/* If max is below the thresold no guiding is possible */
	if (max <= threshold) return INDIGO_GUIDE_ERROR;

	digest->width = next_power_2(include_width);
	digest->height = next_power_2(include_height);
	double (*col_x)[2] = calloc(2 * digest->width * sizeof(double), 1);
	double (*col_y)[2] = calloc(2 * digest->height * sizeof(double), 1);
	double (*fcol_x)[2] = calloc(2 * digest->width * sizeof(double), 1);
	double (*fcol_y)[2] = calloc(2 * digest->height * sizeof(double), 1);
	digest->fft_x = indigo_safe_malloc(2 * digest->width * sizeof(double));
	digest->fft_y = indigo_safe_malloc(2 * digest->height * sizeof(double));

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					value = clear_hot_pixel_8(data8, i, j, width, height) - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i - include_left][RE] += value;
					col_y[y][RE] += value;
				}
			}
			digest->snr = calculate_snr_8(data8, width * height);
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					value = clear_hot_pixel_16(data16, i, j, width, height) - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i - include_left][RE] += value;
					col_y[y][RE] += value;
				}
			}
			digest->snr = calculate_snr_16(data16, width * height);
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					int offset = (i + (j * width)) * 3;
					value = data8[offset] + data8[offset + 1] + data8[offset + 2] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i - include_left][RE] += value;
					col_y[y][RE] += value;
				}
			}
			digest->snr = calculate_snr_8(data8, width * height * 3);
			break;
		}
		case INDIGO_RAW_RGBA32: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					int offset = (i + (j * width)) * 4;
					value = data8[offset] + data8[offset + 1] + data8[offset + 2] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i - include_left][RE] += value;
					col_y[y][RE] += value;
				}
			}
			digest->snr = calculate_snr_8(data8, width * height * 4);
			break;
		}
		case INDIGO_RAW_ABGR32: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					int offset = (i + (j * width)) * 4;
					value = data8[offset + 1] + data8[offset + 2] + data8[offset + 3] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i - include_left][RE] += value;
					col_y[y][RE] += value;
				}
			}
			digest->snr = calculate_snr_8(data8, width * height * 4);
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					int offset = (i + (j * width)) * 3;
					value = data16[offset] + data16[offset + 1] + data16[offset + 2] - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i - include_left][RE] += value;
					col_y[y][RE] += value;
				}
			}
			digest->snr = calculate_snr_16(data16, width * height * 3);
			break;
		}
	}

	switch (raw_type) {
		case INDIGO_RAW_MONO8:
		case INDIGO_RAW_MONO16: {
			calibrate_re(col_x, include_width);
			calibrate_re(col_y, include_height);

			fft(digest->width, col_x, digest->fft_x);
			fft(digest->height, col_y, digest->fft_y);
			break;
		}
		default: {
			/* Remove hot from the digest */
			fcol_x[0][RE] = median3(0, col_x[0][RE], col_x[1][RE]);
			for (int i = 1; i < include_width - 1; i++) {
				fcol_x[i][RE] = median3(col_x[i - 1][RE], col_x[i][RE], col_x[i + 1][RE]);
			}
			fcol_x[include_width - 1][RE] = median3(col_x[include_width - 2][RE], col_x[include_width - 1][RE], 0);

			fcol_y[0][RE] = median3(0, col_y[0][RE], col_y[1][RE]);
			for (int i = 1; i < include_height - 1; i++) {
				fcol_y[i][RE] = median3(col_y[i - 1][RE], col_y[i][RE], col_y[i + 1][RE]);
			}
			fcol_y[include_height - 1][RE] = median3(col_y[include_height - 2][RE], col_y[include_height - 1][RE], 0);

			calibrate_re(fcol_x, include_width);
			calibrate_re(fcol_y, include_height);

			fft(digest->width, fcol_x, digest->fft_x);
			fft(digest->height, fcol_y, digest->fft_y);
		}
	}

	digest->donuts_snr = (calculate_donuts_snr(digest->fft_x, digest->width) + calculate_donuts_snr(digest->fft_y, digest->height)) / 2.0;

	INDIGO_DEBUG(indigo_debug("Donuts: Frame SNR = %g, FFT SNR = %g", digest->snr, digest->donuts_snr));

	digest->algorithm = donuts;
	free(col_x);
	free(col_y);
	free(fcol_x);
	free(fcol_y);
	return INDIGO_OK;
}

/* Contrast routines */

indigo_result indigo_init_saturation_mask(const int width, const int height, uint8_t **mask) {
	int size = width * height;
	uint8_t *buf = indigo_safe_malloc(size * sizeof(uint8_t));
	if (buf == NULL)
		return INDIGO_FAILED;

	memset(buf, 1, size);
	*mask = buf;
	return INDIGO_OK;
}

indigo_result indigo_update_saturation_mask(indigo_raw_type raw_type, const void *data, const int width, const int height, uint8_t *mask) {
	if (data == NULL || mask == NULL) return INDIGO_FAILED;

	int size = width * height;
	switch (raw_type) {
		case INDIGO_RAW_RGB24:
		case INDIGO_RAW_RGB48:
			size *= 3;
			break;
		default:
			break;
	}

	/* mask ~4% of the frame around the saturated area */
	int mask_size = height / 25;

	uint16_t max_luminance;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;
	double sum = 0;

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			max_luminance = SATURATION_8;
			for (int i = 0; i < size; i++) {
				sum += data8[i];
				mask[i] = 1;
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			max_luminance = SATURATION_8;
			for (int i = 0; i < size; i++) {
				sum += data8[i];
				mask[i/3] = 1;
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			max_luminance = SATURATION_16;
			for (int i = 0; i < size; i++) {
				sum += data16[i];
				mask[i] = 1;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			max_luminance = SATURATION_16;
			for (int i = 0; i < size; i++) {
				sum += data16[i];
				mask[i/3] = 1;
			}
			break;
		}
		default:
			return INDIGO_FAILED;
	}

	double mean = sum / size;
	const double threshold = (max_luminance - mean) * 0.3 + mean;
	const int end_x = width - 1;
	const int end_y = height - 1;

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			for (int y = 1; y < end_y; y++) {
				for (int x = 1; x < end_x; x++) {
					int off = y * width + x;
					if (
						data8[off] > max_luminance &&
						/* also check median of the neighbouring pixels to avoid hot pixels and lines */
						median3(data8[off - 1], data8[off], data8[off + 1]) > threshold
					) {
						int min_i = MAX(0, x - mask_size);
						int max_i = MIN(width - 1, x + mask_size);
						int min_j = MAX(0, y - mask_size);
						int max_j = MIN(height - 1, y + mask_size);

						for (int j = min_j; j <= max_j; j++) {
							for (int i = min_i; i <= max_i; i++) {
								mask[j * width + i] = 0;
							}
						}
					}
				}
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int y = 1; y < end_y; y++) {
				for (int x = 1; x < end_x; x++) {
					int off = y * width + x;
					if (
						data16[off] > max_luminance &&
						/* also check median of the neighbouring pixels to avoid hot pixels and lines */
						median3(data16[off - 1], data16[off], data16[off + 1]) > threshold
					) {
						int min_i = MAX(0, x - mask_size);
						int max_i = MIN(width - 1, x + mask_size);
						int min_j = MAX(0, y - mask_size);
						int max_j = MIN(height - 1, y + mask_size);

						for (int j = min_j; j <= max_j; j++) {
							for (int i = min_i; i <= max_i; i++) {
								mask[j * width + i] = 0;
							}
						}
					}
				}
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			for (int y = 1; y < end_y; y++) {
				for (int x = 1; x < end_x; x++) {
					int off = 3 * (y * width + x);
					if (
						data8[off] > max_luminance &&
						/* also check median of the neighbouring pixels to avoid hot pixels and lines */
						(
							median3(data8[off - 3], data8[off], data8[off + 3]) > threshold ||       /* Red Saturated? */
							median3(data8[off - 2], data8[off + 1], data8[off + 4]) > threshold ||   /* Green Saturated? */
							median3(data8[off - 1], data8[off + 2], data8[off + 5]) > threshold      /* Blue Saturated? */
						)
					) {
						int min_i = MAX(0, x - mask_size);
						int max_i = MIN(width - 1, x + mask_size);
						int min_j = MAX(0, y - mask_size);
						int max_j = MIN(height - 1, y + mask_size);

						for (int j = min_j; j <= max_j; j++) {
							for (int i = min_i; i <= max_i; i++) {
								mask[j * width + i] = 0;
							}
						}
					}
				}
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			for (int y = 1; y < end_y; y++) {
				for (int x = 1; x < end_x; x++) {
					int off = 3 * (y * width + x);
					if (
						data16[off] > max_luminance &&
						/* also check median of the neighbouring pixels to avoid hot pixels and lines */
						(
							median3(data16[off - 3], data16[off], data16[off + 3]) > threshold ||       /* Red Saturated? */
							median3(data16[off - 2], data16[off + 1], data16[off + 4]) > threshold ||   /* Green Saturated? */
							median3(data16[off - 1], data16[off + 2], data16[off + 5]) > threshold      /* Blue Saturated? */
						)
					) {
						int min_i = MAX(0, x - mask_size);
						int max_i = MIN(width - 1, x + mask_size);
						int min_j = MAX(0, y - mask_size);
						int max_j = MIN(height - 1, y + mask_size);

						for (int j = min_j; j <= max_j; j++) {
							for (int i = min_i; i <= max_i; i++) {
								mask[j * width + i] = 0;
							}
						}
					}
				}
			}
			break;
		}
		default:
			return INDIGO_FAILED;
	}
	return INDIGO_OK;
}

double indigo_stddev(double set[], const int count) {
	double x = 0, d, m, sum = 0;

	if (count < 1) return 0;

	for (int i = 0; i < count; i++) {
		x += set[i];
	}
	m = x / count;

	for (int i = 0; i < count; i++) {
		d = set[i] - m;
		sum += d * d;
	}

	return sqrt(sum / count);
}

static double indigo_stddev_8(uint8_t set[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			sum += set[y * width + x];
			real_count++;
		}
	}
	m = sum / real_count;

	const double threshold = (SATURATION_8 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			int i = y * width + x;
			/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
			if (set[i] > SATURATION_8 && median3(set[i - 1], set[i], set[i + 1]) > threshold) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median3(set[i - 1], set[i], set[i + 1]), m));
					*saturated = true;
				}
			}
			d = set[i] - m;
			sum += d * d;
			real_count++;
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_masked_8(uint8_t set[], const uint8_t mask[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			int i = y * width + x;
			if (mask[i]) {
				sum += set[i];
				real_count++;
			}
		}
	}
	m = sum / real_count;

	const double threshold = (SATURATION_8 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			int i = y * width + x;
			if (mask[i]) {
				/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
				if (set[i] > SATURATION_8 && median3(set[i - 1], set[i], set[i + 1]) > threshold) {
					if (saturated) {
						if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median3(set[i - 1], set[i], set[i + 1]), m));
						*saturated = true;
					}
				}
				d = set[i] - m;
				sum += d * d;
				real_count++;
			}
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_masked_rgb24(uint8_t set[], const uint8_t mask[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;
	int index = 0, i = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			index = y * width + x;
			i = index * 3;
			if (mask[index]) {
				sum += set[i];
				sum += set[i + 1];
				sum += set[i + 2];
				real_count++;
			}
		}
	}
	m = sum / (real_count * 3);

	const double threshold = (SATURATION_8 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			index = y * width + x;
			i = index * 3;
			if (mask[index]) {
				/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
				if (
					(
						set[i] > SATURATION_8 ||
						set[i + 1] > SATURATION_8 ||
						set[i + 2] > SATURATION_8
					) && (
						median3(set[i - 3], set[i], set[i + 3]) > threshold ||
						median3(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
						median3(set[i - 1], set[i + 2], set[i + 5]) > threshold
					)
				) {
					if (saturated) {
						if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, mean = %.2f", threshold, m));
						*saturated = true;
					}
				}
				d = set[i] - m;
				sum += d * d;
				d = set[i + 1] - m;
				sum += d * d;
				d = set[i + 2] - m;
				sum += d * d;

				real_count++;
			}
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_rgb24(uint8_t set[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;
	int i = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			i = (y * width + x) * 3;
			sum += set[i];
			sum += set[i + 1];
			sum += set[i + 2];
			real_count++;
		}
	}
	m = sum / (real_count * 3);

	const double threshold = (SATURATION_8 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			i = (y * width + x) * 3;
			/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
			if (
				(
					set[i] > SATURATION_8 ||
					set[i + 1] > SATURATION_8 ||
					set[i + 2] > SATURATION_8
				) && (
					median3(set[i - 3], set[i], set[i + 3]) > threshold ||
					median3(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
					median3(set[i - 1], set[i + 2], set[i + 5]) > threshold
				)
			) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, mean = %.2f", threshold, m));
					*saturated = true;
				}
			}
			d = set[i] - m;
			sum += d * d;
			d = set[i + 1] - m;
			sum += d * d;
			d = set[i + 2] - m;
			sum += d * d;

			real_count++;
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_16(uint16_t set[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			sum += set[y * width + x];
			real_count++;
		}
	}
	m = sum / real_count;

	const double threshold = (SATURATION_16 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			int i = y * width + x;
			/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
			if (set[i] > SATURATION_16 && median3(set[i - 1], set[i], set[i + 1]) > threshold) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median3(set[i - 1], set[i], set[i + 1]), m));
					*saturated = true;
				}
			}
			d = set[i] - m;
			sum += d * d;
			real_count++;
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_masked_16(uint16_t set[], const uint8_t mask[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			int i = y * width + x;
			if (mask[i]) {
				sum += set[i];
				real_count++;
			}
		}
	}
	m = sum / real_count;

	const double threshold = (SATURATION_16 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			int i = y * width + x;
			if (mask[i]) {
				/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
				if (set[i] > SATURATION_16 && median3(set[i - 1], set[i], set[i + 1]) > threshold) {
					if (saturated) {
						if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median3(set[i - 1], set[i], set[i + 1]), m));
						*saturated = true;
					}
				}
				d = set[i] - m;
				sum += d * d;
				real_count++;
			}
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_masked_rgb48(uint16_t set[], const uint8_t mask[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;
	int index = 0, i = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			index = y * width + x;
			i = index * 3;
			if (mask[index]) {
				sum += set[i];
				sum += set[i + 1];
				sum += set[i + 2];
				real_count++;
			}
		}
	}
	m = sum / (real_count * 3);

	const double threshold = (SATURATION_16 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			index = y * width + x;
			i = index * 3;
			if (mask[index]) {
				/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
				if (
					(
						set[i] > SATURATION_16 ||
						set[i + 1] > SATURATION_16 ||
						set[i + 2] > SATURATION_16
					) && (
						median3(set[i - 3], set[i], set[i + 3]) > threshold ||
						median3(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
						median3(set[i - 1], set[i + 2], set[i + 5]) > threshold
					)
				) {
					if (saturated) {
						if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, mean = %.2f", threshold, m));
						*saturated = true;
					}
				}
				d = set[i] - m;
				sum += d * d;
				d = set[i + 1] - m;
				sum += d * d;
				d = set[i + 2] - m;
				sum += d * d;

				real_count++;
			}
		}
	}

	return sqrt(sum / real_count);
}

static double indigo_stddev_rgb48(uint16_t set[], const int width, const int height, bool *saturated) {
	double d, m, sum = 0;
	int real_count = 0;
	int i = 0;

	if (saturated) *saturated = false;

	const int end_x = width - 1;
	const int end_y = height - 1;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			i = y * width + x;
			sum += set[i];
			sum += set[i + 1];
			sum += set[i + 2];
			real_count++;
		}
	}
	m = sum / (real_count * 3);

	const double threshold = (SATURATION_16 - m) * 0.3 + m;
	real_count = 0;
	sum = 0;

	for (int y = 1; y < end_y; y++) {
		for (int x = 1; x < end_x; x++) {
			i = (y * width + x) * 3;
			/* Check if saturated feature or hotpixel, hotpixels do not break the estimation */
			if (
				(
					set[i] > SATURATION_16 ||
					set[i + 1] > SATURATION_16 ||
					set[i + 2] > SATURATION_16
				) && (
					median3(set[i - 3], set[i], set[i + 3]) > threshold ||
					median3(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
					median3(set[i - 1], set[i + 2], set[i + 5]) > threshold
				)
			) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, mean = %.2f", threshold, m));
					*saturated = true;
				}
			}
			d = set[i] - m;
			sum += d * d;
			d = set[i + 1] - m;
			sum += d * d;
			d = set[i + 2] - m;
			sum += d * d;

			real_count++;
		}
	}

	return sqrt(sum / real_count);
}

double indigo_contrast(indigo_raw_type raw_type, const void *data, const uint8_t *saturation_mask, const int width, const int height, bool *saturated) {
	if (width <= 0 || height <=0 || data == NULL) return INDIGO_FAILED;

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			if (saturation_mask == NULL) {
				return indigo_stddev_8((uint8_t*)data, width, height, saturated) / 255.0;
			} else {
				return indigo_stddev_masked_8((uint8_t*)data, saturation_mask, width, height, saturated) / 255.0;
			}
		}
		case INDIGO_RAW_MONO16: {
			if (saturation_mask == NULL) {
				return indigo_stddev_16((uint16_t*)data, width, height, saturated) / 65535.0;
			} else {
				return indigo_stddev_masked_16((uint16_t*)data, saturation_mask, width, height, saturated) / 65535.0;
			}
		}
		case INDIGO_RAW_RGB24: {
			if (saturation_mask == NULL) {
				return indigo_stddev_rgb24((uint8_t*)data, width, height, saturated) / 255.0;
			} else {
				return indigo_stddev_masked_rgb24((uint8_t*)data, saturation_mask, width, height, saturated) / 255.0;
			}
		}
		case INDIGO_RAW_RGB48: {
			if (saturation_mask == NULL) {
				return indigo_stddev_rgb48((uint16_t*)data, width, height, saturated) / 65535.0;
			} else {
				return indigo_stddev_masked_rgb48((uint16_t*)data, saturation_mask, width, height, saturated) / 65535.0;
			}
		}
		case INDIGO_RAW_RGBA32: {
			return 0;
		}
		case INDIGO_RAW_ABGR32: {
			return 0;
		}
		default:
			return 0;
	}
}

/* multistar guide */

indigo_result indigo_reduce_multistar_digest(const indigo_frame_digest *avg_ref, const indigo_frame_digest ref[], const indigo_frame_digest new_digest[], const int count, indigo_frame_digest *digest) {
	double drifts[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	double drifts_x[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	double drifts_y[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	double average = 0;
	double drift_x, drift_y;

	if (
		count < 1 ||
		avg_ref->algorithm != centroid ||
		ref[0].algorithm != centroid ||
		new_digest[0].algorithm != centroid ||
		digest == NULL
	) return INDIGO_FAILED;

	digest->algorithm = centroid;
	digest->width = new_digest[0].width;
	digest->height = new_digest[0].height;
	digest->snr = new_digest[0].snr;
	digest->centroid_x = avg_ref->centroid_x;
	digest->centroid_y = avg_ref->centroid_y;

	for (int i = 0; i < count; i++) {
		indigo_calculate_drift(&ref[i], &new_digest[i], &drift_x, &drift_y);
		drifts_x[i] = drift_x;
		drifts_y[i] = drift_y;
		drifts[i] = sqrt(drift_x * drift_x + drift_y * drift_y);
		average += drifts[i];
	}
	average /= count;
	double stddev = indigo_stddev(drifts, count);

	INDIGO_DEBUG(indigo_debug("%s: average = %.4f stddev = %.4f", __FUNCTION__, average, stddev));

	drift_x = 0;
	drift_y = 0;
	int used_count = 0;
	// calculate average drift with removed outliers (cut off at 1.5 * stddev)
	// for count <= 2 use simple average
	for (int i = 0; i < count; i++) {
		if (count <= 2 || fabs(average - drifts[i]) <= 1.5 * stddev) {
			used_count++;
			drift_x += drifts_x[i];
			drift_y += drifts_y[i];
			INDIGO_DEBUG(indigo_debug("%s: ++ Used star [%d] drift = %.4f", __FUNCTION__, i, drifts[i]));
		} else {
			INDIGO_DEBUG(indigo_debug("%s: -- Skip star [%d] drift = %.4f", __FUNCTION__, i, drifts[i]));
		}
	}

	if (used_count < 1) {
		return INDIGO_GUIDE_ERROR;
	}

	drift_x /= used_count;
	drift_y /= used_count;
	digest->centroid_x += drift_x;
	digest->centroid_y += drift_y;
	INDIGO_DEBUG(indigo_debug("%s: == Result using %d of %d stars. Drifts = ( %.3f, %.3f ) digest = ( %.3f, %.3f )", __FUNCTION__, used_count, count, drift_x, drift_y, digest->centroid_x, digest->centroid_y));
	return INDIGO_OK;
}

indigo_result indigo_reduce_weighted_multistar_digest(const indigo_frame_digest *avg_ref, const indigo_frame_digest ref[], const indigo_frame_digest new_digest[], const int count, indigo_frame_digest *digest) {
	double drifts[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	double drifts_x[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	double drifts_y[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	double average = 0;
	double drift_x, drift_y;

	if (
		count < 1 ||
		avg_ref->algorithm != centroid ||
		ref[0].algorithm != centroid ||
		new_digest[0].algorithm != centroid ||
		digest == NULL
	) return INDIGO_FAILED;

	digest->algorithm = centroid;
	digest->width = new_digest[0].width;
	digest->height = new_digest[0].height;
	digest->snr = new_digest[0].snr;
	digest->centroid_x = avg_ref->centroid_x;
	digest->centroid_y = avg_ref->centroid_y;

	for (int i = 0; i < count; i++) {
		indigo_calculate_drift(&ref[i], &new_digest[i], &drift_x, &drift_y);
		drifts_x[i] = drift_x;
		drifts_y[i] = drift_y;
		drifts[i] = sqrt(drift_x * drift_x + drift_y * drift_y);
		average += drifts[i];
	}
	average /= count;
	double stddev = indigo_stddev(drifts, count);

	INDIGO_DEBUG(indigo_debug("%s: average = %.4f stddev = %.4f", __FUNCTION__, average, stddev));

	drift_x = 0;
	drift_y = 0;
	int used_count = 0;
	double sum_weights = 0;
	// calculate weigthed average drift with removed outliers (cut off at 1.5 * stddev)
	// for count <= 2 use weigthed average
	for (int i = 0; i < count; i++) {
		double weight = sqrt(new_digest[i].snr);
		if (count <= 2 || fabs(average - drifts[i]) <= 1.5 * stddev) {
			used_count++;
			drift_x += drifts_x[i] * weight;
			drift_y += drifts_y[i] * weight;
			sum_weights += weight;
			INDIGO_DEBUG(indigo_debug("%s: ++ Used star [%d] drift = %.4f, weight = %.4f", __FUNCTION__, i, drifts[i], weight));
		} else {
			INDIGO_DEBUG(indigo_debug("%s: -- Skip star [%d] drift = %.4f, weight = %.4f", __FUNCTION__, i, drifts[i], weight));
		}
	}

	if (used_count < 1) {
		return INDIGO_GUIDE_ERROR;
	}

	drift_x /= sum_weights;
	drift_y /= sum_weights;
	digest->centroid_x += drift_x;
	digest->centroid_y += drift_y;
	INDIGO_DEBUG(indigo_debug("%s: == Result using %d of %d stars. Drifts = ( %.3f, %.3f ) digest = ( %.3f, %.3f )", __FUNCTION__, used_count, count, drift_x, drift_y, digest->centroid_x, digest->centroid_y));
	return INDIGO_OK;
}

double indigo_guider_reponse(double p_gain, double i_gain, double guide_cycle_time, double drift, double avg_drift) {
	double response = -1 * (p_gain * drift + i_gain * avg_drift * guide_cycle_time);
	INDIGO_DEBUG(indigo_debug("%s(): P = %.4f, I = %.4f, response = %.4f, drift = %.4f, avg_drift = %.4f", __FUNCTION__, p_gain, i_gain, response, drift, avg_drift));
	return response;
}

indigo_result indigo_calculate_drift(const indigo_frame_digest *ref, const indigo_frame_digest *new_digest, double *drift_x, double *drift_y) {
	if (ref == NULL || new_digest == NULL || drift_x == NULL || drift_y == NULL)
		return INDIGO_FAILED;
	if ((ref->width != new_digest->width) || (ref->height != new_digest->height))
		return INDIGO_FAILED;
	if (ref->algorithm == centroid) {
		*drift_x = new_digest->centroid_x - ref->centroid_x;
		*drift_y = new_digest->centroid_y - ref->centroid_y;
		return INDIGO_OK;
	}
	if (ref->algorithm == donuts) {
		double (*c_buf)[2];
		int max_dim = (ref->width > ref->height) ? ref->width : ref->height;
		c_buf = indigo_safe_malloc(2 * max_dim * sizeof(double));
		/* find X correction */
		corellate_fft(ref->width, new_digest->fft_x, ref->fft_x, c_buf);
		*drift_x = find_distance(ref->width, c_buf);
		/* find Y correction */
		corellate_fft(ref->height, new_digest->fft_y, ref->fft_y, c_buf);
		*drift_y = find_distance(ref->height, c_buf);
		free(c_buf);
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_delete_frame_digest(indigo_frame_digest *fdigest) {
	if (fdigest) {
		if (fdigest->algorithm == donuts) {
			if (fdigest->fft_x)
				free(fdigest->fft_x);
			if (fdigest->fft_y)
				free(fdigest->fft_y);
		}
		fdigest->width = 0;
		fdigest->height = 0;
		fdigest->algorithm = none;
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

static const double FIND_STAR_EDGE_CLIPPING = 20;

static int luminance_comparator(const void *item_1, const void *item_2) {
	if (((indigo_star_detection *)item_1)->luminance < ((indigo_star_detection *)item_2)->luminance)
		return 1;
	if (((indigo_star_detection *)item_1)->luminance > ((indigo_star_detection *)item_2)->luminance)
		return -1;
	return 0;
}

/* With radius < 3, no precise star positins will be determined */
indigo_result indigo_find_stars_precise(indigo_raw_type raw_type, const void *data, const uint16_t radius, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found) {
	if (data == NULL || star_list == NULL || stars_found == NULL) return INDIGO_FAILED;

	int  size = width * height;
	uint16_t *buf = indigo_safe_malloc(size * sizeof(uint16_t));
	int star_size = 100;
	const int clip_edge = height >= FIND_STAR_EDGE_CLIPPING * 4 ? FIND_STAR_EDGE_CLIPPING : (height / 4);
	int clip_width  = width - clip_edge;
	int clip_height = height - clip_edge;
	uint16_t max_luminance = 0;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;
	double sum = 0;
	double sum_sq = 0;

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			max_luminance = 0xFF;
			for (int i = 0; i < size; i++) {
				buf[i] = data8[i];
				sum += buf[i];
				sum_sq += buf[i] * buf[i];
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			max_luminance = 0xFFFF;
			for (int i = 0; i < size; i++) {
				buf[i] = data16[i];
				sum += buf[i];
				sum_sq += buf[i] * buf[i];
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			max_luminance = 0xFF;
			for (int i = 0, j = 0; i < 3 * size; i++, j++) {
				buf[j] = (data8[i] + data8[i + 1] + data8[i + 2]) / 3;
				sum += buf[j];
				sum_sq += buf[j] * buf[j];
				i += 2;
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			max_luminance = 0xFF;
			for (int i = 0, j = 0; i < 4 * size; i++, j++) {
				buf[j] = (data8[i] + data8[i + 1] + data8[i + 2]) / 3;
				sum += buf[j];
				sum_sq += buf[j] * buf[j];
				i += 3;
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			max_luminance = 0xFF;
			for (int i = 0, j = 0; i < 4 * size; i++, j++) {
				buf[j] = (data8[i + 1] + data8[i + 2] + data8[i + 3]) / 3;
				sum += buf[j];
				sum_sq += buf[j] * buf[j];
				i += 3;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			max_luminance = 0xFFFF;
			for (int i = 0, j = 0; i < 3 * size; i++, j++) {
				buf[j] = (data16[i] + data16[i + 1] + data16[i + 2]) / 3;
				sum += buf[j];
				sum_sq += buf[j] * buf[j];
				i += 2;
			}
			break;
		}
	}

	// Calculate mean
	double mean = sum / size;

	/* Calculate standard deviation - simplified, approximate estimate,
	   with a nice property that it is less affected by outliers. This proeprty
	   fixes the issue with finding guide stars in the presence of saturated stars,
	   as it effectively filters out the outliers.
	*/
	double stddev = sqrt(fabs(sum_sq / size - mean * mean));

	/* Calculate threshold - add 4.5 stddev threshold for stars */
	uint32_t threshold = 4.5 * stddev + mean;
	indigo_debug("%s(): image mean = %.2f, simplified stddev = %.2f, star detection threshold = %d", __FUNCTION__, mean, stddev, threshold);

	int threshold_hist = threshold * 0.9;

	int found = 0;
	int width2 = width / 2;
	int height2 = height / 2;
	uint32_t lmax = threshold + 1;

	indigo_star_detection star = { 0 };
	int divider = (width > height) ? height2 : width2;
	while (lmax > threshold) {
		lmax = threshold;
		star.x = 0;
		star.y = 0;
		star.nc_distance = 0;
		star.luminance = 0;
		star.oversaturated = 0;

		for (int j = clip_edge; j < clip_height; j++) {
			for (int i = clip_edge; i < clip_width; i++) {
				int off = j * width + i;
				if (
					buf[off] > lmax &&
					/* also check median of the neighbouring pixels to avoid hot pixels and lines */
					median3(buf[off - 1], buf[off], buf[off + 1]) > threshold &&
					median3(buf[off - width], buf[off], buf[off + width]) > threshold &&
					median3(buf[off - width - 1], buf[off], buf[off + width + 1]) > threshold &&
					median3(buf[off - width + 1], buf[off], buf[off + width - 1]) > threshold
				) {
					lmax = buf[off];
					star.x = i;
					star.y = j;
				}
			}
		}
		if (lmax > threshold) {
			double luminance = 0;
			int min_i = MAX(0, star.x - star_size);
			int max_i = MIN(width - 1, star.x + star_size);
			int min_j = MAX(0, star.y - star_size);
			int max_j = MIN(height - 1, star.y + star_size);
			int star_x = (int)star.x;
			int star_y = (int)star.y;
			// clear +X, +Y quadrant
			for (int j = star_y; j <= max_j; j++) {
				if (buf[j * width + star_x] < threshold_hist) break;
				for (int i = star_x; i <= max_i; i++) {
					int off = j * width + i;
					if (buf[off] > threshold_hist) {
						luminance += buf[off] - threshold_hist;
						buf[off] = 0;
					} else {
						break;
					}
				}
			}
			// clear -X, +Y quadrant
			for (int j = star_y; j <= max_j; j++) {
				if (buf[j * width + star_x - 1] < threshold_hist) break;
				for (int i = star_x - 1; i >= min_i; i--) {
					int off = j * width + i;
					if (buf[off] > threshold_hist) {
						luminance += buf[off] - threshold_hist;
						buf[off] = 0;
					} else {
						break;
					}
				}
			}
			// clear +X, -Y quadrant
			for (int j = star_y - 1; j >= min_j; j--) {
				if (buf[j * width + star_x] < threshold_hist) break;
				for (int i = star_x; i <= max_i; i++) {
					int off = j * width + i;
					if (buf[off] > threshold_hist) {
						luminance += buf[off] - threshold_hist;
						buf[off] = 0;
					} else {
						break;
					}
				}
			}
			// clear -X, -Y quadrant
			for (int j = star_y - 1; j >= min_j; j--) {
				if (buf[j * width + star_x - 1] < threshold_hist) break;
				for (int i = star_x - 1; i >= min_i; i--) {
					int off = j * width + i;
					if (buf[off] > threshold_hist) {
						luminance += buf[off] - threshold_hist;
						buf[off] = 0;
					} else {
						break;
					}
				}
			}

			indigo_result res = INDIGO_FAILED;
			if (radius >= 3) {
				indigo_frame_digest center = {0};
				res = indigo_selection_frame_digest_iterative(raw_type, data, &star.x, &star.y, radius, width, height, &center, 2);
				star.x = center.centroid_x;
				star.y = center.centroid_y;
				star.close_to_other = false;
				if(res == INDIGO_OK) {
					indigo_delete_frame_digest(&center);
				}
			}

			/* Check if the star is a duplicate (probably artifact) or is in close proximity to another one.
			   In both cses these stars should not be used */
			if (res == INDIGO_OK || radius < 3) {
				for (int i = 0; i < found; i++) {
					double dx = fabs(star_list[i].x - star.x);
					double dy = fabs(star_list[i].y - star.y);
					if (dx < 1 && dy < 1) {
						/* The star (probably artifact) is a duplicate of another star.
						   We mark the other star as being close to another one, so it
						   won't be used automatically, and we skip the duplicate. */
						indigo_debug("indigo_find_stars(): star (%lf, %lf) skipped, duplicate of #%u = (%lf, %lf)", star.x, star.y, i + 1, star_list[i].x, star_list[i].y);
						star_list[i].close_to_other = true;
						res = INDIGO_FAILED;
						break;
					} else if (dx < radius && dy < radius) {
						/* The star is too close to another star.
						   We mark both star as being close to another one, so they
						   won't be used automatically but we keep both stars in the list. */
						indigo_debug("indigo_find_stars(): star (%lf, %lf), too close to #%u = (%lf, %lf)", star.x, star.y, i + 1, star_list[i].x, star_list[i].y);
						star.close_to_other = true;
						star_list[i].close_to_other = true;
						break;
					}
				}
			}

			if (res == INDIGO_OK || radius < 3) {
				star.oversaturated = lmax == max_luminance;
				star.nc_distance = sqrt((star.x - width2) * (star.x - width2) + (star.y - height2) * (star.y - height2));
				star.nc_distance /= divider;
				star.luminance = (luminance > 0) ? log(fabs(luminance)) : 0;
				star_list[found++] = star;
			}
		}
		if (found >= stars_max) {
			break;
		}
	}
	free(buf);

	qsort(star_list, found, sizeof(indigo_star_detection), luminance_comparator);

	INDIGO_DEBUG(
		for (size_t i = 0;i < found; i++) {
			indigo_debug(
				"%s: star #%u = (%lf, %lf), ncdist = %lf, lum = %lf, close_to_other = %d, oversaturated = %d",
				__FUNCTION__,
				i+1,
				star_list[i].x,
				star_list[i].y,
				star_list[i].nc_distance,
				star_list[i].luminance,
				star_list[i].close_to_other,
				star_list[i].oversaturated
			);
		}
	)

	*stars_found = found;
	return INDIGO_OK;
}

indigo_result indigo_find_stars(indigo_raw_type raw_type, const void *data, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found) {
	return indigo_find_stars_precise(raw_type, data, 0, width, height, stars_max, star_list, stars_found);
}

indigo_result indigo_find_stars_precise_filtered(indigo_raw_type raw_type, const void *data, const uint16_t radius, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found) {
	int safety_margin = width < height ? width * 0.05 : height * 0.05;
	return indigo_find_stars_precise_clipped(raw_type, data, radius, width, height, stars_max, safety_margin, safety_margin, width - 2 * safety_margin, height - 2 * safety_margin, 0, 0, 0, 0, star_list, stars_found);
}

indigo_result indigo_find_stars_precise_clipped(indigo_raw_type raw_type, const void *data, const uint16_t radius, const int width, const int height, const int stars_max, const int include_left, const int include_top, const int include_width, const int include_height, const int exclude_left, const int exclude_top, const int exclude_width, const int exclude_height, indigo_star_detection star_list[], int *stars_found) {
	indigo_result res = indigo_find_stars_precise(raw_type, data, radius, width, height, stars_max, star_list, stars_found);
	if (res != INDIGO_OK) {
		return res;
	}
	/* Filter out stars that are too close to the edges or are oversaturated */
	int include_right = include_left + include_width;
	int include_bottom = include_top + include_height;
	bool clip_excluded = exclude_width > 0 && exclude_height > 0;
	int exclude_right = exclude_left + exclude_width;
	int exclude_bottom = exclude_top + exclude_height;
	int usable = 0;
	for (int i = 0; i < *stars_found; i++) {
		if (star_list[i].oversaturated || star_list[i].close_to_other) {
			indigo_debug("%s: SKIP star #%d (%lf, %lf), oversaturated = %d close_to_other = %d", __FUNCTION__, i+1, star_list[i].x, star_list[i].y, star_list[i].oversaturated, star_list[i].close_to_other);
			continue;
		}
		if (star_list[i].x < include_left || include_right < star_list[i].x || star_list[i].y < include_top || include_bottom < star_list[i].y) {
			indigo_debug("%s: NOT INCLUDED star #%d (%lf, %lf)", __FUNCTION__, i+1, star_list[i].x, star_list[i].y);
			continue;
		}
		if (clip_excluded && ((exclude_left < star_list[i].x && star_list[i].x < exclude_right) || (exclude_top < star_list[i].y && star_list[i].y < exclude_bottom))) {
			indigo_debug("%s: EXCLUDED star #%d (%lf, %lf)", __FUNCTION__, i+1, star_list[i].x, star_list[i].y);
			continue;
		}
		indigo_debug("%s: KEEP star #%d (%lf, %lf) , oversaturated = %d close_to_other = %d", __FUNCTION__, i+1, star_list[i].x, star_list[i].y, star_list[i].oversaturated, star_list[i].close_to_other);
		star_list[usable++] = star_list[i];
	}
	indigo_debug("%s: %d usable stars of %d found", __FUNCTION__, usable, *stars_found);
	*stars_found = usable;
	//for (int i = 0; i < usable; i++) {
	//	indigo_debug("%s: usable star #%d (%lf, %lf)", __FUNCTION__, i+1, star_list[i].x, star_list[i].y);
	//}
	return INDIGO_OK;
}


static int nc_distance_comparator(const void *item_1, const void *item_2) {
	if (((indigo_star_detection *)item_1)->nc_distance < ((indigo_star_detection *)item_2)->nc_distance)
		return 1;
	if (((indigo_star_detection *)item_1)->nc_distance > ((indigo_star_detection *)item_2)->nc_distance)
		return -1;
	return 0;
}

indigo_result indigo_make_psf_map(indigo_raw_type image_raw_type, const void *image_data, const uint16_t radius, const int image_width, const int image_height, const int stars_max, indigo_raw_type map_raw_type, indigo_psf_param map_type, int map_width, int map_height, unsigned char *map_data, double *psf_min, double *psf_max) {
	int pixel_size = 0;
	switch (map_raw_type) {
		case INDIGO_RAW_RGB24:
			pixel_size = 3;
			break;
		case INDIGO_RAW_RGBA32:
			pixel_size = 4;
			break;
		default:
			indigo_error("Unsupported HFD map format");
			return INDIGO_FAILED;
	}
	char *label = "";
	double map_scale = (double)image_width / (double)map_width;
	// extract PSF to nc_distance
	indigo_star_detection *stars = indigo_safe_malloc(stars_max * sizeof(indigo_star_detection));
	int total_stars = 0, used_stars = 0;
	indigo_find_stars_precise(image_raw_type, image_data, radius, image_width, image_height, stars_max, stars, &total_stars);
	for (int i = 0; i < total_stars; i++) {
		indigo_star_detection *star = stars + i;
		if (star->oversaturated || star->close_to_other)
			continue;
		double star_fwhm, star_hfd, star_peak;
		indigo_selection_psf(image_raw_type, image_data, star->x, star->y, radius, image_width, image_height, &star_fwhm, &star_hfd, &star_peak);
		star->x /= map_scale; // scale to map coordimates
		star->y /= map_scale;
		switch (map_type) {
			case fwhm:
				star->nc_distance = star_fwhm;
				label = "FWHM";
				break;
			case hfd:
				star->nc_distance = star_hfd;
				label = "HFD";
				break;
			case peak:
				star->nc_distance = star_peak;
				label = "peak";
				break;
		}
		if (i > used_stars)
			memcpy(stars + used_stars, star, sizeof(indigo_star_detection));
		used_stars++;
		//INDIGO_DEBUG(indigo_debug("%g %g %g %g", star->x, star->y, fwhm, hfd, peak));
	}
	// clip top and bottom 10%
	qsort(stars, used_stars, sizeof(indigo_star_detection), nc_distance_comparator);
	int first_star = used_stars / 10;
	int last_star = used_stars - first_star;
	// compute PSF averages
	double *psfs = indigo_safe_malloc(map_width * map_height * sizeof(double));
	double max_distance = map_width / 4;
	double max_psf = 0, min_psf = 100000;
	for (int j = 0; j < map_height; j++) {
		int jj = j * map_width;
		for (int i = 0; i < map_width; i++) {
			double avg = 0;
			int count = 0;
			for (int k = first_star; k <= last_star; k++) {
				indigo_star_detection *star = stars + k;
				double distance_x = i - star->x + 0.5;
				double distance_y = j - star->y + 0.5;
				double distance = sqrt(distance_x * distance_x + distance_y * distance_y);
				if (distance <= max_distance) {
					avg += star->nc_distance;
					count++;
				}
			}
			int ii = jj +  i;
			if (count > 0) {
				avg = avg / count;
				if (avg < min_psf)
					min_psf = avg;
				if (avg > max_psf)
					max_psf = avg;
				psfs[ii] = avg;
			} else {
				psfs[ii] = 0;
			}
			if (map_raw_type == INDIGO_RAW_RGBA32)
				map_data[ii + 3] = 255;
		}
	}
	if (psf_min)
		*psf_min = min_psf;
	if (psf_max)
		*psf_max = max_psf;
	indigo_log("Inspect %s: Star count = %d, MIN = %g, MAX = %g", label, last_star - first_star, min_psf, max_psf);
	// create PSF map from PSF averages
	double psf_scale = (max_psf - min_psf) / 8;
	for (int j = 0; j < map_height; j++) {
		int jj = j * map_width;
		for (int i = 0; i < map_width; i++) {
			int ii = jj + i;
			int iii = pixel_size * ii;
			double avg = psfs[ii];
			if (avg > 0) {
				int value = 31 * round((avg - min_psf) / psf_scale);
				map_data[iii] = value;
				map_data[iii + 1] = 255 - value;
				map_data[iii + 2] =  0;
			} else {
				map_data[iii] = map_data[iii + 1] = 0;
				map_data[iii + 2] = 255;
			}
			if (map_raw_type == INDIGO_RAW_RGBA32)
				map_data[iii + 3] = 255;
		}
	}
// draw stars over PSF map
//	for (int k = first_star; k <= last_star; k++) {
//		indigo_star_detection *star = stars + k;
//		int i = round(star->x + 0.5);
//		int j = round(star->y + 0.5);
//		int value = 31 * (star->nc_distance - min_psf) / psf_scale;
//		int c = pixel_size * (j * map_width + i);
//		map_data[c + 2] = 0;
//		map_data[c + 1] = value;
//		map_data[c] = 255 - value;
//	}
	indigo_safe_free(psfs);
	indigo_safe_free(stars);
	return INDIGO_OK;
}

//static void save_pgm(const char* filename, const uint8_t* mono, int width, int height) {
//	FILE* file = fopen(filename, "wb");
//	fprintf(file, "P5\n%d %d\n255\n", width, height);
//	fwrite(mono, 1, width * height, file);
//	fclose(file);
//}
//
//static void save_ppm(const char* filename, const uint8_t* rgb, int width, int height) {
//	FILE* file = fopen(filename, "wb");
//	fprintf(file, "P6\n%d %d\n255\n", width, height);
//	fwrite(rgb, 1, width * height * 3, file);
//	fclose(file);
//}

// creates INDIGO_RAW_MONO8 with 0 or 255 values only

uint8_t* indigo_binarize(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma) {
	int size = width * height;
	long sum = 0;
	long sum_sq = 0;
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int value = source_pixels[i];
				sum += value;
				sum_sq += value * value;
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			uint16_t *source_pixels = (uint16_t *)data;
			for (int i = 0; i < size; i++) {
				int value = source_pixels[i];
				sum += value;
				sum_sq += value * value;
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int i3 = 3 * i;
				int value = (source_pixels[i3] + source_pixels[i3 + 1] + source_pixels[i3 + 2]) / 3;
				sum += value;
				sum_sq += value * value;
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int i4 = 4 * i;
				int value = (source_pixels[i4] + source_pixels[i4 + 1] + source_pixels[i4 + 2]) / 3;
				sum += value;
				sum_sq += value * value;
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int i4 = 4 * i;
				int value = (source_pixels[i4 + 1] + source_pixels[i4 + 2] + source_pixels[i4 + 3]) / 3;
				sum += value;
				sum_sq += value * value;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			uint16_t *source_pixels = (uint16_t *)data;
			for (int i = 0; i < size; i++) {
				int i3 = 3 * i;
				int value = (source_pixels[i3] + source_pixels[i3 + 1] + source_pixels[i3 + 2]) / 3;
				sum += value;
				sum_sq += value * value;
			}
			break;
		}
	}
	double mean = (double)sum / size;
	double stddev = sqrt((double)sum_sq / size - mean * mean);
	int threshold = mean + (sigma * stddev);
	sum = 0;
	uint8_t *target_pixels = (uint8_t *)indigo_safe_malloc(size);
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				target_pixels[i] = source_pixels[i] > threshold ? (void)(sum++), 255 : 0;
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			uint16_t *source_pixels = (uint16_t *)data;
			for (int i = 0; i < size; i++) {
				target_pixels[i] = source_pixels[i] > threshold ? (void)(sum++), 255 : 0;
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			uint8_t *source_pixels = (uint8_t *)data;
			threshold *= 3;
			for (int i = 0; i < size; i++) {
				int i3 = 3 * i;
				target_pixels[i] = (source_pixels[i3] + source_pixels[i3 + 1] + source_pixels[i3 + 2]) > threshold ? (void)(sum++), 255 : 0;
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			uint8_t *source_pixels = (uint8_t *)data;
			threshold *= 3;
			for (int i = 0; i < size; i++) {
				int i4 = 4 * i;
				target_pixels[i] = (source_pixels[i4] + source_pixels[i4 + 1] + source_pixels[i4 + 2]) > threshold ? (void)(sum++), 255 : 0;
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			uint8_t *source_pixels = (uint8_t *)data;
			threshold *= 3;
			for (int i = 0; i < size; i++) {
				int i4 = 4 * i;
				target_pixels[i] = (source_pixels[i4 + 1] + source_pixels[i4 + 2] + source_pixels[i4 + 3]) > threshold ? (void)(sum++), 255 : 0;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			uint16_t *source_pixels = (uint16_t *)data;
			threshold *= 3;
			for (int i = 0; i < size; i++) {
				int i3 = 3 * i;
				target_pixels[i] = (source_pixels[i3] + source_pixels[i3 + 1] + source_pixels[i3 + 2]) > threshold ? (void)(sum++), 255 : 0;
			}
			break;
		}
	}
	if (sum > size * 0.1) {
		indigo_error("Too many (%d%%) bright pixels", (int)((100 * sum) / size));
		indigo_safe_free(target_pixels);
		return NULL;
	}
	return target_pixels;
}

// expects INDIGO_RAW_MONO8 data

void indigo_skeletonize(uint8_t* data, int width, int height) {
	uint8_t (*pixels)[width] = (uint8_t (*)[width]) data;
	uint8_t (*temp)[width] = (uint8_t (*)[width]) (uint8_t*)malloc(width * height);
	memcpy(temp, pixels, width * height);
	int change = 1;
	while (change) {
		change = 0;
		for (int y = 1; y < height - 1; y++) {
			for (int x = 1; x < width - 1; x++) {
				if (pixels[y][x] == 255) {
					int neighbors = 0;
					for (int i = -1; i <= 1; i++) {
						for (int j = -1; j <= 1; j++) {
							if (!(i == 0 && j == 0) && pixels[y + i][x + j] == 255) {
								neighbors++;
							}
						}
					}
					if (neighbors >= 4 && neighbors <= 5) {
						temp[y][x] = 0;
						change = 1;
					}
				}
			}
		}
		memcpy(pixels, temp, width * height);
	}
	free(temp);
}

#define RHO_RES			3000
#define THETA_RES 	3000
#define MAX_LINES 	15

static void hough_transform(uint8_t* data, int width, int height, int *hough) {
	uint8_t (*pixels)[width] = (uint8_t (*)[width]) data;
	int (*acc)[THETA_RES] = (int (*)[THETA_RES]) hough;
	double theta_step = M_PI / THETA_RES;
	double rho_step = 2 * sqrt(width * width + height * height) / RHO_RES;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (pixels[y][x] > 0) {
				for (int theta_index = 0; theta_index < THETA_RES; theta_index++) {
					double theta = theta_index * theta_step;
					double rho = x * cos(theta) + y * sin(theta);
					int rho_index = (int)(rho / rho_step) + RHO_RES / 2;
					acc[rho_index][theta_index]++;
				}
			}
		}
	}
}

static int find_hough_max(int *hough, int width, int height, double *rho, double *theta) {
	int (*acc)[THETA_RES] = (int (*)[THETA_RES]) hough;
	int max = 0;
	int max_rho_index = 0;
	int max_theta_index = 0;
	double theta_step = M_PI / THETA_RES;
	double rho_step = 2 * sqrt(width * width + height * height) / RHO_RES;
	for (int theta_index = 0; theta_index < THETA_RES; theta_index++) {
		for (int rho_index = 0; rho_index < RHO_RES; rho_index++) {
			int count = acc[rho_index][theta_index];
			if (count > max) {
				max = count;
				max_rho_index = rho_index;
				max_theta_index = theta_index;
			}
		}
	}
	int DIFF = M_PI / 180 / theta_step;
	for (int diff = -DIFF; diff < DIFF; diff++) {
		int theta_index = max_theta_index + diff;
		if (theta_index < 0) {
			continue;
		}
		if (theta_index >= THETA_RES) {
			break;
		}
		for (int rho_index = 0; rho_index < RHO_RES; rho_index++) {
			acc[rho_index][theta_index] = 0;
		}
	}
	*rho = (max_rho_index - RHO_RES / 2) * rho_step;
	*theta = max_theta_index * theta_step;
	return max;
}

static void hough_to_cartesian(double rho, double theta, double* m, double* b) {
	*m = -1 / tan(theta);
	*b = rho / sin(theta);
}

static void line_intersection(double m1, double b1, double m2, double b2, double* x, double* y) {
	*x = (b2 - b1) / (m1 - m2);
	*y = m1 * (*x) + b1;
}

static double focus_error(const int width, const int height, double rho1, double theta1, double rho2, double theta2, double rho3, double theta3) {
	double m1, b1, m2, b2, m3, b3;
	int w10 = width / 10, w90 = width - w10;
	int h10 = height / 10, h90 = height - h10;
	hough_to_cartesian(rho1, theta1, &m1, &b1);
	hough_to_cartesian(rho2, theta2, &m2, &b2);
	hough_to_cartesian(rho3, theta3, &m3, &b3);
	double x12, y12, x23, y23;
	line_intersection(m1, b1, m2, b2, &x12, &y12);
	line_intersection(m2, b2, m3, b3, &x23, &y23);
	double x_m = (x12 + x23) / 2;
	double y_m = (y12 + y23) / 2;
	if (x_m < w10 || x_m > w90 || y_m < h10 || y_m > h90)
		return INFINITY;
	double x2, y2;
	line_intersection(m1, b1, m3, b3, &x2, &y2);
	return sqrt((x2 - x_m) * (x2 - x_m) + (y2 - y_m) * (y2 - y_m));
}

double indigo_bahtinov_error(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma, double *rho1, double *theta1, double *rho2, double *theta2, double *rho3, double *theta3) {
	uint8_t *mono = indigo_binarize(raw_type, data, width, height, sigma);
	if (mono == NULL) {
		return -1;
	}
	indigo_skeletonize(mono, width, height);
	int *hough = indigo_safe_malloc(RHO_RES * THETA_RES * sizeof(int));
	hough_transform(mono, width, height, hough);
	double rhos[MAX_LINES] = { 0.0 };
	double thetas[MAX_LINES] = { 0.0 };
	for (int line = 0; line < MAX_LINES; line++) {
		int max = find_hough_max(hough, width, height, rhos + line, thetas + line);
		indigo_debug("%s: %3d. %9.3f %9.3f -> %4d", __FUNCTION__, line, rhos[line], thetas[line] / M_PI * 180, max);
	}
	int line1 = -1, line2 = -1, line3 = -1;
	for (int index1 = 0; index1 < MAX_LINES && line1 == -1; index1++) {
		double angle1 = thetas[index1] / M_PI * 180;
		for (int index2 = 0; index2 < MAX_LINES && line2 == -1; index2++) {
			if (index2 == index1) {
				continue;
			}
			double angle2 = thetas[index2] / M_PI * 180;
			if (fabs(angle2 - angle1) > 90) {
				angle2 += (angle2 > 90 ? -180 : 180);
			}
			for (int index3 = 0; index3 < MAX_LINES && line3 == -1; index3++) {
				if (index3 == index1 || index3 == index2) {
					continue;
				}
				double angle3 = thetas[index3] / M_PI * 180;
				if (fabs(angle3 - angle1) > 90) {
					angle3 += (angle3 > 90 ? -180 : 180);
				}
				if (fabs(fabs(angle1 - angle2) - fabs(angle1 - angle3)) > 1) {
					continue;
				}
				if (fabs(angle1 - angle2) < 15 || fabs(angle1 - angle3) < 15 || fabs(angle2 - angle3) < 30) {
					continue;
				}
				if (fabs(angle1 - angle2) > 40 || fabs(angle1 - angle3) > 40 || fabs(angle2 - angle3) > 80) {
					continue;
				}
				if (!(angle2 < angle1 && angle1 < angle3)) {
					continue;
				}
				line1 = index1;
				line2 = index2;
				line3 = index3;
			}
		}
	}
	indigo_safe_free(hough);
	if (line1 != -1) {
		indigo_debug("%s: selected spikes:", __FUNCTION__);
		indigo_debug("%s: %3d. %9.3f %9.3f", __FUNCTION__, line1, rhos[line1], thetas[line1] / M_PI * 180);
		indigo_debug("%s: %3d. %9.3f %9.3f", __FUNCTION__, line2, rhos[line2], thetas[line2] / M_PI * 180);
		indigo_debug("%s: %3d. %9.3f %9.3f", __FUNCTION__, line3, rhos[line3], thetas[line3] / M_PI * 180);
		double error_px = focus_error(width, height, rhos[line1], thetas[line1], rhos[line2], thetas[line2], rhos[line3], thetas[line3]);
		indigo_debug("%s: focus error = %.2fpx", __FUNCTION__, error_px);
		if (error_px < 100) {
			*rho1 = rhos[line1];
			*rho2 = rhos[line2];
			*rho3 = rhos[line3];
			*theta1 = thetas[line1];
			*theta2 = thetas[line2];
			*theta3 = thetas[line3];
			return error_px;
		}
	}
	*rho1 = 0;
	*rho2 = 0;
	*rho3 = 0;
	*theta1 = 0;
	*theta2 = 0;
	*theta3 = 0;
	return -1;
}
