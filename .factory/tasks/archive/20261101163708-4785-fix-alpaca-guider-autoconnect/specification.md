# Specification

Status: approved by owner instruction of 2026-11-01

## Problem and outcome
`indigo_alpaca_connect_paired_guider()` connects a mount's guider whenever the mount is connected and the two are paired, without regard to who connected the mount. A guider connected this way is refused by the Guider Agent. After this change the Alpaca Agent connects the guider only when an Alpaca client asked for the mount connection.

## In scope
Tracking of the Alpaca client's connection request per mount and the guard in the paired guider connection. Regression test and agent version.

## Out of scope
Disconnection behavior of `alpaca_set_connected()`, which still disconnects the guider and mount when a client disconnects. A1, the `auto://` message, the filter reselection race.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A mount connected by a non-Alpaca client sends no guider connection request, whether the guider is already paired or pairs afterwards. | `paired_guider_follows_alpaca_client_only` |
| AC-002 | `PUT connected=true` on the Telescope still connects a paired guider. | same test |
| AC-003 | A guider that pairs after `PUT connected=true` on its mount is still connected (the `system_alpaca` case). | same test |
| AC-004 | The request ends with the client's `connected=false`, with the mount disconnected by anybody else, and with a failed connection; a later non-Alpaca connection then sends no guider request. | same test |
| AC-005 | With the agent loaded, selecting a mount in the Mount Agent and then its guider in the Guider Agent succeeds. | server scenario with the SynScan serial simulator, the setup that reproduced the defect |
| AC-006 | Agent version is higher than 3.0.0.17. | source inspection |

## Failure behavior and edge cases
A connection in progress (BUSY) keeps the request, so a guider pairing during it still follows. A pending timer that fires after the request ended sends the connection already scheduled; the window is the zero-delay timer only.

## Permissions, data, and security requirements
No new input, property, credential or network surface. The new flag is set only from an existing Alpaca request path.

## Compatibility and nonfunctional requirements
An Alpaca client that uses a Telescope already connected by another client, without sending `PUT connected=true`, no longer gets the guider connected for it, so `CanPulseGuide` stays false. ASCOM clients are expected to send the request. Public INDIGO properties are unchanged. No platform-specific code.

## Assumptions and open questions
Assumption: Alpaca clients send `PUT connected=true` before use (low risk, ASCOM usage). No blocking questions.

## Owner decisions and requirement changes
2026-11-01, Peter Polakovic: approved connecting the guider only for an Alpaca client.
