# Specification
Status: final

## Problem and outcome
A calibration whose final position lies outside the range it has just stored must not end OK.

## In scope
The END completion of the focuser calibration.

## Out of scope
Reactions to the out-of-range reading at connect or during idle polling.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | When the position read after END lies outside the new range, X_CALIBRATE ends ALERT with "The focuser reports a position outside the calibrated range, run the calibration again". The position published is the reported one, and the range is the one the controller stored. | `sestosenso3_calibration_lost_position` (fails without the check) |
| AC-002 | A new calibration can be started right afterwards. | same case |
| AC-003 | A calibration ending inside the range ends OK, as before, and the rest of the suite is unchanged. | `sestosenso3_calibration`, `focuser_calibration`, recorded run |

## Failure behavior and edge cases
- A failed position read keeps the earlier behaviour: no position check, and the calibration is
  judged on the range alone.
- An uncalibrated result still fails first, with "The calibration left no travel range".

## Permissions, data, and security requirements
No new inputs.

## Compatibility and nonfunctional requirements
Driver version 30 → 31. Linux only.

## Assumptions and open questions
The meaning of 1000000 is inferred from the logs, because the protocol document does not describe
it. The check is generic, so it covers any out-of-range reading.

## Owner decisions and requirement changes
2026-10-23: the requester asked to fail the calibration with a message to rerun it.
