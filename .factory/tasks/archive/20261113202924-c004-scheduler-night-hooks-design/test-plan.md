# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-005 | anchor/link script | repository | no broken link | after edit |
| T-002 | AC-004 | review against `indigo_agent_scheduler.c`, `indigo_agent_mount.c`, `Sequencer.js` | none | statements match the code | after edit |
| T-003 | AC-001..AC-003 | independent review | fresh subagent | no contradiction left | after edit |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
Scratch files in the session scratchpad only.

## Deferred tests and coverage limitations
The planned tests are listed in SCHEDULER.md "Testing" for phase 5.
