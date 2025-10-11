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
 \file indigo_timer.h
 */

#ifndef indigo_timer_h
#define indigo_timer_h
#include <stdio.h>
#include <pthread.h>
#include <indigo/indigo_bus.h>

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Timer callback function prototype.
 */
typedef void (*indigo_timer_callback)(indigo_device *device);
typedef void (*indigo_timer_with_data_callback)(indigo_device *device, void *timer_data);

/** Timer structure.
 */
typedef struct indigo_timer {
	indigo_device *device;                    ///< device associated with timer
	void *callback;           								///< callback function pointer
	bool canceled;                            ///< timer is canceled (darwin only)
	bool scheduled;
	bool callback_running;
	double delay;
	bool wake;
	int timer_id;
	pthread_cond_t cond;
	pthread_mutex_t cond_mutex;
	pthread_t thread;
	struct indigo_timer **reference;
	struct indigo_timer *next;
	void *timer_data;
	pthread_mutex_t thread_mutex;
	pthread_mutex_t *timer_mutex;
} indigo_timer;

/** Queue structure.
 */

typedef struct indigo_queue_element {
	indigo_device *device;
	struct timespec at;
	indigo_timer_callback callback;
	pthread_mutex_t *element_mutex;
	struct indigo_queue_element *next;
} indigo_queue_element;

typedef struct indigo_queue {
	indigo_device *device;
	pthread_cond_t cond;
	pthread_mutex_t cond_mutex;
	pthread_t thread;
	indigo_queue_element *element;
	int queue_id;
	bool abort;
	pthread_mutex_t thread_mutex;
} indigo_queue;

/** Translate delay into absolute time.
 */
INDIGO_EXTERN struct timespec indigo_delay_to_time(double delay);

/** Set timer.
 */
INDIGO_EXTERN bool indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer);

/** Set timer with arbitrary data.
 */
INDIGO_EXTERN bool indigo_set_timer_with_data(indigo_device *device, double delay, indigo_timer_with_data_callback callback, indigo_timer **timer, void *timer_data);

/** Set timer with arbitrary mutex.
 */
INDIGO_EXTERN bool indigo_set_timer_with_mutex(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer, pthread_mutex_t *timer_mutex);

/** Rescheduled timer (if not null).
 */
INDIGO_EXTERN bool indigo_reschedule_timer(indigo_device *device, double delay, indigo_timer **timer);

/** Rescheduled timer (if not null) with different handler.
 */
INDIGO_EXTERN bool indigo_reschedule_timer_with_callback(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer);

/** Cancel timer.
 */
INDIGO_EXTERN bool indigo_cancel_timer(indigo_device *device, indigo_timer **timer);

/** Cancel timer and wait to cancel.
 */
INDIGO_EXTERN bool indigo_cancel_timer_sync(indigo_device *device, indigo_timer **timer);

/** Cancel all timers for given device.
 */
INDIGO_EXTERN void indigo_cancel_all_timers(indigo_device *device);

/** Create queue
 */
INDIGO_EXTERN indigo_queue *indigo_queue_create(indigo_device *device);

/** Add task to queue
 */
INDIGO_EXTERN void indigo_queue_add(indigo_queue *queue, indigo_device *device, double delay, indigo_timer_callback callback, pthread_mutex_t *element_mutex);

/** Remove elements from queue for given device
 */
INDIGO_EXTERN void indigo_queue_remove(indigo_queue *queue, indigo_device *device);

/** Remove all elements, abort queue and free associated structure
 */
INDIGO_EXTERN void indigo_queue_delete(indigo_queue **queue);

#ifdef __cplusplus
}
#endif

#endif /* indigo_timer_h */
