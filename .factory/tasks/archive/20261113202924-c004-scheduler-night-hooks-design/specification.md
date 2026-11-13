# Specification

Status: approved by owner request 2026-11-13 (design only)

## Problem and outcome
See brief.md.

## Current-state audit
Hooks: `STARTUP` once per `START`, `SHUTDOWN` once per stop, `IDLE` once per idle period (`idle_hook_done`, reset when a task starts). A task may be named by several hooks; `hook_kind()` labels the first. The Mount Agent unparks the mount, and the dome it controls, before a slew (`unpark_before_coordinates()` from `mount_dome_control()`). The guide said a hook "runs every night".

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | SCHEDULER.md describes, as planned work, a scheduler-wide night limit (civil, nautical, astronomical) with start/end offsets, the `NIGHT_START`, `NIGHT_END` and `RESUME` hooks and their rules: no interruption of a task, night state, standby, order, shared tasks, unnamed hooks, failures, stop, night plan, history, persistence | review |
| AC-002 | Tasks keep their own `@twilight`; the night limit times only the hooks | review |
| AC-003 | The survey has a night start/end row; the phase list has the night hooks as phase 5 with later phases renumbered; the testing section lists the planned tests | review |
| AC-004 | The guide describes the current multi-night behaviour correctly and points to the plan; nothing planned is described as delivered | review against the code |
| AC-005 | All links and anchors resolve; no whitespace errors | script, `git diff --check` |

## Failure behavior and edge cases
Covered by AC-001 (no night, offsets, failures, simultaneous hooks).

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Configurations without the new items behave as now.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-13: one night limit, tasks keep `@twilight`; `NIGHT_END` does not interrupt; no sky flats; design only.
- 2026-11-13: night hooks skipped on a night without planned work; `NIGHT_START` runs late when work appears ("yes change the design"). Delaying `NIGHT_START` until the first planned run was offered and not adopted.
- 2026-11-13: owner asked for thorough tests of this behaviour; the planned phase-5 test list in SCHEDULER.md "Testing" was expanded (unit, plan, integration, server acceptance with a settable clock). Test code comes with the implementation.
- 2026-11-13: owner approved the defaults (`NAUTICAL`, offsets 0, range ±240 min) and `SHUTDOWN` after a failed `NIGHT_START` or `RESUME` ("i agree with both as suggested").
