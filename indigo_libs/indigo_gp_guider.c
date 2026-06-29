// Copyright (c) 2026 Rumen G. Bogdanovski
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

//  Predictive PEC (Gaussian Process) guiding algorithm.
//
//  Dependency-free pure-C implementation of a Gaussian Process guiding
//  algorithm, originally developed at the Max Planck Institute for
//  Intelligent Systems (Edgar D. Klenske, Stephan Wenninger, Raffi Enficiaud)
//  and distributed under a BSD license.
//
//  version history
//  3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_gp_guider.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- algorithm constants -------------------------- */

#define CIRCULAR_BUFFER_SIZE 8192 /* raw data storage */
#define REGULAR_BUFFER_SIZE 2048  /* regularized data storage */
#define FFT_SIZE 4096             /* zero-padding for the FFT (>= REGULAR_BUFFER_SIZE) */
#define GRID_INTERVAL 5.0
#define MAX_DITHER_STEPS 10
#define DEFAULT_LEARNING_RATE 0.01
#define HYSTERESIS 0.1
#define JITTER 1e-6

/* indices into the 7 natural hyperparameters */
enum {
	SE0K_LENGTH_SCALE = 0,
	SE0K_SIGNAL_VARIANCE,
	PK_LENGTH_SCALE,
	PK_SIGNAL_VARIANCE,
	SE1K_LENGTH_SCALE,
	SE1K_SIGNAL_VARIANCE,
	PK_PERIOD_LENGTH,
	NUM_HYPERPARAMETERS
};

/* Default hyperparameters and tuning */
#define DEFAULT_CONTROL_GAIN 0.6
#define DEFAULT_PREDICTION_GAIN 0.5
#define DEFAULT_MIN_MOVE 0.2
#define DEFAULT_PERIODS_FOR_INFERENCE 2.0
#define DEFAULT_PERIODS_FOR_PERIOD_ESTIMATION 2.0
#define DEFAULT_POINTS_FOR_APPROXIMATION 100
#define DEFAULT_COMPUTE_PERIOD true

#define DEFAULT_LS_SE0 700.0
#define DEFAULT_SV_SE0 20.0
#define DEFAULT_LS_PK 10.0
#define DEFAULT_PERIOD_PK 200.0
#define DEFAULT_SV_PK 20.0
#define DEFAULT_LS_SE1 25.0
#define DEFAULT_SV_SE1 10.0

/*  Minimal dense linear algebra                                       */
/*  Row-major double matrices, small sizes (n <~ 100), so plain        */
/*  O(n^3) routines are adequate.                                      */

/* C = A(ar x ac) * B(ac x bc), C must be ar x bc and distinct from A, B */
static void mat_mul(const double *A, const double *B, double *C, int ar, int ac, int bc) {
	for (int i = 0; i < ar; i++) {
		for (int j = 0; j < bc; j++) {
			double s = 0.0;
			for (int k = 0; k < ac; k++) {
				s += A[i * ac + k] * B[k * bc + j];
			}
			C[i * bc + j] = s;
		}
	}
}

/* C = A(ar x ac) * B^T(bc x ac -> ac x bc); B is given as (bc x ac) */
static void mat_mul_bt(const double *A, const double *B, double *C, int ar, int ac, int br) {
	for (int i = 0; i < ar; i++) {
		for (int j = 0; j < br; j++) {
			double s = 0.0;
			for (int k = 0; k < ac; k++) {
				s += A[i * ac + k] * B[j * ac + k];
			}
			C[i * br + j] = s;
		}
	}
}

/*
 * LDLT (square-root free Cholesky) factorisation of a symmetric matrix:
 *   A = L * D * L^T,  L unit lower-triangular, D diagonal.
 * L is stored full (only the strict lower part is meaningful, unit diagonal
 * implied), D as a vector. Returns false if a pivot is non-positive (matrix
 * not positive definite even after the jitter that callers add).
 */
typedef struct {
	int n;
	double *L; /* n x n */
	double *D; /* n */
} ldlt_t;

static bool ldlt_factor(ldlt_t *f, const double *A, int n) {
	f->n = n;
	f->L = (double *)calloc((size_t)n * n, sizeof(double));
	f->D = (double *)calloc((size_t)n, sizeof(double));
	if (!f->L || !f->D) {
		free(f->L);
		free(f->D);
		f->L = NULL;
		f->D = NULL;
		return false;
	}
	for (int j = 0; j < n; j++) {
		double d = A[j * n + j];
		for (int k = 0; k < j; k++) {
			d -= f->L[j * n + k] * f->L[j * n + k] * f->D[k];
		}
		f->D[j] = d;
		f->L[j * n + j] = 1.0;
		if (d == 0.0) {
			d = 1e-300; /* avoid division by zero; near-singular handled by callers' jitter */
		}
		for (int i = j + 1; i < n; i++) {
			double s = A[i * n + j];
			for (int k = 0; k < j; k++) {
				s -= f->L[i * n + k] * f->L[j * n + k] * f->D[k];
			}
			f->L[i * n + j] = s / d;
		}
	}
	return true;
}

static void ldlt_free(ldlt_t *f) {
	free(f->L);
	free(f->D);
	f->L = NULL;
	f->D = NULL;
}

/*
 * Solve A * X = B for X, where A = L D L^T was factorised by ldlt_factor.
 * B and X are n x m (m right-hand sides), X may alias B.
 */
static void ldlt_solve(const ldlt_t *f, const double *B, double *X, int m) {
	int n = f->n;
	double *Y = (double *)malloc((size_t)n * m * sizeof(double));
	if (!Y) {
		return;
	}
	/* forward substitution: L Y = B */
	for (int i = 0; i < n; i++) {
		for (int c = 0; c < m; c++) {
			double s = B[i * m + c];
			for (int k = 0; k < i; k++) {
				s -= f->L[i * n + k] * Y[k * m + c];
			}
			Y[i * m + c] = s;
		}
	}
	/* diagonal: Z = D^-1 Y (store back into Y) */
	for (int i = 0; i < n; i++) {
		for (int c = 0; c < m; c++) {
			Y[i * m + c] /= f->D[i];
		}
	}
	/* back substitution: L^T X = Z */
	for (int i = n - 1; i >= 0; i--) {
		for (int c = 0; c < m; c++) {
			double s = Y[i * m + c];
			for (int k = i + 1; k < n; k++) {
				s -= f->L[k * n + i] * X[k * m + c];
			}
			X[i * m + c] = s;
		}
	}
	free(Y);
}

/*  Math tools: FFT, spectrum, Hamming window */

static bool is_nan(double x) { return x != x; }

/* In-place iterative radix-2 Cooley-Tukey FFT. n must be a power of two. */
static void fft_radix2(double *re, double *im, int n) {
	/* bit-reversal permutation */
	for (int i = 1, j = 0; i < n; i++) {
		int bit = n >> 1;
		for (; j & bit; bit >>= 1) {
			j ^= bit;
		}
		j ^= bit;
		if (i < j) {
			double tr = re[i]; re[i] = re[j]; re[j] = tr;
			double ti = im[i]; im[i] = im[j]; im[j] = ti;
		}
	}
	for (int len = 2; len <= n; len <<= 1) {
		double ang = -2.0 * M_PI / len;
		double wlr = cos(ang), wli = sin(ang);
		for (int i = 0; i < n; i += len) {
			double wr = 1.0, wi = 0.0;
			for (int k = 0; k < len / 2; k++) {
				double ur = re[i + k], ui = im[i + k];
				double vr = re[i + k + len / 2] * wr - im[i + k + len / 2] * wi;
				double vi = re[i + k + len / 2] * wi + im[i + k + len / 2] * wr;
				re[i + k] = ur + vr;
				im[i + k] = ui + vi;
				re[i + k + len / 2] = ur - vr;
				im[i + k + len / 2] = ui - vi;
				double nwr = wr * wlr - wi * wli;
				wi = wr * wli + wi * wlr;
				wr = nwr;
			}
		}
	}
}

static int next_pow2(int n) {
	int p = 1;
	while (p < n) {
		p <<= 1;
	}
	return p;
}

/*
 * Power spectrum of a real signal, mirroring math_tools::compute_spectrum.
 * Allocates and returns spectrum[] and frequencies[] (caller frees), sets
 * *out_len. Returns false on allocation failure.
 */
static bool compute_spectrum(const double *data, int n_data, int N, double **spectrum, double **frequencies, int *out_len) {
	if (N < n_data) {
		N = n_data;
	}
	N = next_pow2(N);
	double *re = (double *)calloc((size_t)N, sizeof(double));
	double *im = (double *)calloc((size_t)N, sizeof(double));
	if (!re || !im) {
		free(re);
		free(im);
		return false;
	}
	for (int i = 0; i < n_data; i++) {
		re[i] = data[i];
	}
	fft_radix2(re, im, N);

	int low_index = (int)ceil((double)N / (double)n_data);
	int len = N / 2 - low_index + 1;
	if (len < 1) {
		len = 1;
	}
	double *sp = (double *)malloc((size_t)len * sizeof(double));
	double *fr = (double *)malloc((size_t)len * sizeof(double));
	if (!sp || !fr) {
		free(re);
		free(im);
		free(sp);
		free(fr);
		return false;
	}
	for (int k = 0; k < len; k++) {
		int idx = low_index + k;
		double mag2 = re[idx] * re[idx] + im[idx] * im[idx];
		sp[k] = mag2;
		fr[k] = (double)idx / (double)N;
	}
	free(re);
	free(im);
	*spectrum = sp;
	*frequencies = fr;
	*out_len = len;
	return true;
}


/*  Covariance kernels */

/*
 * Evaluate the covariance K[i][j] = k(x_i, y_j) into out (nx x ny).
 * log_hyper is the GP log-space vector of length 8:
 *   [log_noise, log(ls0), log(sv0), log(lsP), log(svP), log(ls1), log(sv1), log(period)]
 * projection == true uses PeriodicSquareExponential (long SE + periodic),
 * projection == false uses PeriodicSquareExponential2 (long SE + periodic + short SE).
 *
 * Distances are computed directly as (x_i - y_j)^2; the original mean-centering
 * in math_tools::squareDistance is purely for numerical conditioning of the
 * binomial expansion and is unnecessary (and slightly less accurate) here.
 */
static void kernel_eval(double *out, const double *x, int nx, const double *y, int ny, const double *log_hyper, bool projection) {
	double ls_se0 = exp(log_hyper[1]);
	double sv_se0 = exp(2.0 * log_hyper[2]);
	double ls_p = exp(log_hyper[3]);
	double sv_p = exp(2.0 * log_hyper[4]);
	double ls_se1 = exp(log_hyper[5]);
	double sv_se1 = exp(2.0 * log_hyper[6]);
	double pl_p = exp(log_hyper[7]);

	double inv_se0 = -0.5 / (ls_se0 * ls_se0);
	double inv_se1 = -0.5 / (ls_se1 * ls_se1);

	for (int i = 0; i < nx; i++) {
		for (int j = 0; j < ny; j++) {
			double d = x[i] - y[j];
			double d2 = d * d;
			double s = sin(M_PI / pl_p * sqrt(d2)) / ls_p;
			double k = sv_se0 * exp(inv_se0 * d2) + sv_p * exp(-2.0 * s * s);
			if (!projection) {
				k += sv_se1 * exp(inv_se1 * d2);
			}
			out[i * ny + j] = k;
		}
	}
}

/* GP state + guider state */

typedef struct {
	double timestamp;
	double measurement; /* current pointing error */
	double variance;    /* current measurement variance */
	double control;     /* control action */
} data_point;

struct indigo_gp_guider {
	/* tuning */
	double control_gain;
	double prediction_gain;
	double min_move;
	double min_periods_for_inference;
	double min_periods_for_period_estimation;
	int points_for_approximation;
	bool compute_period;
	double learning_rate;

	/* GP log-space hyperparameters (length 8) */
	double log_hyper[NUM_HYPERPARAMETERS + 1];

	/* timing */
	double start_time;
	double last_time;

	double prediction;
	double last_prediction_end;

	int dither_steps;
	bool dithering_active;
	double dither_offset;

	/* cross-session state for the retain-model decision */
	double prev_ra;      /* mount RA (hours) at the previous session start, NaN if none */
	int prev_sop;        /* side of pier at the previous session start, 0 if unknown */
	double stopped_time; /* monotonic time guiding last stopped */

	/* circular buffer (chronological FIFO: index 0 = oldest) */
	data_point *buffer;
	int buf_tail;
	int buf_size;
	int buf_capacity;

	/* cached GP inference (valid after update_gp) */
	bool gp_has_data;
	int gp_n;            /* number of data points used */
	double *gp_data_loc; /* gp_n */
	double *gp_alpha;    /* gp_n */
	ldlt_t gp_chol;      /* factorisation of the Gram matrix (gp_n x gp_n) */
	double *gp_feature;  /* 2 x gp_n explicit-trend feature vectors */
	ldlt_t gp_chol_feat; /* factorisation of the 2x2 feature matrix */
	double gp_beta[2];
};

/* ---- monotonic clock --------------------------------------------------- */

#ifdef INDIGO_GP_GUIDER_TEST_CLOCK
/* Test hook: a virtual clock the test harness advances explicitly, because real
 * guide cycles are seconds apart while a test loop runs in microseconds. */
double indigo_gp_guider_test_clock = 0.0;
static double now_seconds(void) { return indigo_gp_guider_test_clock; }
#else
static double now_seconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif

/* ---- circular buffer --------------------------------------------------- */

static void cb_clear(indigo_gp_guider *g) {
	g->buf_tail = 0;
	g->buf_size = 0;
}

static void cb_push(indigo_gp_guider *g) {
	int idx = (g->buf_tail + g->buf_size) % g->buf_capacity;
	memset(&g->buffer[idx], 0, sizeof(data_point));
	if (g->buf_size < g->buf_capacity) {
		g->buf_size++;
	} else {
		g->buf_tail = (g->buf_tail + 1) % g->buf_capacity;
	}
}

static data_point *cb_at(indigo_gp_guider *g, int n) {
	return &g->buffer[(g->buf_tail + n) % g->buf_capacity];
}

static data_point *cb_last(indigo_gp_guider *g) { return cb_at(g, g->buf_size - 1); }
static data_point *cb_second_last(indigo_gp_guider *g) { return cb_at(g, g->buf_size - 2); }

/* ---- hyperparameter conversion (natural <-> GP log space) -------------- */

static void set_gp_hyperparameters(indigo_gp_guider *g, const double natural[NUM_HYPERPARAMETERS]) {
	double h[NUM_HYPERPARAMETERS];
	memcpy(h, natural, sizeof(h));

	/* prevent length scales from becoming too small (makes GP unstable) */
	if (h[SE0K_LENGTH_SCALE] < 1.0) {
		h[SE0K_LENGTH_SCALE] = 1.0;
	}
	if (h[PK_LENGTH_SCALE] < 1.0) {
		h[PK_LENGTH_SCALE] = 1.0;
	}
	if (h[SE1K_LENGTH_SCALE] < 1.0) {
		h[SE1K_LENGTH_SCALE] = 1.0;
	}

	/* convert periodic length-scale from natural units to standard notation */
	h[PK_LENGTH_SCALE] = 4.0 * sin(h[PK_LENGTH_SCALE] * M_PI / h[PK_PERIOD_LENGTH]);

	for (int i = 0; i < NUM_HYPERPARAMETERS; i++) {
		if (h[i] < 1e-10) {
			h[i] = 1e-10;
		}
	}

	g->log_hyper[0] = log(1.0); /* noise term, unused (heteroscedastic) */
	for (int i = 0; i < NUM_HYPERPARAMETERS; i++) {
		g->log_hyper[i + 1] = log(h[i]);
	}
}

static void get_gp_hyperparameters(const indigo_gp_guider *g, double natural[NUM_HYPERPARAMETERS]) {
	double h[NUM_HYPERPARAMETERS];
	for (int i = 0; i < NUM_HYPERPARAMETERS; i++) {
		h[i] = exp(g->log_hyper[i + 1]);
	}
	/* convert periodic length-scale from standard notation back to natural units */
	h[PK_LENGTH_SCALE] = asin(h[PK_LENGTH_SCALE] / 4.0) * h[PK_PERIOD_LENGTH] / M_PI;
	memcpy(natural, h, sizeof(h));
}

static double get_period_length(const indigo_gp_guider *g) {
	return exp(g->log_hyper[PK_PERIOD_LENGTH + 1]);
}

static void update_period_length(indigo_gp_guider *g, double period_length) {
	double h[NUM_HYPERPARAMETERS];
	get_gp_hyperparameters(g, h);
	if (is_nan(period_length)) {
		period_length = h[PK_PERIOD_LENGTH];
	}
	h[PK_PERIOD_LENGTH] = (1.0 - g->learning_rate) * h[PK_PERIOD_LENGTH] + g->learning_rate * period_length;
	set_gp_hyperparameters(g, h);
}

/* ---- GP inference cache management ------------------------------------- */

static void gp_clear(indigo_gp_guider *g) {
	if (g->gp_has_data) {
		free(g->gp_data_loc);
		free(g->gp_alpha);
		free(g->gp_feature);
		ldlt_free(&g->gp_chol);
		ldlt_free(&g->gp_chol_feat);
		g->gp_data_loc = NULL;
		g->gp_alpha = NULL;
		g->gp_feature = NULL;
	}
	g->gp_has_data = false;
	g->gp_n = 0;
}

/*
 * GP inference on a subset of n most-relevant points (GP::inferSD + GP::infer).
 * data_loc/data_out/data_var hold N points; the n points with the highest
 * covariance to prediction_point are selected. Builds the Gram matrix, its LDLT
 * factorisation, alpha = K^-1 y, and the explicit linear-trend basis (beta).
 */
static void gp_infer_sd(indigo_gp_guider *g, const double *data_loc, const double *data_out, const double *data_var, int N, int n, double prediction_point) {
	gp_clear(g);
	if (N <= 0) {
		return;
	}

	int use_n = n;
	double *loc, *out, *var;
	if (use_n < N) {
		/* covariance between each data point and the prediction location */
		double *cov = (double *)malloc((size_t)N * sizeof(double));
		int *index = (int *)malloc((size_t)N * sizeof(int));
		if (!cov || !index) {
			free(cov);
			free(index);
			return;
		}
		kernel_eval(cov, data_loc, N, &prediction_point, 1, g->log_hyper, false);
		for (int i = 0; i < N; i++) {
			index[i] = i;
		}
		/* partial selection sort for the top use_n by covariance (descending) */
		for (int a = 0; a < use_n; a++) {
			int best = a;
			for (int b = a + 1; b < N; b++) {
				if (cov[index[b]] > cov[index[best]]) {
					best = b;
				}
			}
			int t = index[a];
			index[a] = index[best];
			index[best] = t;
		}
		loc = (double *)malloc((size_t)use_n * sizeof(double));
		out = (double *)malloc((size_t)use_n * sizeof(double));
		var = (double *)malloc((size_t)use_n * sizeof(double));
		if (!loc || !out || !var) {
			free(cov); free(index); free(loc); free(out); free(var);
			return;
		}
		for (int i = 0; i < use_n; i++) {
			loc[i] = data_loc[index[i]];
			out[i] = data_out[index[i]];
			var[i] = data_var[index[i]];
		}
		free(cov);
		free(index);
	} else {
		use_n = N;
		loc = (double *)malloc((size_t)use_n * sizeof(double));
		out = (double *)malloc((size_t)use_n * sizeof(double));
		var = (double *)malloc((size_t)use_n * sizeof(double));
		if (!loc || !out || !var) {
			free(loc); free(out); free(var);
			return;
		}
		memcpy(loc, data_loc, (size_t)use_n * sizeof(double));
		memcpy(out, data_out, (size_t)use_n * sizeof(double));
		memcpy(var, data_var, (size_t)use_n * sizeof(double));
	}

	int nn = use_n;
	/* Gram matrix = K(loc,loc) + diag(var) (heteroscedastic noise) */
	double *gram = (double *)malloc((size_t)nn * nn * sizeof(double));
	if (!gram) {
		free(loc); free(out); free(var);
		return;
	}
	kernel_eval(gram, loc, nn, loc, nn, g->log_hyper, false);
	for (int i = 0; i < nn; i++) {
		gram[i * nn + i] += var[i];
	}

	if (!ldlt_factor(&g->gp_chol, gram, nn)) {
		free(gram); free(loc); free(out); free(var);
		return;
	}
	free(gram);

	/* alpha = K^-1 * data_out */
	double *alpha = (double *)malloc((size_t)nn * sizeof(double));
	if (!alpha) {
		ldlt_free(&g->gp_chol);
		free(loc); free(out); free(var);
		return;
	}
	ldlt_solve(&g->gp_chol, out, alpha, 1);

	/* explicit linear trend basis:
	 *   feature_vectors (2 x nn): row0 = 1, row1 = loc
	 *   feature_matrix  (2 x 2)  = FV * K^-1 * FV^T
	 *   beta = feature_matrix^-1 * (FV * alpha)
	 */
	double *feature = (double *)malloc((size_t)2 * nn * sizeof(double));
	if (!feature) {
		free(alpha);
		ldlt_free(&g->gp_chol);
		free(loc); free(out); free(var);
		return;
	}
	for (int j = 0; j < nn; j++) {
		feature[0 * nn + j] = 1.0;
		feature[1 * nn + j] = loc[j];
	}
	/* FV^T is nn x 2; solve K * S = FV^T -> S (nn x 2) */
	double *fvt = (double *)malloc((size_t)nn * 2 * sizeof(double));
	double *S = (double *)malloc((size_t)nn * 2 * sizeof(double));
	if (!fvt || !S) {
		free(fvt); free(S); free(feature); free(alpha);
		ldlt_free(&g->gp_chol);
		free(loc); free(out); free(var);
		return;
	}
	for (int i = 0; i < nn; i++) {
		fvt[i * 2 + 0] = feature[0 * nn + i];
		fvt[i * 2 + 1] = feature[1 * nn + i];
	}
	ldlt_solve(&g->gp_chol, fvt, S, 2);
	double fmat[4];
	mat_mul(feature, S, fmat, 2, nn, 2); /* (2 x nn)(nn x 2) = 2 x 2 */
	if (!ldlt_factor(&g->gp_chol_feat, fmat, 2)) {
		free(fvt); free(S); free(feature); free(alpha);
		ldlt_free(&g->gp_chol);
		free(loc); free(out); free(var);
		return;
	}
	double fv_alpha[2];
	mat_mul(feature, alpha, fv_alpha, 2, nn, 1); /* (2 x nn)(nn x 1) = 2 */
	ldlt_solve(&g->gp_chol_feat, fv_alpha, g->gp_beta, 1);

	free(fvt);
	free(S);
	free(out);
	free(var);

	g->gp_data_loc = loc;
	g->gp_alpha = alpha;
	g->gp_feature = feature;
	g->gp_n = nn;
	g->gp_has_data = true;
}

/*
 * Projected GP prediction of the mean at the given locations (GP::predict with
 * the projection kernel and explicit trend). Writes mean[] (length nloc).
 */
static void gp_predict_projected(indigo_gp_guider *g, const double *locations, int nloc, double *mean) {
	if (!g->gp_has_data) {
		for (int i = 0; i < nloc; i++) {
			mean[i] = 0.0;
		}
		return;
	}
	int nn = g->gp_n;

	/* mixed_cov = K_proj(locations, data_loc): nloc x nn */
	double *mixed = (double *)malloc((size_t)nloc * nn * sizeof(double));
	if (!mixed) {
		for (int i = 0; i < nloc; i++) {
			mean[i] = 0.0;
		}
		return;
	}
	kernel_eval(mixed, locations, nloc, g->gp_data_loc, nn, g->log_hyper, true);

	/* m = mixed_cov * alpha */
	mat_mul(mixed, g->gp_alpha, mean, nloc, nn, 1);

	/* gamma = K^-1 * mixed_cov^T : nn x nloc */
	double *mixed_t = (double *)malloc((size_t)nn * nloc * sizeof(double));
	double *gamma = (double *)malloc((size_t)nn * nloc * sizeof(double));
	if (!mixed_t || !gamma) {
		free(mixed); free(mixed_t); free(gamma);
		return;
	}
	for (int i = 0; i < nloc; i++) {
		for (int j = 0; j < nn; j++) {
			mixed_t[j * nloc + i] = mixed[i * nn + j];
		}
	}
	ldlt_solve(&g->gp_chol, mixed_t, gamma, nloc);

	/* phi (2 x nloc): row0 = 1, row1 = locations
	 * R = phi - feature_vectors * gamma  (2 x nloc)
	 * m += R^T * beta
	 */
	double *R = (double *)malloc((size_t)2 * nloc * sizeof(double));
	if (!R) {
		free(mixed); free(mixed_t); free(gamma);
		return;
	}
	double *fg = (double *)malloc((size_t)2 * nloc * sizeof(double));
	if (!fg) {
		free(mixed); free(mixed_t); free(gamma); free(R);
		return;
	}
	mat_mul(g->gp_feature, gamma, fg, 2, nn, nloc); /* (2 x nn)(nn x nloc) */
	for (int i = 0; i < nloc; i++) {
		R[0 * nloc + i] = 1.0 - fg[0 * nloc + i];
		R[1 * nloc + i] = locations[i] - fg[1 * nloc + i];
	}
	for (int i = 0; i < nloc; i++) {
		mean[i] += R[0 * nloc + i] * g->gp_beta[0] + R[1 * nloc + i] * g->gp_beta[1];
	}

	free(mixed);
	free(mixed_t);
	free(gamma);
	free(R);
	free(fg);
}

/* Guider data handling ( gaussian process guider) */

static double calculate_variance(double snr) {
	if (snr < 3.4) {
		snr = 3.4;
	}
	double sd = 2.1752 / (snr - 3.3) + 0.5;
	return sd * sd;
}

static void set_timestamp(indigo_gp_guider *g) {
	double current_time = now_seconds();
	double delta = current_time - g->last_time;
	g->last_time = current_time;
	cb_last(g)->timestamp = (current_time - g->start_time) - delta / 2.0 + g->dither_offset;
}

static void handle_guiding(indigo_gp_guider *g, double input, double snr) {
	set_timestamp(g);
	cb_last(g)->measurement = input;
	cb_last(g)->variance = calculate_variance(snr);
	g->last_prediction_end = cb_last(g)->timestamp;
}

static void handle_dark_guiding(indigo_gp_guider *g) {
	set_timestamp(g);
	cb_last(g)->measurement = 0.0;
	cb_last(g)->variance = 1e4;
}

static double estimate_period_length(const double *time, const double *data, int n);

/*
 * regularize_dataset: resample the irregular (timestamp, gear_error, variance)
 * samples onto a fixed GRID_INTERVAL grid by trapezoidal averaging, mirroring
 * GaussianProcessGuider::regularize_dataset.
 *
 * Outputs reg_time/reg_gear/reg_var (caller-allocated, capacity >= grid_size)
 * and returns the number of grid cells produced, or -1 on overrun while
 * dithering.
 */
static int regularize_dataset(indigo_gp_guider *g, const double *timestamps, const double *gear_error, const double *variances, int count, double *reg_time, double *reg_gear, double *reg_var, int grid_size) {
	double grid_interval = GRID_INTERVAL;
	double last_cell_end = -grid_interval;
	double last_timestamp = -grid_interval;
	double last_gear_error = 0.0;
	double last_variance = 0.0;
	double gear_error_sum = 0.0;
	double variance_sum = 0.0;
	int j = 0;

	for (int i = 0; i < count; ++i) {
		if (timestamps[i] < last_cell_end + grid_interval) {
			gear_error_sum += (timestamps[i] - last_timestamp) * 0.5 * (last_gear_error + gear_error[i]);
			variance_sum += (timestamps[i] - last_timestamp) * 0.5 * (last_variance + variances[i]);
			last_timestamp = timestamps[i];
		} else {
			while (timestamps[i] >= last_cell_end + grid_interval) {
				if (g->dithering_active && j >= grid_size) {
					return -1; /* index over-run */
				}
				double inter_timestamp = last_cell_end + grid_interval;
				double proportion = (inter_timestamp - last_timestamp) / (timestamps[i] - last_timestamp);
				double inter_gear_error = proportion * gear_error[i] + (1 - proportion) * last_gear_error;
				double inter_variance = proportion * variances[i] + (1 - proportion) * last_variance;

				gear_error_sum += (inter_timestamp - last_timestamp) * 0.5 * (last_gear_error + inter_gear_error);
				variance_sum += (inter_timestamp - last_timestamp) * 0.5 * (last_variance + inter_variance);

				reg_time[j] = last_cell_end + 0.5 * grid_interval;
				reg_gear[j] = gear_error_sum / grid_interval;
				reg_var[j] = variance_sum / grid_interval;

				last_timestamp = inter_timestamp;
				last_gear_error = inter_gear_error;
				last_variance = inter_variance;
				last_cell_end = inter_timestamp;

				gear_error_sum = 0.0;
				variance_sum = 0.0;
				++j;
			}
		}
	}
	if (j > REGULAR_BUFFER_SIZE) {
		j = REGULAR_BUFFER_SIZE;
	}
	return j;
}

/*
 * UpdateGP: assemble the data, regularize, detrend, optionally estimate the
 * period, and run the GP inference. Returns false on failure (e.g. dithering
 * over-run) so the caller can recover.
 */
static bool update_gp(indigo_gp_guider *g, double prediction_point) {
	int N = g->buf_size;
	if (N < 2) {
		return true;
	}
	int count = N - 1;

	double *timestamps = (double *)malloc((size_t)count * sizeof(double));
	double *gear_error = (double *)malloc((size_t)count * sizeof(double));
	double *variances = (double *)malloc((size_t)count * sizeof(double));
	if (!timestamps || !gear_error || !variances) {
		free(timestamps); free(gear_error); free(variances);
		return false;
	}

	double sum_control = 0.0;
	for (int i = 0; i < count; i++) {
		data_point *p = cb_at(g, i);
		sum_control += p->control;
		timestamps[i] = p->timestamp;
		variances[i] = p->variance;
		gear_error[i] = sum_control + p->measurement; /* accumulated gear error */
	}

	/* regularize onto the fixed grid */
	int grid_size = (int)ceil(timestamps[count - 1] / GRID_INTERVAL) + 1;
	if (grid_size < 1) {
		grid_size = 1;
	}
	double *rt = (double *)malloc((size_t)grid_size * sizeof(double));
	double *rg = (double *)malloc((size_t)grid_size * sizeof(double));
	double *rv = (double *)malloc((size_t)grid_size * sizeof(double));
	if (!rt || !rg || !rv) {
		free(timestamps); free(gear_error); free(variances); free(rt); free(rg); free(rv);
		return false;
	}
	int T = regularize_dataset(g, timestamps, gear_error, variances, count, rt, rg, rv, grid_size);
	free(timestamps);
	free(gear_error);
	free(variances);
	if (T < 0) {
		free(rt); free(rg); free(rv);
		return false;
	}
	if (T < 2) {
		free(rt); free(rg); free(rv);
		return true;
	}

	/* linear least squares de-trend (offset + drift) for the period estimation:
	 *   FM (2 x T): row0 = 1, row1 = time
	 *   weights = (FM FM^T + 1e-3 I)^-1 (FM gear)
	 */
	double A[4] = {0, 0, 0, 0};
	double b[2] = {0, 0};
	for (int i = 0; i < T; i++) {
		A[0] += 1.0;        /* sum 1 */
		A[1] += rt[i];      /* sum t */
		A[3] += rt[i] * rt[i]; /* sum t^2 */
		b[0] += rg[i];
		b[1] += rt[i] * rg[i];
	}
	A[2] = A[1];
	A[0] += 1e-3;
	A[3] += 1e-3;
	/* solve 2x2 SPD system A w = b */
	double det = A[0] * A[3] - A[1] * A[2];
	double w0 = 0, w1 = 0;
	if (det != 0) {
		w0 = (b[0] * A[3] - A[1] * b[1]) / det;
		w1 = (A[0] * b[1] - b[0] * A[2]) / det;
	}

	/* period estimation (optional) */
	double period_length = get_period_length(g);
	if (g->compute_period && cb_last(g)->timestamp > g->min_periods_for_period_estimation * period_length) {
		double *detrend = (double *)malloc((size_t)T * sizeof(double));
		if (detrend) {
			for (int i = 0; i < T; i++) {
				detrend[i] = rg[i] - (w0 + w1 * rt[i]);
			}
			double pl = estimate_period_length(rt, detrend, T);
			update_period_length(g, pl);
			free(detrend);
		}
	}

	/* GP inference uses the (non-detrended) gear error; the explicit trend
	 * basis inside the GP handles the linear component. */
	gp_infer_sd(g, rt, rg, rv, T, g->points_for_approximation, prediction_point);

	free(rt);
	free(rg);
	free(rv);
	return true;
}

static double predict_gear_error(indigo_gp_guider *g, double prediction_location) {
	if (g->last_prediction_end < 0.0) {
		g->last_prediction_end = now_seconds() - g->start_time;
	}

	double next_location[2];
	next_location[0] = g->last_prediction_end;
	next_location[1] = prediction_location + g->dither_offset;

	double prediction[2];
	gp_predict_projected(g, next_location, 2, prediction);

	g->last_prediction_end = next_location[1];
	return prediction[1] - prediction[0];
}

/*
 * EstimatePeriodLength: Hamming window -> power spectrum -> peak with quadratic
 * interpolation, mirroring GaussianProcessGuider::EstimatePeriodLength.
 */
static double estimate_period_length(const double *time, const double *data, int n) {
	if (n < 2) {
		return NAN; /* not reachable: callers guard T >= 2 */
	}

	double *windowed = (double *)malloc((size_t)n * sizeof(double));
	if (!windowed) {
		return NAN;
	}
	for (int i = 0; i < n; i++) {
		double range = (n > 1) ? (double)i / (double)(n - 1) : 0.0;
		double w = 0.54 - 0.46 * cos(2.0 * M_PI * range);
		windowed[i] = data[i] * w;
	}

	double *spectrum = NULL, *frequencies = NULL;
	int len = 0;
	bool ok = compute_spectrum(windowed, n, FFT_SIZE, &spectrum, &frequencies, &len);
	free(windowed);
	if (!ok) {
		return NAN;
	}

	double dt = (time[n - 1] - time[0]) / (double)(n - 1);
	for (int i = 0; i < len; i++) {
		frequencies[i] /= dt;
	}

	/* zero amplitudes for too-large periods (> 1500 s) */
	for (int i = 0; i < len; i++) {
		double period = 1.0 / frequencies[i];
		if (period > 1500.0) {
			spectrum[i] = 0.0;
		}
	}

	int max_index = 0;
	for (int i = 1; i < len; i++) {
		if (spectrum[i] > spectrum[max_index]) {
			max_index = i;
		}
	}
	double max_frequency = frequencies[max_index];

	if (max_index < len - 1 && max_index > 0) {
		double spread = fabs(frequencies[max_index - 1] - frequencies[max_index + 1]);
		double interp_loc[3], interp_dat[3];
		interp_loc[0] = frequencies[max_index - 1];
		interp_loc[1] = frequencies[max_index];
		interp_loc[2] = frequencies[max_index + 1];
		for (int i = 0; i < 3; i++) {
			interp_loc[i] = (interp_loc[i] - max_frequency) / spread;
		}
		interp_dat[0] = spectrum[max_index - 1];
		interp_dat[1] = spectrum[max_index];
		interp_dat[2] = spectrum[max_index + 1];
		double amax = spectrum[max_index];
		for (int i = 0; i < 3; i++) {
			interp_dat[i] /= amax;
		}

		double dmin = interp_dat[0], dmax = interp_dat[0];
		for (int i = 1; i < 3; i++) {
			if (interp_dat[i] < dmin) {
				dmin = interp_dat[i];
			}
			if (interp_dat[i] > dmax) {
				dmax = interp_dat[i];
			}
		}
		if (dmax - dmin < 1e-10) {
			free(spectrum);
			free(frequencies);
			return 1.0 / max_frequency;
		}

		/* phi (3 x 3): row0 = loc^2, row1 = loc, row2 = 1
		 * w = (phi phi^T)^-1 (phi dat) */
		double phi[9];
		for (int c = 0; c < 3; c++) {
			phi[0 * 3 + c] = interp_loc[c] * interp_loc[c];
			phi[1 * 3 + c] = interp_loc[c];
			phi[2 * 3 + c] = 1.0;
		}
		double M[9];
		mat_mul_bt(phi, phi, M, 3, 3, 3); /* phi phi^T */
		double rhs[3];
		mat_mul(phi, interp_dat, rhs, 3, 3, 1);
		ldlt_t f;
		if (ldlt_factor(&f, M, 3)) {
			double w[3];
			ldlt_solve(&f, rhs, w, 1);
			ldlt_free(&f);
			if (w[0] != 0.0) {
				max_frequency = max_frequency - w[1] / (2.0 * w[0]) * spread;
			}
		}
	}

	free(spectrum);
	free(frequencies);
	return 1.0 / max_frequency;
}

/* returns the internal control_signal */
static double gp_deduce_result(indigo_gp_guider *g, double time_step, double prediction_point) {
	handle_dark_guiding(g);

	double control_signal = 0.0;
	if (g->buf_size > 10 && cb_last(g)->timestamp > g->min_periods_for_inference * get_period_length(g)) {
		if (prediction_point < 0.0) {
			prediction_point = now_seconds() - g->start_time;
		}
		update_gp(g, prediction_point + 0.5 * time_step);
		g->prediction = predict_gear_error(g, prediction_point + time_step);
		control_signal += g->prediction;
	}

	cb_push(g);
	cb_last(g)->control = control_signal;

	if (is_nan(control_signal)) {
		control_signal = 0.0;
	}

	INDIGO_DEBUG(indigo_debug("%s(): dark frame, control = %.3f, prediction = %.3f, period = %.2f, points = %d", __FUNCTION__, control_signal, g->prediction, get_period_length(g), g->buf_size));

	return control_signal;
}

/* returns the internal control_signal */
static double gp_result(indigo_gp_guider *g, double input, double snr, double time_step, double prediction_point) {
	double hyst_percentage = 0.0;

	if (g->dithering_active) {
		if (--g->dither_steps <= 0) {
			g->dithering_active = false;
		}
		/* while dithering we don't trust the measurement: pretend we do dark
		 * guiding to keep the GP/FFT consistent, but apply plain proportional
		 * control. update_gp() degrades gracefully on a regularize over-run
		 * (keeps the previous GP), so no recovery is needed here. */
		gp_deduce_result(g, time_step, -1.0);
		INDIGO_DEBUG(indigo_debug("%s(): dithering, input = %.3f, control = %.3f, dither_steps = %d", __FUNCTION__, input, g->control_gain * input, g->dither_steps));
		return g->control_gain * input;
	}

	if (g->buf_size == 1) {
		g->start_time = now_seconds();
		g->last_time = g->start_time;
	}

	handle_guiding(g, input, snr);

	double last_control = 0.0;
	if (g->buf_size > 1) {
		last_control = cb_second_last(g)->control;
	}
	double hysteresis_control = (1.0 - HYSTERESIS) * input + HYSTERESIS * last_control;
	hysteresis_control *= g->control_gain;

	double control_signal = g->control_gain * input;
	if (fabs(input) < g->min_move) {
		control_signal = 0.0;
		hysteresis_control = 0.0;
	}

	if (g->buf_size > 10) {
		if (prediction_point < 0.0) {
			prediction_point = now_seconds() - g->start_time;
		}
		update_gp(g, prediction_point + 0.5 * time_step);
		g->prediction = predict_gear_error(g, prediction_point + time_step);
		control_signal += g->prediction_gain * g->prediction;

		double period_length = get_period_length(g);
		if (cb_last(g)->timestamp < g->min_periods_for_inference * period_length) {
			double percentage = cb_last(g)->timestamp / (g->min_periods_for_inference * period_length);
			if (percentage > 1.0) {
				percentage = 1.0;
			}
			hyst_percentage = 1.0 - percentage;
			control_signal = percentage * control_signal + (1.0 - percentage) * hysteresis_control;
		}
	}

	if (is_nan(control_signal)) {
		control_signal = hysteresis_control;
	}

	cb_push(g);
	cb_last(g)->control = control_signal;

	INDIGO_DEBUG(indigo_debug("%s(): input = %.3f, control = %.3f, reactive = %.3f, prediction = %.3f, hysteresis = %.3f, hyst_pct = %.3f, period = %.2f, points = %d",
		__FUNCTION__, input, control_signal, g->control_gain * input, g->prediction_gain * g->prediction, hysteresis_control, hyst_percentage, get_period_length(g), g->buf_size));

	return control_signal;
}

/* Public API */

static void apply_defaults(indigo_gp_guider *g) {
	g->control_gain = DEFAULT_CONTROL_GAIN;
	g->prediction_gain = DEFAULT_PREDICTION_GAIN;
	g->min_move = DEFAULT_MIN_MOVE;
	g->min_periods_for_inference = DEFAULT_PERIODS_FOR_INFERENCE;
	g->min_periods_for_period_estimation = DEFAULT_PERIODS_FOR_PERIOD_ESTIMATION;
	g->points_for_approximation = DEFAULT_POINTS_FOR_APPROXIMATION;
	g->compute_period = DEFAULT_COMPUTE_PERIOD;
	g->learning_rate = DEFAULT_LEARNING_RATE;

	double natural[NUM_HYPERPARAMETERS];
	natural[SE0K_LENGTH_SCALE] = DEFAULT_LS_SE0;
	natural[SE0K_SIGNAL_VARIANCE] = DEFAULT_SV_SE0;
	natural[PK_LENGTH_SCALE] = DEFAULT_LS_PK;
	natural[PK_SIGNAL_VARIANCE] = DEFAULT_SV_PK;
	natural[SE1K_LENGTH_SCALE] = DEFAULT_LS_SE1;
	natural[SE1K_SIGNAL_VARIANCE] = DEFAULT_SV_SE1;
	natural[PK_PERIOD_LENGTH] = DEFAULT_PERIOD_PK;
	set_gp_hyperparameters(g, natural);
}

indigo_gp_guider *indigo_gp_guider_create(void) {
	indigo_gp_guider *g = (indigo_gp_guider *)calloc(1, sizeof(indigo_gp_guider));
	if (!g) {
		return NULL;
	}
	g->buffer = (data_point *)calloc(CIRCULAR_BUFFER_SIZE, sizeof(data_point));
	if (!g->buffer) {
		free(g);
		return NULL;
	}
	g->buf_capacity = CIRCULAR_BUFFER_SIZE;
	g->prev_ra = NAN;
	g->prev_sop = 0;
	g->stopped_time = 0.0;
	apply_defaults(g);
	indigo_gp_guider_reset(g);
	return g;
}

void indigo_gp_guider_destroy(indigo_gp_guider *g) {
	if (!g) {
		return;
	}
	gp_clear(g);
	free(g->buffer);
	free(g);
}

void indigo_gp_guider_reset(indigo_gp_guider *g) {
	if (!g) {
		return;
	}
	gp_clear(g);
	cb_clear(g);
	/* the first measurement is always relative to a zero control point */
	cb_push(g);
	cb_last(g)->control = 0.0;

	g->last_prediction_end = -1.0;
	g->start_time = now_seconds();
	g->last_time = g->start_time;
	g->prediction = 0.0;
	g->dither_offset = 0.0;
	g->dither_steps = 0;
	g->dithering_active = false;
}

/* wrap an RA difference (hours) into [-12, 12) */
static double norm_ra_hours(double v) {
	while (v < -12.0) {
		v += 24.0;
	}
	while (v >= 12.0) {
		v -= 24.0;
	}
	return v;
}

bool indigo_gp_guider_session_start(indigo_gp_guider *g, double current_ra, int side_of_pier, double retain_pct) {
	if (!g) {
		return false;
	}

	bool need_reset = true;
	double ra_offset = 0.0;

	/* retain only on the same (known) side of pier with valid RA on both ends */
	if (!is_nan(current_ra) && !is_nan(g->prev_ra) && g->prev_sop != 0 && g->prev_sop == side_of_pier) {
		const double SECONDS_PER_HOUR = 3600.0;
		const double SIDEREAL_SECONDS_PER_SEC = 0.9973;

		/* an increase in RA corresponds to a negative shift in gear time */
		ra_offset = norm_ra_hours(g->prev_ra - current_ra) * SECONDS_PER_HOUR / SIDEREAL_SECONDS_PER_SEC;

		double elapsed = now_seconds() - g->stopped_time;
		double worm_offset = elapsed + ra_offset;
		double period_length = get_period_length(g);

		if (fabs(worm_offset) < retain_pct / 100.0 * period_length) {
			need_reset = false;
		}

		/* the GP expects gear time to advance monotonically; it cannot handle the
		 * worm having moved backwards, so reset in that case */
		if (!need_reset && worm_offset < 0.0) {
			need_reset = true;
		}

		INDIGO_DEBUG(indigo_debug("%s(): stopped %.1fs, deltaRA %+.1fs, worm delta %+.1fs (%.1f%% of %.1fs period), limit %.1f%% -> %s",
			__FUNCTION__, elapsed, -ra_offset, worm_offset, fabs(worm_offset) / period_length * 100.0, period_length, retain_pct,
			need_reset ? "reset" : "retain"));
	}

	if (need_reset) {
		indigo_gp_guider_reset(g);
	} else {
		/* resume: shift gear time by the RA slew and predict-only for a few
		 * frames, exactly like settling after a dither */
		g->dither_offset += ra_offset;
		g->dithering_active = true;
		g->dither_steps = MAX_DITHER_STEPS;
	}

	g->prev_ra = current_ra;
	g->prev_sop = side_of_pier;
	return !need_reset;
}

void indigo_gp_guider_session_stop(indigo_gp_guider *g) {
	if (!g) {
		return;
	}
	g->stopped_time = now_seconds();
}

void indigo_gp_guider_set_parameters(indigo_gp_guider *g, double control_gain, double prediction_gain, double min_move, bool compute_period, double period_length) {
	if (!g) {
		return;
	}
	g->control_gain = control_gain;
	g->prediction_gain = prediction_gain;
	g->min_move = min_move;
	g->compute_period = compute_period;
	if (period_length > 0.0) {
		double natural[NUM_HYPERPARAMETERS];
		get_gp_hyperparameters(g, natural);
		natural[PK_PERIOD_LENGTH] = period_length;
		set_gp_hyperparameters(g, natural);
	}
}

double indigo_gp_guider_get_period_length(const indigo_gp_guider *g) {
	if (!g) {
		return 0.0;
	}
	return get_period_length(g);
}

double indigo_gp_guider_response(indigo_gp_guider *g, double drift, double snr, double time_step) {
	if (!g) {
		return 0.0;
	}
	if (snr <= 0.0) {
		snr = 10.0; /* reasonable default when SNR is unknown */
	}
	double control_signal = gp_result(g, drift, snr, time_step, -1.0);
	/* INDIGO's response functions return a correction with the opposite sign (the
	 * pulse direction that counteracts the drift), so negate at the boundary. */
	return -control_signal;
}

double indigo_gp_guider_deduce(indigo_gp_guider *g, double time_step) {
	if (!g) {
		return 0.0;
	}
	double control_signal = gp_deduce_result(g, time_step, -1.0);
	return -control_signal;
}

void indigo_gp_guider_dithered(indigo_gp_guider *g, double amount_px, double ra_rate) {
	if (!g || ra_rate == 0.0) {
		return;
	}
	g->dither_offset += amount_px / ra_rate; /* offset in gear time (seconds) */
	g->dithering_active = true;
	g->dither_steps = MAX_DITHER_STEPS;
}

void indigo_gp_guider_dither_settle_done(indigo_gp_guider *g, bool success) {
	if (!g) {
		return;
	}
	if (success) {
		g->dither_steps = 1; /* the last dither step is executed by result() */
	}
}
