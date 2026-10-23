# Specification
Status: final

## Problem and outcome
A refusal that the controller reports in the parameter's field must fail the request, and its text
must reach the client.

## In scope
The library acknowledgement handling for calibration steps, the direction write, the fine steps and
the ARCO sync. The driver messages for refused moves and syncs.

## Out of scope
Other requests (unchanged), and calibration procedure changes.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A `CAL_FOCUSER` step or `CAL_DIR` write answered with anything but `"done"` fails, and its text becomes `primalucelib_last_error()`. | `field_errors_refuse_calibration_requests` (fails without the change) |
| AC-002 | A calibration whose step is refused this way ends ALERT with the text, and no further step is sent. | `sestosenso3_field_errors` (fails without the change) |
| AC-003 | Focuser and rotator moves and ARCO syncs refused this way end ALERT with "Move refused: …" or "Sync refused: …". | `sestosenso3_field_errors` |
| AC-004 | `"done"` replies behave as before on both models, and the rest of the suite is unchanged. | recorded run |

## Failure behavior and edge cases
- An empty or missing acknowledgement fails the request without a reason, as before for moves.
- A refusal under an `ERROR` key keeps its existing handling.

## Permissions, data, and security requirements
The reply text reaches the client only as a message, through the existing bounded copy.

## Compatibility and nonfunctional requirements
- Driver version 33 → 34.
- Linux only.
- No SESTO SENSO 2 hardware was available. INDI requires "done" from both models.

## Assumptions and open questions
Item 2 (the closed-end search) waits for the requester's decision.

## Owner decisions and requirement changes
2026-10-23: merge first, then fix item 1.
