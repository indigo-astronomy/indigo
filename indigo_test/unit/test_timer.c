// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#if !defined(INDIGO_WINDOWS)
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <indigo/indigo_driver.h>
#include <indigo/indigo_timer.h>

#include "../test_runner.h"

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int callback_count;
	int alternate_callback_count;
	int data_callback_count;
	int running_callback_count;
	int mutex_callback_count;
	int error_count;
	int active_callback_count;
	int max_parallel_count;
	int queue_running_count;
	int queue_finished_count;
	int stress_callback_count;
	int stress_alternate_callback_count;
	int sequence[4096];
	int sequence_count;
	bool callback_started;
	bool callback_finished;
	bool mutex_was_locked;
	int64_t first_callback_time_ns;
	void *last_data;
} timer_test_state;

static timer_test_state state;

static void reset_state(void) {
	memset(&state, 0, sizeof(state));
	pthread_mutex_init(&state.mutex, NULL);
	pthread_cond_init(&state.cond, NULL);
}

static void destroy_state(void) {
	pthread_cond_destroy(&state.cond);
	pthread_mutex_destroy(&state.mutex);
}

static bool wait_for_count(int *counter, int expected_count) {
	for (int i = 0; i < 100; i++) {
		pthread_mutex_lock(&state.mutex);
		bool matched = *counter >= expected_count;
		pthread_mutex_unlock(&state.mutex);
		if (matched) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static bool wait_for_flag(bool *flag) {
	for (int i = 0; i < 100; i++) {
		pthread_mutex_lock(&state.mutex);
		bool matched = *flag;
		pthread_mutex_unlock(&state.mutex);
		if (matched) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static bool wait_for_timer_reference_to_clear(indigo_timer **timer) {
	for (int i = 0; i < 100; i++) {
		if (*timer == NULL) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static int64_t timespec_to_ns(struct timespec time) {
	return (int64_t)time.tv_sec * 1000000000LL + time.tv_nsec;
}

static int64_t now_ns(void) {
	struct timespec time;
#if defined(CLOCK_MONOTONIC)
	clock_gettime(CLOCK_MONOTONIC, &time);
#else
	clock_gettime(CLOCK_REALTIME, &time);
#endif
	return timespec_to_ns(time);
}

static void record_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.callback_count++;
	if (state.first_callback_time_ns == 0) {
		state.first_callback_time_ns = now_ns();
	}
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void record_alternate_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.alternate_callback_count++;
	if (state.first_callback_time_ns == 0) {
		state.first_callback_time_ns = now_ns();
	}
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void record_data_callback(indigo_device *device, void *data) {
	pthread_mutex_lock(&state.mutex);
	state.data_callback_count++;
	state.last_data = data;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void record_parallel_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.active_callback_count++;
	if (state.active_callback_count > state.max_parallel_count) {
		state.max_parallel_count = state.active_callback_count;
	}
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(40000);

	pthread_mutex_lock(&state.mutex);
	state.active_callback_count--;
	state.callback_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void record_mutex_serialized_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.active_callback_count++;
	if (state.active_callback_count > state.max_parallel_count) {
		state.max_parallel_count = state.active_callback_count;
	}
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(20000);

	pthread_mutex_lock(&state.mutex);
	state.active_callback_count--;
	state.mutex_callback_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void long_running_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.running_callback_count++;
	state.callback_started = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(100000);

	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static indigo_timer *cancel_all_followup_timer = NULL;

static void cancel_all_schedules_followup_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.callback_count++;
	state.callback_started = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(20000);
	indigo_set_timer(device, 0.4, record_alternate_callback, &cancel_all_followup_timer);
	indigo_usleep(20000);

	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void mutex_observing_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.mutex_callback_count++;
	state.mutex_was_locked = pthread_mutex_trylock(&DEVICE_CONTEXT->device_mutex) == EBUSY;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void queue_slow_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.queue_running_count++;
	state.callback_started = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(100000);

	pthread_mutex_lock(&state.mutex);
	state.queue_finished_count++;
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void queue_barrier_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.queue_running_count++;
	state.callback_started = true;
	pthread_cond_broadcast(&state.cond);
	while (!state.callback_finished) {
		pthread_cond_wait(&state.cond, &state.mutex);
	}
	state.queue_finished_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void queue_data_callback(indigo_device *device, void *data) {
	pthread_mutex_lock(&state.mutex);
	if (state.sequence_count < (int)(sizeof(state.sequence) / sizeof(state.sequence[0]))) {
		state.sequence[state.sequence_count++] = *(int *)data;
	} else {
		indigo_test_failures++;
	}
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void queue_callback_a(indigo_device *device, void *data) {
	queue_data_callback(device, data);
}

static void queue_callback_b(indigo_device *device, void *data) {
	queue_data_callback(device, data);
}

static pthread_mutex_t *observed_queue_task_mutex = NULL;

static void queue_task_mutex_observing_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.mutex_callback_count++;
	state.mutex_was_locked = observed_queue_task_mutex != NULL && pthread_mutex_trylock(observed_queue_task_mutex) == EBUSY;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

typedef struct {
	indigo_queue *queue;
	int first;
	int second;
	double followup_delay;
} queue_followup_context;

static void queue_enqueue_followup_callback(indigo_device *device, void *data) {
	queue_followup_context *context = (queue_followup_context *)data;

	queue_data_callback(device, &context->first);
	indigo_queue_add_with_data(context->queue, device, INDIGO_TASK_PRIORITY_NORMAL, context->followup_delay, queue_callback_b, &context->second, NULL);
}

static void queue_remove_self_callback(indigo_device *device, void *data) {
	indigo_queue *queue = (indigo_queue *)data;

	pthread_mutex_lock(&state.mutex);
	state.queue_running_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_queue_remove(queue, device, (indigo_timer_callback)queue_remove_self_callback);

	pthread_mutex_lock(&state.mutex);
	state.queue_finished_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void queue_delete_self_callback(indigo_device *device, void *data) {
	indigo_queue **queue = (indigo_queue **)data;

	pthread_mutex_lock(&state.mutex);
	state.queue_running_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_queue_delete(queue);

	pthread_mutex_lock(&state.mutex);
	if (*queue != NULL) {
		state.error_count++;
	}
	state.queue_finished_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static bool wait_for_sequence_count(int expected_count) {
	return wait_for_count(&state.sequence_count, expected_count);
}

static bool wait_for_stress_callback_total(int expected_count) {
	for (int i = 0; i < 200; i++) {
		pthread_mutex_lock(&state.mutex);
		bool matched = state.stress_callback_count + state.stress_alternate_callback_count >= expected_count;
		pthread_mutex_unlock(&state.mutex);
		if (matched) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static indigo_device make_test_device(indigo_device_context *context) {
	indigo_device device;
	memset(&device, 0, sizeof(device));
	strncpy(device.name, "Timer Test Device", INDIGO_NAME_SIZE - 1);
	device.device_context = context;
	pthread_mutex_init(&context->device_mutex, NULL);
	return device;
}

static void destroy_test_device(indigo_device_context *context) {
	pthread_mutex_destroy(&context->device_mutex);
}

static indigo_timer *self_reschedule_timer = NULL;
static int self_reschedule_target = 0;

static void self_reschedule_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	if (self_reschedule_timer == NULL) {
		state.error_count++;
	}
	state.callback_count++;
	int callback_count = state.callback_count;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	if (callback_count < self_reschedule_target) {
		if (!indigo_reschedule_timer(device, 0.001, &self_reschedule_timer)) {
			pthread_mutex_lock(&state.mutex);
			state.error_count++;
			pthread_cond_broadcast(&state.cond);
			pthread_mutex_unlock(&state.mutex);
		}
	}
}

static void self_reschedule_alternate_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	if (self_reschedule_timer == NULL) {
		state.error_count++;
	}
	state.alternate_callback_count++;
	int alternate_callback_count = state.alternate_callback_count;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	if (alternate_callback_count < self_reschedule_target) {
		if (!indigo_reschedule_timer_with_callback(device, 0.001, self_reschedule_alternate_callback, &self_reschedule_timer)) {
			pthread_mutex_lock(&state.mutex);
			state.error_count++;
			pthread_cond_broadcast(&state.cond);
			pthread_mutex_unlock(&state.mutex);
		}
	}
}

static void self_reschedule_switch_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	if (self_reschedule_timer == NULL) {
		state.error_count++;
	}
	state.callback_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	if (!indigo_reschedule_timer_with_callback(device, 0.001, self_reschedule_alternate_callback, &self_reschedule_timer)) {
		pthread_mutex_lock(&state.mutex);
		state.error_count++;
		pthread_cond_broadcast(&state.cond);
		pthread_mutex_unlock(&state.mutex);
	}
}

static indigo_timer *self_cancel_sync_timer = NULL;

static void self_cancel_timer_sync_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.callback_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_cancel_timer_sync(device, &self_cancel_sync_timer);

	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

#if !defined(INDIGO_WINDOWS)
static void run_child_test(void (*child_test)(void)) {
	int before = indigo_test_failures;
	child_test();
	_exit(indigo_test_failures == before ? 0 : 1);
}

static void assert_child_exits_without_deadlock(void (*child_test)(void)) {
	pid_t pid = fork();
	ASSERT_TRUE(pid >= 0);
	if (pid == 0) {
		run_child_test(child_test);
	}

	int status = 0;
	for (int i = 0; i < 20; i++) {
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == pid) {
			ASSERT_TRUE(WIFEXITED(status));
			ASSERT_EQ_INT(0, WEXITSTATUS(status));
			return;
		}
		ASSERT_TRUE(result == 0);
		indigo_usleep(100000);
	}
	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
	ASSERT_TRUE(false);
}
#endif

typedef struct {
	indigo_timer *timer;
	int worker;
	int slot;
	uint32_t magic;
} stress_timer_payload;

static void stress_callback(indigo_device *device, void *data) {
	stress_timer_payload *payload = (stress_timer_payload *)data;
	pthread_mutex_lock(&state.mutex);
	if (payload == NULL || payload->worker < 0 || payload->slot < 0 || payload->magic != 0x54494d45u) {
		state.error_count++;
	} else {
		state.stress_callback_count++;
	}
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void stress_alternate_callback(indigo_device *device, void *data) {
	stress_timer_payload *payload = (stress_timer_payload *)data;
	pthread_mutex_lock(&state.mutex);
	if (payload == NULL || payload->worker < 0 || payload->slot < 0 || payload->magic != 0x54494d45u) {
		state.error_count++;
	} else {
		state.stress_alternate_callback_count++;
	}
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static uint32_t stress_next_random(uint32_t *seed) {
	uint32_t value = *seed;
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	*seed = value;
	return value;
}

#define STRESS_WORKER_COUNT 4
#define STRESS_SLOT_COUNT 8
#define STRESS_ITERATIONS 1250
#define STORM_TIMER_COUNT 512
#define CANCELLATION_STORM_TIMER_COUNT 512
#define QUEUE_CHURN_PRODUCER_COUNT 4
#define QUEUE_CHURN_TASKS_PER_PRODUCER 250
#define QUEUE_CHURN_DEVICE_COUNT 4

typedef struct {
	int worker;
	stress_timer_payload payloads[STRESS_SLOT_COUNT];
} stress_worker_context;

typedef struct {
	stress_timer_payload *payloads;
	pthread_mutex_t *locks;
	int payload_count;
	int worker;
	uint32_t seed;
} shared_stress_worker_context;

typedef struct {
	indigo_timer **timers;
	int first;
	int count;
} cancellation_storm_context;

typedef struct {
	indigo_timer **timer;
	pthread_mutex_t mutex;
	bool stop;
} reschedule_race_context;

typedef struct {
	indigo_queue *queue;
	indigo_device *device;
	int first_value;
	int count;
	int values[64];
} queue_producer_context;

typedef struct {
	int id;
	uint32_t magic;
} queue_churn_payload;

typedef struct {
	indigo_queue *queue;
	indigo_device *devices;
	queue_churn_payload *payloads;
	int producer;
	uint32_t seed;
} queue_churn_producer_context;

typedef struct {
	indigo_queue *queue;
	indigo_device *devices;
	uint32_t seed;
} queue_churn_remover_context;

static void *stress_worker(void *data) {
	stress_worker_context *context = (stress_worker_context *)data;
	uint32_t seed = 0x9e3779b9u ^ (uint32_t)(context->worker * 0x10001u);
	for (int i = 0; i < STRESS_SLOT_COUNT; i++) {
		context->payloads[i].timer = NULL;
		context->payloads[i].worker = context->worker;
		context->payloads[i].slot = i;
		context->payloads[i].magic = 0x54494d45u;
	}
	for (int i = 0; i < STRESS_ITERATIONS; i++) {
		int slot = (int)(stress_next_random(&seed) % STRESS_SLOT_COUNT);
		int action = (int)(stress_next_random(&seed) % 5);
		double delay = (double)(stress_next_random(&seed) % 5000) / 1000000.0;
		stress_timer_payload *payload = context->payloads + slot;
		if (payload->timer == NULL) {
			indigo_set_timer_with_data(NULL, delay, stress_callback, &payload->timer, payload);
		} else if (action == 0) {
			indigo_cancel_timer(NULL, &payload->timer);
		} else if (action == 1) {
			indigo_cancel_timer_sync(NULL, &payload->timer);
		} else if (action == 2) {
			indigo_reschedule_timer(NULL, delay, &payload->timer);
		} else {
			indigo_reschedule_timer_with_callback(NULL, delay, (indigo_timer_callback)stress_alternate_callback, &payload->timer);
		}
		if ((i % 37) == 0) {
			indigo_usleep(1000);
		}
	}
	for (int i = 0; i < STRESS_SLOT_COUNT; i++) {
		indigo_cancel_timer_sync(NULL, &context->payloads[i].timer);
	}
	return NULL;
}

static void *shared_stress_worker(void *data) {
	shared_stress_worker_context *context = (shared_stress_worker_context *)data;
	uint32_t seed = context->seed;
	for (int i = 0; i < STRESS_ITERATIONS; i++) {
		int slot = (int)(stress_next_random(&seed) % context->payload_count);
		int action = (int)(stress_next_random(&seed) % 5);
		double delay = (double)(stress_next_random(&seed) % 4000) / 1000000.0;
		stress_timer_payload *payload = context->payloads + slot;
		pthread_mutex_lock(context->locks + slot);
		if (payload->timer == NULL) {
			indigo_set_timer_with_data(NULL, delay, stress_callback, &payload->timer, payload);
		} else if (action == 1) {
			indigo_cancel_timer(NULL, &payload->timer);
		} else if (action == 2) {
			indigo_cancel_timer_sync(NULL, &payload->timer);
		} else if (action == 3) {
			indigo_reschedule_timer(NULL, delay, &payload->timer);
		} else {
			indigo_reschedule_timer_with_callback(NULL, delay, (indigo_timer_callback)stress_alternate_callback, &payload->timer);
		}
		pthread_mutex_unlock(context->locks + slot);
		if ((i % 53) == 0) {
			indigo_usleep(1000);
		}
	}
	return NULL;
}

static void *cancellation_storm_worker(void *data) {
	cancellation_storm_context *context = (cancellation_storm_context *)data;
	for (int i = context->first; i < context->first + context->count; i++) {
		indigo_cancel_timer_sync(NULL, context->timers + i);
	}
	return NULL;
}

static void *reschedule_race_worker(void *data) {
	reschedule_race_context *context = (reschedule_race_context *)data;
	while (true) {
		pthread_mutex_lock(&context->mutex);
		bool stop = context->stop;
		pthread_mutex_unlock(&context->mutex);
		if (stop) {
			break;
		}
		indigo_reschedule_timer_with_callback(NULL, 0.002, record_alternate_callback, context->timer);
		indigo_usleep(500);
	}
	return NULL;
}

static void *queue_asap_producer(void *data) {
	queue_producer_context *context = (queue_producer_context *)data;
	for (int i = 0; i < context->count; i++) {
		context->values[i] = context->first_value + i;
		indigo_queue_add_with_data(context->queue, context->device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_callback_a, context->values + i, NULL);
	}
	return NULL;
}

static void queue_churn_callback_a(indigo_device *device, void *data) {
	queue_churn_payload *payload = (queue_churn_payload *)data;
	pthread_mutex_lock(&state.mutex);
	state.active_callback_count++;
	if (state.active_callback_count > state.max_parallel_count) {
		state.max_parallel_count = state.active_callback_count;
	}
	if (payload == NULL || payload->magic != 0x51554555u) {
		state.error_count++;
	} else {
		state.stress_callback_count++;
	}
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(500);

	pthread_mutex_lock(&state.mutex);
	state.active_callback_count--;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void queue_churn_callback_b(indigo_device *device, void *data) {
	queue_churn_payload *payload = (queue_churn_payload *)data;
	pthread_mutex_lock(&state.mutex);
	state.active_callback_count++;
	if (state.active_callback_count > state.max_parallel_count) {
		state.max_parallel_count = state.active_callback_count;
	}
	if (payload == NULL || payload->magic != 0x51554555u) {
		state.error_count++;
	} else {
		state.stress_alternate_callback_count++;
	}
	pthread_mutex_unlock(&state.mutex);

	indigo_usleep(500);

	pthread_mutex_lock(&state.mutex);
	state.active_callback_count--;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void *queue_churn_producer(void *data) {
	queue_churn_producer_context *context = (queue_churn_producer_context *)data;
	uint32_t seed = context->seed;
	for (int i = 0; i < QUEUE_CHURN_TASKS_PER_PRODUCER; i++) {
		int payload_index = context->producer * QUEUE_CHURN_TASKS_PER_PRODUCER + i;
		int device_index = (int)(stress_next_random(&seed) % QUEUE_CHURN_DEVICE_COUNT);
		int priority = (int)(stress_next_random(&seed) % 4) * INDIGO_TASK_PRIORITY_HIGH;
		double delay = (double)(stress_next_random(&seed) % 3000) / 1000000.0;
		indigo_timer_with_data_callback callback = (stress_next_random(&seed) & 1) ? queue_churn_callback_a : queue_churn_callback_b;
		context->payloads[payload_index].id = payload_index;
		context->payloads[payload_index].magic = 0x51554555u;
		indigo_queue_add_with_data(context->queue, context->devices + device_index, priority, delay, callback, context->payloads + payload_index, NULL);
		if ((i % 31) == 0) {
			indigo_usleep(500);
		}
	}
	return NULL;
}

static void *queue_churn_remover(void *data) {
	queue_churn_remover_context *context = (queue_churn_remover_context *)data;
	uint32_t seed = context->seed;
	for (int i = 0; i < 200; i++) {
		int device_selector = (int)(stress_next_random(&seed) % (QUEUE_CHURN_DEVICE_COUNT + 1));
		int callback_selector = (int)(stress_next_random(&seed) % 3);
		indigo_device *device = device_selector == QUEUE_CHURN_DEVICE_COUNT ? NULL : context->devices + device_selector;
		indigo_timer_callback callback = NULL;
		if (callback_selector == 1) {
			callback = (indigo_timer_callback)queue_churn_callback_a;
		} else if (callback_selector == 2) {
			callback = (indigo_timer_callback)queue_churn_callback_b;
		}
		indigo_queue_remove(context->queue, device, callback);
		indigo_usleep(700);
	}
	return NULL;
}

static void delay_to_time_handles_zero_and_orders_positive_delays(void) {
	struct timespec zero = indigo_delay_to_time(0);
	ASSERT_EQ_INT(0, (int)zero.tv_sec);
	ASSERT_EQ_INT(0, (int)zero.tv_nsec);

	struct timespec earlier = indigo_delay_to_time(0.01);
	struct timespec later = indigo_delay_to_time(0.05);
	ASSERT_TRUE(later.tv_sec > earlier.tv_sec || (later.tv_sec == earlier.tv_sec && later.tv_nsec > earlier.tv_nsec));
	ASSERT_TRUE(earlier.tv_nsec >= 0);
	ASSERT_TRUE(earlier.tv_nsec < 1000000000L);
	ASSERT_TRUE(later.tv_nsec >= 0);
	ASSERT_TRUE(later.tv_nsec < 1000000000L);
}

static void delay_to_time_normalizes_larger_fractional_delays(void) {
	struct timespec earlier = indigo_delay_to_time(0.75);
	struct timespec later = indigo_delay_to_time(1.75);
	int64_t delta_ns = timespec_to_ns(later) - timespec_to_ns(earlier);

	ASSERT_TRUE(earlier.tv_nsec >= 0);
	ASSERT_TRUE(earlier.tv_nsec < 1000000000L);
	ASSERT_TRUE(later.tv_nsec >= 0);
	ASSERT_TRUE(later.tv_nsec < 1000000000L);
	ASSERT_TRUE(delta_ns > 900000000LL);
	ASSERT_TRUE(delta_ns < 1100000000LL);
}

static void negative_delay_runs_promptly_and_uses_normalized_time(void) {
	struct timespec when = indigo_delay_to_time(-0.01);
	ASSERT_TRUE(when.tv_nsec >= 0);
	ASSERT_TRUE(when.tv_nsec < 1000000000L);

	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, -0.01, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));

	destroy_state();
}

static void zero_delay_timer_runs_promptly_and_clears_reference(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));

	destroy_state();
}

static void nonzero_delay_timer_does_not_fire_before_deadline(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.15, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	indigo_usleep(50000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);

	destroy_state();
}

static void set_timer_runs_callback_and_clears_reference(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.01, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	for (int i = 0; i < 100 && timer != NULL; i++) {
		indigo_usleep(10000);
	}
	ASSERT_TRUE(timer == NULL);

	destroy_state();
}

static void set_timer_with_data_passes_user_data(void) {
	reset_state();
	indigo_timer *timer = NULL;
	int payload = 42;

	ASSERT_TRUE(indigo_set_timer_with_data(NULL, 0.01, record_data_callback, &timer, &payload));
	ASSERT_TRUE(wait_for_count(&state.data_callback_count, 1));
	ASSERT_TRUE(state.last_data == &payload);

	destroy_state();
}

static void set_timer_with_data_passes_null_user_data(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer_with_data(NULL, 0.01, record_data_callback, &timer, NULL));
	ASSERT_TRUE(wait_for_count(&state.data_callback_count, 1));
	ASSERT_TRUE(state.last_data == NULL);

	destroy_state();
}

static void set_timer_with_mutex_runs_callback_while_mutex_is_locked(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer_with_mutex(&device, 0.01, mutex_observing_callback, &timer, &context.device_mutex));
	ASSERT_TRUE(wait_for_count(&state.mutex_callback_count, 1));
	ASSERT_TRUE(state.mutex_was_locked);

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_timer_prevents_pending_callback(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(indigo_cancel_timer(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(300000);
	ASSERT_EQ_INT(0, state.callback_count);

	destroy_state();
}

static void cancel_timer_sync_prevents_pending_callback_and_returns_true(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);

	destroy_state();
}

static void cancel_timer_sync_without_reference_returns_false(void) {
	indigo_timer *timer = NULL;
	ASSERT_FALSE(indigo_cancel_timer_sync(NULL, &timer));
}

static void cancel_timer_sync_waits_for_running_callback(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, long_running_callback, &timer));
	ASSERT_TRUE(wait_for_count(&state.running_callback_count, 1));
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(state.callback_started);
	ASSERT_TRUE(state.callback_finished);
	ASSERT_EQ_INT(1, state.running_callback_count);

	destroy_state();
}

static void cancel_timer_returns_false_for_running_callback_without_waiting(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, long_running_callback, &timer));
	ASSERT_TRUE(wait_for_count(&state.running_callback_count, 1));
	ASSERT_FALSE(indigo_cancel_timer(NULL, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(state.callback_started);
	ASSERT_FALSE(state.callback_finished);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(state.callback_finished);
	ASSERT_EQ_INT(1, state.running_callback_count);

	destroy_state();
}

static void completing_callback_does_not_clear_newer_timer_reference(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, long_running_callback, &timer));
	ASSERT_TRUE(wait_for_count(&state.running_callback_count, 1));
	timer = NULL;
	ASSERT_TRUE(indigo_set_timer(NULL, 0.3, record_alternate_callback, &timer));
	indigo_timer *newer_timer = timer;
	ASSERT_TRUE(wait_for_flag(&state.callback_finished));
	ASSERT_TRUE(timer == newer_timer);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_EQ_INT(0, state.alternate_callback_count);

	destroy_state();
}

#if !defined(INDIGO_WINDOWS)
static void child_cancel_timer_sync_from_own_callback(void) {
	reset_state();
	self_cancel_sync_timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, self_cancel_timer_sync_callback, &self_cancel_sync_timer));
	ASSERT_TRUE(wait_for_flag(&state.callback_finished));
	ASSERT_TRUE(self_cancel_sync_timer == NULL);
	ASSERT_EQ_INT(1, state.callback_count);

	destroy_state();
}

static void cancel_timer_sync_from_own_callback_does_not_deadlock(void) {
	assert_child_exits_without_deadlock(child_cancel_timer_sync_from_own_callback);
}
#endif

static void reschedule_timer_changes_delay_and_callback(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	indigo_usleep(50000);
	ASSERT_TRUE(indigo_reschedule_timer_with_callback(NULL, 0.01, record_alternate_callback, &timer));
	ASSERT_TRUE(wait_for_count(&state.alternate_callback_count, 1));
	indigo_usleep(100000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(1, state.alternate_callback_count);

	destroy_state();
}

static void reschedule_pending_timer_to_earlier_deadline_wakes_scheduler(void) {
	reset_state();
	indigo_timer *timer = NULL;
	int64_t start_ns = now_ns();

	ASSERT_TRUE(indigo_set_timer(NULL, 0.3, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	indigo_usleep(50000);
	ASSERT_TRUE(indigo_reschedule_timer(NULL, 0.02, &timer));
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));
	ASSERT_TRUE(state.first_callback_time_ns - start_ns < 200000000LL);
	ASSERT_EQ_INT(1, state.callback_count);

	destroy_state();
}

static void reschedule_pending_timer_to_later_deadline_prevents_old_deadline(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.08, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	indigo_usleep(40000);
	ASSERT_TRUE(indigo_reschedule_timer(NULL, 0.2, &timer));
	indigo_usleep(80000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));

	destroy_state();
}

static void reschedule_timer_from_callback_repeats_until_complete(void) {
	reset_state();
	self_reschedule_timer = NULL;
	self_reschedule_target = 10;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, self_reschedule_callback, &self_reschedule_timer));
	ASSERT_TRUE(wait_for_count(&state.callback_count, self_reschedule_target));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&self_reschedule_timer));
	ASSERT_EQ_INT(0, state.error_count);
	ASSERT_EQ_INT(self_reschedule_target, state.callback_count);

	destroy_state();
}

static void self_reschedule_can_switch_to_different_callback(void) {
	reset_state();
	self_reschedule_timer = NULL;
	self_reschedule_target = 4;

	ASSERT_TRUE(indigo_set_timer(NULL, 0, self_reschedule_switch_callback, &self_reschedule_timer));
	ASSERT_TRUE(wait_for_count(&state.alternate_callback_count, self_reschedule_target));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&self_reschedule_timer));
	ASSERT_EQ_INT(0, state.error_count);
	ASSERT_EQ_INT(1, state.callback_count);
	ASSERT_EQ_INT(self_reschedule_target, state.alternate_callback_count);

	destroy_state();
}

static void reschedule_without_reference_fails(void) {
	indigo_timer *timer = NULL;
	ASSERT_FALSE(indigo_reschedule_timer(NULL, 0.01, &timer));
}

static void cancel_racing_repeated_reschedule_leaves_one_logical_outcome(void) {
	reset_state();
	indigo_timer *timer = NULL;
	reschedule_race_context context = { &timer, PTHREAD_MUTEX_INITIALIZER, false };
	pthread_t thread;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.02, record_callback, &timer));
	ASSERT_EQ_INT(0, pthread_create(&thread, NULL, reschedule_race_worker, &context));
	indigo_usleep(5000);
	pthread_mutex_lock(&context.mutex);
	context.stop = true;
	pthread_mutex_unlock(&context.mutex);
	indigo_cancel_timer_sync(NULL, &timer);
	ASSERT_EQ_INT(0, pthread_join(thread, NULL));
	pthread_mutex_destroy(&context.mutex);
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(50000);
	ASSERT_TRUE(state.callback_count + state.alternate_callback_count <= 1);
	ASSERT_FALSE(state.callback_count > 0 && state.alternate_callback_count > 0);

	destroy_state();
}

static void reschedule_racing_natural_completion_never_leaves_stuck_reference(void) {
	reset_state();
	indigo_timer *timer = NULL;

	for (int i = 0; i < 100; i++) {
		int before = state.callback_count;
		ASSERT_TRUE(indigo_set_timer(NULL, 0.001, record_callback, &timer));
		indigo_usleep((i % 3) * 500);
		indigo_reschedule_timer(NULL, 0.001, &timer);
		ASSERT_TRUE(wait_for_count(&state.callback_count, before + 1));
		ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));
		ASSERT_EQ_INT(before + 1, state.callback_count);
	}

	destroy_state();
}

static void set_timer_with_non_null_reference_fails_cleanly(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	indigo_timer *original_timer = timer;
	ASSERT_FALSE(indigo_set_timer(NULL, 0.01, record_alternate_callback, &timer));
	ASSERT_TRUE(timer == original_timer);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);

	destroy_state();
}

static void set_timer_can_reuse_reference_after_immediate_completion(void) {
	reset_state();
	indigo_timer *timer = NULL;

	for (int i = 0; i < 100; i++) {
		ASSERT_TRUE(indigo_set_timer(NULL, 0, record_callback, &timer));
		ASSERT_TRUE(wait_for_count(&state.callback_count, i + 1));
		ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));
	}
	ASSERT_EQ_INT(100, state.callback_count);

	destroy_state();
}

static void cancel_timer_twice_is_harmless(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(indigo_cancel_timer(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_FALSE(indigo_cancel_timer(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);

	destroy_state();
}

static void cancel_timer_sync_twice_is_harmless(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_FALSE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);

	destroy_state();
}

static void cancel_timer_sync_with_stale_reference_returns_false(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	indigo_timer *stale_handle = timer;
	ASSERT_TRUE(indigo_cancel_timer(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_alternate_callback, &timer));
	indigo_timer *newer_timer = timer;
	ASSERT_FALSE(indigo_cancel_timer_sync(NULL, &stale_handle));
	ASSERT_TRUE(timer == newer_timer);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);

	destroy_state();
}

static void stale_timer_handle_does_not_cancel_or_reschedule_newer_timer(void) {
	reset_state();
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	indigo_timer *stale_handle = timer;
	ASSERT_TRUE(indigo_cancel_timer(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_alternate_callback, &timer));
	indigo_timer *newer_timer = timer;
	ASSERT_FALSE(indigo_cancel_timer(NULL, &stale_handle));
	ASSERT_TRUE(timer == newer_timer);
	ASSERT_FALSE(indigo_reschedule_timer(NULL, 0, &stale_handle));
	ASSERT_TRUE(timer == newer_timer);
	indigo_usleep(50000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);

	destroy_state();
}

static void canceling_timer_blocked_on_user_mutex_does_not_block_scheduler(void) {
	reset_state();
	pthread_mutex_t user_mutex;
	pthread_mutex_init(&user_mutex, NULL);
	indigo_timer *blocked_timer = NULL;
	indigo_timer *free_timer = NULL;

	pthread_mutex_lock(&user_mutex);
	ASSERT_TRUE(indigo_set_timer_with_mutex(NULL, 0, record_callback, &blocked_timer, &user_mutex));
	indigo_usleep(20000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_TRUE(indigo_set_timer(NULL, 0.01, record_alternate_callback, &free_timer));
	ASSERT_TRUE(wait_for_count(&state.alternate_callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&free_timer));
	ASSERT_TRUE(blocked_timer != NULL);
	pthread_mutex_unlock(&user_mutex);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&blocked_timer));

	pthread_mutex_destroy(&user_mutex);
	destroy_state();
}

static void cancel_many_pending_timers_out_of_deadline_order(void) {
	reset_state();
	indigo_timer *timers[16] = { 0 };
	const double delays[16] = {
		0.34, 0.05, 0.28, 0.11, 0.31, 0.07, 0.25, 0.14,
		0.37, 0.09, 0.22, 0.16, 0.40, 0.12, 0.19, 0.27
	};
	const int order[16] = {
		7, 1, 14, 3, 10, 5, 12, 0, 15, 2, 9, 4, 13, 6, 11, 8
	};

	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(indigo_set_timer(NULL, delays[i], record_callback, timers + i));
		ASSERT_TRUE(timers[i] != NULL);
	}
	for (int i = 0; i < (int)(sizeof(order) / sizeof(order[0])); i++) {
		int index = order[i];
		ASSERT_TRUE(indigo_cancel_timer_sync(NULL, timers + index));
		ASSERT_TRUE(timers[index] == NULL);
	}
	indigo_usleep(450000);
	ASSERT_EQ_INT(0, state.callback_count);
	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(timers[i] == NULL);
	}

	destroy_state();
}

static void timers_for_same_device_run_in_parallel_by_default(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timers[12] = { 0 };

	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(indigo_set_timer(&device, 0.01, record_parallel_callback, timers + i));
	}
	ASSERT_TRUE(wait_for_count(&state.callback_count, (int)(sizeof(timers) / sizeof(timers[0]))));
	ASSERT_TRUE(state.max_parallel_count > 1);
	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(wait_for_timer_reference_to_clear(timers + i));
	}

	destroy_test_device(&context);
	destroy_state();
}

static void timers_with_same_mutex_are_serialized(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timers[8] = { 0 };

	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(indigo_set_timer_with_mutex(&device, 0.01, record_mutex_serialized_callback, timers + i, &context.device_mutex));
	}
	ASSERT_TRUE(wait_for_count(&state.mutex_callback_count, (int)(sizeof(timers) / sizeof(timers[0]))));
	ASSERT_EQ_INT(1, state.max_parallel_count);
	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(wait_for_timer_reference_to_clear(timers + i));
	}

	destroy_test_device(&context);
	destroy_state();
}

static void device_timer_is_linked_until_completion_and_unlinked_afterward(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(&device, 0.01, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(context.timers == timer);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_TRUE(wait_for_timer_reference_to_clear(&timer));
	ASSERT_TRUE(context.timers == NULL);

	destroy_test_device(&context);
	destroy_state();
}

static void canceling_device_timer_unlinks_it_from_device_list(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(&device, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(context.timers == timer);
	ASSERT_TRUE(indigo_cancel_timer_sync(&device, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(context.timers == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);

	destroy_test_device(&context);
	destroy_state();
}

static void null_device_timer_is_not_linked_to_unrelated_device_list(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timer = NULL;

	ASSERT_TRUE(indigo_set_timer(NULL, 0.2, record_callback, &timer));
	ASSERT_TRUE(timer != NULL);
	ASSERT_TRUE(context.timers == NULL);
	ASSERT_TRUE(indigo_cancel_timer_sync(NULL, &timer));
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(context.timers == NULL);

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_all_timers_for_device_prevents_callbacks(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timer_1 = NULL;
	indigo_timer *timer_2 = NULL;

	ASSERT_TRUE(indigo_set_timer(&device, 0.2, record_callback, &timer_1));
	ASSERT_TRUE(indigo_set_timer(&device, 0.2, record_alternate_callback, &timer_2));
	ASSERT_TRUE(timer_1 != NULL);
	ASSERT_TRUE(timer_2 != NULL);
	indigo_cancel_all_timers(&device);
	for (int i = 0; i < 100 && (timer_1 != NULL || timer_2 != NULL); i++) {
		indigo_usleep(10000);
	}
	ASSERT_TRUE(timer_1 == NULL);
	ASSERT_TRUE(timer_2 == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);
	ASSERT_TRUE(context.timers == NULL);

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_all_timers_leaves_other_devices_untouched(void) {
	reset_state();
	indigo_device_context context_1 = { 0 };
	indigo_device_context context_2 = { 0 };
	indigo_device device_1 = make_test_device(&context_1);
	indigo_device device_2 = make_test_device(&context_2);
	indigo_timer *timer_1 = NULL;
	indigo_timer *timer_2 = NULL;

	ASSERT_TRUE(indigo_set_timer(&device_1, 0.2, record_callback, &timer_1));
	ASSERT_TRUE(indigo_set_timer(&device_2, 0.2, record_alternate_callback, &timer_2));
	ASSERT_TRUE(context_1.timers == timer_1);
	ASSERT_TRUE(context_2.timers == timer_2);
	indigo_cancel_all_timers(&device_1);
	ASSERT_TRUE(timer_1 == NULL);
	ASSERT_TRUE(context_1.timers == NULL);
	ASSERT_TRUE(timer_2 != NULL);
	ASSERT_TRUE(context_2.timers == timer_2);
	ASSERT_TRUE(indigo_cancel_timer_sync(&device_2, &timer_2));
	ASSERT_TRUE(timer_2 == NULL);
	ASSERT_TRUE(context_2.timers == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);

	destroy_test_device(&context_2);
	destroy_test_device(&context_1);
	destroy_state();
}

static void cancel_all_timers_waits_for_running_callback_and_cancels_pending(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *running_timer = NULL;
	indigo_timer *pending_timer = NULL;

	ASSERT_TRUE(indigo_set_timer(&device, 0, long_running_callback, &running_timer));
	ASSERT_TRUE(indigo_set_timer(&device, 0.2, record_alternate_callback, &pending_timer));
	ASSERT_TRUE(wait_for_count(&state.running_callback_count, 1));
	indigo_cancel_all_timers(&device);
	ASSERT_TRUE(running_timer == NULL);
	ASSERT_TRUE(pending_timer == NULL);
	ASSERT_TRUE(state.callback_finished);
	indigo_usleep(250000);
	ASSERT_EQ_INT(1, state.running_callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);
	ASSERT_TRUE(context.timers == NULL);

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_all_timers_handles_concurrent_timer_completion(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timers[16] = { 0 };

	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(indigo_set_timer(&device, 0.001, record_callback, timers + i));
		ASSERT_TRUE(timers[i] != NULL);
	}
	indigo_usleep(500);
	indigo_cancel_all_timers(&device);
	for (int i = 0; i < (int)(sizeof(timers) / sizeof(timers[0])); i++) {
		ASSERT_TRUE(timers[i] == NULL);
	}
	ASSERT_TRUE(context.timers == NULL);
	ASSERT_TRUE(state.callback_count <= (int)(sizeof(timers) / sizeof(timers[0])));

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_all_timers_ignores_new_timers_created_by_running_callback(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *timer = NULL;
	cancel_all_followup_timer = NULL;

	ASSERT_TRUE(indigo_set_timer(&device, 0, cancel_all_schedules_followup_callback, &timer));
	ASSERT_TRUE(wait_for_flag(&state.callback_started));
	int64_t start_ns = now_ns();
	indigo_cancel_all_timers(&device);
	int64_t elapsed_ns = now_ns() - start_ns;

	ASSERT_TRUE(elapsed_ns < 300000000LL);
	ASSERT_TRUE(timer == NULL);
	ASSERT_TRUE(cancel_all_followup_timer != NULL);
	ASSERT_TRUE(context.timers == cancel_all_followup_timer);
	ASSERT_EQ_INT(0, state.alternate_callback_count);
	ASSERT_TRUE(indigo_cancel_timer_sync(&device, &cancel_all_followup_timer));
	ASSERT_TRUE(cancel_all_followup_timer == NULL);
	ASSERT_TRUE(context.timers == NULL);
	indigo_usleep(450000);
	ASSERT_EQ_INT(0, state.alternate_callback_count);

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_all_timers_on_empty_device_is_harmless(void) {
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);

	ASSERT_TRUE(context.timers == NULL);
	indigo_cancel_all_timers(&device);
	ASSERT_TRUE(context.timers == NULL);

	destroy_test_device(&context);
}

static void raw_timers_keep_separate_master_and_slave_device_list_ownership(void) {
	reset_state();
	indigo_device_context master_context = { 0 };
	indigo_device_context slave_context = { 0 };
	indigo_device master = make_test_device(&master_context);
	indigo_device slave = make_test_device(&slave_context);
	indigo_timer *master_timer = NULL;
	indigo_timer *slave_timer = NULL;

	slave.master_device = &master;
	ASSERT_TRUE(indigo_set_timer(&master, 0.2, record_callback, &master_timer));
	ASSERT_TRUE(indigo_set_timer(&slave, 0.2, record_alternate_callback, &slave_timer));
	ASSERT_TRUE(master_context.timers == master_timer);
	ASSERT_TRUE(slave_context.timers == slave_timer);
	indigo_cancel_all_timers(&master);
	ASSERT_TRUE(master_timer == NULL);
	ASSERT_TRUE(master_context.timers == NULL);
	ASSERT_TRUE(slave_timer != NULL);
	ASSERT_TRUE(slave_context.timers == slave_timer);
	indigo_cancel_all_timers(&slave);
	ASSERT_TRUE(slave_timer == NULL);
	ASSERT_TRUE(slave_context.timers == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);

	destroy_test_device(&slave_context);
	destroy_test_device(&master_context);
	destroy_state();
}

#if !defined(INDIGO_WINDOWS)
static indigo_timer *self_cancel_all_pending_timer = NULL;

static void self_cancel_all_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.callback_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	indigo_cancel_all_timers(device);

	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void child_cancel_all_timers_from_own_callback(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_timer *self_timer = NULL;
	self_cancel_all_pending_timer = NULL;

	ASSERT_TRUE(indigo_set_timer(&device, 0.2, record_alternate_callback, &self_cancel_all_pending_timer));
	ASSERT_TRUE(indigo_set_timer(&device, 0, self_cancel_all_callback, &self_timer));
	ASSERT_TRUE(wait_for_flag(&state.callback_finished));
	ASSERT_TRUE(self_timer == NULL);
	ASSERT_TRUE(self_cancel_all_pending_timer == NULL);
	indigo_usleep(250000);
	ASSERT_EQ_INT(1, state.callback_count);
	ASSERT_EQ_INT(0, state.alternate_callback_count);
	ASSERT_TRUE(context.timers == NULL);

	destroy_test_device(&context);
	destroy_state();
}

static void cancel_all_timers_from_timer_callback_does_not_deadlock(void) {
	assert_child_exits_without_deadlock(child_cancel_all_timers_from_own_callback);
}
#endif

static void queue_create_starts_worker_and_reports_ready(void) {
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	ASSERT_TRUE(queue != NULL);
	ASSERT_TRUE(queue->ready);
	ASSERT_FALSE(queue->abort);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
}

static void queue_asap_task_runs_promptly(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, record_callback, NULL);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_delayed_task_does_not_run_before_due_time(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0.15, record_callback, NULL);
	indigo_usleep(50000);
	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_executes_runnable_tasks_by_priority(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int low = 1;
	int high = 2;

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_barrier_callback, NULL);
	ASSERT_TRUE(wait_for_flag(&state.callback_started));
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_callback_a, &low, NULL);
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_URGENT, 0, queue_callback_b, &high, NULL);
	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	ASSERT_TRUE(wait_for_sequence_count(2));
	ASSERT_EQ_INT(2, state.sequence[0]);
	ASSERT_EQ_INT(1, state.sequence[1]);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_future_high_priority_task_does_not_block_due_low_priority_task(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int low = 1;
	int high = 2;

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_barrier_callback, NULL);
	ASSERT_TRUE(wait_for_flag(&state.callback_started));
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_callback_a, &low, NULL);
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_URGENT, 0.2, queue_callback_b, &high, NULL);
	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	ASSERT_TRUE(wait_for_sequence_count(1));
	ASSERT_EQ_INT(1, state.sequence[0]);
	ASSERT_EQ_INT(1, state.sequence_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_add_initializes_data_as_null_and_uses_plain_callback(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, record_callback, NULL);
	ASSERT_TRUE(wait_for_count(&state.callback_count, 1));
	ASSERT_EQ_INT(0, state.data_callback_count);
	ASSERT_TRUE(state.last_data == NULL);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_add_with_data_passes_exact_data_pointer(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int payload = 42;

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, record_data_callback, &payload, NULL);
	ASSERT_TRUE(wait_for_count(&state.data_callback_count, 1));
	ASSERT_TRUE(state.last_data == &payload);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_add_null_queue_is_harmless(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	int payload = 42;

	indigo_queue_add(NULL, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, record_callback, NULL);
	indigo_queue_add_with_data(NULL, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, record_data_callback, &payload, NULL);

	ASSERT_EQ_INT(0, state.callback_count);
	ASSERT_EQ_INT(0, state.data_callback_count);

	destroy_test_device(&context);
	destroy_state();
}

static void queue_task_with_mutex_runs_while_mutex_is_held(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	pthread_mutex_t task_mutex;
	pthread_mutex_init(&task_mutex, NULL);
	observed_queue_task_mutex = &task_mutex;

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_task_mutex_observing_callback, &task_mutex);
	ASSERT_TRUE(wait_for_count(&state.mutex_callback_count, 1));
	ASSERT_TRUE(state.mutex_was_locked);

	observed_queue_task_mutex = NULL;
	pthread_mutex_destroy(&task_mutex);
	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_callbacks_are_serialized_for_one_queue(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	for (int i = 0; i < 8; i++) {
		indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, record_parallel_callback, NULL);
	}
	ASSERT_TRUE(wait_for_count(&state.callback_count, 8));
	ASSERT_EQ_INT(1, state.max_parallel_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_inserting_earlier_task_wakes_worker(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int late = 1;
	int early = 2;

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0.25, queue_callback_a, &late, NULL);
	indigo_usleep(50000);
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0.01, queue_callback_b, &early, NULL);

	ASSERT_TRUE(wait_for_sequence_count(1));
	ASSERT_EQ_INT(2, state.sequence[0]);
	ASSERT_TRUE(wait_for_sequence_count(2));
	ASSERT_EQ_INT(1, state.sequence[1]);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_delayed_tasks_inserted_randomly_run_in_due_time_order(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int values[12];
	const int order[12] = { 7, 2, 10, 0, 5, 11, 1, 8, 3, 9, 4, 6 };

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_barrier_callback, NULL);
	ASSERT_TRUE(wait_for_flag(&state.callback_started));
	for (int i = 0; i < (int)(sizeof(order) / sizeof(order[0])); i++) {
		int value = order[i];
		values[value] = value;
		indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0.02 + value * 0.01, queue_callback_a, values + value, NULL);
	}
	pthread_mutex_lock(&state.mutex);
	state.callback_finished = true;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);

	ASSERT_TRUE(wait_for_sequence_count(12));
	for (int i = 0; i < 12; i++) {
		ASSERT_EQ_INT(i, state.sequence[i]);
	}

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_repeated_asap_insertion_from_producers_does_not_lose_tasks(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	pthread_t threads[4];
	queue_producer_context producers[4];
	bool seen[160] = { 0 };

	for (int i = 0; i < 4; i++) {
		producers[i].queue = queue;
		producers[i].device = &device;
		producers[i].first_value = i * 40;
		producers[i].count = 40;
		ASSERT_EQ_INT(0, pthread_create(threads + i, NULL, queue_asap_producer, producers + i));
	}
	for (int i = 0; i < 4; i++) {
		ASSERT_EQ_INT(0, pthread_join(threads[i], NULL));
	}
	ASSERT_TRUE(wait_for_sequence_count(160));
	ASSERT_EQ_INT(160, state.sequence_count);
	for (int i = 0; i < 160; i++) {
		ASSERT_TRUE(state.sequence[i] >= 0);
		ASSERT_TRUE(state.sequence[i] < 160);
		ASSERT_FALSE(seen[state.sequence[i]]);
		seen[state.sequence[i]] = true;
	}

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_callback_can_enqueue_followup_without_deadlock(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	queue_followup_context followup = { queue, 1, 2, 0 };

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_enqueue_followup_callback, &followup, NULL);
	ASSERT_TRUE(wait_for_sequence_count(2));
	ASSERT_EQ_INT(1, state.sequence[0]);
	ASSERT_EQ_INT(2, state.sequence[1]);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_callback_can_enqueue_delayed_followup(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	queue_followup_context followup = { queue, 1, 2, 0.15 };

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_enqueue_followup_callback, &followup, NULL);
	ASSERT_TRUE(wait_for_sequence_count(1));
	ASSERT_EQ_INT(1, state.sequence[0]);
	indigo_usleep(50000);
	ASSERT_EQ_INT(1, state.sequence_count);
	ASSERT_TRUE(wait_for_sequence_count(2));
	ASSERT_EQ_INT(2, state.sequence[1]);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_remove_waits_for_matching_running_task(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_slow_callback, NULL);
	ASSERT_TRUE(wait_for_count(&state.queue_running_count, 1));
	indigo_queue_remove(queue, &device, queue_slow_callback);
	ASSERT_TRUE(state.callback_finished);
	ASSERT_EQ_INT(1, state.queue_finished_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_remove_does_not_wait_for_nonmatching_running_task(void) {
	reset_state();
	indigo_device_context context_1 = { 0 };
	indigo_device_context context_2 = { 0 };
	indigo_device device_1 = make_test_device(&context_1);
	indigo_device device_2 = make_test_device(&context_2);
	indigo_queue *queue = indigo_queue_create(&device_1);

	indigo_queue_add(queue, &device_1, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_slow_callback, NULL);
	ASSERT_TRUE(wait_for_count(&state.queue_running_count, 1));
	indigo_queue_remove(queue, &device_2, record_callback);
	ASSERT_FALSE(state.callback_finished);
	ASSERT_TRUE(state.queue_running_count == 1);
	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	ASSERT_TRUE(state.callback_finished);
	ASSERT_EQ_INT(1, state.queue_finished_count);

	destroy_test_device(&context_2);
	destroy_test_device(&context_1);
	destroy_state();
}

static void queue_remove_from_worker_does_not_deadlock(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_remove_self_callback, queue, NULL);
	ASSERT_TRUE(wait_for_count(&state.queue_finished_count, 1));

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_remove_null_queue_is_harmless(void) {
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);

	indigo_queue_remove(NULL, &device, record_callback);
	indigo_queue_remove(NULL, NULL, NULL);

	destroy_test_device(&context);
}

static void queue_delete_empty_queue_clears_reference(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);

	destroy_test_device(&context);
	destroy_state();
}

static void queue_delete_waits_for_running_task_and_clears_reference(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_slow_callback, NULL);
	ASSERT_TRUE(wait_for_count(&state.queue_running_count, 1));
	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	ASSERT_TRUE(state.callback_finished);
	ASSERT_EQ_INT(1, state.queue_finished_count);

	destroy_test_device(&context);
	destroy_state();
}

static void queue_delete_discards_pending_delayed_tasks(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int payloads[4] = { 1, 2, 3, 4 };

	for (int i = 0; i < (int)(sizeof(payloads) / sizeof(payloads[0])); i++) {
		indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 1, queue_callback_a, payloads + i, NULL);
	}
	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	ASSERT_EQ_INT(0, state.sequence_count);

	destroy_test_device(&context);
	destroy_state();
}

static void queue_remove_deletes_matching_scheduled_tasks(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int removed = 1;
	int retained = 2;

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, &removed, NULL);
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_b, &retained, NULL);
	indigo_queue_remove(queue, &device, (indigo_timer_callback)queue_callback_a);

	ASSERT_TRUE(wait_for_sequence_count(1));
	ASSERT_EQ_INT(2, state.sequence[0]);
	indigo_usleep(100000);
	ASSERT_EQ_INT(1, state.sequence_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context);
	destroy_state();
}

static void queue_remove_device_null_removes_all_tasks_for_device(void) {
	reset_state();
	indigo_device_context context_1 = { 0 };
	indigo_device_context context_2 = { 0 };
	indigo_device device_1 = make_test_device(&context_1);
	indigo_device device_2 = make_test_device(&context_2);
	indigo_queue *queue = indigo_queue_create(&device_1);
	int removed_1 = 1;
	int removed_2 = 2;
	int retained = 3;

	indigo_queue_add_with_data(queue, &device_1, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, &removed_1, NULL);
	indigo_queue_add_with_data(queue, &device_1, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_b, &removed_2, NULL);
	indigo_queue_add_with_data(queue, &device_2, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, &retained, NULL);
	indigo_queue_remove(queue, &device_1, NULL);

	ASSERT_TRUE(wait_for_sequence_count(1));
	ASSERT_EQ_INT(3, state.sequence[0]);
	indigo_usleep(100000);
	ASSERT_EQ_INT(1, state.sequence_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context_2);
	destroy_test_device(&context_1);
	destroy_state();
}

static void queue_remove_null_device_removes_callback_across_devices(void) {
	reset_state();
	indigo_device_context context_1 = { 0 };
	indigo_device_context context_2 = { 0 };
	indigo_device device_1 = make_test_device(&context_1);
	indigo_device device_2 = make_test_device(&context_2);
	indigo_queue *queue = indigo_queue_create(&device_1);
	int removed_1 = 1;
	int removed_2 = 2;
	int retained = 3;

	indigo_queue_add_with_data(queue, &device_1, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, &removed_1, NULL);
	indigo_queue_add_with_data(queue, &device_2, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, &removed_2, NULL);
	indigo_queue_add_with_data(queue, &device_2, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_b, &retained, NULL);
	indigo_queue_remove(queue, NULL, (indigo_timer_callback)queue_callback_a);

	ASSERT_TRUE(wait_for_sequence_count(1));
	ASSERT_EQ_INT(3, state.sequence[0]);
	indigo_usleep(100000);
	ASSERT_EQ_INT(1, state.sequence_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context_2);
	destroy_test_device(&context_1);
	destroy_state();
}

static void queue_remove_null_filters_removes_all_pending_tasks(void) {
	reset_state();
	indigo_device_context context_1 = { 0 };
	indigo_device_context context_2 = { 0 };
	indigo_device device_1 = make_test_device(&context_1);
	indigo_device device_2 = make_test_device(&context_2);
	indigo_queue *queue = indigo_queue_create(&device_1);
	int payloads[4] = { 1, 2, 3, 4 };

	indigo_queue_add_with_data(queue, &device_1, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, payloads + 0, NULL);
	indigo_queue_add_with_data(queue, &device_1, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_b, payloads + 1, NULL);
	indigo_queue_add_with_data(queue, &device_2, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_a, payloads + 2, NULL);
	indigo_queue_add_with_data(queue, &device_2, INDIGO_TASK_PRIORITY_NORMAL, 0.1, queue_callback_b, payloads + 3, NULL);
	indigo_queue_remove(queue, NULL, NULL);

	indigo_usleep(200000);
	ASSERT_EQ_INT(0, state.sequence_count);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	destroy_test_device(&context_2);
	destroy_test_device(&context_1);
	destroy_state();
}

#if !defined(INDIGO_WINDOWS)
static void child_queue_delete_from_worker(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);

	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_delete_self_callback, &queue, NULL);
	ASSERT_TRUE(wait_for_count(&state.queue_finished_count, 1));
	ASSERT_TRUE(queue == NULL);
	ASSERT_EQ_INT(0, state.error_count);

	destroy_test_device(&context);
	destroy_state();
}

static void queue_delete_from_worker_does_not_deadlock(void) {
	assert_child_exits_without_deadlock(child_queue_delete_from_worker);
}
#endif

static void randomized_timer_churn_survives_many_operations(void) {
	reset_state();
	pthread_t threads[STRESS_WORKER_COUNT];
	stress_worker_context contexts[STRESS_WORKER_COUNT];

	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		contexts[i].worker = i;
		ASSERT_EQ_INT(0, pthread_create(threads + i, NULL, stress_worker, contexts + i));
	}
	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		ASSERT_EQ_INT(0, pthread_join(threads[i], NULL));
	}
	if (state.error_count != 0 || state.stress_callback_count == 0 || state.stress_alternate_callback_count == 0) {
		fprintf(stderr, "randomized_timer_churn seed base 0x9e3779b9\n");
	}
	ASSERT_EQ_INT(0, state.error_count);
	ASSERT_TRUE(state.stress_callback_count > 0);
	ASSERT_TRUE(state.stress_alternate_callback_count > 0);
	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		for (int j = 0; j < STRESS_SLOT_COUNT; j++) {
			ASSERT_TRUE(contexts[i].payloads[j].timer == NULL);
		}
	}

	destroy_state();
}

static void shared_timer_slot_churn_survives_competing_producers(void) {
	reset_state();
	pthread_t threads[STRESS_WORKER_COUNT];
	shared_stress_worker_context contexts[STRESS_WORKER_COUNT];
	stress_timer_payload payloads[STRESS_SLOT_COUNT];
	pthread_mutex_t locks[STRESS_SLOT_COUNT];
	const uint32_t seed_base = 0x53484152u;

	for (int i = 0; i < STRESS_SLOT_COUNT; i++) {
		payloads[i].timer = NULL;
		payloads[i].worker = 0;
		payloads[i].slot = i;
		payloads[i].magic = 0x54494d45u;
		pthread_mutex_init(locks + i, NULL);
	}
	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		contexts[i].payloads = payloads;
		contexts[i].locks = locks;
		contexts[i].payload_count = STRESS_SLOT_COUNT;
		contexts[i].worker = i;
		contexts[i].seed = seed_base ^ (uint32_t)(i * 0x10201u);
		ASSERT_EQ_INT(0, pthread_create(threads + i, NULL, shared_stress_worker, contexts + i));
	}
	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		ASSERT_EQ_INT(0, pthread_join(threads[i], NULL));
	}
	for (int i = 0; i < STRESS_SLOT_COUNT; i++) {
		indigo_cancel_timer_sync(NULL, &payloads[i].timer);
	}
	if (state.error_count != 0) {
		fprintf(stderr, "shared_timer_slot_churn seed base 0x%08x\n", seed_base);
	}
	ASSERT_EQ_INT(0, state.error_count);
	ASSERT_TRUE(state.stress_callback_count + state.stress_alternate_callback_count > 0);
	for (int i = 0; i < STRESS_SLOT_COUNT; i++) {
		ASSERT_TRUE(payloads[i].timer == NULL);
		pthread_mutex_destroy(locks + i);
	}

	destroy_state();
}

static void timer_storm_runs_all_callbacks_and_clears_references(void) {
	reset_state();
	indigo_timer *timers[STORM_TIMER_COUNT] = { 0 };
	stress_timer_payload payloads[STORM_TIMER_COUNT];

	for (int i = 0; i < STORM_TIMER_COUNT; i++) {
		payloads[i].timer = NULL;
		payloads[i].worker = 0;
		payloads[i].slot = i;
		payloads[i].magic = 0x54494d45u;
		double delay = (double)(i % 7) / 1000.0;
		ASSERT_TRUE(indigo_set_timer_with_data(NULL, delay, stress_callback, timers + i, payloads + i));
	}
	ASSERT_TRUE(wait_for_stress_callback_total(STORM_TIMER_COUNT));
	ASSERT_EQ_INT(0, state.error_count);
	ASSERT_EQ_INT(STORM_TIMER_COUNT, state.stress_callback_count);
	for (int i = 0; i < STORM_TIMER_COUNT; i++) {
		ASSERT_TRUE(wait_for_timer_reference_to_clear(timers + i));
	}

	destroy_state();
}

static void cancellation_storm_cancels_delayed_timers_from_threads(void) {
	reset_state();
	indigo_timer *timers[CANCELLATION_STORM_TIMER_COUNT] = { 0 };
	pthread_t threads[STRESS_WORKER_COUNT];
	cancellation_storm_context contexts[STRESS_WORKER_COUNT];

	for (int i = 0; i < CANCELLATION_STORM_TIMER_COUNT; i++) {
		ASSERT_TRUE(indigo_set_timer(NULL, 0.5, record_callback, timers + i));
		ASSERT_TRUE(timers[i] != NULL);
	}
	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		contexts[i].timers = timers;
		contexts[i].first = i * (CANCELLATION_STORM_TIMER_COUNT / STRESS_WORKER_COUNT);
		contexts[i].count = CANCELLATION_STORM_TIMER_COUNT / STRESS_WORKER_COUNT;
		ASSERT_EQ_INT(0, pthread_create(threads + i, NULL, cancellation_storm_worker, contexts + i));
	}
	for (int i = 0; i < STRESS_WORKER_COUNT; i++) {
		ASSERT_EQ_INT(0, pthread_join(threads[i], NULL));
	}
	for (int i = 0; i < CANCELLATION_STORM_TIMER_COUNT; i++) {
		ASSERT_TRUE(timers[i] == NULL);
	}
	indigo_usleep(550000);
	ASSERT_EQ_INT(0, state.callback_count);

	destroy_state();
}

static void mixed_device_list_storm_survives_random_cancel_all(void) {
	reset_state();
	indigo_device_context contexts[QUEUE_CHURN_DEVICE_COUNT] = { 0 };
	indigo_device devices[QUEUE_CHURN_DEVICE_COUNT];
	indigo_timer *timers[STORM_TIMER_COUNT] = { 0 };
	uint32_t seed = 0x44455653u;

	for (int i = 0; i < QUEUE_CHURN_DEVICE_COUNT; i++) {
		devices[i] = make_test_device(contexts + i);
	}
	for (int i = 0; i < STORM_TIMER_COUNT; i++) {
		int device_index = i % QUEUE_CHURN_DEVICE_COUNT;
		double delay = 0.002 + (double)(i % 50) / 1000.0;
		ASSERT_TRUE(indigo_set_timer(devices + device_index, delay, record_callback, timers + i));
	}
	for (int i = 0; i < 100; i++) {
		int device_index = (int)(stress_next_random(&seed) % QUEUE_CHURN_DEVICE_COUNT);
		indigo_cancel_all_timers(devices + device_index);
		indigo_usleep(500);
	}
	for (int i = 0; i < QUEUE_CHURN_DEVICE_COUNT; i++) {
		indigo_cancel_all_timers(devices + i);
		ASSERT_TRUE(contexts[i].timers == NULL);
	}
	for (int i = 0; i < STORM_TIMER_COUNT; i++) {
		ASSERT_TRUE(timers[i] == NULL);
	}
	ASSERT_TRUE(state.callback_count <= STORM_TIMER_COUNT);
	for (int i = 0; i < QUEUE_CHURN_DEVICE_COUNT; i++) {
		destroy_test_device(contexts + i);
	}
	destroy_state();
}

static void randomized_queue_churn_survives_producers_and_removal(void) {
	reset_state();
	indigo_device_context contexts[QUEUE_CHURN_DEVICE_COUNT] = { 0 };
	indigo_device devices[QUEUE_CHURN_DEVICE_COUNT];
	indigo_queue *queue = NULL;
	pthread_t producers[QUEUE_CHURN_PRODUCER_COUNT];
	pthread_t remover;
	queue_churn_payload payloads[QUEUE_CHURN_PRODUCER_COUNT * QUEUE_CHURN_TASKS_PER_PRODUCER];
	queue_churn_producer_context producer_contexts[QUEUE_CHURN_PRODUCER_COUNT];
	queue_churn_remover_context remover_context;
	const uint32_t seed_base = 0x51554555u;

	memset(payloads, 0, sizeof(payloads));
	for (int i = 0; i < QUEUE_CHURN_DEVICE_COUNT; i++) {
		devices[i] = make_test_device(contexts + i);
	}
	queue = indigo_queue_create(devices);
	for (int i = 0; i < QUEUE_CHURN_PRODUCER_COUNT; i++) {
		producer_contexts[i].queue = queue;
		producer_contexts[i].devices = devices;
		producer_contexts[i].payloads = payloads;
		producer_contexts[i].producer = i;
		producer_contexts[i].seed = seed_base ^ (uint32_t)(i * 0x1009u);
		ASSERT_EQ_INT(0, pthread_create(producers + i, NULL, queue_churn_producer, producer_contexts + i));
	}
	remover_context.queue = queue;
	remover_context.devices = devices;
	remover_context.seed = seed_base ^ 0xa5a5a5a5u;
	ASSERT_EQ_INT(0, pthread_create(&remover, NULL, queue_churn_remover, &remover_context));
	for (int i = 0; i < QUEUE_CHURN_PRODUCER_COUNT; i++) {
		ASSERT_EQ_INT(0, pthread_join(producers[i], NULL));
	}
	ASSERT_EQ_INT(0, pthread_join(remover, NULL));
	indigo_usleep(50000);
	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
	pthread_mutex_lock(&state.mutex);
	int callbacks_after_delete = state.stress_callback_count + state.stress_alternate_callback_count;
	pthread_mutex_unlock(&state.mutex);
	indigo_usleep(50000);
	if (state.error_count != 0 || state.max_parallel_count > 1) {
		fprintf(stderr, "randomized_queue_churn seed base 0x%08x\n", seed_base);
	}
	ASSERT_EQ_INT(0, state.error_count);
	ASSERT_TRUE(callbacks_after_delete > 0);
	ASSERT_EQ_INT(callbacks_after_delete, state.stress_callback_count + state.stress_alternate_callback_count);
	ASSERT_TRUE(state.max_parallel_count <= 1);

	for (int i = 0; i < QUEUE_CHURN_DEVICE_COUNT; i++) {
		destroy_test_device(contexts + i);
	}
	destroy_state();
}

int main(void) {
	indigo_set_log_level(INDIGO_LOG_PLAIN);
	const indigo_test_case tests[] = {
		{ "delay_to_time_handles_zero_and_orders_positive_delays", delay_to_time_handles_zero_and_orders_positive_delays },
		{ "delay_to_time_normalizes_larger_fractional_delays", delay_to_time_normalizes_larger_fractional_delays },
		{ "negative_delay_runs_promptly_and_uses_normalized_time", negative_delay_runs_promptly_and_uses_normalized_time },
		{ "zero_delay_timer_runs_promptly_and_clears_reference", zero_delay_timer_runs_promptly_and_clears_reference },
		{ "nonzero_delay_timer_does_not_fire_before_deadline", nonzero_delay_timer_does_not_fire_before_deadline },
		{ "set_timer_runs_callback_and_clears_reference", set_timer_runs_callback_and_clears_reference },
		{ "set_timer_with_data_passes_user_data", set_timer_with_data_passes_user_data },
		{ "set_timer_with_data_passes_null_user_data", set_timer_with_data_passes_null_user_data },
		{ "set_timer_with_mutex_runs_callback_while_mutex_is_locked", set_timer_with_mutex_runs_callback_while_mutex_is_locked },
		{ "cancel_timer_prevents_pending_callback", cancel_timer_prevents_pending_callback },
		{ "cancel_timer_sync_prevents_pending_callback_and_returns_true", cancel_timer_sync_prevents_pending_callback_and_returns_true },
		{ "cancel_timer_sync_without_reference_returns_false", cancel_timer_sync_without_reference_returns_false },
		{ "cancel_timer_sync_waits_for_running_callback", cancel_timer_sync_waits_for_running_callback },
		{ "cancel_timer_returns_false_for_running_callback_without_waiting", cancel_timer_returns_false_for_running_callback_without_waiting },
		{ "completing_callback_does_not_clear_newer_timer_reference", completing_callback_does_not_clear_newer_timer_reference },
#if !defined(INDIGO_WINDOWS)
		{ "cancel_timer_sync_from_own_callback_does_not_deadlock", cancel_timer_sync_from_own_callback_does_not_deadlock },
#endif
		{ "reschedule_timer_changes_delay_and_callback", reschedule_timer_changes_delay_and_callback },
		{ "reschedule_pending_timer_to_earlier_deadline_wakes_scheduler", reschedule_pending_timer_to_earlier_deadline_wakes_scheduler },
		{ "reschedule_pending_timer_to_later_deadline_prevents_old_deadline", reschedule_pending_timer_to_later_deadline_prevents_old_deadline },
		{ "reschedule_timer_from_callback_repeats_until_complete", reschedule_timer_from_callback_repeats_until_complete },
		{ "self_reschedule_can_switch_to_different_callback", self_reschedule_can_switch_to_different_callback },
		{ "reschedule_without_reference_fails", reschedule_without_reference_fails },
		{ "cancel_racing_repeated_reschedule_leaves_one_logical_outcome", cancel_racing_repeated_reschedule_leaves_one_logical_outcome },
		{ "reschedule_racing_natural_completion_never_leaves_stuck_reference", reschedule_racing_natural_completion_never_leaves_stuck_reference },
		{ "set_timer_with_non_null_reference_fails_cleanly", set_timer_with_non_null_reference_fails_cleanly },
		{ "set_timer_can_reuse_reference_after_immediate_completion", set_timer_can_reuse_reference_after_immediate_completion },
		{ "cancel_timer_twice_is_harmless", cancel_timer_twice_is_harmless },
		{ "cancel_timer_sync_twice_is_harmless", cancel_timer_sync_twice_is_harmless },
		{ "cancel_timer_sync_with_stale_reference_returns_false", cancel_timer_sync_with_stale_reference_returns_false },
		{ "stale_timer_handle_does_not_cancel_or_reschedule_newer_timer", stale_timer_handle_does_not_cancel_or_reschedule_newer_timer },
		{ "canceling_timer_blocked_on_user_mutex_does_not_block_scheduler", canceling_timer_blocked_on_user_mutex_does_not_block_scheduler },
		{ "cancel_many_pending_timers_out_of_deadline_order", cancel_many_pending_timers_out_of_deadline_order },
		{ "timers_for_same_device_run_in_parallel_by_default", timers_for_same_device_run_in_parallel_by_default },
		{ "timers_with_same_mutex_are_serialized", timers_with_same_mutex_are_serialized },
		{ "device_timer_is_linked_until_completion_and_unlinked_afterward", device_timer_is_linked_until_completion_and_unlinked_afterward },
		{ "canceling_device_timer_unlinks_it_from_device_list", canceling_device_timer_unlinks_it_from_device_list },
		{ "null_device_timer_is_not_linked_to_unrelated_device_list", null_device_timer_is_not_linked_to_unrelated_device_list },
		{ "cancel_all_timers_for_device_prevents_callbacks", cancel_all_timers_for_device_prevents_callbacks },
		{ "cancel_all_timers_leaves_other_devices_untouched", cancel_all_timers_leaves_other_devices_untouched },
		{ "cancel_all_timers_waits_for_running_callback_and_cancels_pending", cancel_all_timers_waits_for_running_callback_and_cancels_pending },
		{ "cancel_all_timers_handles_concurrent_timer_completion", cancel_all_timers_handles_concurrent_timer_completion },
		{ "cancel_all_timers_ignores_new_timers_created_by_running_callback", cancel_all_timers_ignores_new_timers_created_by_running_callback },
		{ "cancel_all_timers_on_empty_device_is_harmless", cancel_all_timers_on_empty_device_is_harmless },
		{ "raw_timers_keep_separate_master_and_slave_device_list_ownership", raw_timers_keep_separate_master_and_slave_device_list_ownership },
#if !defined(INDIGO_WINDOWS)
		{ "cancel_all_timers_from_timer_callback_does_not_deadlock", cancel_all_timers_from_timer_callback_does_not_deadlock },
#endif
		{ "queue_create_starts_worker_and_reports_ready", queue_create_starts_worker_and_reports_ready },
		{ "queue_asap_task_runs_promptly", queue_asap_task_runs_promptly },
		{ "queue_delayed_task_does_not_run_before_due_time", queue_delayed_task_does_not_run_before_due_time },
		{ "queue_executes_runnable_tasks_by_priority", queue_executes_runnable_tasks_by_priority },
		{ "queue_future_high_priority_task_does_not_block_due_low_priority_task", queue_future_high_priority_task_does_not_block_due_low_priority_task },
		{ "queue_add_initializes_data_as_null_and_uses_plain_callback", queue_add_initializes_data_as_null_and_uses_plain_callback },
		{ "queue_add_with_data_passes_exact_data_pointer", queue_add_with_data_passes_exact_data_pointer },
		{ "queue_add_null_queue_is_harmless", queue_add_null_queue_is_harmless },
		{ "queue_task_with_mutex_runs_while_mutex_is_held", queue_task_with_mutex_runs_while_mutex_is_held },
		{ "queue_callbacks_are_serialized_for_one_queue", queue_callbacks_are_serialized_for_one_queue },
		{ "queue_inserting_earlier_task_wakes_worker", queue_inserting_earlier_task_wakes_worker },
		{ "queue_delayed_tasks_inserted_randomly_run_in_due_time_order", queue_delayed_tasks_inserted_randomly_run_in_due_time_order },
		{ "queue_repeated_asap_insertion_from_producers_does_not_lose_tasks", queue_repeated_asap_insertion_from_producers_does_not_lose_tasks },
		{ "queue_callback_can_enqueue_followup_without_deadlock", queue_callback_can_enqueue_followup_without_deadlock },
		{ "queue_callback_can_enqueue_delayed_followup", queue_callback_can_enqueue_delayed_followup },
		{ "queue_remove_waits_for_matching_running_task", queue_remove_waits_for_matching_running_task },
		{ "queue_remove_does_not_wait_for_nonmatching_running_task", queue_remove_does_not_wait_for_nonmatching_running_task },
		{ "queue_remove_from_worker_does_not_deadlock", queue_remove_from_worker_does_not_deadlock },
		{ "queue_remove_null_queue_is_harmless", queue_remove_null_queue_is_harmless },
		{ "queue_delete_empty_queue_clears_reference", queue_delete_empty_queue_clears_reference },
		{ "queue_delete_waits_for_running_task_and_clears_reference", queue_delete_waits_for_running_task_and_clears_reference },
		{ "queue_delete_discards_pending_delayed_tasks", queue_delete_discards_pending_delayed_tasks },
		{ "queue_remove_deletes_matching_scheduled_tasks", queue_remove_deletes_matching_scheduled_tasks },
		{ "queue_remove_device_null_removes_all_tasks_for_device", queue_remove_device_null_removes_all_tasks_for_device },
		{ "queue_remove_null_device_removes_callback_across_devices", queue_remove_null_device_removes_callback_across_devices },
		{ "queue_remove_null_filters_removes_all_pending_tasks", queue_remove_null_filters_removes_all_pending_tasks },
#if !defined(INDIGO_WINDOWS)
		{ "queue_delete_from_worker_does_not_deadlock", queue_delete_from_worker_does_not_deadlock },
#endif
		{ "randomized_timer_churn_survives_many_operations", randomized_timer_churn_survives_many_operations },
		{ "shared_timer_slot_churn_survives_competing_producers", shared_timer_slot_churn_survives_competing_producers },
		{ "timer_storm_runs_all_callbacks_and_clears_references", timer_storm_runs_all_callbacks_and_clears_references },
		{ "cancellation_storm_cancels_delayed_timers_from_threads", cancellation_storm_cancels_delayed_timers_from_threads },
		{ "mixed_device_list_storm_survives_random_cancel_all", mixed_device_list_storm_survives_random_cancel_all },
		{ "randomized_queue_churn_survives_producers_and_removal", randomized_queue_churn_survives_producers_and_removal }
	};
	return indigo_run_tests("timer unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
