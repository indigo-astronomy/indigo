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
//  Extended and generalized to a Multi Kernel GP by Rumen G. Bogdanovski: the
//  covariance is no longer a single periodic kernel over one worm period but a
//  sum over an arbitrary number of periodic stages, each modelling a further
//  gear stage whose period is incommensurate with the others. Every stage is
//  discovered from the spectrum, gated on the strength of its own line, and
//  refused if it is commensurate with a period already modelled.
//
//  version history
//  3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>
//  3.1 Multi Kernel GP by Rumen G. Bogdanovski <rumenastro@gmail.com>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_gp_guider.h>

#ifdef INDIGO_WINDOWS
#include <windows.h>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif
static int clock_gettime(int clk_id, struct timespec *tp) {
	(void)clk_id;
	FILETIME ft;
	ULARGE_INTEGER uli;
	GetSystemTimeAsFileTime(&ft);
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	const uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;
	uint64_t time_100ns = uli.QuadPart - EPOCH_DIFF_100NS;
	tp->tv_sec = (time_t)(time_100ns / 10000000ULL);
	tp->tv_nsec = (long)((time_100ns % 10000000ULL) * 100);
	return 0;
}
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- algorithm constants -------------------------- */

#define CIRCULAR_BUFFER_SIZE 8192 /* raw data storage */
#define REGULAR_BUFFER_SIZE 2048  /* regularized data storage */
#define FFT_SIZE 4096             /* zero-padding for the FFT (>= REGULAR_BUFFER_SIZE) */
#define GRID_INTERVAL 5.0
#define MAX_DITHER_STEPS 60       /* maximum number of dither steps if dither settle down is not signalled */
#define DEFAULT_LEARNING_RATE 0.01
#define HYSTERESIS 0.1
#define JITTER 1e-6

/* Multi Kernel GP: number of additional periodic kernels beyond the worm.
   Each models one further gear stage - a strain-wave input stage, a belt, a
   transfer gear - whose period is incommensurate with the worm's, so their
   errors beat against each other instead of repeating on a common cycle.
 */
#define GP_EXTRA_STAGES 1

/* Indices into the natural hyperparameters: 7 base, then 3 per extra stage.

   The base set describes the three kernels that are summed to form the
   covariance: a long squared exponential (SE0K) for slow aperiodic wander, the
   periodic worm kernel (PK), and a short squared exponential (SE1K) for
   short-term wander. Only SE1K is dropped from the projection kernel, so it
   shapes the fit but never the prediction - it exists to absorb wander that
   would otherwise be misattributed to the worm.

   "Natural" means the units a person would use: seconds for times, pixels for
   amplitudes. set_gp_hyperparameters() converts them to the log-space vector
   the kernel actually evaluates, where a signal variance is the square of the
   amplitude below. Values in brackets are the defaults. */
enum {
	SE0K_LENGTH_SCALE = 0,    /* long SE:  correlation time of the slow wander, s [700] */
	SE0K_SIGNAL_VARIANCE,     /* long SE:  amplitude of that wander, px [20] */
	PK_LENGTH_SCALE,          /* worm:     finest detail resolved within one period, s [10]
	                                       - smaller carries more harmonics, so a sharper
	                                         non-sinusoidal error shape */
	PK_SIGNAL_VARIANCE,       /* worm:     amplitude of the periodic error, px [20] */
	SE1K_LENGTH_SCALE,        /* short SE: correlation time of seeing/short wander, s [25] */
	SE1K_SIGNAL_VARIANCE,     /* short SE: amplitude of that wander, px [10] */
	PK_PERIOD_LENGTH,         /* worm:     the period itself, s [300, then tracked by FFT] */
	NUM_BASE_HYPERPARAMETERS  /* count of the above; the extra stages follow */
};

/* Hyperparameters of extra periodic stage s (0-based), same three quantities
   as the worm's but for a further gear stage. Unlike the worm, the length
   scale is derived from the period rather than set independently, and the
   signal variance is driven by the evidence gate rather than being a fixed
   prior - see update_stage(). */
#define EXK_LENGTH_SCALE(s) (NUM_BASE_HYPERPARAMETERS + 3 * (s) + 0)    /* detail resolved within one period, s */
#define EXK_SIGNAL_VARIANCE(s) (NUM_BASE_HYPERPARAMETERS + 3 * (s) + 1) /* amplitude, px; 0 when the gate is shut */
#define EXK_PERIOD_LENGTH(s) (NUM_BASE_HYPERPARAMETERS + 3 * (s) + 2)   /* the stage's period, s */

#define NUM_HYPERPARAMETERS (NUM_BASE_HYPERPARAMETERS + 3 * GP_EXTRA_STAGES)

/* Default hyperparameters and tuning */
#define DEFAULT_CONTROL_GAIN 0.6
#define DEFAULT_PREDICTION_GAIN 0.5
#define DEFAULT_MIN_MOVE 0.2
#define DEFAULT_PERIODS_FOR_INFERENCE 2.0
#define DEFAULT_PERIODS_FOR_PERIOD_ESTIMATION 2.0
#define DEFAULT_POINTS_FOR_APPROXIMATION 100
#define DEFAULT_COMPUTE_PERIOD true

/* Learning-progress reporting: when auto period estimation is on, the model is
   only as good as its period estimate. period_disagreement (smoothed relative
   error between the slowly-tracked period and the per-frame FFT estimate) maps
   to a 0..1 convergence factor: fully converged at/below CONVERGED_REL, not
   converged at/above DIVERGED_REL, linear in between. */
#define PERIOD_DISAGREEMENT_SMOOTHING 0.1
#define PERIOD_CONVERGED_REL 0.03
#define PERIOD_DIVERGED_REL 0.45

/* Seed-anchored period estimation (estimate_period_length):
   PERIOD_SEED_WINDOW      - half-width of the search band around the seed, as a
                             fraction (0.30 -> +/-30%); a harmonic or sub-harmonic
                             falls outside it and can never be locked onto.
   PERIOD_MAIN_PEAK_FRACTION - the in-band peak is trusted directly once it reaches
                             this fraction of the global peak; below it the
                             fundamental is reconstructed as 2 x the harmonic period. */
#define PERIOD_SEED_WINDOW 0.30
#define PERIOD_MAIN_PEAK_FRACTION 0.90

#define DEFAULT_LS_SE0 700.0
#define DEFAULT_SV_SE0 20.0
#define DEFAULT_LS_PK 10.0
#define DEFAULT_PERIOD_PK 300.0
#define DEFAULT_SV_PK 20.0
#define DEFAULT_LS_SE1 25.0
#define DEFAULT_SV_SE1 10.0

/* ---- extra periodic stages (Multi Kernel GP) -------------------------- */

#define DEFAULT_PERIOD_EXK 60.0
#define DEFAULT_SV_EXK 20.0

/* An extra stage's length scale is a fraction of its own period, not an
   absolute time: a fixed 10 s is a thirteenth of a 130 s period but nearly a
   quarter of a 47 s one, and a periodic kernel can only carry harmonics finer
   than its length scale. Tying the two together keeps the harmonic capacity
   the same whatever period the stage turns out to have - without this the
   stage is fitted as a bare sinusoid and most of the benefit is lost. */
#define EXK_LS_PERIOD_RATIO 13.0

/* Squared-exponential envelope on the extra periodic term, in units of its own
   period, making it quasi-periodic rather than strictly periodic.

   A strictly periodic kernel is coherent across the whole inference window. A
   window holding 115 cycles of a 47 s stage turns a 0.2% period error into a
   quarter cycle of accumulated phase error, which is enough to undo the term
   entirely; the same error on the 130 s worm is three times smaller because
   fewer cycles fit. The envelope limits how far back the prediction draws, so
   phase error stops accumulating. Measured: 20 periods costs ~0.01 px at the
   exact period and makes the residual flat in period error out past 2%. */
#define EXK_DECAY_PERIODS 20.0

/* Evidence gate. The extra term's signal variance is scaled by the smoothed
   AMPLITUDE of its spectral line relative to the worm's - compute_spectrum()
   returns power, so the estimator square-roots it. Below OFF the stage is
   switched out entirely, so a mount with a single periodic component behaves
   exactly as it did before this kernel existed. */
#define EXK_STRENGTH_SMOOTHING 0.05
#define EXK_STRENGTH_OFF 0.15
#define EXK_STRENGTH_FULL 0.35

/* A candidate period commensurate with an already-modelled one is a harmonic
   that kernel already represents through its length scale. Admitting it makes
   the two periodic terms degenerate: they explain the same signal, the split
   of variance between them is arbitrary, the Gram matrix conditioning
   degrades, and the two period trackers fight each other. */
#define EXK_COMMENSURATE_TOL 0.06
#define EXK_COMMENSURATE_MAX_K 5

/* Width, relative and in frequency, of the notch placed over each
   already-modelled line and its harmonics before the next line is searched
   for. */
#define EXK_NOTCH_REL 0.08

/* Bounds on an unseeded candidate. Anything slower than the worm is not a gear
   stage - it is drift and slow wander, already carried by the long SE kernel
   and the explicit linear trend - and letting the search reach down there
   hands the periodic kernel a 300 s "period" that is only the residual of an
   imperfect de-trend. The floor keeps the candidate resolvable on the
   regularisation grid. */
#define EXK_MAX_PERIOD_VS_PK 1.2
#define EXK_MIN_PERIOD (4.0 * GRID_INTERVAL)

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

/* LDLT (square-root free Cholesky) factorisation of a symmetric matrix:
     A = L * D * L^T,  L unit lower-triangular, D diagonal.
   L is stored full (only the strict lower part is meaningful, unit diagonal
   implied), D as a vector. Returns false if a pivot is non-positive (matrix
   not positive definite even after the jitter that callers add). */
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

/* Solve A * X = B for X, where A = L D L^T was factorised by ldlt_factor.
   B and X are n x m (m right-hand sides), X may alias B. */
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

/* Power spectrum of a real signal, mirroring math_tools::compute_spectrum.
   Allocates and returns spectrum[] and frequencies[] (caller frees), sets
   *out_len. Returns false on allocation failure. */
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

/* Evaluate the covariance K[i][j] = k(x_i, y_j) into out (nx x ny).
   log_hyper is the GP log-space vector of length NUM_HYPERPARAMETERS + 1:
     [log_noise, log(ls0), log(sv0), log(lsP), log(svP), log(ls1), log(sv1),
      log(period), then log(lsEx), log(svEx), log(periodEx) per extra stage]
   projection == true uses PeriodicSquareExponential (long SE + periodic),
   projection == false uses PeriodicSquareExponential2 (long SE + periodic + short SE).

   The extra periodic stages stay in BOTH branches: only the short SE is dropped
   from the projection, and an extra gear stage is predictable error, not
   short-term wander, so it must survive into the prediction.

   Distances are computed directly as (x_i - y_j)^2; the original mean-centering
   in math_tools::squareDistance is purely for numerical conditioning of the
   binomial expansion and is unnecessary (and slightly less accurate) here. */
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

	/* extra periodic stages, hoisted out of the inner loop */
	double ex_ls[GP_EXTRA_STAGES], ex_sv[GP_EXTRA_STAGES];
	double ex_pl[GP_EXTRA_STAGES], ex_inv_decay[GP_EXTRA_STAGES];
	bool ex_active[GP_EXTRA_STAGES];
	for (int s = 0; s < GP_EXTRA_STAGES; s++) {
		ex_ls[s] = exp(log_hyper[EXK_LENGTH_SCALE(s) + 1]);
		ex_sv[s] = exp(2.0 * log_hyper[EXK_SIGNAL_VARIANCE(s) + 1]);
		ex_pl[s] = exp(log_hyper[EXK_PERIOD_LENGTH(s) + 1]);
		double ld = EXK_DECAY_PERIODS * ex_pl[s];
		ex_inv_decay[s] = -0.5 / (ld * ld);
		/* the evidence gate drives the signal variance to (effectively) zero
		   when the stage is not present; skip the term entirely then */
		ex_active[s] = ex_sv[s] > 1e-12;
	}

	for (int i = 0; i < nx; i++) {
		for (int j = 0; j < ny; j++) {
			double d = x[i] - y[j];
			double d2 = d * d;
			double ad = sqrt(d2);
			double s = sin(M_PI / pl_p * ad) / ls_p;
			double k = sv_se0 * exp(inv_se0 * d2) + sv_p * exp(-2.0 * s * s);
			for (int e = 0; e < GP_EXTRA_STAGES; e++) {
				if (!ex_active[e]) {
					continue;
				}
				double se = sin(M_PI / ex_pl[e] * ad) / ex_ls[e];
				/* quasi-periodic: periodic core under a decay envelope */
				k += ex_sv[e] * exp(-2.0 * se * se) * exp(ex_inv_decay[e] * d2);
			}
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

/* Runtime state of one periodic stage. Stage 0 is the worm; stages 1.. are the
   further gear stages of the Multi Kernel GP. They are handled uniformly here
   even though the worm keeps two privileges: it is always enabled (it is the
   model's backbone, and its period sets the data ramp and the FFT threshold),
   and it is never gated, so its weight is always 1. */
typedef struct {
	bool enabled;             /* caller asked for this stage to be modelled */
	bool compute_period;      /* track the period online, or hold the commanded one */
	double commanded_period;  /* seed; <= 0 means "find it from the spectrum" */
	double disagreement;      /* smoothed |tracked - FFT estimate| / FFT estimate */
	double strength;          /* smoothed amplitude of its line / the worm's */
} gp_stage;

#define GP_STAGES (1 + GP_EXTRA_STAGES)

/* Hyperparameter indices for a stage. The worm's three live in the base block
   and are not contiguous, so they are mapped rather than indexed. */
static int stage_ls_index(int stage) {
	return stage == 0 ? PK_LENGTH_SCALE : EXK_LENGTH_SCALE(stage - 1);
}

static int stage_sv_index(int stage) {
	return stage == 0 ? PK_SIGNAL_VARIANCE : EXK_SIGNAL_VARIANCE(stage - 1);
}

static int stage_period_index(int stage) {
	return stage == 0 ? PK_PERIOD_LENGTH : EXK_PERIOD_LENGTH(stage - 1);
}

struct indigo_gp_guider {
	/* tuning */
	double control_gain;
	double prediction_gain;
	double min_move;
	double min_periods_for_inference;
	double min_periods_for_period_estimation;
	int points_for_approximation;
	double learning_rate;

	/* periodic stages: [0] is the worm, [1..] the further gear stages */
	gp_stage stage[GP_STAGES];

	/* GP log-space hyperparameters (length NUM_HYPERPARAMETERS + 1) */
	double log_hyper[NUM_HYPERPARAMETERS + 1];

	/* timing */
	double start_time;
	double last_time;

	double prediction;
	double last_prediction_end;
	double learning_progress;    /* 0..1 warm-up ramp, cached for the public getter */
	double time_origin;          /* gear-time offset subtracted before GP inference; matches gp_data_loc */

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
   guide cycles are seconds apart while a test loop runs in microseconds. */
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
	for (int s = 0; s < GP_EXTRA_STAGES; s++) {
		if (h[EXK_LENGTH_SCALE(s)] < 1.0) {
			h[EXK_LENGTH_SCALE(s)] = 1.0;
		}
	}

	/* convert periodic length-scales from natural units to standard notation */
	h[PK_LENGTH_SCALE] = 4.0 * sin(h[PK_LENGTH_SCALE] * M_PI / h[PK_PERIOD_LENGTH]);
	for (int s = 0; s < GP_EXTRA_STAGES; s++) {
		h[EXK_LENGTH_SCALE(s)] = 4.0 * sin(h[EXK_LENGTH_SCALE(s)] * M_PI / h[EXK_PERIOD_LENGTH(s)]);
	}

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
	/* convert periodic length-scales from standard notation back to natural units */
	h[PK_LENGTH_SCALE] = asin(h[PK_LENGTH_SCALE] / 4.0) * h[PK_PERIOD_LENGTH] / M_PI;
	for (int s = 0; s < GP_EXTRA_STAGES; s++) {
		h[EXK_LENGTH_SCALE(s)] = asin(h[EXK_LENGTH_SCALE(s)] / 4.0) * h[EXK_PERIOD_LENGTH(s)] / M_PI;
	}
	memcpy(natural, h, sizeof(h));
}

static double get_stage_period(const indigo_gp_guider *g, int stage) {
	return exp(g->log_hyper[stage_period_index(stage) + 1]);
}

/* the worm period, which the data ramp and the FFT threshold are keyed on */
static double get_period_length(const indigo_gp_guider *g) {
	return get_stage_period(g, 0);
}

/* A stage's 0..1 evidence gate. The worm is never gated - it is the model's
   backbone, not a candidate - so it always reads 1. */
static double stage_gate(const indigo_gp_guider *g, int stage) {
	if (stage == 0) {
		return 1.0;
	}
	double gate = (g->stage[stage].strength - EXK_STRENGTH_OFF) / (EXK_STRENGTH_FULL - EXK_STRENGTH_OFF);
	if (gate < 0.0) {
		gate = 0.0;
	}
	if (gate > 1.0) {
		gate = 1.0;
	}
	return gate;
}

/* Track one stage's period, and for the extra stages scale the signal variance
   by the evidence gate. period_length NaN keeps the current estimate, which is
   what a pinned period wants: only the gate still responds.

   The two kinds of stage differ in two details the caller should not have to
   care about. The worm's signal variance is a fixed prior and its length scale
   is an absolute time, both inherited from the single-kernel model; an extra
   stage is gated, and its length scale follows its own period so that its
   harmonic capacity does not depend on how fast the stage happens to be. */
static void update_stage(indigo_gp_guider *g, int stage, double period_length) {
	double h[NUM_HYPERPARAMETERS];
	get_gp_hyperparameters(g, h);
	int pi = stage_period_index(stage);
	if (is_nan(period_length)) {
		period_length = h[pi];
	}
	h[pi] = (1.0 - g->learning_rate) * h[pi] + g->learning_rate * period_length;
	if (stage > 0) {
		h[stage_ls_index(stage)] = h[pi] / EXK_LS_PERIOD_RATIO;
		h[stage_sv_index(stage)] = stage_gate(g, stage) * DEFAULT_SV_EXK;
	}
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

/* GP inference on a subset of n most-relevant points (GP::inferSD + GP::infer).
   data_loc/data_out/data_var hold N points; the n points with the highest
   covariance to prediction_point are selected. Builds the Gram matrix, its LDLT
   factorisation, alpha = K^-1 y, and the explicit linear-trend basis (beta). */
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
	   feature_vectors (2 x nn): row0 = 1, row1 = loc
	   feature_matrix  (2 x 2)  = FV * K^-1 * FV^T
	   beta = feature_matrix^-1 * (FV * alpha) */
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

/* Projected GP prediction of the mean at the given locations (GP::predict with
   the projection kernel and explicit trend). Writes mean[] (length nloc). */
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
	   R = phi - feature_vectors * gamma  (2 x nloc)
	   m += R^T * beta */
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

static double estimate_period_length(const double *time, const double *data, int n, double seed);
static double estimate_extra_period(const double *time, const double *data, int n, const double *known, int n_known, double seed, double *strength_out);

/* regularize_dataset: resample the irregular (timestamp, gear_error, variance)
   samples onto a fixed GRID_INTERVAL grid by trapezoidal averaging, mirroring
   GaussianProcessGuider::regularize_dataset.

   Outputs reg_time/reg_gear/reg_var (caller-allocated, capacity >= grid_size)
   and returns the number of grid cells produced, or -1 on overrun while
   dithering. */
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

/* UpdateGP: assemble the data, regularize, detrend, optionally estimate the
   period, and run the GP inference. Returns false on failure (e.g. dithering
   over-run) so the caller can recover. */
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

	/* Bound the inference to the most recent window and rebase its start to 0. */
	double window = (double)(REGULAR_BUFFER_SIZE - 1) * GRID_INTERVAL;
	double cutoff = timestamps[count - 1] - window;
	int start = 0;
	while (start < count - 1 && timestamps[start] < cutoff) {
		start++;
	}
	double origin = timestamps[start];
	double *ts = timestamps + start;
	double *ge = gear_error + start;
	double *vr = variances + start;
	int n = count - start;
	for (int i = 0; i < n; i++) {
		ts[i] -= origin;
	}
	prediction_point -= origin;

	/* regularize onto the fixed grid (now bounded to <= REGULAR_BUFFER_SIZE cells) */
	int grid_size = (int)ceil(ts[n - 1] / GRID_INTERVAL) + 1;
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
	int T = regularize_dataset(g, ts, ge, vr, n, rt, rg, rv, grid_size);
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
	   FM (2 x T): row0 = 1, row1 = time
	   weights = (FM FM^T + 1e-3 I)^-1 (FM gear) */
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

	/* Period estimation, one pass over the stages. A stage that is not tracking
	   its period still needs the spectrum when it is gated - the gate has to
	   measure how strong its line is before the kernel may use it - so the
	   spectrum is computed whenever any stage wants either. */
	double period_length = get_period_length(g);
	bool want_spectrum = false;
	for (int s = 0; s < GP_STAGES; s++) {
		if (g->stage[s].enabled && (g->stage[s].compute_period || s > 0)) {
			want_spectrum = true;
			break;
		}
	}
	if (want_spectrum && cb_last(g)->timestamp > g->min_periods_for_period_estimation * period_length) {
		double *detrend = (double *)malloc((size_t)T * sizeof(double));
		if (detrend) {
			for (int i = 0; i < T; i++) {
				detrend[i] = rg[i] - (w0 + w1 * rt[i]);
			}

			/* Stages in order. Each extra stage's search notches out the worm
			   and every stage already resolved, so a stage can never lock onto
			   a line an earlier one is already carrying. */
			double known[GP_STAGES];
			int n_known = 0;
			for (int s = 0; s < GP_STAGES; s++) {
				if (!g->stage[s].enabled) {
					continue;
				}
				/* A pinned worm needs no estimate at all. A pinned extra stage
				   still does, because its gate reads the line strength off the
				   same search. */
				if (!g->stage[s].compute_period && s == 0) {
					known[n_known++] = get_stage_period(g, s);
					continue;
				}
				double pl;
				if (s == 0) {
					/* the worm: anchor the peak search to the commanded period
					   (NaN/0 for auto -> no anchoring, global peak) */
					pl = estimate_period_length(rt, detrend, T, g->stage[0].commanded_period);
				} else {
					double strength = 0.0;
					pl = estimate_extra_period(rt, detrend, T, known, n_known, g->stage[s].commanded_period, &strength);
					/* the strength ramps toward the measured ratio, and decays
					   back toward zero when no admissible line is present */
					g->stage[s].strength = (1.0 - EXK_STRENGTH_SMOOTHING) * g->stage[s].strength + EXK_STRENGTH_SMOOTHING * strength;
				}
				if (!is_nan(pl) && pl > 0.0) {
					/* how far the tracked period still is from what the FFT sees */
					double rel = fabs(get_stage_period(g, s) - pl) / pl;
					g->stage[s].disagreement = (1.0 - PERIOD_DISAGREEMENT_SMOOTHING) * g->stage[s].disagreement + PERIOD_DISAGREEMENT_SMOOTHING * rel;
				}
				/* when the caller pinned this stage's period, hold it; for an
				   extra stage the gate still responds to the spectrum */
				update_stage(g, s, g->stage[s].compute_period ? pl : NAN);
				known[n_known++] = get_stage_period(g, s);
			}
			free(detrend);
		}
	}

	/* GP inference uses the (non-detrended) gear error; the explicit trend
	   basis inside the GP handles the linear component. The cached model now
	   lives in rebased time, so commit the matching origin for predict. */
	g->time_origin = origin;
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

	/* The cached GP lives in rebased time (see update_gp); map the absolute
	   prediction locations into the same frame. last_prediction_end stays
	   absolute. */
	double rebased[2];
	rebased[0] = next_location[0] - g->time_origin;
	rebased[1] = next_location[1] - g->time_origin;

	double prediction[2];
	gp_predict_projected(g, rebased, 2, prediction);

	g->last_prediction_end = next_location[1];
	return prediction[1] - prediction[0];
}

/* Parabolic (quadratic) interpolation of the spectral peak at bin idx; returns the
   refined frequency, falling back to the bin frequency at the edges or when the
   three samples are degenerate. */
static double interp_peak_frequency(const double *spectrum, const double *frequencies, int len, int idx) {
	double max_frequency = frequencies[idx];
	if (idx <= 0 || idx >= len - 1) {
		return max_frequency;
	}
	double spread = fabs(frequencies[idx - 1] - frequencies[idx + 1]);
	double interp_loc[3], interp_dat[3];
	interp_loc[0] = (frequencies[idx - 1] - max_frequency) / spread;
	interp_loc[1] = 0.0;
	interp_loc[2] = (frequencies[idx + 1] - max_frequency) / spread;
	double amax = spectrum[idx];
	interp_dat[0] = spectrum[idx - 1] / amax;
	interp_dat[1] = 1.0;
	interp_dat[2] = spectrum[idx + 1] / amax;
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
		return max_frequency;
	}
	double phi[9];
	for (int c = 0; c < 3; c++) {
		phi[0 * 3 + c] = interp_loc[c] * interp_loc[c];
		phi[1 * 3 + c] = interp_loc[c];
		phi[2 * 3 + c] = 1.0;
	}
	double M[9];
	mat_mul_bt(phi, phi, M, 3, 3, 3);
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
	return max_frequency;
}

/* EstimatePeriodLength: Hamming window -> power spectrum -> peak with quadratic
   interpolation, mirroring GaussianProcessGuider::EstimatePeriodLength.
   When seed > 0 the peak search is confined to +/-30% of the seed so a harmonic
   (near half the seed) or sub-harmonic (near twice it) can never win. In addition,
   while the in-band line is still weak (< 90% of the global peak) but the global
   peak is a harmonic whose fundamental - twice its period - lands in the band, the
   estimate is driven to that reconstructed fundamental (2 x the harmonic period),
   which is a sharper, more stable measurement than the weak in-band peak. Once the
   in-band line strengthens (>= 90% of the global peak) the direct in-band peak is
   used. With seed <= 0 (auto) the global peak is used. */
static double estimate_period_length(const double *time, const double *data, int n, double seed) {
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

	double result;
	if (seed > 0.0) {
		/* strongest peak within +/-PERIOD_SEED_WINDOW of the seed */
		double lo = (1.0 - PERIOD_SEED_WINDOW) * seed, hi = (1.0 + PERIOD_SEED_WINDOW) * seed;
		int local_index = -1;
		for (int i = 0; i < len; i++) {
			if (frequencies[i] <= 0.0) {
				continue;
			}
			double period = 1.0 / frequencies[i];
			if (period >= lo && period <= hi && (local_index < 0 || spectrum[i] > spectrum[local_index])) {
				local_index = i;
			}
		}
		if (local_index < 0) {
			/* nothing in the band: keep the current period */
			free(spectrum);
			free(frequencies);
			return NAN;
		}
		double p_local = 1.0 / interp_peak_frequency(spectrum, frequencies, len, local_index);
		if (spectrum[local_index] >= PERIOD_MAIN_PEAK_FRACTION * spectrum[max_index]) {
			/* the in-band line is strong: trust the direct measurement */
			result = p_local;
		} else {
			/* in-band line still weak; if the global peak is a harmonic whose
			   fundamental (twice its period) lands in the band, drift to that
			   reconstructed fundamental instead - it is sharper and steadier. */
			double p_fund = 2.0 / interp_peak_frequency(spectrum, frequencies, len, max_index);
			result = (p_fund >= lo && p_fund <= hi) ? p_fund : p_local;
		}
	} else {
		result = 1.0 / interp_peak_frequency(spectrum, frequencies, len, max_index);
	}

	free(spectrum);
	free(frequencies);
	return result;
}

/* Estimate the period of a further gear stage (Multi Kernel GP).

   known[0..n_known-1] holds the periods already modelled - the worm first,
   then any extra stages already found. Those lines and their harmonics are
   notched out of the spectrum, and the strongest survivor is taken as the
   candidate. With seed > 0 the search is confined to a band around it; with
   seed <= 0 it is confined to the range a gear stage can plausibly occupy,
   faster than the worm and slower than the regularisation grid.

   *strength_out receives the candidate's AMPLITUDE relative to the worm's line
   (compute_spectrum returns power, hence the square root), which drives the
   evidence gate. Returns NAN when there is no usable line.

   Note there is deliberately no "reconstruct the fundamental from its
   harmonic" rule here, unlike estimate_period_length(). It was implemented and
   measured: it cost more in the ordinary cases - false positives off the
   leakage skirt of the notched worm line, which are real local peaks and
   recur in bursts, so neither a peak test nor a persistence vote excludes them
   - than it gained on a stage whose harmonic dominates. Seeding the period is
   worth far more than reconstructing it. */
static double estimate_extra_period(const double *time, const double *data, int n, const double *known, int n_known, double seed, double *strength_out) {
	*strength_out = 0.0;
	if (n < 2 || n_known < 1 || !(known[0] > 0.0)) {
		return NAN;
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
	for (int i = 0; i < len; i++) {
		double period = (frequencies[i] > 0.0) ? 1.0 / frequencies[i] : 1e30;
		if (period > 1500.0) {
			spectrum[i] = 0.0;
		}
	}

	/* power of the worm's line, the reference for the strength ratio */
	double f_worm = 1.0 / known[0];
	double p_worm = 0.0;
	for (int i = 0; i < len; i++) {
		if (fabs(frequencies[i] - f_worm) <= EXK_NOTCH_REL * f_worm && spectrum[i] > p_worm) {
			p_worm = spectrum[i];
		}
	}

	/* notch every modelled line, its harmonics and its first sub-harmonics */
	for (int i = 0; i < len; i++) {
		double f = frequencies[i];
		if (f <= 0.0) {
			spectrum[i] = 0.0;
			continue;
		}
		for (int m = 0; m < n_known && spectrum[i] > 0.0; m++) {
			if (!(known[m] > 0.0)) {
				continue;
			}
			double fm = 1.0 / known[m];
			for (int k = 1; k <= EXK_COMMENSURATE_MAX_K; k++) {
				double fk = k * fm;
				if (fabs(f - fk) <= EXK_NOTCH_REL * fk) {
					spectrum[i] = 0.0;
					break;
				}
			}
			for (int k = 2; k <= 3 && spectrum[i] > 0.0; k++) {
				double fk = fm / k;
				if (fabs(f - fk) <= EXK_NOTCH_REL * fk) {
					spectrum[i] = 0.0;
					break;
				}
			}
		}
	}

	double lo, hi;
	if (seed > 0.0) {
		lo = (1.0 - PERIOD_SEED_WINDOW) * seed;
		hi = (1.0 + PERIOD_SEED_WINDOW) * seed;
	} else {
		lo = EXK_MIN_PERIOD;
		hi = EXK_MAX_PERIOD_VS_PK * known[0];
	}

	int best = -1;
	for (int i = 0; i < len; i++) {
		if (frequencies[i] <= 0.0 || spectrum[i] <= 0.0) {
			continue;
		}
		double period = 1.0 / frequencies[i];
		if (period < lo || period > hi) {
			continue;
		}
		if (best < 0 || spectrum[i] > spectrum[best]) {
			best = i;
		}
	}
	if (best < 0 || p_worm <= 0.0) {
		free(spectrum);
		free(frequencies);
		return NAN;
	}

	double period = 1.0 / interp_peak_frequency(spectrum, frequencies, len, best);
	double strength = sqrt(spectrum[best] / p_worm);

	free(spectrum);
	free(frequencies);

	if (is_nan(period) || !(period > 0.0)) {
		return NAN;
	}

	/* reject anything commensurate with a line we already model */
	for (int m = 0; m < n_known; m++) {
		if (!(known[m] > 0.0)) {
			continue;
		}
		for (int k = 1; k <= EXK_COMMENSURATE_MAX_K; k++) {
			if (fabs(known[m] / (k * period) - 1.0) < EXK_COMMENSURATE_TOL) {
				return NAN;
			}
			if (fabs(period / (k * known[m]) - 1.0) < EXK_COMMENSURATE_TOL) {
				return NAN;
			}
		}
	}

	*strength_out = strength;
	return period;
}

/* Learning progress 0..1, cached for the public getter. Must be called while
   cb_last() still holds the current frame (before cb_push()).

   Two factors, averaged with equal weight:
     data_ramp  - fraction of the inference window observed; the same warm-up
                  ramp that gp_result() uses to blend in the GP prediction.
     period_ok  - when auto period estimation is on, how well the tracked period
                  agrees with the FFT estimate.
   The data ramp alone reads "done" while the periodic kernel still fits the
   wrong period, so it is only half the score; the other half tracks the period
   actually converging. */
/* Map a smoothed period disagreement to a 0..1 convergence factor: fully
   converged at/below CONVERGED_REL, not converged at/above DIVERGED_REL,
   linear in between. */
static double period_convergence(double disagreement) {
	if (disagreement <= PERIOD_CONVERGED_REL) {
		return 1.0;
	}
	if (disagreement >= PERIOD_DIVERGED_REL) {
		return 0.0;
	}
	return (PERIOD_DIVERGED_REL - disagreement) / (PERIOD_DIVERGED_REL - PERIOD_CONVERGED_REL);
}

/* Convergence of every stage that is both tracking its period and carrying
   real weight, taken as the worst of them: the prediction is only as good as
   the least settled period it rests on. Returns -1 when no stage is tracking,
   so the caller can treat convergence as not applicable. */
static double stages_period_convergence(const indigo_gp_guider *g) {
	double worst = -1.0;
	for (int s = 0; s < GP_STAGES; s++) {
		if (!g->stage[s].enabled || !g->stage[s].compute_period) {
			continue;
		}
		/* an extra stage that the gate has switched out is not load-bearing */
		if (s > 0 && g->stage[s].strength <= EXK_STRENGTH_OFF) {
			continue;
		}
		double ok = period_convergence(g->stage[s].disagreement);
		if (worst < 0.0 || ok < worst) {
			worst = ok;
		}
	}
	return worst;
}

static double compute_learning_progress(indigo_gp_guider *g) {
	if (g->buf_size <= 10) {
		return 0.0;
	}
	double period_length = get_period_length(g);
	if (period_length <= 0.0) {
		return 0.0;
	}
	double data_ramp = cb_last(g)->timestamp / (g->min_periods_for_inference * period_length);
	if (data_ramp < 0.0) {
		data_ramp = 0.0;
	}
	if (data_ramp > 1.0) {
		data_ramp = 1.0;
	}
	double period_ok = stages_period_convergence(g);
	if (period_ok < 0.0) {
		return data_ramp; /* nothing is tracking: convergence is not a factor */
	}
	return 0.5 * data_ramp + 0.5 * period_ok;
}

/* Weight (0..1) for the predictive vs reactive/hysteresis term in gp_result().
   Product of data ramp and period convergence, so prediction is trusted only with
   BOTH enough data AND a converged period - unlike compute_learning_progress(),
   which averages them for display. This keeps the first worm cycle from acting on
   a prediction built on the not-yet-estimated seed period. For a fixed period the
   period factor is 1, so the weight is just the data ramp.
   Call while cb_last() still holds the current frame. */
static double compute_blend_weight(indigo_gp_guider *g) {
	if (g->buf_size <= 10) {
		return 0.0;
	}
	double period_length = get_period_length(g);
	if (period_length <= 0.0) {
		return 0.0;
	}
	double data_ramp = cb_last(g)->timestamp / (g->min_periods_for_inference * period_length);
	if (data_ramp < 0.0) {
		data_ramp = 0.0;
	}
	if (data_ramp > 1.0) {
		data_ramp = 1.0;
	}
	double period_ok = stages_period_convergence(g);
	if (period_ok < 0.0) {
		period_ok = 1.0; /* pinned periods are trusted as given */
	}
	return data_ramp * period_ok;
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

	g->learning_progress = compute_learning_progress(g);

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
		   guiding to keep the GP/FFT consistent, but apply plain proportional
		   control. update_gp() degrades gracefully on a regularize over-run
		   (keeps the previous GP), so no recovery is needed here. */
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

		/* Blend prediction in only as far as the model is trustworthy; stays
		   reactive through the first worm cycle until the period settles. */
		double blend = compute_blend_weight(g);
		hyst_percentage = 1.0 - blend;
		control_signal = blend * control_signal + (1.0 - blend) * hysteresis_control;
	}

	if (is_nan(control_signal)) {
		control_signal = hysteresis_control;
	}

	g->learning_progress = compute_learning_progress(g);

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
	g->learning_rate = DEFAULT_LEARNING_RATE;

	double natural[NUM_HYPERPARAMETERS];
	natural[SE0K_LENGTH_SCALE] = DEFAULT_LS_SE0;
	natural[SE0K_SIGNAL_VARIANCE] = DEFAULT_SV_SE0;
	natural[PK_LENGTH_SCALE] = DEFAULT_LS_PK;
	natural[PK_SIGNAL_VARIANCE] = DEFAULT_SV_PK;
	natural[SE1K_LENGTH_SCALE] = DEFAULT_LS_SE1;
	natural[SE1K_SIGNAL_VARIANCE] = DEFAULT_SV_SE1;
	natural[PK_PERIOD_LENGTH] = DEFAULT_PERIOD_PK;
	for (int s = 0; s < GP_EXTRA_STAGES; s++) {
		natural[EXK_PERIOD_LENGTH(s)] = DEFAULT_PERIOD_EXK;
		natural[EXK_LENGTH_SCALE(s)] = DEFAULT_PERIOD_EXK / EXK_LS_PERIOD_RATIO;
		natural[EXK_SIGNAL_VARIANCE(s)] = 0.0; /* gated off until the evidence arrives */
	}

	for (int s = 0; s < GP_STAGES; s++) {
		/* the worm is the model's backbone and is always modelled; the further
		   stages are off until the caller asks for them */
		g->stage[s].enabled = (s == 0);
		g->stage[s].compute_period = DEFAULT_COMPUTE_PERIOD;
		/* NaN so the next set_stage() re-pins the caller's commanded period.
		   apply_defaults() runs on create and on reset_model(), which is
		   exactly when the period must be re-applied after being returned to
		   the default. */
		g->stage[s].commanded_period = NAN;
		g->stage[s].strength = (s == 0) ? 1.0 : 0.0;
		g->stage[s].disagreement = 1.0;
	}
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
	g->learning_progress = 0.0;
	/* fully diverged until the first FFT estimate */
	for (int s = 0; s < GP_STAGES; s++) {
		g->stage[s].disagreement = 1.0;
		g->stage[s].strength = (s == 0) ? 1.0 : 0.0;
	}
	g->time_origin = 0.0;
	g->dither_offset = 0.0;
	g->dither_steps = 0;
	g->dithering_active = false;
}

void indigo_gp_guider_reset_model(indigo_gp_guider *g) {
	if (!g) {
		return;
	}
	/* full reset to the prior, including the learned worm period (unlike
	   indigo_gp_guider_reset(), which keeps it). User tuning set via
	   indigo_gp_guider_set_parameters() is re-applied by the caller. */
	apply_defaults(g);
	indigo_gp_guider_reset(g);
	/* drop the cross-session retain state so the next session starts clean */
	g->prev_ra = NAN;
	g->prev_sop = 0;
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
		   worm having moved backwards, so reset in that case */
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
		   frames, exactly like settling after a dither */
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

void indigo_gp_guider_set_parameters(indigo_gp_guider *g, double control_gain, double prediction_gain, double min_move) {
	if (!g) {
		return;
	}
	g->control_gain = control_gain;
	g->prediction_gain = prediction_gain;
	g->min_move = min_move;
}

void indigo_gp_guider_set_stage(indigo_gp_guider *g, int stage, bool enabled, bool compute_period, double period_length) {
	if (!g || stage < 0 || stage >= GP_STAGES) {
		return;
	}
	gp_stage *e = &g->stage[stage];
	/* the worm is the model's backbone; it cannot be switched off */
	if (stage == 0) {
		enabled = true;
	}
	e->compute_period = compute_period;
	/* Seed the period only when the commanded value changes. Re-sending the
	   same value must leave the period free to drift under the estimator
	   rather than re-pinning it every frame, so a caller may safely re-apply
	   its settings each cycle. commanded_period is NaN after create or
	   reset_model, so the first call always pins. */
	if (enabled == e->enabled && period_length == e->commanded_period) {
		return;
	}
	e->enabled = enabled;
	e->commanded_period = period_length;

	double natural[NUM_HYPERPARAMETERS];
	get_gp_hyperparameters(g, natural);
	double fallback = (stage == 0) ? DEFAULT_PERIOD_PK : DEFAULT_PERIOD_EXK;
	natural[stage_period_index(stage)] = (period_length > 0.0) ? period_length : fallback;
	if (stage > 0) {
		natural[stage_ls_index(stage)] = natural[stage_period_index(stage)] / EXK_LS_PERIOD_RATIO;
		if (!enabled) {
			natural[stage_sv_index(stage)] = 0.0;
			e->strength = 0.0;
			e->disagreement = 1.0;
		}
	}
	set_gp_hyperparameters(g, natural);
}

int indigo_gp_guider_get_stage_count(void) {
	return GP_STAGES;
}

double indigo_gp_guider_get_stage_period(const indigo_gp_guider *g, int stage) {
	if (!g || stage < 0 || stage >= GP_STAGES) {
		return 0.0;
	}
	return get_stage_period(g, stage);
}

double indigo_gp_guider_get_stage_weight(const indigo_gp_guider *g, int stage) {
	if (!g || stage < 0 || stage >= GP_STAGES || !g->stage[stage].enabled) {
		return 0.0;
	}
	return stage_gate(g, stage);
}

double indigo_gp_guider_get_learning_progress(const indigo_gp_guider *g) {
	if (!g) {
		return 0.0;
	}
	/* Cached during the last gp_result()/gp_deduce_result() while the current
	   frame was still the buffer's last point. Reading the buffer here would be
	   wrong, because those functions push an empty (timestamp 0) point before
	   returning. */
	return g->learning_progress;
}

double indigo_gp_guider_response(indigo_gp_guider *g, double drift, double snr, double time_step) {
	if (!g) {
		return 0.0;
	}
	if (snr <= 0.0) {
		snr = 10.0; /* reasonable default when SNR is unknown */
	}
	double control_signal = gp_result(g, drift, snr, time_step, -1.0);
	/* This is a correction so negate the control signal */
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
