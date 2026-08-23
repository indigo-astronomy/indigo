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
| DRV-001 | High | `agent_alpaca/indigo_alpaca_ccd.c:946` | `CCD_MODE` items are copied into `readoutmodes_labels` and `readoutmodes_names` by raw `property->count` index, but the arrays are only `ALPACA_MAX_ITEMS` long. The setter also accepts `value == ALPACA_MAX_ITEMS`, causing an out-of-bounds read. Clamp mode import to `ALPACA_MAX_ITEMS` and reject `value >= ALPACA_MAX_ITEMS`. | Closed (fixed) |
| DRV-002 | Medium | `agent_alpaca/indigo_alpaca_wheel.c:107` | Wheel slot-offset/name updates store `property->count` as `wheel.count` even though Alpaca wheel arrays are limited to `ALPACA_MAX_FILTERS`. The response paths then serialize `count` entries from fixed 32-element arrays, so a driver exposing more items can read beyond the arrays. Clamp exported count to `ALPACA_MAX_FILTERS`. | Closed (fixed) |
| DRV-003 | Medium | `agent_config/indigo_agent_config.c:686` | Agent configuration builds a semicolon-separated driver filter with repeated `strcat()` into one `INDIGO_VALUE_SIZE` text item. A server exposing enough selected drivers can overflow the filter string. Build with remaining-capacity checks or truncate safely. | Closed (fixed) |
| DRV-004 | Medium | `agent_imager/indigo_agent_imager.c:3485` | The imager agent mirrors wheel slot names by assigning `AGENT_WHEEL_FILTER_PROPERTY->count = property->count`, but the property was initialized with `FILTER_SLOT_COUNT` items. A wheel with more than 24 slots can write past the property items when copying labels. Clamp to `FILTER_SLOT_COUNT` or resize the property before copying. | Closed (fixed) |
| DRV-005 | Medium | `agent_alpaca/indigo_alpaca_switch.c:564` | Alpaca switch-name handling copies `AUX_OUTLET_NAMES` and `AUX_SENSOR_NAMES` with raw `property->count` into fixed `5 * ALPACA_MAX_SWITCHES` storage. The value paths clamp each bank to 8 items, but the name paths do not, so a property with too many names can write past the selected bank. Clamp each name bank to `ALPACA_MAX_SWITCHES`. | Closed (fixed) |
| DRV-006 | Medium | `agent_alpaca/indigo_agent_alpaca.c:670` | `INFO_DEVICE_NAME` text is copied with `strcpy()` into `device_name[INDIGO_NAME_SIZE]`, while INDIGO text values are `INDIGO_VALUE_SIZE`. A long device-name text item can overflow the Alpaca device-name cache. Use `INDIGO_COPY_NAME()` or another bounded copy. | Closed (fixed) |
| DRV-007 | Medium | `dome_nexdome3/indigo_dome_nexdome3.c:1203` | The optional NexDome custom command path formats `NEXDOME_COMMAND_ITEM->text.value` into `char command[NEXDOME_CMD_LEN]` with `sprintf()`. The text value can be much larger than the 100-byte command buffer, so a long custom command overflows the stack buffer. Use `snprintf()` and reject/truncate oversized commands. | Open |
| DRV-008 | Medium | `focuser_steeldrive2/indigo_focuser_steeldrive2.c:642` | SteelDrive2 formats the user-editable `X_NAME` text item into `char command[64]` with `sprintf("$BS SET NAME:%s", ...)`. `X_NAME_ITEM->text.value` is an INDIGO text value, so a long name can overflow the command buffer before it is sent. Bound the accepted name length or use `snprintf()` with state feedback on truncation. | Closed (fixed) |
| DRV-009 | Medium | `dome_nexdome3/indigo_dome_nexdome3.c:567` | NexDome3 parses incoming `XB->...` messages with `sscanf(message, "XB->%s", state)` into `char state[20]`. A malformed or unexpectedly long controller message can overflow the stack buffer before the value is copied to the INDIGO text item. Add a field width or parse with bounded copying. | Open |
| DRV-010 | Medium | `system_ascol/libascol/libascol.c:405` | `ascol_parse_devname()` parses `DEVICE_PORT_ITEM->text.value` with unbounded `%s` into the caller's `host` buffer; the system driver passes `char host[255]`, while the INDIGO text value can be larger. A long `tcp://...` or `ascol://...` value can overflow `host`. Use width-limited parsing or pass the destination size. | Open |
| DRV-011 | Medium | `agent_astrometry/indigo_agent_astrometry.c:205` | The Astrometry agent builds `buffer[8192]` with `vsnprintf()` and then appends `" 2>&1"` into another 8192-byte buffer with `sprintf()`. A command truncated to the full source buffer can overflow `command_buf`; the same file also parses `Field size` units into `char s[16]` with unbounded `%s`. Use a single bounded `snprintf()` and width-limited parsing. | Closed (fixed) |
| DRV-012 | Medium | `agent_astap/indigo_agent_astap.c:331` | The ASTAP agent has the same equal-sized `buffer` to `command_buf` append overflow as Astrometry. It also builds index parameters and index paths with repeated `sprintf()` into 512-byte buffers using `base_dir`, which can be near the buffer limit. Convert command, parameter, and path construction to checked `snprintf()` with remaining-capacity tracking. | Closed (fixed) |
| DRV-013 | Medium | `gps_gpsd/indigo_gps_gpsd.c:59` | `gpsd_open()` copies the editable device-port text into `host_name[INDIGO_NAME_SIZE]` and `port[15]` with `strcpy()`/`strncpy()` without checking host or port length. A long `gpsd://...` value can overflow the host or port buffer before `gps_open()`. Parse with bounded lengths and reject invalid endpoints. | Closed (fixed) |
| DRV-014 | Medium | `mount_synscan/indigo_mount_synscan_driver.c:1013` | `synscan_save_position()` writes the HOME-based `.indigo` path with `snprintf()` but then appends the park filename using `sprintf(buffer + path_end, ...)`. If HOME is long enough for `snprintf()` to truncate, `path_end` is the would-have-written length and can point past `buffer`. Compose the complete path with one checked `snprintf()`. | Open |
| DRV-015 | Medium | `ccd_ptp/indigo_ptp.c:1533` | PTP string switch values are decoded into `PTP_MAX_CHARS` 256-byte entries, but refreshed property names are copied into `char str[INDIGO_NAME_SIZE]` with `strcpy()`. A camera-provided string value longer than 127 bytes can overflow `str` before the item name is updated. Use bounded copying and define a deterministic truncation or rejection policy for item names. | Open |
| DRV-016 | High | `agent_mount/indigo_agent_mount.c:2172` | `AGENT_MOUNT_ENABLE_JOYSTICK_CONTROL` was ignored by the `agent_update_property()` forwarding path. `JOYSTICK_MOUNT_*` updates were forwarded to the selected mount before the gated joystick handling in `snoop_changes()` could run, so disabling joystick control in `AGENT_PROCESS_FEATURES` did not prevent joystick motion, park, tracking, home, or abort commands. | Closed (fixed) |
| DRV-017 | High | `agent_mount/indigo_agent_mount.c:1840` | The refactor removed the old disabled-by-default `AGENT_DOME_SLAVING` and `AGENT_FIELD_DEROTATION` properties, then initialized the replacement `AGENT_PROCESS_FEATURES` items for dome slaving, derotation, and joystick control to `true`. Existing configurations saved under the old property names were no longer loaded into these new items, so upgrading could silently enable dome, rotator, and joystick-driven hardware behavior that was previously disabled. | Closed (fixed) |
| DRV-018 | High | `agent_mount/indigo_agent_mount.c:521` | Mount park/unpark with dome slaving now sends `DOME_PARK` immediately after `MOUNT_PARK`, instead of waiting for the mount park state to complete successfully. If the mount park later fails or the process is aborted, dome/roof park motion has already been started and `abort_process()` only sends `MOUNT_ABORT_MOTION`, creating a hardware-safety regression. Preserve the previous sequencing or abort/guard dome motion explicitly. | Closed (fixed) |
| DRV-019 | Medium | `ccd_ptp/indigo_ptp_olympus.c:696` | Olympus initialization logs a failed `CameraControlMode` switch and calls raw-USB recovery, but ignores missing confirmation and recovery failure before scheduling event polling and returning success. A disconnected, wedged, or wildcard-matched unsupported Olympus body can be reported connected even though remote capture and live view require PC-control mode. | Closed (fixed) |

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

### DRV-008 (Closed — fixed)

`focuser_name_handler()` in `indigo_focuser_steeldrive2.c` now uses `snprintf()`
instead of `sprintf()`. If the formatted result would meet or exceed the 64-byte
command buffer (i.e., the name is too long for the `$BS SET NAME:…` protocol
command), the handler sets `X_NAME_PROPERTY` to `INDIGO_ALERT_STATE` without
sending anything to the device, giving the user visible feedback that the name was
rejected rather than silently truncating or overflowing.

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
