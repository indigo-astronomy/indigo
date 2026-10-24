# Specification
Status: final

## Problem and outcome
A rotator disconnect and reconnect must keep the ARCO calibrated.

## In scope
The rotator connect and disconnect.

## Out of scope
The ARCO calibration itself, and the focuser.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A rotator disconnect does not switch the ARCO port off. | `rotator_lifecycle` (fails with the old disconnect); hardware |
| AC-002 | A connect switches the port on only when the full state reports it off. | `rotator_lifecycle`, `rotator_metadata`; hardware |
| AC-003 | After a calibration, rotator and focuser disconnect/reconnect cycles leave `CALRESTART.MOT2` at 0. | `sestosenso3_arco_stays_calibrated` (fails with the old disconnect); hardware |
| AC-004 | The rest of the suite is unchanged. | recorded run |

## Failure behavior and edge cases
- A full state without the `ARCO` key switches the port on, as before.
- A failed switch-on fails the connection, as before.

## Permissions, data, and security requirements
No new inputs.

## Compatibility and nonfunctional requirements
- Driver version 34 → 35.
- After an INDIGO disconnect the ARCO port stays on. INDI behaves the same way.
- Linux only.

## Assumptions and open questions
Whether a power cycle also clears the ARCO calibration was not tested.

## Owner decisions and requirement changes
2026-10-23: the requester approved keeping the ARCO enabled.
