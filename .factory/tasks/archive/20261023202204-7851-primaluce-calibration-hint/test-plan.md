# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | simulator `focuser_calibration` | normal profile | the label contains "fully in"; the message appears after START | before (fails) and after |
| T-002 | AC-003 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |

## Baseline evidence
Recorded result 67/67 at 3.0.0.35.

## Bug reproduction
T-001 fails without the label and without the message.

## Negative cases and permission boundaries
The failed-step path is covered by the existing `focuser_calibration` fault and `sestosenso3_field_errors`.

## Isolation and cleanup
One simulator per case.

## Deferred tests and coverage limitations
No hardware check: the change affects text only. Linux only.
