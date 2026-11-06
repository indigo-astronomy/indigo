# Specification

Status: approved for implementation (2026-11-06). All decisions are recorded below.

## Problem and outcome
Add support for the ToupTek ATLAS SH20 strain-wave mount to INDIGO as a new dialect of the existing
`indigo_mount_lx200` driver. The dialect follows the serial protocol in [protocol.md](protocol.md) and the
conventions of the driver's other dialects. Users get what the ToupTek INDI driver offers through standard INDIGO
mount and guider properties, plus vendor-prefixed custom properties named after the existing `X_ZWO_*` set. The
dialect must avoid the INDI reference defects N1–N9 in protocol.md.

## In scope
- `indigo_drivers/mount_lx200/indigo_mount_lx200.driver`: a new `MOUNT_TYPE` item `TOUPTEK` ("ToupTek SH20"),
  dialect branches, init/update functions and three custom properties. Regenerated `.c`/`.h`. Driver version 82 → 83.
- `indigo_drivers/mount_lx200/mount_lx200_simulator/mount_lx200_simulator.c`: a new `--model touptek` profile.
- `indigo_test/integration/test_mount_lx200_simulator.c`: SH20 dialect cases.
- `indigo_drivers/mount_lx200/PROTOCOL.md`: a ToupTek column (16 dialects), using protocol.md as its source.
- `indigo_docs/PROPERTIES.md` and the `MIGRATION_STATUS.md` test counts for the `mount_lx200` row. The `mount_lx200` `README.md`
  supported-devices text changes only with owner approval.

## Out of scope
- BLE transport. Firmware update commands (`:Update…#`) are never sent.
- Custom/King tracking rates, PEC and alignment points.
- Changes to the other 15 dialects. Their full existing suite must keep passing.

## Decisions applied
| ID | Decision (owner, 2026-11-06) | Effect |
| --- | --- | --- |
| D1 | Dialect in `mount_lx200`, not a separate driver | The scope above. No new driver directory or registration |
| D2 | Expose find-home | Implemented as the **standard** `MOUNT_HOME.SEARCH` item (`MOUNT_HOME_SEARCH_ITEM`, already used by `mount_ioptron`) instead of a new custom property. The standard item meets the request without adding a custom name |
| D3 | Use the lx200 slew-rate mapping | GUIDE `:R1#` (0.5×), CENTERING `:R4#` (4×), FIND `:R7#` (60×), MAX `:R9#` (1440×), as for ZWO |
| D4 | Follow the lx200 naming convention | Shared `X_MOUNT_TYPE` item `TOUPTEK` and shared `X_MOUNT_MODE`. Vendor-prefixed `X_TOUPTEK_BUZZER`, `X_TOUPTEK_MERIDIAN`, `X_TOUPTEK_MERIDIAN_LIMIT`, mirroring `X_ZWO_BUZZER`, `X_ZWO_MERIDIAN`, `X_ZWO_MERIDIAN_LIMIT` |
| D5 | Use the park state the mount reports (owner, 2026-11-06, after S5) | `:GPS#` 0/1/2 at connect and while polling. Nothing is persisted |
| D6 | No hardware available | Simulator-validated only. Hardware-only items (literal `:GVP#`/`:GG#` replies, Q5–Q7, Q9) stay documented limitations. Hardware validation is not claimed |

## Property mapping (ToupTek dialect)
| Property | Behavior | Protocol |
| --- | --- | --- |
| `X_MOUNT_TYPE` | New item `TOUPTEK`. `DETECT` selects it when `:GVP#` answers `SH20` (S5/S6). A manual choice skips the probes | `:GVP#` |
| `MOUNT_INFO` | VENDOR `ToupTek`, MODEL from `:GVP#`, FIRMWARE from `:GV#` (first 5 characters, as TMount uses them) | `:GVP#`, `:GV#` |
| `DEVICE_PORT`, `DEVICE_BAUDRATE` | Serial 115200 8N1 (set for the dialect, like NYX). Network `lx200://192.168.4.1:8080` (default port 8080 for the dialect) | — |
| `MOUNT_GEOGRAPHIC_COORDINATES` | latitude/longitude, east-positive ±180 | `:St`, `:Sg`; `:Gt#`/`:Gg#` |
| `MOUNT_UTC_TIME`, `MOUNT_SET_HOST_TIME` | visible | `:SG` (standard sign, S5) → `:SL` → `:SC` |
| `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_ON_COORDINATES_SET` | GOTO `:Sr` `:Sd` `:MS#` (`1` = OK, `0`/`e<n>#` = refused); SYNC `:Sr` `:Sd` `:CM#` | |
| `MOUNT_SLEW_RATE`, `MOUNT_MOTION_*`, `MOUNT_ABORT_MOTION` | D3 mapping; `:Mn/s/e/w`, `:Qn/s/e/w`, `:Q#` | |
| `MOUNT_TRACKING` | `:Te#`/`:Td#` → `1`; polled with `:GTS#` | |
| `MOUNT_TRACK_RATE` | SIDEREAL/SOLAR/LUNAR; readback `:GT#` (ASCII, plus binary 0..2 tolerated, N1) | `:TQ#` `:TS#` `:TL#` |
| `MOUNT_GUIDE_RATE` | one value for both axes, 10–90 %. `GUIDER_RATE` stays hidden as for ZWO (lx200 convention) | `:Sgr0.nn#`; `:Ggr#` |
| `MOUNT_PARK` | park slew; parked/parking from `:GPS#` 2/1/0 at connect and while polling | `:MP1#` / `:MP0#`; `:GPS#` |
| `MOUNT_PARK_SET` | CURRENT | `:SPc#` → `1` |
| `MOUNT_HOME` | HOME (go home, ends at `H` in `:GU#`) and SEARCH (D2) | `:MH#`; `:MFH#` → `1` |
| `MOUNT_SIDE_OF_PIER` | read-only, EQ mode only | `:Gm#` `0`=E, `1`=W (not the ZWO letters) |
| `X_MOUNT_MODE` | EQUATORIAL/ALTAZ, read-only, from `:GU#` `G`/`Z`. This is the lx200 convention (D4): the property is read-only for every dialect, and a change needs a mount restart | `:GU#` |
| `X_TOUPTEK_BUZZER` | OFF/LOW/HIGH | `:SBzn#`; `:GBz#` |
| `X_TOUPTEK_MERIDIAN`, `X_TOUPTEK_MERIDIAN_LIMIT` | auto flip, track past the meridian, limit −15…15°. EQ mode only | `:STaFTsLL#` → `1`; `:GTa#` (same format as ZWO) |
| Guider `GUIDER_GUIDE_*` | pulse guiding | `:Mg[nsew]DDDD#` |

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `DETECT` identifies the SH20 from `:GVP#` = `SH20`, and an explicit `TOUPTEK` choice works too. Connect reads `:GV#`, mode, tracking, rate, guide rate, buzzer, meridian (EQ only), park (`:GPS#`), site and time, and publishes the device state. A bootloader reply to `:GEM#` (`F3`/`Updating`) refuses the connection with a message. A port that does not answer ends `CONNECTION` ALERT with the port closed. `DETECT` behavior for the other dialects is unchanged. | simulator `--model touptek`, started in a non-default state (also parked); trace |
| AC-002 | GOTO sends `:Sr`, `:Sd`, `:MS#` and stays BUSY until `:GU#` reports stopped and the coordinates arrive. `0` or `e<n>#` at any step → ALERT, no later command, and a message. A slew that never completes → ALERT after the driver's existing timeout. | simulator fault injection |
| AC-003 | SYNC sends `:CM#` without a slew. `e<n>#` → ALERT. | simulator |
| AC-004 | The slew-rate mapping is R1/R4/R7/R9. Manual motion and abort send the documented commands and leave no BUSY. | simulator |
| AC-005 | Tracking on/off and the three rates are written and read back. External changes are published once. | simulator |
| AC-006 | Guide rate round-trips through `:Sgr`/`:Ggr#` and is applied to both axes. Pulses send `:Mg?DDDD#` and meet the guider timing rules. | simulator; timing measurement |
| AC-007 | Park/unpark/park-set as mapped. Park completion comes from `:GPS#` = `2#`, not from the `N` flag (N6). Parked guards refuse motion, GOTO and pulses. | simulator |
| AC-008 | HOME completes on `H`. SEARCH sends `:MFH#` and reports the `0`/`1` result. Abort cancels homing. | simulator |
| AC-009 | Site/time order and signs round-trip, including the 0/360 boundary. `:SG` uses the standard sign. | simulator |
| AC-010 | `X_MOUNT_MODE` (read-only), `X_TOUPTEK_BUZZER` and `X_TOUPTEK_MERIDIAN(_LIMIT)` are read at connect, and the latter two are written with readback. A one-item meridian change resends the others. In ALTAZ mode the meridian properties and the side of pier are not defined. | simulator `--model touptek`, with and without `--touptek-altaz` |
| AC-011 | Side of pier parses `0`/`1`, is read-only, and is defined in EQ mode only. | simulator |
| AC-012 | Malformed or partial replies mark only the affected property ALERT. A lost coordinate poll keeps the last values with ALERT. `e<n>#` is decoded wherever a boolean reply is expected (N3/N4/N5). | simulator fault injection |
| AC-013 | No regression: the complete existing `test_mount_lx200_simulator` suite and the hardware-free cases for the other 15 dialects pass unchanged. | full suite |
| AC-014 | The driver version is 83. Regeneration is reproducible. `PROTOCOL.md`, `PROPERTIES.md` and `MIGRATION_STATUS.md` (test counts only) are updated. | review |

## Failure behavior and edge cases
- The SH20 boolean replies have no `#`. A leading `e` means an error reply terminated by `#`, decoded with the
  existing ZWO error table where the codes match (to be confirmed, Q8).
- Low-precision `:GR#` replies are rejected for the dialect. `:U#` is never sent (N8).
- A mount-mode change takes effect only after a power cycle. The driver keeps the current geometry and warns.

## Permissions, data, and security requirements
No new transport. All replies are bounded, NUL-terminated and validated before use, including the N1 index case.

## Compatibility and nonfunctional requirements
Linux/macOS/Windows through the existing portable I/O. No change to the other dialects' wire behavior, which the reference traces
in the existing suite confirm.

## Assumptions and open questions
The `:GU#` flags as in ZWO; `:Gm#` `1` = west; `:GVP#` = `SH20` (inferred from S5/S6). Remaining hardware questions: Q5–Q7 and Q9, plus the literal `:GVP#` and `:GG#` replies. None of them blocks a simulator-validated implementation.

## Open decisions
None. D7 decided: non-interactive simulator run.

## Owner decisions and requirement changes
- 2026-11-05, Rumen G. Bogdanovski: driver to be named `indigo_mount_touptek` (superseded below).
- 2026-11-06, Rumen G. Bogdanovski: D1 changed to a dialect in `mount_lx200` following its conventions. D2 expose find-home,
  D3 lx200 slew-rate mapping, D4 lx200 naming convention, D5 always start unparked.
- 2026-11-06, Rumen G. Bogdanovski: D5 use the reported `:GPS#` state; D6 no hardware; asked to verify the simulator against the INDI driver (plan step 1b).
- 2026-11-06, Rumen G. Bogdanovski: INDI cross-check dropped (D8 withdrawn). D7: non-interactive simulator run; implement autonomously.
- 2026-11-06, planner: TMount (S5) shows `:GVP#`, `:GPS#`, 115200 baud and TCP port 8080. Autodetection and real park state are added, and D5 is satisfied by reading the state instead of assuming it.
- 2026-11-06, developer: `X_MOUNT_MODE` is read-only and `GUIDER_RATE` is hidden, following the lx200 convention chosen in D4 (AC-010 and AC-006 adjusted). The error codes are passed on as `eN`, because TMount's texts for them are localized resources that were not decoded.
- 2026-11-06, planner: D2 implemented through the standard `MOUNT_HOME.SEARCH` item instead of `X_MOUNT_FIND_HOME`. The owner can
  overrule this.
