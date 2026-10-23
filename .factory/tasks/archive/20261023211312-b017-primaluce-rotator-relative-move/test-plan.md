# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 to AC-003 | simulator `rotator_relative_move` | normal | +30 to 30, -50 to -20, -170 refused, +170 aborted | after |
| T-002 | AC-001 to AC-004 | simulator `sestosenso3_rotator_relative_move` | sestosenso3 (angle rounding, DEG acknowledgement) | same, with exact targets | before (fails: -20.00006) and after |
| T-003 | AC-005 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |

## Baseline evidence
Recorded 67/67 at 3.0.0.37.

## Bug reproduction
T-002 failed before the origin change: the request was -20.00006 instead of -20.

## Negative cases and permission boundaries
The out-of-range refusal and the abort.

## Isolation and cleanup
One simulator per case.

## Deferred tests and coverage limitations
- No hardware check of the relative move yet; it is offered with the commit plan.
- Only Linux was run.
