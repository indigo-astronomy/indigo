# Specification
Status: final

## Problem and outcome
`indigo_aux_alto` opens, closes and positions an ALTO cover over USB (or HTTP), with abort, presets, LEDs and the INDI
calibration sequence.

## In scope
See brief.md.

## Out of scope
Hold current, torque values, the panel's WiFi configuration.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | One device "PrimaluceLab ALTO" with `INDIGO_INTERFACE_AUX_DUSTCAP`; a non-ALTO is refused and the port released. | `metadata`, `unsupported_device`; HW contract |
| AC-002 | `AUX_COVER` opens to 100 and closes to 0, `X_COVER_POSITION` reaches any 0–100 %, BUSY with progress, OK at the target; the ends are shown only at 0 and 100. | `close_and_open`, `partial_position`, `connect_closed`; HW |
| AC-003 | Abort stops the cover (decelerated) and ends ALERT at the position reached; an abort while idle sends nothing; a refused stop is reported. | `abort_*`; HW `alto_aborts_a_move` |
| AC-004 | Refused moves, stalls and lost readbacks end ALERT, a moving cover is stopped, and the next move works. | `move_refused`, `stalled_move`, `motion_readback_failures` |
| AC-005 | Motion started elsewhere is followed; overlapping requests are refused; a disconnect stops a moving cover. | `external_motion_observed`, `overlap_refused`, `disconnect_during_motion` |
| AC-006 | Presets: named rule with reason, steps conversion, recall moves the cover, readback after reconnect. | `presets`; HW |
| AC-007 | Calibration: START, drive, STORE closed, drive, STORE open, CANCEL; moves refused meanwhile; motion controls refused outside. | `calibration` (simulator model only) |
| AC-008 | LEDs, state polling, poll failure and recovery, lifecycle, transport loss. | remaining cases; HW |
| AC-009 | The GIOTTO and focuser drivers, which link the same library, are unchanged. | GIOTTO suite 25/25, focuser suite |

## Failure behavior and edge cases
Only integers 0–100 are sent as `POSITION` (the controller accepts anything); an uncalibrated cover refuses moves.

## Permissions, data, and security requirements
Controller replies are untrusted; refusal texts are published through `"%s"` only.

## Compatibility and nonfunctional requirements
New driver version 1 (3.0.0.1); Linux verified; Windows and Xcode projects registered, not built here.

## Assumptions and open questions
The calibration sequence is INDI's and unverified on hardware.

## Owner decisions and requirement changes
2026-10-24: full access, INDI calibration with later recalibration, README approved.
