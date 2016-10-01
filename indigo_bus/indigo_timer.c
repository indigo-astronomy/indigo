//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include "indigo_timer.h"

static pthread_mutex_t timer_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

typedef struct {
  bool in_use;
  bool cancel;
  pthread_t thread;
  pthread_mutex_t timer_mutex;
  pthread_cond_t timer_cond;
  indigo_device *device;
  int timer_id;
  void *data;
  double delay;
  indigo_timer_callback callback;
} timer_type;


static timer_type *timers = NULL;

#define SECOND 1000000000


static void *thread_handler(timer_type *timer) {
  INDIGO_DEBUG(indigo_debug("timer thread started"));
  struct timespec time;
  pthread_mutex_init(&timer->timer_mutex, NULL);
  pthread_cond_init(&timer->timer_cond, NULL);
  while (true) {
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += (int)timer->delay;
    time.tv_nsec += (long)((timer->delay - (int)timer->delay) * 1000000000L);
    if (time.tv_nsec > SECOND) {
      time.tv_sec++;
      time.tv_nsec -= SECOND;
    }
    INDIGO_DEBUG(indigo_debug("timer %d wait started", timer->timer_id));
    pthread_mutex_lock(&timer->timer_mutex);
    while (!timer->cancel) {
      if (pthread_cond_timedwait(&timer->timer_cond, &timer->timer_mutex, &time) != 0)
        break;
    }
    pthread_mutex_unlock(&timer->timer_mutex);
    if (timer->cancel) {
      INDIGO_DEBUG(indigo_debug("timer %d wait canceled", timer->timer_id));
    } else {
      INDIGO_DEBUG(indigo_debug("timer %d wait finished", timer->timer_id));
      timer->callback(timer->device, timer->timer_id, timer->data, timer->delay);
    }
    //indigo_debug("timer thread suspend", timer->timer_id);
    pthread_mutex_lock(&timer->timer_mutex);
    timer->in_use = false;
    while (!timer->in_use)
      pthread_cond_wait(&timer->timer_cond, &timer->timer_mutex);
    pthread_mutex_unlock(&timer->timer_mutex);
    //indigo_debug("timer thread resume", timer->timer_id);
  }
}

indigo_result indigo_set_timer(indigo_device *device, int timer_id, void *data, double delay, indigo_timer_callback callback) {
  if (!pthread_mutex_lock(&timer_mutex)) {
    if (timers == NULL) {
      timers = malloc(INDIGO_MAX_TIMERS * sizeof(timer_type));
      memset(timers, 0, INDIGO_MAX_TIMERS * sizeof(timer_type));
    }
    for (int i = 0; i < INDIGO_MAX_TIMERS; i++) {
      timer_type *timer = &timers[i];
      if (!timer->in_use) {
        timer->in_use = true;
        timer->cancel = false;
        timer->device = device;
        timer->timer_id = timer_id;
        timer->data = data;
        timer->delay = delay;
        timer->callback = callback;
        if (timer->thread == NULL) {
          pthread_create(&timer->thread, NULL, (void*)(void *)thread_handler, timer);
        } else {
          pthread_mutex_lock(&timer->timer_mutex);
          pthread_cond_signal(&timer->timer_cond);
          pthread_mutex_unlock(&timer->timer_mutex);
        }
        pthread_mutex_unlock(&timer_mutex);
        return INDIGO_OK;
      }
    }
    pthread_mutex_unlock(&timer_mutex);
    return INDIGO_TOO_MANY_ELEMENTS;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_cancel_timer(indigo_device *device, int timer_id) {
  if (!pthread_mutex_lock(&timer_mutex)) {
    if (timers == NULL) {
      timers = malloc(INDIGO_MAX_TIMERS * sizeof(timer_type));
      memset(timers, 0, INDIGO_MAX_TIMERS * sizeof(timer_type));
    }
    for (int i = 0; i < INDIGO_MAX_TIMERS; i++) {
      timer_type *timer = &timers[i];
      if (timer->in_use && timer->device == device && timer->timer_id == timer_id) {
        pthread_mutex_lock(&timer->timer_mutex);
        timer->cancel = true;
        pthread_cond_signal(&timer->timer_cond);
        pthread_mutex_unlock(&timer->timer_mutex);
        pthread_mutex_unlock(&timer_mutex);
        return INDIGO_OK;
      }
    }
    pthread_mutex_unlock(&timer_mutex);
    return INDIGO_NOT_FOUND;
  }
  return INDIGO_LOCK_ERROR;
}

