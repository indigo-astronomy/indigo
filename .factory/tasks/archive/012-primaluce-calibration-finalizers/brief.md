# Task brief

- Task ID: 012-primaluce-calibration-finalizers
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
Task 011 left a documented limitation in `focuser_primaluce` 3.0.0.27: the X_CALIBRATE handler
blocks the device queue. START waits up to about 12 s (the steps and a SESTO SENSO 3 restart), and
END waits up to 5 s for the new range. The requester asked for this to be fixed. Desired outcome:
the waits run as finalizers, abort and other requests are served between them, and the calibration
behaviour is otherwise unchanged.

## Users and use cases
Owners of SESTO SENSO 2 and 3 focusers who calibrate through INDIGO clients.

## Scope and non-goals
In scope: the calibration start, the restart wait and the range poll of `focuser_primaluce`, abort
during the start, regression tests, and a supervised hardware check.
Non-goals: library changes, other blocking code in the driver, and the task 011 non-goals.

## Constraints and references
Root, driver and test AGENTS.md (handler + finalizer rule, generator `_finalizer` semantics); task
011 (archived); the requester's SESTO SENSO 3 unit (firmware 1.0.9).

## Initial questions
Resolved:
- The recurring repository audit is overdue. The requester explicitly asked for this follow-up of
  task 011 on the same driver (2026-10-23), so the audit follows after this task.
- The requester approved a supervised hardware check (2026-10-23).
