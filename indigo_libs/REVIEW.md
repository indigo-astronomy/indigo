# indigo_libs Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Focused baseline review recorded findings; full subtree review not complete. All recorded findings resolved (LIB-001 closed as a false positive). |

## Scope

Core INDIGO library code, including the bus, timers, protocol adapters, base drivers, utility helpers, portable I/O, image/format helpers, and shared public headers.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| LIB-001 | High | `indigo_timer.c:556` | Claimed `indigo_queue_add()` leaves `indigo_queue_task.data` uninitialized so queue dispatch could read a garbage pointer. False positive: the task is allocated with `indigo_safe_malloc()`, which zeroes the allocation, so `data` is always `NULL`. See finding summary below. | Closed (false positive) |

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

## Review Focus

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
