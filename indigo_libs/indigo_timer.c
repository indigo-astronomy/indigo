// Copyright (c) 2017-2026 CloudMakers, s. r. o.
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO timers
 \file indigo_timer.c
 */

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#if !defined(INDIGO_WINDOWS)
#include <unistd.h>
#endif

#include <indigo/indigo_timer.h>
#include <indigo/indigo_driver.h>

#define SEC_NS    1000000000LL       /* 1 sec in nanoseconds */

#if defined(INDIGO_WINDOWS)
#include <windows.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

static int clock_gettime(int clk_id, struct timespec* tp) {
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

#if defined(CLOCK_MONOTONIC)
#define INDIGO_TIMER_CLOCK CLOCK_MONOTONIC
#define INDIGO_TIMER_CLOCK_IS_MONOTONIC 1
#else
#define INDIGO_TIMER_CLOCK CLOCK_REALTIME
#define INDIGO_TIMER_CLOCK_IS_MONOTONIC 0
#endif

#if INDIGO_TIMER_CLOCK_IS_MONOTONIC
#define clock_time(ts) clock_gettime(CLOCK_MONOTONIC, ts)
#else
#define clock_time(ts) clock_gettime(CLOCK_REALTIME, ts)
#endif

typedef enum {
	INDIGO_TIMER_PENDING = 1,
	INDIGO_TIMER_RUNNING,
	INDIGO_TIMER_COMPLETED,
	INDIGO_TIMER_CANCELED
} indigo_timer_state;

typedef struct indigo_timer_worker {
	pthread_t thread;
	struct indigo_timer_worker *next;
} indigo_timer_worker;

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_once_t once;
	pthread_t thread;
	indigo_timer *pending;
	indigo_timer *timers;
	indigo_timer_worker *finished_workers;
#if !defined(INDIGO_WINDOWS)
	pid_t pid;
#endif
	bool thread_started;
	bool start_failed;
	bool ready;
} indigo_timer_scheduler;

// Protected by timer_scheduler.mutex.
static uint64_t next_timer_id = 0;
static indigo_timer_scheduler timer_scheduler = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.once = PTHREAD_ONCE_INIT,
	.pending = NULL,
	.timers = NULL,
	.finished_workers = NULL,
#if !defined(INDIGO_WINDOWS)
	.pid = 0,
#endif
	.thread_started = false,
	.start_failed = false,
	.ready = false
};

static void *timer_scheduler_func(void *arg);
static void *timer_callback_func(void *arg);
static bool scheduler_insert_pending_locked(indigo_timer *timer);
static bool scheduler_remove_pending_locked(indigo_timer *timer);
static void complete_timer_locked(indigo_timer *timer, indigo_timer_state state);
static void free_timer_if_unused_locked(indigo_timer *timer);
static bool start_timer_scheduler_locked(void);
static void enqueue_finished_worker_locked(pthread_t thread);
static void reap_finished_workers_locked(void);
static bool reschedule_timer_with_callback_locked(double delay, indigo_timer_with_data_callback callback, indigo_timer **timer);

static inline int timespec_cmp(const struct timespec *a, const struct timespec *b) {
	if (a->tv_sec < b->tv_sec) return -1;
	if (a->tv_sec > b->tv_sec) return 1;
	if (a->tv_nsec < b->tv_nsec) return -1;
	if (a->tv_nsec > b->tv_nsec) return 1;
	return 0;
}

static struct timespec normalize_timespec(struct timespec time) {
	while (time.tv_nsec >= SEC_NS) {
		time.tv_nsec -= SEC_NS;
		time.tv_sec++;
	}
	while (time.tv_nsec < 0) {
		time.tv_nsec += SEC_NS;
		time.tv_sec--;
	}
	return time;
}

static void init_timer_cond(pthread_cond_t *cond) {
#if defined(INDIGO_LINUX) && INDIGO_TIMER_CLOCK_IS_MONOTONIC
	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);
	pthread_condattr_setclock(&condattr, INDIGO_TIMER_CLOCK);
	pthread_cond_init(cond, &condattr);
	pthread_condattr_destroy(&condattr);
#else
	pthread_cond_init(cond, NULL);
#endif
}

static int timer_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *at) {
#if defined(INDIGO_MACOS) && INDIGO_TIMER_CLOCK_IS_MONOTONIC
	struct timespec now;
	clock_time(&now);
	struct timespec relative = {
		.tv_sec = at->tv_sec - now.tv_sec,
		.tv_nsec = at->tv_nsec - now.tv_nsec
	};
	relative = normalize_timespec(relative);
	if (relative.tv_sec < 0) {
		relative.tv_sec = 0;
		relative.tv_nsec = 0;
	}
	return pthread_cond_timedwait_relative_np(cond, mutex, &relative);
#else
	return pthread_cond_timedwait(cond, mutex, at);
#endif
}

static void init_timer_scheduler_once(void) {
	init_timer_cond(&timer_scheduler.cond);
	timer_scheduler.pending = NULL;
	timer_scheduler.timers = NULL;
	timer_scheduler.finished_workers = NULL;
	timer_scheduler.ready = false;
	timer_scheduler.start_failed = false;
#if !defined(INDIGO_WINDOWS)
	timer_scheduler.pid = getpid();
#endif
	pthread_mutex_lock(&timer_scheduler.mutex);
	start_timer_scheduler_locked();
	pthread_mutex_unlock(&timer_scheduler.mutex);
}

static bool ensure_timer_scheduler(void) {
	pthread_once(&timer_scheduler.once, init_timer_scheduler_once);
#if !defined(INDIGO_WINDOWS)
	if (timer_scheduler.pid != getpid()) {
		pthread_mutex_init(&timer_scheduler.mutex, NULL);
		init_timer_cond(&timer_scheduler.cond);
		timer_scheduler.pending = NULL;
		timer_scheduler.timers = NULL;
		timer_scheduler.finished_workers = NULL;
		timer_scheduler.thread_started = false;
		timer_scheduler.start_failed = false;
		timer_scheduler.ready = false;
		timer_scheduler.pid = getpid();
		pthread_mutex_lock(&timer_scheduler.mutex);
		start_timer_scheduler_locked();
		pthread_mutex_unlock(&timer_scheduler.mutex);
	}
#endif
	return timer_scheduler.thread_started && !timer_scheduler.start_failed;
}

static bool start_timer_scheduler_locked(void) {
	if (pthread_create(&timer_scheduler.thread, NULL, timer_scheduler_func, NULL) == 0) {
		timer_scheduler.thread_started = true;
		timer_scheduler.start_failed = false;
		while (!timer_scheduler.ready) {
			pthread_cond_wait(&timer_scheduler.cond, &timer_scheduler.mutex);
		}
		return true;
	} else {
		timer_scheduler.thread_started = false;
		timer_scheduler.start_failed = true;
		indigo_error("Failed to start timer scheduler thread");
		return false;
	}
}

static void *timer_scheduler_func(void *arg) {
	(void)arg;
	pthread_detach(pthread_self());
	indigo_rename_thread("Timer scheduler");
	pthread_mutex_lock(&timer_scheduler.mutex);
	timer_scheduler.ready = true;
	pthread_cond_broadcast(&timer_scheduler.cond);
	while (true) {
		reap_finished_workers_locked();
		if (timer_scheduler.pending == NULL) {
			pthread_cond_wait(&timer_scheduler.cond, &timer_scheduler.mutex);
			continue;
		}
		struct timespec now;
		clock_time(&now);
		if (timespec_cmp(&timer_scheduler.pending->at, &now) > 0) {
			timer_cond_timedwait(&timer_scheduler.cond, &timer_scheduler.mutex, &timer_scheduler.pending->at);
			continue;
		}
		indigo_timer *timer = timer_scheduler.pending;
		scheduler_remove_pending_locked(timer);
		timer->state = INDIGO_TIMER_RUNNING;
		timer->worker_started = true;
		if (pthread_create(&timer->callback_thread, NULL, timer_callback_func, timer) != 0) {
			indigo_error("Failed to start timer callback thread");
			timer->worker_started = false;
			complete_timer_locked(timer, INDIGO_TIMER_COMPLETED);
		}
	}
}

static void *timer_callback_func(void *arg) {
	indigo_timer *timer = (indigo_timer *)arg;
	indigo_rename_thread("Timer callback #%" PRIu64, timer->timer_id);
	// Scheduler mutex is not held while callbacks run, and scheduler code never locks timer_mutex.
	if (timer->timer_mutex) {
		pthread_mutex_lock(timer->timer_mutex);
	}
	if (timer->has_data) {
		((indigo_timer_with_data_callback)timer->callback)(timer->device, timer->timer_data);
	} else {
		((indigo_timer_callback)timer->callback)(timer->device);
	}
	if (timer->timer_mutex) {
		pthread_mutex_unlock(timer->timer_mutex);
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	enqueue_finished_worker_locked(pthread_self());
	if (timer->reschedule_requested && !timer->cancel_requested && !timer->canceled) {
		timer->state = INDIGO_TIMER_PENDING;
		timer->completed = false;
		timer->worker_started = false;
		timer->reschedule_requested = false;
		bool earliest_changed = scheduler_insert_pending_locked(timer);
		if (earliest_changed) {
			pthread_cond_signal(&timer_scheduler.cond);
		}
		pthread_cond_broadcast(&timer->completed_cond);
		pthread_cond_broadcast(&timer_scheduler.cond);
	} else {
		complete_timer_locked(timer, INDIGO_TIMER_COMPLETED);
	}
	pthread_mutex_unlock(&timer_scheduler.mutex);
	return NULL;
}

static bool timer_deadline_before(const indigo_timer *a, const indigo_timer *b) {
	int result = timespec_cmp(&a->at, &b->at);
	return result < 0 || (result == 0 && a->timer_id < b->timer_id);
}

static bool scheduler_insert_pending_locked(indigo_timer *timer) {
	bool earliest_changed = timer_scheduler.pending == NULL || timer_deadline_before(timer, timer_scheduler.pending);
	indigo_timer **link = &timer_scheduler.pending;
	while (*link != NULL && !timer_deadline_before(timer, *link)) {
		link = &(*link)->scheduler_next;
	}
	timer->scheduler_next = *link;
	*link = timer;
	return earliest_changed;
}

static bool scheduler_remove_pending_locked(indigo_timer *timer) {
	indigo_timer **link = &timer_scheduler.pending;
	while (*link != NULL) {
		if (*link == timer) {
			*link = timer->scheduler_next;
			timer->scheduler_next = NULL;
			return true;
		}
		link = &(*link)->scheduler_next;
	}
	return false;
}

static void unlink_device_timer_locked(indigo_timer *timer) {
	indigo_device *device = timer->device;
	if (device == NULL || DEVICE_CONTEXT == NULL) {
		timer->next = NULL;
		return;
	}
	indigo_timer **link = &DEVICE_CONTEXT->timers;
	while (*link != NULL) {
		if (*link == timer) {
			*link = timer->next;
			break;
		}
		link = &(*link)->next;
	}
	timer->next = NULL;
}

static void clear_timer_reference_locked(indigo_timer *timer) {
	if (timer->reference != NULL && *timer->reference == timer) {
		*timer->reference = NULL;
	}
	timer->reference = NULL;
}

static void register_timer_locked(indigo_timer *timer) {
	timer->registry_next = timer_scheduler.timers;
	timer_scheduler.timers = timer;
}

static bool unregister_timer_locked(indigo_timer *timer) {
	indigo_timer **link = &timer_scheduler.timers;
	while (*link != NULL) {
		if (*link == timer) {
			*link = timer->registry_next;
			timer->registry_next = NULL;
			return true;
		}
		link = &(*link)->registry_next;
	}
	return false;
}

static bool timer_is_live_locked(indigo_timer *timer) {
	for (indigo_timer *current = timer_scheduler.timers; current != NULL; current = current->registry_next) {
		if (current == timer) {
			return true;
		}
	}
	return false;
}

static bool timer_reference_is_valid_locked(indigo_timer **timer) {
	return timer != NULL && *timer != NULL && timer_is_live_locked(*timer) && (*timer)->reference == timer && *(*timer)->reference == *timer;
}

static void free_timer_if_unused_locked(indigo_timer *timer) {
	if (timer->completed && timer->waiters == 0 && unregister_timer_locked(timer)) {
		pthread_cond_destroy(&timer->completed_cond);
		indigo_safe_free(timer);
	}
}

static void enqueue_finished_worker_locked(pthread_t thread) {
	indigo_timer_worker *worker = indigo_safe_malloc(sizeof(indigo_timer_worker));
	worker->thread = thread;
	worker->next = timer_scheduler.finished_workers;
	timer_scheduler.finished_workers = worker;
}

static void reap_finished_workers_locked(void) {
	indigo_timer_worker *worker = timer_scheduler.finished_workers;
	timer_scheduler.finished_workers = NULL;
	while (worker != NULL) {
		indigo_timer_worker *next = worker->next;
		pthread_t thread = worker->thread;
		pthread_mutex_unlock(&timer_scheduler.mutex);
		pthread_join(thread, NULL);
		indigo_safe_free(worker);
		pthread_mutex_lock(&timer_scheduler.mutex);
		worker = next;
	}
}

static void complete_timer_locked(indigo_timer *timer, indigo_timer_state state) {
	unlink_device_timer_locked(timer);
	clear_timer_reference_locked(timer);
	timer->state = state;
	timer->canceled = state == INDIGO_TIMER_CANCELED;
	timer->completed = true;
	pthread_cond_broadcast(&timer->completed_cond);
	pthread_cond_broadcast(&timer_scheduler.cond);
	free_timer_if_unused_locked(timer);
}

static void init_timer_state_primitives(indigo_timer *timer) {
	init_timer_cond(&timer->completed_cond);
}

static void reset_timer_state(indigo_timer *timer, double delay) {
	timer->state = INDIGO_TIMER_PENDING;
	timer->at = indigo_delay_to_time(delay);
	timer->cancel_requested = false;
	timer->reschedule_requested = false;
	timer->completed = false;
	timer->worker_started = false;
	timer->waiters = 0;
	timer->callback_thread = (pthread_t)0;
	timer->scheduler_next = NULL;
	timer->registry_next = NULL;
}

static void prepare_timer_for_schedule(indigo_timer *timer, indigo_device *device, double delay, indigo_timer_with_data_callback callback, void *timer_data, bool has_data, pthread_mutex_t *timer_mutex) {
	timer->canceled = false;
	timer->device = device;
	timer->callback = callback;
	timer->timer_data = timer_data;
	timer->has_data = has_data;
	timer->timer_mutex = timer_mutex;
	reset_timer_state(timer, delay);
}

static void link_device_timer_locked(indigo_timer *timer) {
	indigo_device *device = timer->device;
	if (device != NULL && DEVICE_CONTEXT != NULL) {
		timer->next = DEVICE_CONTEXT->timers;
		DEVICE_CONTEXT->timers = timer;
	} else {
		timer->next = NULL;
	}
}

static indigo_timer *create_timer_object(indigo_device *device, double delay, indigo_timer_with_data_callback callback, void *timer_data, bool has_data, pthread_mutex_t *timer_mutex) {
	indigo_timer *timer = indigo_safe_malloc(sizeof(indigo_timer));
	timer->timer_id = next_timer_id++;
	init_timer_state_primitives(timer);
	timer->reference = NULL;
	timer->next = NULL;
	prepare_timer_for_schedule(timer, device, delay, callback, timer_data, has_data, timer_mutex);
	register_timer_locked(timer);
	return timer;
}

struct timespec indigo_delay_to_time(double delay) {
	struct timespec time = { 0, 0 };
	if (delay == 0) {
		return time;
	}
	clock_time(&time);
	time.tv_sec += (int)delay;
	time.tv_nsec += (long)(SEC_NS * (delay - (int)delay));
	return normalize_timespec(time);
}

static bool set_timer(indigo_device *device, double delay, indigo_timer_with_data_callback callback, indigo_timer **timer, void *timer_data, bool has_data, pthread_mutex_t *timer_mutex) {
	if (!ensure_timer_scheduler()) {
		indigo_error("Attempt to set timer without running scheduler");
		return false;
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	if (timer != NULL && *timer != NULL) {
		pthread_mutex_unlock(&timer_scheduler.mutex);
		indigo_error("Attempt to set timer with non-NULL reference");
		return false;
	}
	indigo_timer *t = create_timer_object(device, delay, callback, timer_data, has_data, timer_mutex);
	t->reference = timer;
	if (timer != NULL) {
		*timer = t;
	}
	link_device_timer_locked(t);
	bool earliest_changed = scheduler_insert_pending_locked(t);
	if (earliest_changed) {
		pthread_cond_signal(&timer_scheduler.cond);
	}
	pthread_mutex_unlock(&timer_scheduler.mutex);
	return true;
}

bool indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer) {
	return set_timer(device, delay, (indigo_timer_with_data_callback)callback, timer, NULL, false, NULL);
}

bool indigo_set_timer_with_data(indigo_device *device, double delay, indigo_timer_with_data_callback callback, indigo_timer **timer, void *timer_data) {
	return set_timer(device, delay, (indigo_timer_with_data_callback)callback, timer, timer_data, true, NULL);
}

bool indigo_set_timer_with_mutex(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer, pthread_mutex_t *timer_mutex) {
	return set_timer(device, delay, (indigo_timer_with_data_callback)callback, timer, NULL, false, timer_mutex);
}

static bool reschedule_timer_with_callback_locked(double delay, indigo_timer_with_data_callback callback, indigo_timer **timer) {
	bool result = false;
	if (timer == NULL || *timer == NULL) {
		indigo_error("Attempt to reschedule timer without reference!");
	} else if (!timer_reference_is_valid_locked(timer)) {
		if (timer_is_live_locked(*timer)) {
			indigo_error("timer #%" PRIu64 " - attempt to reschedule timer with outdated reference!", (*timer)->timer_id);
		} else {
			indigo_error("Attempt to reschedule timer with outdated reference!");
		}
	} else if ((*timer)->state == INDIGO_TIMER_PENDING && !(*timer)->canceled) {
		indigo_timer *t = *timer;
		scheduler_remove_pending_locked(t);
		t->at = indigo_delay_to_time(delay);
		t->callback = (indigo_timer_with_data_callback)callback;
		t->reschedule_requested = false;
		bool earliest_changed = scheduler_insert_pending_locked(t);
		if (earliest_changed) {
			pthread_cond_signal(&timer_scheduler.cond);
		}
		result = true;
	} else if ((*timer)->state == INDIGO_TIMER_RUNNING && !(*timer)->canceled && (*timer)->worker_started && pthread_equal(pthread_self(), (*timer)->callback_thread)) {
		indigo_timer *t = *timer;
		t->at = indigo_delay_to_time(delay);
		t->callback = (indigo_timer_with_data_callback)callback;
		t->reschedule_requested = true;
		result = true;
	} else {
		indigo_error("Attempt to reschedule timer without reference or canceled timer!");
	}
	return result;
}

bool indigo_reschedule_timer(indigo_device *device, double delay, indigo_timer **timer) {
	(void)device;
	bool result = false;
	if (!ensure_timer_scheduler()) {
		return false;
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	if (timer == NULL || *timer == NULL) {
		indigo_error("Attempt to reschedule timer without reference!");
	} else {
		result = reschedule_timer_with_callback_locked(delay, (indigo_timer_with_data_callback)(*timer)->callback, timer);
	}
	pthread_mutex_unlock(&timer_scheduler.mutex);
	return result;
}

bool indigo_reschedule_timer_with_callback(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer) {
	(void)device;
	if (!ensure_timer_scheduler()) {
		return false;
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	bool result = reschedule_timer_with_callback_locked(delay, (indigo_timer_with_data_callback)callback, timer);
	pthread_mutex_unlock(&timer_scheduler.mutex);
	return result;
}

bool indigo_cancel_timer(indigo_device *device, indigo_timer **timer) {
	(void)device;
	bool result = false;
	if (!ensure_timer_scheduler()) {
		return false;
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	if (timer == NULL || *timer == NULL) {
		result = false;
	} else if (!timer_reference_is_valid_locked(timer)) {
		if (timer_is_live_locked(*timer)) {
			indigo_error("timer #%" PRIu64 " - attempt to cancel timer with outdated reference!", (*timer)->timer_id);
		} else {
			indigo_error("Attempt to cancel timer with outdated reference!");
		}
	} else if ((*timer)->state == INDIGO_TIMER_PENDING) {
		scheduler_remove_pending_locked(*timer);
		complete_timer_locked(*timer, INDIGO_TIMER_CANCELED);
		result = true;
	} else if ((*timer)->state == INDIGO_TIMER_RUNNING) {
		(*timer)->cancel_requested = true;
		result = false;
	}
	pthread_mutex_unlock(&timer_scheduler.mutex);
	return result;
}

bool indigo_cancel_timer_sync(indigo_device *device, indigo_timer **timer) {
	(void)device;
	bool result = false;
	if (!ensure_timer_scheduler()) {
		return false;
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	if (timer == NULL || *timer == NULL) {
		result = false;
	} else if (!timer_reference_is_valid_locked(timer)) {
		indigo_error("Attempt to cancel timer with outdated reference!");
	} else {
		indigo_timer *t = *timer;
		if (t->state == INDIGO_TIMER_PENDING) {
			scheduler_remove_pending_locked(t);
			complete_timer_locked(t, INDIGO_TIMER_CANCELED);
			result = true;
		} else if (t->state == INDIGO_TIMER_RUNNING) {
			bool self_callback = t->worker_started && pthread_equal(pthread_self(), t->callback_thread);
			t->cancel_requested = true;
			t->reschedule_requested = false;
			result = true;
			if (self_callback) {
				unlink_device_timer_locked(t);
				clear_timer_reference_locked(t);
			} else {
				t->waiters++;
				while (!t->completed) {
					pthread_cond_wait(&t->completed_cond, &timer_scheduler.mutex);
				}
				t->waiters--;
				free_timer_if_unused_locked(t);
			}
		}
	}
	pthread_mutex_unlock(&timer_scheduler.mutex);
	return result;
}

void indigo_cancel_all_timers(indigo_device *device) {
	if (device == NULL || DEVICE_CONTEXT == NULL) {
		return;
	}
	if (!ensure_timer_scheduler()) {
		return;
	}
	pthread_mutex_lock(&timer_scheduler.mutex);
	size_t device_timer_count = 0;
	for (indigo_timer *timer = DEVICE_CONTEXT->timers; timer != NULL; timer = timer->next) {
		device_timer_count++;
	}
	// Cancel pending timers immediately, but wait only for callbacks that were already
	// running when this function was called. Timers created by those callbacks are left
	// to their owners instead of extending this cancel-all operation indefinitely.
	indigo_timer **running_timers = device_timer_count > 0 ? indigo_safe_malloc(device_timer_count * sizeof(indigo_timer *)) : NULL;
	size_t running_timer_count = 0;
	indigo_timer *timer = DEVICE_CONTEXT->timers;
	while (timer != NULL) {
		indigo_timer *next = timer->next;
		if (timer->state == INDIGO_TIMER_PENDING) {
			scheduler_remove_pending_locked(timer);
			complete_timer_locked(timer, INDIGO_TIMER_CANCELED);
		} else if (timer->state == INDIGO_TIMER_RUNNING) {
			bool self_callback = timer->worker_started && pthread_equal(pthread_self(), timer->callback_thread);
			timer->cancel_requested = true;
			timer->reschedule_requested = false;
			if (self_callback) {
				unlink_device_timer_locked(timer);
				clear_timer_reference_locked(timer);
			} else {
				timer->waiters++;
				running_timers[running_timer_count++] = timer;
			}
		}
		timer = next;
	}
	for (size_t i = 0; i < running_timer_count; i++) {
		indigo_timer *running_timer = running_timers[i];
		while (!running_timer->completed) {
			pthread_cond_wait(&running_timer->completed_cond, &timer_scheduler.mutex);
		}
		running_timer->waiters--;
		free_timer_if_unused_locked(running_timer);
	}
	pthread_mutex_unlock(&timer_scheduler.mutex);
	indigo_safe_free(running_timers);
}

// Peek the scheduled time of either the highest-priority runnable task or the first future task.
// If any runnable task exists, *at is <= now and the caller can dequeue work immediately.
// Otherwise *at is the earliest future deadline and the caller can timed-wait until then.
// The time is copied out while queue->mutex is held - returning the task itself would let
// indigo_queue_remove() free it before the caller dereferences it.
static bool peek_task_at_locked(indigo_queue *queue, const struct timespec *now, struct timespec *at) {
	indigo_queue_task *runnable_task = queue->task;
	indigo_queue_task *current = queue->task;
	int highest_priority = -1;
	while (current) {
		if (timespec_cmp(&current->at, now) > 0) {
			break;
		}
		if (current->priority > highest_priority) {
			highest_priority = current->priority;
			runnable_task = current;
		}
		current = current->next;
	}
	bool found = runnable_task != NULL;
	if (found) {
		*at = runnable_task->at;
	}
	return found;
}

// Dequeue the highest priority runnable task from the queue
static indigo_queue_task *dequeue_runnable_task_locked(indigo_queue *queue) {
	struct timespec now;
	clock_time(&now);
	indigo_queue_task *runnable_task = NULL;
	indigo_queue_task *prev_to_runnable_task = NULL;
	indigo_queue_task *current = queue->task;
	indigo_queue_task *prev = NULL;
	int highest_priority = -1;
	while (current) {
		if (timespec_cmp(&current->at, &now) > 0) {
			break;
		}
		if (current->priority > highest_priority) {
			highest_priority = current->priority;
			runnable_task = current;
			prev_to_runnable_task = prev;
		}
		prev = current;
		current = current->next;
	}
	if (runnable_task) {
		if (prev_to_runnable_task) {
			prev_to_runnable_task->next = runnable_task->next;
		} else {
			queue->task = runnable_task->next;
		}
	}
	return runnable_task;
}

static bool task_matches(indigo_queue_task *task, indigo_device *device, indigo_timer_callback callback) {
	return task != NULL && (device == NULL || task->device == device) && (callback == NULL || task->callback == callback);
}

// Remove tasks matching device or device and callback
static void remove_tasks_locked(indigo_queue *queue, indigo_device *device, indigo_timer_callback callback) {
	indigo_queue_task **link = &queue->task;
	while (*link) {
		indigo_queue_task *task = *link;
		if (task_matches(task, device, callback)) {
			*link = task->next;
			indigo_safe_free(task);
		} else {
			link = &task->next;
		}
	}
}

// Enqueue the highest priority runnable task from the queue ignoring priority
static void enqueue_task_locked(indigo_queue *queue, indigo_queue_task *task) {
	if (queue->task == NULL || timespec_cmp(&task->at, &queue->task->at) < 0) {
		task->next = queue->task;
		queue->task = task;
	} else {
		indigo_queue_task *next = queue->task;
		while (next->next != NULL && timespec_cmp(&next->next->at, &task->at) <= 0) {
			next = next->next;
		}
		task->next = next->next;
		next->next = task;
	}
}

static double task_delay(indigo_queue_task *task) {
	struct timespec now;
	clock_time(&now);
	double delay = 0.0;
	if (task->at.tv_sec != 0 || task->at.tv_nsec != 0) {
		delay = (now.tv_sec - task->at.tv_sec) + (now.tv_nsec - task->at.tv_nsec) / 1e9;
	}
	return delay;
}

// wrapper for executing queue task = scheduled call of handler
static void *queue_func(indigo_queue *queue) {
	indigo_rename_thread("Queue %s", queue->device->name);
	// wakeup indigo_queue_create, ready flag avoids a lost wakeup if the signal arrives before the wait
	pthread_mutex_lock(&queue->mutex);
	queue->ready = true;
	pthread_cond_signal(&queue->cond);
	pthread_mutex_unlock(&queue->mutex);
	// main loop waiting for wakeup signal or timeout
	pthread_mutex_lock(&queue->mutex);
	while (true) {
		struct timespec now;
		clock_time(&now);
		if (queue->abort) { // abort is a predicate of cond, it has to be checked with queue->mutex held, otherwise the wakeup signaled by indigo_queue_delete may be lost and the wait never returns
			break;
		}
		struct timespec at;
		if (!peek_task_at_locked(queue, &now, &at)) { // no task, wait for wakeup
			pthread_cond_wait(&queue->cond, &queue->mutex);
			continue;
		} else if (timespec_cmp(&at, &now) > 0) { // wait for timeout for the next task
			timer_cond_timedwait(&queue->cond, &queue->mutex, &at);
			continue;
		}
		pthread_mutex_unlock(&queue->mutex);
		while (true) {
			pthread_mutex_lock(&queue->mutex);
			if (queue->abort) {
				pthread_mutex_unlock(&queue->mutex);
				break;
			}
			indigo_queue_task *runnable_task = dequeue_runnable_task_locked(queue);
			if (!runnable_task) {
				pthread_mutex_unlock(&queue->mutex);
				break; // no ready tasks, exit inner loop and go to sleep
			}
			queue->running_task = runnable_task;
			queue->running = true;
			pthread_mutex_unlock(&queue->mutex);

			// Calculate execution delay from scheduled time only if debug logging is enabled
			if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
				double delay = task_delay(runnable_task);
				INDIGO_TRACE(indigo_trace("Executing task %p: priority %d, delay %.6fs", runnable_task->callback, runnable_task->priority, delay));
			}

			if (runnable_task->task_mutex) { // if there is specific mutex for task, lock it
				pthread_mutex_lock(runnable_task->task_mutex);
			}
			if (runnable_task->has_data) {
				((indigo_timer_with_data_callback)runnable_task->callback)(runnable_task->device, runnable_task->data);
			} else {
				runnable_task->callback(runnable_task->device);
			}
			if (runnable_task->task_mutex) { // if there is specific mutex for task, unlock it
				pthread_mutex_unlock(runnable_task->task_mutex);
			}
			indigo_safe_free(runnable_task);
			pthread_mutex_lock(&queue->mutex);
			queue->running_task = NULL;
			queue->running = false;
			pthread_cond_broadcast(&queue->cond);
			pthread_mutex_unlock(&queue->mutex);
		}
		pthread_mutex_lock(&queue->mutex);
	}
	bool self_delete_requested = queue->self_delete_requested;
	pthread_mutex_unlock(&queue->mutex);
	if (self_delete_requested) {
		pthread_detach(queue->thread);
		pthread_cond_destroy(&queue->cond);
		pthread_mutex_destroy(&queue->mutex);
		indigo_safe_free(queue);
	}
	return NULL;
}

// create queue, device should be master device if we need serialize access to shared connection. otherwise it may be any device.
indigo_queue *indigo_queue_create(indigo_device *device) {
	indigo_queue *queue = indigo_safe_malloc(sizeof(indigo_queue));
	queue->device = device;
	init_timer_cond(&queue->cond);
	pthread_mutex_init(&queue->mutex, NULL);
	queue->running_task = NULL;
	queue->running = false;
	queue->abort = false;
	queue->ready = false;
	queue->self_delete_requested = false;
	if (pthread_create(&queue->thread, NULL, (void * (*)(void*))queue_func, queue) != 0) {
		indigo_error("Failed to start queue thread");
		pthread_cond_destroy(&queue->cond);
		pthread_mutex_destroy(&queue->mutex);
		indigo_safe_free(queue);
		return NULL;
	}
	pthread_mutex_lock(&queue->mutex);
	while (!queue->ready) {
		pthread_cond_wait(&queue->cond, &queue->mutex);
	}
	pthread_mutex_unlock(&queue->mutex);
	return queue;
}

// add task to queue. note queue carries device which is usually master device while task may be related to some of slave devices

void indigo_queue_add(indigo_queue *queue, indigo_device *device, int priority, double delay, indigo_timer_callback callback, pthread_mutex_t *task_mutex) {
	if (queue == NULL) {
		return;
	}
	indigo_queue_task *task = indigo_safe_malloc(sizeof(indigo_queue_task));
	task->device = device;
	task->priority = priority;
	task->at = indigo_delay_to_time(delay);
	task->callback = callback;
	task->data = NULL;
	task->has_data = false;
	task->task_mutex = task_mutex;
	pthread_mutex_lock(&queue->mutex);
	task->next = NULL;
	enqueue_task_locked(queue, task);
	pthread_cond_signal(&queue->cond);
	pthread_mutex_unlock(&queue->mutex);
}

void indigo_queue_add_with_data(indigo_queue *queue, indigo_device *device, int priority, double delay, indigo_timer_with_data_callback callback, void *data, pthread_mutex_t *task_mutex) {
	if (queue == NULL) {
		return;
	}
	indigo_queue_task *task = indigo_safe_malloc(sizeof(indigo_queue_task));
	task->device = device;
	task->priority = priority;
	task->at = indigo_delay_to_time(delay);
	task->callback = (indigo_timer_callback)callback;
	task->data = data;
	task->has_data = true;
	task->task_mutex = task_mutex;
	pthread_mutex_lock(&queue->mutex);
	task->next = NULL;
	enqueue_task_locked(queue, task);
	pthread_cond_signal(&queue->cond);
	pthread_mutex_unlock(&queue->mutex);
}

// remove scheduled tasks from queue, if device is NULL, remove all tasks otherwise remove tasks related to device
void indigo_queue_remove(indigo_queue *queue, indigo_device *device, indigo_timer_callback callback) {
	if (queue) {
		pthread_mutex_lock(&queue->mutex);
		remove_tasks_locked(queue, device, callback);
		pthread_cond_signal(&queue->cond);
		while (!pthread_equal(pthread_self(), queue->thread) && queue->running && task_matches(queue->running_task, device, callback)) {
			pthread_cond_wait(&queue->cond, &queue->mutex);
		}
		pthread_mutex_unlock(&queue->mutex);
	}
}

// remove all tasks from queue and queue itself
void indigo_queue_delete(indigo_queue **queue) {
	if (queue == NULL || *queue == NULL) {
		return;
	}
	indigo_queue *q = *queue;
	bool self_delete = pthread_equal(pthread_self(), q->thread);
	pthread_mutex_lock(&q->mutex);
	q->abort = true;
	q->self_delete_requested = self_delete;
	remove_tasks_locked(q, NULL, NULL);
	pthread_cond_broadcast(&q->cond);
	while (!self_delete && q->running) {
		pthread_cond_wait(&q->cond, &q->mutex);
	}
	pthread_mutex_unlock(&q->mutex);
	if (!self_delete) {
		pthread_join(q->thread, NULL);
		pthread_cond_destroy(&q->cond);
		pthread_mutex_destroy(&q->mutex);
		indigo_safe_free(q);
		*queue = NULL;
	} else {
		*queue = NULL;
	}
}
