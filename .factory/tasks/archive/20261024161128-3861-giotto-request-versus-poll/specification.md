# Specification
Status: final

## Problem and outcome
A light switch or intensity request accepted while a poll is in flight is executed as requested.

## In scope
`indigo_aux_giotto` on_timer; `giotto_simulator` delayed reply; `test_aux_giotto_simulator`.

## Out of scope
Other handlers; the ALTO.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A brightness requested while a poll reading a different value is in flight is sent once; the poll's value is never sent; the intensity ends OK at the request with the request as target. | `request_versus_poll` (fails on 3.0.0.1) |
| AC-002 | External changes, poll failures and recovery still behave as before. | the rest of the suite |

## Failure behavior and edge cases
A BUSY property is skipped by both the OK publication and the failed-poll ALERT; the next poll compares again.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Driver version 1 to 2.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-24: fix both drivers in separate commits.
