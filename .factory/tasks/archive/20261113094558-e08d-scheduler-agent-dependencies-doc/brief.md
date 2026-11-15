# Task brief

- Task ID: 20261113094558-e08d-scheduler-agent-dependencies-doc
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
The owner saw "site coordinates unknown" for all tasks: the server had no Mount Agent loaded, and the documents did not say plainly which agents the Scheduler Agent needs. Owner request 2026-11-13: "please add in the design and the users guide which agents it depends on directly". The owner committed the first version (e33784568) and asked for a review and fixes.

## Users and use cases
Observers setting up a server for the scheduler.

## Scope and non-goals
In scope: `indigo_drivers/agent_scheduler/doc/SCHEDULER.md` and `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`. Non-goals: code changes.

## Constraints and references
`indigo_agent_scheduler.c` (fixed agent names, `observe_property()`, `agent_busy_bit()`, aborts), `indigo_agent_scheduler_sky.c` (`needs_sky()`), `indigo_agent_scheduler_header.c` (keys that require `@target`), `indigo_agent_mount.c` (`AGENT_SITE_DATA_SOURCE`, `handle_site_change()`).

## Initial questions
None.
