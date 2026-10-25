# Specification
Status: final

## Problem and outcome
The case passes whichever side of the pier the mount takes.

## In scope
`indigo_test/integration/test_ccd_simulator.c`.

## Out of scope
Simulator and mount code.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The guide pulse shifts are checked for the side of the pier the mount reports after the slew. | the case; recorded run |
| AC-002 | The rest of the suite is unchanged. | recorded run 26/26 |

## Failure behavior and edge cases
None.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: look at the CCD simulator failure.
