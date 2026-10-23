# Specification
Status: final

## Problem and outcome
ARCO moves on a SESTO SENSO 3 end ALERT while the ARCO turns, and syncs fail. Moves, aborts and
syncs have to work on the SESTO SENSO 3 without changing SESTO SENSO 2 behaviour.

## In scope
The library acknowledgement handling for the ARCO move and sync, reply-line parsing, the driver's
rotator completion check, and the simulator SESTO SENSO 3 ARCO replies.

## Out of scope
ARCO calibration on the SESTO SENSO 3, focuser behaviour.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | An ARCO move acknowledged under `MOT2.DEG` or `MOT2.STEP` is accepted and ends OK at the target. | `sestosenso3_rotator_move`, `rotator_move`; hardware |
| AC-002 | A move that stops within 0.1° of the target ends OK. | `sestosenso3_rotator_move` (angle off by 0.00006°); hardware |
| AC-003 | A sync acknowledged under `MOT2.DEG` or `MOT2.SYNC_POS` is accepted. | `sestosenso3_rotator_sync`, `rotator_sync`; hardware |
| AC-004 | A reply line with text in front of the JSON object is parsed. | `debug_text_before_a_reply_is_skipped`; `sestosenso3_rotator_sync`; hardware |
| AC-005 | An abort of an ARCO move still stops it and ends ALERT, and the SESTO SENSO 2 cases are unchanged. | `rotator_abort`; recorded run; hardware |

## Failure behavior and edge cases
- A move that stops further than 0.1° from the target ends ALERT, as before.
- A refused sync keeps the previous angle.
- A line without any `{` is still a parse failure.

## Permissions, data, and security requirements
No new inputs. The prefix skip moves the reply within its own buffer.

## Compatibility and nonfunctional requirements
Driver version 28 to 29. Linux only.

## Assumptions and open questions
The DEG acknowledgement and the debug text are firmware 1.0.9 observations.

## Owner decisions and requirement changes
- 2026-10-23: the requester chose the 0.1° tolerance.
- 2026-10-23: the requester reported this defect directly after task 012, so it was fixed before the overdue repository audit.
