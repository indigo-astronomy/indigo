# Quality review

Verdict: pass
Note: after the fixes below; unfixed items are tracked as FND-20261024202957-6bd9-fix-ccd-rpi-001
Reviewed revision: base 923bca9a214d4b5d9ba2982e2964e982f26c007a plus uncommitted diff of indigo_optional_drivers/ccd_rpi/ (Makefile, indigo_ccd_rpi.cpp)
Reviewer: fresh-context general-purpose subagent (read-only), same host and model as the implementer; resolutions by the implementer
Date: 2026-10-24

## Findings and resolutions
| # | Severity | Finding | Resolution |
| --- | --- | --- | --- |
| 1 | high | `handle_focus()` took the master device lock; `ccd_close()` detaches the focuser under that lock and `indigo_cancel_all_timers()` waits for running focuser timers: deadlock on disconnect during a focuser move (regression of the first candidate) | fixed: lock removed; the detach already waits for a running move before the camera is deleted, `MoveLens()` is serialized by `m_operationMutex` |
| 2 | medium | a callback of an aborted exposure could publish its frame for a newly started exposure and read `m_config` while `ConfigureStill()` replaces it (pre-existing) | fixed: exposure sequence counter checked before publishing; frame format and geometry are captured in `ReadFrame()` under `m_cameraMutex` |
| 2a | medium | found while fixing 2: `indigo_set_timer()` fails while the previous callback reference is still set, leaving a fast follow-up exposure BUSY (pre-existing) | fixed: `indigo_cancel_timer_sync()` before scheduling, scheduling failure publishes ALERT |
| 3 | low-medium | autofocus cannot complete with one request per exposure, RPI_AF reports OK at once, AF result is not read back | not fixed: needs a streaming AF design and an AF camera to test; reported to the owner |
| 4 | low | abort or timeout drops a pending AF trigger | not fixed, part of 3 |
| 5 | low | subframe ignored for Raw stream captures (monochrome) | fixed: Raw captures fall back to full frame with an error log |
| 6 | formatting | two calls wrapped over lines in rewritten code | fixed |

## Retest
Full hardware scope rerun after the fixes; see [verification.md](verification.md).
