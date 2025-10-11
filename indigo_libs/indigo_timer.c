// Copyright (c) 2017-2025 CloudMakers, s. r. o.
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

/** INDIGO timers
 \file indigo_timer.c
 */

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <indigo/indigo_timer.h>
#include <indigo/indigo_driver.h>

//#ifdef __MACH__ /* Mac OSX prior Sierra is missing clock_gettime() */
//#include <mach/clock.h>
//#include <mach/mach.h>
//void utc_time(struct timespec *ts) {
//	clock_serv_t cclock;
//	mach_timespec_t mts;
//	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
//	clock_get_time(cclock, &mts);
//	mach_port_deallocate(mach_task_self(), cclock);
//	ts->tv_sec = mts.tv_sec;
//	ts->tv_nsec = mts.tv_nsec;
//}
//#endif

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
	tp->tv_sec = (time_t)(time_100ns / SEC_NS);
	tp->tv_nsec = (long)((time_100ns % SEC_NS) * 100);
	return 0;
}
#endif

#define utc_time(ts) clock_gettime(CLOCK_REALTIME, ts)

static int timer_count = 0;
static indigo_timer *free_timer = NULL;

pthread_mutex_t timers_mutex = PTHREAD_MUTEX_INITIALIZER;

struct timespec indigo_delay_to_time(double delay) {
	struct timespec time = { 0, 0 };
	if (delay == 0) {
		return time;
	}
	utc_time(&time);
	time.tv_sec += (int)delay;
	time.tv_nsec += (long)(SEC_NS * (delay - (int)delay));
	if ((1 <= time.tv_sec ) || ((0 == time.tv_sec) && (0 <= time.tv_nsec))) {
		if (SEC_NS <= time.tv_nsec) {
			time.tv_nsec -= SEC_NS;
			time.tv_sec++;
		} else if (0 > time.tv_nsec) {
			time.tv_nsec += SEC_NS;
			time.tv_sec--;
		}
	} else {
		if ((-1 * SEC_NS) >= time.tv_nsec) {
			time.tv_nsec += SEC_NS;
			time.tv_sec--;
		} else if (0 < time.tv_nsec) {
			time.tv_nsec -= SEC_NS;
			time.tv_sec++;
		}
	}
	return time;
}

static void *timer_func(indigo_timer *timer) {
	pthread_detach(pthread_self());
	indigo_rename_thread("Timer #%d", timer->timer_id);
	while (true) {
		while (timer->scheduled) {
			// INDIGO_TRACE(indigo_trace("timer #%d - sleep for %gs (%p)", timer->timer_id, timer->delay, timer->reference));
			if (timer->delay > 0) {
				struct timespec end = indigo_delay_to_time(timer->delay);
				while (!timer->canceled) {
					pthread_mutex_lock(&timer->cond_mutex);
					int rc = pthread_cond_timedwait(&timer->cond, &timer->cond_mutex, &end);
					pthread_mutex_unlock(&timer->cond_mutex);
					if (rc == ETIMEDOUT) {
						break;
					}
				}
			}
			timer->scheduled = false;
			if (!timer->canceled) {
				if (timer->timer_mutex) {
					pthread_mutex_lock(timer->timer_mutex);
				}
				pthread_mutex_lock(&timer->thread_mutex);
				timer->callback_running = true;
				// INDIGO_TRACE(indigo_trace("timer #%d - callback %p started (%p)", timer->timer_id, timer->callback, timer->reference));
				if (timer->timer_data) {
					((indigo_timer_with_data_callback)timer->callback)(timer->device, timer->timer_data);
				} else {
					((indigo_timer_callback)timer->callback)(timer->device);
				}
				timer->callback_running = false;
				// INDIGO_TRACE(indigo_trace("timer #%d - callback %p finished (%p)", timer->timer_id, timer->callback, timer->reference));
				pthread_mutex_unlock(&timer->thread_mutex);
				if (timer->timer_mutex) {
					pthread_mutex_unlock(timer->timer_mutex);
				}
			}
		}
		// INDIGO_TRACE(indigo_trace("timer #%d - done", timer->timer_id));
		pthread_mutex_lock(&timers_mutex);
		if (timer->reference) {
			*timer->reference = NULL;
		}
		indigo_device *device = timer->device;
		if (device != NULL) {
			if (DEVICE_CONTEXT->timers == timer) {
				DEVICE_CONTEXT->timers = timer->next;
			} else {
				indigo_timer *previous = DEVICE_CONTEXT->timers;
				while (previous->next != NULL) {
					if (previous->next == timer) {
						previous->next = timer->next;
						break;
					}
					previous = previous->next;
				}
			}
		}
		timer->next = free_timer;
		free_timer = timer;
		timer->wake = false;
		pthread_mutex_unlock(&timers_mutex);
		// INDIGO_TRACE(indigo_trace("timer #%d - released", timer->timer_id));
		pthread_mutex_lock(&timer->cond_mutex);
		while (!timer->wake)
			pthread_cond_wait(&timer->cond, &timer->cond_mutex);
		pthread_mutex_unlock(&timer->cond_mutex);
	}
	return NULL;
}

static bool set_timer(indigo_device *device, double delay, indigo_timer_with_data_callback callback, indigo_timer **timer, void *timer_data, pthread_mutex_t *timer_mutex) {
	indigo_timer *t = NULL;
	int retry = 0;
	// This is to fix the race between end of process, e.g. exposure, and end of its timer handler
	while (timer && *timer) {
		if (retry++ == 1000) {
			indigo_error("Attempt to set timer with non-NULL reference");
			return false;
		}
		indigo_usleep(100);
	}
	if (retry) {
		double retry_time = 0.0001 * retry;
		indigo_error("Spent %gs waiting for the timer reference", retry_time);
		delay -= retry_time;
		if (delay < 0) {
			delay = 0;
		}
	}
	pthread_mutex_lock(&timers_mutex);
	if (free_timer != NULL) {
		t = free_timer;
		// INDIGO_TRACE(indigo_trace("timer #%d - reusing (%p)", t->timer_id, t));
		free_timer = free_timer->next;
		t->wake = true;
		t->callback_running = false;
		t->canceled = false;
		t->scheduled = true;
		t->delay = delay;
		if ((t->device = device) != NULL) {
			t->next = DEVICE_CONTEXT->timers;
			DEVICE_CONTEXT->timers = t;
		} else {
			t->next = NULL;
		}
		t->callback = callback;
		t->timer_data = timer_data;
		t->timer_mutex = timer_mutex;
		pthread_mutex_lock(&t->cond_mutex);
		pthread_cond_signal(&t->cond);
		pthread_mutex_unlock(&t->cond_mutex);
	} else {
		t = indigo_safe_malloc(sizeof(indigo_timer));
		t->timer_id = timer_count++;
		// INDIGO_TRACE(indigo_trace("timer #%d - allocating (%p)", t->timer_id, t));
		pthread_mutex_init(&t->cond_mutex, NULL);
		pthread_mutex_init(&t->thread_mutex, NULL);
		pthread_cond_init(&t->cond, NULL);
		t->canceled = false;
		t->callback_running = false;
		t->scheduled = true;
		if ((t->device = device) != NULL) {
			t->next = DEVICE_CONTEXT->timers;
			DEVICE_CONTEXT->timers = t;
		} else {
			t->next = NULL;
		}
		t->delay = delay;
		t->callback = callback;
		t->timer_data = timer_data;
		t->timer_mutex = timer_mutex;
		pthread_create(&t->thread, NULL, (void * (*)(void*))timer_func, t);
	}
	if (timer) {
		t->reference = timer;
		*timer = t;
	} else {
		t->reference = NULL;
	}
	pthread_mutex_unlock(&timers_mutex);
	return true;
}

bool indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer) {
	return set_timer(device, delay, (indigo_timer_with_data_callback)callback, timer, NULL, NULL);
}

bool indigo_set_timer_with_data(indigo_device *device, double delay, indigo_timer_with_data_callback callback, indigo_timer **timer, void *timer_data) {
	return set_timer(device, delay, (indigo_timer_with_data_callback)callback, timer, timer_data, NULL);
}

bool indigo_set_timer_with_mutex(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer, pthread_mutex_t *timer_mutex) {
	return set_timer(device, delay, (indigo_timer_with_data_callback)callback, timer, NULL, timer_mutex);
}

bool indigo_reschedule_timer(indigo_device *device, double delay, indigo_timer **timer) {
	if (*timer != NULL) {
		return indigo_reschedule_timer_with_callback(device, delay, (*timer)->callback, timer);
	} else {
		indigo_error("Attempt to reschedule timer without reference!");
		return false;
	}
}
	
bool indigo_reschedule_timer_with_callback(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer) {
	bool result = false;
	pthread_mutex_lock(&timers_mutex);
	if (*timer != NULL && (*timer)->canceled == false) {
		if (*timer != *(*timer)->reference) {
			indigo_error("timer #%d - attempt to reschedule timer with outdated reference!", (*timer)->timer_id);
		} else {
			// INDIGO_TRACE(indigo_trace("timer #%d - rescheduled for %gs", (*timer)->timer_id, (*timer)->delay));
			(*timer)->delay = delay;
			(*timer)->scheduled = true;
			(*timer)->callback = callback;
			result = true;
		}
	} else {
		indigo_error("Attempt to reschedule timer without reference or canceled timer!");
	}
	pthread_mutex_unlock(&timers_mutex);
	return result;
}

bool indigo_cancel_timer(indigo_device *device, indigo_timer **timer) {
	bool result = false;
	pthread_mutex_lock(&timers_mutex);
	if (*timer != NULL) {
		if (*timer != *(*timer)->reference) {
			indigo_error("timer #%d - attempt to cancel timer with outdated reference!", (*timer)->timer_id);
		} else if (!(*timer)->callback_running) {
			// INDIGO_TRACE(indigo_trace("timer #%d - cancel requested", (*timer)->timer_id));
			(*timer)->canceled = true;
			(*timer)->scheduled = false;
			(*timer)->reference = NULL; // as far as it is cancel and forget we can't clear reference by timer_func
			pthread_mutex_lock(&(*timer)->cond_mutex);
			pthread_cond_signal(&(*timer)->cond);
			pthread_mutex_unlock(&(*timer)->cond_mutex);
			*timer = NULL;
			result = true;
		}
	}
	pthread_mutex_unlock(&timers_mutex);
	return result;
}

bool indigo_cancel_timer_sync(indigo_device *device, indigo_timer **timer) {
	bool must_wait = false;
	indigo_timer *timer_buffer = NULL;
	pthread_mutex_lock(&timers_mutex);
	if (*timer != NULL) {
		if ((*timer)->reference != NULL && *timer != *(*timer)->reference) {
			indigo_error("Attempt to cancel timer with outdated reference!");
		} else {
			// INDIGO_TRACE(indigo_trace("timer #%d - cancel requested", (*timer)->timer_id));
			(*timer)->canceled = true;
			(*timer)->scheduled = false;
      timer_buffer = *timer;
			if ((*timer)->callback_running) {
				must_wait = true;
			} else {
				pthread_mutex_lock(&(timer_buffer)->cond_mutex);
				pthread_cond_signal(&(timer_buffer)->cond);
				pthread_mutex_unlock(&(timer_buffer)->cond_mutex);
			}
			/* Save a local copy of the timer instance as *timer can be set
			 to NULL by *timer_func() after cancel_thread_mutex is released */
		}
	}
	pthread_mutex_unlock(&timers_mutex);
	/* if must_wait == true then timer_buffer != NULL (see above) */
	if (must_wait) {
		// INDIGO_TRACE(indigo_trace("timer #%d - waiting to finish", timer_buffer->timer_id));
		/* just wait for the callback to finish */
		pthread_mutex_lock(&timer_buffer->thread_mutex);
		pthread_mutex_unlock(&timer_buffer->thread_mutex);
		*timer = NULL;
	}
	/* if must_wait == true timer is canceled else it was not running */
	return must_wait;
}

void indigo_cancel_all_timers(indigo_device *device) {
	indigo_timer *timer;
	while (true) {
		pthread_mutex_lock(&timers_mutex);
		timer = DEVICE_CONTEXT->timers;
		if (timer) {
			DEVICE_CONTEXT->timers = timer->next;
		}
		pthread_mutex_unlock(&timers_mutex);
		if (timer == NULL) {
			break;
		}
		indigo_cancel_timer_sync(device, &timer);
	}
}

int queue_count = 0;

static inline int timespec_cmp(const struct timespec *a, const struct timespec *b) {
	if (a->tv_sec < b->tv_sec) return -1;
	if (a->tv_sec > b->tv_sec) return 1;
	if (a->tv_nsec < b->tv_nsec) return -1;
	if (a->tv_nsec > b->tv_nsec) return 1;
	return 0;
}

// wrapper for executing queue element = scheduled call of handler

static void *queue_func(indigo_queue *queue) {
	indigo_rename_thread("Queue %s", queue->device->name);
	// wakeup indigo_queue_create
	pthread_mutex_lock(&queue->cond_mutex);
	pthread_cond_signal(&queue->cond);
	pthread_mutex_unlock(&queue->cond_mutex);
	// main loop waiting for wakeup signal or timeout
	while (!queue->abort) {
		pthread_mutex_lock(&queue->thread_mutex);
		pthread_mutex_lock(&timers_mutex);
		indigo_queue_element *element = queue->element;
		pthread_mutex_unlock(&timers_mutex);
		pthread_mutex_lock(&queue->cond_mutex);
		if (element && (element->at.tv_sec != 0 || element->at.tv_nsec != 0)) { // wait for timeout for the next task
			pthread_cond_timedwait(&queue->cond, &queue->cond_mutex, &element->at);
		} else if (element == NULL) { // no task, wait for wakeup
			pthread_cond_wait(&queue->cond, &queue->cond_mutex);
		}
		pthread_mutex_unlock(&queue->cond_mutex); // this is to make indigo_queue_remove wait for currently executed task
		// inner loop executing all scheduled tasks
		while (!queue->abort) {
			pthread_mutex_lock(&timers_mutex);
			indigo_queue_element *element = queue->element;
			struct timespec now;
			utc_time(&now);
			if (element && timespec_cmp(&element->at, &now) <= 0) {
				queue->element = element->next; // if next tasks exists and it is time to execute it, remove it from queue head
			} else {
				pthread_mutex_unlock(&timers_mutex);
				break; // otherwise exit inner loop and go to sleep
			}
			pthread_mutex_unlock(&timers_mutex);
			if (element->element_mutex) { // if there is specific mutex for task, lock it
				pthread_mutex_lock(element->element_mutex);
			}
			element->callback(element->device);
			if (element->element_mutex) { // if there is specific mutex for task, unlock it
				pthread_mutex_unlock(element->element_mutex);
			}
		}
		pthread_mutex_unlock(&queue->thread_mutex);
	}
	return NULL;
}

// create queue, device should be master device if we need serialize access to shared connection. otherwise it may be any device.

indigo_queue *indigo_queue_create(indigo_device *device) {
	// create queue structure
	indigo_queue *queue = indigo_safe_malloc(sizeof(indigo_queue));
	queue->device = device;
	queue->queue_id = queue_count++;
	pthread_mutex_init(&queue->cond_mutex, NULL);
	pthread_cond_init(&queue->cond, NULL);
	pthread_mutex_init(&queue->thread_mutex, NULL);
	// start background thread
	pthread_create(&queue->thread, NULL, (void * (*)(void*))queue_func, queue);
	// wait for thread to signal it is ready to process tasks to make indigo_queue_create synchronous
	pthread_mutex_lock(&queue->cond_mutex);
	pthread_cond_wait(&queue->cond, &queue->cond_mutex);
	pthread_mutex_unlock(&queue->cond_mutex);
	return queue;
}

// add task to queue. note queue carries device which is usually master device while task may be related to some of slave devices

void indigo_queue_add(indigo_queue *queue, indigo_device *device, double delay, indigo_timer_callback callback, pthread_mutex_t *element_mutex) {
	indigo_queue_element *element = indigo_safe_malloc(sizeof(indigo_queue_element));
	element->device = device;
	element->at = indigo_delay_to_time(delay);
	element->callback = callback;
	element->element_mutex = element_mutex;
	pthread_mutex_lock(&timers_mutex);
	if (queue->element == NULL) { // if queue is empty, insert at the beginning
		queue->element = element;
	} else if (timespec_cmp(&element->at, &queue->element->at) < 0) { // if queue is not empty, but element should be inserted before existing head
		element->next = queue->element;
		queue->element = element;
	} else { // if queue is not empty, insert in the proper place
		indigo_queue_element *next = queue->element;
		while (next->next != NULL && timespec_cmp(&next->next->at, &element->at) <= 0) {
			next = next->next;
		}
		element->next = next->next;
		next->next = element;
	}
	pthread_mutex_unlock(&timers_mutex);
	// wakeup queue_func to rescan queue
	pthread_mutex_lock(&queue->cond_mutex);
	pthread_cond_signal(&queue->cond);
	pthread_mutex_unlock(&queue->cond_mutex);
}

// remove scheduled tasks from queue, if device is NULL, remove all tasks otherwise remove tasks related to device

void indigo_queue_remove(indigo_queue *queue, indigo_device *device) {
	if (queue) {
		pthread_mutex_lock(&timers_mutex);
		indigo_queue_element *element = queue->element;
		if (device == NULL) {
			// just remove all elements
			while (element) {
				indigo_queue_element *next = element->next;
				indigo_safe_free(element);
				element = next;
			}
			queue->element = NULL;
		} else {
			// remove only tasks related to particular device
			// check head first
			while (element && element->device == device) {
				indigo_queue_element *next = element->next;
				indigo_safe_free(element);
				element = next;
			}
			queue->element = element;
			// them rest of queue
			while (element) {
				indigo_queue_element *next = element->next;
				while (next && next->device == device) {
					next = element->next;
					indigo_safe_free(element->next);
					element->next = next;
				}
				element = element->next;
			}
		}
		pthread_mutex_unlock(&timers_mutex);
		// wakeup queue_func to rescan queue
		pthread_mutex_lock(&queue->cond_mutex);
		pthread_cond_signal(&queue->cond);
		pthread_mutex_unlock(&queue->cond_mutex);
		// wait for currently running task to be synchronous, but not for indigo_queue_remove called from currently executed task
		if (pthread_self() != queue->thread) {
			pthread_mutex_lock(&queue->thread_mutex);
			pthread_mutex_unlock(&queue->thread_mutex);
		}
	}
}

void indigo_queue_delete(indigo_queue **queue) {
	if (*queue) {
		// exit queue_func master loop to exit
		(*queue)->abort = true;
		// remove all tasks
		indigo_queue_remove(*queue, NULL);
		// release queue structure
		indigo_safe_free(*queue);
		*queue = NULL;
	}
}
