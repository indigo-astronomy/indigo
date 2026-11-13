# Task brief

- Task ID: 20261113215617-5678-scheduler-night-hooks
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
Implement phase 5 of the Scheduler Agent design, the night hooks, as approved in task [20261113202924-c004-scheduler-night-hooks-design](../20261113202924-c004-scheduler-night-hooks-design/specification.md) (b0c64a17a). Owner request 2026-11-13: "plese implement". Earlier owner request: "this should be tested well add tests for this behaviour".

## Users and use cases
Observers running the scheduler unattended over several nights: prepare the equipment (cooler, roof, cover) at the start of each night with work, put it away at its end, resume after an idle park.

## Scope and non-goals
In scope: `indigo_drivers/agent_scheduler/` (agent, new night module, plan module, version), `indigo_libs/indigo/indigo_names.h`, unit, integration and server acceptance tests, test Makefile, Xcode and Visual Studio projects, `indigo_docs/PROPERTIES.md`, `SCHEDULER.md`, the user guide, `MIGRATION_STATUS.md` counts, recorded test run. Non-goals: sky flats, safety (phase 6), related Scripting Agent (phase 4), web UI.

## Constraints and references
SCHEDULER.md "Night hooks"; owner decisions in the design task: one scheduler-wide night limit, tasks keep `@twilight`, `NIGHT_END` never interrupts, nights without work skip both hooks, `NAUTICAL` and offsets 0 (±240 min), `SHUTDOWN` after a failed `NIGHT_START`/`RESUME`.

## Initial questions
None.
