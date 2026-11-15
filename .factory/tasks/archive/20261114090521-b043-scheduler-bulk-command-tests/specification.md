# Specification

Status: approved by owner request 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
Covered before: hints, item order, running task under RESET_ALL, urgent order, counters, DELETE_ALL rejected while a task runs, hooks kept, pending urgent deleted, hook items unchanged, restart. Not covered: repeat/retry counters and script errors under RESET_ALL, disabled flag, ignored task name, empty lists, rerun after START, DELETE_ALL during a pending stop and during the shutdown hook, case-insensitive hook names, property removal, history, name/slot reuse; JSON transport of the hints.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | RESET_ALL clears repeat and retry counters, rechecks a script error, keeps a disabled task disabled, ignores the task name, reports 0 on an empty list, and finished work runs again after START | integration reset_all_runs_finished_work_again |
| AC-002 | DELETE_ALL is rejected while a stop waits for the task and while the shutdown hook runs, accepted once stopped; a hook name in another case keeps its task; the deleted task's property is removed; the history is unchanged; name and slot are reusable; empty list reports 0 | integration delete_all_waits_for_the_shutdown_hook |
| AC-003 | JSON item-hint gap recorded as a finding | findings check |
| AC-004 | SCHEDULER.md testing list, MIGRATION_STATUS 57 / 0 | review, recorded run |

## Failure behavior and edge cases
See criteria.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-14: JSON item hints recorded as a finding only.
