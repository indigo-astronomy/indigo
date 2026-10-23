# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `test_primalucelib` `field_errors_refuse_calibration_requests` | scripted peer | the step and the write fail with the text; a "done" step passes and clears the reason | before (fails) and after |
| T-002 | AC-002, AC-003 | simulator `sestosenso3_field_errors` | `fielderror` fault | ALERT with the text for the calibration step, the focuser move, the rotator move and the sync | before (fails) and after |
| T-003 | AC-004 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |

## Baseline evidence
Recorded 65/65 at 3.0.0.33. The merged tree passed the quick check.

## Bug reproduction
T-001 and T-002 fail without the change. Without it, START answered with `Error: invalid parameter` does not end ALERT.

## Negative cases and permission boundaries
The `reject` fault (ERROR key) is unchanged in the existing cases.

## Isolation and cleanup
One simulator per case.

## Deferred tests and coverage limitations
- There is no hardware check, because the refusal cannot be provoked on demand.
- Only Linux was run.
