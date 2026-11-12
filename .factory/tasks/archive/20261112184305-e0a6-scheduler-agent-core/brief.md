# Task brief

- Task ID: 20261112184305-e0a6-scheduler-agent-core
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-12

## Problem and desired outcome
Implement phase 2 of the scheduler design in `indigo_docs/SCHEDULER.md`: the core of a new Scheduler Agent (`indigo_drivers/agent_scheduler/`) that stores tasks (header + `Sequencer.js` body), selects them by constraints and priority between tasks, runs them through the Scripting Agent and lets urgent tasks preempt non-urgent ones.

## Users and use cases
Observers running unattended nights; clients (AIN, web UI, MCP, scripts) uploading tasks and urgent targets.

## Scope and non-goals
In scope (phase 2): header parser and validation; task store, properties and config persistence of tasks and status; wrapper script, run, abort and urgent preemption; ephemeris grid and constraints; selection, scoring, repeat and retry; scheduler control; server, build and project registration; `indigo_names.h`, `PROPERTIES.md`; unit and integration tests.
Non-goals: any change to `Sequencer.js` (owner instruction 2026-11-12: "do not modify the sequencer yet"), hooks, night plan preview, run history, duration estimate and measurement, safety, web UI.

## Constraints and references
- Design: `indigo_docs/SCHEDULER.md` (branch `scheduler`).
- Owner instructions 2026-11-12: work on branch `scheduler`; follow the general and design rules; do not modify the sequencer; source files are copyright Rumen G. Bogdanovski.
- Rules: root AGENTS.md, indigo_drivers/AGENTS.md, indigo_test/AGENTS.md, indigo_libs/AGENTS.md, indigo_docs/AGENTS.md.

## Initial questions
None blocking; deviations are recorded in plan.md.
