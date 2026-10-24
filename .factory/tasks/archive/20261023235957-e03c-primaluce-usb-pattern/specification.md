# Specification
Status: final

## Problem and outcome
The PrimaLuce focuser registers one serial match pattern: product "CP2102N", VID `0x10c4`, PID `0xea60`.

## In scope
The `.driver` serial pattern and its generated code; the test; REFACTOR.md, PROTOCOL.md, MIGRATION_STATUS.md.

## Out of scope
The framework's Windows port enumeration.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A CP2102N port with `10c4:ea60` matches; a CP2102N name with another VID/PID, and another bridge with `10c4:ea60`, do not. | `usb_match_pattern` (fails without the change) |
| AC-002 | The rest of the suite is unchanged. | recorded run |

## Failure behavior and edge cases
Without a matching port the user selects the port by hand, as before.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
- Driver version 39 to 40.
- On Windows the pattern does not match, because the port list carries no VID/PID.

## Assumptions and open questions
All PrimaLuce controllers are assumed to keep the default CP2102N identity; two models were seen.

## Owner decisions and requirement changes
2026-10-24: the requester asked for the VID/PID preference.
