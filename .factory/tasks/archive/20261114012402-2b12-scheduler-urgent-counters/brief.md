# Task brief

- Task ID: 20261114012402-2b12-scheduler-urgent-counters
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-14

## Problem and desired outcome
Owner report 2026-11-14: "scheduler does not show how many urgent tasks completed". One-off urgent tasks leave the task list when they end, so AGENT_SCHEDULER_STATE never counts them. Owner decisions: new items URGENT_DONE and URGENT_FAILED placed before TOTAL; kept with the history and reset by AGENT_SCHEDULER_CLEAR_HISTORY.

## Users and use cases
Observers using urgent follow-ups (alerts, transients).

## Scope and non-goals
In scope: agent, names header, PROPERTIES.md, SCHEDULER.md, user guide, integration test, sandbox status line, MIGRATION_STATUS count, recorded run. Non-goals: changing DONE/FAILED/TOTAL.

## Constraints and references
AGENT_SCHEDULER_STATE is read-only, so it is not saved; the counters travel in the saved RESTORE_HISTORY text.

## Initial questions
Answered by the owner (above).
