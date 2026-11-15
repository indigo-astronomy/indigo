# Task brief

- Task ID: 20261112213513-7aaa-scheduler-phase-3a
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-12

## Problem and desired outcome
Owner request 2026-11-12: "start with 1 and then phase 3a". Step 1: run the Scheduler Agent in a real `indigo_server` with the CCD, mount and guider simulators (non-interactive simulator run, owner choice). Phase 3a: startup, shutdown and idle hooks; run history; duration estimate (`Sequence.prototype.estimate()`) and measured durations.

## Users and use cases
Observers preparing an unattended night: unpark and cool at start, park when idle and at the end, check in the morning what ran.

## Scope and non-goals
In scope: server acceptance test `indigo_test/integration/run_scheduler_simulators.py`; agent hooks, history, estimate and measurement; `Sequencer.js` `estimate()`; tests; docs; regenerated MCP help. Non-goals: night plan preview (phase 3b), safety (phase 4), web UI.

## Constraints and references
Branch `scheduler`; no commits before owner approval of the commit plan; design `indigo_docs/SCHEDULER.md`.

## Initial questions
Test mode decided by the owner: non-interactive simulator run.
