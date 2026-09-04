# Timers and Handler Queues

This note describes the internal implementation model behind
`indigo_timer.c` and the timer/queue APIs declared in `indigo_timer.h`.
The public function names are intentionally unchanged, but the implementation
is built around explicit ownership, condition predicates, and short critical
sections.

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
