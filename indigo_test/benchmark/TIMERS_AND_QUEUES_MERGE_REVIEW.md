# Timers and Handler Queues: Merge Review

This note compares the reimplemented timer and queue subsystem on the
`refactoring` branch against the current `master`, covering correctness,
measured latency and jitter, and what still needs attention before the branch
lands. The numbers come from `bench_timer.c` and `bench_queue.c` beside this
file; `indigo_docs/TIMERS_AND_QUEUES.md` describes the implementation model
itself.

| | |
| --- | --- |
| Branches | `master` cd408e64b, `refactoring` 2de56f5ef |
| Files | `indigo_libs/indigo_timer.c`, `indigo_libs/indigo/indigo_timer.h` |
| Change | +836 / -347 lines |
| Platform | macOS 24.6, arm64, clang, `libindigo` at `-O3` |
| Measured | 4-5 September 2026 |

## Recommendation

Merge the refactor. It closes several real defect classes and fixes timing
correctness on macOS. The cost is a few microseconds per dispatch and one
measured regression worth fixing first.

Steady-state accuracy is a tie: a 10 ms periodic timer lands within noise of
`master` on both branches. What separates them is behavior under conditions the
benchmarks can provoke and the old code cannot survive, namely wall-clock steps,
forked children, stale handles, and concurrent load across the two subsystems.
The one place `master` is clearly better is a burst of timers due at the same
instant, where the refactor is two to three times slower and much less
predictable.

## Method

Two benchmark programs live under `indigo_test/benchmark/`. Both call only timer
and queue API whose signatures are unchanged between the branches, so one source
builds against either library. `master` was built in a separate `git worktree`,
both benchmark harnesses were compiled with identical flags, and passes were
alternated between branches to cancel out drift in machine load.

All timings come from a monotonic clock read inside the benchmark, so they never
depend on the clock the library uses internally. Each figure below is the mean of
two independent passes of five runs; the two passes agreed closely except where
noted. Warm-up samples are discarded.

```
make -C indigo_test benchmark
```

Alongside the benchmarks the branch carries a rewritten unit suite:
`indigo_test/unit/test_timer.c` grows from 331 to 2635 lines and 86 tests, which
passed five consecutive runs at about 12.1 s each with no flakiness.

## What master gets wrong

These are the reasons to move. The first was reproduced with a standalone
harness; the rest are static findings from reading both implementations.

- **Hang.** The queue worker tests `queue->abort` outside `cond_mutex` and then
  waits, so `indigo_queue_delete()` can set the flag, empty the queue and signal
  into a condition variable with no waiter. The worker then sleeps forever and
  `pthread_join()` never returns.
- **Use after free.** `peek_task()` returns a task still linked in the list, then
  drops `timers_mutex`. The caller dereferences `task->at` as the wait deadline
  while a concurrent `indigo_queue_remove()` can free it.
- **Wrong timer.** Timer structs are recycled through a free list and a handle is
  validated only by `*timer == *(*timer)->reference`. A stale handle to a
  recycled struct passes that test and can cancel or reschedule a different
  logical timer.
- **Lifetime.** `indigo_cancel_timer_sync()` waits by locking `thread_mutex` on a
  struct that can be recycled into a new timer while it waits.
- **Fork.** No `pthread_atfork()` handling at all. A forked child inherits timer
  structs referencing threads that do not exist.
- **Startup.** `pthread_create()` is never checked. Combined with the `ready`
  flag added in d99b4c4b0, a failed queue thread now leaves
  `indigo_queue_create()` waiting on a condition variable forever.
- **Self delete.** `indigo_queue_delete()` called from the queue's own worker
  calls `pthread_join()` on itself, ignores `EDEADLK`, then frees the queue while
  the worker is still running on it.
- **Contention.** Queue operations take the global `timers_mutex` shared with
  every timer in the process. This one is measured below.

All of these are live. Drivers never name the queue API directly, which is easy
to misread as the queue being unused: they reach it through the wrappers in
`indigo_driver.c`. `indigo_execute_handler()` and its priority, data and delay
variants lazily call `indigo_queue_create()` and then `indigo_queue_add()`;
`indigo_cancel_pending_handler()` and `indigo_cancel_pending_handlers()` call
`indigo_queue_remove()`; and `indigo_device_detach()` calls
`indigo_queue_delete()` at `indigo_driver.c:1063`. There are 508 wrapper call
sites across 94 driver files, plus 223 uses of the
`INDIGO_COPY_VALUES_PROCESS_CHANGE` family of macros in `indigo_bus.h`, which
call `indigo_execute_handler()` in turn.

The queue is therefore on the hot path of ordinary property-change handling for
nearly every driver, and the hang above is reached through the one call site
that matters most: device detach. A queue that is idle at the moment a device
detaches is exactly the state that opens the race.

## Timers

Lateness is the gap between when a callback actually ran and the deadline the
caller asked for. All figures are microseconds, lower is better. `n` is 1500 per
row, 1000 for the burst rows.

| Scenario / branch | mean | median | p95 | p99 | worst |
| --- | ---: | ---: | ---: | ---: | ---: |
| **One-shot 5 ms timer, lateness** | | | | | |
| master | **1113** | **1301** | **1338** | **1363** | **1614** |
| refactoring | 1207 | 1390 | 1457 | 1503 | 1689 |
| **Zero-delay timer, dispatch cost** | | | | | |
| master | **10.0** | **8.5** | **20.5** | **29.0** | **123** |
| refactoring | 46.2 | 34.5 | 102 | 135 | 321 |
| **Self-rescheduling 10 ms timer, lateness** | | | | | |
| master | **2029** | 2511 | 2586 | 2644 | 7356 |
| refactoring | 2071 | **2470** | **2699** | **2727** | **3023** |
| **200 timers due at the same instant, lateness** | | | | | |
| master | **3036** | **3096** | **5558** | **5636** | **5801** |
| refactoring | 7531 | 7249 | 13728 | 16222 | 20099 |

Burst is the only row where the two passes disagreed materially on the refactor,
with means of 5630 and 9432 us. `master` stayed at 2868 and 3205.

**Periodic accuracy is a tie.** Means differ by 42 us against a standard
deviation of about 750, which is noise. The refactor holds a tighter tail, worst
case 3.0 ms against master's 7.4 ms, on the strength of one outlier.

**Dispatch costs 36 us more,** reproducibly, because every firing spawns a worker
thread where `master` keeps one alive per timer and reuses it. In absolute terms
this is small next to the platform floor described below, but it is 4.5x and it
would be a much larger share on Linux.

**Simultaneous fan-out is the real regression.** One scheduler thread creates 200
workers in sequence and interleaves joining finished ones on the same loop, so
lateness accumulates and varies with system state. Master's independent per-timer
threads wake in parallel.

## Handler queues

The queue picture inverts. In isolation the refactor is a little slower, but
under concurrent timer activity, which is the condition that actually holds in a
running server, `master` collapses. Microseconds, `n` is 1000 per latency row and
50 per drain row.

| Scenario / branch | mean | median | p95 | p99 | worst |
| --- | ---: | ---: | ---: | ---: | ---: |
| **`indigo_queue_add()` against a backlog, call cost** | | | | | |
| master | 1.9 | 1.0 | 4.0 | 13.0 | 27 |
| refactoring | **1.7** | 1.0 | 4.5 | **11.5** | **20** |
| **Zero-delay task on an idle queue, dispatch** | | | | | |
| master | **3.4** | **3.0** | **5.0** | **7.5** | **14** |
| refactoring | 4.8 | 4.5 | 8.0 | 14.5 | 151 |
| **Delayed 5 ms task, lateness** | | | | | |
| master | 1112 | 1275 | 1304 | **1333** | **2003** |
| refactoring | **1104** | 1275 | **1303** | 1344 | 5700 |
| **Task re-adding itself every 10 ms, lateness** | | | | | |
| master | **2038** | 2520 | **2558** | 2593 | 3986 |
| refactoring | 2067 | 2520 | 2561 | **2587** | **2988** |
| **Draining a batch of 200 ready tasks, per task** | | | | | |
| master | **1.3** | **1.2** | **2.3** | **2.7** | **3.6** |
| refactoring | 1.7 | 1.6 | 3.6 | 3.8 | 3.9 |
| **The same drain while timers churn, per task** | | | | | |
| master | 8.6 | 8.1 | 15.8 | 31.7 | 40.2 |
| refactoring | **1.9** | **2.0** | **2.7** | **3.0** | **3.0** |

The last two rows are the same workload, differing only in whether another thread
is creating and cancelling timers at the same time. `master` goes from 1.3 to
8.6 us per task, a 4x to 5x slowdown, while the refactor moves from 1.7 to 1.9.
The cause is visible in the code: on `master`, `enqueue_task()`,
`dequeue_runnable_task()`, `peek_task()` and `remove_tasks()` all take the global
`timers_mutex`, the same lock every `indigo_set_timer()` and every timer
completion takes process-wide. The refactor gives each queue its own mutex,
disjoint from the scheduler's.

Everything else is a wash or a fraction of a microsecond. One number worth noting
for driver authors: queue dispatch on the refactor is about seven times cheaper
than timer dispatch, 4.8 us against 46 us, because the queue worker persists
while each timer firing spawns a thread. Where per-fire cost matters, moving work
from a timer to a handler queue is a bigger lever than the branch choice.

## The platform floor

A 5 ms timer runs about 1.3 ms late, a 10 ms timer about 2.5 ms late, and a
zero-delay timer tens of microseconds. That is a constant 25% of the requested
interval, it appears identically on both branches and in both subsystems, and it
is an order of magnitude larger than anything separating the two
implementations.

This is macOS applying proportional leeway to condition-variable waits. Using
`pthread_cond_timedwait_relative_np()`, as the refactor does, does not avoid it.
If macOS timing accuracy matters, the lever is the thread QoS class, and it would
help both branches equally. It is worth treating as its own piece of work rather
than as an argument about this merge.

## Before merging

Five items to resolve or consciously accept, and one raised earlier and now
withdrawn.

### High: burst dispatch is 2x to 3x slower and erratic

200 timers sharing a deadline give means of 5630 and 9432 us across two passes
against master's 2868 and 3205, with p99 reaching 19 ms. The scheduler thread
serialises `pthread_create()` per firing and reaps finished workers on the same
loop, so spawning queues behind joining.

Suggested: replace thread-per-firing with a small worker pool, or at minimum move
reaping off the dispatch path so creation is not serialised behind it. Whether it
matters in production depends on how often many timers share a deadline, so it is
worth checking against a real multi-device session before deciding how far to go.

### Medium: no Linux measurements

Every number here is macOS on Apple Silicon. On Linux both branches already use
`CLOCK_MONOTONIC`, so the correctness argument for the refactor's clock handling
disappears there, and the absent 25% coalescing floor would make the 36 us
dispatch overhead a far larger relative share.

Suggested: re-run both benchmarks on the Linux target before treating the
performance conclusions as settled.

### Medium: three API behavior changes to audit against drivers

`indigo_set_timer()` with a non-NULL reference now fails immediately, where
`master` spun up to 100 ms and usually succeeded, then silently subtracted the
spun time from the requested delay. `indigo_cancel_timer_sync()` now returns
`true` when it cancels a pending timer, where `master` returned `false` unless it
actually had to wait. `indigo_queue_create()` can now return `NULL`.

Suggested: grep the drivers for callers that test the two timer return values.
The queue one needs a fix rather than a grep: the only caller is
`indigo_execute_handler_in()` and its variants in `indigo_driver.c`, which assign
the result straight into `DEVICE_CONTEXT->queue` without checking it. On the
refactor a failed creation then makes `indigo_queue_add()` return quietly, so the
handler is dropped with no diagnostic and the device silently stops responding to
property changes. Check the result at those four call sites and log it.

### Low: `indigo_timer` is a larger public struct

The struct grows to twenty fields and stays in the public header. The comment
tells callers not to touch it, which is right, but anything compiled against the
old header has to be rebuilt.

Suggested: confirm the version bump communicates this, or move the body behind an
opaque handle if the ABI is meant to hold.

### Low: a Windows clock macro claims something untrue

`INDIGO_TIMER_CLOCK_IS_MONOTONIC` can evaluate to 1 on Windows while the local
`clock_gettime()` shim ignores `clk_id` and returns wall time. Behavior is still
correct, because the condition variable is wall-clock too, but the macro asserts
a guarantee the platform is not giving.

Suggested: gate the macro on the platform actually providing a monotonic source,
so a future reader does not rely on it.

### Withdrawn: queue dispatch cost climbing within a process

An earlier pass showed the refactor's per-run queue dispatch mean rising across a
single process, from 2.9 to 8.4 us over five runs, while `master` stayed flat,
and this was flagged as possible state accumulation. It did not survive scrutiny:
that comparison had the two harnesses built with different compiler flags.
Rebuilt identically, the trend disappears and the residual variation is noise.
The same correction narrowed the queue dispatch gap from roughly 2x to 1.4x.

## Reproducing

Both benchmarks live in the tree and run outside the test target, since they
measure rather than assert:

```
make all                          # build libindigo first
make -C indigo_test benchmark     # bench_timer, then bench_queue
```

Each takes an optional label and sample counts, for example
`bench_queue refactoring 5 200 200 200`. To compare against another branch, check
it out in a worktree, build `libindigo` there, and compile the same benchmark
source against it with matching flags. The sources deliberately use only
long-stable API so this works without edits, and matching the flags matters, as
the withdrawn finding above shows.
