# Specification
Status: final

## Problem and outcome
The user must learn before and at START that the current position becomes the innermost one.

## In scope
The X_CALIBRATE label and a message at the first start step.

## Out of scope
Changes to the calibration sequence.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The X_CALIBRATE label says the focuser has to be fully in ("Calibrate focuser (fully in first)"). | `focuser_calibration` (fails without it) |
| AC-002 | After the first successful start step, X_CALIBRATE publishes "The current position is stored as the innermost one". | `focuser_calibration` (fails without it) |
| AC-003 | The calibration behaviour and the rest of the suite are unchanged. | recorded run |

## Failure behavior and edge cases
A failed first step publishes the failure, not the hint.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
- Driver version 35 → 36.
- The property name is unchanged.
- The label still starts with "Calibrate focuser", the text the README refers to.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-23: the requester chose option (a).
