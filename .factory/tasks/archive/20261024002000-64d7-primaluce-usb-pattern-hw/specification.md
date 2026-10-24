# Specification
Status: final

## Problem and outcome
REFACTOR.md states the observed hardware result of the port preselection.

## In scope
`indigo_drivers/focuser_primaluce/REFACTOR.md`.

## Out of scope
Code, tests.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | REFACTOR.md reports the selected port, the ports present and the connect result, as logged. | comparison with the server log |

## Failure behavior and edge cases
Not applicable.

## Permissions, data, and security requirements
No serial numbers or MAC addresses are recorded.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-24: the requester asked for the record.
