# Timers and Handler Queues
Revision: 05.09.2026 (draft)

Authors: **Peter Polakovic** & **Rumen G. Bogdanovski**

e-mail: *peter.polakovic@cloudmakers.eu*, *rumenastro@gmail.com*

This note describes the internal implementation model behind
`indigo_timer.c` and the timer/queue APIs declared in `indigo_timer.h`.
The public function names are intentionally unchanged, but the implementation
is built around explicit ownership, condition predicates, and short critical
sections.

Drivers use both facilities. The timer API in `indigo_timer.h` is called
directly for polling, timeouts and any work that should run independently of
other activity on the device. Handler queues are normally reached through the
wrappers in `indigo_driver.h`, described in
[Driver-Facing Wrappers](#driver-facing-wrappers). [Examples](#examples) shows
the patterns both are meant to be used in.

## Goals

- Keep the existing public source API.
- Do not require binary ABI compatibility for the public struct layout.
- Run timer callbacks with the existing one-thread-per-timer behavior.
- Preserve timer parallelism across different timers by default.
- Serialize work only when a caller supplies a mutex, or when work is submitted
  through a handler queue.
- Make cancellation and deletion safe when they race with callback start,
  callback completion, rescheduling, and reference reuse.

## Clock Model

Timer and queue deadlines are stored as absolute `struct timespec` values.
When `CLOCK_MONOTONIC` is available, deadlines are based on the monotonic clock
so wall-clock changes do not make timers fire early or late. Otherwise the code
falls back to `CLOCK_REALTIME`.

Linux can bind a condition variable to `CLOCK_MONOTONIC` with
`pthread_condattr_setclock()`. macOS has `CLOCK_MONOTONIC` but the supported
deployment target does not provide that pthread condition-variable clock API, so
the implementation uses `pthread_cond_timedwait_relative_np()` there and
computes a relative wait from the monotonic absolute deadline. Other platforms
use ordinary absolute timed waits.

## Why A Scheduler Thread

The previous timer model put most timer ownership into the timer worker itself:
each timer had its own sleeping thread, and cancellation/rescheduling had to
coordinate directly with a thread that might still be sleeping, might be about
to run the callback, might already be running it, or might be tearing its own
timer object down. That shape makes edge cases hard to reason about because the
same object lifetime is touched from several directions at once.

The scheduler-thread model separates waiting from callback execution. One
thread owns the ordered pending list and decides when timers become due. Due
timers still get their own callback threads, so the externally visible
one-thread-per-timer callback behavior is preserved, but sleeping, wakeups,
reference validation, cancellation, and object teardown all pass through one
small synchronization point.

This gives the implementation a few important properties:

- a newly earlier timer only has to wake one scheduler condition variable;
- cancellation can remove a pending timer without racing its sleeping worker;
- running callbacks have a clear state and completion condition to wait on;
- stale `indigo_timer **` references can be checked against one live registry;
- callback threads can be joinable and reaped by the scheduler without asking
  callers to join anything;
- user callbacks never execute under the scheduler mutex, so driver code does
  not inherit the scheduler's internal lock ordering.

The alternative of a bounded callback pool was considered cleaner under very
large timer storms, but it would change the current parallelism model. INDIGO
drivers may already rely on independent timers being able to run concurrently,
so the reimplementation keeps one callback thread per due timer and uses the
scheduler only for coordination.

## Timer Scheduler Thread

Timers are coordinated by one process-local scheduler thread named
`Timer scheduler`. It is started lazily the first time a timer API needs it and
lives for the process lifetime. The scheduler thread does not run user
callbacks directly. Its job is to:

- own the pending timer list;
- sleep until the earliest pending deadline;
- wake when a newly inserted or rescheduled timer becomes the earliest timer;
- move due timers from pending state to running state;
- start one joinable callback thread for each due timer;
- join callback threads after they report completion;
- maintain a registry of live timer objects so stale public timer references
  are rejected before the implementation dereferences them.

The scheduler has one mutex and one condition variable. The mutex protects the
pending list, live registry, finished-worker list, timer state fields, device
timer links, public references, and waiter counts. User callbacks never run
while the scheduler mutex is held.

The scheduler thread is detached because callers never join it. Callback
threads are joinable because their stack and pthread resources must be reaped.
Callback threads enqueue their own `pthread_t` into the scheduler's
finished-worker list before returning; the scheduler joins those threads on its
next pass through the loop.

After `fork()`, the child process must not reuse thread state inherited from the
parent. On non-Windows platforms `pthread_atfork()` locks the scheduler mutex
before the fork, unlocks it in the parent, and in the child resets only ordinary
scheduler state while still holding that mutex before unlocking it. It does not
reinitialize pthread primitives in the child. Inherited timers are discarded:
their public reference slots and device timer-list ownership are cleared, so the
same slots are reusable. The first timer API used in the child then starts a new
scheduler thread for the child process. A process-id guard remains in the normal
timer API path so an unreset scheduler state is rejected instead of being
reinitialized concurrently by multiple child threads.

## Timer Lifecycle

Creating a timer allocates an `indigo_timer`, initializes its condition
variable, assigns a monotonically increasing `timer_id`, records the public
reference slot if one was supplied, optionally links the timer into the device's
timer list, registers it in the live-timer registry, and inserts it into the
pending list sorted by deadline and `timer_id`.

When the timer becomes due, the scheduler removes it from the pending list,
marks it running, and starts a callback thread named
`Timer callback #<timer_id>`. That thread optionally locks the caller-provided
timer mutex, invokes either the plain callback or the callback-with-data form,
then unlocks the mutex.

Timer callbacks can reschedule their own timer through the public reschedule
API. Self-reschedule does not allocate a new timer object. It marks the running
timer for reschedule and updates its next deadline and callback. When the
current callback returns, the callback thread either reinserts the same timer
into the pending list or completes it.

The callback thread tracks its current timer in thread-local storage while the
user callback is running. The timer implementation uses this to recognize
self-reschedule and self-cancel paths without scanning timer lists.

`indigo_reschedule_timer()` preserves the callback form and data payload.
`indigo_reschedule_timer_with_callback()` accepts a plain one-argument callback,
so it explicitly changes the timer to the plain callback form and discards any
previous data payload.

Completion unlinks the timer from its device list, clears the public reference
only if it still points to this exact timer object, unregisters the timer from
the live registry, wakes waiters, and frees the timer once no waiter still holds
it.

The callback-with-data form is selected by an explicit internal `has_data`
flag, not by the payload pointer. This allows `indigo_set_timer_with_data()` to
pass `NULL` as a valid user payload.

## Timer Cancellation

`indigo_cancel_timer()` is asynchronous:

- If the referenced timer is pending, it removes and completes it, clears the
  reference, and returns `true`.
- If the timer callback is already running, it requests cancellation of any
  future reschedule and returns `false` without waiting.
- If the reference is `NULL` or stale, it returns `false`.

`indigo_cancel_timer_sync()` is synchronous with respect to a live timer:

- If the timer is pending, it cancels and completes it, then returns `true`.
- If the timer is running on another thread, it requests cancellation, disables
  further self-reschedule, waits until the callback completes, and returns
  `true`.
- If it is called from the timer's own callback thread, it requests cancellation
  of future reschedule and clears the device/reference ownership that is safe to
  clear without waiting for itself.
- If the reference is `NULL` or stale, it returns `false`.

`indigo_cancel_all_timers(device)` uses snapshot semantics. It cancels timers
currently linked to the device and waits for callbacks that were already running
when the function took its snapshot. Timers created later by those running
callbacks are not part of the same cancellation operation; callers that need to
stop higher-level activity must prevent new scheduling at their own state level.

## Device Timers

Raw timers are linked to `DEVICE_CONTEXT->timers` for the exact device passed to
the timer API. Master and slave device contexts therefore keep separate raw
timer ownership.

`indigo_set_device_timer()` is a driver-level wrapper that schedules a timer
with the device mutex. If the device has a master, the master's device mutex is
used; otherwise the device's own mutex is used. This makes callbacks scheduled
through that wrapper serialize with other work using the same device mutex.

Plain `indigo_set_timer()` and `indigo_set_timer_with_data()` do not imply
device-level serialization. Multiple timers for the same device may run in
parallel unless a shared mutex is supplied.

## Handler Queues

Handler queues implement serialized, per-queue execution. They are used by the
`indigo_execute_handler()` family and can also be created directly with
`indigo_queue_create()`.

Each queue owns one worker thread, one mutex, and one condition variable. Queue
creation waits until the worker has set a `ready` predicate, which avoids a
lost wakeup between thread creation and the first wait.

Queue tasks store a device pointer, priority, deadline, callback, optional data
payload, optional task mutex, and next pointer. Pending tasks are inserted in
deadline order. When one or more tasks are due, the worker selects the highest
priority runnable task among the due tasks. Future high-priority tasks do not
block lower-priority tasks that are already due. Priority is a signed integer:
higher values run first, and negative values are valid. Tasks with equal
priority retain deadline/insertion order.

The queue worker never runs more than one task at a time. It removes a runnable
task from the pending list, records it as the running task, releases the queue
mutex, optionally locks the task mutex, invokes the callback, unlocks the task
mutex, reacquires the queue mutex, clears the running task, wakes waiters, and
frees the task.

The callback-with-data form is tracked by `indigo_queue_task.has_data`, so a
`NULL` data pointer is a valid payload for queued data callbacks.

Queue tasks have a default maximum run-time diagnostic threshold of 0.1 seconds.
If a task callback runs longer than its current threshold, the queue logs a
debug message after the callback returns. A running handler can call
`indigo_set_handler_max_run_time()` to raise the threshold for its own current
task, or pass 0 to disable the threshold for that task.

Queues also track their pending task count. If a queue grows beyond the default
limit of 100 pending tasks, it logs one debug message for that queue lifetime.
Callers can use `indigo_queue_set_max_pending_tasks()` to change the limit for
a queue, or set it to 0 for unlimited.

## Handler Queue Removal And Deletion

`indigo_queue_remove(queue, device, callback)` removes pending tasks matching
the supplied filters. A `NULL` device matches all devices. A `NULL` callback
matches all callbacks. If the currently running task also matches and the
caller is not the queue worker itself, removal waits until that running callback
finishes.

`indigo_queue_delete(&queue)` marks the queue aborted, removes pending tasks,
and wakes the worker. When called from another thread, it waits for any running
task, joins the worker thread, destroys synchronization primitives, frees the
queue, and clears the caller's pointer. When called from the queue worker, it
marks self-deletion and returns; the worker frees the queue after the current
callback unwinds.

## Driver-Facing Wrappers

Drivers normally do not call the queue API in `indigo_timer.h` at all. They use
the wrappers in `indigo_driver.h`, which own the queue for them: the queue is
created lazily on the first handler, it belongs to the **master device**, and it
is deleted by `indigo_device_detach()`. All slave devices of one master share
that master's queue, so their handlers are serialized against each other.

Every wrapper passes the master device mutex as the task mutex, so a queued
handler also excludes anything else holding that mutex, including timers
scheduled with `indigo_set_device_timer()`.

### Task Priorities

Priority is a signed integer; higher values run first among tasks that are
already due. A task scheduled for the future never preempts a due task,
whatever its priority.

| Constant | Value | Intended use |
| --- | ---: | --- |
| `INDIGO_TASK_PRIORITY_NORMAL` | 0 | Ordinary property change handlers |
| `INDIGO_TASK_PRIORITY_HIGH` | 5 | Work that should overtake ordinary handlers |
| `INDIGO_TASK_PRIORITY_TIME` | 10 | Time critical work, for example guiding |
| `INDIGO_TASK_PRIORITY_URGENT` | 20 | Aborts and guide pulse finalizers |

Values are ordinary integers, so intermediate and negative priorities are legal
if a driver needs them.

### Handler Wrappers

| Wrapper | Priority | Delay |
| --- | --- | --- |
| `indigo_execute_handler(device, handler)` | `NORMAL` | none |
| `indigo_execute_handler_with_data(device, handler, data)` | `NORMAL` | none |
| `indigo_execute_handler_in(device, delay, handler)` | `TIME` | given |
| `indigo_execute_handler_with_data_in(device, delay, handler, data)` | `TIME` | given |
| `indigo_execute_priority_handler(device, priority, handler)` | given | none |
| `indigo_execute_priority_handler_with_data(device, priority, handler, data)` | given | none |
| `indigo_execute_priority_handler_in(device, priority, delay, handler)` | given | given |
| `indigo_execute_priority_handler_with_data_in(device, priority, delay, handler, data)` | given | given |

Note the asymmetry in the first four rows: the immediate forms use
`INDIGO_TASK_PRIORITY_NORMAL`, but the `_in` forms use
`INDIGO_TASK_PRIORITY_TIME`. A driver that only adds a delay to an existing
`indigo_execute_handler()` call therefore also raises its priority by ten. Use
the explicit `indigo_execute_priority_handler_in()` form when the priority
matters.

Two wrappers remove queued work. Both match on the calling device, so they
never touch tasks belonging to a sibling device sharing the same queue:

- `indigo_cancel_pending_handlers(device)` drops every pending task for that
  device.
- `indigo_cancel_pending_handler(device, handler)` drops only the pending tasks
  bound to that one handler.

Both wait if the currently running task also matches, unless they are called
from the queue worker itself. A handler can therefore cancel its own siblings
without deadlocking.

`indigo_set_device_timer(device, delay, handler, &timer)` is the timer-side
counterpart. It schedules a normal timer with the master device mutex, so the
callback serializes with queued handlers even though it does not run on the
queue.

### Property Change Macros

The `indigo_bus.h` macros wrap the common change-handler shape. They copy the
incoming values, set the property busy, update it, and dispatch the handler with
`indigo_execute_handler()`, so they all run at `INDIGO_TASK_PRIORITY_NORMAL`:

- `INDIGO_COPY_VALUES_PROCESS_CHANGE(property, handler)` ignores the request if
  the property is already busy.
- `INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(property, handler)` accepts it even
  when busy.
- `INDIGO_COPY_TARGETS_PROCESS_CHANGE(property, handler)` copies number targets
  rather than values.
- `INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(property, handler)` and
  `INDIGO_COPY_TARGETS_PROCESS_SYNC_CHANGE(property, handler)` call the handler
  inline instead of queueing it. Use them only for handlers that cannot block,
  since they run on the caller's thread.

## Examples

### Property change handler

The common case. The change handler validates and acknowledges, the queued
handler does the blocking I/O:

```c
static void focuser_position_callback(indigo_device *device) {
	if (focuser_move(device, FOCUSER_POSITION_ITEM->number.target)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		indigo_property_copy_targets(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_execute_handler(device, focuser_position_callback);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}
```

The same thing with the macro:

```c
	if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_callback);
		return INDIGO_OK;
	}
```

### Abort that has to overtake queued work

An abort is useless behind a queue of pending moves, so it runs at
`INDIGO_TASK_PRIORITY_URGENT` and drops the pending work first:

```c
	if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		indigo_cancel_pending_handler(device, focuser_position_callback);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, focuser_abort_callback);
		return INDIGO_OK;
	}
```

### Deferred handler with an explicit priority

A guide pulse starts the motion now and stops it after the pulse length. The
finalizer must not sit behind ordinary handlers, so the priority is stated
explicitly rather than inherited from the `_in` form:

```c
static void guider_guide_ra_callback(indigo_device *device) {
	int west = GUIDER_GUIDE_WEST_ITEM->number.value;
	int east = GUIDER_GUIDE_EAST_ITEM->number.value;
	guider_start_ra(device, west, east);
	if (west > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, west / 1000.0, guider_guide_ra_finish_callback);
	} else if (east > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, east / 1000.0, guider_guide_ra_finish_callback);
	}
}
```

### Passing data to a handler

`has_data` is tracked separately from the pointer, so `NULL` is a valid payload:

```c
static void wheel_slot_callback(indigo_device *device, void *data) {
	int slot = (int)(intptr_t)data;
	...
}

	indigo_execute_handler_with_data(device, wheel_slot_callback, (void *)(intptr_t)slot);
```

### Periodic polling with a timer

Polling belongs on a timer, not on the queue. The callback reschedules itself,
which reuses the same timer object rather than allocating a new one:

```c
static void ccd_temperature_callback(indigo_device *device) {
	ccd_read_temperature(device);
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	indigo_reschedule_timer(device, 5.0, &PRIVATE_DATA->temperature_timer);
}

static indigo_result ccd_attach(indigo_device *device) {
	...
	indigo_set_device_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);
	return INDIGO_OK;
}
```

Cancel it before the device goes away. `indigo_cancel_timer_sync()` guarantees
the callback is no longer running when it returns, which is what makes freeing
private data safe:

```c
	indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
```

### Choosing between a timer and a handler queue

Both run work off the caller's thread, and both are first-class: neither
replaces the other. They differ in what they guarantee:

| | Timer | Handler queue |
| --- | --- | --- |
| Concurrency | Independent timers run in parallel | One task at a time per master device |
| Ordering | None between timers | Deadline, then priority |
| Cost per dispatch | One thread created per firing | Reuses the queue worker |
| Cancellation | Per timer reference | By device, or by device and handler |
| Natural fit | Polling, timeouts, pulse endings | Property change handlers, device I/O |

The choice follows from the concurrency the work needs, not from cost. Use a
timer when the work must run independently of whatever else the device is
doing, and a queue when it must be serialized with it. Cost only matters at the
margin: handler dispatch is cheaper than timer dispatch, because the queue
worker persists while every timer firing creates a thread, so a very frequent
short task is worth placing on a queue if its ordering requirements allow it.

## Locking Invariants

- The scheduler mutex protects timer object lifetime and all timer lists.
- Queue mutexes protect only their own queue state.
- Scheduler code never locks caller-provided timer or task mutexes.
- User callbacks do not run under the scheduler mutex or a queue mutex.
- Public timer references are validated against the live registry before the
  pointed-to timer object is used.
- Public references are cleared only when they still point to the same timer or
  queue object being completed or deleted.
- Condition-variable waits always use a predicate guarded by the corresponding
  mutex.

## Behavioral Non-Goals

- There is no bounded global callback pool; due timers still run on one
  callback thread per timer.
- There is no scheduler shutdown API.
- The public timer and queue structs remain exposed for source compatibility,
  but their fields are private implementation details and are not ABI-stable.
