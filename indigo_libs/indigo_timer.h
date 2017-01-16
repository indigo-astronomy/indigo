//
//  indigo_timer.h
//  indigo
//
//  Created by Peter Polakovic on 16/01/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_timer_h
#define indigo_timer_h

#include <stdio.h>

#include "indigo_bus.h"

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

#endif /* indigo_timer_h */
