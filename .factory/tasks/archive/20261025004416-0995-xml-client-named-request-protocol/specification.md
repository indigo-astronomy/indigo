# Specification
Status: final

## Problem and outcome
A client that starts with a request for a device or a property ends on the INDIGO protocol with an INDIGO server, so
states and names are read as published.

## In scope
`indigo_xml_client_parser_enumerate_properties()`.

## Out of scope
`parse_state()` (its legacy `Idle` → OK reading stays for INDI servers); the server.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | While the server has not switched, a request for a device or property carries `version='1.7' switch='<current>'` before `device` and `name`, with the name in INDIGO form; after the switch it does not. | unit case (fails on the unchanged library) |
| AC-002 | A server that switches: a following `Idle` reads as IDLE. A server that never switches (INDI): `Idle` still reads as OK. | unit case |
| AC-003 | `indigo_prop_tool get_state` shows IDLE for a property the driver publishes IDLE. | GIOTTO hardware session |
| AC-004 | The rest of the unit suite and the switch-related integration test pass. | `make test-unit`, `test_detach_abort` |

## Failure behavior and edge cases
An INDI server ignores `switch`, as it does on the request for everything.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
INDIGO servers that already handle `switch` on any `getProperties` (they do).

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: fix the tool; the defect is in the shared XML client, fixed there.
