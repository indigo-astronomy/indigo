# Task brief

- Task ID: 20261114095750-d502-scheduler-eligibility-and-device-tests
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-14

## Problem and desired outcome
Owner request 2026-11-14: extend the scheduler tests, especially task eligibility and runnability and more complex tasks with exposures, filter changes and coordinates; a fairly complete test with positive and negative edge cases, because the scheduler is hard to test by hand. Owner decision: defects found are reported with a proposed fix and implemented only after approval.

## Users and use cases
Maintainers and users of the Scheduler Agent.

## Scope and non-goals
In scope: integration cases, server acceptance cases with the CCD, wheel and mount simulators, SCHEDULER.md testing list, approved agent fixes, MIGRATION_STATUS count, recorded run, findings. Non-goals: hardware runs.

## Constraints and references
Existing unit tests cover each constraint's sky computation; the gaps were end-to-end eligibility, score, limits, delays and device tasks.

## Initial questions
Defect handling: owner chose report and wait. Defect 1 (priority inversion with pending estimates): owner approved the fix. Defect 2 (trailing whitespace across a restart): owner chose a finding.
