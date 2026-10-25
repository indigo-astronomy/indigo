# Specification
Status: final

## Problem and outcome
`X_WIFI_STATUS` is IDLE while the device cannot be reached over WiFi, OK otherwise.

## In scope
`primalucelib_wifi_status.reachable`; `indigo_aux_giotto` 7, `indigo_aux_alto` 7, `indigo_focuser_primaluce` 45.

## Out of scope
The items and their values.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | OK in access point mode with the access point on and in station mode once joined. | `property_contract`, station-mode cases (three suites) |
| AC-002 | IDLE with WiFi off and in station mode with `LANCONN` `disconnected`; a change of state alone is published. | GIOTTO `wifi_modes`, unit case; GIOTTO hardware trace |
| AC-003 | The rest of the suites is unchanged. | recorded runs |

## Failure behavior and edge cases
A missing `LANCONN` in station mode counts as not reachable.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Versions as above.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: IDLE when disconnected or WiFi off.
