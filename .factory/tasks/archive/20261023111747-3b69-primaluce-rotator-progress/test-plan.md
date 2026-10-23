# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | simulator `rotator_move`, `sestosenso3_rotator_move` | normal and sestosenso3 profiles | angle between 10 and 200 while BUSY, then 200 OK | before (fails) and after |
| T-002 | AC-002 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |
| T-003 | AC-001, AC-003 | supervised hardware | requester's SESTO SENSO 3 with ARCO | angle counts up during the move; the first BUSY value is the measured angle | after |
| T-004 | AC-003 | simulator `absolute_move`, `rotator_move`, `sestosenso3_rotator_move` | first BUSY value from the test client log | the measured start position, not the request | before (fails) and after |

## Baseline evidence
Recorded 65/65 at 3.0.0.32.

## Bug reproduction
T-001 fails without the fix: "Rotator never published an angle between 10 and 200 while moving".

## Negative cases and permission boundaries
`rotator_abort` and `rotator_move_failure` are unchanged.

## Isolation and cleanup
One simulator per case. The server is stopped after the hardware check.

## Deferred tests and coverage limitations
Only Linux is run.
