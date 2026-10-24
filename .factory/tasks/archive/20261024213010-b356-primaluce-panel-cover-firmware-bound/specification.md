# Specification
Status: final

## Problem and outcome
Firmware versions of any length leave `INFO` within its item: the text is cut off at the item size.

## In scope
`indigo_aux_giotto`, `indigo_aux_alto` (version 5), their simulators and suites.

## Out of scope
The focuser (fixed upstream).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | With 300 character application and web versions, `INFO.DEVICE_FW_REVISION` holds the first 511 characters of "<app> / <web>" and the serial number is intact. | `long_firmware` in both suites (aborts on 3.0.0.4) |
| AC-002 | The rest of both suites is unchanged. | recorded runs |

## Failure behavior and edge cases
A long application version alone is truncated by the copy; the append then adds nothing.

## Permissions, data, and security requirements
Device replies are untrusted; no write beyond the item.

## Compatibility and nonfunctional requirements
Version 4 to 5 for both drivers.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: fix in a separate task after the merge.
