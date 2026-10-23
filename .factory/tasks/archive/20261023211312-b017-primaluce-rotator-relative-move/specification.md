# Specification
Status: final

## Problem and outcome
ROTATOR_RELATIVE_MOVE is shown and moves the ARCO by the requested offset.

## In scope
The ROTATOR_RELATIVE_MOVE handler, its completion and abort, and the PROPERTIES.md entry.

## Out of scope
Wrap-around, and other rotator properties.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | ROTATOR_RELATIVE_MOVE is shown. A request moves the ARCO with `MOVE_ABS` to the current angle plus the offset, ROTATOR_POSITION publishes the progress, and both properties end OK. | `rotator_relative_move`, `sestosenso3_rotator_relative_move` |
| AC-002 | A target outside the ROTATOR_POSITION range is refused with a reason and no command. | the same cases |
| AC-003 | An abort during a relative move ends both properties ALERT. | the same cases |
| AC-004 | Relative moves from a reached target do not accumulate the SESTO SENSO 3 rounding. | `sestosenso3_rotator_relative_move` (failed before the fix) |
| AC-005 | The rest of the suite is unchanged. | recorded run |

## Failure behavior and edge cases
- A failed angle read ends the move ALERT, with a reason.
- A refused `MOVE_ABS` gives "Move refused: …".
- A move of an ARCO that needs calibration ends with that reason, through ROTATOR_POSITION.

## Permissions, data, and security requirements
No new inputs beyond the offset, which the framework range-checks (-180..180).

## Compatibility and nonfunctional requirements
- Driver version 37 to 38.
- PROPERTIES.md is updated.
- Only Linux was run.

## Assumptions and open questions
No wrap-around, as in the ZWO driver.

## Owner decisions and requirement changes
2026-10-24: the requester asked to proceed with the relative move.
