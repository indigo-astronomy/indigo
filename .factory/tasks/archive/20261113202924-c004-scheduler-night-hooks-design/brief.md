# Task brief

- Task ID: 20261113202924-c004-scheduler-night-hooks-design
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
A scheduler left running for several nights runs `STARTUP` only on `START` and `SHUTDOWN` only on a stop; nothing runs at dusk or dawn, and nothing restores what the `IDLE` hook switched off. Owner request 2026-11-13: "there should be idle and resume hooks ... and also night start and night end which could be nautical or astronomical twilight when you can start camera cooler or stop camera cooler", compared with N.I.N.A., Ekos, SGP and the other schedulers of the design document. Owner decisions: one night limit for the whole scheduler, tasks keep their own `@twilight`; `NIGHT_END` never interrupts a task; no sky flats now; option 1: design only, reviewed by the owner before any code.

## Users and use cases
Observers running the scheduler unattended over several nights.

## Scope and non-goals
In scope: `indigo_drivers/agent_scheduler/doc/SCHEDULER.md` (planned design, survey, phase list, tests) and `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` (current multi-night behaviour and a pointer to the plan). Non-goals: code, `PROPERTIES.md`, tests, sky flats.

## Constraints and references
`indigo_agent_scheduler.c` (`hook_kind()`, `hook_slot()`, idle period, start/stop), `indigo_agent_mount.c` (`unpark_before_coordinates()`), `Sequencer.js`; competitor documentation cited in the conversation (ACP Scheduler, KStars handbook, Target Scheduler, SGP help, Voyager wiki). `indigo_docs/AGENTS.md`: label future work explicitly.

## Initial questions
None; owner decisions above.
