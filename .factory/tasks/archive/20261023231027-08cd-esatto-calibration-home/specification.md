# Specification
Status: final

## Problem and outcome
An ESATTO offers no calibration it cannot run, and a move that its home sensor ends early gives that reason.

## In scope
The model handling at connect, the move completion message, the simulator ESATTO profiles, the tests, and the
documentation (PROTOCOL.md, PROPERTIES.md, REFACTOR.md).

## Out of scope
The relative `MOVE` command, and other models' behaviour.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | On an ESATTO, X_CALIBRATE is not defined and no `CAL_FOCUSER` is sent. A SESTO SENSO still shows it. | `esatto_profile`; `focuser_calibration`, `sestosenso3_calibration` |
| AC-002 | An ESATTO flagged by `CALRESTART` gives "<model> needs calibration, which an ESATTO cannot run from the driver" and the controller's error text. | `esatto_needs_calibration` |
| AC-003 | On an ESATTO, a move that ends at 0 short of a target above 0 ends ALERT with "The focuser reached its home sensor and reset its position to 0". The next move runs normally. | `esatto_home_reset` |
| AC-004 | The documentation no longer says the ARCO has no relative command. | review of the diff |
| AC-005 | The rest of the suite is unchanged. | recorded run |

## Failure behavior and edge cases
- An aborted move keeps no reason, as before.
- A needs-calibration flag takes precedence over the home-sensor reason.

## Permissions, data, and security requirements
No new inputs.

## Compatibility and nonfunctional requirements
- Driver version 38 to 39.
- Only Linux was run.

## Assumptions and open questions
The home-sensor reset is inferred from one hardware log. The simulator models its limit on `CAL_HOMEP` (1000);
this is an assumption.

## Owner decisions and requirement changes
2026-10-24: the requester approved hiding the calibration, the home-sensor reason and the wording correction.
