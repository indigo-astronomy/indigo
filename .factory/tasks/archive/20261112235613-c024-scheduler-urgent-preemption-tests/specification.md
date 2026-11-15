# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
Integration cases cover retry and fail outcomes separately, without queued work or the night plan; the server acceptance run covers only `@preempted retry`.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | An urgent task inserted while a task runs and another waits preempts the running task, runs first and is shown Busy first in the plan; a retried task is planned again before the waiting one and reruns first without using its retry; history records the preemption | integration `urgent_insertion_retries_the_preempted_task` |
| AC-002 | The same with `@preempted fail`: the preempted task becomes FAILED, leaves the plan, never runs again, and the waiting task follows the urgent one | integration `urgent_insertion_fails_the_preempted_task` |
| AC-003 | With the simulators, a guided capture with `@preempted fail` is interrupted (exposure and guiding stopped), the urgent frames are saved, the task is FAILED and its capture does not continue | server case "urgent task fails a preempted capture" |
| AC-004 | No regression | recorded run |

## Failure behavior and edge cases
Covered by the cases above.

## Permissions, data, and security requirements
Not applicable; tests only.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
