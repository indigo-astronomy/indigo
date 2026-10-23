# Specification
Status: final

## Problem and outcome
A successful calibration announces its completion.

## In scope
The focuser END completion and the ARCO calibration completion.

## Out of scope
Failure messages (unchanged).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A successful focuser END publishes "Calibration complete, range <min> to <max>" with the stored range. | `focuser_calibration`, `sestosenso3_calibration` (fail without it) |
| AC-002 | A finished ARCO calibration publishes "ARCO calibration complete". | `rotator_calibration` (fails without it) |
| AC-003 | The rest of the suite is unchanged. | recorded run |

## Failure behavior and edge cases
Failures keep their reasons.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Driver version 36 to 37.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-23: the requester asked for the messages on both devices.
