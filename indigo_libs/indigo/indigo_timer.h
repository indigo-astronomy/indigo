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
#include <stdint.h>
#include <pthread.h>
#include <time.h>
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
 *
 * The fields of this structure are private implementation details. They are
 * exposed only because existing source code uses `indigo_timer *` handles.
 * Callers must not read, write, allocate, copy, or embed timer objects.
 */
typedef struct indigo_timer {
	indigo_device *device;                    ///< device associated with timer
	void *callback;           								///< callback function pointer
	bool canceled;                            ///< timer is canceled (darwin only)
	uint64_t timer_id;
	struct indigo_timer **reference;
	struct indigo_timer *next;
	void *timer_data;
	bool has_data;
	pthread_mutex_t *timer_mutex;
	int state;
	struct timespec at;
	bool cancel_requested;
	bool reschedule_requested;
	bool completed;
	bool worker_started;
	int waiters;
	pthread_t callback_thread;
	pthread_cond_t completed_cond;
	struct indigo_timer *scheduler_next;
	struct indigo_timer *registry_next;
} indigo_timer;

/** Queue structures.
 *
 * The fields of these structures are private implementation details. Public
 * callers should use only `indigo_queue *` handles and the queue functions
 * declared below.
 */

#define INDIGO_TASK_PRIORITY_NORMAL   0
#define INDIGO_TASK_PRIORITY_HIGH     5
#define INDIGO_TASK_PRIORITY_TIME    10 // time critical tasks (e.g. guiding)
#define INDIGO_TASK_PRIORITY_URGENT  20 // urgent tasks (more urgent than time critical)

typedef struct indigo_queue_task {
	indigo_device *device;
	int priority;
	struct timespec at;
	indigo_timer_callback callback;
	void *data;
	bool has_data;
	pthread_mutex_t *task_mutex;
	struct indigo_queue_task *next;
} indigo_queue_task;

typedef struct indigo_queue {
	indigo_device *device;
	pthread_cond_t cond;
	pthread_t thread;
	indigo_queue_task *task;
	bool abort;
	bool ready; // guard against a lost wakeup race condition
	pthread_mutex_t mutex;
	indigo_queue_task *running_task;
	bool running;
	bool self_delete_requested;
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

/** Reschedule timer (if not null) with a different plain callback.
 * Any data callback payload is discarded.
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

/** Add task to queue. Higher signed priority values run first among due tasks.
 */
INDIGO_EXTERN void indigo_queue_add(indigo_queue *queue, indigo_device *device, int priority, double delay, indigo_timer_callback callback, pthread_mutex_t *task_mutex);

/** Add task with data. Higher signed priority values run first among due tasks.
 */
INDIGO_EXTERN void indigo_queue_add_with_data(indigo_queue *queue, indigo_device *device, int priority, double delay, indigo_timer_with_data_callback callback, void *data, pthread_mutex_t *task_mutex);

/** Remove tasks from queue for given device and handler
 */
INDIGO_EXTERN void indigo_queue_remove(indigo_queue *queue, indigo_device *device, indigo_timer_callback callback);

/** Remove all tasks, abort queue and free associated structure
 */
INDIGO_EXTERN void indigo_queue_delete(indigo_queue **queue);

#ifdef __cplusplus
}
#endif

#endif /* indigo_timer_h */
