# indigo_timer.c Refactor Notes

## Goal

Rewrite `indigo_timer.c` and, if necessary, the private layout in
`indigo/indigo_timer.h` while preserving the public API and observable behavior
used by existing INDIGO drivers.

The current implementation has grown around races in timer references, detached
worker reuse, callback completion waits, and the handler queue. The replacement
should be designed as concurrency infrastructure rather than patched in place.

## Public API To Preserve

The following functions and callback typedefs are part of the public surface and
must keep their names, signatures, and broad semantics:

- `indigo_timer_callback`
- `indigo_timer_with_data_callback`
- `indigo_delay_to_time()`
- `indigo_set_timer()`
- `indigo_set_timer_with_data()`
- `indigo_set_timer_with_mutex()`
- `indigo_reschedule_timer()`
- `indigo_reschedule_timer_with_callback()`
- `indigo_cancel_timer()`
- `indigo_cancel_timer_sync()`
- `indigo_cancel_all_timers()`
- `indigo_queue_create()`
- `indigo_queue_add()`
- `indigo_queue_add_with_data()`
- `indigo_queue_remove()`
- `indigo_queue_delete()`

The priority constants in `indigo_timer.h` are also public and are used by
`indigo_driver.c` and generated drivers.

`struct indigo_timer`, `struct indigo_queue_task`, and `struct indigo_queue` are
currently exposed in the header even though callers normally treat them as
opaque handles. Binary ABI compatibility for their field layout is not required
for this refactor. Preserve source compatibility for public functions and
typedefs, but make these structs internally coherent and document that their
fields are private implementation details.

## Observed Usage Patterns

Legacy timer API:

- `indigo_set_timer(device, 0, callback, NULL)` is widely used as "run soon on
  another thread" from change-property callbacks.
- Many drivers store `indigo_timer *` in private data and expect the pointer to
  become `NULL` when the one-shot timer completes or is canceled.
- Polling callbacks commonly reschedule themselves with
  `indigo_reschedule_timer(device, delay, &PRIVATE_DATA->timer)`.
- `indigo_reschedule_timer_with_callback()` is used by camera exposure code to
  change both deadline and callback.
- `indigo_cancel_timer()` is used as a best-effort nonblocking cancellation,
  often for exposure/guiding abort paths. Success means the pending callback was
  prevented; it may fail/return false if the callback is already running.
- `indigo_cancel_timer_sync()` is used during disconnect and detach to avoid
  freeing device/private data while a timer callback may still touch it.
- Its return value is almost never used. The only in-tree behavioral dependency
  found during this review is `indigo_drivers/ccd_ssag/indigo_ccd_ssag.c`,
  where disconnect aborts an exposure only if `indigo_cancel_timer_sync()`
  returns true.
- `indigo_cancel_all_timers(device)` is called from base detach paths and some
  agents. It must not deadlock when called from a callback belonging to the same
  device.
- `indigo_set_device_timer()` in `indigo_driver.c` wraps
  `indigo_set_timer_with_mutex()` with the device or master-device mutex.

Handler queue API:

- `indigo_execute_handler*()` in `indigo_driver.c` maps logical slave devices to
  the master device queue but passes the original logical device to the handler.
- Queue tasks are expected to run serially under `DEVICE_CONTEXT->device_mutex`
  for the master queue, preventing concurrent hardware access.
- `indigo_execute_handler()` is normal priority and ASAP.
- `indigo_execute_handler_in()` currently uses `INDIGO_TASK_PRIORITY_TIME`, not
  normal priority, so delayed polling finalizers can outrank ordinary queued
  work.
- Priority ordering matters for guiding and abort paths:
  higher-priority runnable tasks should execute before lower-priority runnable
  tasks, while future tasks should not block earlier due tasks.
- `indigo_cancel_pending_handlers(device)` removes queued, not-yet-started tasks
  for the logical device. It waits for a matching running task to finish when
  called from another thread.
- Direct queue users exist, but most drivers go through `indigo_execute_handler*()`.

Documented guidance in `DRIVER_DEVELOPMENT_BASICS.md`:

- Timer callbacks run in a separate thread.
- `indigo_cancel_timer_sync()` is important on disconnect/detach, but should not
  be called directly from the main change-property bus callback because it may
  deadlock.
- INDIGO 3.0 prefers per-device handler queues for new driver work. Queue
  handlers run one at a time in the background and can be prioritized.

Unit tests in `indigo_test/unit/test_timer.c` already cover:

- delay conversion,
- callback execution and reference clearing,
- data callbacks,
- mutex-wrapped callbacks,
- pending cancellation,
- sync cancellation waiting for a running callback,
- rescheduling delay/callback,
- device-wide timer cancellation,
- queue priority,
- queue removal and queue deletion.

These tests are useful but not sufficient for the suspected race/deadlock
classes.

## Current Implementation Risks

- Global `timers_mutex` protects both the timer pool/device timer lists and all
  queue task lists. This creates unnecessary coupling and can create lock-order
  hazards between unrelated timers and queues.
- Timer objects are recycled by detached per-timer threads. A stale external
  `indigo_timer *` can point to a reused object unless every access verifies the
  reference pointer correctly.
- Some state (`scheduled`, `canceled`, `callback_running`, `wake`, `delay`,
  callback pointers, data pointers, and reference pointers) is read/written under
  different mutexes or without the condition-variable mutex that owns the wait
  predicate.
- The timed-wait loop checks `canceled` outside the condition mutex, so it is
  exposed to data races and lost wakeups.
- `set_timer()` waits for `*timer == NULL` by polling outside the timer lock.
  This is a symptom of a lifecycle race between callback completion and external
  reference clearing.
- `indigo_cancel_timer_sync()` waits by locking `thread_mutex`, which is also
  used around callback execution. That is not a condition-variable predicate and
  is fragile if cancellation races with callback start/finish or object reuse.
- `indigo_cancel_timer_sync()` returns `must_wait`, so it returns false when it
  successfully cancels a pending timer. That is surprising for a boolean API.
  Because only one in-tree call site currently uses the return value, the
  implementation may either preserve the historical return meaning or change it
  to success/failure after first updating that call site explicitly.
- `indigo_cancel_all_timers()` removes a timer from the device list before
  calling `indigo_cancel_timer_sync(device, &timer)`. Because the local pointer
  is not the timer's registered reference, this can trip the outdated-reference
  logic and leave original references uncleared in some races.
- Queue wait predicates are split between `queue->cond_mutex` and
  `timers_mutex`. A task can be enqueued/removed while the queue worker decides
  what to wait for, making correctness depend on careful signalling rather than
  one mutex owning the predicate.
- Queue tasks do not initialize all fields in `indigo_queue_add()`; in
  particular `data` should be explicitly set to `NULL`.
- Queue deletion sets `abort`, calls `indigo_queue_remove()`, then joins. The
  current lock order and wait-for-running-task behavior need careful treatment
  if deletion is called from the queue thread or while a task tries to delete its
  own queue.

## Proposed Design

Use two independent subsystems in the implementation:

1. A timer scheduler with one manager thread and per-timer completion state.
2. A per-queue worker with a private queue mutex/condition variable.

### Timer Scheduler

Represent every timer as a heap-owned object with:

- callback metadata: device, callback, optional data, optional callback mutex,
  external reference address;
- state: pending, running, completed, canceled;
- absolute due time;
- generation number for stale-handle detection;
- linked-list links for scheduler and device membership;
- mutex/condition variable for completion waiters.

Use one scheduler mutex/condition variable to own the pending timer list and all
timer state transitions. The scheduler thread waits on the earliest due timer
using the same monotonic clock used by `indigo_delay_to_time()` on Linux.

When a timer becomes due:

- remove it from the pending scheduler list while holding the scheduler mutex;
- mark it running;
- release scheduler lock;
- optionally lock `timer_mutex`;
- invoke the callback;
- unlock `timer_mutex`;
- reacquire scheduler lock;
- if the callback rescheduled the same timer, put it back into the pending list;
  otherwise complete it, clear the external reference if still matching, unlink
  it from the device timer list, and broadcast completion.

Avoid detached reusable per-timer worker threads. Use a one-thread-per-timer
callback execution model, but make those worker threads joinable and owned by an
explicit lifecycle/reaper path. This preserves the high parallelism of the
legacy API while removing the object-reuse and completion-wait races from the
old implementation.

Keep timer callbacks parallel across timers for the same device. Legacy timer
callers that need serialization must continue to use `indigo_set_device_timer()`
or `indigo_set_timer_with_mutex()`. INDIGO 3.0 handler queues remain the
serialized execution path by design.

Important semantics:

- `indigo_set_timer()` with a non-NULL reference should fail if `*timer` is
  already non-NULL instead of polling indefinitely. If preserving the historical
  grace period is required, implement it as a bounded wait on the referenced
  timer's completion condition, not a sleep loop.
- `indigo_reschedule_timer()` should be valid for a pending timer and for the
  timer's own running callback. In the running/self-reschedule case, record a new
  due time and callback to be installed after the callback returns.
- `indigo_cancel_timer()` should atomically prevent a pending callback, clear the
  external reference, unlink device membership, notify the scheduler, and return
  true. If the callback is already running, it should mark cancellation requested
  if useful but return false to preserve best-effort semantics.
- `indigo_cancel_timer_sync()` should prevent a pending callback or wait until a
  running callback completes. It must not wait on itself when called from its own
  callback; in that case it should only clear/cancel future reschedules and
  return.
- `indigo_cancel_all_timers()` should collect matching timer objects under the
  scheduler lock, cancel each through internal handles, and then wait for running
  callbacks outside the scheduler lock. It should skip self-wait for the current
  timer callback thread.
- External references must only be cleared if the reference still points at the
  timer/generation being completed. This protects callers that cancel and create
  a new timer in the same storage.

### Handler Queues

Give each `indigo_queue` its own mutex/condition variable. All of these fields
must be owned by that one mutex:

- task list,
- abort flag,
- ready flag,
- running task identity/callback/device,
- active task count or running flag.

Queue worker loop:

- wait while not aborted and no task is due;
- timed-wait until the earliest due task;
- among due tasks, dequeue the highest priority runnable task;
- release the queue mutex;
- run the callback under `task_mutex` if provided;
- reacquire the queue mutex, clear running state, broadcast waiters, and repeat.

Queue insertion should maintain an ordered-by-time list. Priority is applied only
among runnable tasks, matching the current behavior and tests.

`indigo_queue_remove()` should:

- remove matching pending tasks under the queue mutex;
- if called from another thread and a matching task is running, wait on the
  queue condition until it finishes;
- return immediately when called from the queue worker itself.

`indigo_queue_delete()` should:

- be safe for `NULL` and already-empty queues;
- set abort and remove all pending tasks under the queue mutex;
- wake the worker;
- join only when called from a different thread;
- destroy mutex/condition resources and clear `*queue`.

## Atomic Implementation Steps

Each step should leave the tree buildable, or be paired with the immediately
following step when the header/source contract changes.

1. Add regression tests for currently documented behavior that must survive the
   rewrite: basic set/fire/reference clearing, cancel pending, cancel running,
   reschedule pending, self-reschedule, device-wide cancellation, queue priority,
   queue removal, and queue deletion.
2. Decide the `indigo_cancel_timer_sync()` return value before writing its
   replacement. Since the return is almost unused, prefer `true` when a timer
   existed and was made safe, and `false` when no matching timer existed; update
   the SSAG disconnect call site if this changes its behavior.
3. Add the missing race/deadlock regression tests listed in the test plan below,
   including the chosen `indigo_cancel_timer_sync()` return-value contract.
4. Update `indigo_timer.h` struct layouts for the new implementation without
   preserving binary ABI, and document that exposed fields are private
   implementation details.
5. Introduce internal enums/helpers in `indigo_timer.c` for timer state,
   timespec comparison, timespec normalization, and monotonic-clock condition
   variable initialization.
6. Replace the shared `timers_mutex` use in queue code with a private
   `indigo_queue` mutex while keeping the old queue algorithm unchanged.
7. Initialize all queue task fields explicitly in `indigo_queue_add()` and
   `indigo_queue_add_with_data()`.
8. Rework queue wait predicates so the task list, `abort`, `ready`, and running
   task state are all read and written while holding the queue mutex.
9. Rework `indigo_queue_remove()` to remove pending matches and wait for a
   matching running task only when called from outside the queue worker thread.
10. Rework `indigo_queue_delete()` to set abort, discard pending tasks, wake,
   join from non-worker threads, destroy queue synchronization primitives, and
   clear `*queue`.
11. Run the timer unit test after the queue-only changes and fix any behavior
    drift before touching the timer scheduler.
12. Introduce the new timer object state fields and helper functions while still
    compiling the old timer execution path.
13. Add a scheduler singleton with mutex, condition variable, pending list,
    initialization guard, and earliest-deadline insertion/removal helpers.
14. Implement scheduler thread startup and idle waiting without changing public
    timer functions yet.
15. Rewrite `indigo_set_timer*()` to allocate a fresh timer object, initialize
    metadata, set the external reference atomically, link the device timer list,
    enqueue by deadline, and wake the scheduler.
16. Rewrite due-timer dispatch so callbacks execute outside the scheduler lock
    on one joinable worker thread per due timer, under the optional user/device
    mutex.
17. Implement timer completion in one place: unlink from device list, clear the
    external reference only if it still points to the completing timer, broadcast
    completion waiters, and free or retire the object only after waiters are
    safe.
18. Implement `indigo_reschedule_timer_with_callback()` for pending timers,
    including deadline reordering and scheduler wakeup when the new deadline is
    earlier.
19. Implement self-reschedule from a running callback as a distinct transition
    applied after the callback returns.
20. Implement `indigo_reschedule_timer()` as a thin wrapper that preserves the
    current callback and delegates to the callback-changing variant.
21. Implement `indigo_cancel_timer()` as atomic pending cancellation with
    reference clearing and nonblocking false return for already-running timers.
22. Implement `indigo_cancel_timer_sync()` as cancel-or-wait using the timer
    completion condition variable, with explicit self-callback handling to avoid
    deadlock.
23. Implement `indigo_cancel_all_timers()` by collecting device timers under the
    scheduler lock, canceling them through internal handles, and waiting outside
    the lock while skipping self-wait.
24. Remove the old detached timer thread reuse/free-list code after the new path
    passes the unit tests.
25. Audit all paths for a single lock order: scheduler mutex before no timer
    object locks, queue mutex independent of scheduler mutex, callback/user
    mutexes never held while taking scheduler or queue locks.
26. Run `make -C indigo_test test-unit` and fix failures before integration
    testing.
27. Run a narrow timer-heavy simulator integration test, preferably CCD
    simulator if the local build has the required driver archives.
28. Run a sanitizer/thread-check build if available on the platform; otherwise
    run the timer unit test repeatedly in a loop to shake out timing bugs.
29. Update `DRIVER_DEVELOPMENT_BASICS.md` only if the observable API semantics
    change or if the refactor clarifies previously ambiguous return values.
30. Inspect `git diff` for unrelated churn, generated files, build products,
    and accidental formatting sweeps.
31. Run `make -C indigo_test test-clean` after validation unless build
    artifacts are intentionally being kept.

## Test Plan

Extend `indigo_test/unit/test_timer.c` before or alongside the implementation:

### Timer Basics

- `indigo_delay_to_time(0)` returns `{ 0, 0 }`.
- Small positive fractional delays produce normalized `tv_nsec` values in the
  `[0, 1000000000)` range.
- Larger fractional delays crossing a second boundary normalize correctly.
- Negative delays, if accepted by the API, are treated as immediately due and do
  not produce invalid `timespec` values.
- A timer with `delay == 0` runs promptly, clears the external reference, and
  does not require caller-side sleeps beyond bounded polling for completion.
- A timer with a nonzero delay does not fire before its deadline within a
  reasonable tolerance.
- `indigo_set_timer_with_data()` passes the exact user data pointer to the
  callback.
- `indigo_set_timer_with_mutex()` runs the callback while the supplied mutex is
  held.
- Multiple timers created for the same device without a mutex run in parallel
  when their deadlines overlap.
- Multiple timers created with the same mutex are serialized even when their
  deadlines overlap.

### Timer References And Lifetime

- A completed one-shot timer clears `*timer` exactly once.
- A canceled pending timer clears `*timer` immediately and never calls its
  callback.
- A callback that completes naturally does not clear a newer timer stored in the
  same external reference slot.
- Repeated immediate timers can reuse the same external reference slot hundreds
  of times without hitting a polling race or stale-handle error.
- `indigo_set_timer()` with a non-NULL reference and `*timer != NULL` follows
  the chosen contract deterministically: it either fails cleanly or waits only
  through a bounded, condition-variable based handoff.
- Calling `indigo_cancel_timer()` twice on the same reference is harmless: the
  first call may cancel, the second returns false and leaves the reference NULL.
- Calling `indigo_cancel_timer_sync()` twice on the same reference is harmless:
  the first call makes the timer safe, the second returns false and leaves the
  reference NULL.
- A stale local `indigo_timer *` handle cannot cancel or reschedule a newer timer
  that reused the same external reference storage.

### Timer Reschedule

- `indigo_reschedule_timer()` fails cleanly when the external reference is NULL.
- Rescheduling a pending timer to an earlier deadline wakes the scheduler and
  fires near the new deadline, not the original later deadline.
- Rescheduling a pending timer to a later deadline prevents firing near the old
  deadline.
- `indigo_reschedule_timer_with_callback()` changes both deadline and callback;
  only the replacement callback runs.
- A callback can reschedule itself for a fixed number of iterations and the
  reference remains non-NULL until the last iteration completes.
- A self-rescheduling callback can switch to a different callback with
  `indigo_reschedule_timer_with_callback()`.
- Canceling a timer while another thread repeatedly tries to reschedule it never
  runs both an obsolete callback and a replacement callback for the same logical
  generation.
- Reschedule and natural completion racing on a short delay either produce one
  well-defined callback invocation or a clean reschedule; they must not leave a
  stuck non-NULL reference.

### Timer Cancellation

- `indigo_cancel_timer()` returns true for a pending timer that it prevented,
  clears the reference, and leaves the callback count unchanged.
- `indigo_cancel_timer()` returns false for an already running callback and does
  not wait for it.
- `indigo_cancel_timer_sync()` returns true when a timer existed and is safe
  after the call, including the pending-not-running case.
- `indigo_cancel_timer_sync()` returns false when the reference is already NULL
  or does not identify a live timer.
- `indigo_cancel_timer_sync()` waits for an already running callback to finish
  and clears the external reference before returning.
- `indigo_cancel_timer_sync()` called by the timer's own callback does not wait
  on itself. It may prevent a requested self-reschedule, but it must return.
- Canceling a timer whose callback is blocked on its user mutex does not deadlock
  with the scheduler lock.
- Canceling many pending timers out of deadline order leaves no callbacks
  running and no non-NULL external references.

### Device Timer Lists

- Timers scheduled with a non-NULL `device` are linked into
  `DEVICE_CONTEXT->timers` while pending/running and unlinked on completion or
  cancellation.
- Timers scheduled with `device == NULL` are not linked into any device timer
  list.
- `indigo_cancel_all_timers(device)` cancels every pending timer for that device
  and leaves timers for other devices untouched.
- `indigo_cancel_all_timers(device)` called while one of the device timers is
  running waits for that callback, cancels other pending timers, clears all
  references, and leaves `DEVICE_CONTEXT->timers == NULL`.
- `indigo_cancel_all_timers(device)` called from one of the device's own timer
  callbacks does not self-deadlock; use a subprocess/timeout wrapper for this
  regression so a broken implementation cannot hang the entire unit suite.
- `indigo_cancel_all_timers(device)` works when callbacks concurrently complete
  and remove themselves from the device list.
- `indigo_cancel_all_timers(device)` is harmless when the device has no timers.
- Timers bound to slave and master devices keep their own device-list ownership;
  serialization belongs to `indigo_set_device_timer()` and queues, not to raw
  timer list membership.

### Handler Queue Basics

- A newly created queue starts its worker and reports ready without a lost
  wakeup.
- ASAP tasks run promptly.
- Delayed tasks do not run before their due time within a reasonable tolerance.
- Tasks with identical due time execute with higher runnable priority first.
- Priority is applied among runnable tasks only; a future high-priority task
  does not block an already due lower-priority task.
- `indigo_queue_add()` initializes `data` as NULL and calls the plain callback
  form.
- `indigo_queue_add_with_data()` passes the exact data pointer to the callback.
- A task with a `task_mutex` runs while that mutex is held.
- Queue callbacks run serially for one queue, even when many tasks are due at
  once.

### Handler Queue Scheduling Edges

- Inserting a new earlier task while the worker is timed-waiting for a later task
  wakes the worker and runs the earlier task first.
- Inserting many delayed tasks in random order produces nondecreasing due-time
  execution, except where priority among already due tasks deliberately reorders
  them.
- Repeated ASAP insertion from multiple producer threads does not lose tasks.
- A callback can enqueue a follow-up task on the same queue without deadlocking.
- A callback can enqueue a delayed follow-up task on the same queue and the
  worker sleeps and wakes for it correctly.
- A callback can call `indigo_queue_remove()` for its own callback/device without
  waiting on itself.

### Handler Queue Removal And Deletion

- `indigo_queue_remove(queue, device, callback)` removes only pending tasks that
  match both filters.
- `indigo_queue_remove(queue, device, NULL)` removes all pending tasks for that
  device and keeps tasks for other devices.
- `indigo_queue_remove(queue, NULL, callback)` removes pending tasks matching the
  callback across all devices.
- `indigo_queue_remove(queue, NULL, NULL)` removes all pending tasks.
- `indigo_queue_remove()` called from another thread while a matching task is
  running waits for that task to finish.
- `indigo_queue_remove()` called from another thread while a nonmatching task is
  running does not wait for it unnecessarily.
- `indigo_queue_remove(NULL, ...)` is harmless.
- `indigo_queue_delete()` with no tasks exits promptly and clears `*queue`.
- `indigo_queue_delete()` with many delayed tasks exits promptly without running
  those tasks.
- `indigo_queue_delete()` while a task is running waits for the running task when
  called from another thread, then clears `*queue`.
- `indigo_queue_delete()` must not join the worker from the worker thread itself;
  if this scenario is supported, cover it with a subprocess/timeout wrapper.

### Randomized And Stress Tests

- Add a deterministic pseudo-random timer churn test with a fixed seed. Use
  several producer threads, each with multiple timer reference slots, and run
  thousands of operations chosen from set, cancel, sync cancel, reschedule, and
  reschedule-with-callback using short random delays.
- The timer churn test should verify no crashes, no deadlocks, no callback data
  corruption, all references NULL after cleanup, and at least some callbacks from
  both original and replacement callbacks.
- Add a variant where producer threads intentionally compete on a shared set of
  timer reference slots. The expected callback count can be loose, but the test
  must verify no stale handle can affect a newer generation and no reference is
  left non-NULL after cleanup.
- Add a timer storm test that schedules hundreds or thousands of timers with
  mixed zero and short delays, then waits for all callbacks and verifies all
  references clear.
- Add a cancellation storm test that schedules hundreds or thousands of delayed
  timers, cancels them from multiple threads, then verifies no canceled callback
  ran and all references are NULL.
- Add a mixed device-list storm test with timers spread over several fake
  devices, repeatedly calling `indigo_cancel_all_timers()` on random devices
  while unrelated device timers continue to run or complete.
- Add a deterministic pseudo-random queue churn test with multiple producer
  threads inserting ASAP and delayed tasks at mixed priorities while another
  thread removes random device/callback subsets.
- The queue churn test should verify no lost ready wakeups, no callbacks after
  deletion, no task data corruption, and serialized execution within a queue.
- Run the timer and queue stress tests repeatedly in a loop in the validation
  phase. Keep them deterministic by seed, and print the seed on failure so a
  flaky interleaving can be reproduced.

### Test Harness Requirements

- Use bounded polling helpers rather than long fixed sleeps.
- Any test that intentionally covers a known deadlock class must run the risky
  operation in an isolated child process or equivalent timeout wrapper so the
  whole unit suite cannot hang forever.
- Keep all test data alive until every timer/queue callback that may reference
  it has completed or been synchronously canceled.
- Protect shared counters and callback observations with a test mutex.
- Avoid relying on private struct fields in new tests where possible. If a test
  needs to pause a worker to create a deterministic ordering window, add a test
  callback barrier instead of locking queue internals directly.
- Keep random stress iteration counts large enough to expose races in normal
  development runs, but bounded enough for `make -C indigo_test test-unit` to
  remain practical.
- Add heavier looped stress targets separately if the normal unit target becomes
  too slow.

Validation sequence:

1. Build `indigo_libs`.
2. Run `make -C indigo_test test-unit`.
3. Run a narrow integration target that uses timers heavily, at minimum the CCD
   simulator integration test if available in the local build.
4. Run `make -C indigo_test test-clean` after validation.
