# Specification

Status: approved by owner answers 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
update_counters() counts only tasks in the list; finish_task(), finish_stop() and the expiry in update_task_states() remove one-off urgent tasks; the history keeps their runs (mark U). The state property is read-only and therefore not saved.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | AGENT_SCHEDULER_STATE items in the order DONE, FAILED, URGENT_DONE, URGENT_FAILED, TOTAL | integration urgent_tasks_are_counted |
| AC-002 | A one-off urgent task that succeeds counts in URGENT_DONE; one that fails or expires in URGENT_FAILED; a deleted or aborted one is not counted; stored @priority urgent tasks stay in DONE/FAILED | same |
| AC-003 | The counters survive a restart and are reset (and saved reset) by AGENT_SCHEDULER_CLEAR_HISTORY | same |
| AC-004 | Older versions ignore the saved counter line; older configurations load with zero counters | review of restore_history() |
| AC-005 | Docs, names, version 0x03000007, MIGRATION_STATUS 53 / 0 | review, recorded run |

## Failure behavior and edge cases
Counters clamped to 0..1000000 on restore and increment.

## Permissions, data, and security requirements
None new.

## Compatibility and nonfunctional requirements
Clients reading TOTAL by index would see a different item; INDIGO clients address items by name.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-14: new items before TOTAL; reset with the history.
