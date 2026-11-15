# Specification

Status: approved (design task b0c64a17a and owner request 2026-11-13)

## Problem and outcome
See brief.md and SCHEDULER.md "Night hooks".

## Current-state audit
- Agent `indigo_agent_scheduler.c` 0x03000005: hooks `STARTUP` (once per START, `startup_pending`), `SHUTDOWN` (every stop, `shutdown_ran`), `IDLE` (once per idle period, `idle_since`/`idle_hook_done`, reset on task start); `finish_hook()` handles hook results; a failed STARTUP stops without SHUTDOWN. All scheduling on the handler queue (`process()` every 0.5 s); bus observations under `observed.mutex`. Persistence through `save_config()` (SETTINGS, HOOKS, history). Plan `update_plan()` over `indigo_scheduler_plan()` until `indigo_scheduler_night_end()` (sunrise).
- Sky module: 48 h grid from now at 5 min; no past samples, so not usable for night starts up to 4 h back (offsets) — the night module samples the Sun itself.
- Tests: unit header/sky/plan; integration `test_agent_scheduler` 36 cases (real time, real Scripting Agent, stand-in Mount and Guider agents, site movable); server acceptance `run_scheduler_simulators.py` 7 cases; recorded run 43/43 (3.0.0.5).
- No known defects in the affected code; design gap: no per-night hooks (design task).
- Platforms: Linux, macOS, Windows projects; pure C, no OS-specific code added.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Nights from the Sun at CIVIL/NAUTICAL/ASTRONOMICAL with offsets ±240 min, merged, dropped, polar cases | unit night |
| AC-002 | NIGHT_START once per night with work, NIGHT_END after it at the night end, never interrupting a task; END before START; START in night handled after STARTUP | unit night, integration |
| AC-003 | Night without work: no hooks, message once; late work runs NIGHT_START before it | unit night, integration, server acceptance |
| AC-004 | Standby: set by IDLE/NIGHT_END, cleared by successful RESUME/NIGHT_START, task start and START; RESUME only in standby; IDLE not in standby; night hooks begin idle periods | integration |
| AC-005 | Failed NIGHT_START/RESUME runs SHUTDOWN and stops with Alert | integration |
| AC-006 | Shared hook task runs once when due back to back; missing hook task reported; empty items change nothing | integration |
| AC-007 | Plan shows night hooks with reserved durations, horizon to the night end, none on a night without work; no-night item and message | unit plan, integration |
| AC-008 | AGENT_SCHEDULER_NIGHT, offsets and hook items persisted | integration persistence |
| AC-009 | Configurations without night hooks behave as before | the 36 earlier cases, idle_without_resume_behaves_as_before |
| AC-010 | Devices really prepared and put away | server acceptance with CCD and mount simulators |
| AC-011 | Names, PROPERTIES.md, docs, projects, version 0x03000006, MIGRATION_STATUS count | review, builds |

## Failure behavior and edge cases
AC-002..AC-006; site unknown: no night decisions; hand-started sequence: hooks wait.

## Permissions, data, and security requirements
No new input beyond property values bounded by the framework; hook names resolved by lookup.

## Compatibility and nonfunctional requirements
Old configurations load (new items default); night computation hourly, 73 Sun positions + 865 altitudes.

## Assumptions and open questions
Hardware testing: not performed (agent is device-independent; the server acceptance test uses the CCD and mount simulators). No hardware validation is claimed.

## Owner decisions and requirement changes
- 2026-11-13: design decisions as in brief.md.
- 2026-11-14: implementation detail differing from the design text, documented in SCHEDULER.md: any task start also ends the standby, so that IDLE without RESUME still runs once per idle period as before (AC-009); nights come from the agent's own Sun sampling, not the 48 h grid.
