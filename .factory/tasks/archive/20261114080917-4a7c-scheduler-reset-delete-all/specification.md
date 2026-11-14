# Specification

Status: approved by owner answers 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
AGENT_SCHEDULER_TASK_COMMAND has RESET, ENABLE, DISABLE, MOVE_UP, MOVE_DOWN applied to the task named in AGENT_SCHEDULER_TASK_NAME; RESET of a running task is rejected and moves the task to the end of the urgent order. DELETE_TASK deletes one task, aborting it when it runs. Hooks name stored tasks by name.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Items RESET_ALL and DELETE_ALL appended after MOVE_DOWN, each with a `warn_on_set` hint; the task name is ignored | integration reset_all_keeps_running_task_and_urgent_order |
| AC-002 | RESET_ALL resets every task except a running one, keeps the relative order of pending urgent tasks and the disabled flag; the message gives the count and the running task | same |
| AC-003 | DELETE_ALL is rejected with Alert while the scheduler is started or a task/hook runs | integration delete_all_keeps_hooks_and_needs_a_stopped_scheduler |
| AC-004 | DELETE_ALL deletes stored and urgent tasks except the tasks named in AGENT_SCHEDULER_HOOKS; the hook items are unchanged; the message gives deleted and kept counts | same |
| AC-005 | Both results are saved and survive a restart | both cases |
| AC-006 | Docs, sandbox verbs, version 0x03000008, MIGRATION_STATUS 55 / 0 | review, recorded run |

## Failure behavior and edge cases
DELETE_ALL with only hook tasks reports 0 deleted. Without hooks it deletes everything.

## Permissions, data, and security requirements
The hint is advisory; clients that ignore hints (indigo_prop_tool, scripts) send the command without confirmation. DELETE_ALL's stopped-scheduler rule is the agent-side safeguard.

## Compatibility and nonfunctional requirements
Items appended, existing item positions unchanged.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-14: keep named hooks; DELETE_ALL only while stopped; warn_on_set on DELETE_ALL and RESET_ALL.
