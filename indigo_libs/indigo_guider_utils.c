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

#include <indigo/indigo_bus.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_utils.h>

#define RE (0)
#define IM (1)
#define PI_2 (6.2831853071795864769252867665590057683943L)

static void _fft(const int n, const int offset, const int delta, const double (*x)[2], double (*X)[2], double (*_X)[2]);

static void fft(const int n, const double (*x)[2], double (*X)[2]) {
	double (*_X)[2] = malloc(2 * n * sizeof(double));
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
	double (*C)[2] = malloc(2 * n * sizeof(double));
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
//	INDIGO_DEBUG(indigo_debug("max_subp = %5.2f max: %d -> %5.2f %5.2f %5.2f\n", max_subp, max, c[prev][0], c[max][0], c[next][0]));
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
	static int d1[][2] = { { 0, 0 }, { -1, 0 }, { 0, -1 }, { 0, 1 }, { 1, 0 }, { -1, -1 }, { -1, 1 }, { 1, -1 }, { 1, 1 }, { -2, 0 }, { 0, -2 }, { 0, 2 }, { 2, 0 }, { -2, -1 }, { -2, 1 }, { -1, -2 }, { -1, 2 }, { 1, -2 }, { 1, 2 }, { 2, -1 }, { 2, 1 }, { -2, -2 }, { -2, 2 }, { 2, -2 }, { 2, 2 }, { -3, 0 }, { 0, -3 }, { 0, 3 }, { 3, 0 }, { -3, -1 }, { -3, 1 }, { -1, -3 }, { -1, 3 }, { 1, -3 }, { 1, 3 }, { 3, -1 }, { 3, 1 }, { -3, -2 }, { -3, 2 }, { -2, -3 }, { -2, 3 }, { 2, -3 }, { 2, 3 }, { 3, -2 }, { 3, 2 }, { -4, 0 }, { 0, -4 }, { 0, 4 }, { 4, 0 }, { -4, -1 }, { -4, 1 }, { -1, -4 }, { -1, 4 }, { 1, -4 }, { 1, 4 }, { 4, -1 }, { 4, 1 }, { -3, -3 }, { -3, 3 }, { 3, -3 }, { 3, 3 }, { -4, -2 }, { -4, 2 }, { -2, -4 }, { -2, 4 }, { 2, -4 }, { 2, 4 }, { 4, -2 }, { 4, 2 }, { -5, 0 }, { -4, -3 }, { -4, 3 }, { -3, -4 }, { -3, 4 }, { 0, -5 }, { 0, 5 }, { 3, -4 }, { 3, 4 }, { 4, -3 }, { 4, 3 }, { 5, 0 }, { -5, -1 }, { -5, 1 }, { -1, -5 }, { -1, 5 }, { 1, -5 }, { 1, 5 }, { 5, -1 }, { 5, 1 }, { -5, -2 }, { -5, 2 }, { -2, -5 }, { -2, 5 }, { 2, -5 }, { 2, 5 }, { 5, -2 }, { 5, 2 }, { -4, -4 }, { -4, 4 }, { 4, -4 }, { 4, 4 }, { -5, -3 }, { -5, 3 }, { -3, -5 }, { -3, 5 }, { 3, -5 }, { 3, 5 }, { 5, -3 }, { 5, 3 }, { -6, 0 }, { 0, -6 }, { 0, 6 }, { 6, 0 }, { -6, -1 }, { -6, 1 }, { -1, -6 }, { -1, 6 }, { 1, -6 }, { 1, 6 }, { 6, -1 }, { 6, 1 }, { -6, -2 }, { -6, 2 }, { -2, -6 }, { -2, 6 }, { 2, -6 }, { 2, 6 }, { 6, -2 }, { 6, 2 }, { -5, -4 }, { -5, 4 }, { -4, -5 }, { -4, 5 }, { 4, -5 }, { 4, 5 }, { 5, -4 }, { 5, 4 }, { -6, -3 }, { -6, 3 }, { -3, -6 }, { -3, 6 }, { 3, -6 }, { 3, 6 }, { 6, -3 }, { 6, 3 }, { -7, 0 }, { 0, -7 }, { 0, 7 }, { 7, 0 }, { -7, -1 }, { -7, 1 }, { -5, -5 }, { -5, 5 }, { -1, -7 }, { -1, 7 }, { 1, -7 }, { 1, 7 }, { 5, -5 }, { 5, 5 }, { 7, -1 }, { 7, 1 }, { -6, -4 }, { -6, 4 }, { -4, -6 }, { -4, 6 }, { 4, -6 }, { 4, 6 }, { 6, -4 }, { 6, 4 }, { -7, -2 }, { -7, 2 }, { -2, -7 }, { -2, 7 }, { 2, -7 }, { 2, 7 }, { 7, -2 }, { 7, 2 }, { -7, -3 }, { -7, 3 }, { -3, -7 }, { -3, 7 }, { 3, -7 }, { 3, 7 }, { 7, -3 }, { 7, 3 }, { -6, -5 }, { -6, 5 }, { -5, -6 }, { -5, 6 }, { 5, -6 }, { 5, 6 }, { 6, -5 }, { 6, 5 }, { -8, 0 }, { 0, -8 }, { 0, 8 }, { 8, 0 }, { -8, -1 }, { -8, 1 }, { -7, -4 }, { -7, 4 }, { -4, -7 }, { -4, 7 }, { -1, -8 }, { -1, 8 }, { 1, -8 }, { 1, 8 }, { 4, -7 }, { 4, 7 }, { 7, -4 }, { 7, 4 }, { 8, -1 }, { 8, 1 }, { -8, -2 }, { -8, 2 }, { -2, -8 }, { -2, 8 }, { 2, -8 }, { 2, 8 }, { 8, -2 }, { 8, 2 }, { -6, -6 }, { -6, 6 }, { 6, -6 }, { 6, 6 }, { -8, -3 }, { -8, 3 }, { -3, -8 }, { -3, 8 }, { 3, -8 }, { 3, 8 }, { 8, -3 }, { 8, 3 }, { -7, -5 }, { -7, 5 }, { -5, -7 }, { -5, 7 }, { 5, -7 }, { 5, 7 }, { 7, -5 }, { 7, 5 }, { -8, -4 }, { -8, 4 }, { -4, -8 }, { -4, 8 }, { 4, -8 }, { 4, 8 }, { 8, -4 }, { 8, 4 }, { -9, 0 }, { 0, -9 }, { 0, 9 }, { 9, 0 }, { -9, -1 }, { -9, 1 }, { -1, -9 }, { -1, 9 }, { 1, -9 }, { 1, 9 }, { 9, -1 }, { 9, 1 }, { -9, -2 }, { -9, 2 }, { -7, -6 }, { -7, 6 }, { -6, -7 }, { -6, 7 }, { -2, -9 }, { -2, 9 }, { 2, -9 }, { 2, 9 }, { 6, -7 }, { 6, 7 }, { 7, -6 }, { 7, 6 }, { 9, -2 }, { 9, 2 }, { -8, -5 }, { -8, 5 }, { -5, -8 }, { -5, 8 }, { 5, -8 }, { 5, 8 }, { 8, -5 }, { 8, 5 }, { -9, -3 }, { -9, 3 }, { -3, -9 }, { -3, 9 }, { 3, -9 }, { 3, 9 }, { 9, -3 }, { 9, 3 }, { -9, -4 }, { -9, 4 }, { -4, -9 }, { -4, 9 }, { 4, -9 }, { 4, 9 }, { 9, -4 }, { 9, 4 }, { -7, -7 }, { -7, 7 }, { 7, -7 }, { 7, 7 }, { -10, 0 }, { -8, -6 }, { -8, 6 }, { -6, -8 }, { -6, 8 }, { 0, -10 }, { 0, 10 }, { 6, -8 }, { 6, 8 }, { 8, -6 }, { 8, 6 }, { 10, 0 }, { -10, -1 }, { -10, 1 }, { -1, -10 }, { -1, 10 }, { 1, -10 }, { 1, 10 }, { 10, -1 }, { 10, 1 }, { -10, -2 }, { -10, 2 }, { -2, -10 }, { -2, 10 }, { 2, -10 }, { 2, 10 }, { 10, -2 }, { 10, 2 }, { -9, -5 }, { -9, 5 }, { -5, -9 }, { -5, 9 }, { 5, -9 }, { 5, 9 }, { 9, -5 }, { 9, 5 }, { -10, -3 }, { -10, 3 }, { -3, -10 }, { -3, 10 }, { 3, -10 }, { 3, 10 }, { 10, -3 }, { 10, 3 }, { -8, -7 }, { -8, 7 }, { -7, -8 }, { -7, 8 }, { 7, -8 }, { 7, 8 }, { 8, -7 }, { 8, 7 }, { -10, -4 }, { -10, 4 }, { -4, -10 }, { -4, 10 }, { 4, -10 }, { 4, 10 }, { 10, -4 }, { 10, 4 }, { -9, -6 }, { -9, 6 }, { -6, -9 }, { -6, 9 }, { 6, -9 }, { 6, 9 }, { 9, -6 }, { 9, 6 }, { -10, -5 }, { -10, 5 }, { -5, -10 }, { -5, 10 }, { 5, -10 }, { 5, 10 }, { 10, -5 }, { 10, 5 }, { -8, -8 }, { -8, 8 }, { 8, -8 }, { 8, 8 }, { -9, -7 }, { -9, 7 }, { -7, -9 }, { -7, 9 }, { 7, -9 }, { 7, 9 }, { 9, -7 }, { 9, 7 }, { -10, -6 }, { -10, 6 }, { -6, -10 }, { -6, 10 }, { 6, -10 }, { 6, 10 }, { 10, -6 }, { 10, 6 }, { -9, -8 }, { -9, 8 }, { -8, -9 }, { -8, 9 }, { 8, -9 }, { 8, 9 }, { 9, -8 }, { 9, 8 }, { -10, -7 }, { -10, 7 }, { -7, -10 }, { -7, 10 }, { 7, -10 }, { 7, 10 }, { 10, -7 }, { 10, 7 }, { -9, -9 }, { -9, 9 }, { 9, -9 }, { 9, 9 }, { -10, -8 }, { -10, 8 }, { -8, -10 }, { -8, 10 }, { 8, -10 }, { 8, 10 }, { 10, -8 }, { 10, 8 }, { -10, -9 }, { -10, 9 }, { -9, -10 }, { -9, 10 }, { 9, -10 }, { 9, 10 }, { 10, -9 }, { 10, 9 }, { -10, -10 }, { -10, 10 }, { 10, -10 }, { 10, 10 } };
	static int d2[][2] = { { -1, 0 }, { 0, -1 }, { 0, 1 }, { 1, 0 } };
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
	double min = 1e20, max = 0, total = 0, value;
	int ce = xx + radius, le = yy + radius;
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
					value = ((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2];
					break;
				}
				case INDIGO_RAW_RGB48: {
					kk *= 3;
					value = ((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2];
					break;
				}
			}
			if (value > max)
				max = value;
			if (value < min)
				min = value;
			total += value;
		}
	}
	*peak = max - min;
	*hfd = *fwhm = 0;
	double half_flux = total / 2;
	total = 0;
	for (int k = 0; k < sizeof(d1); k++) {
		int i = d1[k][0];
		int j = d1[k][1];
		if (abs(i) > radius || abs(j) > radius)
			break;
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
			case INDIGO_RAW_RGB48: {
				kk *= 3;
				value = ((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2];
				break;
			}
		}
		total += value;
		if (total >= half_flux) {
			*hfd = 2 * sqrt(i * i + j * j);
			break;
		}
	}
	double half_max = *peak / 2 + min;
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
		tmp = 0;
	*fwhm = tmp;
	return INDIGO_OK;
}

indigo_result indigo_selection_frame_digest(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *c) {
	if ((width <= 2 * radius) || (height <= 2 * radius))
		return INDIGO_FAILED;
	int xx = (int)round(*x);
	int yy = (int)round(*y);
	if (xx < radius || width - radius < xx)
		return INDIGO_FAILED;
	if (yy < radius || height - radius < yy)
		return INDIGO_FAILED;
	if ((data == NULL) || (c == NULL))
		return INDIGO_FAILED;
	double m10 = 0, m01 = 0, m00 = 0, min = 1e20, max = 0;
	int ce = xx + radius, le = yy + radius;
	for (int j = yy - radius; j <= le; j++) {
		int k = j * width;
		for (int i = xx - radius; i <= ce; i++) {
			int kk = k + i;
			double value;
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
				case INDIGO_RAW_RGB48: {
					kk *= 3;
					value = ((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2];
					break;
				}
			}
			if (value > max)
				max = value;
			if (value < min)
				min = value;
		}
	}
	double scale = (max - min) / 10000;
	if (scale == 0)
		return INDIGO_FAILED;
	for (int j = yy - radius; j <= le; j++) {
		int k = j * width;
		for (int i = xx - radius; i <= ce; i++) {
			int kk = k + i;
			double value;
			switch (raw_type) {
				case INDIGO_RAW_MONO8: {
					value = (((uint8_t *)data)[kk] - min) / scale;
					break;
				}
				case INDIGO_RAW_MONO16: {
					value = (((uint16_t *)data)[kk] - min) / scale;
					break;
				}
				case INDIGO_RAW_RGB24: {
					kk *= 3;
					value = (((uint8_t *)data)[kk] + ((uint8_t *)data)[kk + 1] + ((uint8_t *)data)[kk + 2] - min) / scale;
					break;
				}
				case INDIGO_RAW_RGB48: {
					kk *= 3;
					value = (((uint16_t *)data)[kk] + ((uint16_t *)data)[kk + 1] + ((uint16_t *)data)[kk + 2] - min) / scale;
					break;
				}
			}
			m10 += i * value;
			m01 += j * value;
			m00 += value;
		}
	}
	c->width = width;
	c->height = height;
	c->centroid_x = *x = m10 / m00;
	c->centroid_y = *y = m01 / m00;
	c->algorithm = centroid;
	//INDIGO_DEBUG(indigo_log("indigo_selection_frame_digest: centroid = [%5.2f, %5.2f]", c->centroid_x, c->centroid_y));
	return INDIGO_OK;
}

indigo_result indigo_centroid_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *c) {
	if ((width < 3) || (height < 3))
		return INDIGO_FAILED;
	if ((data == NULL) || (c == NULL))
		return INDIGO_FAILED;
	double m10 = 0, m01 = 0, m00 = 0;
	int ci = 0, li = 0, max = width * height;
	for (int i = 0; i < max; i++) {
		double value;
		switch (raw_type) {
			case INDIGO_RAW_MONO8: {
				value = ((uint8_t *)data)[i];
				break;
			}
			case INDIGO_RAW_MONO16: {
				value = ((uint16_t *)data)[i];
				break;
			}
			case INDIGO_RAW_RGB24: {
				value = ((uint8_t *)data)[i] + ((uint8_t *)data)[i + 1] + ((uint8_t *)data)[i + 2];
				i += 2;
				break;
			}
			case INDIGO_RAW_RGB48: {
				value = ((uint16_t *)data)[i] + ((uint16_t *)data)[i + 1] + ((uint16_t *)data)[i + 2];
				i += 2;
				break;
			}
		}
		m10 += ci * value;
		m01 += li * value;
		m00 += value;
		ci++;
		if (ci == width) {
			ci = 0;
			li++;
		}
	}
	c->width = width;
	c->height = height;
	c->centroid_x = m10 / m00;
	c->centroid_y = m01 / m00;
	c->algorithm = centroid;
	//INDIGO_DEBUG(indigo_debug("indigo_centroid_frame_digest: centroid = [%5.2f, %5.2f]", c->centroid_x, c->centroid_y));
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

indigo_result indigo_donuts_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *c) {
	if ((width < 3) || (height < 3))
		return INDIGO_FAILED;
	if ((data == NULL) || (c == NULL))
		return INDIGO_FAILED;
	c->width = next_power_2(width);
	c->height = next_power_2(height);
	double (*col_x)[2] = calloc(2 * c->width * sizeof(double), 1);
	double (*col_y)[2] = calloc(2 * c->height * sizeof(double), 1);
	c->fft_x = malloc(2 * c->width * sizeof(double));
	c->fft_y = malloc(2 * c->height * sizeof(double));
	int ci = 0, li = 0, max = width * height;
	for (int i = 0; i < max; i++) {
		double value;
		switch (raw_type) {
			case INDIGO_RAW_MONO8: {
				value = ((uint8_t *)data)[i];
				break;
			}
			case INDIGO_RAW_MONO16: {
				value = ((uint16_t *)data)[i];
				break;
			}
			case INDIGO_RAW_RGB24: {
				value = ((uint8_t *)data)[i] + ((uint8_t *)data)[i + 1] + ((uint8_t *)data)[i + 2];
				break;
			}
			case INDIGO_RAW_RGB48: {
				value = ((uint16_t *)data)[i] + ((uint16_t *)data)[i + 1] + ((uint16_t *)data)[i + 2];
				break;
			}
		}
		col_x[ci][RE] += value;
		col_y[li][RE] += value;
		ci++;
		if (ci == width) {
			ci = 0;
			li++;
		}
	}
	c->snr = (calibrate_re(col_x, width) + calibrate_re(col_y, height)) / 2;
//	printf("col_x:");
//	for (i=0; i < c->width; i++) {
//		printf(" %5.2f\n",col_x[i][RE]);
//	}
//	printf("\n");
//	printf("col_y:");
//	for (i=0; i < fdigest->height; i++) {
//		printf(" %5.2f",col_y[i][RE]);
//	}
//	printf("\n");
	fft(c->width, col_x, c->fft_x);
	fft(c->height, col_y, c->fft_y);
	c->algorithm = donuts;
	free(col_x);
	free(col_y);
	return INDIGO_OK;
}

indigo_result indigo_calculate_drift(const indigo_frame_digest *ref, const indigo_frame_digest *new, double *drift_x, double *drift_y) {
	if (ref == NULL || new == NULL || drift_x == NULL || drift_y == NULL)
		return INDIGO_FAILED;
	if ((ref->width != new->width) || (ref->height != new->height))
		return INDIGO_FAILED;
	if (ref->algorithm == centroid) {
		*drift_x = new->centroid_x - ref->centroid_x;
		*drift_y = new->centroid_y - ref->centroid_y;
		return INDIGO_OK;
	}
	if (ref->algorithm == donuts) {
		double (*c_buf)[2];
		int max_dim = (ref->width > ref->height) ? ref->width : ref->height;
		c_buf = malloc(2 * max_dim * sizeof(double));
		/* find X correction */
		corellate_fft(ref->width, new->fft_x, ref->fft_x, c_buf);
		*drift_x = find_distance(ref->width, c_buf);
		/* find Y correction */
		corellate_fft(ref->height, new->fft_y, ref->fft_y, c_buf);
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
