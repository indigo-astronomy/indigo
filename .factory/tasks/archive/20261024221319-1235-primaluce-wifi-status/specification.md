# Specification
Status: final

## Problem and outcome
The three drivers show where the device can be reached over WiFi.

## In scope
`primalucelib_reply_wifi_status()`, `primalucelib_get_wifi_status()`, decoded reply strings; `X_WIFI_STATUS`
(IP_ADDRESS, PORT, CONNECTION) and credential read-back on `indigo_aux_giotto` 6, `indigo_aux_alto` 6,
`indigo_focuser_primaluce` 44.

## Out of scope
Network discovery.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | At connect, `X_WIFI_STATUS` is read-only and shows the access point address, port 80 and `access point` in access point mode. | `property_contract` (three suites) |
| AC-002 | After a switch to station mode it shows the station address, port 80 and the network state; with the access point off no address and `off`. | `wifi_modes` (GIOTTO, ALTO), `wifi_station_mode` (focuser) |
| AC-003 | A station address reported outside station mode is not shown; reply strings are decoded; a refused reply fails. | unit case |
| AC-005 | After a credential change the properties show what the device stored: a kept SSID with a message, the stored values after a refusal, escaped characters as entered. | `wifi_credentials` (GIOTTO), `controller_settings`, `wifi_escaped_credentials` |
| AC-004 | The rest of the suites is unchanged. | recorded runs |

## Failure behavior and edge cases
A failed status request leaves the shown values; the next state poll tries again.

## Permissions, data, and security requirements
Read-only device data; no credentials are shown in the new property.

## Compatibility and nonfunctional requirements
One more request with the device state every 10 s.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: implement the proposed read-only property on all three drivers.
2026-10-25: one address field (IP_ADDRESS) for both modes, a PORT, CONNECTION showing the network state or the access
point; show the credentials the device stored (it keeps the access point SSID in access point mode).
