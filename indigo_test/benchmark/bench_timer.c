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

// version history
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

// Per-fire latency and jitter benchmark for indigo_set_timer() and
// indigo_reschedule_timer().
//
// This is a measurement tool, not a pass/fail test. It always exits 0 and is
// deliberately kept out of the `test` target: the numbers depend on machine
// load and on kernel timer behavior, so they are meaningful only when compared
// against another run on the same machine.
//
// Usage:
//   build/benchmark/bench_timer [label [runs [one_shots [repeat_fires [burst]]]]]
//
//   label         free-form name printed in the header, e.g. a branch name
//   runs          repetitions of the whole scenario set (default 5)
//   one_shots     one-shot timers per run (default 300, 20 discarded as warm-up)
//   repeat_fires  fires of the self-rescheduling timer per run (default 300)
//   burst         timers scheduled for the same deadline per run (default 200)
//
// Reported scenarios, all in microseconds:
//
//   one-shot 5ms       lateness of a single timer against its requested deadline
//   one-shot 0ms       the same with no delay, so it isolates dispatch cost
//   repeat 10ms        lateness of a timer that reschedules itself from its own
//                      callback, the usual INDIGO periodic pattern
//   repeat 10ms        interval between consecutive fires of that timer, which
//     fire interval    shows jitter directly against the 10000us target
//   burst 50ms         lateness when many timers come due at the same instant
//
// Lateness is measured with a monotonic clock read inside this program, so it
// does not depend on which clock the library uses internally.
//
// The benchmark uses only timer API that has been stable across INDIGO
// versions, so the same source can be built against two different libraries to
// compare implementations. To A/B a branch, build libindigo in a second
// worktree and point the include and library paths at it.

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <indigo/indigo_driver.h>
#include <indigo/indigo_timer.h>

#if defined(CLOCK_MONOTONIC)
#define BENCH_CLOCK CLOCK_MONOTONIC
#else
#define BENCH_CLOCK CLOCK_REALTIME
#endif

#define MAX_BURST 512
#define MAX_RUNS 64
#define MAX_SAMPLES 100000
#define WARMUP 20

static int64_t mono_ns(void) {
	struct timespec ts;
	clock_gettime(BENCH_CLOCK, &ts);
	return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// ---------------------------------------------------------------- statistics

typedef struct {
	double *v;
	size_t n;
	size_t cap;
} stats;

static void st_init(stats *s, size_t cap) {
	s->v = indigo_safe_malloc(cap * sizeof(double));
	s->n = 0;
	s->cap = cap;
}

static void st_add(stats *s, double x) {
	if (s->n < s->cap) {
		s->v[s->n++] = x;
	}
}

static double st_mean(stats *s, size_t from) {
	if (s->n <= from) {
		return 0.0;
	}
	double sum = 0;
	for (size_t i = from; i < s->n; i++) {
		sum += s->v[i];
	}
	return sum / (s->n - from);
}

static int compare_double(const void *a, const void *b) {
	double x = *(const double *)a, y = *(const double *)b;
	return (x > y) - (x < y);
}

static void st_report(const char *label, stats *s) {
	if (s->n == 0) {
		printf("%-34s      - no samples\n", label);
		return;
	}
	double *v = indigo_safe_malloc(s->n * sizeof(double));
	memcpy(v, s->v, s->n * sizeof(double));
	qsort(v, s->n, sizeof(double), compare_double);
	double mean = st_mean(s, 0);
	double variance = 0;
	for (size_t i = 0; i < s->n; i++) {
		double d = v[i] - mean;
		variance += d * d;
	}
	double deviation = s->n > 1 ? sqrt(variance / (s->n - 1)) : 0.0;
	printf("%-34s %6zu %9.1f %9.1f %9.1f %9.1f %9.1f %9.1f %9.1f\n",
		label, s->n, v[0], mean, v[s->n / 2], v[(size_t)(s->n * 0.95)],
		v[(size_t)(s->n * 0.99)], v[s->n - 1], deviation);
	indigo_safe_free(v);
}

// ------------------------------------------------------------ shared harness

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int64_t expected_ns;
static bool fired;

static stats one_shot_delayed, one_shot_immediate, repeat_lateness, repeat_interval, burst_lateness;

// Wait until the library has cleared the caller's reference so that the next
// iteration starts from a quiescent state.
static void wait_for_slot(indigo_timer **slot) {
	for (int i = 0; i < 200000 && *slot != NULL; i++) {
		indigo_usleep(50);
	}
}

// ------------------------------------------------------------- one-shot fire

static stats *one_shot_target;

static void one_shot_callback(indigo_device *device) {
	int64_t at = mono_ns();
	(void)device;
	pthread_mutex_lock(&lock);
	st_add(one_shot_target, (double)(at - expected_ns) / 1000.0);
	fired = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&lock);
}

static void benchmark_one_shot(indigo_device *device, double delay, int iterations, stats *target) {
	for (int i = 0; i < iterations + WARMUP; i++) {
		indigo_timer *slot = NULL;
		size_t before = target->n;
		pthread_mutex_lock(&lock);
		fired = false;
		one_shot_target = target;
		expected_ns = mono_ns() + (int64_t)(delay * 1e9);
		bool scheduled = indigo_set_timer(device, delay, one_shot_callback, &slot);
		if (!scheduled) {
			pthread_mutex_unlock(&lock);
			fprintf(stderr, "indigo_set_timer() failed\n");
			exit(1);
		}
		while (!fired) {
			pthread_cond_wait(&cond, &lock);
		}
		pthread_mutex_unlock(&lock);
		if (i < WARMUP) {
			target->n = before;
		}
		wait_for_slot(&slot);
	}
}

// ------------------------------------------ repeating timer (self-reschedule)

static indigo_timer *repeat_slot = NULL;
static int repeat_remaining;
static int repeat_warmup;
static double repeat_period;
static bool repeat_done;
static bool repeat_failed;
static int64_t repeat_previous_fire;

static void repeat_callback(indigo_device *device) {
	int64_t at = mono_ns();
	pthread_mutex_lock(&lock);
	if (repeat_warmup > 0) {
		repeat_warmup--;
	} else {
		st_add(&repeat_lateness, (double)(at - expected_ns) / 1000.0);
		if (repeat_previous_fire != 0) {
			st_add(&repeat_interval, (double)(at - repeat_previous_fire) / 1000.0);
		}
	}
	repeat_previous_fire = at;
	if (--repeat_remaining > 0) {
		expected_ns = mono_ns() + (int64_t)(repeat_period * 1e9);
		if (!indigo_reschedule_timer(device, repeat_period, &repeat_slot)) {
			repeat_failed = true;
			repeat_done = true;
			pthread_cond_signal(&cond);
		}
	} else {
		repeat_done = true;
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&lock);
}

static void benchmark_repeat(indigo_device *device, double period, int fires) {
	pthread_mutex_lock(&lock);
	repeat_slot = NULL;
	repeat_period = period;
	repeat_remaining = fires + WARMUP;
	repeat_warmup = WARMUP;
	repeat_done = false;
	repeat_failed = false;
	repeat_previous_fire = 0;
	expected_ns = mono_ns() + (int64_t)(period * 1e9);
	if (!indigo_set_timer(device, period, repeat_callback, &repeat_slot)) {
		pthread_mutex_unlock(&lock);
		fprintf(stderr, "indigo_set_timer() failed\n");
		exit(1);
	}
	while (!repeat_done) {
		pthread_cond_wait(&cond, &lock);
	}
	bool failed = repeat_failed;
	pthread_mutex_unlock(&lock);
	if (failed) {
		fprintf(stderr, "indigo_reschedule_timer() failed\n");
		exit(1);
	}
	wait_for_slot(&repeat_slot);
}

// ------------------------------------------------------------- burst dispatch

static int64_t burst_expected_ns[MAX_BURST];
static indigo_timer *burst_slots[MAX_BURST];
static int burst_fired;

static void burst_callback(indigo_device *device, void *data) {
	int64_t at = mono_ns();
	(void)device;
	size_t index = (size_t)(intptr_t)data;
	pthread_mutex_lock(&lock);
	st_add(&burst_lateness, (double)(at - burst_expected_ns[index]) / 1000.0);
	burst_fired++;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&lock);
}

static void benchmark_burst(indigo_device *device, int count, double delay) {
	memset(burst_slots, 0, sizeof(burst_slots));
	pthread_mutex_lock(&lock);
	burst_fired = 0;
	pthread_mutex_unlock(&lock);
	for (int i = 0; i < count; i++) {
		burst_expected_ns[i] = mono_ns() + (int64_t)(delay * 1e9);
		if (!indigo_set_timer_with_data(device, delay, burst_callback, &burst_slots[i], (void *)(intptr_t)i)) {
			fprintf(stderr, "indigo_set_timer_with_data() failed\n");
			exit(1);
		}
	}
	pthread_mutex_lock(&lock);
	while (burst_fired < count) {
		pthread_cond_wait(&cond, &lock);
	}
	pthread_mutex_unlock(&lock);
	for (int i = 0; i < count; i++) {
		wait_for_slot(&burst_slots[i]);
	}
}

// ---------------------------------------------------------------------- main

int main(int argc, char **argv) {
	const char *label = argc > 1 ? argv[1] : "current tree";
	int runs = argc > 2 ? atoi(argv[2]) : 5;
	int one_shots = argc > 3 ? atoi(argv[3]) : 300;
	int repeat_fires = argc > 4 ? atoi(argv[4]) : 300;
	int burst_count = argc > 5 ? atoi(argv[5]) : 200;
	if (runs < 1 || runs > MAX_RUNS) {
		runs = 5;
	}
	if (burst_count > MAX_BURST) {
		burst_count = MAX_BURST;
	}

	indigo_set_log_level(INDIGO_LOG_ERROR);

	indigo_device_context context;
	memset(&context, 0, sizeof(context));
	pthread_mutex_init(&context.device_mutex, NULL);
	indigo_device device;
	memset(&device, 0, sizeof(device));
	strncpy(device.name, "Benchmark Device", INDIGO_NAME_SIZE - 1);
	device.device_context = &context;

	st_init(&one_shot_delayed, MAX_SAMPLES);
	st_init(&one_shot_immediate, MAX_SAMPLES);
	st_init(&repeat_lateness, MAX_SAMPLES);
	st_init(&repeat_interval, MAX_SAMPLES);
	st_init(&burst_lateness, MAX_SAMPLES);

	printf("# %s: %d runs, %d one-shot timers, %d repeat fires, %d burst timers per run\n",
		label, runs, one_shots, repeat_fires, burst_count);

	double run_mean_one_shot[MAX_RUNS], run_mean_repeat[MAX_RUNS];
	for (int run = 0; run < runs; run++) {
		size_t before = one_shot_delayed.n;
		benchmark_one_shot(&device, 0.005, one_shots, &one_shot_delayed);
		run_mean_one_shot[run] = st_mean(&one_shot_delayed, before);

		benchmark_one_shot(&device, 0.0, one_shots, &one_shot_immediate);

		before = repeat_lateness.n;
		benchmark_repeat(&device, 0.010, repeat_fires);
		run_mean_repeat[run] = st_mean(&repeat_lateness, before);

		benchmark_burst(&device, burst_count, 0.050);
		fprintf(stderr, "run %d of %d done\n", run + 1, runs);
	}

	printf("\n%-34s %6s %9s %9s %9s %9s %9s %9s %9s\n",
		"scenario (microseconds)", "n", "min", "mean", "median", "p95", "p99", "max", "stddev");
	st_report("one-shot 5ms: lateness", &one_shot_delayed);
	st_report("one-shot 0ms: dispatch cost", &one_shot_immediate);
	st_report("repeat 10ms: lateness", &repeat_lateness);
	st_report("repeat 10ms: fire interval", &repeat_interval);
	st_report("burst 50ms: lateness", &burst_lateness);

	printf("\nper-run mean, one-shot 5ms:");
	for (int run = 0; run < runs; run++) {
		printf(" %.1f", run_mean_one_shot[run]);
	}
	printf("\nper-run mean, repeat 10ms: ");
	for (int run = 0; run < runs; run++) {
		printf(" %.1f", run_mean_repeat[run]);
	}
	printf("\n");

	indigo_safe_free(one_shot_delayed.v);
	indigo_safe_free(one_shot_immediate.v);
	indigo_safe_free(repeat_lateness.v);
	indigo_safe_free(repeat_interval.v);
	indigo_safe_free(burst_lateness.v);
	pthread_mutex_destroy(&context.device_mutex);
	return 0;
}
