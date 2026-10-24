# Test and regression plan

Status: final

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `usb_match_pattern` | synthetic port entries | only the CP2102N with `10c4:ea60` matches | after implementation |
| T-002 | AC-002 | recorded run | all profiles | 72/72 | final |

## Baseline evidence
71/71 at 3.0.0.39.

## Bug reproduction
Not a defect; `usb_match_pattern` was run against the unchanged driver and failed on the CP2102N name behind `0403:6001`.

## Negative cases and permission boundaries
The two non-matching entries above.

## Isolation and cleanup
Synthetic port entries only; no real port is opened by the case.
