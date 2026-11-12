# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001..003 | integration `test_agent_scripting_sequencer` "library reload keeps running sequence" | Scripting Agent with Sequencer.js | sequence, loop property and enabled flipper kept; idle objects replaced | before and after the fix |
| T-002 | AC-004 | integration `test_agent_scheduler` `library_reload_keeps_scheduled_task` | Scheduler and Scripting agents | task DONE, no failure | after the fix |
| T-003 | AC-005 | full Sequencer, Scripting, Scheduler suites, ASan | as recorded | no new failure | after the fix |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
T-001 failed before the fix (verification.md).

## Negative cases and permission boundaries
Idle reload still replaces the objects.

## Isolation and cleanup
Existing per-case processes and private HOME directories.

## Deferred tests and coverage limitations
A flip that is actually in progress is covered by the registration check only, not by a simulated meridian flip.
