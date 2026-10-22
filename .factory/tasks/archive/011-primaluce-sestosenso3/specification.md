# Specification
Status: final

## Problem and outcome
On a SESTO SENSO 3 the requester saw `FOCUSER_POSITION` alternate between OK and ALERT, and moves
failed. `focuser_primaluce` must work with SESTO SENSO 2 and 3, following the protocol document
"PrimaluceLab - Communication Protocol - SS3 v1.0" (06 Oct 2025, requester's copy, not committed).
The simulator must model the SESTO SENSO 3.

## In scope
The library request/reply link, SESTO SENSO 3 moves and semi-automatic calibration in the driver,
the SESTO SENSO 3 simulator model, regression tests, and validation on the requester's unit.

## Out of scope
SESTO SENSO 3 SC automatic calibration, `RECOVER_DELAY`, stored positions 10 to 12 (new
properties), FND-000052 (WiFi credential escaping).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A reply that answers an earlier request (late, duplicate, or after a restart) never answers a later request; the link recovers by itself. | Library unit cases; SESTO SENSO 3 restart case; hardware moves. |
| AC-002 | An uncalibrated SESTO SENSO 3 refuses moves with a reason instead of accepting moves that never run. | Simulator case; hardware. |
| AC-003 | SESTO SENSO 3 calibration START and START_INVERTED work, including the controller restart after a direction change; END stops the run and publishes the new range at once, without a reconnect. | Simulator cases; supervised hardware calibration. |
| AC-004 | While a calibration step runs, further X_CALIBRATE requests are refused. While the run moves, only END is accepted. END without a run is refused. | Simulator cases; hardware. |
| AC-005 | Abort ends a SESTO SENSO 3 calibration run with the calibration commands. | Simulator case (hardware not exercised). |
| AC-006 | A move that the controller acknowledges but never runs ends ALERT with a reason. | Simulator case. |
| AC-007 | SESTO SENSO 2 behaviour and the existing suite are preserved. | Recorded run of the full suite. |

## Failure behavior and edge cases
- Up to `PRIMALUCE_MAX_STALE_REPLIES` stale replies are dropped. After that, or after a timeout,
  the request fails and the link is marked failed.
- A restart that never answers fails START with ALERT.
- A range that never becomes valid after END ends the calibration ALERT, and the focuser stays
  uncalibrated.

## Permissions, data, and security requirements
No new inputs from clients beyond existing switch items. Controller replies are parsed by the
existing jsmn path; verb extraction is bounded.

## Compatibility and nonfunctional requirements
- Portable INDIGO APIs only.
- The driver version goes from 26 to 27.
- Built and tested on Linux x64 only; macOS and Windows are not verified (recorded as a limitation).

## Assumptions and open questions
The restart and `CAL_MAXPOS` delays are firmware 1.0.9 observations. The polling tolerates other
timings within 10 s (restart) and 5 s (range).

## Owner decisions and requirement changes
- 2026-10-23, requester: the overdue repository audit is deferred until after this task.
- 2026-10-23, requester: hardware work is approved (small moves, supervised calibrations).
