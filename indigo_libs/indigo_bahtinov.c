// Copyright (c) 2019-2026 CloudMakers, s. r. o.
// Copyright (c) 2026 Rumen Bogdanovski
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
// 3.0 by Rumen Bogdanovski <rumenastro@gmail.com>: Bahtinov mask analysis moved from indigo_raw_utils.c and rewritten

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_bahtinov.h>

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
	int64_t sum = 0;
	int64_t sum_sq = 0;
	switch (raw_type) {
		case INDIGO_RAW_MONO8: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int value = source_pixels[i];
				sum += value;
				sum_sq += (int64_t)value * value;
			}
			break;
		}
		case INDIGO_RAW_MONO16: {
			uint16_t *source_pixels = (uint16_t *)data;
			for (int i = 0; i < size; i++) {
				int value = source_pixels[i];
				sum += value;
				sum_sq += (int64_t)value * value;
			}
			break;
		}
		case INDIGO_RAW_RGB24: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int i3 = 3 * i;
				int value = (source_pixels[i3] + source_pixels[i3 + 1] + source_pixels[i3 + 2]) / 3;
				sum += value;
				sum_sq += (int64_t)value * value;
			}
			break;
		}
		case INDIGO_RAW_RGBA32: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int i4 = 4 * i;
				int value = (source_pixels[i4] + source_pixels[i4 + 1] + source_pixels[i4 + 2]) / 3;
				sum += value;
				sum_sq += (int64_t)value * value;
			}
			break;
		}
		case INDIGO_RAW_ABGR32: {
			uint8_t *source_pixels = (uint8_t *)data;
			for (int i = 0; i < size; i++) {
				int i4 = 4 * i;
				int value = (source_pixels[i4 + 1] + source_pixels[i4 + 2] + source_pixels[i4 + 3]) / 3;
				sum += value;
				sum_sq += (int64_t)value * value;
			}
			break;
		}
		case INDIGO_RAW_RGB48: {
			uint16_t *source_pixels = (uint16_t *)data;
			for (int i = 0; i < size; i++) {
				int i3 = 3 * i;
				int value = (source_pixels[i3] + source_pixels[i3 + 1] + source_pixels[i3 + 2]) / 3;
				sum += value;
				sum_sq += (int64_t)value * value;
			}
			break;
		}
	}
	double mean = (double)sum / size;
	double stddev = sqrt((double)sum_sq / size - mean * mean);
	int threshold = (int)(mean + (sigma * stddev));
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
	uint8_t *pixels =  data;
	uint8_t *temp = (uint8_t *)malloc(width * height);
	memcpy(temp, pixels, width * height);
	int change = 1;
	while (change) {
		change = 0;
		for (int y = 1; y < height - 1; y++) {
			for (int x = 1; x < width - 1; x++) {
				if (pixels[y * width + x] == 255) {
					int neighbors = 0;
					for (int i = -1; i <= 1; i++) {
						for (int j = -1; j <= 1; j++) {
							if (!(i == 0 && j == 0) && pixels[(y + i) * width + x + j] == 255) {
								neighbors++;
							}
						}
					}
					if (neighbors >= 4 && neighbors <= 5) {
						temp[y * width + x] = 0;
						change = 1;
					}
				}
			}
		}
		memcpy(pixels, temp, width * height);
	}
	free(temp);
}

// Bahtinov mask pattern analysis
//
// The star is located first, then the central spike is found as the strongest line through the star core. Line strength is
// the sum of samples along the line minus the mean of two parallel side bands, which cancels the smooth halo of the star.
// The central spike is then subtracted (its amplitude along the spike times its mean cross-section) and the outer spikes are
// searched with a matched filter whose weights are the radial profile of the central spike. All gratings of a Bahtinov mask
// have the same period, so in narrowband light the diffraction dots of all spikes are at the same distances from the star.
// The focus error is the signed distance of the outer spikes crossing from the central spike.

#define BAHTINOV_SAMPLE_STEP			0.5		// sample step along a line (px)
#define BAHTINOV_SIDE_BAND				3.0		// offset of the side bands (px)
#define BAHTINOV_EXCLUSION				1.5		// outer spike samples closer to the central spike are ignored in free search (px)
#define BAHTINOV_CUT_HALF_WIDTH		16		// half width of the central spike cross-section in sample steps
#define BAHTINOV_MIN_DELTA				4.0		// outer spike search range relative to the central spike (deg)
#define BAHTINOV_MAX_DELTA				40.0
#define BAHTINOV_DELTA_STEP				0.25
#define BAHTINOV_SYMMETRY					1.5		// max difference of the left and right outer spike angles (deg)
#define BAHTINOV_COMPETITOR				3.0		// min distance of a competing outer spike pair (deg)
#define BAHTINOV_MIN_CONFIDENCE		1.2		// min ratio of the best and the best competing pair in free search
#define BAHTINOV_KNOWN_WINDOW			0.5		// search window around a known mask angle (deg)
#define BAHTINOV_KNOWN_STEP				0.05
#define BAHTINOV_KNOWN_MIN_SNR		2.75	// min outer spike SNR with known mask angle
#define BAHTINOV_MIN_CENTRAL_SNR	8.0		// min central spike SNR
#define BAHTINOV_MAX_CROSSING			0.5		// max distance of the outer spikes crossing from the star centre (core radii)
#define BAHTINOV_NOISE_LINES			400		// number of random lines used to estimate the score noise

typedef struct {
	const float *v;
	int width, height;
	double cx, cy;
	int count, half;
	double *t;
	double *weights;
	bool exclude;
	double ex_theta, ex_d, ex_width;
} bahtinov_context;

// v is a 2x2 box filtered image, pixel (x, y) is centred at (x + 0.5, y + 0.5)
static inline double bahtinov_sample(const bahtinov_context *ctx, double x, double y) {
	x -= 0.5;
	y -= 0.5;
	if (x < 0) {
		x = 0;
	} else if (x > ctx->width - 1.001) {
		x = ctx->width - 1.001;
	}
	if (y < 0) {
		y = 0;
	} else if (y > ctx->height - 1.001) {
		y = ctx->height - 1.001;
	}
	int x0 = (int)x, y0 = (int)y;
	double fx = x - x0, fy = y - y0;
	const float *p = ctx->v + y0 * ctx->width + x0;
	return p[0] * (1 - fx) * (1 - fy) + p[1] * fx * (1 - fy) + p[ctx->width] * (1 - fx) * fy + p[ctx->width + 1] * fx * fy;
}

// line through centre + d * n, direction u, theta is the angle from vertical (clockwise), u = (sin, -cos), n = (cos, sin),
// i.e. normal form x * cos(theta) + y * sin(theta) = cx * cos(theta) + cy * sin(theta) + d
static double bahtinov_line_score(const bahtinov_context *ctx, double theta, double d, double *profile) {
	double ux = sin(theta), uy = -cos(theta), nx = cos(theta), ny = sin(theta);
	double sx = BAHTINOV_SIDE_BAND * nx, sy = BAHTINOV_SIDE_BAND * ny;
	double ec = cos(ctx->ex_theta), es = sin(ctx->ex_theta);
	double sum = 0;
	for (int i = 0; i < ctx->count; i++) {
		double x = ctx->cx + d * nx + ctx->t[i] * ux, y = ctx->cy + d * ny + ctx->t[i] * uy;
		double value = 0;
		if (!ctx->exclude || fabs((x - ctx->cx) * ec + (y - ctx->cy) * es - ctx->ex_d) > ctx->ex_width) {
			value = bahtinov_sample(ctx, x, y) - 0.5 * (bahtinov_sample(ctx, x + sx, y + sy) + bahtinov_sample(ctx, x - sx, y - sy));
		}
		if (profile) {
			profile[i] = value;
		}
		sum += ctx->weights ? ctx->weights[i] * value : value;
	}
	return sum;
}

static double bahtinov_refine(const bahtinov_context *ctx, double *theta, double *d, double lo, double hi) {
	double dth = 0.5 * M_PI / 180, dd = 0.5, best = -INFINITY, best_theta = *theta, best_d = *d;
	for (int iteration = 0; iteration < 5; iteration++) {
		double t0 = best_theta, d0 = best_d;
		for (int i = -4; i <= 4; i++) {
			double th = t0 + i * dth / 2;
			if (th < lo || th > hi) {
				continue;
			}
			for (int j = -4; j <= 4; j++) {
				double s = bahtinov_line_score(ctx, th, d0 + j * dd / 2, NULL);
				if (s > best) {
					best = s;
					best_theta = th;
					best_d = d0 + j * dd / 2;
				}
			}
		}
		dth /= 2;
		dd /= 2;
	}
	*theta = best_theta;
	*d = best_d;
	return best;
}

static int bahtinov_compare(const void *a, const void *b) {
	float x = *(const float *)a, y = *(const float *)b;
	return x < y ? -1 : x > y;
}

static float bahtinov_median(float *values, int count) {
	qsort(values, count, sizeof(float), bahtinov_compare);
	return values[count / 2];
}

// subtract a model of the central spike (amplitude along the spike times its mean cross-section) from a copy of the image
static float *bahtinov_subtract_central(const bahtinov_context *ctx, double tc, double dc, double r_min, double r_max) {
	int size = ctx->width * ctx->height;
	double ux = sin(tc), uy = -cos(tc), nx = cos(tc), ny = sin(tc);
	double cut[2 * BAHTINOV_CUT_HALF_WIDTH + 1] = { 0 }, cut_weight = 0;
	double edge = BAHTINOV_CUT_HALF_WIDTH * BAHTINOV_SAMPLE_STEP;
	double *amplitude = indigo_safe_malloc(ctx->count * sizeof(double));
	for (int i = 0; i < ctx->count; i++) {
		double x = ctx->cx + dc * nx + ctx->t[i] * ux, y = ctx->cy + dc * ny + ctx->t[i] * uy;
		double local = 0.5 * (bahtinov_sample(ctx, x + edge * nx, y + edge * ny) + bahtinov_sample(ctx, x - edge * nx, y - edge * ny));
		amplitude[i] = bahtinov_sample(ctx, x, y) - local;
		if (amplitude[i] > 0) {
			for (int k = -BAHTINOV_CUT_HALF_WIDTH; k <= BAHTINOV_CUT_HALF_WIDTH; k++) {
				cut[k + BAHTINOV_CUT_HALF_WIDTH] += amplitude[i] * (bahtinov_sample(ctx, x + k * BAHTINOV_SAMPLE_STEP * nx, y + k * BAHTINOV_SAMPLE_STEP * ny) - local);
			}
			cut_weight += amplitude[i];
		}
	}
	double peak = cut[BAHTINOV_CUT_HALF_WIDTH];
	if (cut_weight <= 0 || peak <= 0) {
		indigo_safe_free(amplitude);
		return NULL;
	}
	for (int k = 0; k <= 2 * BAHTINOV_CUT_HALF_WIDTH; k++) {
		cut[k] = cut[k] > 0 ? cut[k] / peak : 0;
	}
	float *clean = indigo_safe_malloc(size * sizeof(float));
	memcpy(clean, ctx->v, size * sizeof(float));
	for (int y = 0; y < ctx->height; y++) {
		for (int x = 0; x < ctx->width; x++) {
			double px = x + 0.5 - ctx->cx, py = y + 0.5 - ctx->cy;
			double o = px * nx + py * ny - dc, t = px * ux + py * uy;
			if (fabs(o) > edge || fabs(t) < r_min || fabs(t) > r_max - BAHTINOV_SAMPLE_STEP) {
				continue;
			}
			int i = t < 0 ? (int)((t + r_max) / BAHTINOV_SAMPLE_STEP) : ctx->half + (int)((t - r_min) / BAHTINOV_SAMPLE_STEP);
			if (i < 0 || i >= ctx->count || amplitude[i] <= 0) {
				continue;
			}
			double fo = o / BAHTINOV_SAMPLE_STEP + BAHTINOV_CUT_HALF_WIDTH;
			int k = (int)fo;
			if (k < 0 || k >= 2 * BAHTINOV_CUT_HALF_WIDTH) {
				continue;
			}
			clean[y * ctx->width + x] -= (float)(amplitude[i] * (cut[k] + (fo - k) * (cut[k + 1] - cut[k])));
		}
	}
	indigo_safe_free(amplitude);
	return clean;
}

// angles of the outer spike pair: free search over all symmetric pairs or around a known mask angle
static bool bahtinov_outer_spikes(bahtinov_context *ctx, double tc, double core, const double *mask_angle, double *tl, double *dl, double *sl, double *tr, double *dr, double *sr, double *confidence) {
	bool known = mask_angle != NULL && mask_angle[0] != 0 && mask_angle[1] != 0;
	double dmin = BAHTINOV_MIN_DELTA, dmax = BAHTINOV_MAX_DELTA, step = BAHTINOV_DELTA_STEP;
	if (known) {
		dmin = fmin(fabs(mask_angle[0]), fabs(mask_angle[1])) - BAHTINOV_KNOWN_WINDOW;
		dmax = fmax(fabs(mask_angle[0]), fabs(mask_angle[1])) + BAHTINOV_KNOWN_WINDOW;
		step = BAHTINOV_KNOWN_STEP;
		if (dmin < 1) {
			dmin = 1;
		}
	}
	int angles = (int)((dmax - dmin) / step) + 1;
	double *score_l = indigo_safe_malloc(angles * sizeof(double)), *score_r = indigo_safe_malloc(angles * sizeof(double));
	double *offset_l = indigo_safe_malloc(angles * sizeof(double)), *offset_r = indigo_safe_malloc(angles * sizeof(double));
	for (int a = 0; a < angles; a++) {
		double delta = (dmin + a * step) * M_PI / 180;
		score_l[a] = score_r[a] = -INFINITY;
		if (known && fabs(dmin + a * step - fabs(mask_angle[0])) > BAHTINOV_KNOWN_WINDOW && fabs(dmin + a * step - fabs(mask_angle[1])) > BAHTINOV_KNOWN_WINDOW) {
			continue;
		}
		for (double d = -core; d <= core; d += BAHTINOV_SAMPLE_STEP) {
			double s = bahtinov_line_score(ctx, tc - delta, d, NULL);
			if (s > score_l[a]) {
				score_l[a] = s;
				offset_l[a] = d;
			}
			s = bahtinov_line_score(ctx, tc + delta, d, NULL);
			if (s > score_r[a]) {
				score_r[a] = s;
				offset_r[a] = d;
			}
		}
		if (known && fabs(dmin + a * step - fabs(mask_angle[0])) > BAHTINOV_KNOWN_WINDOW) {
			score_l[a] = -INFINITY;
		}
		if (known && fabs(dmin + a * step - fabs(mask_angle[1])) > BAHTINOV_KNOWN_WINDOW) {
			score_r[a] = -INFINITY;
		}
	}
	int best_l = -1, best_r = -1;
	double best = -INFINITY, second = -INFINITY;
	for (int a = 0; a < angles; a++) {
		for (int c = 0; c < angles; c++) {
			if (!known && abs(a - c) * step > BAHTINOV_SYMMETRY) {
				continue;
			}
			double s = score_l[a] + score_r[c];
			if (s > best) {
				best = s;
				best_l = a;
				best_r = c;
			}
		}
	}
	if (best_l >= 0 && !known) {
		for (int a = 0; a < angles; a++) {
			for (int c = 0; c < angles; c++) {
				if (abs(a - c) * step > BAHTINOV_SYMMETRY || fabs((a + c) / 2.0 - (best_l + best_r) / 2.0) * step < BAHTINOV_COMPETITOR) {
					continue;
				}
				if (score_l[a] + score_r[c] > second) {
					second = score_l[a] + score_r[c];
				}
			}
		}
	}
	*confidence = known ? 0 : (second > 0 ? best / second : 10);
	if (best_l >= 0 && isfinite(best)) {
		double l_lo = tc - dmax * M_PI / 180, l_hi = tc - dmin * M_PI / 180, r_lo = tc + dmin * M_PI / 180, r_hi = tc + dmax * M_PI / 180;
		if (known) {
			l_lo = tc - (fabs(mask_angle[0]) + BAHTINOV_KNOWN_WINDOW) * M_PI / 180;
			l_hi = tc - (fabs(mask_angle[0]) - BAHTINOV_KNOWN_WINDOW) * M_PI / 180;
			r_lo = tc + (fabs(mask_angle[1]) - BAHTINOV_KNOWN_WINDOW) * M_PI / 180;
			r_hi = tc + (fabs(mask_angle[1]) + BAHTINOV_KNOWN_WINDOW) * M_PI / 180;
		}
		*tl = tc - (dmin + best_l * step) * M_PI / 180;
		*dl = offset_l[best_l];
		*tr = tc + (dmin + best_r * step) * M_PI / 180;
		*dr = offset_r[best_r];
		*sl = bahtinov_refine(ctx, tl, dl, l_lo, l_hi);
		*sr = bahtinov_refine(ctx, tr, dr, r_lo, r_hi);
	}
	indigo_safe_free(score_l);
	indigo_safe_free(score_r);
	indigo_safe_free(offset_l);
	indigo_safe_free(offset_r);
	return best_l >= 0 && isfinite(best);
}

bool indigo_bahtinov_analyze(indigo_raw_type raw_type, const void *data, const int width, const int height, const double *mask_angle, indigo_bahtinov_result *result) {
	memset(result, 0, sizeof(indigo_bahtinov_result));
	if (data == NULL || width < 32 || height < 32) {
		return false;
	}
	int size = width * height;
	float *lum = indigo_safe_malloc(size * sizeof(float));
	for (int i = 0; i < size; i++) {
		switch (raw_type) {
			case INDIGO_RAW_MONO8:
				lum[i] = ((const uint8_t *)data)[i];
				break;
			case INDIGO_RAW_MONO16:
				lum[i] = ((const uint16_t *)data)[i];
				break;
			case INDIGO_RAW_RGB24:
				lum[i] = ((const uint8_t *)data)[3 * i] + ((const uint8_t *)data)[3 * i + 1] + ((const uint8_t *)data)[3 * i + 2];
				break;
			case INDIGO_RAW_RGBA32:
				lum[i] = ((const uint8_t *)data)[4 * i] + ((const uint8_t *)data)[4 * i + 1] + ((const uint8_t *)data)[4 * i + 2];
				break;
			case INDIGO_RAW_ABGR32:
				lum[i] = ((const uint8_t *)data)[4 * i + 1] + ((const uint8_t *)data)[4 * i + 2] + ((const uint8_t *)data)[4 * i + 3];
				break;
			case INDIGO_RAW_RGB48:
				lum[i] = ((const uint16_t *)data)[3 * i] + ((const uint16_t *)data)[3 * i + 1] + ((const uint16_t *)data)[3 * i + 2];
				break;
			default:
				indigo_safe_free(lum);
				return false;
		}
	}
	// 2x2 box filter removes the Bayer pattern of undebayered frames and is harmless for mono and colour frames
	float *v = indigo_safe_malloc(size * sizeof(float));
	for (int y = 0; y < height; y++) {
		int y1 = y + 1 < height ? y + 1 : y;
		for (int x = 0; x < width; x++) {
			int x1 = x + 1 < width ? x + 1 : x;
			v[y * width + x] = 0.25f * (lum[y * width + x] + lum[y * width + x1] + lum[y1 * width + x] + lum[y1 * width + x1]);
		}
	}
	// background and noise (median and MAD of a subsample)
	int step = size > 65536 ? size / 65536 : 1, n = 0;
	for (int i = 0; i < size; i += step) {
		lum[n++] = v[i];
	}
	float background = bahtinov_median(lum, n);
	for (int i = 0; i < n; i++) {
		lum[i] = fabsf(lum[i] - background);
	}
	float noise = 1.4826f * bahtinov_median(lum, n);
	if (noise <= 0) {
		noise = 1;
	}
	for (int i = 0; i < size; i++) {
		v[i] -= background;
	}
	// star centre: centroid of pixels above 30% of the peak of a 3x3 box filtered copy
	float max = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float sum = 0;
			int count = 0;
			for (int j = -1; j <= 1; j++) {
				for (int i = -1; i <= 1; i++) {
					if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) {
						sum += v[(y + j) * width + x + i];
						count++;
					}
				}
			}
			lum[y * width + x] = sum / count;
			if (lum[y * width + x] > max) {
				max = lum[y * width + x];
			}
		}
	}
	double sx = 0, sy = 0, sw = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float s = lum[y * width + x];
			if (s > 0.3f * max) {
				sx += x * s;
				sy += y * s;
				sw += s;
			}
		}
	}
	indigo_safe_free(lum);
	if (sw <= 0 || max < 5 * noise) {
		indigo_debug("%s: no star found", __FUNCTION__);
		indigo_safe_free(v);
		return false;
	}
	bahtinov_context ctx = { .v = v, .width = width, .height = height, .cx = sx / sw + 0.5, .cy = sy / sw + 0.5 };
	// core radius: first ring where the azimuthal median drops below the noise level
	int core = 3, max_r = (width < height ? width : height) / 2;
	float *ring = indigo_safe_malloc((8 * max_r + 64) * sizeof(float));
	for (int r = 2; r < max_r; r++) {
		int steps = (int)(2 * M_PI * r) + 8;
		for (int s = 0; s < steps; s++) {
			ring[s] = (float)bahtinov_sample(&ctx, ctx.cx + r * cos(2 * M_PI * s / steps), ctx.cy + r * sin(2 * M_PI * s / steps));
		}
		if (bahtinov_median(ring, steps) < noise) {
			core = r;
			break;
		}
	}
	indigo_safe_free(ring);
	double r_min = core, r_max = 8 * core + 20;
	double edge = fmin(fmin(ctx.cx, ctx.cy), fmin(width - ctx.cx, height - ctx.cy)) - 4;
	if (r_max > edge) {
		r_max = edge;
	}
	if (r_max < r_min + 10) {
		indigo_debug("%s: star too large or too close to the edge", __FUNCTION__);
		indigo_safe_free(v);
		return false;
	}
	ctx.half = (int)((r_max - r_min) / BAHTINOV_SAMPLE_STEP);
	ctx.count = 2 * ctx.half;
	ctx.t = indigo_safe_malloc(ctx.count * sizeof(double));
	for (int i = 0; i < ctx.half; i++) {
		ctx.t[i] = -r_max + i * BAHTINOV_SAMPLE_STEP;
		ctx.t[ctx.half + i] = r_min + i * BAHTINOV_SAMPLE_STEP;
	}
	indigo_debug("%s: star at (%.1f, %.1f), background %.1f, noise %.2f, core %d, sampled %.0f-%.0f", __FUNCTION__, ctx.cx, ctx.cy, background, noise, core, r_min, r_max);
	// central spike: the strongest line through the core
	double tc = 0, dc = 0, sc = -INFINITY;
	for (int a = -90; a < 90; a++) {
		for (int d = -core; d <= core; d++) {
			double s = bahtinov_line_score(&ctx, a * M_PI / 180, d, NULL);
			if (s > sc) {
				sc = s;
				tc = a * M_PI / 180;
				dc = d;
			}
		}
	}
	sc = bahtinov_refine(&ctx, &tc, &dc, -INFINITY, INFINITY);
	// matched filter weights for the outer spikes: smoothed radial profile of the central spike
	double *profile = indigo_safe_malloc(ctx.count * sizeof(double));
	double *weights = indigo_safe_malloc(ctx.count * sizeof(double));
	double norm = 0;
	bahtinov_line_score(&ctx, tc, dc, profile);
	for (int i = 0; i < ctx.count; i++) {
		double sum = 0;
		int count = 0;
		for (int j = i - 2; j <= i + 2; j++) {
			if (j >= 0 && j < ctx.count && (j < ctx.half) == (i < ctx.half)) {
				sum += profile[j];
				count++;
			}
		}
		weights[i] = sum > 0 ? sum / count : 0;
		norm += weights[i] * weights[i];
	}
	norm = norm > 0 ? sqrt(norm) : 1;
	for (int i = 0; i < ctx.count; i++) {
		weights[i] /= norm;
	}
	indigo_safe_free(profile);
	// score noise of the plain and the matched filter from lines far from the pattern
	float *scores = indigo_safe_malloc(2 * BAHTINOV_NOISE_LINES * sizeof(float));
	unsigned int seed = 12345;
	int count = 0;
	for (int i = 0; i < BAHTINOV_NOISE_LINES; i++) {
		seed = seed * 1103515245 + 12345;
		double th = ((seed >> 8) % 18000) / 100.0 - 90;
		seed = seed * 1103515245 + 12345;
		double d = ((seed >> 8) % 1000) / 1000.0 * 2 * core - core;
		double diff = fmod(fabs(th - tc * 180 / M_PI), 180);
		if ((diff > 90 ? 180 - diff : diff) < 50) {
			continue;
		}
		ctx.weights = NULL;
		scores[count] = (float)fabs(bahtinov_line_score(&ctx, th * M_PI / 180, d, NULL));
		ctx.weights = weights;
		scores[BAHTINOV_NOISE_LINES + count] = (float)fabs(bahtinov_line_score(&ctx, th * M_PI / 180, d, NULL));
		count++;
	}
	double plain_noise = count > 0 ? 1.4826 * bahtinov_median(scores, count) : 1;
	double matched_noise = count > 0 ? 1.4826 * bahtinov_median(scores + BAHTINOV_NOISE_LINES, count) : 1;
	indigo_safe_free(scores);
	// outer spikes on the image with the central spike subtracted
	bool known = mask_angle != NULL && mask_angle[0] != 0 && mask_angle[1] != 0;
	float *clean = bahtinov_subtract_central(&ctx, tc, dc, r_min, r_max);
	if (clean) {
		ctx.v = clean;
	}
	ctx.weights = weights;
	ctx.exclude = true;
	ctx.ex_theta = tc;
	ctx.ex_d = dc;
	ctx.ex_width = known ? 0 : BAHTINOV_EXCLUSION;
	double tl = 0, dl = 0, sl = 0, tr = 0, dr = 0, sr = 0, confidence = 0;
	bool found = bahtinov_outer_spikes(&ctx, tc, core, mask_angle, &tl, &dl, &sl, &tr, &dr, &sr, &confidence);
	indigo_safe_free(clean);
	indigo_safe_free(weights);
	indigo_safe_free(ctx.t);
	indigo_safe_free(v);
	if (!found || plain_noise <= 0 || matched_noise <= 0) {
		indigo_debug("%s: outer spikes not found", __FUNCTION__);
		return false;
	}
	// crossing of the outer spikes and its signed distance from the central spike
	double rc = ctx.cx * cos(tc) + ctx.cy * sin(tc) + dc;
	double rl = ctx.cx * cos(tl) + ctx.cy * sin(tl) + dl;
	double rr = ctx.cx * cos(tr) + ctx.cy * sin(tr) + dr;
	double det = cos(tl) * sin(tr) - sin(tl) * cos(tr);
	if (fabs(det) < 1e-9) {
		return false;
	}
	double px = (rl * sin(tr) - rr * sin(tl)) / det, py = (rr * cos(tl) - rl * cos(tr)) / det;
	double crossing = hypot(px - ctx.cx, py - ctx.cy);
	result->error = px * cos(tc) + py * sin(tc) - rc;
	result->snr[0] = sc / plain_noise;
	result->snr[1] = sl / matched_noise;
	result->snr[2] = sr / matched_noise;
	result->angle[0] = (tl - tc) * 180 / M_PI;
	result->angle[1] = (tr - tc) * 180 / M_PI;
	result->confidence = confidence;
	double rv[3] = { rc, rl, rr }, tv[3] = { tc, tl, tr };
	for (int i = 0; i < 3; i++) {
		double th = fmod(tv[i], 2 * M_PI), r = rv[i];
		if (th < 0) {
			th += 2 * M_PI;
		}
		if (th >= M_PI) {
			th -= M_PI;
			r = -r;
		}
		result->rho[i] = r;
		result->theta[i] = th;
	}
	indigo_debug("%s: central %.2f deg (SNR %.1f), outer %+.2f deg (SNR %.1f) %+.2f deg (SNR %.1f), confidence %.2f, crossing %.1fpx from centre, error %+.3fpx", __FUNCTION__, tc * 180 / M_PI, result->snr[0], result->angle[0], result->snr[1], result->angle[1], result->snr[2], confidence, crossing, result->error);
	if (result->snr[0] < BAHTINOV_MIN_CENTRAL_SNR) {
		indigo_debug("%s: central spike too weak", __FUNCTION__);
		return false;
	}
	if (known ? (result->snr[1] < BAHTINOV_KNOWN_MIN_SNR || result->snr[2] < BAHTINOV_KNOWN_MIN_SNR) : confidence < BAHTINOV_MIN_CONFIDENCE) {
		indigo_debug("%s: outer spikes not reliable", __FUNCTION__);
		return false;
	}
	if (crossing > BAHTINOV_MAX_CROSSING * core) {
		indigo_debug("%s: outer spikes do not cross at the star", __FUNCTION__);
		return false;
	}
	return true;
}

double indigo_bahtinov_error(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma, double *rho1, double *theta1, double *rho2, double *theta2, double *rho3, double *theta3) {
	indigo_bahtinov_result result;
	bool found = indigo_bahtinov_analyze(raw_type, data, width, height, NULL, &result);
	*rho1 = found ? result.rho[0] : 0;
	*theta1 = found ? result.theta[0] : 0;
	*rho2 = found ? result.rho[1] : 0;
	*theta2 = found ? result.theta[1] : 0;
	*rho3 = found ? result.rho[2] : 0;
	*theta3 = found ? result.theta[2] : 0;
	return found ? fabs(result.error) : -1;
}
