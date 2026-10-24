# Specification
Status: final

## Problem and outcome
A cover request accepted while an idle poll is in flight is executed as requested.

## In scope
`indigo_aux_alto` on_timer; `alto_simulator` delayed reply; `test_aux_alto_simulator`.

## Out of scope
Other handlers; the GIOTTO.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A position requested while a poll that sees motion started elsewhere is in flight is sent once, and the cover ends OK at the request with the request as target. | `request_versus_poll` (fails on 3.0.0.1) |
| AC-002 | External motion, poll failures and recovery still behave as before. | the rest of the suite |

## Failure behavior and edge cases
Outside a move, a BUSY cover property suppresses publication, motion following and the failed-poll ALERT; the next poll looks again.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Driver version 1 to 2.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-24: fix both drivers in separate commits.
