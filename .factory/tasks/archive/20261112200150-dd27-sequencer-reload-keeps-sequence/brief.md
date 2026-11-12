# Task brief

- Task ID: 20261112200150-dd27-sequencer-reload-keeps-sequence
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-12

## Problem and desired outcome
Reloading `Sequencer.js` replaced the `indigo_sequencer` singleton and deleted the loop properties, orphaning a running sequence (FND-20261112184305-e0a6-scheduler-agent-core-002). Clients send the library before every script, so any script run during a scheduled task broke it. Outcome: a reload keeps a running sequence and an enabled flip.

## Users and use cases
Scheduler Agent runs, web UI and AIN users running scripts while a sequence executes.

## Scope and non-goals
In scope: `Sequencer.js` reload logic, `Sequencer.md`, regression tests in the sequencer and scheduler suites, regenerated `Sequencer.js.dat`, versions of the Scripting and Scheduler agents, design text. Non-goals: other Sequencer behaviour.

## Constraints and references
Owner instruction 2026-11-12: "commit and fix the sequencer" (branch `scheduler`).

## Initial questions
None.
