# Specification
Status: final

## Problem and outcome
`make -C indigo_test test-server-leaks` reports no definite or indirect loss.

## In scope
`indigo_ccd_detach()` releases `CCD_FPS_PROPERTY`; the guider agent releases `AGENT_GUIDER_RESET_MKGP_PROPERTY`
(3.0.0.54); the astrometry agent frees its device private data at shutdown (2.0.0.32).

## Out of scope
Other leaks of kinds the check does not fail on.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The leak check ends with 0 bytes definitely and indirectly lost. | leak check |
| AC-002 | The unit suite, the guider agent and CCD simulator suites behave as before; the astrometry suite fails only where it failed before. | `make test-unit`, recorded runs, baseline runs without the fixes |

## Failure behavior and edge cases
None.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Agent versions raised.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: fix the leaks.
