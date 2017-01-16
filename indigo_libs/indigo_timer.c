//
//  indigo_timer.c
//  indigo
//
//  Created by Peter Polakovic on 16/01/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include "indigo_timer.h"

#include "indigo_driver.h"

#define NANO	1000000000L

int timer_count = 0;
indigo_timer *free_timer;

pthread_mutex_t free_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cancel_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *timer_func(indigo_timer *timer) {
	while (true) {
		while (timer->scheduled) {
			INDIGO_DEBUG(indigo_debug("timer #%d (of %d) used for %gs", timer->timer_id, timer_count, timer->delay));
			if (timer->delay > 0) {
				struct timespec end;
				clock_gettime(CLOCK_REALTIME, &end);
				end.tv_sec += (int)timer->delay;
				end.tv_nsec += NANO * (timer->delay - (int)timer->delay);
				while (!timer->canceled) {
					pthread_mutex_lock(&timer->mutex);
					int rc = pthread_cond_timedwait(&timer->cond, &timer->mutex, &end);
					pthread_mutex_unlock(&timer->mutex);
					if (rc == ETIMEDOUT)
						break;
				}
			}

			timer->scheduled = false;
			if (!timer->canceled) {
				timer->callback(timer->device);
			}
		}
		
		INDIGO_DEBUG(indigo_debug("timer #%d done", timer->timer_id));

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
		
		pthread_mutex_lock(&timer->mutex);
		while (!timer->wake)
			 pthread_cond_wait(&timer->cond, &timer->mutex);
		pthread_mutex_unlock(&timer->mutex);
	}
	return NULL;
}

indigo_timer *indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback) {
	indigo_timer *timer = NULL;
	pthread_mutex_lock(&free_timer_mutex);
	if (free_timer != NULL) {
		timer = free_timer;
		free_timer = free_timer->next;
		timer->wake = true;
		timer->canceled = false;
		timer->scheduled = true;
		timer->delay = delay;
		if ((timer->device = device) != NULL) {
			timer->next = DEVICE_CONTEXT->timers;
			DEVICE_CONTEXT->timers = timer;
		} else {
			timer->next = NULL;
		}
		timer->callback = callback;
		pthread_mutex_lock(&timer->mutex);
		pthread_cond_signal(&timer->cond);
		pthread_mutex_unlock(&timer->mutex);
	} else {
		timer = malloc(sizeof(indigo_timer));
		timer->timer_id = timer_count++;
		pthread_mutex_init(&timer->mutex, NULL);
		pthread_cond_init(&timer->cond, NULL);
		timer->canceled = false;
		timer->scheduled = true;
		if ((timer->device = device) != NULL) {
			timer->next = DEVICE_CONTEXT->timers;
			DEVICE_CONTEXT->timers = timer;
		} else {
			timer->next = NULL;
		}
		timer->delay = delay;
		timer->callback = callback;
		pthread_create(&timer->thread, NULL, (void * (*)(void*))timer_func, timer);
	}
	pthread_mutex_unlock(&free_timer_mutex);
	return timer;
}

// TODO: do we need device?

bool indigo_reschedule_timer(indigo_device *device, double delay, indigo_timer **timer) {
	bool result = false;
	pthread_mutex_lock(&cancel_timer_mutex);
	if (*timer != NULL) {
		(*timer)->delay = delay;
		(*timer)->scheduled = true;
		result = true;
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
	return result;
}

// TODO: do we need device?

bool indigo_cancel_timer(indigo_device *device, indigo_timer **timer) {
	bool result = false;
	pthread_mutex_lock(&cancel_timer_mutex);
	if (*timer != NULL) {
		(*timer)->canceled = true;
		(*timer)->scheduled = false;
		pthread_mutex_lock(&(*timer)->mutex);
		pthread_cond_signal(&(*timer)->cond);
		pthread_mutex_unlock(&(*timer)->mutex);
		*timer = NULL;
		result = true;
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
	return result;
}

void indigo_cancel_all_timers(indigo_device *device) {
	pthread_mutex_lock(&cancel_timer_mutex);
	indigo_timer *timer;
	while ((timer = DEVICE_CONTEXT->timers) != NULL) {
		DEVICE_CONTEXT->timers = timer->next;
		timer->device = NULL;
		timer->next = NULL;
		timer->canceled = true;
		timer->scheduled = true;
		pthread_mutex_lock(&timer->mutex);
		pthread_cond_signal(&timer->cond);
		pthread_mutex_unlock(&timer->mutex);
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
}
