# Task brief

- Task ID: 20261112235613-c024-scheduler-urgent-preemption-tests
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
Owner request 2026-11-13: "add tests for urgent task insertion and interrupting a running task ... in both cases with fail and retry the task".

## Users and use cases
Observers relying on urgent tasks (transients) to interrupt the night plan safely.

## Scope and non-goals
In scope: integration and server acceptance tests, test list in SCHEDULER.md, test counts and records. Non-goals: agent behaviour changes.

## Constraints and references
Branch `scheduler`; no commits before owner approval; non-interactive simulator mode as in the earlier scheduler tasks.

## Initial questions
Placement clarified by the owner: both preemption outcomes, fail and retry.
