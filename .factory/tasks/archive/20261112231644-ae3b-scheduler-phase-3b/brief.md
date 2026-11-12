# Task brief

- Task ID: 20261112231644-ae3b-scheduler-phase-3b
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
Owner request 2026-11-13: "please proceed with 3b". Phase 3b of `indigo_docs/SCHEDULER.md`: the night plan preview `AGENT_SCHEDULER_PLAN`, after the next-start search is made cheaper (FND-20261112184305-e0a6-scheduler-agent-core-003).

## Users and use cases
Observers who want to see before and during the night which tasks are expected to run when, and why the others will not run tonight.

## Scope and non-goals
In scope: per-task constraint tables and their use in selection; the plan simulation module; the plan property; tests; docs; project files. Non-goals: safety (phase 4), web UI, any change of the selection rules.

## Constraints and references
Branch `scheduler`; no commits before owner approval of the commit plan; design `indigo_docs/SCHEDULER.md` "Ephemeris grid" and "Night plan preview".

## Initial questions
None; test mode as in phase 3a: non-interactive simulator run.
