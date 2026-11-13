# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | indigo_test/unit/test_agent_scheduler_night.c (7 night computation cases) | fixed dates and sites | nights on the limits, offsets, merges, polar | before integration |
| T-002 | AC-002, AC-003 | same file, 9 decision cases | synthetic and real nights | once per night, order, skip, late work | before integration |
| T-003 | AC-007 | indigo_test/unit/test_agent_scheduler_plan.c hooks_reserve_time | November site | hooks reserve time, follow runs | before integration |
| T-004 | AC-002..AC-009 | indigo_test/integration/test_agent_scheduler.c, 16 new cases and persistence | site longitude moved to dusk or dawn | see verification.md | after agent |
| T-005 | AC-010 | indigo_test/integration/run_scheduler_simulators.py night case | private server, CCD and mount simulators | cooler on/unparked, then off/parked | after agent |
| T-006 | AC-009 | 36 earlier integration cases | unchanged | pass | after agent |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Late skip notice: hooks_named_during_a_night_without_work fails without the fix (mutation run).

## Negative cases and permission boundaries
Failed NIGHT_START, RESUME, NIGHT_END; missing hook task; no night; night without work.

## Isolation and cleanup
Each integration case in its own process with a private HOME; server test in a private temporary directory, removed after a passing run; scratch files in the session scratchpad.

## Deferred tests and coverage limitations
No hardware; Linux only; site unknown during a handled night covered by the unit decision tests only.
