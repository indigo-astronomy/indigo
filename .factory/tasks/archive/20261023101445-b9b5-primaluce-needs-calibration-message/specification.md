# Specification
Status: final

## Problem and outcome
A move of a motor that the controller flags for calibration and that does not run must end ALERT
with the reason.

## In scope
The focuser and rotator move completion messages, and the calibration flags.

## Out of scope
Up-front refusal, other move failure paths.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A rotator move that ends short of its target while `CALRESTART.MOT2` was set at connect ends ALERT with "The rotator needs calibration". | `sestosenso3_rotator_needs_calibration`; hardware |
| AC-002 | A focuser move that ends short of its target while `CALRESTART.MOT1` was set ends ALERT with "The focuser needs calibration". | `needs_calibration` (extended) |
| AC-003 | After a successful calibration of that motor, moves work, and a later short move gives the generic reason. | `sestosenso3_rotator_needs_calibration` |
| AC-004 | Moves of a flagged SESTO SENSO 2 that do run still end OK, and the rest of the suite is unchanged. | `needs_calibration`; recorded run |

## Failure behavior and edge cases
- Without the flag, a move ending short of its target says "… stopped before reaching the target".
- An aborted move keeps its current behaviour.

## Permissions, data, and security requirements
No new inputs.

## Compatibility and nonfunctional requirements
Driver version 31 → 32. Linux only.

## Assumptions and open questions
The flag is read at connect. A flag the controller raises later, for example after `stop_calib`, is
covered by the existing SESTO SENSO 3 range handling for the focuser.

## Owner decisions and requirement changes
2026-10-23: the requester asked for a failure message with the reason.
