# Specification

Status: approved by owner report 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
Server started with `-b-` on a random port; the control panel itself answered (HTTP 200 on localhost, 127.0.0.1 and the LAN address).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The sandbox server is announced by Bonjour as "Scheduler Sandbox" by default; `--name` and `--no-bonjour` change it | `indigo_prop_tool discover` |
| AC-002 | The start-up lines print the port and the control panel URL for this and for other computers | run |
| AC-003 | The guide describes the port, the announcement and the options | review |

## Failure behavior and edge cases
No network: the host name is printed instead of an address.

## Permissions, data, and security requirements
The announcement makes a local test server visible on the LAN, like any indigo_server; `--no-bonjour` turns it off.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
