# Specification
Status: final

## Problem and outcome
The X_CALIBRATE handler of `focuser_primaluce` 3.0.0.27 blocks the device queue for up to about
12 s at START and 5 s at END. Each wait has to run as a finalizer.

## In scope
The calibration start steps, the SESTO SENSO 3 restart wait, the range poll after END, abort during
the start, and disconnect.

## Out of scope
The library's blocking `primalucelib_wait_for_restart()`, which the WiFi mode change still uses, and
unrelated driver handlers.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | During the SESTO SENSO 3 restart wait and the start steps, other requests to the device are served while X_CALIBRATE is BUSY. | `sestosenso3_calibration_restart` (fails with 3.0.0.27). |
| AC-002 | During the range poll after END, other requests are served while X_CALIBRATE is BUSY. | `sestosenso3_calibration` (fails with 3.0.0.27). |
| AC-003 | An abort during the restart wait or the start steps cancels the calibration. X_CALIBRATE ends ALERT "Calibration aborted", no further step is sent, and the range is kept unless the SESTO SENSO 3 had entered the procedure. | `sestosenso3_abort_during_restart` (fails with 3.0.0.27). |
| AC-004 | The step order, the 1 s step spacing, the restart polling limits, the range polling limits and the published states and messages are as in 3.0.0.27. | Existing calibration cases; recorded run; hardware check. |
| AC-005 | The SESTO SENSO 2 calibration and the rest of the suite are unchanged. | Recorded run. |
| AC-006 | While the controller restarts, requests are refused at once ("The controller is restarting") instead of waiting for the read timeout, the idle polls pause without publishing ALERT, and an abort acts at once. | Library unit case; `sestosenso3_calibration_restart`, `sestosenso3_abort_during_restart`; hardware. |
| AC-007 | The late replies a SESTO SENSO 3 sends for the requests it received while restarting do not fail the next request. | `sestosenso3_calibration_restart` with a 5 s simulated restart (fails without the drain); hardware. |

## Failure behavior and edge cases
- A failed step ends the calibration ALERT.
- A restart without an answer ends it ALERT with "The focuser did not answer after the restart".
- A range that never becomes valid ends it ALERT with "The calibration left no travel range".
- Disconnect cancels the pending finalizers through the generator.

## Permissions, data, and security requirements
No new inputs; replies are parsed by the same library calls.

## Compatibility and nonfunctional requirements
Driver version 27 → 28. Only Linux is tested.

## Assumptions and open questions
None blocking.

## Owner decisions and requirement changes
- 2026-10-23: the requester asked for the fix, which is why it goes ahead of the overdue audit.
- 2026-10-23: the requester approved the supervised hardware check.
- 2026-10-23: the requester's hardware findings extended the scope with AC-006 and AC-007: requests
  waited behind the silent controller, and the calibration failed after the restart.
