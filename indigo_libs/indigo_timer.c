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

#include "indigo_timer.h"

#include "indigo_driver.h"

#define NANO	1000000000L

// TODO: abort timer on cancel
// TODO: cancel timers on detach_device

int timer_count = 0;
indigo_timer *free_timer;

pthread_mutex_t free_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cancel_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *timer_func(indigo_timer *timer) {
	while (true) {
		while (timer->scheduled) {
			indigo_log("timer #%d (of %d) used for %gs", timer->timer_id, timer_count, timer->delay);
			if (timer->delay > 0) {
				struct timespec end, now;
				clock_gettime(CLOCK_REALTIME, &now);
				end.tv_sec = now.tv_sec + (int)timer->delay;
				end.tv_nsec = now.tv_nsec + NANO * (timer->delay - (int)timer->delay);
				while (true) {
					long udelay = 1000000L * (end.tv_sec - now.tv_sec) + (end.tv_nsec - now.tv_nsec) / 1000L;
					if (udelay <= 0) {
						if (udelay < -100000L)
							indigo_log("timer #%d fired %ldms late", timer->timer_id, -udelay / 1000L);
						break;
					}
					usleep((unsigned int)udelay);
					clock_gettime(CLOCK_REALTIME, &now);
				}
			}

			if (!timer->canceled) {
				timer->scheduled = false;
				timer->callback(timer->device);
			}
		}
		
		pthread_mutex_lock(&free_timer_mutex);
		timer->wake = false;
		timer->next = free_timer;
		free_timer = timer;
		pthread_mutex_unlock(&free_timer_mutex);
		
		pthread_mutex_lock(&timer->mutex);
		while (!timer->wake)
			 pthread_cond_wait(&timer->cond, &timer->mutex);
		timer->wake = false;
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
		timer->next = NULL;
		timer->wake = true;
		timer->canceled = false;
		timer->scheduled = true;
		timer->device = device;
		timer->delay = delay;
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
		timer->device = device;
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
		*timer = NULL;
		result = true;
	}
	pthread_mutex_unlock(&cancel_timer_mutex);
	return result;
}
