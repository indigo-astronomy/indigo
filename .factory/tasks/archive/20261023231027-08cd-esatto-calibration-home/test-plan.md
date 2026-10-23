# Test and regression plan

Status: final

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `esatto_profile` | `esatto` profile | no X_CALIBRATE, no `CAL_FOCUSER` | after implementation |
| T-002 | AC-002 | `esatto_needs_calibration` | `esatto-needs-calibration` profile | sensor error and the ESATTO message | after implementation |
| T-003 | AC-003 | `esatto_home_reset` | `esatto` profile | 17500 to 400 ends ALERT at 0 with the reason; 500 then runs OK | after implementation |
| T-004 | AC-005 | recorded run | all profiles | 71/71 | final |

## Baseline evidence
69/69 at 3.0.0.38, recorded in the driver README.

## Bug reproduction
The three cases were run against the unchanged driver and failed (verification.md, V3).

## Negative cases and permission boundaries
An aborted move and a needs-calibration flag keep their existing reasons.

## Isolation and cleanup
Each case starts its own simulator in a temporary fixture directory.
