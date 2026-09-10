# indigo_libs Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Focused baseline and timer/queue reimplementation reviews recorded findings; full subtree review not complete. |

## Scope

Core INDIGO library code, including the bus, timers, protocol adapters, base drivers, utility helpers, portable I/O, image/format helpers, and shared public headers.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| LIB-001 | High | `indigo_timer.c:556` | Claimed `indigo_queue_add()` leaves `indigo_queue_task.data` uninitialized so queue dispatch could read a garbage pointer. False positive: the task is allocated with `indigo_safe_malloc()`, which zeroes the allocation, so `data` is always `NULL`. See finding summary below. | Closed (false positive) |
| LIB-002 | Medium | `indigo/indigo_names.h:2686` | `AGENT_MOUNT_ENABLE_DEROTATION_ITEM_NAME` published the item name as `ENABLE_DEROTACTION` instead of `ENABLE_DEROTATION`. This misspelled the new public protocol/config item and would either force clients to use the typo or break saved settings and integrations when corrected later. | Closed (fixed) |
| LIB-003 | High | `indigo_timer.c:243` | The `pthread_atfork()` child handler called mutex/condition-variable initialization and condition-variable attribute APIs. After `fork()` from a multithreaded process, those are not async-signal-safe operations. The fork protocol now locks the existing scheduler mutex in the prepare handler and only resets ordinary state in the child while that mutex is held. | Closed (fixed) |
| LIB-004 | Medium | `indigo_timer.c:253` | The child reset dropped the live registry without clearing inherited public timer slots or device timer lists. The child fork handler now discards inherited timers by clearing both forms of ownership before resetting the registry. | Closed (fixed) |
| LIB-005 | Medium | `indigo_timer.c:578` | `indigo_reschedule_timer_with_callback()` changed only the callback pointer. It now explicitly discards a prior data payload and changes the timer to the plain callback form before dispatch. | Closed (fixed) |
| LIB-006 | Medium | `indigo_timer.c:753` | Queue dispatch initialized priority selection to `-1`. Runnable negative-priority tasks were never dequeued, leaving the worker in a busy loop. Selection now initializes from the first due task, so the complete signed `int` priority domain is supported. | Closed (fixed) |
| LIB-007 | Medium | `indigo_bus.c:indigo_property_copy_values`, `indigo_property_copy_targets` | Range comparisons did not reject NaN. | Closed (fixed) |
| LIB-008 | Medium | `indigo_ccd_driver.c:92` | A countdown callback arriving after `countdown_endtime` skips the value update and deadline reset, but keeps rescheduling while exposure remains BUSY. The displayed remainder can stay positive during readout. LOW priority can aggravate delays; increasing priority alone does not fix the expired-deadline path. | Closed (fixed) |
| LIB-009 | Low | `indigo_ccd_driver.c:78` | Countdown deadlines and remaining time use `gettimeofday()`. A wall-clock adjustment can increase the displayed remainder or move the countdown past its deadline independently of elapsed exposure time. | Closed (fixed) |

## Finding Summaries

### LIB-001 (Closed — false positive)

The finding claimed `indigo_queue_add()` leaves `indigo_queue_task.data` uninitialized, and
that since queue dispatch in `queue_func()` (`indigo_timer.c:516`) selects the callback
signature based on `data == NULL`, a non-zero garbage value could invoke the callback with
the wrong signature and a bogus data pointer.

This is not a defect. Tasks are allocated with `indigo_safe_malloc()` (`indigo_bus.h:908`),
which `memset`s the entire allocation to zero before returning it. `task->data` is therefore
guaranteed to be `NULL` after `indigo_queue_add()` runs, so dispatch always takes the
one-argument callback path as intended. `indigo_queue_add_with_data()` simply overwrites this
zeroed field with the caller's pointer. There is no code path that reads an uninitialized or
garbage `data`.

Resolution: no code change. The finding was based on the assumption of an uninitialized
allocation, which does not hold given `indigo_safe_malloc()`'s zeroing contract. Closed as a
false positive.

### LIB-002 (Closed — fixed)

`AGENT_MOUNT_ENABLE_DEROTATION_ITEM_NAME` now publishes `ENABLE_DEROTATION` instead of
the misspelled `ENABLE_DEROTACTION`, so the public item name matches the macro and
intended feature name.

### LIB-003 (Closed — fixed)

`reset_timer_scheduler_after_fork_child()` is registered as the child callback
of `pthread_atfork()` and invokes `pthread_mutex_init()`, `pthread_cond_init()`,
and, on Linux, `pthread_condattr_init()`, `pthread_condattr_setclock()`, and
`pthread_condattr_destroy()`. A child created from a multithreaded parent may
only safely call async-signal-safe functions before `exec()`. These pthread
initialization APIs do not meet that requirement. The fork recovery path can
therefore hang or otherwise misbehave precisely when it is intended to recover
from inherited thread state.

Resolution: the at-fork prepare handler locks the existing scheduler mutex, the
parent handler unlocks it, and the child handler resets only ordinary scheduler
state before unlocking it. No pthread mutex, condition variable, or condition
attribute is initialized or destroyed after `fork()`. The timer unit suite now
forks while a callback is running and verifies that the child can start and
complete a timer with its replacement scheduler.

### LIB-004 (Closed — fixed)

The child handler resets `timer_scheduler.timers` and `pending` to `NULL`, but
does not clear each inherited timer's public `reference` slot or unlink its
`device->device_context->timers` node. A child which inherits a pending timer
then has a non-NULL slot that fails the live-registry validation, preventing a
new timer from being scheduled into the usual slot. Calling cancel-all happens
to unlink the stale node, but it is accidental cleanup rather than a defined
fork lifecycle.

Resolution: inherited timers are explicitly discarded in the child fork handler.
It clears each timer's public reference slot only when that slot still points to
the timer, unlinks device timer-list ownership, then drops the scheduler
registry. The unit suite verifies that a child sees the inherited slot and
device list cleared, and can immediately reuse the exact same slot for a new
timer.

### LIB-005 (Closed — fixed)

`indigo_reschedule_timer_with_callback()` accepts an `indigo_timer_callback`,
but `reschedule_timer_with_callback_locked()` does not reset `has_data` when it
replaces the callback. The callback worker consequently casts the replacement
one-argument callback to `indigo_timer_with_data_callback` whenever the prior
schedule used `indigo_set_timer_with_data()`. Extra arguments happen to be
tolerated on common current ABIs, but the call is undefined in C and is not a
valid cross-platform API implementation.

Resolution: the internal reschedule helper now distinguishes preserving a
timer's existing callback form from replacing it through the public plain
callback API. `indigo_reschedule_timer_with_callback()` clears `has_data` and
`timer_data` for both pending and self-rescheduled running timers. The unit
suite covers both paths and confirms the next callback uses the plain form.

### LIB-006 (Closed — fixed)

`dequeue_runnable_task_locked()` starts priority selection at `-1`. It returns
no task when every due task has a priority below zero. The outer worker loop
then immediately observes the same overdue task again, so it spins instead of
waiting, while the callback never runs. `indigo_queue_add()` and
`indigo_queue_add_with_data()` take arbitrary `int priority` values and expose
no range restriction.

Resolution: both queue selection helpers now initialize their choice from the
first due task rather than an out-of-domain sentinel. The resulting selection
accepts every signed `int` value and retains the pending-list order for equal
priorities. A regression test submits three negative-priority tasks while a
barrier task holds the worker and verifies execution in descending priority
order.

### LIB-007 (Closed — fixed)

Range comparisons did not reject NaN. Both copy helpers now handle non-finite requests centrally: they retain the affected item's accepted value/target and continue copying other valid items. Finite values retain existing min/max clamping. This replaces redundant driver input validation.

Validation: NaN/infinity preservation, valid sibling copying and finite clamping unit tests pass.

### LIB-008 (Closed — countdown termination)

The update guard requires both `countdown_endtime >= now` and a displayed value of at least 1, while the rescheduling guard requires neither. Consequently an expired deadline, or a driver setting value to 0 while retaining BUSY for image readout, leaves a recurring task with no useful update. DSI's `ccd_exposure_finalizer` explicitly sets value to 0 before readout and can retain BUSY while polling (`ccd_dsi/indigo_ccd_dsi.driver:219`). Countdown termination must handle elapsed deadlines and a driver-supplied zero without changing exposure state to OK; only the driver knows when acquisition/readout has completed.

Priority analysis: LOW is -5, normal immediate handlers use 0, and `indigo_execute_handler_in()` uses TIME (10). Selection considers only due tasks, so a future exposure finalizer does not prevent countdown updates. Selection has no priority aging, and callback execution is synchronous on a single queue worker with the device/master mutex. Sustained due higher-priority work can starve countdown; a running callback or held task mutex can delay it regardless of priority. Changing countdown priority alone cannot repair its termination conditions.

Proposed correction: reject inactive/canceled/non-BUSY countdowns, terminate on a driver-supplied zero, compute remaining time without excluding elapsed deadlines, clear the deadline before publishing zero, and reschedule only an active positive countdown. Preserve the current <=0.25 s zero-display tolerance in the minimal fix. Keep LOW initially; consider priority tuning only with measured queue contention. Keep countdown on the existing serialized queue rather than introducing another property writer. Preserve suspend/resume behavior used by FLI RBI flushing and SBIG exposure coordination.

### LIB-009 (Closed — wall-clock dependence)

Use a portable elapsed-time clock consistently for countdown start, resume and callback calculations. `indigo_delay_to_time(0)` exposes the scheduler's clock domain, but the scheduler currently falls back to realtime where CLOCK_MONOTONIC is unavailable, including its Windows implementation. It is not an unconditional portable monotonic-clock fix. Do not globally replace `get_time_hd()` without auditing its image timing/FPS callers. Clock portability can be a separate shared-layer change from the minimal countdown termination correction.

Resolution (2026-09-09): countdown now runs on the shared background queue defined in `indigo_bus.c` and owned by bus start/stop. The final implementation deliberately preserves the original direct property-update model, with no new mutexes, device locks or device registry. It terminates on elapsed deadlines or a driver-supplied zero and cancels old countdown work on replacement, suspension and detach. It preserves BUSY and target. `indigo_monotonic_time()` uses POSIX CLOCK_MONOTONIC or Windows QueryPerformanceCounter; image timing is unchanged. Dedicated countdown and CCD simulator validation is recorded in `indigo_test/CHANGES.md`. No Windows/hardware execution or comprehensive race-freedom claim; shared callbacks must remain short. No subtree baseline advancement.

## Review Focus

Focused countdown inspection at `48442826a` (2026-09-09): `LIB-008` follows directly from the expired-deadline guard at line 92 and rescheduling guard at line 105. For example, with value 1, deadline 100 and callback time 100.1, the value stays 1 and another callback is scheduled. This remains true until exposure leaves BUSY or another path clears/disables the countdown. Queue dispatch correctly accepts negative priorities after `279fbd064`; sustained higher-priority ready work can still starve LOW tasks, and running handlers cannot be preempted. No hardware reproduction or full-subtree review; the baseline is unchanged.

- Memory ownership, allocation, copying, and release paths.
- Timer, queue, async, and callback lifetime behavior.
- Thread safety and lock ordering.
- Property definition, update, deletion, matching, and state transitions.
- Protocol serialization/parsing compatibility.
- Cross-platform behavior across Linux, macOS, and Windows.
- Unit coverage under `indigo_test/unit/`.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Initial review baseline only. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Focused baseline review of timer queue dispatch and selected property helper ownership paths; recorded `LIB-001`. |
| `d9b39b84e3780dca0c9e7cbb901b63a62586b106` | `afdd54618e5520c4983598c33b662c022962df7c` | 2026-08-18 | Requested review of the last two commits touching shared INDIGO names; recorded `LIB-002`. |
| `3bf23ba0f062b98ce881c0026da973125d517b8b` | `3fe6337a09e4847d2688389c57c9d533b1cfb387` | 2026-09-04 | Focused deep review of timer/queue reimplementation and its three follow-up review commits. Recorded `LIB-003` through `LIB-006`; this does not advance the subtree-wide review marker. |
| `84298256404b3aee1028d29ce152233ffd8afe2e` | working tree | 2026-09-09 | Focused review of non-finite numeric input in `indigo_property_copy_values()` and `indigo_property_copy_targets()`; recorded and closed `LIB-007`. Preservation of accepted values/targets, valid sibling copying and finite clamping are covered by passing unit tests; the full unit suite passed. No full-subtree review or baseline advancement. |
| `017ba602857378e4aed489c065c76eacae15924c` | `48442826a` | 2026-09-09 | Focused countdown and scheduling analysis; recorded LIB-008 and LIB-009. Existing timer unit suite passed during the initial investigation, including negative-priority and future-priority ordering cases; no dedicated countdown runtime reproduction or hardware validation. No full-subtree review or baseline advancement. |

## Scoped empty-START follow-up — 2026-09-10

At the user's request, the shared platesolver empty-selection defect and its fix are tracked under the same [DRV-145](../indigo_drivers/REVIEW.md) as Mount/Imager/Guider. `indigo_platesolver.c` now returns OK without scheduling an exposure for an all-false START request and no longer copies values before its BUSY guard. The minimal shared-platesolver bus fixture in `indigo_test/integration/test_agent_imager.c` fails against the original object and passes after the fix. See the driver finding and `indigo_test/CHANGES.md` for scope and validation. No full library review or baseline advance is claimed.
