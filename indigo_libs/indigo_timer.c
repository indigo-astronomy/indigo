// Copyright (c) 2017 CloudMakers, s. r. o.
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

#define utc_time(ts) clock_gettime(CLOCK_REALTIME, ts)

#define NANO	1000000000L

int timer_count = 0;
indigo_timer *free_timer = NULL;

pthread_mutex_t free_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cancel_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *timer_func(indigo_timer *timer) {
	pthread_detach(pthread_self());
	while (true) {
		while (timer->scheduled) {
			INDIGO_TRACE(indigo_trace("timer #%d - sleep for %gs (%p)", timer->timer_id, timer->delay, timer->reference));
			if (timer->delay > 0) {
				struct timespec end;
				utc_time(&end);
				end.tv_sec += (int)timer->delay;
				end.tv_nsec += NANO * (timer->delay - (int)timer->delay);
				normalize_timespec(&end);
				while (!timer->canceled) {
					pthread_mutex_lock(&timer->mutex);
					int rc = pthread_cond_timedwait(&timer->cond, &timer->mutex, &end);
					pthread_mutex_unlock(&timer->mutex);
					if (rc == ETIMEDOUT) {
						break;
					}
				}
			}
			timer->scheduled = false;
			if (!timer->canceled) {
				pthread_mutex_lock(&timer->callback_mutex);
				timer->callback_running = true;
				INDIGO_TRACE(indigo_trace("timer #%d - callback %p started (%p)", timer->timer_id, timer->callback, timer->reference));
				if (timer->data)
					((indigo_timer_with_data_callback)timer->callback)(timer->device, timer->data);
				else
					((indigo_timer_callback)timer->callback)(timer->device);
				timer->callback_running = false;
				if (!timer->scheduled && timer->reference)
					*timer->reference = NULL;
				INDIGO_TRACE(indigo_trace("timer #%d - callback %p finished (%p)", timer->timer_id, timer->callback, timer->reference));
				pthread_mutex_unlock(&timer->callback_mutex);
			} else {
				if (timer->reference)
					*timer->reference = NULL;
				INDIGO_TRACE(indigo_trace("timer #%d - canceled", timer->timer_id));
			}
		}
		INDIGO_TRACE(indigo_trace("timer #%d - done", timer->timer_id));
		pthread_mutex_lock(&cancel_timer_mutex);
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
		pthread_mutex_unlock(&cancel_timer_mutex);
		pthread_mutex_lock(&free_timer_mutex);
		timer->next = free_timer;
		free_timer = timer;
		timer->wake = false;
		pthread_mutex_unlock(&free_timer_mutex);
		INDIGO_TRACE(indigo_trace("timer #%d - released", timer->timer_id));	
		pthread_mutex_lock(&timer->mutex);
		while (!timer->wake)
			pthread_cond_wait(&timer->cond, &timer->mutex);
		pthread_mutex_unlock(&timer->mutex);
	}
	return NULL;
}

bool indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer) {
	return indigo_set_timer_with_data(device, delay, (indigo_timer_with_data_callback)callback, timer, NULL);
}

bool indigo_set_timer_at_s(indigo_device *device, char *time_str, indigo_timer_callback callback, indigo_timer **timer, void *data) {
	struct tm tm_time;
	time_t target_time;
	memset(&tm_time, 0, sizeof(struct tm));

	// Try to parse with seconds (yyyy-mm-dd hh:mm:ss)
	int year, month, day, hour, minute, second = 0;

	int items = sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
	if (items < 6) {
		items = sscanf(time_str, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);
	}
	if (items < 5) {
		indigo_error("Failed to parse datetime string '%s' (expected format: yyyy-mm-dd hh:mm:ss or yyyy-mm-dd hh:mm)", time_str);
		return false;
	}

	tm_time.tm_year = year - 1900;
	tm_time.tm_mon = month - 1;
	tm_time.tm_mday = day;
	tm_time.tm_hour = hour;
	tm_time.tm_min = minute;
	tm_time.tm_sec = second;
	tm_time.tm_isdst = 0;

	target_time = timegm(&tm_time);
	if (target_time == (time_t)-1) {
		indigo_error("Failed to convert parsed time to timestamp");
		return false;
	}

	double delay = difftime(target_time, time(NULL));
	if (delay < 0) {
		delay = 0;
	}
	return indigo_set_timer_with_data(device, delay, (indigo_timer_with_data_callback)callback, timer, data);
}

bool indigo_set_timer_at(indigo_device *device, long start_at, indigo_timer_with_data_callback callback, indigo_timer **timer, void *data) {
	double delay = difftime(start_at, time(NULL));
	if (delay < 0) {
		delay = 0;
	}
	return indigo_set_timer_with_data(device, delay, callback, timer, data);
}

bool indigo_set_timer_with_data(indigo_device *device, double delay, indigo_timer_with_data_callback callback, indigo_timer **timer, void *data) {
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
	pthread_mutex_lock(&free_timer_mutex);
	if (free_timer != NULL) {
		t = free_timer;
		INDIGO_TRACE(indigo_trace("timer #%d - reusing (%p)", t->timer_id, t));
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
		t->data = data;
		pthread_mutex_lock(&t->mutex);
		pthread_cond_signal(&t->cond);
		pthread_mutex_unlock(&t->mutex);
	} else {
		t = indigo_safe_malloc(sizeof(indigo_timer));
		t->timer_id = timer_count++;
		INDIGO_TRACE(indigo_trace("timer #%d - allocating (%p)", t->timer_id, t));
		pthread_mutex_init(&t->mutex, NULL);
		pthread_mutex_init(&t->callback_mutex, NULL);
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
		t->data = data;
		pthread_create(&t->thread, NULL, (void * (*)(void*))timer_func, t);
	}
	if (timer) {
		t->reference = timer;
		*timer = t;
	} else {
		t->reference = NULL;
	}
	pthread_mutex_unlock(&free_timer_mutex);
	return true;
}

// TODO: do we need device?

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
	pthread_mutex_lock(&cancel_timer_mutex);
	if (*timer != NULL && (*timer)->canceled == false) {
		if (*timer != *(*timer)->reference) {
			indigo_error("timer #%d - attempt to reschedule timer with outdated reference!", (*timer)->timer_id);
		} else {
			INDIGO_TRACE(indigo_trace("timer #%d - rescheduled for %gs", (*timer)->timer_id, (*timer)->delay));
			(*timer)->delay = delay;
			(*timer)->scheduled = true;
			(*timer)->callback = callback;
			result = true;
		}
	} else {
		indigo_error("Attempt to reschedule timer without reference or canceled timer!");
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
	return result;
}

// TODO: do we need device?

bool indigo_cancel_timer(indigo_device *device, indigo_timer **timer) {
	bool result = false;
	pthread_mutex_lock(&cancel_timer_mutex);
	if (*timer != NULL) {
		if (*timer != *(*timer)->reference) {
			indigo_error("timer #%d - attempt to cancel timer with outdated reference!", (*timer)->timer_id);
		} else {
			INDIGO_TRACE(indigo_trace("timer #%d - cancel requested", (*timer)->timer_id));
			(*timer)->canceled = true;
			(*timer)->scheduled = false;
			(*timer)->reference = NULL; // as far as it is cancel and forget we can't clear reference by timer_func
			pthread_mutex_lock(&(*timer)->mutex);
			pthread_cond_signal(&(*timer)->cond);
			pthread_mutex_unlock(&(*timer)->mutex);
			*timer = NULL;
			result = true;
		}
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
	return result;
}

bool indigo_cancel_timer_sync(indigo_device *device, indigo_timer **timer) {
	bool must_wait = false;
	indigo_timer *timer_buffer = NULL;
	pthread_mutex_lock(&cancel_timer_mutex);
	if (*timer != NULL) {
		if ((*timer)->reference != NULL && *timer != *(*timer)->reference) {
			indigo_error("Attempt to cancel timer with outdated reference!");
		} else {
			INDIGO_TRACE(indigo_trace("timer #%d - cancel requested", (*timer)->timer_id));
			(*timer)->canceled = true;
			(*timer)->scheduled = false;
      timer_buffer = *timer;
      must_wait = true;
			pthread_mutex_lock(&(timer_buffer)->mutex);
			pthread_cond_signal(&(timer_buffer)->cond);
			pthread_mutex_unlock(&(timer_buffer)->mutex);
			/* Save a local copy of the timer instance as *timer can be set
			 to NULL by *timer_func() after cancel_timer_mutex is released */
		}
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
	/* if must_wait == true then timer_buffer != NULL (see above) */
	if (must_wait) {
		INDIGO_TRACE(indigo_trace("timer #%d - waiting to finish", timer_buffer->timer_id));
		/* just wait for the callback to finish */
		pthread_mutex_lock(&(timer_buffer)->callback_mutex);
		pthread_mutex_unlock(&(timer_buffer)->callback_mutex);
		*timer = NULL;
	}
	/* if must_wait == true timer is canceled else it was not running */
	return must_wait;
}

void indigo_cancel_all_timers(indigo_device *device) {
	indigo_timer *timer;
	while (true) {
		pthread_mutex_lock(&cancel_timer_mutex);
		timer = DEVICE_CONTEXT->timers;
		if (timer)
			DEVICE_CONTEXT->timers = timer->next;
		pthread_mutex_unlock(&cancel_timer_mutex);
		if (timer == NULL) {
			break;
		}
		indigo_cancel_timer_sync(device, &timer);
	}
}
