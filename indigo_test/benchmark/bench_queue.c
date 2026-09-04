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

// Per-task latency, jitter and throughput benchmark for handler queues.
//
// This is a measurement tool, not a pass/fail test. It always exits 0 and is
// deliberately kept out of the `test` target: the numbers depend on machine
// load and on kernel timer behavior, so they are meaningful only when compared
// against another run on the same machine.
//
// Usage:
//   build/benchmark/bench_queue [label [runs [tasks [periodic_fires [batch]]]]]
//
//   label           free-form name printed in the header, e.g. a branch name
//   runs            repetitions of the whole scenario set (default 5)
//   tasks           tasks per latency scenario (default 200, 20 discarded as warm-up)
//   periodic_fires  fires of the self-requeueing task per run (default 200)
//   batch           tasks per drained batch (default 200)
//
// Reported scenarios, all in microseconds:
//
//   add call cost      time spent inside indigo_queue_add() with a backlog of
//                      pending tasks, so ordered insertion is exercised
//   queue 0ms          time from indigo_queue_add() to callback entry on an idle
//     dispatch         queue, which isolates per-task dispatch cost
//   queue 5ms          lateness of a delayed task against its requested deadline
//   queue 10ms         lateness of a task that re-adds itself from its own
//     periodic         callback, the usual periodic handler-queue pattern
//   queue 10ms         interval between consecutive fires of that task, which
//     fire interval    shows jitter directly against the 10000us target
//   drain              microseconds per task when a batch of ready tasks is
//                      enqueued at once and run to completion
//   drain under        the same batch while another thread continuously creates
//     timer churn      and cancels timers, which exposes any lock shared between
//                      the timer and queue subsystems
//
// Timings come from a monotonic clock read inside this program, so they do not
// depend on which clock the library uses internally.
//
// The benchmark uses only queue API that has been stable across INDIGO
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

#define MAX_RUNS 64
#define MAX_SAMPLES 100000
#define WARMUP 20
#define BACKLOG_DEPTH 200
#define DRAINS_PER_RUN 10
#define FAR_FUTURE 3600.0

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

static indigo_queue *queue;
static int64_t expected_ns;
static bool fired;
static int completed;

static stats add_cost, dispatch, delayed, periodic_lateness, periodic_interval, drain, drain_under_churn;

static void noop_callback(indigo_device *device) {
	(void)device;
}

// ------------------------------------------------------------- add call cost

static void benchmark_add_cost(indigo_device *device, int tasks) {
	for (int i = 0; i < BACKLOG_DEPTH; i++) {
		indigo_queue_add(queue, device, INDIGO_TASK_PRIORITY_NORMAL, FAR_FUTURE, noop_callback, NULL);
	}
	for (int i = 0; i < tasks; i++) {
		int64_t before = mono_ns();
		indigo_queue_add(queue, device, INDIGO_TASK_PRIORITY_NORMAL, FAR_FUTURE, noop_callback, NULL);
		int64_t after = mono_ns();
		st_add(&add_cost, (double)(after - before) / 1000.0);
	}
	indigo_queue_remove(queue, NULL, NULL);
}

// -------------------------------------------------------- single task timing

static stats *single_target;

static void single_callback(indigo_device *device) {
	int64_t at = mono_ns();
	(void)device;
	pthread_mutex_lock(&lock);
	st_add(single_target, (double)(at - expected_ns) / 1000.0);
	fired = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&lock);
}

static void benchmark_single(indigo_device *device, double delay, int tasks, stats *target) {
	for (int i = 0; i < tasks + WARMUP; i++) {
		size_t before = target->n;
		pthread_mutex_lock(&lock);
		fired = false;
		single_target = target;
		expected_ns = mono_ns() + (int64_t)(delay * 1e9);
		indigo_queue_add(queue, device, INDIGO_TASK_PRIORITY_NORMAL, delay, single_callback, NULL);
		while (!fired) {
			pthread_cond_wait(&cond, &lock);
		}
		if (i < WARMUP) {
			target->n = before;
		}
		pthread_mutex_unlock(&lock);
	}
}

// ------------------------------------------------ periodic task (self-requeue)

static int periodic_remaining;
static int periodic_warmup;
static double periodic_period;
static bool periodic_done;
static int64_t periodic_previous_fire;

static void periodic_callback(indigo_device *device) {
	int64_t at = mono_ns();
	pthread_mutex_lock(&lock);
	if (periodic_warmup > 0) {
		periodic_warmup--;
	} else {
		st_add(&periodic_lateness, (double)(at - expected_ns) / 1000.0);
		if (periodic_previous_fire != 0) {
			st_add(&periodic_interval, (double)(at - periodic_previous_fire) / 1000.0);
		}
	}
	periodic_previous_fire = at;
	if (--periodic_remaining > 0) {
		expected_ns = mono_ns() + (int64_t)(periodic_period * 1e9);
		indigo_queue_add(queue, device, INDIGO_TASK_PRIORITY_NORMAL, periodic_period, periodic_callback, NULL);
	} else {
		periodic_done = true;
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&lock);
}

static void benchmark_periodic(indigo_device *device, double period, int fires) {
	pthread_mutex_lock(&lock);
	periodic_period = period;
	periodic_remaining = fires + WARMUP;
	periodic_warmup = WARMUP;
	periodic_done = false;
	periodic_previous_fire = 0;
	expected_ns = mono_ns() + (int64_t)(period * 1e9);
	indigo_queue_add(queue, device, INDIGO_TASK_PRIORITY_NORMAL, period, periodic_callback, NULL);
	while (!periodic_done) {
		pthread_cond_wait(&cond, &lock);
	}
	pthread_mutex_unlock(&lock);
}

// ----------------------------------------------------------- backlog drain

static void drain_callback(indigo_device *device) {
	(void)device;
	pthread_mutex_lock(&lock);
	completed++;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&lock);
}

static void benchmark_drain(indigo_device *device, int batch, stats *target) {
	pthread_mutex_lock(&lock);
	completed = 0;
	pthread_mutex_unlock(&lock);
	int64_t before = mono_ns();
	for (int i = 0; i < batch; i++) {
		indigo_queue_add(queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, drain_callback, NULL);
	}
	pthread_mutex_lock(&lock);
	while (completed < batch) {
		pthread_cond_wait(&cond, &lock);
	}
	pthread_mutex_unlock(&lock);
	int64_t after = mono_ns();
	st_add(target, (double)(after - before) / 1000.0 / batch);
}

// -------------------------------------------------------------- timer churn

static bool churn_running;

static void *churn_func(void *arg) {
	indigo_device *device = (indigo_device *)arg;
	indigo_rename_thread("Benchmark churn");
	while (true) {
		pthread_mutex_lock(&lock);
		bool running = churn_running;
		pthread_mutex_unlock(&lock);
		if (!running) {
			break;
		}
		indigo_timer *timer = NULL;
		if (indigo_set_timer(device, FAR_FUTURE, noop_callback, &timer)) {
			indigo_cancel_timer(device, &timer);
		}
	}
	return NULL;
}

// ---------------------------------------------------------------------- main

int main(int argc, char **argv) {
	const char *label = argc > 1 ? argv[1] : "current tree";
	int runs = argc > 2 ? atoi(argv[2]) : 5;
	int tasks = argc > 3 ? atoi(argv[3]) : 200;
	int periodic_fires = argc > 4 ? atoi(argv[4]) : 200;
	int batch = argc > 5 ? atoi(argv[5]) : 200;
	if (runs < 1 || runs > MAX_RUNS) {
		runs = 5;
	}

	indigo_set_log_level(INDIGO_LOG_ERROR);

	indigo_device_context context;
	memset(&context, 0, sizeof(context));
	pthread_mutex_init(&context.device_mutex, NULL);
	indigo_device device;
	memset(&device, 0, sizeof(device));
	strncpy(device.name, "Benchmark Device", INDIGO_NAME_SIZE - 1);
	device.device_context = &context;

	queue = indigo_queue_create(&device);
	if (queue == NULL) {
		fprintf(stderr, "indigo_queue_create() failed\n");
		return 1;
	}

	st_init(&add_cost, MAX_SAMPLES);
	st_init(&dispatch, MAX_SAMPLES);
	st_init(&delayed, MAX_SAMPLES);
	st_init(&periodic_lateness, MAX_SAMPLES);
	st_init(&periodic_interval, MAX_SAMPLES);
	st_init(&drain, MAX_SAMPLES);
	st_init(&drain_under_churn, MAX_SAMPLES);

	printf("# %s: %d runs, %d tasks, %d periodic fires, %d tasks per drained batch\n",
		label, runs, tasks, periodic_fires, batch);

	double run_mean_dispatch[MAX_RUNS], run_mean_periodic[MAX_RUNS];
	for (int run = 0; run < runs; run++) {
		benchmark_add_cost(&device, tasks);

		size_t before = dispatch.n;
		benchmark_single(&device, 0.0, tasks, &dispatch);
		run_mean_dispatch[run] = st_mean(&dispatch, before);

		benchmark_single(&device, 0.005, tasks, &delayed);

		before = periodic_lateness.n;
		benchmark_periodic(&device, 0.010, periodic_fires);
		run_mean_periodic[run] = st_mean(&periodic_lateness, before);

		for (int i = 0; i < DRAINS_PER_RUN; i++) {
			benchmark_drain(&device, batch, &drain);
		}

		pthread_mutex_lock(&lock);
		churn_running = true;
		pthread_mutex_unlock(&lock);
		pthread_t churn_thread;
		bool churning = pthread_create(&churn_thread, NULL, churn_func, &device) == 0;
		for (int i = 0; i < DRAINS_PER_RUN; i++) {
			benchmark_drain(&device, batch, &drain_under_churn);
		}
		pthread_mutex_lock(&lock);
		churn_running = false;
		pthread_mutex_unlock(&lock);
		if (churning) {
			pthread_join(churn_thread, NULL);
		}

		fprintf(stderr, "run %d of %d done\n", run + 1, runs);
	}

	printf("\n%-34s %6s %9s %9s %9s %9s %9s %9s %9s\n",
		"scenario (microseconds)", "n", "min", "mean", "median", "p95", "p99", "max", "stddev");
	st_report("add call cost", &add_cost);
	st_report("queue 0ms: dispatch", &dispatch);
	st_report("queue 5ms: lateness", &delayed);
	st_report("queue 10ms: periodic lateness", &periodic_lateness);
	st_report("queue 10ms: fire interval", &periodic_interval);
	st_report("drain: per task", &drain);
	st_report("drain under timer churn: per task", &drain_under_churn);

	printf("\nper-run mean, queue 0ms dispatch:");
	for (int run = 0; run < runs; run++) {
		printf(" %.1f", run_mean_dispatch[run]);
	}
	printf("\nper-run mean, queue 10ms periodic:");
	for (int run = 0; run < runs; run++) {
		printf(" %.1f", run_mean_periodic[run]);
	}
	printf("\n");

	indigo_queue_delete(&queue);
	indigo_safe_free(add_cost.v);
	indigo_safe_free(dispatch.v);
	indigo_safe_free(delayed.v);
	indigo_safe_free(periodic_lateness.v);
	indigo_safe_free(periodic_interval.v);
	indigo_safe_free(drain.v);
	indigo_safe_free(drain_under_churn.v);
	pthread_mutex_destroy(&context.device_mutex);
	return 0;
}
