# Task brief

- Task ID: 20261112075644-5de2-scheduler-design
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-12

## Problem and desired outcome
INDIGO runs one `Sequencer.js` sequence at a time and has no scheduler that selects tasks by sky constraints and priority, or interrupts work for an urgent target. The owner asked for a survey of the schedulers in other applications (Ekos, SGP, N.I.N.A., Voyager, ACP) and for a scheduler design. This task delivers the design document `indigo_docs/SCHEDULER.md`; implementation is a later task.

## Users and use cases
Observers running unattended nights with several targets; observatories that must react to urgent targets (supernovae, GRB alerts, comets).

## Scope and non-goals
In scope: the design document and its Xcode project reference. Non-goals: any code, property definitions in `indigo_names.h`, `PROPERTIES.md` entries (added when properties exist).

## Constraints and references
Owner decisions (2026-11-11/12): only urgent tasks preempt non-urgent tasks; scheduling decisions between tasks only; a preempted task is marked FAILED or RETRY; a task is a `// @key value` header followed by a `Sequencer.js` body; tasks are stored like Scripting Agent scripts, the agent does not read or write files. Approved plan: owner-approved Claude Code plan (session 2026-11-12). References: `indigo_drivers/agent_scripting/`, `indigo_libs/indigo/indigo_align.h`, `indigo_libs/indigo/indigocat/`.

## Initial questions
None open; property names and defaults are marked as subject to change in the document.
