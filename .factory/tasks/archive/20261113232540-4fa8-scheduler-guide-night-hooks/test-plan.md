# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-002 | private server, the three task files extracted from the guide, the documented commands | indigo_server -p 7711 with the Scripting and Scheduler agents, scratch HOME | uploads OK, offset -30, hooks labelled | after edit |
| T-002 | AC-004 | anchor script, git diff --check | repository | no broken link | after edit |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
Private server stopped; scratch files in the session scratchpad.

## Deferred tests and coverage limitations
The night timeline itself is covered by the recorded run of c9f5cbc5f (60/60), not repeated.
