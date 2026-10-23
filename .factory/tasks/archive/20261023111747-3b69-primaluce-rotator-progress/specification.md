# Specification
Status: final

## Problem and outcome
ROTATOR_POSITION must publish the measured angle while a move runs.

## In scope
The rotator movement finalizer.

## Out of scope
Other properties.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | While a rotator move is BUSY, ROTATOR_POSITION publishes angles between the start and the target. | `rotator_move`, `sestosenso3_rotator_move` (fail without the fix); hardware |
| AC-002 | Completion, abort and failure behaviour of rotator moves is unchanged. | rotator cases; recorded run |
| AC-003 | The first BUSY update of a focuser or rotator move shows the measured position as value and the request as target. | `absolute_move`, `rotator_move`, `sestosenso3_rotator_move` (fail without the change); hardware |

## Failure behavior and edge cases
Unchanged: a failed readback ends the move ALERT.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
- Driver version 32 to 33.
- Publications are at most every 0.2 s, the poll period.
- Only Linux is covered.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-23: the requester reported the frozen angle.
