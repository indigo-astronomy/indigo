# indigo_drivers Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Complete scoped static baseline review recorded findings and coverage manifest; `externals`, `bin_externals`, and simulator directories were excluded. |

## Scope

Portable INDIGO drivers and agents under `indigo_drivers/`, including generated drivers, hardware drivers, and agent implementations.

For the 2026-08-01 scoped baseline pass, simulator directories and SDK/vendor subtrees named `externals` or `bin_externals` were intentionally excluded. The pass covered 490 C-family source/header files in 136 remaining top-level driver and agent directories.

## Focused Review Notes

2026-08-31 hot-plug SDK serialization pass: reviewed active `indigo_drivers/` hot-plug implementations using `libusb_hotplug_register_callback()` or vendor hot-plug callbacks, excluding `externals`, `bin_externals`, and disabled `#ifdef HOTPLUG` QHY code. This was a focused static review for races where a hot-plug enumeration/open/close path serializes access with a driver-global mutex but the normal connect/open/close path calls the same vendor SDK without that mutex. This pass did not advance `Last reviewed commit`.

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
| DRV-001 | High | `agent_alpaca/indigo_alpaca_ccd.c:946` | `CCD_MODE` items are copied into `readoutmodes_labels` and `readoutmodes_names` by raw `property->count` index, but the arrays are only `ALPACA_MAX_ITEMS` long. The setter also accepts `value == ALPACA_MAX_ITEMS`, causing an out-of-bounds read. Clamp mode import to `ALPACA_MAX_ITEMS` and reject `value >= ALPACA_MAX_ITEMS`. | Closed (fixed) |
| DRV-002 | Medium | `agent_alpaca/indigo_alpaca_wheel.c:107` | Wheel slot-offset/name updates store `property->count` as `wheel.count` even though Alpaca wheel arrays are limited to `ALPACA_MAX_FILTERS`. The response paths then serialize `count` entries from fixed 32-element arrays, so a driver exposing more items can read beyond the arrays. Clamp exported count to `ALPACA_MAX_FILTERS`. | Closed (fixed) |
| DRV-003 | Medium | `agent_config/indigo_agent_config.c:686` | Agent configuration builds a semicolon-separated driver filter with repeated `strcat()` into one `INDIGO_VALUE_SIZE` text item. A server exposing enough selected drivers can overflow the filter string. Build with remaining-capacity checks or truncate safely. | Closed (fixed) |
| DRV-004 | Medium | `agent_imager/indigo_agent_imager.c:3485` | The imager agent mirrors wheel slot names by assigning `AGENT_WHEEL_FILTER_PROPERTY->count = property->count`, but the property was initialized with `FILTER_SLOT_COUNT` items. A wheel with more than 24 slots can write past the property items when copying labels. Clamp to `FILTER_SLOT_COUNT` or resize the property before copying. | Closed (fixed) |
| DRV-005 | Medium | `agent_alpaca/indigo_alpaca_switch.c:564` | Alpaca switch-name handling copies `AUX_OUTLET_NAMES` and `AUX_SENSOR_NAMES` with raw `property->count` into fixed `5 * ALPACA_MAX_SWITCHES` storage. The value paths clamp each bank to 8 items, but the name paths do not, so a property with too many names can write past the selected bank. Clamp each name bank to `ALPACA_MAX_SWITCHES`. | Closed (fixed) |
| DRV-006 | Medium | `agent_alpaca/indigo_agent_alpaca.c:670` | `INFO_DEVICE_NAME` text is copied with `strcpy()` into `device_name[INDIGO_NAME_SIZE]`, while INDIGO text values are `INDIGO_VALUE_SIZE`. A long device-name text item can overflow the Alpaca device-name cache. Use `INDIGO_COPY_NAME()` or another bounded copy. | Closed (fixed) |
| DRV-007 | Medium | `dome_nexdome3/indigo_dome_nexdome3.c:1203` | The optional NexDome custom command path formats `NEXDOME_COMMAND_ITEM->text.value` into `char command[NEXDOME_CMD_LEN]` with `sprintf()`. The text value can be much larger than the 100-byte command buffer, so a long custom command overflows the stack buffer. Use `snprintf()` and reject/truncate oversized commands. | Closed (fixed) |
| DRV-008 | Medium | `focuser_steeldrive2/indigo_focuser_steeldrive2.c:642` | SteelDrive2 formats the user-editable `X_NAME` text item into `char command[64]` with `sprintf("$BS SET NAME:%s", ...)`. `X_NAME_ITEM->text.value` is an INDIGO text value, so a long name can overflow the command buffer before it is sent. Bound the accepted name length or use `snprintf()` with state feedback on truncation. | Closed (fixed) |
| DRV-009 | Medium | `dome_nexdome3/indigo_dome_nexdome3.c:567` | NexDome3 parses incoming `XB->...` messages with `sscanf(message, "XB->%s", state)` into `char state[20]`. A malformed or unexpectedly long controller message can overflow the stack buffer before the value is copied to the INDIGO text item. Add a field width or parse with bounded copying. | Closed (fixed) |
| DRV-010 | Medium | `system_ascol/libascol/libascol.c:405` | `ascol_parse_devname()` parses `DEVICE_PORT_ITEM->text.value` with unbounded `%s` into the caller's `host` buffer; the system driver passes `char host[255]`, while the INDIGO text value can be larger. A long `tcp://...` or `ascol://...` value can overflow `host`. Use width-limited parsing or pass the destination size. | Closed (fixed) |
| DRV-011 | Medium | `agent_astrometry/indigo_agent_astrometry.c:205` | The Astrometry agent builds `buffer[8192]` with `vsnprintf()` and then appends `" 2>&1"` into another 8192-byte buffer with `sprintf()`. A command truncated to the full source buffer can overflow `command_buf`; the same file also parses `Field size` units into `char s[16]` with unbounded `%s`. Use a single bounded `snprintf()` and width-limited parsing. | Closed (fixed) |
| DRV-012 | Medium | `agent_astap/indigo_agent_astap.c:331` | The ASTAP agent has the same equal-sized `buffer` to `command_buf` append overflow as Astrometry. It also builds index parameters and index paths with repeated `sprintf()` into 512-byte buffers using `base_dir`, which can be near the buffer limit. Convert command, parameter, and path construction to checked `snprintf()` with remaining-capacity tracking. | Closed (fixed) |
| DRV-013 | Medium | `gps_gpsd/indigo_gps_gpsd.c:59` | `gpsd_open()` copies the editable device-port text into `host_name[INDIGO_NAME_SIZE]` and `port[15]` with `strcpy()`/`strncpy()` without checking host or port length. A long `gpsd://...` value can overflow the host or port buffer before `gps_open()`. Parse with bounded lengths and reject invalid endpoints. | Closed (fixed) |
| DRV-014 | Medium | `mount_synscan/indigo_mount_synscan_driver.c:1013` | `synscan_save_position()` writes the HOME-based `.indigo` path with `snprintf()` but then appends the park filename using `sprintf(buffer + path_end, ...)`. If HOME is long enough for `snprintf()` to truncate, `path_end` is the would-have-written length and can point past `buffer`. Compose the complete path with one checked `snprintf()`. | Closed (fixed) |
| DRV-015 | Medium | `ccd_ptp/indigo_ptp.c:1533` | PTP string switch values are decoded into `PTP_MAX_CHARS` 256-byte entries, but refreshed property names are copied into `char str[INDIGO_NAME_SIZE]` with `strcpy()`. A camera-provided string value longer than 127 bytes can overflow `str` before the item name is updated. Use bounded copying and define a deterministic truncation or rejection policy for item names. | Closed (fixed) |
| DRV-016 | High | `agent_mount/indigo_agent_mount.c:2172` | `AGENT_MOUNT_ENABLE_JOYSTICK_CONTROL` was ignored by the `agent_update_property()` forwarding path. `JOYSTICK_MOUNT_*` updates were forwarded to the selected mount before the gated joystick handling in `snoop_changes()` could run, so disabling joystick control in `AGENT_PROCESS_FEATURES` did not prevent joystick motion, park, tracking, home, or abort commands. | Closed (fixed) |
| DRV-017 | High | `agent_mount/indigo_agent_mount.c:1840` | The refactor removed the old disabled-by-default `AGENT_DOME_SLAVING` and `AGENT_FIELD_DEROTATION` properties, then initialized the replacement `AGENT_PROCESS_FEATURES` items for dome slaving, derotation, and joystick control to `true`. Existing configurations saved under the old property names were no longer loaded into these new items, so upgrading could silently enable dome, rotator, and joystick-driven hardware behavior that was previously disabled. | Closed (fixed) |
| DRV-018 | High | `agent_mount/indigo_agent_mount.c:521` | Mount park/unpark with dome slaving now sends `DOME_PARK` immediately after `MOUNT_PARK`, instead of waiting for the mount park state to complete successfully. If the mount park later fails or the process is aborted, dome/roof park motion has already been started and `abort_process()` only sends `MOUNT_ABORT_MOTION`, creating a hardware-safety regression. Preserve the previous sequencing or abort/guard dome motion explicitly. | Closed (fixed) |
| DRV-019 | Medium | `ccd_ptp/indigo_ptp_olympus.c:696` | Olympus initialization logs a failed `CameraControlMode` switch and calls raw-USB recovery, but ignores missing confirmation and recovery failure before scheduling event polling and returning success. A disconnected, wedged, or wildcard-matched unsupported Olympus body can be reported connected even though remote capture and live view require PC-control mode. | Closed (fixed) |
| DRV-020 | Medium | `agent_mount/indigo_agent_mount.c:1189` | Mount and rotator deselection can leave `AGENT_MOUNT_STATE_DOME_SLAVING` / `AGENT_MOUNT_STATE_FIELD_DEROTATION` reporting the previous state after the required device is gone. Clear the slaving lights when the mount is deselected, and clear field derotation when the rotator is deselected. | Closed (fixed) |
| DRV-021 | Medium | `agent_mount/indigo_agent_mount.c:1043` | The autonomous slaving path treats `DOME_HORIZONTAL_COORDINATES` / `ROTATOR_POSITION` `ALERT` as eligible for another command and then unconditionally reports the slaving light as `OK`, masking dome or rotator failures despite the state-light contract saying `ALERT` on error. Propagate or preserve alert state until the dependent device reports recovery. | Closed (fixed) |
| DRV-022 | High | `wheel_asi/indigo_wheel_asi.c:173`, `focuser_asi/indigo_focuser_asi.c:557`, `rotator_asi/indigo_rotator_asi.c:189`, `ccd_asi/indigo_ccd_asi.c:254`, `guider_asi/indigo_guider_asi.c:89` | ZWO ASI-family connect/open paths call SDK enumeration/open/close APIs without the driver-global hot-plug mutex, so plug/unplug timers can race `EFW/EAF/CAA/ASI/USB2ST4` SDK global state during connect or disconnect. | Closed (fixed) |
| DRV-023 | High | `ccd_playerone/indigo_ccd_playerone.c:213`, `wheel_playerone/indigo_wheel_playerone.c:157` | PlayerOne camera and wheel connect paths call `POAOpen*` / properties APIs without the driver-global hot-plug mutex while plug/unplug handlers enumerate and temporarily open/close the same SDK under that mutex. | Closed (fixed) |
| DRV-024 | Medium | `ccd_fli/indigo_ccd_fli.c:145`, `focuser_fli/indigo_focuser_fli.c:152`, `wheel_fli/indigo_wheel_fli.c:122` | FLI connect paths use only per-device `usb_mutex` around `FLIOpen()` / `FLIClose()`, while hot-plug handlers protect `FLICreateList()` / `FLIList*()` / `FLIDeleteList()` with a separate driver-global mutex. | Closed (fixed) |
| DRV-025 | Medium | `ccd_svb/indigo_ccd_svb.c:194` | SVBONY normal connect calls `SVBOpenCamera()` with only the device `usb_mutex`, but hot-plug serializes SDK enumeration and temporary open/close with `device_mutex`. | Open |
| DRV-026 | Medium | `ccd_touptek/indigo_ccd_touptek.c:944` | ToupTek/OEM connect paths open devices with the vendor SDK without the hot-plug `mutex`, while the hot-plug refresh path enumerates devices and mutates shared presence state under that mutex. | Open |
| DRV-027 | Medium | `ccd_dsi/indigo_ccd_dsi.c:271` | Meade DSI connect opens the camera outside `device_mutex`, but plug/unplug scans and non-macOS temporary probe opens are serialized with `device_mutex`, leaving a scan/open race class. | Open |
| DRV-028 | Medium | `ccd_qsi/indigo_ccd_qsi.cpp:374` | QSI hot-plug uses the global `QSICamera cam` under `device_mutex`, but connect uses the same SDK object without that mutex for connect-time SDK calls. | Open |
| DRV-029 | High | `guider_asi/indigo_guider_asi.c:389` | `process_plug_event()` locks `indigo_device_enumeration_mutex` and never unlocks it on the successful attach path, so the first successful ASI USB-ST4 plug event can permanently block later plug/unplug enumeration. | Closed (fixed) |

## Finding Summaries

### DRV-001 (Closed — fixed)

The `CCD_MODE` update path in `indigo_alpaca_ccd.c` now clamps the import loop to
`ALPACA_MAX_ITEMS` before writing into the fixed-size `readoutmodes_labels` and
`readoutmodes_names` arrays, preventing an out-of-bounds write when a camera exposes
more than 128 readout modes.

The `alpaca_set_readoutmode()` guard was changed from `value > ALPACA_MAX_ITEMS` to
`value >= ALPACA_MAX_ITEMS`, so index 128 (one past the last valid slot) is now
correctly rejected with `InvalidValue` instead of triggering an out-of-bounds read.

### DRV-002 (Closed — fixed)

`wheel.count` is set from three sources in `indigo_alpaca_wheel_update_property()`:
`item->number.max` from `WHEEL_SLOT`, and `property->count` from both
`WHEEL_SLOT_OFFSET` and `WHEEL_SLOT_NAME`. All three assignments now clamp to
`ALPACA_MAX_FILTERS` (32) before storing. The `alpaca_get_names()` and
`alpaca_get_focusoffsets()` getters return `wheel.count` as the serialization
bound over 32-element fixed arrays, so an unclamped count could previously drive
iteration past the end of those arrays.

### DRV-003 (Closed — fixed)

The driver-filter builder in `indigo_agent_config.c` now uses a pointer/end-sentinel
pattern instead of repeated `strcat()`. A write pointer `p` and an `end` pointer
(`filter->text.value + INDIGO_VALUE_SIZE - 1`) bound every semicolon and name copy;
the outer loop exits as soon as the buffer is full, and `memcpy()` with a per-item
`avail` cap prevents any single name from writing past `end`. The string is
null-terminated after each copy so the buffer is always valid if iteration is
interrupted early.

### DRV-004 (Closed — fixed)

The `WHEEL_SLOT_NAME` handler in `indigo_agent_imager.c` now computes
`count = min(property->count, FILTER_SLOT_COUNT)` before assigning
`AGENT_WHEEL_FILTER_PROPERTY->count` and iterating over the label copy loop.
`AGENT_WHEEL_FILTER_PROPERTY` is allocated with exactly `FILTER_SLOT_COUNT` (24)
items, so a wheel advertising more than 24 slots previously wrote past the end of
the items array.

### DRV-005 (Closed — fixed)

The `AUX_OUTLET_NAMES` and `AUX_SENSOR_NAMES` handlers in `indigo_alpaca_switch.c`
now clamp `property->count` to `ALPACA_MAX_SWITCHES` (8) before the name-copy loop,
matching the clamp already applied by the value-update paths in the same function.
`switchname` is a flat `5 * ALPACA_MAX_SWITCHES` array indexed by `offset + i`; an
unclamped count lets `i` run past the end of the bank and into adjacent banks or
beyond the array.

### DRV-006 (Closed — fixed)

`strcpy(alpaca_device->device_name, item->text.value)` in `indigo_agent_alpaca.c`
replaced with `INDIGO_COPY_NAME()`. `device_name` is `INDIGO_NAME_SIZE` (128 bytes)
but INDIGO text values are `INDIGO_VALUE_SIZE` (512 bytes), so the unbounded copy
could overflow the cache field. `driver_info` and `driver_version` use the same
`strcpy` pattern but are declared `INDIGO_VALUE_SIZE` and were not overflowable.

### DRV-007 (Closed — fixed)

The `CMD_AID` custom-command handler in `indigo_dome_nexdome3.c` (line 1167) now
uses `snprintf(command, sizeof(command), "%s\n", …)` and checks the return value
against `sizeof(command)` (100 bytes). If the formatted string would be truncated,
`NEXDOME_COMMAND_PROPERTY` is set to `INDIGO_ALERT_STATE` and no data is sent to
the controller. A non-truncated command proceeds as before and the property is set
to `INDIGO_OK_STATE`.

### DRV-008 (Closed — fixed)

`focuser_name_handler()` in `indigo_focuser_steeldrive2.c` now uses `snprintf()`
instead of `sprintf()`. If the formatted result would meet or exceed the 64-byte
command buffer (i.e., the name is too long for the `$BS SET NAME:…` protocol
command), the handler sets `X_NAME_PROPERTY` to `INDIGO_ALERT_STATE` without
sending anything to the device, giving the user visible feedback that the name was
rejected rather than silently truncating or overflowing.

### DRV-009 (Closed — fixed)

`handle_xb()` in `indigo_dome_nexdome3.c` parsed the `XB->…` state token with
unbounded `%s` into `char state[20]`. Changed to `%19s` so `sscanf` writes at most
19 characters plus the null terminator, capping the write to exactly the size of
the destination buffer.

### DRV-010 (Closed — fixed)

All three `sscanf` calls in `ascol_parse_devname()` (`libascol.c:409–413`) now use
`%254s` instead of `%s`. The only call site passes `char host[255]`, so the field
width cap of 254 (plus the implicit null terminator) exactly fills that buffer
without overflowing it. A `DEVICE_PORT_ITEM` text value longer than
`"tcp://"`/`"ascol://"` + 254 characters is silently truncated at the host
extraction step, which is acceptable since `atoi()` on the remaining port string
will produce an invalid port and `ascol_open()` will fail with a connection error.

### DRV-011 (Closed — fixed)

Two fixes in `execute_command()` / the output-parsing loop in `indigo_agent_astrometry.c`:

- Line 205: `sprintf(command_buf, "%s 2>&1", buffer)` replaced with
  `snprintf(command_buf, sizeof(command_buf), "%s 2>&1", buffer)`. Both buffers are
  8 KiB; if `vsnprintf()` fills `buffer` to its limit, the six-byte `" 2>&1"` suffix
  would write past `command_buf`. `command_buf` is used only for the error-log
  message; truncation there is acceptable.

- Line 256: `%s` in `sscanf(line, "Field size: %lg x %lg %s", ...)` replaced with
  `%15s`. The destination `s` is `char s[16]`, so the field-width cap prevents an
  unbounded write from a malformed solver output line.

### DRV-012 (Closed — fixed)

Four categories of `sprintf()` replaced in `indigo_agent_astap.c`:

- **Line 331** (`execute_command`): `sprintf(command_buf, "%s 2>&1", buffer)` →
  `snprintf(command_buf, sizeof(command_buf), ...)`. Same overflow as DRV-011:
  `buffer` is 8 KiB, `command_buf` is 8 KiB, and the `" 2>&1"` suffix can exceed
  the destination if the source filled its own buffer.

- **Lines 432–434** (`base`/`file`/`ini`): three `sprintf()` calls building
  temporary-file paths from `base_dir` (up to 511 bytes) converted to `snprintf()`.

- **Lines 447–468** (`params` accumulation): seven incremental `sprintf(params +
  params_index, ...)` calls converted to `snprintf()` with a shared `params_avail`
  counter. Each append checks `params_avail > 1` before writing and updates
  `params_avail` afterward; the `-d "base_dir/item_name"` append (the highest-risk
  one) is guarded in the same way.

- **Lines 527 and 561** (`path`): two `sprintf(path, "%s/%s", base_dir, ...)` calls
  building index-directory paths converted to `snprintf(path, sizeof(path), ...)`.

### DRV-013 (Closed — fixed)

`gpsd_open()` in `indigo_gps_gpsd.c` now validates host and port lengths before
copying:

- No-colon path: `strlen(text) >= sizeof(host_name)` triggers an early `return false`
  with an error log, preventing the unbounded `strcpy()` into the 128-byte
  `host_name` buffer.
- Colon path: `colon - text >= sizeof(host_name)` rejects an oversized host segment
  before `strncpy()`, and `strlen(colon + 1) >= sizeof(port)` rejects an oversized
  port string before the second `strcpy()` into the 15-byte `port` buffer.

In both reject cases `gps_open()` is never called, so no partial or truncated
endpoint reaches the gpsd library.

### DRV-014 (Closed — fixed)

`synscan_save_position()` in `indigo_mount_synscan_driver.c` previously stored the
return value of `snprintf(buffer, …, "%s/.indigo", HOME)` as `path_end` and then
used `sprintf(buffer + path_end, "/synscan-…")` to append the filename. `snprintf`
returns the length that would have been written regardless of truncation, so a long
`HOME` value makes `path_end ≥ INDIGO_VALUE_SIZE` and `buffer + path_end` points
past the array.

The fix follows the pattern already used in `synscan_restore_position()` (line
1035): the directory path is built with one `snprintf()` (for `mkdir`), and the
complete park-file path is built with a second independent `snprintf()` using the
same `HOME` base, instead of indexing into the partially filled buffer.

### DRV-015 (Closed — fixed)

In `indigo_ptp.c`, the `ptp_str_type` branch of the property-refresh loop copied
camera-provided string switch values into `char str[INDIGO_NAME_SIZE]` (128 bytes)
with `strcpy()`, while the source `sw_str.values[i]` entries are `PTP_MAX_CHARS`
(256 bytes) wide. A value between 128 and 255 bytes would overflow `str` before it
was used to update the item name via `INDIGO_COPY_NAME`.

`strcpy(str, …)` replaced with `INDIGO_COPY_NAME(str, …)`, which clears `str` with
`memset` and copies at most `INDIGO_NAME_SIZE - 1` bytes, establishing truncation
as the explicit policy for overlong camera strings.

### DRV-016 (Closed — fixed)

The duplicate `agent_update_property()` forwarding branches for `JOYSTICK_MOUNT_*`
properties were removed. Updates for agent-owned mirror properties now go through
`snoop_changes()`, where `AGENT_MOUNT_ENABLE_JOYSTICK_CONTROL_ITEM->sw.value` gates all
joystick-driven mount actions. The RA east motion branch in `snoop_changes()` was also
corrected to update `MOUNT_MOTION_RA`/`MOUNT_MOTION_EAST`.

### DRV-017 (Closed — fixed)

The new `AGENT_PROCESS_FEATURES` items for dome slaving, frame derotation, and joystick
control now default to disabled. A saved `AGENT_PROCESS_FEATURES` configuration still
overrides these defaults when loaded, but upgrades from configurations containing only the
old `AGENT_DOME_SLAVING`/`AGENT_FIELD_DEROTATION` properties no longer enable hardware
actions implicitly.

### DRV-018 (Closed — fixed)

The uncommitted `agent_mount` change starts dome park/unpark in parallel with mount
park/unpark when dome slaving is enabled. The previous flow parked or unparked the dome
only after the mount operation reached the expected final state. Starting both devices at
once means a mount failure or early abort can still leave the dome/roof moving, while the
agent's abort path only sends `MOUNT_ABORT_MOTION`.

The abort path now sends `DOME_ABORT_MOTION` as well when a dome is selected, so a user
abort during a combined mount/dome process attempts to stop both controlled devices.

### DRV-019 (Closed — fixed)

`ptp_olympus_initialise()` treats a failed `SetDevicePropValue(CameraControlMode)` as
recoverable, which is needed for the OM-1 mode-switch timeout case, but the implementation
does not require either a C108 confirmation event or a successful `ptp_olympus_recover()`.
The function then schedules `ptp_olympus_check_event()` and returns `true`. For a camera
that was unplugged during the switch, remains wedged after reset, or matches the Olympus
wildcard without supporting the OM PC-control extension, the driver can publish an OK
connection even though `ptp_olympus_exposure()` and `ptp_olympus_liveview()` depend on
that mode.

The Olympus init path now accepts the expected raw-USB timeout workaround only after the
camera-control property-change event is observed and `ptp_olympus_recover()` succeeds. If
the switch is unconfirmed, recovery fails, or the ICA transport reports a direct switch
failure, initialization returns `false` and the normal connection error path closes the
PTP session instead of reporting the camera connected.

### DRV-020 (Closed — fixed)

`FILTER_MOUNT_LIST` deselection now clears both `DOME_SLAVING` and
`FIELD_DEROTATION` along with the mount operation lights before updating
`AGENT_MOUNT_STATE`, so no slaving state remains visible after the selected mount is
removed.

The rotator deselection branch now calls `set_slaving_lights()` for
`FIELD_DEROTATION` after setting `selected_rotator_index = 0` and
`rotator_position_state = INDIGO_IDLE_STATE`, mirroring the existing dome deselection
cleanup for `DOME_SLAVING`.

### DRV-021 (Closed — fixed)

The periodic `handle_mount_change()` slaving path now checks dependent-device
`INDIGO_ALERT_STATE` before issuing autonomous corrections. If
`DOME_HORIZONTAL_COORDINATES` is alert, `AGENT_MOUNT_STATE_DOME_SLAVING_ITEM` is set
to `INDIGO_ALERT_STATE` and no new dome azimuth command is sent from that pass.

The field derotation path now does the same for `ROTATOR_POSITION`: while the rotator
position property is alert, `AGENT_MOUNT_STATE_FIELD_DEROTATION_ITEM` is set to
`INDIGO_ALERT_STATE` and no autonomous derotation command is issued. The light is only
returned to `OK` by the non-alert correction path.

### DRV-022 (Closed)

The ZWO SDK-backed hot-plug drivers serialize plug/unplug enumeration with a driver-global mutex, but normal connect/open and disconnect/close paths use only per-device `usb_mutex` locks:

- `wheel_asi`: `wheel_connect_callback()` calls `find_index_by_device_id()` (`EFWGetNum()` / `EFWGetID()`), `EFWOpen()`, initial property reads, and `EFWClose()` outside `indigo_device_enumeration_mutex`, while `process_plug_event()` / `process_unplug_event()` hold that mutex around `EFWGetNum()` / `EFWGetID()` / temporary `EFWOpen()` / `EFWClose()`.
- `focuser_asi`: same pattern for `EAFGetNum()` / `EAFGetID()` / `EAFOpen()` / `EAFClose()` in the USB path. Bluetooth paths are separate and not part of this finding.
- `rotator_asi`: same pattern for `CAAGetNum()` / `CAAGetID()` / `CAAOpen()` / `CAAClose()`.
- `ccd_asi`: hot-plug holds `indigo_device_enumeration_mutex` while enumerating cameras and temporarily opening/closing one camera for ID/serial data, but `asi_open()` / `asi_close()` call `ASIOpenCamera()` / `ASIInitCamera()` / `ASICloseCamera()` without that mutex.
- `guider_asi`: hot-plug holds `indigo_device_enumeration_mutex` around `USB2ST4GetNum()` / `USB2ST4GetID()`, but `asi_open()` / `asi_close()` call `USB2ST4Open()` / `USB2ST4Close()` without it.

The ASI wheel crash log showed this exact shape: a connect callback entered SDK enumeration while hot-plug/unplug callbacks were also active, and the closed SDK dereferenced invalid internal state. These drivers should use one driver-global SDK mutex for enumeration/open/close paths, or otherwise prove the vendor SDK calls are reentrant.

Fixed by reusing each driver's hot-plug mutex around the normal USB enumeration/open/initial-read/close path, with CCD and guider hot-unplug detaching outside that mutex to avoid self-deadlock when detach invokes close.

### DRV-023 (Closed)

The PlayerOne camera and wheel drivers have the same split-lock shape:

- `ccd_playerone`: `process_plug_event()` holds `indigo_device_enumeration_mutex` while calling `POAGetCameraCount()`, `POAGetCameraProperties()`, temporary `POAOpenCamera()`, and `POACloseCamera()`, but `playerone_open()` / `playerone_close()` use only `PRIVATE_DATA->usb_mutex` around `POAOpenCamera()` / `POAInitCamera()` / `POACloseCamera()`.
- `wheel_playerone`: `process_plug_event()` holds `indigo_device_enumeration_mutex` around `POAGetPWCount()`, `POAGetPWProperties()`, temporary `POAOpenPW()`, and `POAClosePW()`, but `wheel_connect_callback()` calls `POAGetPWPropertiesByHandle()`, `POAOpenPW()`, and `POAClosePW()` outside that mutex.

If the PlayerOne SDK has global enumeration/open state like the ZWO SDK, hot-plug timers can race normal connect/disconnect.

Fixed by reusing each driver's hot-plug mutex around the normal camera/wheel open/initial-property/close paths, with hot-unplug detaching devices outside that mutex to avoid self-deadlock when detach invokes close.

### DRV-024 (Closed)

The FLI CCD, focuser, and wheel drivers protect hot-plug enumeration with a driver-global mutex, but connect paths open and close devices under only per-device `usb_mutex`:

- `ccd_fli`: `fli_open()` calls `FLIOpen()` and error-path `FLIClose()` while hot-plug uses `indigo_device_enumeration_mutex` around `FLICreateList()` / `FLIListFirst()` / `FLIListNext()` / `FLIDeleteList()`.
- `focuser_fli`: `fli_focuser_connect()` calls `FLIOpen()` outside `indigo_device_enumeration_mutex`, while hot-plug enumeration is serialized with `indigo_device_enumeration_mutex`.
- `wheel_fli`: `wheel_connect_callback()` calls `find_index_by_device_fname()` against the shared enumerated arrays and then `FLIOpen()` / `FLIClose()` outside `indigo_device_enumeration_mutex`, while hot-plug updates those arrays under that mutex.

The risk is lower confidence than the ZWO finding because it depends on libfli's internal reentrancy, but the driver-level locking suggests enumeration is already considered global state.

Fixed by reusing each driver's hot-plug mutex around normal `FLIOpen()` / `FLIClose()` paths, including the wheel's shared enumerated file-name lookup. Hot-unplug detaches devices outside that mutex to avoid self-deadlock when detach invokes close.

### DRV-025 (Open)

`ccd_svb` uses `device_mutex` in hot-plug paths around `SVBGetNumOfConnectedCameras()`, `SVBGetCameraInfo()`, temporary `SVBOpenCamera()`, property/probing calls, and `SVBCloseCamera()`. Normal `svb_open()` / close paths use only `PRIVATE_DATA->usb_mutex` around `SVBOpenCamera()` and later `SVBCloseCamera()`. A connect racing with an arrival/removal timer can therefore overlap SDK enumeration and open/close state.

### DRV-026 (Open)

`ccd_touptek` uses a driver-global `mutex` for `process_plug_event()` while refreshing the device inventory with `SDK_CALL(EnumV2)` and updating shared `devices[]` / `present` state. The CCD, guider, wheel, and focuser connect callbacks call `SDK_CALL(Open)` directly from normal connect paths without taking that mutex. If the vendor hot-plug callback schedules a refresh during user connect/disconnect, SDK enumeration and open can overlap.

### DRV-027 (Open)

`ccd_dsi` hot-plug uses `device_mutex` around `dsi_scan_usb()` and, on non-macOS paths, a temporary `dsi_open_camera()` used to name/probe the camera. Normal connect enters `camera_open(device)` outside `device_mutex`. That leaves the same scan/open race class as the ASI wheel issue, even though macOS avoids the temporary open inside plug handling because it can reset the device.

### DRV-028 (Open)

`ccd_qsi` uses a single global `QSICamera cam` object. `process_plug_event()` protects `cam.get_AvailableCameras()` with `device_mutex`, but `ccd_connect_callback()` uses the same `cam` object for `get_Connected()`, `get_SelectCamera()`, and the subsequent connect sequence without holding `device_mutex`. Because the SDK object is shared across all QSI devices, hot-plug enumeration can race connect-time SDK state.

### DRV-029 (Closed)

`guider_asi` has a direct mutex leak in `process_plug_event()`: it locks `indigo_device_enumeration_mutex`, handles error returns correctly, but the successful path attaches the new guider and stores it in `devices[slot]` without unlocking before returning. After the first successful plug event, later ASI USB-ST4 plug/unplug handlers block forever on the same mutex. The fix is a straightforward unlock on the success path, plus considering the DRV-022 serialization fix for normal open/close.

Fixed by unlocking `indigo_device_enumeration_mutex` on the successful attach path.

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
| `d9b39b84e3780dca0c9e7cbb901b63a62586b106` | `afdd54618e5520c4983598c33b662c022962df7c` | 2026-08-18 | Requested review of the last two commits touching `agent_mount`; recorded `DRV-016` and `DRV-017`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `a18baada350fd21298fc602fd1751518cc8254ba` | 2026-08-22 | Focused review of Olympus/OM System support under `ccd_ptp`; recorded `DRV-019`. Did not advance the folder baseline because other `indigo_drivers` changes in this range were not reviewed. |
| `017ba602857378e4aed489c065c76eacae15924c` | `f8b7086ebdd408c34366a8acdac27e6311103911` + working tree | 2026-08-25 | Focused review of `agent_mount/indigo_agent_mount.c` `AGENT_MOUNT_STATE_DOME_SLAVING` and `AGENT_MOUNT_STATE_FIELD_DEROTATION` usage; recorded `DRV-020` and `DRV-021`. Did not advance the folder baseline because the rest of `indigo_drivers` was not reviewed. |
| `017ba602857378e4aed489c065c76eacae15924c` | working tree | 2026-08-31 | Focused review of active hot-plug driver SDK enumeration/open/close serialization under `indigo_drivers`; recorded `DRV-022` through `DRV-029`. Did not advance the folder baseline because the rest of `indigo_drivers` was not reviewed. |
