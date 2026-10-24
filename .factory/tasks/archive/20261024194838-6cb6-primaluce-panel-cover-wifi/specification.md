# Specification
Status: final

## Problem and outcome
GIOTTO and ALTO users can select the WiFi mode and set both credentials from INDIGO, with the focuser's semantics.

## In scope
`primalucelib` (`primalucelib_select_lan_config`, `primalucelib_select_wifi`, credential checks, `LANCFG`
acknowledgement); `indigo_aux_giotto` and `indigo_aux_alto` (`X_WIFI`, `X_WIFI_AP`, `X_WIFI_STA`);
`indigo_focuser_primaluce` on the shared selection; both simulators; the three suites and the unit test.

## Out of scope
Escaping `"` and `\` in credentials; README changes without approval.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | GIOTTO and ALTO define the three properties, filled from the device at connect. | `property_contract` of both suites; hardware sessions |
| AC-002 | OFF and AP switch the access point in `ap` mode without a restart; a change between STA and the access point writes `LANCFG` and restarts once; the device is served afterwards. | `wifi_modes` (GIOTTO, ALTO), `wifi_station_mode` (focuser), unit cases; hardware sessions |
| AC-003 | Credentials are written and read back; values WiFi cannot use are refused with a reason and not sent. | `wifi_credentials`, `controller_settings`, unit case; hardware sessions |
| AC-004 | A refused mode keeps the mode and restarts nothing; the ALTO refuses a restart while moving or calibrating. | `wifi_mode_refused`, `wifi_modes` (ALTO), `wifi_restart_waits_for_the_cover` |
| AC-005 | The rest of the three suites is unchanged. | recorded runs |

## Failure behavior and edge cases
Over HTTP a restart drops the link: AP succeeds, OFF fails with a reason, the driver says the device leaves its network.

## Permissions, data, and security requirements
Credentials are user input sent unescaped to the device, so the library refuses `"` and `\`. Passwords are shown in
clear text in the properties, as the focuser already did.

## Compatibility and nonfunctional requirements
GIOTTO 3 to 4, ALTO 3 to 4, focuser 41 to 42; property names as in the focuser.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-24: proceed with the proposal, hardware access granted (server stopped; GIOTTO plugged in later).
