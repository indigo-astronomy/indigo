# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | simulator `focuser_calibration`, `sestosenso3_calibration` | normal and sestosenso3 profiles | message with the stored range | before (fails) and after |
| T-002 | AC-002 | simulator `rotator_calibration` | normal | "ARCO calibration complete" | before (fails) and after |
| T-003 | AC-003 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |

## Baseline evidence
67/67 on the merged master.

## Bug reproduction
T-001 and T-002 fail without the messages.

## Negative cases and permission boundaries
The failure paths are covered by the existing cases.

## Isolation and cleanup
One simulator per case.

## Deferred tests and coverage limitations
No hardware check, since only messages change; Linux only.
