# indigo_libs Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Focused baseline review recorded findings; full subtree review not complete. |

## Scope

Core INDIGO library code, including the bus, timers, protocol adapters, base drivers, utility helpers, portable I/O, image/format helpers, and shared public headers.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| LIB-001 | High | `indigo_timer.c:556` | `indigo_queue_add()` does not initialize `indigo_queue_task.data`, but queue dispatch uses `data == NULL` to choose whether to call a one-argument callback or cast it to a two-argument callback. A non-zero garbage value can call the callback with the wrong signature and a garbage data pointer. | Open |

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
