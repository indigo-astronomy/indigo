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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO timers
 \file indigo_timer.h
 */

#ifndef indigo_timer_h
#define indigo_timer_h

#include <stdio.h>
#include <pthread.h>

#include "indigo_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Timer callback function prototype.
 */
typedef void (*indigo_timer_callback)(indigo_device *device);

/** Timer structure.
 */
typedef struct indigo_timer {
	indigo_device *device;                    ///< device associated with timer
	indigo_timer_callback callback;           ///< callback function pointer
	bool canceled;                            ///< timer is canceled (darwin only)
	bool scheduled;
	double delay;
	bool wake;
	int timer_id;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	pthread_t thread;
	struct indigo_timer *next;
} indigo_timer;

/* fix timespec so that abs(tv_nsec) < 1s */
#define SEC_NS    1000000000LL       /* 1 sec in nanoseconds */
static inline void normalize_timespec(struct timespec *ts) {
	if((1 <= ts->tv_sec ) || ((0 == ts->tv_sec) && (0 <= ts->tv_nsec))) {
		/* timespec is non-negative, so ns >= 1s and ns < 0s are not ok */
		if (SEC_NS <= ts->tv_nsec) {
			ts->tv_nsec -= SEC_NS;
			ts->tv_sec++;
		} else if ( 0 > (ts)->tv_nsec ) {
			ts->tv_nsec += SEC_NS;
			ts->tv_sec--;
		}
	} else {
		/* timespec is negative, so ns <= -1s and ns > 0s are not ok */
		if ( (-1 * SEC_NS) >= ts->tv_nsec ) {
			ts->tv_nsec += SEC_NS;
			ts->tv_sec--;
		} else if ( 0 < ts->tv_nsec ) {
			ts->tv_nsec -= SEC_NS;
			ts->tv_sec++;
		}
	}
}

/** Set timer.
 */
extern indigo_timer *indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback);

/** Rescheduled timer (if not null).
 */
extern bool indigo_reschedule_timer(indigo_device *device, double delay, indigo_timer **timer);

/** Cancel timer.
 */
extern bool indigo_cancel_timer(indigo_device *device, indigo_timer **timer);

/** Cancel all timers for given device.
 */
extern void indigo_cancel_all_timers(indigo_device *device);

#ifdef __cplusplus
}
#endif

#endif /* indigo_timer_h */
