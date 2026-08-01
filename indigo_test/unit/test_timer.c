// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <errno.h>
#include <pthread.h>
#include <string.h>

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
	int sequence[16];
	int sequence_count;
	bool callback_started;
	bool callback_finished;
	bool mutex_was_locked;
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

static void record_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.callback_count++;
	pthread_cond_broadcast(&state.cond);
	pthread_mutex_unlock(&state.mutex);
}

static void record_alternate_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.alternate_callback_count++;
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

static void mutex_observing_callback(indigo_device *device) {
	pthread_mutex_lock(&state.mutex);
	state.mutex_callback_count++;
	state.mutex_was_locked = pthread_mutex_trylock(&DEVICE_CONTEXT->device_mutex) == EBUSY;
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

static bool wait_for_sequence_count(int expected_count) {
	return wait_for_count(&state.sequence_count, expected_count);
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

static void reschedule_without_reference_fails(void) {
	indigo_timer *timer = NULL;
	ASSERT_FALSE(indigo_reschedule_timer(NULL, 0.01, &timer));
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

static void queue_executes_runnable_tasks_by_priority(void) {
	reset_state();
	indigo_device_context context = { 0 };
	indigo_device device = make_test_device(&context);
	indigo_queue *queue = indigo_queue_create(&device);
	int low = 1;
	int high = 2;

	pthread_mutex_lock(&queue->thread_mutex);
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_callback_a, &low, NULL);
	indigo_queue_add_with_data(queue, &device, INDIGO_TASK_PRIORITY_URGENT, 0, queue_callback_b, &high, NULL);
	pthread_mutex_unlock(&queue->thread_mutex);

	ASSERT_TRUE(wait_for_sequence_count(2));
	ASSERT_EQ_INT(2, state.sequence[0]);
	ASSERT_EQ_INT(1, state.sequence[1]);

	indigo_queue_delete(&queue);
	ASSERT_TRUE(queue == NULL);
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

int main(void) {
	const indigo_test_case tests[] = {
		{ "delay_to_time_handles_zero_and_orders_positive_delays", delay_to_time_handles_zero_and_orders_positive_delays },
		{ "set_timer_runs_callback_and_clears_reference", set_timer_runs_callback_and_clears_reference },
		{ "set_timer_with_data_passes_user_data", set_timer_with_data_passes_user_data },
		{ "set_timer_with_mutex_runs_callback_while_mutex_is_locked", set_timer_with_mutex_runs_callback_while_mutex_is_locked },
		{ "cancel_timer_prevents_pending_callback", cancel_timer_prevents_pending_callback },
		{ "cancel_timer_sync_waits_for_running_callback", cancel_timer_sync_waits_for_running_callback },
		{ "reschedule_timer_changes_delay_and_callback", reschedule_timer_changes_delay_and_callback },
		{ "reschedule_without_reference_fails", reschedule_without_reference_fails },
		{ "cancel_all_timers_for_device_prevents_callbacks", cancel_all_timers_for_device_prevents_callbacks },
		{ "queue_executes_runnable_tasks_by_priority", queue_executes_runnable_tasks_by_priority },
		{ "queue_remove_deletes_matching_scheduled_tasks", queue_remove_deletes_matching_scheduled_tasks }
	};
	return indigo_run_tests("timer unit tests", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
