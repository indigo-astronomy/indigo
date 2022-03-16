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

static int median(int a, int b, int c) {
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

	indigo_debug("HFD : background = %2f, stddev = %.2f, threshold = %.2f, max = %.2f", background, stddev,  background + 2 * stddev, max);
	indigo_debug("FWHM: background = %2f, stddev = %.2f, threshold = %.2f, max = %.2f", background, stddev,  background + 6 * stddev, max);

	/* HFD works fine with gignal 2 * stddev */
	if (max < background + 2 * stddev) {
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
				value -= background;
					if (value > 0) {
					double dist = sqrt((x - i) * (x - i) + (y - j) * (y - j));
					prod += dist * value;
					total += value;
				}
			}
		}
		*hfd = 2 * prod / total;
	}

	/* FWHM is erratic with peak < 6*stddev */
	if (max < background + 6 * stddev) {
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
	while (converge_iterations--) {
		// indigo_debug("%s: X = %.3f, Y= %3f", __FUNCTION__, *x, *y);
		result = indigo_selection_frame_digest(raw_type, data, x, y, radius, width, height, digest);
	}
	return result;
}

indigo_result indigo_selection_frame_digest(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *digest) {
	const int xx = (int)round(*x);
	const int yy = (int)round(*y);

	if ((width <= 2 * radius + 1) || (height <= 2 * radius + 1))
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
					sum += value;
					if (value > max) max = value;
				}
			}
			break;
		}
	}

	/* Set threshold 20% above average value */
	double threshold = 1.20 * sum / ((2 * radius + 1) * (2 * radius + 1));

	INDIGO_DEBUG(indigo_debug("Selection: threshold = %.3f, max = %.3f", threshold, max));

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
	digest->algorithm = centroid;
	//INDIGO_DEBUG(indigo_log("indigo_selection_frame_digest: centroid = [%5.2f, %5.2f]", digest->centroid_x, digest->centroid_y));
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
	digest->algorithm = centroid;
	//INDIGO_DEBUG(indigo_debug("indigo_centroid_frame_digest: centroid = [%5.2f, %5.2f]", digest->centroid_x, digest->centroid_y));
	return INDIGO_OK;
}

#define BG_RADIUS	5

static double calibrate_re(double (*vector)[2], int size) {
	int first = BG_RADIUS + 1, last = size - BG_RADIUS - 1;
	double avg = 0;
	double mins[size];
	for (int i = first; i <= last; i++) {
		double min = vector[i - BG_RADIUS][RE];
		for (int j = -BG_RADIUS + 1; j <= BG_RADIUS; j++) {
			double value = vector[i + j][RE];
			if (value < min)
				min = value;
		}
		mins[i] = min;
	}
	for (int i = 0; i < first; i++)
		vector[i][RE] = 0;
	for (int i = last + 1; i < size; i++)
		vector[i][RE] = 0;
	avg = 0;
	int count = last - first + 1;
	for (int i = first; i <= last; i++) {
		double value = vector[i][RE] - mins[i];
		vector[i][RE] = value;
		avg += value;
	}
	avg /= count;
	double stddev = 0;
	for (int i = first; i <= last; i++) {
		double value = vector[i][RE] - avg;
		stddev += value * value;
	}
	stddev /= count;
	double threshold = avg + sqrt(stddev);
	double signal_ms = 0, noise_ms = 0;
	int signal_count = 0, noise_count = 0;
	for (int i = first; i <= last; i++) {
		double value = vector[i][RE];
		if (value > threshold) {
			signal_ms += value * value;
			signal_count++;
		} else {
			noise_ms += value * value;
			noise_count++;
		}
	}

	double snr = (signal_ms / signal_count) / (noise_ms / noise_count);
	//INDIGO_DEBUG(indigo_debug("calibrate_re: threshold = %g, S/N = %g", threshold, snr));
	return snr;
}

indigo_result indigo_donuts_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, const int edge_clipping, indigo_frame_digest *digest) {
	const int xx = edge_clipping;
	const int yy = edge_clipping;

	if (width <= 2 * xx)
		return INDIGO_FAILED;
	if (height <= 2 * yy)
		return INDIGO_FAILED;
	if ((data == NULL) || (digest == NULL))
		return INDIGO_FAILED;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;

	double sum = 0, max = 0;
	const int ce = width - xx, le = height - yy;
	const int cs = xx, ls = yy;
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

	int sub_width = width - 2 * xx;
	int sub_height = height - 2 * yy;
	/* Set threshold 15% above average value */
	double threshold = 1.15 * sum / (sub_width * sub_height);

	INDIGO_DEBUG(indigo_debug("Donuts: threshold = %.3f, max = %.3f, edge_clipping = %dpx", threshold, max, edge_clipping));

	/* If max is below the thresold no guiding is possible */
	if (max <= threshold) return INDIGO_GUIDE_ERROR;

	digest->width = next_power_2(sub_width);
	digest->height = next_power_2(sub_height);
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

					col_x[i-xx][RE] += value;
					col_y[y][RE] += value;
				}
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			for (int j = ls; j < le; j++) {
				int y = j - ls;
				for (int i = cs; i < ce; i++) {
					value = clear_hot_pixel_16(data16, i, j, width, height) - threshold;
					/* Set all values below the threshold to 0 */
					if (value < 0) value = 0;

					col_x[i-xx][RE] += value;
					col_y[y][RE] += value;
				}
			}
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

					col_x[i-xx][RE] += value;
					col_y[y][RE] += value;
				}
			}
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

					col_x[i-xx][RE] += value;
					col_y[y][RE] += value;
				}
			}
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

					col_x[i-xx][RE] += value;
					col_y[y][RE] += value;
				}
			}
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

					col_x[i-xx][RE] += value;
					col_y[y][RE] += value;
				}
			}
			break;
		}
	}

	switch (raw_type) {
		case INDIGO_RAW_MONO8:
		case INDIGO_RAW_MONO16: {
			digest->snr = (calibrate_re(col_x, sub_width) + calibrate_re(col_y, sub_height)) / 2;

			fft(digest->width, col_x, digest->fft_x);
			fft(digest->height, col_y, digest->fft_y);
			break;
		}
		default: {
			/* Remove hot from the digest */
			fcol_x[0][RE] = median(0, col_x[0][RE], col_x[1][RE]);
			for (int i = 1; i < sub_width-1; i++) {
				fcol_x[i][RE] = median(col_x[i - 1][RE], col_x[i][RE], col_x[i + 1][RE]);
			}
			fcol_x[sub_width - 1][RE] = median(col_x[sub_width - 2][RE], col_x[sub_width - 1][RE], 0);

			fcol_y[0][RE] = median(0, col_y[0][RE], col_y[1][RE]);
			for (int i = 1; i < sub_height-1; i++) {
				fcol_y[i][RE] = median(col_y[i - 1][RE], col_y[i][RE], col_y[i + 1][RE]);
			}
			fcol_y[sub_height - 1][RE] = median(col_y[sub_height - 2][RE], col_y[sub_height - 1][RE], 0);

			digest->snr = (calibrate_re(fcol_x, sub_width) + calibrate_re(fcol_y, sub_height)) / 2;

			fft(digest->width, fcol_x, digest->fft_x);
			fft(digest->height, fcol_y, digest->fft_y);
		}
	}

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
						median(data8[off - 1], data8[off], data8[off + 1]) > threshold
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
						median(data16[off - 1], data16[off], data16[off + 1]) > threshold
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
							median(data8[off - 3], data8[off], data8[off + 3]) > threshold ||       /* Red Saturated? */
							median(data8[off - 2], data8[off + 1], data8[off + 4]) > threshold ||   /* Green Saturated? */
							median(data8[off - 1], data8[off + 2], data8[off + 5]) > threshold      /* Blue Saturated? */
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
							median(data16[off - 3], data16[off], data16[off + 3]) > threshold ||       /* Red Saturated? */
							median(data16[off - 2], data16[off + 1], data16[off + 4]) > threshold ||   /* Green Saturated? */
							median(data16[off - 1], data16[off + 2], data16[off + 5]) > threshold      /* Blue Saturated? */
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
			if (set[i] > SATURATION_8 && median(set[i - 1], set[i], set[i + 1]) > threshold) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median(set[i - 1], set[i], set[i + 1]), m));
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
				if (set[i] > SATURATION_8 && median(set[i - 1], set[i], set[i + 1]) > threshold) {
					if (saturated) {
						if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median(set[i - 1], set[i], set[i + 1]), m));
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
						median(set[i - 3], set[i], set[i + 3]) > threshold ||
						median(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
						median(set[i - 1], set[i + 2], set[i + 5]) > threshold
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
					median(set[i - 3], set[i], set[i + 3]) > threshold ||
					median(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
					median(set[i - 1], set[i + 2], set[i + 5]) > threshold
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
			if (set[i] > SATURATION_16 && median(set[i - 1], set[i], set[i + 1]) > threshold) {
				if (saturated) {
					if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median(set[i - 1], set[i], set[i + 1]), m));
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
				if (set[i] > SATURATION_16 && median(set[i - 1], set[i], set[i + 1]) > threshold) {
					if (saturated) {
						if (!(*saturated)) INDIGO_DEBUG(indigo_debug("Saturation detected: threshold = %.2f, median = %d, mean = %.2f", threshold, median(set[i - 1], set[i], set[i + 1]), m));
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
						median(set[i - 3], set[i], set[i + 3]) > threshold ||
						median(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
						median(set[i - 1], set[i + 2], set[i + 5]) > threshold
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
					median(set[i - 3], set[i], set[i + 3]) > threshold ||
					median(set[i - 2], set[i + 1], set[i + 4]) > threshold ||
					median(set[i - 1], set[i + 2], set[i + 5]) > threshold
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
	double drifts[MAX_MULTISTAR_COUNT] = {0};
	double drifts_x[MAX_MULTISTAR_COUNT] = {0};
	double drifts_y[MAX_MULTISTAR_COUNT] = {0};
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
	const int clip_edge   = height >= FIND_STAR_EDGE_CLIPPING * 4 ? FIND_STAR_EDGE_CLIPPING : (height / 4);
	int clip_width  = width - clip_edge;
	int clip_height = height - clip_edge;
	uint16_t max_luminance = 0;

	uint8_t *data8 = (uint8_t *)data;
	uint16_t *data16 = (uint16_t *)data;
	uint32_t threshold = 0;

	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			max_luminance = 0xFF;
			for (int i = 0; i < size; i++) {
				buf[i] = data8[i];
				threshold += buf[i];
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			max_luminance = 0xFFFF;
			for (int i = 0; i < size; i++) {
				buf[i] = data16[i];
				threshold += buf[i];
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			max_luminance = 0xFF;
			for (int i = 0, j = 0; i < 3 * size; i++, j++) {
				buf[j] = (data8[i] + data8[i + 1] + data8[i + 2]) / 3;
				threshold += buf[j];
				i += 2;
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			max_luminance = 0xFF;
			for (int i = 0, j = 0; i < 4 * size; i++, j++) {
				buf[j] = (data8[i] + data8[i + 1] + data8[i + 2]) / 3;
				threshold += buf[j];
				i += 3;
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			max_luminance = 0xFF;
			for (int i = 0, j = 0; i < 4 * size; i++, j++) {
				buf[j] = (data8[i + 1] + data8[i + 2] + data8[i + 3]) / 3;
				threshold += buf[j];
				i += 3;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			max_luminance = 0xFFFF;
			for (int i = 0, j = 0; i < 3 * size; i++, j++) {
				buf[j] = (data16[i] + data16[i + 1] + data16[i + 2]) / 3;
				threshold += buf[j];
				i += 2;
			}
			break;
		}
	}

	/* Look for stars 35% brighter than the frame average */
	threshold = 1.35 * threshold / size;
	int threshold_hist = threshold * 0.99;

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
				    median(buf[off - 1], buf[off], buf[off + 1]) > threshold &&
				    median(buf[off - width], buf[off], buf[off + width]) > threshold
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
						luminance += buf[off] - threshold;
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
						luminance += buf[off] - threshold;
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
						luminance += buf[off] - threshold;
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
						luminance += buf[off] - threshold;
						buf[off] = 0;
					} else {
						break;
					}
				}
			}

			indigo_result res = INDIGO_FAILED;
			if (radius >= 3) {
				indigo_frame_digest center;
				res = indigo_selection_frame_digest_iterative(raw_type, data, &star.x, &star.y, radius, width, height, &center, 2);
				star.x = center.centroid_x;
				star.y = center.centroid_y;
				indigo_delete_frame_digest(&center);
			}

			if (res == INDIGO_OK || radius < 3) {
				star.oversaturated = lmax == max_luminance;
				star.nc_distance = sqrt((star.x - width2) * (star.x - width2) + (star.y - height2) * (star.y - height2));
				star.nc_distance /= divider;
				star.luminance = log(fabs(luminance));
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
			indigo_debug("indigo_find_stars: star #%u: x = %lf, y = %lf, ncdist = %lf, lum = %lf", i+1, star_list[i].x, star_list[i].y, star_list[i].nc_distance, star_list[i].luminance);
		}
	)

	*stars_found = found;
	return INDIGO_OK;
}

indigo_result indigo_find_stars(indigo_raw_type raw_type, const void *data, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found) {
	return indigo_find_stars_precise(raw_type, data, 0, width, height, stars_max, star_list, stars_found);
}
