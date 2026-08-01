# indigo_drivers Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Complete scoped static baseline review recorded findings and coverage manifest; `externals`, `bin_externals`, and simulator directories were excluded. |

## Scope

Portable INDIGO drivers and agents under `indigo_drivers/`, including generated drivers, hardware drivers, and agent implementations.

For the 2026-08-01 scoped baseline pass, simulator directories and SDK/vendor subtrees named `externals` or `bin_externals` were intentionally excluded. The pass covered 490 C-family source/header files in 136 remaining top-level driver and agent directories.

## Coverage Manifest

| Group | Directories |
| --- | --- |
| Agents | `agent_alpaca`, `agent_astap`, `agent_astrometry`, `agent_auxiliary`, `agent_config`, `agent_guider`, `agent_imager`, `agent_mount`, `agent_scripting`, `agent_snoop`, `agent_solver`, `agent_test` |
| Auxiliary | `aux_arteskyflat`, `aux_astromechanics`, `aux_cloudwatcher`, `aux_dragonfly`, `aux_dsusb`, `aux_fbc`, `aux_flatmaster`, `aux_flipflat`, `aux_geoptikflat`, `aux_joystick`, `aux_mgbox`, `aux_ppb`, `aux_rts`, `aux_skyalert`, `aux_sqm`, `aux_svbpowerbox`, `aux_uch`, `aux_upb`, `aux_upb3`, `aux_usbdp`, `aux_wbplusv3`, `aux_wbprov3`, `aux_wcv4ec` |
| CCD/camera | `ccd_altair`, `ccd_apogee`, `ccd_asi`, `ccd_atik`, `ccd_baccam`, `ccd_bresser`, `ccd_dsi`, `ccd_fli`, `ccd_iidc`, `ccd_mallin`, `ccd_meade`, `ccd_mi`, `ccd_ogma`, `ccd_omegonpro`, `ccd_pentax`, `ccd_playerone`, `ccd_ptp`, `ccd_qhy`, `ccd_qhy2`, `ccd_qsi`, `ccd_rising`, `ccd_sbig`, `ccd_ssag`, `ccd_ssg`, `ccd_svb`, `ccd_svb2`, `ccd_sx`, `ccd_touptek`, `ccd_uvc` |
| Dome | `dome_baader`, `dome_beaver`, `dome_dragonfly`, `dome_nexdome`, `dome_nexdome3`, `dome_skyroof`, `dome_talon6ror` |
| Focuser | `focuser_asi`, `focuser_askar`, `focuser_astroasis`, `focuser_astromechanics`, `focuser_dmfc`, `focuser_dsd`, `focuser_efa`, `focuser_fc3`, `focuser_fcusb`, `focuser_fli`, `focuser_focusdreampro`, `focuser_ioptron`, `focuser_lacerta`, `focuser_lakeside`, `focuser_lunatico`, `focuser_mjkzz`, `focuser_moonlite`, `focuser_mypro2`, `focuser_nfocus`, `focuser_nstep`, `focuser_optec`, `focuser_optecfl`, `focuser_primaluce`, `focuser_prodigy`, `focuser_qhy`, `focuser_robofocus`, `focuser_steeldrive2`, `focuser_usbv3`, `focuser_wemacro` |
| Mount | `mount_asi`, `mount_ioptron`, `mount_lx200`, `mount_nexstar`, `mount_nexstaraux`, `mount_pmc8`, `mount_rainbow`, `mount_starbook`, `mount_synscan`, `mount_temma` |
| Other device classes | `ao_sx`, `gps_gpsd`, `gps_nmea`, `guider_asi`, `guider_cgusbst4`, `guider_gpusb`, `system_ascol` |
| Rotator | `rotator_asi`, `rotator_falcon`, `rotator_lunatico`, `rotator_optec`, `rotator_wa` |
| Wheel | `wheel_asi`, `wheel_astroasis`, `wheel_atik`, `wheel_fli`, `wheel_indigo`, `wheel_manual`, `wheel_mi`, `wheel_optec`, `wheel_playerone`, `wheel_qhy`, `wheel_quantum`, `wheel_sx`, `wheel_trutek`, `wheel_xagyl` |

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| DRV-001 | High | `agent_alpaca/indigo_alpaca_ccd.c:946` | `CCD_MODE` items are copied into `readoutmodes_labels` and `readoutmodes_names` by raw `property->count` index, but the arrays are only `ALPACA_MAX_ITEMS` long. The setter also accepts `value == ALPACA_MAX_ITEMS`, causing an out-of-bounds read. Clamp mode import to `ALPACA_MAX_ITEMS` and reject `value >= ALPACA_MAX_ITEMS`. | Open |
| DRV-002 | Medium | `agent_alpaca/indigo_alpaca_wheel.c:107` | Wheel slot-offset/name updates store `property->count` as `wheel.count` even though Alpaca wheel arrays are limited to `ALPACA_MAX_FILTERS`. The response paths then serialize `count` entries from fixed 32-element arrays, so a driver exposing more items can read beyond the arrays. Clamp exported count to `ALPACA_MAX_FILTERS`. | Open |
| DRV-003 | Medium | `agent_config/indigo_agent_config.c:686` | Agent configuration builds a semicolon-separated driver filter with repeated `strcat()` into one `INDIGO_VALUE_SIZE` text item. A server exposing enough selected drivers can overflow the filter string. Build with remaining-capacity checks or truncate safely. | Open |
| DRV-004 | Medium | `agent_imager/indigo_agent_imager.c:3485` | The imager agent mirrors wheel slot names by assigning `AGENT_WHEEL_FILTER_PROPERTY->count = property->count`, but the property was initialized with `FILTER_SLOT_COUNT` items. A wheel with more than 24 slots can write past the property items when copying labels. Clamp to `FILTER_SLOT_COUNT` or resize the property before copying. | Open |
| DRV-005 | Medium | `agent_alpaca/indigo_alpaca_switch.c:564` | Alpaca switch-name handling copies `AUX_OUTLET_NAMES` and `AUX_SENSOR_NAMES` with raw `property->count` into fixed `5 * ALPACA_MAX_SWITCHES` storage. The value paths clamp each bank to 8 items, but the name paths do not, so a property with too many names can write past the selected bank. Clamp each name bank to `ALPACA_MAX_SWITCHES`. | Open |
| DRV-006 | Medium | `agent_alpaca/indigo_agent_alpaca.c:670` | `INFO_DEVICE_NAME` text is copied with `strcpy()` into `device_name[INDIGO_NAME_SIZE]`, while INDIGO text values are `INDIGO_VALUE_SIZE`. A long device-name text item can overflow the Alpaca device-name cache. Use `INDIGO_COPY_NAME()` or another bounded copy. | Open |
| DRV-007 | Medium | `dome_nexdome3/indigo_dome_nexdome3.c:1203` | The optional NexDome custom command path formats `NEXDOME_COMMAND_ITEM->text.value` into `char command[NEXDOME_CMD_LEN]` with `sprintf()`. The text value can be much larger than the 100-byte command buffer, so a long custom command overflows the stack buffer. Use `snprintf()` and reject/truncate oversized commands. | Open |
| DRV-008 | Medium | `focuser_steeldrive2/indigo_focuser_steeldrive2.c:642` | SteelDrive2 formats the user-editable `X_NAME` text item into `char command[64]` with `sprintf("$BS SET NAME:%s", ...)`. `X_NAME_ITEM->text.value` is an INDIGO text value, so a long name can overflow the command buffer before it is sent. Bound the accepted name length or use `snprintf()` with state feedback on truncation. | Open |
| DRV-009 | Medium | `dome_nexdome3/indigo_dome_nexdome3.c:567` | NexDome3 parses incoming `XB->...` messages with `sscanf(message, "XB->%s", state)` into `char state[20]`. A malformed or unexpectedly long controller message can overflow the stack buffer before the value is copied to the INDIGO text item. Add a field width or parse with bounded copying. | Open |
| DRV-010 | Medium | `system_ascol/libascol/libascol.c:405` | `ascol_parse_devname()` parses `DEVICE_PORT_ITEM->text.value` with unbounded `%s` into the caller's `host` buffer; the system driver passes `char host[255]`, while the INDIGO text value can be larger. A long `tcp://...` or `ascol://...` value can overflow `host`. Use width-limited parsing or pass the destination size. | Open |
| DRV-011 | Medium | `agent_astrometry/indigo_agent_astrometry.c:205` | The Astrometry agent builds `buffer[8192]` with `vsnprintf()` and then appends `" 2>&1"` into another 8192-byte buffer with `sprintf()`. A command truncated to the full source buffer can overflow `command_buf`; the same file also parses `Field size` units into `char s[16]` with unbounded `%s`. Use a single bounded `snprintf()` and width-limited parsing. | Open |
| DRV-012 | Medium | `agent_astap/indigo_agent_astap.c:331` | The ASTAP agent has the same equal-sized `buffer` to `command_buf` append overflow as Astrometry. It also builds index parameters and index paths with repeated `sprintf()` into 512-byte buffers using `base_dir`, which can be near the buffer limit. Convert command, parameter, and path construction to checked `snprintf()` with remaining-capacity tracking. | Open |
| DRV-013 | Medium | `gps_gpsd/indigo_gps_gpsd.c:59` | `gpsd_open()` copies the editable device-port text into `host_name[INDIGO_NAME_SIZE]` and `port[15]` with `strcpy()`/`strncpy()` without checking host or port length. A long `gpsd://...` value can overflow the host or port buffer before `gps_open()`. Parse with bounded lengths and reject invalid endpoints. | Open |
| DRV-014 | Medium | `mount_synscan/indigo_mount_synscan_driver.c:1013` | `synscan_save_position()` writes the HOME-based `.indigo` path with `snprintf()` but then appends the park filename using `sprintf(buffer + path_end, ...)`. If HOME is long enough for `snprintf()` to truncate, `path_end` is the would-have-written length and can point past `buffer`. Compose the complete path with one checked `snprintf()`. | Open |
| DRV-015 | Medium | `ccd_ptp/indigo_ptp.c:1533` | PTP string switch values are decoded into `PTP_MAX_CHARS` 256-byte entries, but refreshed property names are copied into `char str[INDIGO_NAME_SIZE]` with `strcpy()`. A camera-provided string value longer than 127 bytes can overflow `str` before the item name is updated. Use bounded copying and define a deterministic truncation or rejection policy for item names. | Open |

## Review Focus

- Driver lifecycle: `INDIGO_DRIVER_INIT`, `INDIGO_DRIVER_SHUTDOWN`, and `INDIGO_DRIVER_INFO`.
- Attach/detach ordering and resource cleanup.
- Connection/disconnection behavior and property visibility.
- Property state transitions, especially `BUSY -> OK` and `BUSY -> ALERT`.
- Handler queue, timer, and async usage.
- Generated driver source synchronization with `.driver` inputs.
- Simulator coverage under `indigo_test/integration/`.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Initial review baseline only. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Focused baseline review of `agent_alpaca` fixed-size adapter arrays and selected driver lifecycle paths; recorded `DRV-001` and `DRV-002`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Scoped static pass over 490 non-simulator, non-SDK C-family/driver files under `indigo_drivers`; recorded `DRV-003` and `DRV-004`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Completed the requested all-driver scoped static pass for non-simulator, non-SDK `indigo_drivers`; recorded `DRV-005` and `DRV-006`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Follow-up pass over user-text command formatting and SDK-reported name handling; recorded `DRV-007` and `DRV-008`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Follow-up pass over timer, async, and mutex-heavy paths in the scoped driver set; no additional source-backed findings recorded. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Final remaining-driver pass over protocol parsing, response tokenization, mirrored property resizing, and unchecked stack-buffer copies; recorded `DRV-009` and `DRV-010`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Exhaustive scoped directory enumeration plus repeatable static scans over all 136 included top-level directories and 490 C-family files; recorded `DRV-011` through `DRV-015`. |
