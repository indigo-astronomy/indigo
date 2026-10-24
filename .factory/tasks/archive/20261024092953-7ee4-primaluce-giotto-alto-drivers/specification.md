# Specification
Status: final

## Problem and outcome
`indigo_aux_giotto` controls a GIOTTO over USB (or HTTP) as an INDIGO light box with presets and settings.

## In scope
See brief.md.

## Out of scope
The ALTO-1; the panel's WiFi configuration; `LIGHT_FADING_TIME` (undocumented, unit unknown).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | One device "PrimaluceLab GIOTTO" with `INDIGO_INTERFACE_AUX_LIGHTBOX`; a non-GIOTTO or a panel without a brightness range is refused and the port released. | `metadata`, `unsupported_device`, `no_brightness_range`; HW contract |
| AC-002 | `AUX_LIGHT_SWITCH` and `AUX_LIGHT_INTENSITY` (device level 0..`MAX_BRIGHTNESS`) control and show the panel; the connect values are read, not written. | `light_switch`, `intensity`, `connect_reads_the_panel`; HW |
| AC-003 | Presets: brightness, names, recall (brightness only); the unnamed-preset refusal names its reason. | `presets`, `presets_refused`; HW `giotto_uses_presets` |
| AC-004 | Fading, light at power on and LEDs are set and read back after reconnect. | `settings`; HW |
| AC-005 | Refusals end ALERT with the panel's reason and the device's value; unanswered requests end ALERT. | `*_refused`, `intensity_unanswered`, `settings_refused` |
| AC-006 | Polling publishes external changes; failed polls are ALERT and recover. | `external_change_polled`, `poll_failure`, `environment_polled` |
| AC-007 | Disconnect switches the light off and keeps the brightness. | `disconnect_switches_the_light_off`; HW `giotto_reconnects` |
| AC-008 | Lifecycle: INIT/SHUTDOWN, shutdown refused while connected, reconnect, transport loss. | lifecycle cases; HW |
| AC-009 | The focuser driver, which links the same library, is unchanged. | focuser suite (not recorded) |

## Failure behavior and edge cases
See AC-005/006; a preset name with `"` or `\` is not sent and is reported.

## Permissions, data, and security requirements
Controller replies are untrusted: refusal texts are published through `"%s"` only.

## Compatibility and nonfunctional requirements
New driver version 1 (3.0.0.1). Linux verified; Windows and Xcode projects registered but not built here.

## Assumptions and open questions
WiFi untested with the GIOTTO.

## Owner decisions and requirement changes
2026-10-24: two drivers; GIOTTO first; light off at disconnect; README approved; audit deferred.
